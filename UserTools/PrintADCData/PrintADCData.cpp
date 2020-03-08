#include "PrintADCData.h"

PrintADCData::PrintADCData():Tool(){}


bool PrintADCData::Initialise(std::string configfile, DataModel &data){

  std::cout << "PrintADCData: tool initializing" << std::endl;
  /////////////////// Useful header ///////////////////////
  if(configfile!="") m_variables.Initialise(configfile); // loading config file
  //m_variables.Print();

  m_data= &data; //assigning transient data pointer
  /////////////////////////////////////////////////////////////////
  PulsesOnly = false;
  MaxWaveforms = 5000;
  LEDsUsed = "unknown";
  LEDSetpoints = "unknown";
  use_led_waveforms = false;
  pulse_threshold = 5;

  int annieeventexists = m_data->Stores.count("ANNIEEvent");
  m_variables.Get("verbosity",verbosity);
  m_variables.Get("OutputFile",outputfile);
  m_variables.Get("SaveWaveforms",SaveWaves);
  m_variables.Get("PulseThreshold", pulse_threshold); 
  m_variables.Get("UseLEDWaveforms", use_led_waveforms); 
  m_variables.Get("WavesWithPulsesOnly",PulsesOnly);
  m_variables.Get("MaxWaveforms",MaxWaveforms);
  m_variables.Get("LEDsUsed",LEDsUsed);
  m_variables.Get("LEDSetpoints",LEDSetpoints);


  if(annieeventexists==0) {
    std::cout << "PrintADCData: No ANNIE Event in store to print!" << std::endl;
    return false;
  }

  CurrentRun = -1;
  CurrentSubrun = -1;

  //Initialize ROOT file that holds pulse examples
  std::string rootfile_out_prefix="_PrintADCDataWaves";
  std::string rootfile_out_root=".root";
  std::string rootfile_out_name=outputfile+rootfile_out_prefix+rootfile_out_root;

  if(SaveWaves){
   file_out=new TFile(rootfile_out_name.c_str(),"RECREATE"); //create one root file for each run to save the detailed plots and fits for all PMTs 
   file_out->cd();
  }

  hist_pulseocc_2D_y_phi = new TH2F("hist_pulseocc_2D_y_phi","Spatial distribution(mean pulses per acquisition, all PMTs)",100,0,360,25,-2.5,2.5);
  hist_frachit_2D_y_phi = new TH2F("hist_frachit_2D_y_phi","Spatial distribution (% of waveforms with a pulse, all PMTs)",100,0,360,25,-2.5,2.5);
  hist_hittime_channelnum = new TH2F("hist_hittime_channelnum","Pulse peak time distribution for channel numbers",6,-1,5,700,0,70000);
  hist_hitcharge_channelnum = new TH2F("hist_hitcharge_channelnum","Pulse charge distribution for channel numbers",5,-1,5,700,0,70000);
  hist_PEVNHit = new TH2F("hist_PEVNHit","Number of photoelectrons as a function of number of pulses",6000,0,6000,500,0,500);
  hist_ChargePoint = new TH2F("hist_ChargePoint","Number of photoelectrons as a function of charge point z-component",6000, 0, 6000,100,0,1);
  hist_ChargeBalance = new TH2F("hist_ChargeBalance","Number of photoelectrons as a function of charge balance cut",6000,0,6000,100,0,1);
  hist_TotPEVsMaxPE = new TH2F("hist_TotPEVsMaxPE","Number of photoelectrons as a function of the highest photoelectron count",6000,0,6000,2000,0,2000);

  //Initialize text file saving pulse occupancy info
  std::string pulsefile_out_prefix="_PulseInfo.txt";
  std::string pulseinfo_file = outputfile+pulsefile_out_prefix;
  result_file.open(pulseinfo_file);

  //Initialize needed information for occupancy plots; taken from Michael's code 
  auto get_geometry= m_data->Stores.at("ANNIEEvent")->Header->Get("AnnieGeometry",geom);
  if(!get_geometry){
  	Log("DigitBuilder Tool: Error retrieving Geometry from ANNIEEvent!",v_error,verbosity); 
  	return false; 
  }
  n_tank_pmts = geom->GetNumDetectorsInSet("Tank");
  Position detector_center=geom->GetTankCentre();
  tank_center_x = detector_center.X();
  tank_center_y = detector_center.Y();
  tank_center_z = detector_center.Z();

  //Initialize some ADC data histograms
  wave_lengths = new TH1F("wave_lengths","Number of samples in ADC waveforms",50000,0,50000);
  m_data->CStore.Get("ChannelNumToTankPMTCrateSpaceMap",ChannelNumToTankPMTCrateSpaceMap);
  m_data->CStore.Get("ChannelNumToTankPMTSPEChargeMap",ChannelKeyToSPEMap);

  WaveformNum = 0;
  std::cout << "PrintADCData: tool initialized" << std::endl;
  return true;
}


