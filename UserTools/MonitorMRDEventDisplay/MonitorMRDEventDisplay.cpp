#include "MonitorMRDEventDisplay.h"

MonitorMRDEventDisplay::MonitorMRDEventDisplay():Tool(){}


bool MonitorMRDEventDisplay::Initialise(std::string configfile, DataModel &data){

	//-------------------------------------------------------------------------------
	//----------------------- Useful header -----------------------------------------
	//-------------------------------------------------------------------------------

	if(configfile!="")  m_variables.Initialise(configfile);
	m_data= &data;

	//-------------------------------------------------------
	//-----------------Get Configuration---------------------
	//-------------------------------------------------------

  m_variables.Get("verbose",verbosity);
  m_variables.Get("OutputPath",outpath);
  m_variables.Get("ActiveSlots",active_slots);
  m_variables.Get("InActiveChannels",inactive_channels);

  if (verbosity > 2) std::cout <<"MonitorMRDEventDisplay: Initialising"<<std::endl;

  if (outpath == "fromStore") m_data->CStore.Get("OutPath",outpath);
  if (verbosity > 1) std::cout <<"Output path for plots is "<<outpath<<std::endl;


  //get objects with allocated memory in ROOT
  //std::cout <<"List of Objects (Start of Initialise)"<<std::endl;
  //gObjectTable->Print();

  //-------------------------------------------------------
  //-----------------Get active channels-------------------
  //-------------------------------------------------------

  ifstream file(active_slots.c_str());
  int temp_crate, temp_slot;
  int loopnum=0;
  num_active_slots=0;
  num_active_slots_cr1=0;
  num_active_slots_cr2=0;

  while (!file.eof()){
    file>>temp_crate>>temp_slot;
    loopnum++;
    if (verbosity > 2) std::cout<<loopnum<<" , "<<temp_crate<<" , "<<temp_slot<<std::endl;
    if (file.eof()) break;
    //if (loopnum!=13){
      if (temp_crate-min_crate<0 || temp_crate-min_crate>=num_crates) {
        std::cout <<"ERROR (MonitorMRDEventDisplay): Specified crate out of range [7...8]. Continue with next entry."<<std::endl;
        continue;
      }
      if (temp_slot<1 || temp_slot>num_slots){
        std::cout <<"ERROR (MonitorMRDEventDisplay): Specified slot out of range [1...24]. Continue with next entry."<<std::endl;
        continue;
      }
      active_channel[temp_crate-min_crate][temp_slot-1]=1;		//slot start with number 1 instead of 0, crates with 7
      nr_slot.push_back((temp_crate-min_crate)*100+(temp_slot)); 
      num_active_slots++;
      if (temp_crate-min_crate==0) {num_active_slots_cr1++;active_slots_cr1.push_back(temp_slot);}
      if (temp_crate-min_crate==1) {num_active_slots_cr2++;active_slots_cr2.push_back(temp_slot);}
    //}
  }
  file.close();

  //-------------------------------------------------------
  //-------------Get inactive channels---------------------
  //-------------------------------------------------------

  ifstream file_inactive(inactive_channels.c_str());
  int temp_channel;

  while (!file_inactive.eof()){
    file_inactive>>temp_crate>>temp_slot>>temp_channel;
    if (verbosity > 2) std::cout<<temp_crate<<" , "<<temp_slot<<" , "<<temp_channel<<std::endl;
    if (file_inactive.eof()) break;

      if (temp_crate-min_crate<0 || temp_crate-min_crate>=num_crates) {
        std::cout <<"ERROR (MonitorMRDEventDisplay): Specified crate out of range [7...8]. Continue with next entry."<<std::endl;
        continue;
      }
      if (temp_slot<1 || temp_slot>num_slots){
        std::cout <<"ERROR (MonitorMRDEventDisplay): Specified slot out of range [1...24]. Continue with next entry."<<std::endl;
        continue;
      }
      if (temp_channel<0 || temp_channel > num_channels){
	std::cout <<"ERROR (MonitorMRDEventDisplay): Specified channel out of range [0...31]. Continue with next entry."<<std::endl;
        continue;
      }

	if (temp_crate == min_crate){
      	inactive_ch_crate1.push_back(temp_channel);
      	inactive_slot_crate1.push_back(temp_slot);
      } else if (temp_crate == min_crate+1){
      	inactive_ch_crate2.push_back(temp_channel);
      	inactive_slot_crate2.push_back(temp_slot);
      } else {
      	std::cout <<"ERROR (MonitorMRDEventDisplay): Crate # out of range, entry ("<<temp_crate<<"/"<<temp_slot<<"/"<<temp_channel<<") not added to inactive channel configuration." <<std::endl;
      }
    
  }
  file_inactive.close();

  //-------------------------------------------------------
  //----------Initialize storing containers----------------
  //-------------------------------------------------------

  MonitorMRDEventDisplay::InitializeVectors();

  //-------------------------------------------------------
  //------Setup time variables for periodic updates--------
  //-------------------------------------------------------

  period_update = boost::posix_time::time_duration(0,0,30,0);	//set update frequency for this tool to be higher than MonitorMRDTime (5mins->30sec)
  last = boost::posix_time::ptime(boost::posix_time::second_clock::local_time());


	return true;
}


