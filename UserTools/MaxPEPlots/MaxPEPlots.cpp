#include "MaxPEPlots.h"

MaxPEPlots::MaxPEPlots():Tool(){}


bool MaxPEPlots::Initialise(std::string configfile, DataModel &data){

  /////////////////// Useful header ///////////////////////
  if(configfile!="") m_variables.Initialise(configfile); // loading config file
  //m_variables.Print();

  m_data= &data; //assigning transient data pointer
  /////////////////////////////////////////////////////////////////

  std::string thr_file;

  m_variables.Get("MaxPEFilename",maxpe_filename);
  m_variables.Get("ThresholdFile",thr_file);
  m_variables.Get("verbosity",verbosity);

  ifstream file_thresholds(thr_file);
  int temp_chkey, temp_baseline, temp_mpe;
  while (!file_thresholds.eof()){
    file_thresholds >> temp_chkey >> temp_baseline >> temp_mpe;
    map_baseline[temp_chkey] = temp_baseline;
    map_mpe[temp_chkey] = temp_mpe;
    if (file_thresholds.eof()) break;
  }
  file_thresholds.close();


  f_maxpe = new TFile(maxpe_filename.c_str(),"RECREATE");
  t_maxpe = new TTree("t_maxpe","Max PE");
  t_maxpe->Branch("max_pe_global",&max_pe_global);
  t_maxpe->Branch("max_pe",&max_pe);
  t_maxpe->Branch("extended",&extended); 
  t_maxpe->Branch("extended_trig",&extended_trig);
  t_maxpe->Branch("triggerword",&triggerword); 
  t_maxpe->Branch("baseline",&baseline);
  t_maxpe->Branch("amplitude",&amplitude);
  t_maxpe->Branch("chkey",&chkey);
  t_maxpe->Branch("n_mpe",&n_mpe);

  h_maxpe = new TH1F("h_maxpe","Max P.E. distribution",5000,0,1000);
  h_maxpe_prompt = new TH1F("h_maxpe_prompt","Max P.E. distribution (prompt)",5000,0,1000);
  h_maxpe_delayed = new TH1F("h_maxpe_delayed","Max P.E. distribution (extended)",5000,0,1000);
  h_maxpe_chankey = new TH2F("h_maxpe_chankey","Max P.E. distribution vs. chankey",132,332,464,500,0,500);  
  h_maxpe_prompt_chankey = new TH2F("h_maxpe_prompt_chankey","Max P.E. distribution (prompt) vs. chankey",132,332,464,500,0,500);  
  h_maxpe_delayed_chankey = new TH2F("h_maxpe_delayed_chankey","Max P.E. distribution (delayed) vs. chankey",132,332,464,500,0,500);  
  h_maxpe_trigword5 = new TH1F("h_maxpe_trigword5","Max P.E. distribution (Trgword 5)",5000,0,1000);
  h_maxpe_prompt_trigword5 = new TH1F("h_maxpe_prompt_trigword5","Max P.E. distribution (prompt, Trgword 5)",5000,0,1000);
  h_maxpe_delayed_trigword5 = new TH1F("h_maxpe_delayed_trigword5","Max P.E. distribution (extended, Trgword 5)",5000,0,1000);
  h_maxpe_chankey_trigword5 = new TH2F("h_maxpe_chankey_trigword5","Max P.E. distribution vs. chankey (Trgword 5)",132,332,464,500,0,500);  
  h_maxpe_prompt_chankey_trigword5 = new TH2F("h_maxpe_prompt_chankey_trigword5","Max P.E. distribution (prompt) vs. chankey (Trgword 5)",132,332,464,500,0,500);  
  h_maxpe_delayed_chankey_trigword5 = new TH2F("h_maxpe_delayed_chankey_trigword5","Max P.E. distribution (delayed) vs. chankey (Trgword 5)",132,332,464,500,0,500);  
  h_multiplicity_prompt_mpe = new TH1F("h_multiplicity_prompt_mpe","Multiplicity of PMTs above MPE threshold (prompt window)",140,0,140);
  h_multiplicity_delayed_mpe = new TH1F("h_multiplicity_delayed_mpe","Multiplicity of PMTs above MPE threshold (extended window)",140,0,140);
  h_multiplicity_prompt_mpe_extended0 = new TH1F("h_multiplicity_prompt_mpe_extended0","Multiplicity of PMTs above MPE threshold (prompt window, trig: no extended)",140,0,140);
  h_multiplicity_delayed_mpe_extended0 = new TH1F("h_multiplicity_delayed_mpe_extended0","Multiplicity of PMTs above MPE threshold (extended window, trig: no extended)",140,0,140);
  h_multiplicity_prompt_mpe_extended1 = new TH1F("h_multiplicity_prompt_mpe_extended1","Multiplicity of PMTs above MPE threshold (prompt window, trig: CC extended)",140,0,140);
  h_multiplicity_delayed_mpe_extended1 = new TH1F("h_multiplicity_delayed_mpe_extended1","Multiplicity of PMTs above MPE threshold (extended window, trig: CC extended)",140,0,140);
  h_multiplicity_prompt_mpe_extended2 = new TH1F("h_multiplicity_prompt_mpe_extended2","Multiplicity of PMTs above MPE threshold (prompt window, trig: Non-CC extended)",140,0,140);
  h_multiplicity_delayed_mpe_extended2 = new TH1F("h_multiplicity_delayed_mpe_extended2","Multiplicity of PMTs above MPE threshold (extended window, trig: Non-CC extended)",140,0,140);
  h_multiplicity_prompt_mpe_5pe = new TH1F("h_multiplicity_prompt_mpe_5pe","Multiplicity of PMTs above MPE threshold (prompt window, > 5p.e.)",140,0,140);
  h_multiplicity_delayed_mpe_5pe = new TH1F("h_multiplicity_delayed_mpe_5pe","Multiplicity of PMTs above MPE threshold (extended window,<5p.e.)",140,0,140);
  h_baseline_diff = new TH1F("h_baseline_diff","Baseline difference",500,0,500);
  h_chankey_prompt_mpe = new TH1F("h_chankey_prompt_mpe","Chankey PMTs above MPE threshold (prompt window)",140,330,470);
  h_chankey_delayed_mpe = new TH1F("h_chankey_delayed_mpe","Chankey PMTs above MPE threshold (extended window)",140,330,470);
  h_chankey_prompt_mpe_5pe = new TH1F("h_chankey_prompt_mpe_5pe","Chankey PMTs above MPE threshold (prompt window, > 5p.e.)",140,330,470);
  h_chankey_delayed_mpe_5pe = new TH1F("h_chankey_delayed_mpe_5pe","Chankey PMTs above MPE threshold (extended window, < 5p.e.)",140,330,470);
  h_maxpe_mpe_prompt = new TH2F("h_maxpe_mpe_prompt","# MPE channels vs. Max P.E. (prompt)",200,0,1000,100,0,140);
  h_maxpe_mpe_extended = new TH2F("h_maxpe_mpe_extended","# MPE channels vs. Max P.E. (extended)",200,0,1000,100,0,140);
  h_eventtypes_trig = new TH1F("h_eventtypes_trig","Trigger event types (extended)",4,0,4);
  h_window_opened = new TH1F("h_window_opened","Extended window opened",6,0,6);
  h_extended_trig = new TH1F("h_extended_trig","Extended window triggered",6,0,6);
  h_nowindow = new TH1F("h_nowindow","No VME extended window",2,0,2);

  m_data->Stores["ANNIEEvent"]->Header->Get("AnnieGeometry",geom);
  n_tank_pmts = geom->GetNumDetectorsInSet("Tank");
  std::map<std::string, std::map<unsigned long,Detector*>>* Detectors = geom->GetDetectors();

  for (std::map<unsigned long,Detector*>::iterator it  = Detectors->at("Tank").begin(); it != Detectors->at("Tank").end(); ++it){

    Detector* apmt = it->second;
    unsigned long detkey = it->first;
    pmt_detkeys.push_back(detkey);
    PMT_maxpe.insert(std::pair<unsigned long, double>(detkey,0.));
  }

  m_data->CStore.Get("ChannelNumToTankPMTSPEChargeMap",ChannelNumToTankPMTSPEChargeMap);

 /*
  std::vector<TH1F*> h_maxpe_prompt;
  std::vector<TH1F*> h_maxpe_delayed;
*/

  for (int i_pmt = 0; i_pmt < (int) pmt_detkeys.size(); i_pmt++){
    std::stringstream ss_baseline_prompt, ss_baseline_delayed, ss_amplitude_prompt, ss_amplitude_delayed;
    std::stringstream ss_title_baseline_prompt, ss_title_baseline_delayed, ss_title_amplitude_prompt, ss_title_amplitude_delayed;
    std::stringstream ss_time_pulse, ss_title_time_pulse;
    ss_baseline_prompt << "h_baseline_prompt_"<<i_pmt+332;
    ss_baseline_delayed << "h_baseline_delayed_"<<i_pmt+332;
    ss_amplitude_prompt << "h_amplitude_prompt_"<<i_pmt+332;
    ss_amplitude_delayed << "h_amplitude_delayed_"<<i_pmt+332;
    ss_title_baseline_prompt << "Baseline Prompt Chankey "<<i_pmt+332;
    ss_title_baseline_delayed << "Baseline Extended Chankey "<<i_pmt+332;
    ss_title_amplitude_prompt << "Amplitude Prompt Chankey "<<i_pmt+332;
    ss_title_amplitude_delayed << "Amplitude Extended Chankey "<<i_pmt+332;
    ss_time_pulse << "h_time_pulse_"<<i_pmt+332;
    ss_title_time_pulse << "Time Pulses Chankey "<<i_pmt+332;
    TH1F *h_baseline_prompt = new TH1F(ss_baseline_prompt.str().c_str(),ss_title_baseline_prompt.str().c_str(),300,300,400);
    TH1F *h_baseline_delayed = new TH1F(ss_baseline_delayed.str().c_str(),ss_title_baseline_delayed.str().c_str(),300,300,400);
    TH1F *h_amplitude_prompt = new TH1F(ss_amplitude_prompt.str().c_str(),ss_title_amplitude_prompt.str().c_str(),5000,300,4000);
    TH1F *h_amplitude_delayed = new TH1F(ss_amplitude_delayed.str().c_str(),ss_title_amplitude_delayed.str().c_str(),5000,300,4000);
    TH1F *h_time_pulse = new TH1F(ss_time_pulse.str().c_str(),ss_title_time_pulse.str().c_str(),200,0,2000);
    hv_baseline_prompt.push_back(h_baseline_prompt);
    hv_baseline_delayed.push_back(h_baseline_delayed);
    hv_amplitude_prompt.push_back(h_amplitude_prompt);
    hv_amplitude_delayed.push_back(h_amplitude_delayed);
    hv_time_pulse.push_back(h_time_pulse);
  }


  return true;
}