bool PrintADCData::Execute(){
  //Make sure we're in the right ROOT file
  file_out->cd();
  //Just print the whole dang thing
  if(verbosity>3) std::cout << "PrintADCData: getting total entries from header" << std::endl;
  m_data->Stores["ANNIEEvent"]->Header->Get("TotalEntries",totalentries);
  if(verbosity>3) std::cout << "PrintADCData: Number of ANNIEEvent entries: " << totalentries << std::endl;
  if(verbosity>3) std::cout << "PrintADCData: looping through entries" << std::endl;
  if(use_led_waveforms) m_data->Stores["ANNIEEvent"]->Get("RawLEDADCData",RawADCData);
  else m_data->Stores["ANNIEEvent"]->Get("RawADCData",RawADCData);
  bool have_auxdata = m_data->Stores["ANNIEEvent"]->Get("RawADCAuxData",RawADCAuxData);
  m_data->Stores["ANNIEEvent"]->Get("RunNumber",RunNum);
  m_data->Stores["ANNIEEvent"]->Get("SubrunNumber",SubrunNum);
  
  if (CurrentRun == -1){
    CurrentRun = RunNum;
    CurrentSubrun = SubrunNum;
  }
  else if (RunNum != CurrentRun || SubrunNum != CurrentSubrun){
  	Log("PrintADCData Tool: New run/subrun encountered.  Clearing occupancy info",v_message,verbosity);
    this->SaveOccupancyInfo(CurrentRun, CurrentSubrun);
    this->ClearOccupancyInfo();
  }
  
  if(verbosity>2) std::cout << "Num. of PMT signals for entry: " << RawADCData.size() << std::endl;
  auto get_pulses = m_data->Stores["ANNIEEvent"]->Get("RecoADCHits",RecoADCHits);
  if(!get_pulses){
  	Log("PrintADCData Tool: No reconstructed pulses! Did you run the ADCHitFinder first?",v_error,verbosity); 
  	return false;
  };

  //Print out the raw ADC waveforms to the opened ROOT file
  if ( RawADCData.empty() ) {
    Log("PrintADCData Error: Found an empty RawADCData entry in event", 0,
      verbosity);
  }
  else {
    this->PrintInfoInData(RawADCData,false);
    
    if(verbosity>4) std::cout << "PrintADCData tool: Now calculating charge point value" << std::endl;
    this->CalculateChargePoint(RawADCData);
    this->CalculateChargeBalance(RawADCData);
    this->CalculateMaxPE(RawADCData);
  }
  if(verbosity>4) std::cout << "PrintADCData tool: Moving to aux channels" << std::endl;
  if (have_auxdata){
    if ( RawADCAuxData.empty() ) {
      Log("PrintADCData Error: Found an empty RawADCData entry in event", 0,
        verbosity);
    }
    else {
      this->PrintInfoInData(RawADCAuxData,true);
    }
  }
  return true;
}


bool PrintADCData::Finalise(){
  file_out->cd();
  
  wave_lengths->GetXaxis()->SetTitle("Number of samples in waveform (ADC)");
  wave_lengths->Write();
  this->Make2DHists();
  if(SaveWaves) file_out->Close();
  this->SaveOccupancyInfo(CurrentRun, CurrentSubrun);
  result_file.close();
  std::cout << "PrintADCData exitting" << std::endl;
  return true;
}