bool MonitorMRDEventDisplay::Execute(){

	if (verbosity > 2) std::cout <<"MonitorMRDEventDisplay: Executing"<<std::endl;

  //-------------------------------------------------------
  //---------------How much time passed?-------------------
  //-------------------------------------------------------

	current = (boost::posix_time::second_clock::local_time());
	duration = boost::posix_time::time_duration(current - last); 

  //-------------------------------------------------------
  //-----------------Get MRD Live data---------------------
  //-------------------------------------------------------

	std::string State;
	m_data->CStore.Get("State",State);

	if (State == "MRDSingle"){
		//update live vectors

		m_data->Stores["CCData"]->Get("Single",MRDout); 

    	OutN = MRDout.OutN;
    	Trigger = MRDout.Trigger;
    	Value = MRDout.Value;
    	Slot = MRDout.Slot;
    	Channel = MRDout.Channel;
    	Crate = MRDout.Crate;
    	TimeStamp = MRDout.TimeStamp;

    	if (verbosity > 2) std::cout <<"Read in Data: >>>>>>>>>>>>>>>>>>> "<<std::endl;
    	for (int i_entry = 0; i_entry < Channel.size(); i_entry++){

    		if (verbosity > 2) std::cout <<"Crate "<<Crate.at(i_entry)<<", Slot "<<Slot.at(i_entry)<<", Channel "<<Channel.at(i_entry)<<std::endl;
    		
    		std::vector<int>::iterator it = std::find(nr_slot.begin(), nr_slot.end(), (Slot.at(i_entry))+(Crate.at(i_entry)-min_crate)*100);
			if (it == nr_slot.end()){
        		std::cout <<"ERROR (MonitorMRDEventDisplay): Read-out Crate/Slot/Channel number not active according to configuration file. Check the configfile to process the data..."<<std::endl;
        		std::cout <<"Crate: "<<Crate.at(i_entry)<<", Slot: "<<Slot.at(i_entry)<<std::endl;
        		continue;
      		}
		int active_slot_nr = std::distance(nr_slot.begin(),it);
    		int ch = active_slot_nr*num_channels+Channel.at(i_entry);
    		if (verbosity > 2) std::cout <<", ch nr: "<<ch<<", TDC: "<<Value.at(i_entry)<<", timestamp: "<<TimeStamp<<std::endl;
    		//std::cout <<"append TDC to live_tdc"<<std::endl;
    		live_tdc.at(ch).push_back(Value.at(i_entry));
    		//std::cout <<"append timestamp to live_timestamp"<<std::endl;
    		live_timestamp.at(ch).push_back(TimeStamp);
        live_tdc_hour.at(ch).push_back(Value.at(i_entry));
        live_timestamp_hour.at(ch).push_back(TimeStamp);

    	}

    	vector_timestamp.push_back(TimeStamp);
    	vector_nchannels.push_back(Channel.size());

    	current_stamp = TimeStamp;

    	Value.clear();
    	Slot.clear();
    	Channel.clear();
    	Crate.clear();

    	MRDout.Value.clear();
    	MRDout.Slot.clear();
    	MRDout.Channel.clear();
    	MRDout.Crate.clear();
    	MRDout.Type.clear();

	}else if (State == "DataFile" || State == "Wait"){
		//do nothing
		if (verbosity > 2) std::cout <<"MonitorMRDEventDisplay: State is "<<State<<", do nothing"<<std::endl;
	}else {
		std::cout <<"ERROR (MonitorMRDEventDisplay): State not recognized! Please check data format of MRD file."<<std::endl;
	}

  //-------------------------------------------------------
  //-----------------Create plots--------------------------
  //-------------------------------------------------------

	if (verbosity > 1) std::cout <<"current: "<<current<<", last: "<<last<<", duration: "<<duration<<std::endl;

  	if(duration>=period_update){

	    if (verbosity > 0) std::cout <<"MRDMonitorEventDisplay: 30sec passed... Updating plots!"<<std::endl;
	    
	    update_plots=true;
	    last=current;
	    MonitorMRDEventDisplay::EraseOldData();
	    MonitorMRDEventDisplay::UpdateMonitorPlots();

	}

	return true;
}


