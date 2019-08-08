#include "GenerateHits.h"
#include <cstdlib>
#include <time.h>

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

  int nhits;
  m_variables.Get("nhits", nhits);

  //initialize random seed
  srand(time(NULL));

  //all the hits
  vector<LAPPDHit> hits;

  //generate uniformly distributed hits
  //accross the surface of the LAPPD with
  //random values of position charge and time
  for(int i = 0; i < nhits; i++)
  {
    Q=1; 
    //abs pos is not used for my analysis at the moment
    pos.push_back(0.);
    pos.push_back(0.);
    pos.push_back(0.);
    relpos.push_back(fRand(-98, 98));
    relpos.push_back(fRand(-98, 98));
    tc = fRand(0, 24.9); //ns, converted to ps later in LAPPDSim processing
    LAPPDHit temphit(TubeID,tc,Q,pos,relpos);
    pos.clear();
    relpos.clear();
    hits.push_back(temphit);

  }

  cout << "inserting " << hits.size() << " synthetic hits " << endl;
  // stuff the two hits into MCLAPPHit
  MCLAPPDHit.insert(pair <int,vector<LAPPDHit>> (0,hits));

  // add MCLAPPDHit to the ANNIEEvent store
  m_data->Stores["ANNIEEvent"]->Set("MCLAPPDHit",MCLAPPDHit);

  return true;
}


double GenerateHits::fRand(double fMin, double fMax)
{
    double f = (double)rand() / RAND_MAX;
    return fMin + f * (fMax - fMin);
}

bool GenerateHits::Finalise(){
  cout << "Finalized generate alright " << endl;

  return true;
}
