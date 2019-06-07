#include "CheckDetectorCounts.h"

CheckDetectorCounts::CheckDetectorCounts():Tool(){}


bool CheckDetectorCounts::Initialise(std::string configfile, DataModel &data){

  /////////////////// Usefull header ///////////////////////
  if(configfile!="")  m_variables.Initialise(configfile); //loading config file
  //m_variables.Print();

  m_data= &data; //assigning transient data pointer
  /////////////////////////////////////////////////////////////////
  
  Geometry* anniegeom=nullptr;
  int get_ok = m_data->Stores.at("ANNIEEvent")->Header->Get("AnnieGeometry",anniegeom);
  if(get_ok==0) cerr<<"couldn't find the AnnieGeometry!"<<endl;
  int numtankpmts = anniegeom->GetNumDetectorsInSet("Tank");
  int numlappdpmts = anniegeom->GetNumDetectorsInSet("LAPPD");
  int nummrdpmts = anniegeom->GetNumDetectorsInSet("MRD");
  int numvetopmts = anniegeom->GetNumDetectorsInSet("Veto");
  
  cout<<"detector had "<<numlappdpmts<<" LAPPDs, "<<numtankpmts<<" PMTs, "
      <<nummrdpmts<<" MRD PMTs and "<<numvetopmts<<" Veto PMTs"<<endl;

  return true;
}


bool CheckDetectorCounts::Execute(){

  return true;
}


bool CheckDetectorCounts::Finalise(){

  return true;
}