void PrintADCData::PrintInfoInData(std::map<unsigned long, std::vector<Waveform<uint16_t>> > RawADCData,
        bool isAuxData)
{
  std::vector<int> CrateSpaceInfo;
  double total_PE = 0;
  double num_hits = 0;

  for (const auto& temp_pair : RawADCData) {
    const auto& channel_key = temp_pair.first;
    
    //If working with PMT ADC data, fill out pulse occupancy information
    if(!isAuxData){
      std::vector<std::vector<ADCPulse>> buffer_pulses = RecoADCHits.at(channel_key);
      CrateSpaceInfo = ChannelNumToTankPMTCrateSpaceMap.at(channel_key);
      int num_pulses = 0;
      for (int i = 0; i < buffer_pulses.size(); i++){
          std::vector<ADCPulse> onebuffer_pulses = buffer_pulses.at(i);
          for (int j = 0; j < onebuffer_pulses.size(); j++){
            ADCPulse apulse = onebuffer_pulses.at(j);
            double pulse_charge = apulse.charge();
            double pulse_PE = 0;
            std::map<int, double>::iterator it = ChannelKeyToSPEMap.find(channel_key);
            if(it != ChannelKeyToSPEMap.end()){ //Charge to SPE conversion is available
              pulse_PE  = pulse_charge / ChannelKeyToSPEMap.at(channel_key);
              total_PE+=pulse_PE;
              num_hits+=1;
            } else {
              if(verbosity>2){
                std::cout << "FOUND A HIT FOR CHANNELKEY " << channel_key << "BUT NO CONVERSION " <<
                    "TO PE AVAILABLE.  SKIPPING PE." << std::endl;
              }
            }
            hist_hittime_channelnum->Fill(CrateSpaceInfo.at(2),apulse.peak_time());
            hist_hitcharge_channelnum->Fill(CrateSpaceInfo.at(2),pulse_charge);
            int pulse_height = apulse.raw_amplitude() - apulse.baseline();
            if (pulse_height >= pulse_threshold) num_pulses +=1;
          }
      }
      //Fill in pulse log information
      std::map<unsigned long, int>::iterator it = NumWaves.find(channel_key);
      if(it != NumWaves.end()){ //First time encountering channel key
        NumWaves.at(channel_key)+=1;
        NumPulses.at(channel_key)+=num_pulses;
        if(num_pulses>0) NumWavesWithAPulse.at(channel_key)+=1;
      }
      else {
        NumWaves.emplace(channel_key,1);
        NumPulses.emplace(channel_key,num_pulses);
        if(num_pulses>0) NumWavesWithAPulse.emplace(channel_key,1);
        else NumWavesWithAPulse.emplace(channel_key,0);
      }

      if(PulsesOnly){
        if (num_pulses==0) continue;
      }
    }

    if(verbosity>3) std::cout << "Waveform info for channel " << channel_key << std::endl;
    //Default running: raw_waveforms only has one entry.  If we go to a
    //hefty-mode style of running though, this could have multiple minibuffers
    //const std::vector< Waveform<unsigned short> > raw_waveforms;
    //raw_waveforms = temp_pair.second;
    const auto& raw_waveforms = temp_pair.second;
    //Length of waveform vector should be 1 in normal mode, but greater than
    //1 in a hefty-like format (multiple waveform buffers)
    if(verbosity>3)std::cout << "Waveform size: " << raw_waveforms.size() << std::endl;
    for (int j=0; j < raw_waveforms.size(); j++){
      if(verbosity>4)std::cout << "Printing waveform info for buffer at index " << j << std::endl;
      Waveform<unsigned short> awaveform = raw_waveforms.at(j);
      if(verbosity>4)std::cout << "Waveform start time: " << std::setprecision(16) << awaveform.GetStartTime() << std::endl;
      std::vector<unsigned short>* thewave=awaveform.GetSamples();
      int wavelength = thewave->size();
      file_out->cd();
      wave_lengths->Fill(wavelength);
      if(verbosity>4){
        std::cout << "BEGIN SAMPLES" << std::endl;
        for (int i=0; i < wavelength; i++){
          std::cout << thewave->at(i) << std::endl;
        }
      }
    }
    // Method directly taken from Marcus' PrintADCData 
    // make a numberline to act as the x-axis of the plotting TGraph
    // Each Waveform represents one minibuffer on this channel
    if(SaveWaves && (WaveformNum < MaxWaveforms)){
      Log("PrintADCData Tool: Looping over "+to_string(raw_waveforms.size())
           +" minibuffers",v_debug,verbosity);
      for(Waveform<uint16_t> wfrm : raw_waveforms){
        std::vector<uint16_t>* samples = wfrm.GetSamples();
        double StartTime = wfrm.GetStartTime();
        int SampleLength = samples->size();
        numberline.resize(SampleLength);
        std::iota(numberline.begin(),numberline.end(),0);
        upcastdata.resize(SampleLength);
        
        // for plotting on a TGraph we need to up-cast the data from uint16_t to int32_t
        Log("PrintADCData Tool: Making TGraph",v_debug,verbosity);
        for(int samplei=0; samplei<SampleLength; samplei++){
            upcastdata.at(samplei) = samples->at(samplei);
        }
        //I hate my life
        std::string ckey = to_string(channel_key);
        if(mb_graph){ delete mb_graph; }
        mb_graph = new TGraph(SampleLength, numberline.data(), upcastdata.data());
        std::string graph_name = "mb_graph_"+to_string(channel_key)+"_"+to_string(StartTime);
        //Place graph into correct channel_key tree
        mb_graph->SetName(graph_name.c_str());
        std::string title = graph_name+";Time since acquisition start (ADC);Sample value (ADC)";
        mb_graph->SetTitle(title.c_str());
        std::map<std::string, TDirectory * >::iterator it = ChanKeyToDirectory.find(ckey);
        if(it == ChanKeyToDirectory.end()){ //No Tree made yet for this channel key.  make it
          ChanKeyToDirectory.emplace(ckey, file_out->mkdir(ckey.c_str()));
        }
        ChanKeyToDirectory.at(ckey)->cd();
        mb_graph->Write();
        WaveformNum+=1;
      } // end loop over minibuffers
    }
  }
  if(verbosity>4) std::cout << "Filling the PEVNHit histogram now" << std::endl;
  hist_PEVNHit->Fill(total_PE,num_hits);
}

