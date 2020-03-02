#include "MonitorMRDLive.h"

MonitorMRDLive::MonitorMRDLive():Tool(){}


bool MonitorMRDLive::Initialise(std::string configfile, DataModel &data){


  /////////////////// Useful header ///////////////////////
  if(configfile!="")  m_variables.Initialise(configfile); //loading config file
  m_data= &data; //assigning transient data pointer
  /////////////////////////////////////////////////////////////////

  //only for debugging memory leaks, otherwise comment out
  //std::cout <<"List of Objects (beginning of initialise): "<<std::endl;
  //gObjectTable->Print();

  m_variables.Get("OutputPath",outpath_temp);
  m_variables.Get("ActiveSlots",active_slots);
  m_variables.Get("InActiveChannels",inactive_channels);
  m_variables.Get("LoopbackChannels",loopback_channels);
  m_variables.Get("verbose",verbosity);
  m_variables.Get("AveragePlots",draw_average);

  if (verbosity > 2) std::cout <<"Tool MonitorMRDLive: Initialising...."<<std::endl;


  if (outpath_temp == "fromStore") m_data->CStore.Get("OutPath",outpath);
  if (verbosity > 1) std::cout <<"Output path for plots is "<<outpath<<std::endl;

  //-------------------------------------------------------
  //-------------------Get active slots--------------------
  //-------------------------------------------------------

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
    nr_slot.push_back((temp_crate-min_crate)*100+(temp_slot)); 
    active_channel[temp_crate-min_crate][temp_slot-1]=1;        //crates numbering starts at 7, slot numbering at 1
    if (temp_crate-min_crate == 0) {n_active_slots_cr1++;active_slots_cr1.push_back(temp_slot);}
    else if (temp_crate-min_crate == 1){n_active_slots_cr2++;active_slots_cr2.push_back(temp_slot);}
  }
  file.close();
  num_active_slots = n_active_slots_cr1+n_active_slots_cr2;

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
      std::cout <<"ERROR (MonitorMRDLive): Specified crate out of range [7...8]. Continue with next entry."<<std::endl;
      continue;
    }
    if (temp_slot<1 || temp_slot>num_slots){
      std::cout <<"ERROR (MonitorMRDLive): Specified slot out of range [1...24]. Continue with next entry."<<std::endl;
      continue;
    }
    if (temp_channel<0 || temp_channel > num_channels){
      std::cout <<"ERROR (MonitorMRDLive): Specified channel out of range [0...31]. Continue with next entry."<<std::endl;
      continue;
    }

    if (temp_crate == min_crate){
      inactive_ch_crate1.push_back(temp_channel);
      inactive_slot_crate1.push_back(temp_slot);
    } else if (temp_crate == min_crate+1){
      inactive_ch_crate2.push_back(temp_channel);
      inactive_slot_crate2.push_back(temp_slot);
    } else {
      std::cout <<"ERROR (MonitorMRDLive): Crate # out of range, entry ("<<temp_crate<<"/"<<temp_slot<<"/"<<temp_channel<<") not added to inactive channel configuration." <<std::endl;
    }
  }
  file_inactive.close();

  //-------------------------------------------------------
  //-------------Get loopback channels---------------------
  //-------------------------------------------------------

  ifstream file_loopback(loopback_channels.c_str());
  std::string temp_loopback_name;
  int i_loopback=0;

  while (!file_loopback.eof()){
    file_loopback>>temp_loopback_name>>temp_crate>>temp_slot>>temp_channel;
    if (verbosity > 2) std::cout<<temp_loopback_name<<": "<<temp_crate<<" , "<<temp_slot<<" , "<<temp_channel<<std::endl;
    if (file_loopback.eof()) break;

    if (temp_crate-min_crate<0 || temp_crate-min_crate>=num_crates) {
      std::cout <<"ERROR (MonitorMRDLive): Specified loopback crate out of range [7...8]. Continue with next entry."<<std::endl;
      continue;
    }
    if (temp_slot<1 || temp_slot>num_slots){
      std::cout <<"ERROR (MonitorMRDLive): Specified loopback slot out of range [1...24]. Continue with next entry."<<std::endl;
      continue;
    }
    if (temp_channel<0 || temp_channel > num_channels){
      std::cout <<"ERROR (MonitorMRDLive): Specified loopback channel out of range [0...31]. Continue with next entry."<<std::endl;
      continue;
    }

    loopback_name.push_back(temp_loopback_name);
    loopback_crate.push_back(temp_crate);
    loopback_slot.push_back(temp_slot);
    loopback_channel.push_back(temp_channel);

    int linecolor = 0;
    if (i_loopback ==0) linecolor = 1;
    else if (i_loopback ==1) linecolor = 4;
    else linecolor = 4+i_loopback;

    map_triggertype_color.emplace(temp_loopback_name,linecolor);
    TText *trigger_label = new TText(0.9,0.96-0.02*i_loopback,temp_loopback_name.c_str());
    trigger_label->SetNDC();
    trigger_label->SetTextSize(0.02);
    trigger_label->SetTextColor(linecolor);
    trigger_labels.push_back(trigger_label);
    i_loopback++;

  }
  file_loopback.close();

  map_triggertype_color.emplace("no loopback",2);
  TText *trigger_label = new TText(0.9,0.92,"no loopback");
  trigger_label->SetNDC();
  trigger_label->SetTextSize(0.02);
  trigger_label->SetTextColor(2);
  trigger_labels.push_back(trigger_label);

  for (unsigned int i_vector = 0; i_vector < loopback_channel.size(); i_vector++){
    if (verbosity > 1) std::cout <<"Loopback "<<loopback_name.at(i_vector)<<": Cr "<<loopback_crate.at(i_vector)<<", Sl "<<loopback_slot.at(i_vector)<<", Ch "<<loopback_channel.at(i_vector)<<std::endl;
  }

  //-------------------------------------------------------
  //----------Initialize storing containers----------------
  //-------------------------------------------------------

  MonitorMRDLive::InitializeVectors();

  //-------------------------------------------------------
  //----------Initialize histograms------------------------
  //-------------------------------------------------------

  n_bins_loglive = 10;

  gROOT->cd();

  hChannel_cr1 = new TH1I("hChannel_cr1","TDC Channels Rack 7",num_active_slots*num_channels,0,num_active_slots*num_channels);
  hChannel_cr2 = new TH1I("hChannel_cr2","TDC Channels Rack 8",num_active_slots*num_channels,0,num_active_slots*num_channels);
  hSlot_cr1 = new TH1D("hSlot_cr1","TDC Slots Rack 7",num_active_slots,0,num_active_slots);
  hSlot_cr2 = new TH1D("hSlot_cr2","TDC Slots Rack 8",num_active_slots,0,num_active_slots);
  hCrate = new TH1D("hCrate","TDC values (all crates)",num_crates,0,num_crates);
  h2D_cr1 = new TH2I("h2D_cr1","TDC values 2D Rack 7",num_slots,0,num_slots,num_channels,0,num_channels);
  h2D_cr2 = new TH2I("h2D_cr2","TDC values 2D Rack 8",num_slots,0,num_slots,num_channels,0,num_channels);
  hTimes = new TH1F("hTimes","TDC values current event",100,0,1000);
  rate_crate1 = new TH2F("rate_crate1","Rates Rack 7 (5 mins)",num_slots,0,num_slots,num_channels,0,num_channels);
  rate_crate2 = new TH2F("rate_crate2","Rates Rack 8 (5 mins)",num_slots,0,num_slots,num_channels,0,num_channels);
  rate_crate1_hour = new TH2F("rate_crate1_hour","Rates Rack 7 (1 hour)",num_slots,0,num_slots,num_channels,0,num_channels);
  rate_crate2_hour = new TH2F("rate_crate2_hour","Rates Rack 8 (1 hour)",num_slots,0,num_slots,num_channels,0,num_channels);
  TDC_hist = new TH1F("TDC_hist","TDC live events (5 mins)",100,0,1000);
  TDC_hist_hour = new TH1F("TDC_hist_hour","TDC live events (1 hour)",100,0,1000);
  n_paddles_hit = new TH1F("n_paddles_hit","N Paddles (5 mins)",50,0,50);
  n_paddles_hit_hour = new TH1F("n_paddles_hit_hour","N Paddles (1 hour)",50,0,50);
  log_live_events = new TH1F("log_live_events","Live Event history (last 10 events)",n_bins_loglive,0,n_bins_loglive);       //show the time stamps of the last 10 live events

  hChannel_cr1->SetStats(0);
  hChannel_cr2->SetStats(0);
  hChannel_cr1->SetLineColor(8);
  hChannel_cr2->SetLineColor(9);
  hChannel_cr1->SetLineWidth(2);
  hChannel_cr2->SetLineWidth(2);
  hSlot_cr1->SetStats(0);
  hSlot_cr2->SetStats(0); 
  hSlot_cr1->SetLineColor(8);
  hSlot_cr2->SetLineColor(9);
  hCrate->SetLineColor(1);
  hCrate->SetStats(0);
  hCrate->SetLineWidth(2);
  hTimes->SetStats(0);

  hChannel_cr1->GetYaxis()->SetTitle("TDC");
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
  hChannel_cr1->SetTickLength(0,"X");   //workaround to only have labels for every slot
  hChannel_cr2->SetTickLength(0,"X");
  label_cr1 = new TPaveText(0.1,0.93,0.25,0.98,"NDC");
  label_cr1->SetFillColor(0);
  label_cr1->SetTextColor(8);
  label_cr1->AddText("Rack 7");
  label_cr2 = new TPaveText(0.75,0.93,0.9,0.98,"NDC");
  label_cr2->SetFillColor(0);
  label_cr2->SetTextColor(9);
  label_cr2->AddText("Rack 8");

  hSlot_cr1->GetYaxis()->SetTitle("average TDC");
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

  hCrate->GetYaxis()->SetTitle("average TDC");
  hCrate->GetXaxis()->SetBinLabel(1,labels_crate[0]);
  hCrate->GetXaxis()->SetBinLabel(2,labels_crate[1]);

  hTimes->GetXaxis()->SetTitle("TDC");
  hTimes->GetYaxis()->SetTitle("#");

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

  rate_crate1_hour->SetStats(0);
  rate_crate1_hour->GetXaxis()->SetNdivisions(num_slots);
  rate_crate1_hour->GetYaxis()->SetNdivisions(num_channels);
  for (int i_label=0;i_label<int(num_channels);i_label++){
    std::stringstream ss_slot, ss_ch;
    ss_slot<<(i_label+1);
    ss_ch<<(i_label);
    std::string str_ch = "ch "+ss_ch.str();
    rate_crate1_hour->GetYaxis()->SetBinLabel(i_label+1,str_ch.c_str());
    if (i_label<num_slots){
      std::string str_slot = "slot "+ss_slot.str();
      rate_crate1_hour->GetXaxis()->SetBinLabel(i_label+1,str_slot.c_str());
    }
  }
  rate_crate1_hour->LabelsOption("v");
  rate_crate2_hour->SetStats(0);
  rate_crate2_hour->GetXaxis()->SetNdivisions(num_slots);
  rate_crate2_hour->GetYaxis()->SetNdivisions(num_channels);
  for (int i_label=0;i_label<int(num_channels);i_label++){
    std::stringstream ss_slot, ss_ch;
    ss_slot<<(i_label+1);
    ss_ch<<(i_label);
    std::string str_ch = "ch "+ss_ch.str();
    rate_crate2_hour->GetYaxis()->SetBinLabel(i_label+1,str_ch.c_str());
    if (i_label<num_slots){
      std::string str_slot = "slot "+ss_slot.str();
      rate_crate2_hour->GetXaxis()->SetBinLabel(i_label+1,str_slot.c_str());
    }
  }
  rate_crate2_hour->LabelsOption("v");

  label_rate_cr1 = new TLatex(0.88,0.93,"R_{live} [Hz]");
  label_rate_cr1->SetNDC();
  label_rate_cr1->SetTextSize(0.03);
  label_rate_cr2 = new TLatex(0.88,0.93,"R_{live} [Hz]");
  label_rate_cr2->SetNDC();
  label_rate_cr2->SetTextSize(0.03);
  label_tdc = new TText(0.92,0.93,"TDC");
  label_tdc->SetNDC();
  label_tdc->SetTextSize(0.03);

  h2D_cr1->SetStats(0);
  h2D_cr1->GetXaxis()->SetNdivisions(num_slots);
  h2D_cr1->GetYaxis()->SetNdivisions(num_channels);
  h2D_cr1->SetTitle("Rack 7 (Live Event)");

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

  h2D_cr2->SetStats(0);
  h2D_cr2->GetXaxis()->SetNdivisions(num_slots);
  h2D_cr2->GetYaxis()->SetNdivisions(num_channels);
  h2D_cr2->SetTitle("Rack 8 (Live Event)");
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

  TDC_hist->GetXaxis()->SetTitle("TDC");
  TDC_hist->GetYaxis()->SetTitle("#");
  TDC_hist_hour->GetXaxis()->SetTitle("TDC");
  TDC_hist_hour->GetYaxis()->SetTitle("#");
  n_paddles_hit->GetXaxis()->SetTitle("# hit paddles");
  n_paddles_hit->GetYaxis()->SetTitle("#");
  n_paddles_hit_hour->GetXaxis()->SetTitle("# hit paddles");
  n_paddles_hit_hour->GetYaxis()->SetTitle("#");
  rate_crate1_hour->SetTitle("Live Rates Rack 7 (1 hour)");
  rate_crate2_hour->SetTitle("Live Rates Rack 8 (1 hour)");

  log_live_events->GetXaxis()->SetLabelSize(0.035);
  log_live_events->GetXaxis()->SetLabelOffset(0.01);
  log_live_events->SetStats(0);
  log_live_events->GetYaxis()->SetRangeUser(0.,1.);
  log_live_events->GetYaxis()->SetTickLength(0.);
  log_live_events->GetYaxis()->SetLabelOffset(999);

  for (int i_log = 0; i_log < n_bins_loglive; i_log++){
    TLine *line = new TLine(i_log+0.5,0.,i_log+0.5,1.);
    line->SetLineStyle(1);
    line->SetLineWidth(5);
    vector_lines.push_back(line);
  }

  //-------------------------------------------------------
  //----------Color inactive channels in grey--------------
  //-------------------------------------------------------

  inactive_crate1 = 0;
  inactive_crate2 = 0;
  for (int i_slot=0;i_slot<num_slots;i_slot++){
    if (active_channel[0][i_slot]==0){
      TBox *box_inactive = new TBox(i_slot,0,i_slot+1,num_channels);
      vector_box_inactive.push_back(box_inactive);
      box_inactive->SetFillStyle(3004);
      box_inactive->SetFillColor(1);
      inactive_crate1++;
    }
  }
  for (unsigned int i_ch = 0; i_ch < inactive_ch_crate1.size(); i_ch++){
    if (verbosity > 2) std::cout <<"inactive ch crate1, entry "<<i_ch<<std::endl;
    TBox *box_inactive = new TBox(inactive_slot_crate1.at(i_ch)-1,inactive_ch_crate1.at(i_ch),inactive_slot_crate1.at(i_ch),inactive_ch_crate1.at(i_ch)+1);
    vector_box_inactive.push_back(box_inactive);
    box_inactive->SetFillStyle(3004);
    box_inactive->SetFillColor(1);
    inactive_crate1++;
  }

  for (unsigned int i_slot=0;i_slot<num_slots;i_slot++){
    if (active_channel[1][i_slot]==0){
      TBox *box_inactive = new TBox(i_slot,0,i_slot+1,num_channels);
      vector_box_inactive.push_back(box_inactive);
      box_inactive->SetFillColor(1);
      box_inactive->SetFillStyle(3004);
      inactive_crate2++;
    }
  }
  for (unsigned int i_ch = 0; i_ch < inactive_ch_crate2.size(); i_ch++){
    if (verbosity > 2) std::cout <<"inactive ch crate2, entry "<<i_ch<<std::endl;
    TBox *box_inactive = new TBox(inactive_slot_crate2.at(i_ch)-1,inactive_ch_crate2.at(i_ch),inactive_slot_crate2.at(i_ch),inactive_ch_crate2.at(i_ch)+1);
    vector_box_inactive.push_back(box_inactive);
    box_inactive->SetFillStyle(3004);
    box_inactive->SetFillColor(1);
    inactive_crate2++;
  }

  //-------------------------------------------------------
  //----------Initialize canvases--------------------------
  //-------------------------------------------------------

  c_FreqChannels = new TCanvas("MRD Freq ch","MRD TDC monitor Ch",900,600);
  c_FreqSlots = new TCanvas("MRD Freq slots","MRD TDC monitor Sl",900,600);
  c_FreqCrates = new TCanvas("MRD Freq crates","MRD TDC monitor Cr",900,600);
  c_Freq2D = new TCanvas("MRD Freq 2D","MRD TDC monitor 2D",1000,600);
  c_Times = new TCanvas("MRD_Times","MRD TDC monitor Times",900,600);
  c_loglive = new TCanvas("MRD Live Events","MRD Live Events",900,600);
  canvas_rates = new TCanvas("canvas_rates","Rates electronics space",1000,600);
  canvas_rates_hour = new TCanvas("canvas_rates_hour","Rates electronics space",1000,600);
  canvas_tdc_live = new TCanvas("canvas_tdc_live","Canvas TDC",900,600);
  canvas_tdc_hour = new TCanvas("canvas_tdc_hour","Canvas TDC (1 hour)",900,600);
  canvas_npaddles = new TCanvas("canvas_npaddles","Canvas NPaddles",900,600);
  canvas_npaddles_hour = new TCanvas("canvas_npaddles_hour","Canvas NPaddles (1 hour)",900,600);

  //-------------------------------------------------------
  //------Setup time variables for periodic updates--------
  //-------------------------------------------------------

  period_update = boost::posix_time::time_duration(0,0,30,0); //set update frequency for this tool to be higher than MonitorMRDTime (5mins->30sec)
  last = boost::posix_time::ptime(boost::posix_time::second_clock::local_time());
  Epoch = new boost::posix_time::ptime(boost::gregorian::from_string("1970/1/1"));

  //omit warning messages from ROOT: info messages - 1001, warning messages - 2001, error messages - 3001
  gROOT->ProcessLine("gErrorIgnoreLevel = 3001;");

  return true;

}


