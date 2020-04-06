#include "MRDLoopbackAnalysis.h"

MRDLoopbackAnalysis::MRDLoopbackAnalysis():Tool(){}


bool MRDLoopbackAnalysis::Initialise(std::string configfile, DataModel &data){

  /////////////////// Useful header ///////////////////////
  if(configfile!="") m_variables.Initialise(configfile); // loading config file
  //m_variables.Print();

  m_data= &data; //assigning transient data pointer
  /////////////////////////////////////////////////////////////////

  verbosity = 0;
  PreviousTimeStamp = 0;

  m_variables.Get("verbosity",verbosity);
  m_variables.Get("OutputROOTFile",output_rootfile);

  Log("MRDLoopbackAnalysis Tool: Initialising.",v_message,verbosity);

  std::stringstream ss_rootfilename;
  ss_rootfilename << output_rootfile << ".root";
  Log("MRDLoopbackAnalysis tool: Create ROOT-file "+ss_rootfilename.str(),v_message,verbosity);

  mrddigitts_file = new TFile(ss_rootfilename.str().c_str(),"RECREATE");
  time_diff_beam = new TH1D("time_diff_beam","Time difference between beam events",500,0,1000);
  time_diff_cosmic = new TH1D("time_diff_cosmic","Time difference between cosmic events",5000,0,10000);
  mrddigitts_beamloopback = new TH2D("mrddigitts_beamloopback","MRD Times vs Beam Loopback TDC",1000,0,1000,1000,0,4000);
  mrddigitts_cosmicloopback = new TH2D("mrddigitts_cosmicloopback","MRD Times vs Cosmic Loopback TDC",1000,0,1000,1000,0,4000);
  time_diff_loopback_beam = new TH2D("time_diff_loopback_beam","Time difference between beam events vs loopback TDC",1000,0,1000,500,0,1000);
  time_diff_loopback_cosmic = new TH2D("time_diff_loopback_cosmic","Time difference between cosmic events vs loopback TDC",1000,0,1000,500,0,10000);
  time_diff_hittimes_beam = new TH2D("time_diff_hittimes_beam","Beam hit times vs. time difference last event",500,0,1000,1000,0,4000);
  time_diff_hittimes_cosmic = new TH2D("time_diff_hittimes_cosmic","Cosmic hit times vs. time difference last event",500,0,1000,1000,0,4000);


  return true;
}


bool MRDLoopbackAnalysis::Execute(){

  Log("MRDLoopbackAnalysis Tool: Executing.",v_message,verbosity);
  
  int beam_tdc = 0;
  int cosmic_tdc = 0;
  std::string MRDTriggertype;
  int evnum;
  TimeClass timeclass_timestamp;
  std::map<std::string,int> mrd_loopback_tdc;
  bool get_ok = true;

  get_ok = m_data->CStore.Get("MrdTimeClusters",MrdTimeClusters);
  if (not get_ok) { Log("MRDLoopbackAnalysis tool: Error retrieving MrdTimeClusters map from CStore, did you run TimeClustering beforehand?",v_error,verbosity); return true; }
  if (MrdTimeClusters.size()!=0){
    get_ok = m_data->CStore.Get("MrdDigitTimes",MrdDigitTimes);
    if (not get_ok) { Log("MRDLoopbackAnalysis tool: Error retrieving MrdDigitTimes map from CStore, did you run TimeClustering beforehand?",v_error,verbosity); return true; }
  }
  get_ok = m_data->Stores.at("ANNIEEvent")->Get("MRDTriggerType",MRDTriggertype);
  if (get_ok) Log("MRDLoopbackAnalysis tool: MRDTriggertype is "+MRDTriggertype,v_message,verbosity);
  else { Log("MRDLoopbackAnaysis tool: Did not find MRDTriggertype in ANNIEEvent BoostStore! Cancel Execute step",v_error,verbosity); return true;}
  get_ok = m_data->Stores.at("ANNIEEvent")->Get("EventNumber",evnum);
  if (!get_ok) Log("MRDLoopbackAnaysis tool: Did not find EventNumber in ANNIEEvent BoostStore!",v_message,verbosity);
  get_ok = m_data->Stores.at("ANNIEEvent")->Get("EventTime",timeclass_timestamp);
  if (!get_ok) { Log("MRDLoopbackAnaysis tool: Did not find EventTime in ANNIEEvent BoostStore! Cancel Execute step",v_error,verbosity); return true;}
  get_ok = m_data->Stores.at("ANNIEEvent")->Get("MRDLoopbackTDC",mrd_loopback_tdc);
  if (!get_ok) { Log("MRDLoopbackAnaysis tool: Did not find MRDLoopbackTDC in ANNIEEvent BoostStore! Cancel Execute step",v_error,verbosity); return true;}


  //Loop over MRDLoopbackTDC map
  for (std::map<std::string,int>::iterator it = mrd_loopback_tdc.begin(); it != mrd_loopback_tdc.end(); it++){
    std::string loopback_name = it->first;
    int loopback_tdc = it->second;
    Log("MRDLoopbackTDC tool: "+loopback_name+", loopback TDC = "+std::to_string(loopback_tdc),v_message,verbosity);
    if (loopback_name == "BeamLoopbackTDC") beam_tdc = loopback_tdc;
    else if (loopback_name == "CosmicLoopbackTDC") cosmic_tdc = loopback_tdc;
  }

  uint64_t eventtime = timeclass_timestamp.GetNs();
  unsigned long utime_diff = 0;
  if (PreviousTimeStamp !=0){
    utime_diff = eventtime/1E3 - PreviousTimeStamp; //in milliseconds
    if (MRDTriggertype == "Cosmic"){
      time_diff_cosmic->Fill(utime_diff);
      time_diff_loopback_cosmic->Fill(cosmic_tdc,utime_diff);
    }
    else if (MRDTriggertype == "Beam") {
      time_diff_beam->Fill(utime_diff);
      time_diff_loopback_beam->Fill(beam_tdc,utime_diff);
    }
  }
  PreviousTimeStamp = eventtime/1E3;

  for (unsigned int i_cluster = 0; i_cluster < MrdTimeClusters.size(); i_cluster++){
    std::vector<int> single_mrdcluster = MrdTimeClusters.at(i_cluster);
    int numdigits = single_mrdcluster.size();
    for (int thisdigit = 0; thisdigit < numdigits; thisdigit++){
      int digit_value = single_mrdcluster.at(thisdigit);
      int time = MrdDigitTimes.at(digit_value);
      if (MRDTriggertype == "Cosmic") {
        mrddigitts_cosmicloopback->Fill(cosmic_tdc,time);
        time_diff_hittimes_cosmic->Fill(utime_diff,time);
      }
      else if (MRDTriggertype == "Beam"){
        mrddigitts_beamloopback->Fill(beam_tdc,time);
        time_diff_hittimes_beam->Fill(utime_diff,time);
      }
    }
  }

  return true;
}


bool MRDLoopbackAnalysis::Finalise(){

  Log("MRDLoopbackAnalysis Tool: Finalising.",v_message,verbosity);
  
  mrddigitts_file->cd();
  mrddigitts_file->Write();
  mrddigitts_file->Close();
  delete mrddigitts_file;
  mrddigitts_file = 0;

  return true;
}