void PrintADCData::SaveOccupancyInfo(uint32_t Run, uint32_t Subrun)
{
  //TODO: eventually, also load individual channel thresholds
  result_file << "run_number," << Run << std::endl;
  result_file << "subrun_number," << Subrun << std::endl;
  result_file << "LEDs_used," << LEDsUsed << std::endl;
  result_file << "LED_setpoints," << LEDSetpoints << std::endl;
  result_file << "pulse_threshold," << pulse_threshold << std::endl;
  result_file << "channel_key,numwaves,numpulses,numwaveswithapulse" << std::endl;
  m_variables.Get("LEDsUsed",LEDsUsed);
  m_variables.Get("LEDSetpoints",LEDSetpoints);
  for (const auto& temp_pair : NumWaves) {
    const auto& channel_key = temp_pair.first;
    if(verbosity>2){
      std::cout << "Printing information for channel_key "<<channel_key << std::endl;
      std::cout << "channel_key,"<<channel_key<<std::endl;
      std::cout << "numwaves,"<<NumWaves.at(channel_key)<<std::endl;
      std::cout << "numpulses,"<<NumPulses.at(channel_key)<<std::endl;
      std::cout << "numwaveswithpulse,"<<NumWavesWithAPulse.at(channel_key)<<std::endl;
    }
    result_file << channel_key << "," << NumWaves.at(channel_key) << 
        "," << NumPulses.at(channel_key) << "," << NumWavesWithAPulse.at(channel_key) << 
        std::endl;
  }
  return;
}

void PrintADCData::ClearOccupancyInfo()
{
  NumWaves.clear();
  NumPulses.clear();
  NumWavesWithAPulse.clear();
  return;
}