bool MonitorMRDLive::Execute(){

  if (verbosity > 2) std::cout <<"Tool MonitorMRDLive: Executing...."<<std::endl;

  //-------------------------------------------------------
  //---------------How much time passed?-------------------
  //-------------------------------------------------------

  current = (boost::posix_time::second_clock::local_time());
  duration = boost::posix_time::time_duration(current - last);

  if (verbosity > 2) std::cout <<"duration: "<<duration<<std::endl; 

  //-------------------------------------------------------
  //---------------Get live event info---------------------
  //-------------------------------------------------------

  bool has_cc;
  m_data->CStore.Get("HasCCData",has_cc);

  std::string State;
  m_data->CStore.Get("State",State);

  if (has_cc){
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

    std::string trigger_type = "no loopback";
    if (verbosity > 2) std::cout <<"MonitorMRDLive: Read in Data: >>>>>>>>>>>>>>>>>>> "<<std::endl;
    for (unsigned int i_entry = 0; i_entry < Channel.size(); i_entry++){
      if (verbosity > 2) std::cout <<"Crate "<<Crate.at(i_entry)<<", Slot "<<Slot.at(i_entry)<<", Channel "<<Channel.at(i_entry)<<std::endl;
      std::vector<int>::iterator it = std::find(nr_slot.begin(), nr_slot.end(), (Slot.at(i_entry))+(Crate.at(i_entry)-min_crate)*100);
      if (it == nr_slot.end()){
        std::cout <<"ERROR (MonitorMRDLive): Read-out Crate/Slot/Channel number not active according to configuration file. Check the configfile to process the data..."<<std::endl;
        std::cout <<"Crate: "<<Crate.at(i_entry)<<", Slot: "<<Slot.at(i_entry)<<std::endl;
        continue;
      }
      int active_slot_nr = std::distance(nr_slot.begin(),it);
      int ch = active_slot_nr*num_channels+Channel.at(i_entry);
      if (verbosity > 2) std::cout <<", ch nr: "<<ch<<", TDC: "<<Value.at(i_entry)<<", timestamp: "<<TimeStamp<<std::endl;
      live_tdc.at(ch).push_back(Value.at(i_entry));
      live_timestamp.at(ch).push_back(TimeStamp);
      live_tdc_hour.at(ch).push_back(Value.at(i_entry));
      live_timestamp_hour.at(ch).push_back(TimeStamp);
      for (unsigned int i_loopback=0; i_loopback< loopback_crate.size(); i_loopback++){
        if (Crate.at(i_entry) == loopback_crate.at(i_loopback) && Slot.at(i_entry) == loopback_slot.at(i_loopback) && Channel.at(i_entry) == loopback_channel.at(i_loopback)) trigger_type = loopback_name.at(i_loopback);
      }
    }

    vector_timestamp.push_back(TimeStamp);
    vector_timestamp_hour.push_back(TimeStamp);
    vector_nchannels.push_back(Channel.size());
    vector_nchannels_hour.push_back(Channel.size());
    vector_triggertype.push_back(trigger_type);
    current_stamp = TimeStamp;

    t = time(0);
    struct tm *now = localtime( & t );
    title_time.str("");
    title_time<<(now->tm_year + 1900) << '-' << (now->tm_mon + 1) << '-' <<  now->tm_mday<<','<<now->tm_hour<<':'<<now->tm_min<<':'<<now->tm_sec;

    //plot all the live monitoring plots
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
    //std::cout <<"MonitorMRDLive: List of Objects (after execute step)"<<std::endl;
    //gObjectTable->Print();


  } else if (State == "DataFile" || State == "Wait"){

    if (verbosity > 3) std::cout <<"Status File (Data File or Wait): MonitorMRDLive not executed..."<<std::endl;        

  } else {

    if (verbosity > 1) std::cout <<"State not recognized: "<<State<<std::endl;

  }
  }
  //plot the integrated rate monitoring plots

  if (verbosity > 2) {
    std::cout <<"duration: "<<duration<<std::endl;
    std::cout <<"current: "<<current<<std::endl;
  }

  if(duration>=period_update){

    if (verbosity > 0) std::cout <<"MonitorMRDLive: 30sec passed... Updating rate plots!"<<std::endl;
    last=current;
    MonitorMRDLive::EraseOldData();
    MonitorMRDLive::UpdateRatePlots();

  }

  return true;
  
}


