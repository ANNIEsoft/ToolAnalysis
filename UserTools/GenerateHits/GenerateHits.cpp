#include "GenerateHits.h"

GenerateHits::GenerateHits():Tool(){}


bool GenerateHits::Initialise(std::string configfile, DataModel &data){

  /////////////////// Usefull header ///////////////////////
  if(configfile!="")  m_variables.Initialise(configfile); //loading config file
  //m_variables.Print();

  m_data= &data; //assigning transient data pointer
  /////////////////////////////////////////////////////////////////

  return true;
}


bool GenerateHits::Execute(){

  //indexed by tube (not channel) id
  std::map<int,vector<LAPPDHit>> MCLAPPDHit;

  // store two photon hits on LAPPD, id=0
  int TubeID =0;
  std::vector<double> pos;
  std::vector<double> relpos;
  double tc,Q;

  // MC truth hit 1
  Q=1.;
  pos.push_back(10.);
  pos.push_back(5.);
  pos.push_back(0.);
  relpos.push_back(10.);
  relpos.push_back(5.);
  tc = 1.050;
  LAPPDHit ahit1(TubeID,tc,Q,pos,relpos);

  // clear vectors
  pos.clear();
  relpos.clear();

/*
  // MC truth hit 2
  tc = 1.140;
  Q=1.;
  pos.push_back(20.);
  pos.push_back(10.);
  pos.push_back(0.);
  relpos.push_back(20.);
  relpos.push_back(10.);
  LAPPDHit ahit2(TubeID,tc,Q,pos,relpos);
*/

  vector<LAPPDHit> hits;
  hits.push_back(ahit1);
//  hits.push_back(ahit2);

  // stuff the two hits into MCLAPPHit
  MCLAPPDHit.insert(pair <int,vector<LAPPDHit>> (0,hits));

  // add MCLAPPDHit to the ANNIEEvent store
  m_data->Stores["ANNIEEvent"]->Set("MCLAPPDHit",MCLAPPDHit);

  return true;
}


bool GenerateHits::Finalise(){

  return true;
}