void PrintADCData::Make2DHists(){
  std::map<std::string, std::map<unsigned long,Detector*>>* Detectors = geom->GetDetectors();
  for (std::map<unsigned long,Detector*>::iterator it  = Detectors->at("Tank").begin(); it != Detectors->at("Tank").end(); ++it){

    Detector* apmt = it->second;
    unsigned long detkey = it->first;
    unsigned long chankey = apmt->GetChannels()->begin()->first;
    pmt_detkeys.push_back(detkey);
    detkey_to_chankey.emplace(detkey,chankey);
    std::string dettype = apmt->GetDetectorType();

    if (verbosity > 1) std::cout <<"PMT with detkey: "<<detkey<<", has type "<<dettype<<std::endl;
    if (verbosity > 1) std::cout <<"PMT Print:" <<apmt->Print()<<std::endl;

    Position position_PMT = apmt->GetDetectorPosition();
    if (verbosity > 2) std::cout <<"detkey: "<<detkey<<", chankey: "<<chankey<<std::endl;
    if (verbosity > 2) std::cout <<"filling PMT position maps"<<std::endl;

    x_PMT.insert(std::pair<unsigned long,double>(detkey,position_PMT.X()-tank_center_x));
    y_PMT.insert(std::pair<unsigned long,double>(detkey,position_PMT.Y()-tank_center_y));
    z_PMT.insert(std::pair<unsigned long,double>(detkey,position_PMT.Z()-tank_center_z));

    double rho = sqrt(pow(x_PMT.at(detkey),2)+pow(y_PMT.at(detkey),2));
    if (x_PMT.at(detkey) < 0) rho*=-1;
    rho_PMT.insert(std::pair<unsigned long, double>(detkey,rho));
    double phi;
    if (x_PMT.at(detkey)>0 && z_PMT.at(detkey)>0) phi = atan(x_PMT.at(detkey)/z_PMT.at(detkey));
    if (x_PMT.at(detkey)>0 && z_PMT.at(detkey)<0) phi = TMath::Pi()/2+atan(-z_PMT.at(detkey)/x_PMT.at(detkey));
    if (x_PMT.at(detkey)<0 && z_PMT.at(detkey)<0) phi = TMath::Pi()+atan(x_PMT.at(detkey)/z_PMT.at(detkey));
    if (x_PMT.at(detkey)<0 && z_PMT.at(detkey)>0) phi = 3*TMath::Pi()/2+atan(z_PMT.at(detkey)/-x_PMT.at(detkey));
    phi_PMT.insert(std::pair<unsigned long,double>(detkey,phi));

    if (verbosity > 2) std::cout <<"detectorkey: "<<detkey<<", position: ("<<position_PMT.X()<<","<<position_PMT.Y()<<","<<position_PMT.Z()<<")"<<std::endl;
    if (verbosity > 2) std::cout <<"rho PMT "<<detkey<<": "<<rho<<std::endl;
    if (verbosity > 2) std::cout <<"y PMT: "<<y_PMT.at(detkey)<<std::endl;

  }
  for (int i_tube=0;i_tube<n_tank_pmts;i_tube++){
    unsigned long detkey = pmt_detkeys[i_tube];
    if(x_PMT.at(detkey)<-2000) continue;
    unsigned long chankey = detkey_to_chankey.at(detkey);
    std::map<unsigned long, int>::iterator it = NumWaves.find(chankey);
    if(it != NumWaves.end()){ //There were some pulses for this channel
      double meanpulses=((double)NumPulses.at(chankey))/((double)NumWaves.at(chankey));
      double frachit=((double)NumWavesWithAPulse.at(chankey))/((double)NumWaves.at(chankey));
      hist_pulseocc_2D_y_phi->SetBinContent(int(phi_PMT[detkey]/2/TMath::Pi()*100)+1,int((y_PMT[detkey]+2.5)/5.*25)+1,meanpulses);
      hist_frachit_2D_y_phi->SetBinContent(int(phi_PMT[detkey]/2/TMath::Pi()*100)+1,int((y_PMT[detkey]+2.5)/5.*25)+1,frachit);
    }
  }
  hist_pulseocc_2D_y_phi->GetXaxis()->SetTitle("#phi [deg]");
  hist_pulseocc_2D_y_phi->GetYaxis()->SetTitle("y [m]");
  hist_pulseocc_2D_y_phi->SetStats(0);
  hist_frachit_2D_y_phi->GetXaxis()->SetTitle("#phi [deg]");
  hist_frachit_2D_y_phi->GetYaxis()->SetTitle("y [m]");
  hist_frachit_2D_y_phi->SetStats(0);
  hist_hittime_channelnum->GetXaxis()->SetTitle("Channel number");
  hist_hittime_channelnum->GetYaxis()->SetTitle("Pulse peak time [ns]");
  hist_hittime_channelnum->SetStats(0);
  hist_hitcharge_channelnum->GetXaxis()->SetTitle("Channel number");
  hist_hitcharge_channelnum->GetYaxis()->SetTitle("Pulse charge [nC]");
  hist_hitcharge_channelnum->SetStats(0);
  hist_PEVNHit->GetXaxis()->SetTitle("Total PE");
  hist_PEVNHit->GetYaxis()->SetTitle("Number of Pulses");
  hist_PEVNHit->SetStats(0);
  hist_ChargeBalance->GetXaxis()->SetTitle("Total PE");
  hist_ChargeBalance->GetYaxis()->SetTitle("Charge balance parameter");
  hist_ChargeBalance->SetStats(0);
  hist_ChargePoint->GetXaxis()->SetTitle("Total PE");
  hist_ChargePoint->GetYaxis()->SetTitle("Charge point z-component");
  hist_ChargePoint->SetStats(0);
  hist_TotPEVsMaxPE->GetXaxis()->SetTitle("Total PE");
  hist_TotPEVsMaxPE->GetYaxis()->SetTitle("Highest PE pulse");
  hist_TotPEVsMaxPE->SetStats(0);
  std::string hd = "2D_Histograms";
  TDirectory* histdir = file_out->mkdir(hd.c_str());
  histdir->cd();
  hist_pulseocc_2D_y_phi->Write();
  hist_frachit_2D_y_phi->Write();
  hist_hittime_channelnum->Write();
  hist_hitcharge_channelnum->Write();
  hist_PEVNHit->Write();
  hist_ChargePoint->Write();
  hist_ChargeBalance->Write();
  hist_TotPEVsMaxPE->Write();

  return;
}