bool MonitorMRDLive::Finalise(){

  if (verbosity > 2) std::cout <<"Tool MonitorMRDLive: Finalising..."<<std::endl;

  //delete all pointers that are still active

  for (unsigned int i_box = 0; i_box < vector_box_inactive.size(); i_box++){
    delete vector_box_inactive.at(i_box);
  }
  delete label_cr1;
  delete label_cr2;
  delete label_rate_cr1;
  delete label_rate_cr2;
  delete label_tdc;
  for (unsigned int i_line = 0; i_line < vector_lines.size(); i_line++){
    delete vector_lines.at(i_line);
  }
  for (unsigned int i_trigger =0; i_trigger < trigger_labels.size(); i_trigger++){
    delete trigger_labels.at(i_trigger);
  }

  delete hChannel_cr1;
  delete hChannel_cr2;
  delete hSlot_cr1;
  delete hSlot_cr2;
  delete hCrate;
  delete h2D_cr1;
  delete h2D_cr2;
  delete hTimes;
  delete rate_crate1;
  delete rate_crate2;
  delete rate_crate1_hour;
  delete rate_crate2_hour;
  delete TDC_hist;
  delete TDC_hist_hour;
  delete n_paddles_hit;
  delete n_paddles_hit_hour;

  delete c_FreqChannels;
  delete c_FreqSlots;
  delete c_FreqCrates;
  delete c_Freq2D;
  delete c_Times;
  delete canvas_tdc_live;
  delete canvas_tdc_hour;
  delete canvas_npaddles;
  delete canvas_npaddles_hour;
  delete canvas_rates;

  return true;
}


