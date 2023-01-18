#include "StoreDecodedTimestamps.h"

StoreDecodedTimestamps::StoreDecodedTimestamps():Tool(){}


bool StoreDecodedTimestamps::Initialise(std::string configfile, DataModel &data){

  /////////////////// Useful header ///////////////////////
  if(configfile!="") m_variables.Initialise(configfile); // loading config file
  //m_variables.Print();

  m_data= &data; //assigning transient data pointer
  /////////////////////////////////////////////////////////////////

  delete_timestamps = 1;		//This should only be set to false if used in combination with ANNIEEventBuilder

  //Get configuration variables
  m_variables.Get("verbosity",verbosity);
  m_variables.Get("OutputFile",output_timestamps);
  m_variables.Get("SaveMRD",save_mrd);
  m_variables.Get("SavePMT",save_pmt);
  m_variables.Get("SaveCTC",save_ctc);
  m_variables.Get("DeleteTimestamps",delete_timestamps);	//Use this delete option if not used in combination with ANNIEEventBuilder

  f_timestamps = new TFile(output_timestamps.c_str(),"RECREATE");
  t_timestamps_mrd = new TTree("tree_timestamps_mrd","Timestamps Tree MRD");
  t_timestamps_pmt = new TTree("tree_timestamps_pmt","Timestamps Tree PMT");
  t_timestamps_ctc = new TTree("tree_timestamps_ctc","Timestamps Tree CTC");
  t_timestamps_pmt->Branch("t_pmt",&t_pmt);
  t_timestamps_mrd->Branch("t_mrd",&t_mrd);
  t_timestamps_ctc->Branch("t_ctc",&t_ctc);
  t_timestamps_pmt->Branch("t_pmt_sec",&t_pmt_sec);
  t_timestamps_mrd->Branch("t_mrd_sec",&t_mrd_sec);
  t_timestamps_ctc->Branch("t_ctc_sec",&t_ctc_sec);
  t_timestamps_ctc->Branch("triggerword_ctc",&triggerword_ctc);

  InProgressTankEvents = new std::map<uint64_t, std::map<std::vector<int>, std::vector<uint16_t> > >;
  ExecuteNr=0;

  new_pmt_data = false;
  new_mrd_data = false;
  new_ctc_data = false;

  return true;
}


bool StoreDecodedTimestamps::Execute(){

  bool mrddata=false;
  bool pmtdata=false;
  bool ctcdata=false;
  if (save_mrd) m_data->CStore.Get("NewMRDDataAvailable",mrddata);
  if (save_pmt) m_data->CStore.Get("NewTankPMTDataAvailable",pmtdata);
  if (save_ctc) m_data->CStore.Get("NewCTCDataAvailable",ctcdata);
  
  if (mrddata && save_mrd) new_mrd_data = true;
  if (pmtdata && save_pmt) new_pmt_data = true;
  if (ctcdata && save_ctc) new_ctc_data = true;

  // ensure we always build if the toolchain is stopping
  bool toolchain_stopping=false;
  m_data->vars.Get("StopLoop",toolchain_stopping);
  // likewise ensure we try to do all possible matching if we've hit the end of the file
  bool file_completed=false;
  m_data->CStore.Set("FileCompleted",file_completed);
  toolchain_stopping |= file_completed;

  ExecuteNr++;
  if (ExecuteNr%10 != 0 && (!toolchain_stopping)) return true;

  //Get decoded times from subsystems
  if (new_mrd_data) {

    m_data->CStore.Get("MRDEvents",MRDEvents);
 
    for(std::pair<uint64_t,std::vector<std::pair<unsigned long,int>>> apair : MRDEvents){
      uint64_t MRDTimeStamp = apair.first;
      t_mrd = (ULong64_t) MRDTimeStamp;
      t_mrd_sec = double(t_mrd)/(1.E9);
      t_timestamps_mrd->Fill();     

    }
    if (delete_timestamps){
      MRDEvents.clear();
      m_data->CStore.Set("NewMRDDataAvailable",false);
      m_data->CStore.Set("MRDEvents",MRDEvents);    
    }

  }

   if (new_pmt_data) {
    m_data->CStore.Get("InProgressTankEvents",InProgressTankEvents);
    std::vector<uint64_t> timestamps_delete;
    for(std::pair<uint64_t,std::map<std::vector<int>, std::vector<uint16_t>>> apair : *InProgressTankEvents){
      uint64_t PMTCounterTimeNs = apair.first;
      std::map<std::vector<int>, std::vector<uint16_t>> aWaveMap = apair.second;
      if (aWaveMap.size() == (133)){
        if (AlmostCompleteWaveforms.find(PMTCounterTimeNs)!=AlmostCompleteWaveforms.end()) AlmostCompleteWaveforms[PMTCounterTimeNs]++;
        else AlmostCompleteWaveforms.emplace(PMTCounterTimeNs,0);
        std::cout <<"AlmostCompleteWaveforms for PMTCounterTimeNs: "<<PMTCounterTimeNs<<": "<<AlmostCompleteWaveforms.at(PMTCounterTimeNs)<<std::endl;
      }

      if (aWaveMap.size() >= 134 || ((aWaveMap.size() == 133) && (AlmostCompleteWaveforms.at(PMTCounterTimeNs)>=5))|| (aWaveMap.size() == 133 && toolchain_stopping)){
        t_pmt = (ULong64_t) PMTCounterTimeNs;
        t_pmt_sec = double(t_pmt)/1.E9;
        t_timestamps_pmt->Fill();
        timestamps_delete.push_back(PMTCounterTimeNs);
      }
    }
    if (delete_timestamps){
      for (int i_delete=0; i_delete < (int) timestamps_delete.size(); i_delete++){
        InProgressTankEvents->erase(timestamps_delete.at(i_delete));
      }
    }
  }

  if (new_ctc_data) {
    m_data->CStore.Get("TimeToTriggerWordMap",TimeToTriggerWordMap);
    for(std::pair<uint64_t,std::vector<uint32_t>> apair : *TimeToTriggerWordMap){
      uint64_t CTCTimeStamp = apair.first;
      std::vector<uint32_t> CTCWord = apair.second;
      t_ctc = (ULong64_t) CTCTimeStamp;
      t_ctc_sec = (double(t_ctc))/1.E9;
      for (int i_vec=0; i_vec < (int) CTCWord.size(); i_vec++){
        triggerword_ctc = (int) CTCWord.at(i_vec);
        t_timestamps_ctc->Fill();
      }
    }
    if (delete_timestamps){
      TimeToTriggerWordMap->clear();
    }
  }

  new_ctc_data = false;
  new_pmt_data = false;
  new_mrd_data = false;

  return true;
}


bool StoreDecodedTimestamps::Finalise(){

  f_timestamps->cd();
  t_timestamps_mrd->Write();
  t_timestamps_pmt->Write();
  t_timestamps_ctc->Write();
  f_timestamps->Close();
  delete f_timestamps;

  return true;
}