void PrintADCData::CalculateChargePoint(std::map<unsigned long, std::vector<Waveform<uint16_t>> > RawADCData)
{

  double x_weight = 0;
  double y_weight = 0;
  double z_weight = 0;
  std::vector<int> CrateSpaceInfo;
  double total_PE = 0;
  for (const auto& temp_pair : RawADCData) {
    const auto& channel_key = temp_pair.first;
    
    //Get the total photoelectron count and total pulses for channel
    std::vector<std::vector<ADCPulse>> buffer_pulses = RecoADCHits.at(channel_key);
    CrateSpaceInfo = ChannelNumToTankPMTCrateSpaceMap.at(channel_key);
    double total_channelPE = 0;
    int num_hits = 0;
    for (int i = 0; i < buffer_pulses.size(); i++){
      std::vector<ADCPulse> onebuffer_pulses = buffer_pulses.at(i);
      for (int j = 0; j < onebuffer_pulses.size(); j++){
        ADCPulse apulse = onebuffer_pulses.at(j);
        double pulse_charge = apulse.charge();
        double pulse_PE = 0;
        if(verbosity>4) std::cout << "Finding charge to SPE conversion for channel " << channel_key << std::endl;
        std::map<int, double>::iterator it = ChannelKeyToSPEMap.find(channel_key);
        if(it != ChannelKeyToSPEMap.end()){ //Charge to SPE conversion is available
          pulse_PE  = pulse_charge / ChannelKeyToSPEMap.at(channel_key);
          if(verbosity>4) std::cout << "Channel pulse PE is " << pulse_PE << std::endl;
          total_channelPE+=pulse_PE;
          num_hits+=1;
        } else {
          if(verbosity>2){
            std::cout << "FOUND A HIT FOR CHANNELKEY " << channel_key << "BUT NO CONVERSION " <<
                "TO PE AVAILABLE.  SKIPPING PE." << std::endl;
          }
        }
      }
    }
    total_PE+=total_channelPE;
    if(verbosity>4) std::cout << "Calculated channel PE for charge point is: " << total_channelPE << std::endl;
    Detector* this_detector = geom->ChannelToDetector(channel_key);
    Position det_position = this_detector->GetDetectorPosition();
    double pos_mag = sqrt(pow(det_position.X(),2) + pow(det_position.Y(),2) + pow(det_position.Z(),2));
    x_weight+= det_position.X()*total_PE/pos_mag;
    y_weight+= det_position.Y()*total_PE/pos_mag;
    z_weight+= det_position.Z()*total_PE/pos_mag;
  }
  if(verbosity>4) std::cout << "Generate charge weight direction and add info to ChargePoint histogram" << std::endl;
  Direction charge_weight(x_weight,y_weight,z_weight);
  hist_ChargePoint->Fill(total_PE,charge_weight.Z());
  return;
}