void MonitorMRDLive::MRDTDCPlots(){

  if (verbosity > 2) std::cout <<"Plotting MRD Single Event Monitors...."<<std::endl;

  double max_cr1, max_cr2, min_cr1, min_cr2, max_scale, min_scale;
  double max_slot = 0;
  double min_slot = 99999999;
  double max_ch = 0;
  double min_ch = 99999999; 

  double times_slots[num_crates][num_slots] = {0};
  double n_times_slots[num_crates][num_slots] = {0};

  for (unsigned int i=0;i<Value.size();i++){

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
      } else {
      std::cout <<"ERROR (MonitorMRDLive): Slot # "<<Slot.at(i)<<" is not connected according to the configuration file. Abort this entry..."<<std::endl;
      continue;
      }
    } else std::cout <<"ERROR (MonitorMRDLive): The read-in crate number does not exist. Continue with next event... "<<std::endl;
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
  c_FreqChannels->cd();

  min_cr1=hChannel_cr1->GetMinimum();
  min_cr2 = hChannel_cr2->GetMinimum();
  min_scale = (min_cr1<min_cr2)? min_cr1 : min_cr2;
  max_cr1=hChannel_cr1->GetMaximum();
  max_cr2 = hChannel_cr2->GetMaximum();
  max_scale = (max_cr1>max_cr2)? max_cr1 : max_cr2;
      
  hChannel_cr1->GetYaxis()->SetRangeUser(min_scale,max_scale+20);
  hChannel_cr1->SetTitle(title_time.str().c_str());
  hChannel_cr1->GetYaxis()->SetRangeUser(min_ch-5,max_ch+5);
  hChannel_cr2->GetYaxis()->SetRangeUser(min_ch-5,max_ch+5);
  c_FreqChannels->SetGridy();


  //std::cout <<"Draw hChannel_cr1 & hChannel_cr2"<<std::endl;
  hChannel_cr1->Draw();
  hChannel_cr2->Draw("same");
  TLine *separate_crates = new TLine(num_channels*n_active_slots_cr1,min_ch-5,num_channels*n_active_slots_cr1,max_ch+5);
  separate_crates->SetLineStyle(2);
  separate_crates->SetLineWidth(2);
  separate_crates->Draw("same");
  label_cr1->Draw();
  label_cr2->Draw();
  c_FreqChannels->Update();
  TF1 *f1 = new TF1("f1","x",0,num_active_slots);       //workaround to only have labels for every slot
  TGaxis *labels_grid = new TGaxis(0,c_FreqChannels->GetUymin(),num_active_slots*num_channels,c_FreqChannels->GetUymin(),"f1",num_active_slots,"w");
  labels_grid->SetLabelSize(0);
  labels_grid->Draw("w");
  std::stringstream ss_tmp;
  ss_tmp<<outpath<<"TDC_Channels_Live.jpg";
  if (draw_average) c_FreqChannels->SaveAs(ss_tmp.str().c_str());       //basically a duplicate of the 2D view, might bring more confusion than use


  if (verbosity > 2) std::cout <<"Creating Slot Frequency plot..."<<std::endl;
  c_FreqSlots->cd();
  hSlot_cr1->GetYaxis()->SetRangeUser(min_slot-5,max_slot+5);
  hSlot_cr1->SetTitle(title_time.str().c_str());
  c_FreqSlots->SetGridy();
  c_FreqSlots->SetGridx();
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
  c_FreqCrates->cd();
  c_FreqCrates->SetGridy();

  hCrate->SetTitle(title_time.str().c_str());
  c_FreqCrates->SetGridx();
  c_FreqCrates->SetGridy();
  hCrate->Draw();
  std::stringstream ss_crate;
  ss_crate<<outpath<<"TDC_Crates.jpg";
  if (draw_average) c_FreqCrates->SaveAs(ss_crate.str().c_str());

  if (verbosity > 2) std::cout <<"Creating 2D Frequency plot..."<<std::endl;
  c_Freq2D->cd();
  std::string str_2D = " (Live Event)";
  std::string title_2D = title_time.str()+str_2D;
  c_Freq2D->SetTitle(title_2D.c_str());
  c_Freq2D->Divide(2,1);
  TPad *p1 = (TPad*) c_Freq2D->cd(1);
  p1->SetGrid();
  h2D_cr1->Draw("colz");
  if (verbosity > 2) {
    std::cout <<"Drawing inactive boxes"<<std::endl;
    std::cout <<"vector_box_inactive.size(): "<<vector_box_inactive.size()<<std::endl;
  }
  for (int i_ch = 0; i_ch < inactive_crate1; i_ch++){
    vector_box_inactive.at(i_ch)->Draw("same");
  }
  if (verbosity > 2) std::cout <<"Updating p1"<<std::endl;
  p1->Update();
  if (h2D_cr1->GetEntries()!=0){
    if (min_ch == max_ch) h2D_cr1->GetZaxis()->SetRangeUser(min_ch-0.5,max_ch+0.5);
    else h2D_cr1->GetZaxis()->SetRangeUser(min_ch,max_ch);
    TPaletteAxis *palette = 
  (TPaletteAxis*)h2D_cr1->GetListOfFunctions()->FindObject("palette");
    palette->SetX1NDC(0.9);
    palette->SetX2NDC(0.92);
    palette->SetY1NDC(0.1);
    palette->SetY2NDC(0.9);
  }

  label_tdc->Draw();
  TPad *p2 = (TPad*) c_Freq2D->cd(2);
  p2->SetGrid();
  h2D_cr2->Draw("colz");
  for (int i_ch = 0; i_ch < inactive_crate2; i_ch++){
    vector_box_inactive.at(inactive_crate1+i_ch)->Draw("same");
  }
  p2->Update();
  if (h2D_cr2->GetEntries()!=0){
    if (min_ch == max_ch) h2D_cr2->GetZaxis()->SetRangeUser(min_ch-0.5,max_ch+0.5);
    else h2D_cr2->GetZaxis()->SetRangeUser(min_ch,max_ch);
    TPaletteAxis *palette = 
  (TPaletteAxis*)h2D_cr2->GetListOfFunctions()->FindObject("palette");
    palette->SetX1NDC(0.9);
    palette->SetX2NDC(0.92);
    palette->SetY1NDC(0.1);
    palette->SetY2NDC(0.9);
  }

  label_tdc->Draw();
  std::stringstream ss_2D;
  ss_2D<<outpath<<"MRD_TDC_Live_2D.jpg";
  c_Freq2D->SaveAs(ss_2D.str().c_str());

  if (verbosity > 2) std::cout <<"Creating TDC histogram ... "<<std::endl;
  c_Times->cd();
  std::string str_live_event = " (Live Event)";
  std::string title_Times = title_time.str()+str_live_event;
  hTimes->SetTitle(title_Times.c_str());
  if (hTimes->GetEntries() == 0) {
    TH1F *hTimes_temp = new TH1F("hTimes_temp",title_Times.c_str(),200,0,100);
    hTimes_temp->SetStats(0);
    hTimes_temp->GetXaxis()->SetTitle("TDC");
    hTimes_temp->Draw();
    std::stringstream ss_Times;
    ss_Times<<outpath<<"MRD_TDC_Live.jpg";
    c_Times->SaveAs(ss_Times.str().c_str());
    delete hTimes_temp;
  } else {
    hTimes->Draw();
    std::stringstream ss_Times;
    ss_Times<<outpath<<"MRD_TDC_Live.jpg";
    c_Times->SaveAs(ss_Times.str().c_str());
  }

  //create histogram showing the time stamps of the last live events

  int min_bin = (int(vector_triggertype.size())>n_bins_loglive)? vector_triggertype.size()-n_bins_loglive : 0;
  unsigned long time_diff = vector_timestamp.at(vector_triggertype.size()-1)-vector_timestamp.at(min_bin);

  for (int i_trigger = vector_triggertype.size(); i_trigger > min_bin; i_trigger--){

    int bin_nr = n_bins_loglive - vector_triggertype.size() + i_trigger;
    boost::posix_time::ptime labeltime = *Epoch + boost::posix_time::time_duration(int(vector_timestamp.at(i_trigger-1)/1000./60./60.),int(vector_timestamp.at(i_trigger-1)/1000./60.)%60,int(vector_timestamp.at(i_trigger-1)/1000.)%60,vector_timestamp.at(i_trigger-1)%1000);
    struct tm label_tm = boost::posix_time::to_tm(labeltime);
    std::stringstream ss_time;
    ss_time << label_tm.tm_hour<<':'<<label_tm.tm_min<<':'<<label_tm.tm_sec;
    log_live_events->GetXaxis()->SetBinLabel(bin_nr,ss_time.str().c_str());
    vector_lines.at(bin_nr-1)->SetLineColor(map_triggertype_color[vector_triggertype.at(i_trigger-1)]);

  }

  c_loglive->cd();
  log_live_events->Draw();
  for (unsigned int i_l = 0; i_l < vector_lines.size(); i_l++){
    vector_lines.at(i_l)->Draw("same");
  }
  for (unsigned int i_triglabel =0; i_triglabel < trigger_labels.size(); i_triglabel++){
    trigger_labels.at(i_triglabel)->Draw();
  }
  std::stringstream ss_loglive;
  ss_loglive << outpath << "MRD_LiveHistory.jpg";
  c_loglive->SaveAs(ss_loglive.str().c_str());
    
  //delete pointers that are not used anymore


  if (verbosity > 2) std::cout <<"Deleting pointers"<<std::endl;

  delete separate_crates;
  delete separate_crates2;
  delete f1;
  delete labels_grid;

  //clean up histograms and canvases

  if (verbosity > 2) std::cout <<"Resetting hists and clearing canvases"<<std::endl;

  hChannel_cr1->Reset();
  hChannel_cr2->Reset();   //two separate histograms for the channels of the two respective crates
  hSlot_cr1->Reset();
  hSlot_cr2->Reset();
  hCrate->Reset();        //average crate/slot information
  h2D_cr1->Reset();
  h2D_cr2->Reset();        //2D representation of channels, slots
  hTimes->Reset();
  log_live_events->Reset();

  c_FreqChannels->Clear();
  c_FreqSlots->Clear();
  c_FreqCrates->Clear();
  c_Freq2D->Clear();
  c_Times->Clear();
  c_loglive->Clear();

}

