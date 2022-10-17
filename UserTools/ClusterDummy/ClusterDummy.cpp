#include "ClusterDummy.h"

ClusterDummy::ClusterDummy():Tool(){}


bool ClusterDummy::Initialise(std::string configfile, DataModel &data)
{
  
  /////////////////// Useful header ///////////////////////
  if(configfile!="") m_variables.Initialise(configfile); // loading config file
  //m_variables.Print();
  
  m_data= &data; //assigning transient data pointer
  /////////////////////////////////////////////////////////////////
  return true;
}


bool ClusterDummy::Execute()
{
  std::map<unsigned long,vector<LAPPDPulse>> SimpleRecoLAPPDPulses;
  m_data->Stores["ANNIEEvent"]->Get("SimpleRecoLAPPDPulses",SimpleRecoLAPPDPulses);
  LAPPDHit Hit;
  vector<LAPPDHit> HitVect;
  HitVect.push_back(Hit);
  std::map<unsigned long, vector<LAPPDHit>> Hits;

  map <unsigned long,vector<LAPPDPulse>> :: iterator itr;

  for(itr = SimpleRecoLAPPDPulses.begin(); itr !=SimpleRecoLAPPDPulses.end(); ++itr)
    {
      unsigned long channel= itr->first;
      Hits.insert(pair<unsigned long, vector<LAPPDHit>> (channel,HitVect));
    }
  m_data->Stores["ANNIEEvent"]->Set("Clusters", Hits);
  return true;
}


bool ClusterDummy::Finalise()
{
  
  return true;
}