void PrintADCData::CalculateChargeBalance(std::map<unsigned long, std::vector<Waveform<uint16_t>> > RawADCData)
{
  double total_Q = 0;
  double total_QSquared = 0;
  double total_PE = 0;
  for (const auto& temp_pair : RawADCData) {
    const auto& channel_key = temp_pair.first;
    
    //Get the total photoelectron count and total pulses for channel
    std::vector<std::vector<ADCPulse>> buffer_pulses = RecoADCHits.at(channel_key);
    double total_channelPE = 0;
    double total_channelQ = 0;
    for (int i = 0; i < buffer_pulses.size(); i++){
      std::vector<ADCPulse> onebuffer_pulses = buffer_pulses.at(i);
      for (int j = 0; j < onebuffer_pulses.size(); j++){
        ADCPulse apulse = onebuffer_pulses.at(j);
        double pulse_charge = apulse.charge();
        double pulse_PE = 0;
        if(verbosity>4) std::cout << "Finding charge to SPE conversion for channel " << channel_key << std::endl;
        std::map<int, double>::iterator it = ChannelKeyToSPEMap.find(channel_key);
        if(it != ChannelKeyToSPEMap.end()){ //Charge to SPE conversion is available
          pulse_PE  = pulse_charge / ChannelKeyToSPEMap.at(channel_key);
          total_channelQ+= pulse_charge;
          if(verbosity>4) std::cout << "Channel pulse PE is " << pulse_PE << std::endl;
          total_channelPE+=pulse_PE;
        } else {
          if(verbosity>2){
            std::cout << "FOUND A HIT FOR CHANNELKEY " << channel_key << "BUT NO CONVERSION " <<
                "TO PE AVAILABLE.  SKIPPING PE." << std::endl;
          }
        }
      }
    }
    total_PE+=total_channelPE;
    total_Q+=total_channelQ;
    total_QSquared+=(total_channelQ*total_channelQ);
  }
  if(verbosity>4) std::cout << "Calculate charge balance cut and fill into histogram" << std::endl;
  double charge_balance  = sqrt((total_QSquared)/(total_Q*total_Q) - (1./126.));
  hist_ChargeBalance->Fill(total_PE,charge_balance);
  return;
}

void PrintADCData::CalculateMaxPE(std::map<unsigned long, std::vector<Waveform<uint16_t>> > RawADCData)
{
  double total_PE = 0;
  double max_PE = 0;
  for (const auto& temp_pair : RawADCData) {
    const auto& channel_key = temp_pair.first;
    
    //Get the total photoelectron count and total pulses for channel
    std::vector<std::vector<ADCPulse>> buffer_pulses = RecoADCHits.at(channel_key);
    double total_channelPE = 0;
    for (int i = 0; i < buffer_pulses.size(); i++){
      std::vector<ADCPulse> onebuffer_pulses = buffer_pulses.at(i);
      for (int j = 0; j < onebuffer_pulses.size(); j++){
        ADCPulse apulse = onebuffer_pulses.at(j);
        double pulse_charge = apulse.charge();
        double pulse_PE = 0;
        if(verbosity>4) std::cout << "Finding charge to SPE conversion for channel " << channel_key << std::endl;
        std::map<int, double>::iterator it = ChannelKeyToSPEMap.find(channel_key);
        if(it != ChannelKeyToSPEMap.end()){ //Charge to SPE conversion is available
          pulse_PE  = pulse_charge / ChannelKeyToSPEMap.at(channel_key);
          if(pulse_PE > max_PE) max_PE = pulse_PE;
          if(verbosity>4) std::cout << "Channel pulse PE is " << pulse_PE << std::endl;
          total_channelPE+=pulse_PE;
        } else {
          if(verbosity>2){
            std::cout << "FOUND A HIT FOR CHANNELKEY " << channel_key << "BUT NO CONVERSION " <<
                "TO PE AVAILABLE.  SKIPPING PE." << std::endl;
          }
        }
      }
    }
    total_PE+=total_channelPE;
  }
  if(verbosity>4) std::cout << "Fill max PE vs total PE histogram" << std::endl;
  hist_TotPEVsMaxPE->Fill(total_PE,max_PE);
  return;
}