bool MonitorMRDEventDisplay::Finalise(){

	if (verbosity > 2) std::cout <<"MonitorMRDEventDisplay: Finalising"<<std::endl;

	MRDdata->Delete();

	//delete all the pointer to objects that are still active
	delete rate_crate1;
	delete rate_crate2;
	delete rate_crate1_hour;
	delete rate_crate2_hour;
	delete TDC_hist;
	delete TDC_hist_hour;
	delete TDC_hist_coincidence;
	delete n_paddles_hit;
	delete n_paddles_hit_hour;
	delete n_paddles_hit_coincidence;

	return true;
}

void MonitorMRDEventDisplay::EraseOldData(){

	if (verbosity > 2) std::cout <<"MonitorMRDEventDisplay: EraseOldData"<<std::endl;

	for (int i_ch = 0; i_ch < num_active_slots*num_channels; i_ch++){
		for (int i=0; i<live_tdc.at(i_ch).size();i++){
			if (current_stamp - live_timestamp.at(i_ch).at(i) > integration_period*60*1000){
				live_tdc.at(i_ch).erase(live_tdc.at(i_ch).begin()+i);
				live_timestamp.at(i_ch).erase(live_timestamp.at(i_ch).begin()+i);
			}
		}
		for (int i=0; i<live_tdc_hour.at(i_ch).size();i++){
			if (current_stamp - live_timestamp_hour.at(i_ch).at(i) > integration_period2*60*1000){
				live_tdc_hour.at(i_ch).erase(live_tdc_hour.at(i_ch).begin()+i);
				live_timestamp_hour.at(i_ch).erase(live_timestamp_hour.at(i_ch).begin()+i);
			}
		}
	}
	
	for (int i_entry = 0; i_entry < vector_timestamp.size(); i_entry++){
		if (current_stamp - vector_timestamp.at(i_entry) > integration_period*60*1000){
			vector_timestamp.erase(vector_timestamp.begin()+i_entry);
			vector_nchannels.erase(vector_nchannels.begin()+i_entry);
		}
	}

	for (int i_entry = 0; i_entry < vector_timestamp_hour.size(); i_entry++){
		if (current_stamp - vector_timestamp_hour.at(i_entry) > integration_period2*60*1000){
			vector_timestamp_hour.erase(vector_timestamp_hour.begin()+i_entry);
			vector_nchannels_hour.erase(vector_nchannels_hour.begin()+i_entry);
		}
	}

}

void MonitorMRDEventDisplay::InitializeVectors(){

	if (verbosity > 2) std::cout <<"MonitorMRDEventDisplay: InitializeVectors"<<std::endl;

	std::vector<unsigned int> vec_tdc, vec_tdc_hour;
	std::vector<ULong64_t> vec_timestamp, vec_timestamp_hour;
	for (int i_ch = 0; i_ch < num_active_slots*num_channels; i_ch++){
		live_tdc.push_back(vec_tdc);
		live_timestamp.push_back(vec_timestamp);
		live_tdc_hour.push_back(vec_tdc_hour);
		live_timestamp_hour.push_back(vec_timestamp_hour);
	}

}