void MonitorMRDLive::InitializeVectors(){

  if (verbosity > 2) std::cout <<"MonitorMRDLive: InitializeVectors"<<std::endl;

  std::vector<unsigned int> vec_tdc, vec_tdc_hour;
  std::vector<ULong64_t> vec_timestamp, vec_timestamp_hour;
  for (int i_ch = 0; i_ch < num_active_slots*num_channels; i_ch++){
    live_tdc.push_back(vec_tdc);
    live_timestamp.push_back(vec_timestamp);
    live_tdc_hour.push_back(vec_tdc_hour);
    live_timestamp_hour.push_back(vec_timestamp_hour);
  }

}

void MonitorMRDLive::EraseOldData(){

  if (verbosity > 2) std::cout <<"MonitorMRDLive: EraseOldData"<<std::endl;

  for (int i_ch = 0; i_ch < num_active_slots*num_channels; i_ch++){
    for (unsigned int i=0; i<live_tdc.at(i_ch).size();i++){
      if (current_stamp - live_timestamp.at(i_ch).at(i) > integration_period*60*1000){
      live_tdc.at(i_ch).erase(live_tdc.at(i_ch).begin()+i);
      live_timestamp.at(i_ch).erase(live_timestamp.at(i_ch).begin()+i);
      }
    }
    for (unsigned int i=0; i<live_tdc_hour.at(i_ch).size();i++){
      if (current_stamp - live_timestamp_hour.at(i_ch).at(i) > integration_period_hour*60*1000){
        live_tdc_hour.at(i_ch).erase(live_tdc_hour.at(i_ch).begin()+i);
        live_timestamp_hour.at(i_ch).erase(live_timestamp_hour.at(i_ch).begin()+i);
      }
    }
  }
  for (unsigned int i_entry = 0; i_entry < vector_timestamp.size(); i_entry++){
    if (current_stamp - vector_timestamp.at(i_entry) > integration_period*60*1000){
      vector_timestamp.erase(vector_timestamp.begin()+i_entry);
      vector_nchannels.erase(vector_nchannels.begin()+i_entry);
      vector_triggertype.erase(vector_triggertype.begin()+i_entry);
    }
  }
  for (unsigned int i_entry = 0; i_entry < vector_timestamp_hour.size(); i_entry++){
    if (current_stamp - vector_timestamp_hour.at(i_entry) > integration_period_hour*60*1000){
      vector_timestamp_hour.erase(vector_timestamp_hour.begin()+i_entry);
      vector_nchannels_hour.erase(vector_nchannels_hour.begin()+i_entry);
    }
  }

}

