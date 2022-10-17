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

  // create ANNIEEvent BoostStore, if it doesn't exist

/*  if(!m_data->Stores.count("ANNIEEVENT")){
    m_data->Stores["ANNIEEvent"]= new BoostStore(false,0);
  }
  */


  return true;
}


bool GenerateHits::Execute(){

  //indexed by tube (not channel) id
  std::map<unsigned long,vector<LAPPDHit>> MCLAPPDHit;

  // photon hits on LAPPD, id=0
  int TubeID =0;
  std::vector<double> pos;
  std::vector<double> relpos;
  double tc,Q;

  int nhits,randomizepositions;
  m_variables.Get("nhits", nhits);
  m_variables.Get("radomizepositions", randomizepositions);

  double paracoord=0.0;
  double perpcoord=0.0;
  double trigtime=0.0;
  m_variables.Get("paracoord", paracoord);
  m_variables.Get("perpcoord", perpcoord);
  m_variables.Get("trigtime", trigtime);


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

    if(randomizepositions==1){
      relpos.push_back(fRand(-98, 98));
      relpos.push_back(fRand(-98, 98));
      tc = fRand(0, 24.9); //ns, converted to ps later in LAPPDSim processing
    }
    else{
      relpos.push_back(paracoord);
      relpos.push_back(perpcoord);
      tc = trigtime;
    }

    LAPPDHit temphit(TubeID,tc,Q,pos,relpos);
    pos.clear();
    relpos.clear();
    hits.push_back(temphit);

  }

  //cout << "inserting " << hits.size() << " synthetic hits " << endl;
  // stuff the hits into MCLAPPHit
  MCLAPPDHit.insert(pair <unsigned long,vector<LAPPDHit>> (0,hits));
  // add MCLAPPDHit to the ANNIEEvent store
  m_data->Stores["ANNIEEvent"]->Set("MCLAPPDHits",MCLAPPDHit);

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
