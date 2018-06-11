#include "LAPPDlasertestHitFinder.h"


const double c = 29.98; //cm/ns
const int side = 1;



LAPPDlasertestHitFinder::LAPPDlasertestHitFinder():Tool(){}


bool LAPPDlasertestHitFinder::Initialise(std::string configfile, DataModel &data){

  /////////////////// Usefull header ///////////////////////
  if(configfile!="")  m_variables.Initialise(configfile); //loading config file
  //m_variables.Print();

  m_data= &data; //assigning transient data pointer
  /////////////////////////////////////////////////////////////////

  m_variables.Get("stripID1",stripID.at(0));
  m_variables.Get("stripID2",stripID.at(1));
  m_variables.Get("stripID3",stripID.at(2));
  m_variables.Get("MaxTimeWindow", MaxTimeWindow);
  m_variables.Get("MinTimeWindow", MinTimeWindow);
  m_variables.Get("TwoSided", TwoSided);
  m_variables.Get("CenterChannel", CenterChannel);
  m_variables.Get("PTRange", PTRange);

  return true;
}


bool LAPPDlasertestHitFinder::Execute(){

std::vector<double> AbsPosition;
AbsPosition.push_back(0);
AbsPosition.push_back(0);
AbsPosition.push_back(0);
std::vector<double> LocalPosition;
std::vector<LAPPDPulse> NeighboursPulses;
double ParaPosition;
double PerpPosition;
double HitTime;

std::map<int,vector<LAPPDPulse>> LAPPDPulseCluster;
m_data->Stores["ANNIEEvent"]->Get("SimpleRecoLAPPDPulses",LAPPDPulseCluster);

LAPPDPulse MaxPulse;
double MaxAmp =-1.0;

//Find the biggest pulse inside the time window
  for (int i = 0; i < 3; i++){
    std::vector<LAPPDPulse> TempVector;
    TempVector = LAPPDPulseCluster.at(i);

    for (int j = 0; j < TempVector.size(); j++){
      if (TempVector.at(j).GetTheTime().GetNs() >= MinTimeWindow && TempVector.at(j).GetTheTime().GetNs() <= MaxTimeWindow){
        if (TempVector.at(j).GetPeak() > MaxAmp){
          MaxAmp = TempVector.at(j).GetPeak();
          MaxPulse = LAPPDPulseCluster.at(i).at(j);
        }
      }
    }
  }


//ParallelPosition and time

  if (TwoSided) {

    // what is the strip ID of the max channel?
    int cid = MaxPulse.GetChannelID();
    int sid = stripID.at(cid);

    bool hasopposite=false;
    // look to see if we have an opposite strip...if so, we grab it
    std::vector<LAPPDPulse> negaVector;
    for(int k=0; k<3; k++){

      if(sid == -stripID.at(k)){
        hasopposite=true;
        negaVector = LAPPDPulseCluster.at(k);
      }
    }

    // is there a pulse in the time window, on the opposite side of the strip? (I hope so)
    LAPPDPulse OpposPulse;
    for (int j = 0; j < negaVector.size(); j++){
    //Find pulse inside this vector that has the same peak and thetime
      if (fabs(negaVector.at(j).GetTheTime().GetNs() - MaxPulse.GetTheTime().GetNs()) < PTRange && (fabs(negaVector.at(j).GetPeak() - MaxAmp) / MaxAmp) <= 0.1){
        OpposPulse = negaVector.at(j);
        Deltatime = (MaxPulse.GetTpsec() - OpposPulse.GetTpsec());
      }
      ParaPosition = Deltatime*c;
      LocalPosition.push_back(ParaPosition);
      HitTime = (MaxPulse.GetTpsec() + OpposPulse.GetTpsec())/2;
    }
  }
  else {
    ParaPosition = 0;
    HitTime = MaxPulse.GetTpsec();
  }


//Perpendicular position
  std::vector<int> x(3);
  double SumAbove;
  double SumBelow;
  x[0] = -1;
  x[1] = 0;
  x[2] = 1;
  if (side == 1 && CenterChannel == MaxPulse.GetChannelID()){
    for (int i = -1; i < 2; i++){
      std::vector<LAPPDPulse> neighbourVector;
      neighbourVector = LAPPDPulseCluster.at(MaxPulse.GetChannelID() + i);

      for (int j = 0; j < neighbourVector.size(); j++){
        if (fabs(neighbourVector.at(j).GetTheTime().GetNs() - MaxPulse.GetTheTime().GetNs()) < PTRange){
          NeighboursPulses.push_back(neighbourVector.at(j));
        }
      }
    }


    for (int i=0; i<3; i++){
      SumAbove +=  (x.at(i) * NeighboursPulses.at(i).GetPeak());
      SumBelow +=  (NeighboursPulses.at(i).GetPeak());
    }
    PerpPosition = SumAbove / SumBelow;
  }

  //Store data in LAPPDHit
  LAPPDHit kHit(MaxPulse.GetChannelID(), MaxPulse.GetTheTime(), 0, AbsPosition, LocalPosition, MaxPulse.GetTpsec());
  m_data->Stores["ANNIEEvent"]->Set("RecoLaserTestHit",kHit);
  m_data->Stores["ANNIEEvent"]->Set("isTwoSided",TwoSided);
  if(TwoSided){


  } else{


  }

  return true;
}


bool LAPPDlasertestHitFinder::Finalise(){

  return true;
}
