#include "saveLAPPDInfo.h"

saveLAPPDInfo::saveLAPPDInfo():Tool(){}


bool saveLAPPDInfo::Initialise(std::string configfile, DataModel &data){

  /////////////////// Useful header ///////////////////////
  if(configfile!="") m_variables.Initialise(configfile); // loading config file
  //m_variables.Print();

  m_data= &data; //assigning transient data pointer
  /////////////////////////////////////////////////////////////////

  m_variables.Get("OutputFile",output_path);
  output_csv.open(output_path.c_str());

  output_csv << "Run, SubRun, Part, Ev, GlobalEv, TriggerTime, LAPPDEventTime, LAPPDOffset, TimeSinceBeamgate, InBeamWindow, MRDTrack, NoVeto" << std::endl;

  return true;
}


bool saveLAPPDInfo::Execute(){

  bool has_lappd = false;
  m_data->CStore.Get("LAPPD_HasData",has_lappd);

  if (has_lappd){
    int run_number;
    int subrun_number;
    int part_number;
    int ev_number;
    int global_ev_number;
    uint64_t trigger_time;
    uint64_t event_time_lappd;
    uint64_t lappd_offset;
    uint64_t time_since_beamgate;
    bool in_beam_window;
    bool mrd_track;
    bool no_veto;

    m_data->Stores["ANNIEEvent"]->Get("RunNumber",run_number);
    m_data->Stores["ANNIEEvent"]->Get("SubrunNumber",subrun_number);
    m_data->Stores["ANNIEEvent"]->Get("PartNumber",part_number);
    m_data->Stores["ANNIEEvent"]->Get("CTCTimestamp",trigger_time);

    m_data->Stores.at("RecoEvent")->Get("PMTMRDCoinc",mrd_track);
    m_data->Stores.at("RecoEvent")->Get("NoVeto",no_veto);

    m_data->Stores["ANNIEEvent"]->Get("LocalEventNumber",ev_number);
    m_data->Stores["ANNIEEvent"]->Get("EventNumber",global_ev_number);
    m_data->CStore.Get("LAPPD_EventTime",event_time_lappd);
    m_data->CStore.Get("LAPPD_Offset",lappd_offset);
    m_data->CStore.Get("LAPPD_TimeSinceBeam",time_since_beamgate);
    m_data->CStore.Get("LAPPD_InBeamWindow",in_beam_window);

    output_csv << run_number << ", ";
    output_csv << subrun_number << ", ";
    output_csv << part_number << ", ";
    output_csv << ev_number << ", ";
    output_csv << global_ev_number << ", ";
    output_csv << trigger_time << ", ";
    output_csv << event_time_lappd << ", ";
    output_csv << lappd_offset << ", ";
    output_csv << time_since_beamgate << ", ";
    output_csv << in_beam_window << ", ";
    output_csv << mrd_track << ", ";
    output_csv << no_veto << ", ";
    output_csv << std::endl;
  }

  return true;
}


bool saveLAPPDInfo::Finalise(){

  output_csv.close();
  return true;
}