void MonitorMRDEventDisplay::UpdateMonitorPlots(){

	if (verbosity > 2) std::cout <<"MonitorMRDEventDisplay: UpdateMonitorPlots"<<std::endl;

    t = time(0);
    struct tm *now = localtime( & t );
    title_time.str("");
    title_time<<(now->tm_year + 1900) << '-' << (now->tm_mon + 1) << '-' <<  now->tm_mday<<','<<now->tm_hour<<':'<<now->tm_min<<':'<<now->tm_sec;

    min_ch = 99999999;
    max_ch = 0;

  	rate_crate1 = new TH2F("rate_crate1","Rates Crate 7",num_slots,0,num_slots,num_channels,0,num_channels);
	rate_crate2 = new TH2F("rate_crate2","Rates Crate 8",num_slots,0,num_slots,num_channels,0,num_channels);
	TDC_hist = new TH1F("TDC_hist","TDC",180,0,180);
	TDC_hist_coincidence = new TH1F("TDC_hist_coincidence","TDC (coincident)",180,0,180);
	n_paddles_hit = new TH1F("n_paddles_hit","N Paddles",100,1,0);
	n_paddles_hit_coincidence = new TH1F("n_paddles_hit_coincidence","N Paddles (coincident)",15,0,15);

	rate_crate1->SetStats(0);
	rate_crate1->GetXaxis()->SetNdivisions(num_slots);
	rate_crate1->GetYaxis()->SetNdivisions(num_channels);
	for (int i_label=0;i_label<int(num_channels);i_label++){
		std::stringstream ss_slot, ss_ch;
		ss_slot<<(i_label+1);
		ss_ch<<(i_label);
		std::string str_ch = "ch "+ss_ch.str();
		rate_crate1->GetYaxis()->SetBinLabel(i_label+1,str_ch.c_str());
		if (i_label<num_slots){
			std::string str_slot = "slot "+ss_slot.str();
			rate_crate1->GetXaxis()->SetBinLabel(i_label+1,str_slot.c_str());
		}
	}
	rate_crate1->LabelsOption("v");

	rate_crate2->SetStats(0);
	rate_crate2->GetXaxis()->SetNdivisions(num_slots);
	rate_crate2->GetYaxis()->SetNdivisions(num_channels);
	for (int i_label=0;i_label<int(num_channels);i_label++){
		std::stringstream ss_slot, ss_ch;
		ss_slot<<(i_label+1);
		ss_ch<<(i_label);
		std::string str_ch = "ch "+ss_ch.str();
		rate_crate2->GetYaxis()->SetBinLabel(i_label+1,str_ch.c_str());
		if (i_label<num_slots){
			std::string str_slot = "slot "+ss_slot.str();
			rate_crate2->GetXaxis()->SetBinLabel(i_label+1,str_slot.c_str());
		}
	}
	rate_crate2->LabelsOption("v");


	TDC_hist->GetXaxis()->SetTitle("TDC");
	TDC_hist->GetYaxis()->SetTitle("#");
	TDC_hist_coincidence->GetXaxis()->SetTitle("TDC");
	TDC_hist_coincidence->GetYaxis()->SetTitle("#");
	n_paddles_hit->GetXaxis()->SetTitle("# paddles hit");
	n_paddles_hit->GetYaxis()->SetTitle("#");
	n_paddles_hit_coincidence->GetXaxis()->SetTitle("# paddles hit");
	n_paddles_hit_coincidence->GetYaxis()->SetTitle("#");

    std::string tdc_title = "TDC ";
    std::string tdc_title_coincident = "TDC (coincident) ";
    std::string n_paddles_title = "N Paddles ";
    std::string n_paddles_coincidence_title = "N Paddles (coincident) ";
    std::string tdc_Title = tdc_title+title_time.str();
    std::string tdc_Title_coincident = tdc_title_coincident+title_time.str();
    std::string n_paddles_Title = n_paddles_title+title_time.str();
    std::string n_paddles_coincidence_Title = n_paddles_coincidence_title+title_time.str();

    TDC_hist->SetTitle(tdc_Title.c_str());
    TDC_hist_coincidence->SetTitle(tdc_Title_coincident.c_str());
    n_paddles_hit->SetTitle(n_paddles_Title.c_str());
    n_paddles_hit_coincidence->SetTitle(n_paddles_coincidence_Title.c_str());

    for (int i_ch = 0; i_ch < num_active_slots*num_channels; i_ch++){

    	int crate_id = (i_ch < num_active_slots_cr1*num_channels)? min_crate : min_crate+1;
    	int slot_id;
    	if (i_ch < num_active_slots_cr1*num_channels) slot_id = nr_slot.at(i_ch / num_channels);
    	else slot_id = nr_slot.at(i_ch / num_channels) - 100.;
    	int channel_id = i_ch % num_channels;

    	if (verbosity > 2){
	  std::cout <<"Crate: "<<crate_id<<", Slot: "<<slot_id<<", Channel: "<<channel_id<<std::endl;
    	  std::cout <<"live_tdc.size: "<<live_tdc.at(i_ch).size()<<", rate: "<<live_tdc.at(i_ch).size()/(integration_period*60)<<std::endl;
	}

    	for (int i_entry= 0; i_entry < live_tdc.at(i_ch).size(); i_entry++){

    		if (crate_id == min_crate) rate_crate1->SetBinContent(slot_id,channel_id+1,live_tdc.at(i_ch).size()/(integration_period*60));		//display in Hz
    		else if (crate_id == min_crate+1) rate_crate2->SetBinContent(slot_id,channel_id+1,live_tdc.at(i_ch).size()/(integration_period*60));
    		TDC_hist->Fill(live_tdc.at(i_ch).at(i_entry));
    		if (live_tdc.at(i_ch).size()/(integration_period*60) > max_ch) max_ch = live_tdc.at(i_ch).size()/(integration_period*60); 
    		if (live_tdc.at(i_ch).size()/(integration_period*60) < min_ch) min_ch = live_tdc.at(i_ch).size()/(integration_period*60); 

    	}

    }

    for (int i_entry = 0; i_entry < vector_nchannels.size(); i_entry++){

    	n_paddles_hit->Fill(vector_nchannels.at(i_entry));

    	//fill coincident hits here afterwards

    }


    //first plot the rate 2D histograms (most work)

	TCanvas *canvas_rates = new TCanvas("canvas_rates","Rates electronics space",1000,600);
    canvas_rates->SetTitle(title_time.str().c_str());
	canvas_rates->Divide(2,1);


	TPad *p1 = (TPad*) canvas_rates->cd(1);
	p1->SetGrid();
	rate_crate1->Draw("colz");

	//color inactive channels in grey
	std::vector<TBox*> vector_box_inactive;
	for (int i_slot=0;i_slot<num_slots;i_slot++){
		if (active_channel[0][i_slot]==0){
			TBox *box_inactive = new TBox(i_slot,0,i_slot+1,num_channels);
			vector_box_inactive.push_back(box_inactive);
			box_inactive->SetFillStyle(3004);
			box_inactive->SetFillColor(1);
			box_inactive->Draw("same");  
		}
	}
	for (int i_ch = 0; i_ch < inactive_ch_crate1.size(); i_ch++){
			if (verbosity > 2) std::cout <<"inactive ch crate1, entry "<<i_ch<<std::endl;
			TBox *box_inactive = new TBox(inactive_slot_crate1.at(i_ch)-1,inactive_ch_crate1.at(i_ch),inactive_slot_crate1.at(i_ch),inactive_ch_crate1.at(i_ch)+1);
			vector_box_inactive.push_back(box_inactive);
			box_inactive->SetFillStyle(3004);
			box_inactive->SetFillColor(1);
			box_inactive->Draw("same");  
	}
	p1->Update();

	if (rate_crate1->GetMaximum()>0.){
		if (min_ch == max_ch) rate_crate1->GetZaxis()->SetRangeUser(min_ch-0.5,max_ch+0.5);
		else rate_crate1->GetZaxis()->SetRangeUser(min_ch,max_ch);
		TPaletteAxis *palette = 
		(TPaletteAxis*) rate_crate1->GetListOfFunctions()->FindObject("palette");
		palette->SetX1NDC(0.9);
		palette->SetX2NDC(0.92);
		palette->SetY1NDC(0.1);
		palette->SetY2NDC(0.9);
	}

	rate_crate2->SetTitle("Rates Rack 8");
	TPad *p2 = (TPad*) canvas_rates->cd(2);
	p2->SetGrid();

	rate_crate2->Draw("colz");
	for (int i_slot=0;i_slot<num_slots;i_slot++){
		if (active_channel[1][i_slot]==0){
			TBox *box_inactive = new TBox(i_slot,0,i_slot+1,num_channels);
			vector_box_inactive.push_back(box_inactive);
			box_inactive->SetFillColor(1);
			box_inactive->SetFillStyle(3004);
			box_inactive->Draw("same");  
		}
	}
	for (int i_ch = 0; i_ch < inactive_ch_crate2.size(); i_ch++){
		if (verbosity > 2) std::cout <<"inactive ch crate2, entry "<<i_ch<<std::endl;
		TBox *box_inactive = new TBox(inactive_slot_crate2.at(i_ch)-1,inactive_ch_crate2.at(i_ch),inactive_slot_crate2.at(i_ch),inactive_ch_crate2.at(i_ch)+1);
		vector_box_inactive.push_back(box_inactive);
		box_inactive->SetFillStyle(3004);
		box_inactive->SetFillColor(1);
		box_inactive->Draw("same");  
	}
	p2->Update();

	if (rate_crate2->GetMaximum()>0.){
		if (min_ch == max_ch) rate_crate2->GetZaxis()->SetRangeUser(min_ch-0.5,max_ch+0.5);
		else rate_crate2->GetZaxis()->SetRangeUser(min_ch,max_ch);

		TPaletteAxis *palette = 
		(TPaletteAxis*)rate_crate2->GetListOfFunctions()->FindObject("palette");
		palette->SetX1NDC(0.9);
		palette->SetX2NDC(0.92);
		palette->SetY1NDC(0.1);
		palette->SetY2NDC(0.9);
	}

	//p2->Modified();
	std::stringstream ss_rate_electronics;
	ss_rate_electronics<<outpath<<"MRD_Rates_Electronics.jpg";
	canvas_rates->SaveAs(ss_rate_electronics.str().c_str());

	//plot the other histograms as well

    TCanvas *canvas_tdc = new TCanvas("canvas_tdc","Canvas TDC",900,600);
    TCanvas *canvas_tdc_coincidence = new TCanvas("canvas_tdc_coincidence","Canvas TDC (coinc)",900,600);
    TCanvas *canvas_npaddles = new TCanvas("canvas_npaddles","Canvas NPaddles",900,600);
    TCanvas *canvas_npaddles_coincidence = new TCanvas("canvas_npaddles_coincidence","Canvas NPaddles (coinc)",900,600);

    canvas_tdc->cd();
    TDC_hist->Draw();
    canvas_tdc_coincidence->cd();
    TDC_hist_coincidence->Draw();
    canvas_npaddles->cd();
    n_paddles_hit->Draw();
    canvas_npaddles_coincidence->cd();
    n_paddles_hit_coincidence->Draw();

    std::stringstream ss_tdc;
    ss_tdc<<outpath<<"MRD_TDC.jpg";
    canvas_tdc->SaveAs(ss_tdc.str().c_str());
    //std::stringstream ss_tdc_coincidence;
    //ss_tdc_coincidence<<outpath<<"MRD_TDC_Event.jpg";
    //canvas_tdc_coincidence->SaveAs(ss_tdc_coincidence.str().c_str());
    std::stringstream ss_npaddles;
    ss_npaddles<<outpath<<"MRD_NPaddles.jpg";
    canvas_npaddles->SaveAs(ss_npaddles.str().c_str());
    //std::stringstream ss_npaddles_coincidence;
    //ss_npaddles_coincidence<<outpath<<"MRD_NPaddles_Event.jpg";
    //canvas_npaddles_coincidence->SaveAs(ss_npaddles_coincidence.str().c_str());

    for (int i_box = 0; i_box < vector_box_inactive.size(); i_box++){
    	delete vector_box_inactive.at(i_box);
    }

    delete rate_crate1;
    delete rate_crate2;
    delete rate_crate1_hour;
    delete rate_crate2_hour;
    delete TDC_hist;
    delete TDC_hist_hour;
    delete TDC_hist_coincidence;
    delete n_paddles_hit;
    delete n_paddles_hit_hour;
    delete n_paddles_hit_coincidence;

    delete canvas_tdc;
    delete canvas_tdc_coincidence;
    delete canvas_npaddles;
    delete canvas_npaddles_coincidence;
    delete canvas_rates;

    //get list of allocated objects (ROOT)
    //std::cout <<"List of Objects (End of UpdateMonitorPlots)"<<std::endl;
    //gObjectTable->Print();

}