bool MaxPEPlots::Execute(){

  max_pe_global = 0;
  max_pe.clear();
  baseline.clear();
  amplitude.clear();
  chkey.clear();

  int annieeventexists = m_data->Stores.count("ANNIEEvent");
  if(!annieeventexists){ cerr<<"no ANNIEEvent store!"<<endl;}

  //Get Data Streams for event
  bool got_datastreams = m_data->Stores["ANNIEEvent"]->Get("DataStreams",DataStreams);
  if (!got_datastreams){
    std::cout <<"No DataStreams in ANNIEEvent store!"<<std::endl;
    return false;
  }
  if (DataStreams["Tank"]==0) return true;	//Don't do anything if no tank hits

  /*std::map<unsigned long, std::vector<Waveform<unsigned short>>> raw_waveform_map;
  bool has_raw = m_data->Stores["ANNIEEvent"]->Get("RawADCData",raw_waveform_map);
  std::cout <<"has_raw: "<<has_raw<<std::endl;
  if (!has_raw) {
    Log("RunValidation tool: Did not find RawADCData in ANNIEEvent! Abort",v_error,verbosity);
    return false;
  }
  extended = false;
  int size_of_window = 2000;
  if (has_raw){
    for (auto& temp_pair : raw_waveform_map) {
      const auto& achannel_key = temp_pair.first;
      auto& araw_waveforms = temp_pair.second;
      for (unsigned int i=0; i< araw_waveforms.size(); i++){
        auto samples = araw_waveforms.at(i).GetSamples();
        int size_sample = 2*samples->size();
        if (size_sample > size_of_window) {
          size_of_window = size_sample;
          extended = true;
        }
      }
    }
  }*/

  std::map<unsigned long, std::vector<int>> raw_acqsize_map;
  bool has_raw = m_data->Stores["ANNIEEvent"]->Get("RawAcqSize",raw_acqsize_map);
  std::cout <<"has_raw: "<<has_raw<<std::endl;
  if (!has_raw) {
    Log("RunValidation tool: Did not find RawAcqSize in ANNIEEvent! Abort",v_error,verbosity);
    return false;
  }
  extended = false;
  int size_of_window = 2000;
  if (has_raw){
    for (auto& temp_pair : raw_acqsize_map) {
      const auto& achannel_key = temp_pair.first;
      auto& araw_acqsize = temp_pair.second;
      for (unsigned int i=0; i< araw_acqsize.size(); i++){
        int size_sample = 2*araw_acqsize.at(i);
        if (size_sample > size_of_window) {
          size_of_window = size_sample;
          extended = true;
        }
      }
    }
  }

  //Get extended triggerword information (0: no extended window, 1: CC extended window, 2: Non-CC extended window)
  int TriggerExtended;
  m_data->Stores["ANNIEEvent"]->Get("TriggerExtended",TriggerExtended);


  std::map<unsigned long, std::vector<std::vector<ADCPulse>>> RecoADCHits;
  bool got_recoadc = m_data->Stores["ANNIEEvent"]->Get("RecoADCData",RecoADCHits);

  n_mpe = 0;
  std::vector<int> chankey_thr;
  
  bool got_trigword = m_data->Stores["ANNIEEvent"]->Get("TriggerWord",triggerword);
  if (!got_trigword){
    std::cout <<"No trgword in ANNIEEvent!" << std::endl;
  }

  if (got_recoadc){

    int recoadcsize = RecoADCHits.size();
    int adc_loop = 0;
    if (verbosity > 0) std::cout <<"RecoADCHits size: "<<recoadcsize<<std::endl;
    for (std::pair<unsigned long, std::vector<std::vector<ADCPulse>>> apair : RecoADCHits){
      unsigned long chankey = apair.first;
      Detector *thistube = geom->ChannelToDetector(chankey);
      int detectorkey = thistube->GetDetectorID();
      int vecid = detectorkey-332;
      if (thistube->GetDetectorElement()=="Tank"){
        std::vector<std::vector<ADCPulse>> pulses = apair.second;
        for (int i_minibuffer = 0; i_minibuffer < pulses.size(); i_minibuffer++){
          std::vector<ADCPulse> apulsevector = pulses.at(i_minibuffer);
          for (int i_pulse=0; i_pulse < apulsevector.size(); i_pulse++){
            ADCPulse apulse = apulsevector.at(i_pulse);
            double start_time = apulse.start_time();
            double peak_time = apulse.peak_time();
            double temp_baseline = apulse.baseline();
            double sigma_baseline = apulse.sigma_baseline();
            double cal_amplitude = apulse.amplitude();
            double temp_amplitude = apulse.raw_amplitude();
            double raw_area = apulse.raw_area();
            baseline.push_back(temp_baseline);
            amplitude.push_back(temp_amplitude);
            chkey.push_back(detectorkey);
            if (temp_amplitude > map_mpe[detectorkey]) {
              if (std::find(chankey_thr.begin(),chankey_thr.end(),detectorkey) == chankey_thr.end()) {
                n_mpe++;
                chankey_thr.push_back(detectorkey);
              }
            }
            if (triggerword==5){
            hv_time_pulse.at(vecid)->Fill(peak_time);
            if (extended) {
              hv_baseline_delayed.at(vecid)->Fill(temp_baseline);
              hv_amplitude_delayed.at(vecid)->Fill(temp_amplitude);
            } else {
              hv_baseline_prompt.at(vecid)->Fill(temp_baseline);
              hv_amplitude_prompt.at(vecid)->Fill(temp_amplitude);
            }
            }
          }
        }
      }
    }
  }

  if (triggerword == 5){
    if (TriggerExtended == 0) {
      h_eventtypes_trig->Fill(0);
      if (!extended) h_window_opened->Fill(0);
      else h_window_opened->Fill(1);
      if (chankey_thr.size()==0) h_extended_trig->Fill(0);
      else if (chankey_thr.size() > 0) h_extended_trig->Fill(1);
    }
    else if (TriggerExtended == 1) {
      h_eventtypes_trig->Fill(1); 
      h_eventtypes_trig->Fill(2);
      if (extended) h_window_opened->Fill(2);
      else h_window_opened->Fill(3);
      if (chankey_thr.size() > 0) h_extended_trig->Fill(2);
      else h_extended_trig->Fill(3);
    }
    else if (TriggerExtended == 2) {
      h_eventtypes_trig->Fill(1); 
      h_eventtypes_trig->Fill(3);
      if (extended) h_window_opened->Fill(4);
      else h_window_opened->Fill(5);
      if (chankey_thr.size() == 0) h_extended_trig->Fill(4);
      else if (chankey_thr.size() > 0) h_extended_trig->Fill(5);
    }
  if (extended) {
    if (chankey_thr.size() > 0) h_nowindow->Fill(0);
    h_multiplicity_delayed_mpe->Fill(n_mpe);
    if (TriggerExtended == 0) h_multiplicity_delayed_mpe_extended0->Fill(n_mpe);
    else if (TriggerExtended == 1) h_multiplicity_delayed_mpe_extended1->Fill(n_mpe);
    else if (TriggerExtended == 2) h_multiplicity_delayed_mpe_extended2->Fill(n_mpe);
    for (int i_ch=0; i_ch < chankey_thr.size(); i_ch++){
      h_chankey_delayed_mpe->Fill(chankey_thr.at(i_ch));
    }
  }
  else {
    if (chankey_thr.size() > 0) h_nowindow->Fill(1);
    h_multiplicity_prompt_mpe->Fill(n_mpe);
    if (TriggerExtended == 0) h_multiplicity_prompt_mpe_extended0->Fill(n_mpe);
    else if (TriggerExtended == 1) h_multiplicity_prompt_mpe_extended1->Fill(n_mpe);
    else if (TriggerExtended == 2) h_multiplicity_prompt_mpe_extended2->Fill(n_mpe);
    for (int i_ch=0; i_ch < chankey_thr.size(); i_ch++){
      h_chankey_prompt_mpe->Fill(chankey_thr.at(i_ch));
    }
  }
  }

  bool got_evnum = m_data->Stores["ANNIEEvent"]->Get("EventNumber", evnum);
  if (!got_evnum){
    std::cout <<"No EventNumber in ANNIEEvent!" << std::endl;
  }

  bool got_hits = m_data->Stores["ANNIEEvent"]->Get("Hits", Hits);
  if (!got_hits){
    std::cout << "No Hits store in ANNIEEvent! " << std::endl;
    return false;
  }


  for (int i_pmt = 0; i_pmt < n_tank_pmts; i_pmt++){
    unsigned long detkey = pmt_detkeys[i_pmt];
    PMT_maxpe[detkey] = 0;
  }

  std::cout <<"extended: "<<extended<<std::endl;

  int vectsize = Hits->size();
  for(std::pair<unsigned long, std::vector<Hit>>&& apair : *Hits){
    unsigned long chankey = apair.first;
    Detector* thistube = geom->ChannelToDetector(chankey);
    int detectorkey = thistube->GetDetectorID();
    if (thistube->GetDetectorElement()=="Tank"){
      std::vector<Hit>& ThisPMTHits = apair.second;
      for (Hit &ahit : ThisPMTHits){
        double thistime = ahit.GetTime();
        if (thistime > 2000) continue;
        double thischarge = ahit.GetCharge();
        double thischarge_pe = thischarge/ChannelNumToTankPMTSPEChargeMap->at(detectorkey);
        if (thischarge_pe > max_pe_global) max_pe_global = thischarge_pe;
        if (thischarge_pe > PMT_maxpe[detectorkey]) PMT_maxpe[detectorkey] = thischarge_pe;
        if (verbosity > 2) std::cout << "Key: " << detectorkey << ", charge "<<ahit.GetCharge()<<", time "<<ahit.GetTime()<<std::endl;
      }
    }
  }

  h_maxpe->Fill(max_pe_global);
  if (!extended) h_maxpe_prompt->Fill(max_pe_global);
  else h_maxpe_delayed->Fill(max_pe_global);


  if (triggerword == 5){
    h_maxpe_trigword5->Fill(max_pe_global);
    if (!extended) {
      h_maxpe_prompt_trigword5->Fill(max_pe_global);
      h_maxpe_mpe_prompt->Fill(max_pe_global,n_mpe);
    }
    else {
      h_maxpe_delayed_trigword5->Fill(max_pe_global);
      h_maxpe_mpe_extended->Fill(max_pe_global,n_mpe);
    }
    if (extended && max_pe_global < 5.) {
      h_multiplicity_delayed_mpe_5pe->Fill(n_mpe);
      for (int i_ch=0; i_ch < chankey_thr.size(); i_ch++){
        h_chankey_delayed_mpe_5pe->Fill(chankey_thr.at(i_ch));
      }
    }
    else if (!extended && max_pe_global > 5.){
      h_multiplicity_prompt_mpe_5pe->Fill(n_mpe);
      for (int i_ch=0; i_ch < chankey_thr.size(); i_ch++){
        h_chankey_prompt_mpe_5pe->Fill(chankey_thr.at(i_ch));
      }
    }
  }

  for (int i_pmt = 0; i_pmt < n_tank_pmts; i_pmt++){
    unsigned long detkey = pmt_detkeys[i_pmt];
    h_maxpe_chankey->Fill(detkey,PMT_maxpe[detkey]);
    if (!extended) h_maxpe_prompt_chankey->Fill(detkey,PMT_maxpe[detkey]);
    else h_maxpe_delayed_chankey->Fill(detkey,PMT_maxpe[detkey]);
    max_pe.push_back(PMT_maxpe[detkey]);
    if (triggerword == 5){
      h_maxpe_chankey_trigword5->Fill(detkey,PMT_maxpe[detkey]);
      if (!extended) h_maxpe_prompt_chankey_trigword5->Fill(detkey,PMT_maxpe[detkey]);
      else h_maxpe_delayed_chankey_trigword5->Fill(detkey,PMT_maxpe[detkey]);
    }
  }

  t_maxpe->Fill();

  return true;
}


