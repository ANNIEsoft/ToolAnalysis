#include "MonitorMRDLive.h"

MonitorMRDLive::MonitorMRDLive():Tool(){}


bool MonitorMRDLive::Initialise(std::string configfile, DataModel &data){

  //std::cout <<"Tool MonitorMRDLive: Initialising...."<<std::endl;

  /////////////////// Useful header ///////////////////////
  if(configfile!="")  m_variables.Initialise(configfile); //loading config file
  m_data= &data; //assigning transient data pointer
  /////////////////////////////////////////////////////////////////

  //only for debugging memory leaks, otherwise comment out
  /*std::cout <<"List of Objects (beginning of initialise): "<<std::endl;
  gObjectTable->Print();*/

  m_variables.Get("OutputPath",outpath);
  m_variables.Get("ActiveSlots",active_slots);
  m_variables.Get("verbose",verbosity);
  m_variables.Get("AveragePlots",draw_average);

  if (outpath == "fromStore") m_data->CStore.Get("OutPath",outpath);
  if (verbosity > 1) std::cout <<"Output path for plots is "<<outpath<<std::endl;

  num_active_slots=0;
  n_active_slots_cr1=0;
  n_active_slots_cr2=0;

  ifstream file(active_slots.c_str());
  int temp_crate, temp_slot;
  while (!file.eof()){

    file>>temp_crate>>temp_slot;
    if (file.eof()) break;
    if (temp_crate-min_crate<0 || temp_crate-min_crate>=num_crates) {
        std::cout <<"ERROR (MonitorMRDLive): Specified crate "<<temp_crate<<" out of range [7...8]. Continue with next entry."<<std::endl;
        continue;
    }
    if (temp_slot<1 || temp_slot>num_slots){
        std::cout <<"ERROR (MonitorMRDLive): Specified slot out of range [1...24]. Continue with next entry."<<std::endl;
        continue;
    }
    active_channel[temp_crate-min_crate][temp_slot-1]=1;        //crates numbering starts at 7, slot numbering at 1
    if (temp_crate-min_crate == 0) {n_active_slots_cr1++;active_slots_cr1.push_back(temp_slot);}
    else if (temp_crate-min_crate == 1){n_active_slots_cr2++;active_slots_cr2.push_back(temp_slot);}

  }
  file.close();
  num_active_slots = n_active_slots_cr1+n_active_slots_cr2;

  //omit warning messages from ROOT: info messages - 1001, warning messages - 2001, error messages - 3001
  gROOT->ProcessLine("gErrorIgnoreLevel = 3001;");

  return true;

}


bool MonitorMRDLive::Execute(){

  if (verbosity > 2) std::cout <<"Tool MonitorMRDLive: Executing...."<<std::endl;

  std::string State;
  m_data->CStore.Get("State",State);

  if (State == "MRDSingle"){				//is data valid for live event plotting?->MRDSingle state

    if (verbosity > 2) std::cout <<"MRDSingle Event: MonitorMRDLive is executed..."<<std::endl;
    
    m_data->Stores["CCData"]->Get("Single",MRDout); 

    OutN = MRDout.OutN;
    Trigger = MRDout.Trigger;
    Value = MRDout.Value;
    Slot = MRDout.Slot;
    Channel = MRDout.Channel;
    Crate = MRDout.Crate;
    TimeStamp = MRDout.TimeStamp;

    if (verbosity > 2){
    std::cout <<"OutN: "<<OutN<<std::endl;
    std::cout <<"Trigger: "<<Trigger<<std::endl;
    std::cout <<"MRD data size: "<<Value.size()<<std::endl;
    std::cout <<"TimeStamp: "<<TimeStamp<<std::endl;
    }

    t = time(0);
    struct tm *now = localtime( & t );

    title_time.str("");
    title_time<<(now->tm_year + 1900) << '-' << (now->tm_mon + 1) << '-' <<  now->tm_mday<<','<<now->tm_hour<<':'<<now->tm_min<<':'<<now->tm_sec;

    //plot all the monitoring plots
    MonitorMRDLive::MRDTDCPlots();

    //clean up
    Value.clear();
    Slot.clear();
    Channel.clear();
    Crate.clear();
    Type.clear();
    MRDout.Value.clear();
    MRDout.Slot.clear();
    MRDout.Channel.clear();
    MRDout.Crate.clear();
    MRDout.Type.clear();

    //only for debugging memory leaks, otherwise comment out
    /*std::cout <<"List of Objects (after execute step)"<<std::endl;
    gObjectTable->Print();*/

    return true;

  } else if (State == "DataFile" || State == "Wait"){

      if (verbosity > 3) std::cout <<"Status File (Data File or Wait): MonitorMRDLive not executed..."<<std::endl;        
      return true;

  } else {

    if (verbosity > 1) std::cout <<"State not recognized: "<<State<<std::endl;
    return true;

  }
  
}


