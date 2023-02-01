#include "checkLAPPDStatus.h"

checkLAPPDStatus::checkLAPPDStatus():Tool(){}


bool checkLAPPDStatus::Initialise(std::string configfile, DataModel &data){

  /////////////////// Useful header ///////////////////////
  if(configfile!="") m_variables.Initialise(configfile); // loading config file
  //m_variables.Print();

  m_data= &data; //assigning transient data pointer
  /////////////////////////////////////////////////////////////////

  return true;
}


bool checkLAPPDStatus::Execute(){

  std::map<std::string,bool> DataStreams;
  m_data->Stores["ANNIEEvent"]->Get("DataStreams",DataStreams);
  for (std::map<std::string,bool>::iterator it=DataStreams.begin(); it!= DataStreams.end(); it++){
    //std::cout <<it->first<<": "<<it->second<<std::endl;
  }

  m_data->CStore.Set("LAPPD_HasData",false);

  if (DataStreams["LAPPD"] == 1){
    PsecData psec;
    uint64_t event_time_lappd, lappd_offset;
    m_data->Stores["ANNIEEvent"]->Get("LAPPDData",psec);
    m_data->Stores["ANNIEEvent"]->Get("EventTimeLAPPD",event_time_lappd);
    m_data->Stores["ANNIEEvent"]->Get("LAPPDOffset",lappd_offset);
    std::cout <<"EventTimeLAPPD: "<<event_time_lappd<<", LAPPDOffset: "<<lappd_offset<<std::endl;
    psec.Print();
    m_data->CStore.Set("LAPPD_HasData",true);
    m_data->CStore.Set("LAPPD_Psec",psec);
    m_data->CStore.Set("LAPPD_EventTime",event_time_lappd);
    m_data->CStore.Set("LAPPD_Offset",lappd_offset);
  }
  
  return true;
}


bool checkLAPPDStatus::Finalise(){

  return true;
}