void MonitorMRDLive::UpdateRatePlots(){

  if (verbosity > 2) std::cout <<"MonitorMRDLive: UpdateRatePlots"<<std::endl;

  t = time(0);
  struct tm *now = localtime( & t );
  title_time.str("");
  title_time<<(now->tm_year + 1900) << '-' << (now->tm_mon + 1) << '-' <<  now->tm_mday<<','<<now->tm_hour<<':'<<now->tm_min<<':'<<now->tm_sec;

  double min_ch = 99999999;
  double max_ch = 0;
  double min_ch_hour = 99999999;
  double max_ch_hour = 0;

  std::string tdc_title = "Live TDC ";
  std::string n_paddles_title = "Live N Paddles ";
  std::string str_fivemin = " (last 5 mins)";
  std::string str_hour = " (last hour)";
  std::string tdc_Title = tdc_title+title_time.str()+str_fivemin;
  std::string tdc_Title_hour = tdc_title+title_time.str()+str_hour;
  std::string n_paddles_Title = n_paddles_title+title_time.str()+str_fivemin;
  std::string n_paddles_Title_hour = n_paddles_title+title_time.str()+str_hour;

  TDC_hist->SetTitle(tdc_Title.c_str());
  TDC_hist_hour->SetTitle(tdc_Title_hour.c_str());
  n_paddles_hit->SetTitle(n_paddles_Title.c_str());
  n_paddles_hit_hour->SetTitle(n_paddles_Title_hour.c_str());

  for (int i_ch = 0; i_ch < num_active_slots*num_channels; i_ch++){
    int crate_id = (i_ch < n_active_slots_cr1*num_channels)? min_crate : min_crate+1;
    int slot_id;
    if (i_ch < n_active_slots_cr1*num_channels) slot_id = nr_slot.at(i_ch / num_channels);
    else slot_id = nr_slot.at(i_ch / num_channels) - 100.;
    int channel_id = i_ch % num_channels;
    if (verbosity > 2){
      std::cout <<"Crate: "<<crate_id<<", Slot: "<<slot_id<<", Channel: "<<channel_id<<std::endl;
      std::cout <<"live_tdc.size: "<<live_tdc.at(i_ch).size()<<", rate: "<<live_tdc.at(i_ch).size()/(integration_period*60)<<std::endl;
    }
    for (unsigned int i_entry= 0; i_entry < live_tdc.at(i_ch).size(); i_entry++){
      if (crate_id == min_crate) rate_crate1->SetBinContent(slot_id,channel_id+1,live_tdc.at(i_ch).size()/(integration_period*60));   //display in Hz
      else if (crate_id == min_crate+1) rate_crate2->SetBinContent(slot_id,channel_id+1,live_tdc.at(i_ch).size()/(integration_period*60));
      TDC_hist->Fill(live_tdc.at(i_ch).at(i_entry));
      if (live_tdc.at(i_ch).size()/(integration_period*60) > max_ch) max_ch = live_tdc.at(i_ch).size()/(integration_period*60); 
      if (live_tdc.at(i_ch).size()/(integration_period*60) < min_ch) min_ch = live_tdc.at(i_ch).size()/(integration_period*60); 
    }
    for (unsigned int i_entry = 0; i_entry < live_tdc_hour.at(i_ch).size(); i_entry++){
      if (crate_id == min_crate) rate_crate1_hour->SetBinContent(slot_id,channel_id+1,live_tdc_hour.at(i_ch).size()/(integration_period_hour*60));
      else if (crate_id == min_crate+1) rate_crate2_hour->SetBinContent(slot_id,channel_id+1,live_tdc_hour.at(i_ch).size()/(integration_period_hour*60));
      TDC_hist_hour->Fill(live_tdc_hour.at(i_ch).at(i_entry));
      if (live_tdc_hour.at(i_ch).size()/(integration_period_hour*60) > max_ch_hour) max_ch_hour = live_tdc_hour.at(i_ch).size()/(integration_period_hour*60);
      if (live_tdc_hour.at(i_ch).size()/(integration_period_hour*60) < min_ch_hour) min_ch_hour = live_tdc_hour.at(i_ch).size()/(integration_period_hour*60);
    }
  }

  for (unsigned int i_entry = 0; i_entry < vector_nchannels.size(); i_entry++){
    n_paddles_hit->Fill(vector_nchannels.at(i_entry));
  }

  for (unsigned int i_entry = 0; i_entry < vector_nchannels_hour.size(); i_entry++){
    n_paddles_hit_hour->Fill(vector_nchannels_hour.at(i_entry));
  }

  //first plot the rate 2D histograms (most work)

  canvas_rates->cd();
  canvas_rates->SetTitle(title_time.str().c_str());
  canvas_rates->Divide(2,1);

  TPad *p1 = (TPad*) canvas_rates->cd(1);
  p1->SetGrid();
  rate_crate1->SetTitle("Live Rates Rack 7 (5 mins)");
  rate_crate1->Draw("colz");
  for (int i_ch = 0; i_ch < inactive_crate1; i_ch++){
    vector_box_inactive.at(i_ch)->Draw("same");
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

  label_rate_cr1->Draw();

  rate_crate2->SetTitle("Live Rates Rack 8 (5 mins)");
  TPad *p2 = (TPad*) canvas_rates->cd(2);
  p2->SetGrid();
  rate_crate2->Draw("colz");
  for (int i_ch = 0; i_ch < inactive_crate2; i_ch++){
    vector_box_inactive.at(inactive_crate1+i_ch)->Draw("same");
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
  label_rate_cr2->Draw();

  //p2->Modified();
  std::stringstream ss_rate_electronics;
  ss_rate_electronics<<outpath<<"MRD_Rates_Live_5min.jpg";
  canvas_rates->SaveAs(ss_rate_electronics.str().c_str());

  //Also plot the rate 2D histograms for 1 hour

  canvas_rates_hour->cd();
  canvas_rates_hour->SetTitle(title_time.str().c_str());
  canvas_rates_hour->Divide(2,1);

  TPad *p1_hour = (TPad*) canvas_rates_hour->cd(1);
  p1_hour->SetGrid();
  rate_crate1_hour->SetTitle("Live Rates Rack 7 (1 hour)");
  rate_crate1_hour->Draw("colz");

  //color inactive channels in grey
  for (int i_ch = 0; i_ch < inactive_crate1; i_ch++){
    vector_box_inactive.at(i_ch)->Draw("same");
  }  
  p1_hour->Update();

  if (rate_crate1_hour->GetEntries()!=0){
    if (min_ch_hour == max_ch_hour) rate_crate1_hour->GetZaxis()->SetRangeUser(min_ch_hour-0.5,max_ch_hour+0.5);
    else rate_crate1_hour->GetZaxis()->SetRangeUser(min_ch_hour,max_ch_hour);
    TPaletteAxis *palette_hour = 
    (TPaletteAxis*) rate_crate1_hour->GetListOfFunctions()->FindObject("palette");
    palette_hour->SetX1NDC(0.9);
    palette_hour->SetX2NDC(0.92);
    palette_hour->SetY1NDC(0.1);
    palette_hour->SetY2NDC(0.9);
  }
  label_rate_cr1->Draw();

  TPad *p2_hour = (TPad*) canvas_rates_hour->cd(2);
  p2_hour->SetGrid();
  rate_crate2_hour->Draw("colz");
  for (int i_ch = 0; i_ch < inactive_crate2; i_ch++){
    vector_box_inactive.at(inactive_crate1+i_ch)->Draw("same");
  }
  p2_hour->Update();

  if (rate_crate2_hour->GetEntries()!=0 ){
    if (min_ch_hour == max_ch_hour) rate_crate2_hour->GetZaxis()->SetRangeUser(min_ch_hour-0.5,max_ch_hour+0.5);
    else rate_crate2_hour->GetZaxis()->SetRangeUser(min_ch_hour,max_ch_hour);
    TPaletteAxis *palette_hour = 
    (TPaletteAxis*)rate_crate2_hour->GetListOfFunctions()->FindObject("palette");
    palette_hour->SetX1NDC(0.9);
    palette_hour->SetX2NDC(0.92);
    palette_hour->SetY1NDC(0.1);
    palette_hour->SetY2NDC(0.9);
  }

  label_rate_cr2->Draw();
  std::stringstream ss_rate_electronics_hour;
  ss_rate_electronics_hour<<outpath<<"MRD_Rates_Live_1hour.jpg";
  canvas_rates_hour->SaveAs(ss_rate_electronics_hour.str().c_str());

  //plot the other histograms as well

  canvas_tdc_live->cd();
  TDC_hist->Draw();
  canvas_tdc_hour->cd();
  TDC_hist_hour->Draw();
  canvas_npaddles->cd();
  n_paddles_hit->Draw();
  canvas_npaddles_hour->cd();
  n_paddles_hit_hour->Draw();

  //save TDC & NPaddles plot
  std::stringstream ss_tdc;
  ss_tdc<<outpath<<"MRD_TDC_Live_5min.jpg";
  canvas_tdc_live->SaveAs(ss_tdc.str().c_str());
  std::stringstream ss_tdc_hour;
  ss_tdc_hour<<outpath<<"MRD_TDC_Live_1hour.jpg";
  canvas_tdc_hour->SaveAs(ss_tdc_hour.str().c_str());
  std::stringstream ss_npaddles;
  ss_npaddles<<outpath<<"MRD_NPaddles_Live_5min.jpg";
  canvas_npaddles->SaveAs(ss_npaddles.str().c_str());
  std::stringstream ss_npaddles_hour;
  ss_npaddles_hour<<outpath<<"MRD_NPaddles_Live_1hour.jpg";
  canvas_npaddles_hour->SaveAs(ss_npaddles_hour.str().c_str());

  //cleanup pointers for histograms and canvases

  rate_crate1->Reset();
  rate_crate2->Reset();
  rate_crate1_hour->Reset();
  rate_crate2_hour->Reset();
  TDC_hist->Reset();
  TDC_hist_hour->Reset();
  n_paddles_hit->Reset();
  n_paddles_hit_hour->Reset();
  canvas_rates->Clear();
  canvas_rates_hour->Clear();
  canvas_tdc_live->Clear();
  canvas_tdc_hour->Clear();
  canvas_npaddles->Clear();
  canvas_npaddles_hour->Clear();

  //get list of allocated objects (ROOT)
  //std::cout <<"MonitorMRDLive: List of Objects (End of UpdateRatePlots)"<<std::endl;
  //gObjectTable->Print();

}