bool MonitorMRDLive::Finalise(){

  if (verbosity > 2) std::cout <<"Tool MonitorMRDLive: Finalising..."<<std::endl;

  return true;
}


void MonitorMRDLive::MRDTDCPlots(){

  if (verbosity > 1) std::cout <<"Plotting MRD Single Event Monitors...."<<std::endl;

  TH1I *hChannel_cr1, *hChannel_cr2;  //two separate histograms for the channels of the two respective crates
  TH1D *hSlot_cr1, *hSlot_cr2;
  TH1D *hCrate;
  TH2I *h2D_cr1, *h2D_cr2;    //2D representation of channels, slots
  TH1F *hTimes;
  std::vector<TH1I*> hSlot_Channel;

  for (int i_active = 0; i_active<num_active_slots; i_active++){
    int slot_num, crate_num;
    if (i_active < n_active_slots_cr1){
      crate_num = min_crate;
      slot_num = active_slots_cr1.at(i_active);
    }
    else {
      crate_num = min_crate+1;
      slot_num = active_slots_cr2.at(i_active-n_active_slots_cr1);
    }
    std::stringstream ss_name_hist;
    std::stringstream ss_title_hist;
    std::string crate_str="cr";
    std::string slot_str = "_slot";
    ss_name_hist<<crate_str<<crate_num<<slot_str<<slot_num;
    ss_title_hist << title_time.str() << " (Crate " << crate_num << " Slot " << slot_num <<")";
    TH1I *hist = new TH1I(ss_name_hist.str().c_str(),ss_title_hist.str().c_str(),num_channels,0,num_channels);
    hist->GetXaxis()->SetTitle("Channel #");
    hist->GetYaxis()->SetTitle("TDC");
    hist->SetLineWidth(2);
    hSlot_Channel.push_back(hist);
  }

  TCanvas *c_FreqChannels;
  TCanvas *c_FreqSlots;
  TCanvas *c_FreqCrates;
  TCanvas *c_Freq2D;
  TCanvas *c_Times;
  TCanvas *c_SlotChannels;
  double max_cr1, max_cr2, min_cr1, min_cr2, max_scale, min_scale;
  double max_slot = 0;
  double min_slot = 99999999;
  double max_ch = 0;
  double min_ch = 99999999; 


  hChannel_cr1 = new TH1I("hChannel_cr1","TDC Channels Rack 7",num_active_slots*num_channels,0,num_active_slots*num_channels);
  hChannel_cr2 = new TH1I("hChannel_cr2","TDC Channels Rack 8",num_active_slots*num_channels,0,num_active_slots*num_channels);
  hSlot_cr1 = new TH1D("hSlot_cr1","TDC Slots Rack 7",num_active_slots,0,num_active_slots);
  hSlot_cr2 = new TH1D("hSlot_cr2","TDC Slots Rack 8",num_active_slots,0,num_active_slots);
  hCrate = new TH1D("hCrate","TDC values (all crates)",num_crates,0,num_crates);
  h2D_cr1 = new TH2I("h2D_cr1","TDC values 2D Rack 7",num_slots,0,num_slots,num_channels,0,num_channels);
  h2D_cr2 = new TH2I("h2D_cr2","TDC values 2D Rack 8",num_slots,0,num_slots,num_channels,0,num_channels);
  hTimes = new TH1F("hTimes","TDC values current event",200,1,0);

  for (int i_crate=0;i_crate<num_crates;i_crate++){
    for (int i_slot=0;i_slot<num_slots;i_slot++){
      if (active_channel[i_crate][i_slot]==0) continue;
      for (int i_channel=0;i_channel<num_channels;i_channel++){
        mapping_vector_ch.push_back(i_crate*num_slots+i_slot*num_channels+i_channel);
      }
    }
  }

  const char *labels_crate[2]={"Rack 7","Rack 8"};

  hChannel_cr1->SetLineColor(8);
  hChannel_cr2->SetLineColor(9);
  hSlot_cr1->SetLineColor(8);
  hSlot_cr2->SetLineColor(9);
  hCrate->SetLineColor(1);

  double times_slots[num_crates][num_slots] = {0};
  double n_times_slots[num_crates][num_slots] = {0};


  for (int i=0;i<Value.size();i++){
  hTimes->Fill(Value.at(i));
  times_slots[Crate.at(i)-min_crate][Slot.at(i)-1] += Value.at(i);    //slot numbers start at 1
  n_times_slots[Crate.at(i)-min_crate][Slot.at(i)-1]++;
  if (Value.at(i) > max_ch) max_ch = Value.at(i);
  if (Value.at(i) < min_ch) min_ch = Value.at(i);
        
        if (Crate.at(i) == min_crate) {
            if(active_channel[0][Slot.at(i)-1]==1) {
                std::vector<int>::iterator it = std::find(active_slots_cr1.begin(),active_slots_cr1.end(),Slot.at(i));
                int index = std::distance(active_slots_cr1.begin(),it);
                hChannel_cr1->SetBinContent((index)*num_channels+Channel.at(i)+1,Value.at(i));     //slot numbers start at +1?
                h2D_cr1->SetBinContent(Slot.at(i),Channel.at(i)+1,Value.at(i));
                hSlot_Channel.at(index)->SetBinContent(Channel.at(i)+1,Value.at(i));
            } else {
              std::cout <<"ERROR (MonitorMRDLive): Slot # "<<Slot.at(i)<<" is not connected according to the configuration file. Abort this entry..."<<std::endl;
              continue;
            }
        } else if (Crate.at(i) == min_crate+1) {
          if(active_channel[1][Slot.at(i)-1]==1) {
                std::vector<int>::iterator it = std::find(active_slots_cr2.begin(),active_slots_cr2.end(),Slot.at(i));
                int index = std::distance(active_slots_cr2.begin(),it);
                hChannel_cr2->SetBinContent(n_active_slots_cr1*num_channels+(index)*num_channels+Channel.at(i)+1,Value.at(i));
                h2D_cr2->SetBinContent(Slot.at(i),Channel.at(i)+1,Value.at(i));
                hSlot_Channel.at(n_active_slots_cr1+index)->SetBinContent(Channel.at(i)+1,Value.at(i));
          }else {
            std::cout <<"ERROR (MonitorMRDLive): Slot # "<<Slot.at(i)<<" is not connected according to the configuration file. Abort this entry..."<<std::endl;
            continue;
          }
        }
        else std::cout <<"ERROR (MonitorMRDLive): The read-in crate number does not exist. Continue with next event... "<<std::endl;
      }

      if (verbosity > 2) std::cout <<"Iterating over slots..."<<std::endl;

      for (int i_slot=0;i_slot<num_slots;i_slot++){

        if (active_channel[0][i_slot]==1){
          if (n_times_slots[0][i_slot]>0){
          if (times_slots[0][i_slot]/num_channels > max_slot) max_slot = times_slots[0][i_slot]/num_channels;
          if (times_slots[0][i_slot]/num_channels < min_slot) min_slot = times_slots[0][i_slot]/num_channels;
          std::vector<int>::iterator it = std::find(active_slots_cr1.begin(),active_slots_cr1.end(),i_slot+1);
          int index = std::distance(active_slots_cr1.begin(),it);
          hSlot_cr1->SetBinContent(index+1,times_slots[0][i_slot]/num_channels);
        }
        }
        if (active_channel[1][i_slot]==1){
          if (n_times_slots[1][i_slot]>0){
          if (times_slots[1][i_slot]/num_channels > max_slot) max_slot = times_slots[1][i_slot]/num_channels;
          if (times_slots[1][i_slot]/num_channels < min_slot) min_slot = times_slots[1][i_slot]/num_channels;
          std::vector<int>::iterator it = std::find(active_slots_cr2.begin(),active_slots_cr2.end(),i_slot+1);
          int index = std::distance(active_slots_cr2.begin(),it);
          hSlot_cr2->SetBinContent(n_active_slots_cr1+index+1,times_slots[1][i_slot]/num_channels);
        }
      }

      }          

      if (verbosity > 2) std::cout <<"Iterating over Crates...."<<std::endl;
      for (int i_crate=0;i_crate<num_crates;i_crate++){

        double mean_tdc_crate=0.;
        int num_tdc_crate=0;
        int n_slots = (i_crate == 0)? n_active_slots_cr1 : n_active_slots_cr2;
        for (int i_slot=0;i_slot<n_slots;i_slot++){
            int slot_nr = (i_crate ==0)? active_slots_cr1.at(i_slot) : active_slots_cr2.at(i_slot);    //only consider active slots for averaging
            if (n_times_slots[i_crate][slot_nr-1]>0) mean_tdc_crate+=(times_slots[i_crate][slot_nr-1]/n_times_slots[i_crate][slot_nr-1]);
            num_tdc_crate++;
          
        }

        hCrate->SetBinContent(i_crate+1,mean_tdc_crate/num_tdc_crate);

      }

      if (verbosity > 2) std::cout <<"Creating channel frequency plot..."<<std::endl;
      c_FreqChannels = new TCanvas("MRD Freq ch","MRD TDC monitor",900,600);
      hChannel_cr1->SetStats(0);
      hChannel_cr2->SetStats(0);

      min_cr1=hChannel_cr1->GetMinimum();
      min_cr2 = hChannel_cr2->GetMinimum();
      min_scale = (min_cr1<min_cr2)? min_cr1 : min_cr2;
      max_cr1=hChannel_cr1->GetMaximum();
      max_cr2 = hChannel_cr2->GetMaximum();
      max_scale = (max_cr1>max_cr2)? max_cr1 : max_cr2;
      
      hChannel_cr1->GetYaxis()->SetRangeUser(min_scale,max_scale+20);
      hChannel_cr1->GetYaxis()->SetTitle("TDC value");
      hChannel_cr1->SetTitle(title_time.str().c_str());
      hChannel_cr1->GetXaxis()->SetNdivisions(int(num_active_slots));
      hChannel_cr2->GetXaxis()->SetNdivisions(int(num_active_slots));
      for (int i_label=0;i_label<int(num_active_slots);i_label++){
        if (i_label<n_active_slots_cr1){
          std::stringstream ss_slot;
          ss_slot<<(active_slots_cr1.at(i_label));
          std::string str_slot = "slot "+ss_slot.str();
          hChannel_cr1->GetXaxis()->SetBinLabel(num_channels*(i_label+0.5),str_slot.c_str());
        }
        else {
          std::stringstream ss_slot;
          ss_slot<<(active_slots_cr2.at(i_label-n_active_slots_cr1));
          std::string str_slot = "slot "+ss_slot.str();
          hChannel_cr1->GetXaxis()->SetBinLabel(num_channels*(i_label+0.5),str_slot.c_str());          
        }
      }
      hChannel_cr1->LabelsOption("v");
      hChannel_cr1->SetTickLength(0,"X");		//workaround to only have labels for every slot
      hChannel_cr2->SetTickLength(0,"X");
      hChannel_cr1->GetYaxis()->SetRangeUser(min_ch-5,max_ch+5);
      hChannel_cr2->GetYaxis()->SetRangeUser(min_ch-5,max_ch+5);
      c_FreqChannels->SetGridy();
      hChannel_cr1->SetLineWidth(2);
      hChannel_cr2->SetLineWidth(2);

      //std::cout <<"Draw hChannel_cr1 & hChannel_cr2"<<std::endl;
      hChannel_cr1->Draw();
      hChannel_cr2->Draw("same");
      TLine *separate_crates = new TLine(num_channels*n_active_slots_cr1,min_ch-5,num_channels*n_active_slots_cr1,max_ch+5);
      separate_crates->SetLineStyle(2);
      separate_crates->SetLineWidth(2);
      separate_crates->Draw("same");
    
      TPaveText *label_cr1 = new TPaveText(0.1,0.93,0.25,0.98,"NDC");
      label_cr1->SetFillColor(0);
      label_cr1->SetTextColor(8);
      label_cr1->AddText("Rack 7");
      label_cr1->Draw();
      TPaveText *label_cr2 = new TPaveText(0.75,0.93,0.9,0.98,"NDC");
      label_cr2->SetFillColor(0);
      label_cr2->SetTextColor(9);
      label_cr2->AddText("Rack 8");
      label_cr2->Draw();

      c_FreqChannels->Update();

      TF1 *f1 = new TF1("f1","x",0,num_active_slots);       //workaround to only have labels for every slot
      TGaxis *labels_grid = new TGaxis(0,c_FreqChannels->GetUymin(),num_active_slots*num_channels,c_FreqChannels->GetUymin(),"f1",num_active_slots,"w");
      labels_grid->SetLabelSize(0);
      labels_grid->Draw("w");


      std::stringstream ss_tmp;
      ss_tmp<<outpath<<"TDC_Channels.jpg";
      c_FreqChannels->SaveAs(ss_tmp.str().c_str());

      if (verbosity > 2) std::cout <<"Creating Slot Frequency plot..."<<std::endl;
      c_FreqSlots = new TCanvas("MRD Freq slots","MRD TDC monitor 2",900,600);
      hSlot_cr1->SetStats(0);
      hSlot_cr2->SetStats(0);

      hSlot_cr1->GetYaxis()->SetRangeUser(min_slot-5,max_slot+5);
      hSlot_cr1->GetYaxis()->SetTitle("average TDC value");
      hSlot_cr1->SetTitle(title_time.str().c_str());
      hSlot_cr1->SetLineWidth(2);
      hSlot_cr1->GetXaxis()->SetNdivisions(int(num_active_slots));
      for (int i_label=0;i_label<int(num_active_slots);i_label++){
        if (i_label<n_active_slots_cr1){
          std::stringstream ss_slot;
          ss_slot<<(active_slots_cr1.at(i_label));
          std::string str_slot = "slot "+ss_slot.str();
          hSlot_cr1->GetXaxis()->SetBinLabel(i_label+1,str_slot.c_str());
        }
        else {
          std::stringstream ss_slot;
          ss_slot<<(active_slots_cr2.at(i_label-n_active_slots_cr1));
          std::string str_slot = "slot "+ss_slot.str();
          hSlot_cr1->GetXaxis()->SetBinLabel(i_label+1,str_slot.c_str());
        }
      }
      hSlot_cr1->LabelsOption("v");
      hSlot_cr2->SetLineWidth(2);
      c_FreqSlots->SetGridy();
      c_FreqSlots->SetGridx();
      //std::cout <<"Drawing hSlot_cr1 & hSlot_cr2"<<std::endl;
      hSlot_cr1->Draw();
      hSlot_cr2->Draw("same");
      TLine *separate_crates2 = new TLine(n_active_slots_cr1,min_slot-5,n_active_slots_cr1,max_slot+5);
      separate_crates2->SetLineStyle(2);
      separate_crates2->SetLineWidth(2);
      separate_crates2->Draw("same");
      label_cr1->Draw();
      label_cr2->Draw();


      std::stringstream ss_ch;
      ss_ch<<outpath<<"TDC_Slots.jpg";
      if (draw_average) c_FreqSlots->SaveAs(ss_ch.str().c_str());

      if (verbosity > 2) std::cout <<"Creating Crate Frequency plot..."<<std::endl;
      c_FreqCrates = new TCanvas("MRD Freq crates","MRD TDC monitor 3",900,600);
      c_FreqCrates->SetGridy();
      hCrate->SetStats(0);
      hCrate->GetYaxis()->SetTitle("average TDC value");
      //hCrate->GetXaxis()->SetTitle("Crate #");
      hCrate->SetTitle(title_time.str().c_str());
      hCrate->SetLineWidth(2);
      hCrate->GetXaxis()->SetBinLabel(1,labels_crate[0]);
      hCrate->GetXaxis()->SetBinLabel(2,labels_crate[1]);
      c_FreqCrates->SetGridx();
      c_FreqCrates->SetGridy();
      //std::cout <<"Drawing hCrate..."<<std::endl;
      hCrate->Draw();

      std::stringstream ss_crate;
      ss_crate<<outpath<<"TDC_Crates.jpg";
      if (draw_average) c_FreqCrates->SaveAs(ss_crate.str().c_str());


      c_Freq2D = new TCanvas("MRD Freq 2D","MRD TDC monitor 4",1000,600);
      c_Freq2D->SetTitle(title_time.str().c_str());
      c_Freq2D->Divide(2,1);

      h2D_cr1->SetStats(0);
      h2D_cr1->GetXaxis()->SetNdivisions(num_slots);
      h2D_cr1->GetYaxis()->SetNdivisions(num_channels);
      h2D_cr1->SetTitle("Rack 7");
      TPad *p1 = (TPad*) c_Freq2D->cd(1);
      p1->SetGrid();
      for (int i_label=0;i_label<int(num_channels);i_label++){
        
        std::stringstream ss_slot, ss_ch;
        ss_slot<<(i_label+1);
        ss_ch<<(i_label);
        std::string str_ch = "ch "+ss_ch.str();
        h2D_cr1->GetYaxis()->SetBinLabel(i_label+1,str_ch.c_str());
       
        if (i_label<num_slots){

          std::string str_slot = "slot "+ss_slot.str();
          h2D_cr1->GetXaxis()->SetBinLabel(i_label+1,str_slot.c_str());
        
        }
      }      
      h2D_cr1->LabelsOption("v");
      //std::cout <<"Drawing h2D_cr1..."<<std::endl;
      h2D_cr1->Draw("colz");

      std::vector<TBox*> vector_box_inactive;
       //coloring inactive slots in the histograms in grey-ish
      for (int i_slot=0;i_slot<num_slots;i_slot++){
        if (active_channel[0][i_slot]==0){
         
          TBox *box_inactive = new TBox(i_slot,0,i_slot+1,num_channels);
          vector_box_inactive.push_back(box_inactive);
          box_inactive->SetFillStyle(3004);
          box_inactive->SetFillColor(1);
          box_inactive->Draw("same");  
        }
      }
      p1->Update();
      if (h2D_cr1->GetMaximum()>0.){
        if (min_ch == max_ch) h2D_cr1->GetZaxis()->SetRangeUser(min_ch-0.5,max_ch+0.5);
        else h2D_cr1->GetZaxis()->SetRangeUser(min_ch,max_ch);
        TPaletteAxis *palette = 
      (TPaletteAxis*)h2D_cr1->GetListOfFunctions()->FindObject("palette");
        palette->SetX1NDC(0.9);
        palette->SetX2NDC(0.92);
        palette->SetY1NDC(0.1);
        palette->SetY2NDC(0.9);
        //h2D_cr1->GetZaxis()->SetTitle("TDC value");
      }
      //p1->Modified();

      h2D_cr2->SetStats(0);
      h2D_cr2->GetXaxis()->SetNdivisions(num_slots);
      h2D_cr2->GetYaxis()->SetNdivisions(num_channels);
      h2D_cr2->SetTitle("Rack 8");
      TPad *p2 = (TPad*) c_Freq2D->cd(2);
      p2->SetGrid();
      for (int i_label=0;i_label<int(num_channels);i_label++){
        std::stringstream ss_slot, ss_ch;
        ss_slot<<(i_label+1);
        ss_ch<<(i_label);
        std::string str_ch = "ch "+ss_ch.str();
        h2D_cr2->GetYaxis()->SetBinLabel(i_label+1,str_ch.c_str());
        if (i_label<num_slots){
          std::string str_slot = "slot "+ss_slot.str();
          h2D_cr2->GetXaxis()->SetBinLabel(i_label+1,str_slot.c_str());
        }
        
      }
      h2D_cr2->LabelsOption("v");
      //std::cout <<"Draw h2D_cr2..."<<std::endl;
      h2D_cr2->Draw("colz");
      //coloring inactive slots in the histograms in grey-ish
      for (int i_slot=0;i_slot<num_slots;i_slot++){
        if (active_channel[1][i_slot]==0){
          TBox *box_inactive = new TBox(i_slot,0,i_slot+1,num_channels);
          vector_box_inactive.push_back(box_inactive);
          box_inactive->SetFillColor(1);
          box_inactive->SetFillStyle(3004);
          box_inactive->Draw("same");  
        }
      }
      p2->Update();
      if (h2D_cr2->GetMaximum()>0.){
        if (min_ch == max_ch) h2D_cr2->GetZaxis()->SetRangeUser(min_ch-0.5,max_ch+0.5);
        else h2D_cr2->GetZaxis()->SetRangeUser(min_ch,max_ch);

        TPaletteAxis *palette = 
      (TPaletteAxis*)h2D_cr2->GetListOfFunctions()->FindObject("palette");
        //palette2->SetName("palette2");
        palette->SetX1NDC(0.9);
        palette->SetX2NDC(0.92);
        palette->SetY1NDC(0.1);
        palette->SetY2NDC(0.9);
        //h2D_cr2->GetZaxis()->SetTitle("TDC value");
      }
      
      //p2->Modified();
      std::stringstream ss_2D;
      ss_2D<<outpath<<"TDC_2D.jpg";
      c_Freq2D->SaveAs(ss_2D.str().c_str());

      c_Times = new TCanvas("MRD_Times","MRD TDC monitor 5",900,600);
      hTimes->GetXaxis()->SetTitle("TDC value");
      hTimes->GetYaxis()->SetTitle("Frequency");
      hTimes->SetStats(0);
      hTimes->SetLineWidth(2);
      hTimes->SetTitle(title_time.str().c_str());
      if (hTimes->GetEntries() == 0) {
        TH1F *hTimes_temp = new TH1F("hTimes_temp",title_time.str().c_str(),200,0,100);
        hTimes_temp->Draw();
        std::stringstream ss_Times;
        ss_Times<<outpath<<"TDC_Hist.jpg";
        c_Times->SaveAs(ss_Times.str().c_str());
        delete hTimes_temp;
      } else {
        //std::cout <<"Draw hTimes..."<<std::endl;
        //std::cout <<"hTimes entries: "<<hTimes->GetEntries()<<", max: "<<hTimes->GetMaximum()<<", min: "<<hTimes->GetMinimum()<<std::endl;
        hTimes->Draw();
        std::stringstream ss_Times;
        ss_Times<<outpath<<"TDC_Hist.jpg";
        c_Times->SaveAs(ss_Times.str().c_str());
      }

      for (int i_slot = 0; i_slot < num_active_slots; i_slot++){

        /*                            //too verbose, omit displaying single channel plots for all slots for now
        std::stringstream ss_title;
        std::string prefix = "TDC_SlotChannels_";
        ss_title << prefix << (i_slot)+1;
        c_SlotChannels = new TCanvas(ss_title.str().c_str(),"Canvas",900,600);
        c_SlotChannels->cd();
        hSlot_Channel.at(i_slot)->SetStats(0);
        hSlot_Channel.at(i_slot)->Draw();
        std::stringstream ss_savepath;
        ss_savepath << outpath << prefix << (i_slot+1) <<".png";
        c_SlotChannels->SetGridx();
        c_SlotChannels->SaveAs(ss_savepath.str().c_str());
        c_SlotChannels->Clear();
        delete c_SlotChannels;*/

        delete hSlot_Channel.at(i_slot);

      }

    
    delete separate_crates;
    delete separate_crates2;
    delete label_cr1;
    delete label_cr2;
    delete f1;
    delete labels_grid;
    for (int i_box = 0; i_box < vector_box_inactive.size(); i_box++){
          delete vector_box_inactive.at(i_box);
    }
    delete hChannel_cr1;
    delete hChannel_cr2;
    delete hSlot_cr1;
    delete hSlot_cr2;
    delete hCrate;
    delete h2D_cr1;
    delete h2D_cr2;
    delete hTimes;
    delete c_FreqChannels;
    delete c_FreqSlots;
    delete c_FreqCrates;
    delete c_Freq2D;
    delete c_Times;



}