bool MaxPEPlots::Finalise(){

  for (int i_pmt=0; i_pmt < (int) hv_baseline_prompt.size(); i_pmt++){
    double fit_mean=0;
    if (hv_baseline_prompt.at(i_pmt)->GetEntries()>0){
      double mean =hv_baseline_prompt.at(i_pmt)->GetMean();
      hv_baseline_prompt.at(i_pmt)->Fit("gaus","","",mean-10,mean+10);
      TF1 *fit = hv_baseline_prompt.at(i_pmt)->GetFunction("gaus");
      fit_mean = fit->GetParameter(1);
    }
    double baseline_diff = fit_mean - map_baseline[i_pmt+332];
    h_baseline_diff->SetBinContent(i_pmt+333,baseline_diff);
  }

  //Set labels for histograms
  h_eventtypes_trig->GetXaxis()->SetBinLabel(1,"Prompt window");
  h_eventtypes_trig->GetXaxis()->SetBinLabel(2,"Extended window");
  h_eventtypes_trig->GetXaxis()->SetBinLabel(3,"Extended window - CC");
  h_eventtypes_trig->GetXaxis()->SetBinLabel(4,"Extended window - NC");
  h_window_opened->GetXaxis()->SetBinLabel(1,"Prompt trig - prompt");
  h_window_opened->GetXaxis()->SetBinLabel(2,"Prompt trig - extended");
  h_window_opened->GetXaxis()->SetBinLabel(3,"Extended CC trig - extended");
  h_window_opened->GetXaxis()->SetBinLabel(4,"Extended CC trig - prompt");
  h_window_opened->GetXaxis()->SetBinLabel(5,"Extended NC trig - extended");
  h_window_opened->GetXaxis()->SetBinLabel(6,"Extended NC trig - prompt");
  h_extended_trig->GetXaxis()->SetBinLabel(1,"Prompt trig - no thr condition");
  h_extended_trig->GetXaxis()->SetBinLabel(2,"Prompt trig - thr condition");
  h_extended_trig->GetXaxis()->SetBinLabel(3,"Extended CC trig - thr condition");
  h_extended_trig->GetXaxis()->SetBinLabel(4,"Extended CC trig - no thr condition");
  h_extended_trig->GetXaxis()->SetBinLabel(5,"Extended NC trig - no thr condition");
  h_extended_trig->GetXaxis()->SetBinLabel(6,"Extended NC trig - thr condition");
  h_nowindow->GetXaxis()->SetBinLabel(1,"Extended window - observed");
  h_nowindow->GetXaxis()->SetBinLabel(2,"Extended window - not observed");

  TH1F *h_multiplicity_prompt_delayed_mpe = (TH1F*) h_multiplicity_prompt_mpe->Clone();
  h_multiplicity_prompt_delayed_mpe->SetName("h_multiplicity_prompt_delayed_mpe");
  h_multiplicity_prompt_delayed_mpe->Sumw2();
  h_multiplicity_prompt_delayed_mpe->Add(h_multiplicity_delayed_mpe);
  TH1F *h_multiplicity_ratio_mpe = (TH1F*) h_multiplicity_delayed_mpe->Clone();
  h_multiplicity_ratio_mpe->SetName("h_multiplicity_ratio_mpe");
  h_multiplicity_ratio_mpe->Sumw2();
  h_multiplicity_ratio_mpe->Divide(h_multiplicity_prompt_delayed_mpe);


  f_maxpe->Write();
  f_maxpe->Close();
  delete f_maxpe;

  return true;
}
