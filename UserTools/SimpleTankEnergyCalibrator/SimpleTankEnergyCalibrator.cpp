#include "SimpleTankEnergyCalibrator.h"

SimpleTankEnergyCalibrator::SimpleTankEnergyCalibrator():Tool(){}


bool SimpleTankEnergyCalibrator::Initialise(std::string configfile, DataModel &data){

  /////////////////// Useful header ///////////////////////
  if(configfile!="") m_variables.Initialise(configfile); // loading config file
  //m_variables.Print();

  m_data= &data; //assigning transient data pointer
  /////////////////////////////////////////////////////////////////

  verbosity = 0;

  m_variables.Get("verbosity",verbosity);
  m_variables.Get("TankBeamWindowStart",TankBeamWindowStart);
  m_variables.Get("TankBeamWindowEnd",TankBeamWindowEnd);
  m_variables.Get("TankNHitThreshold",TankNHitThreshold);
  m_variables.Get("MinPenetrationDepth",MinPenetrationDepth);
  m_variables.Get("MaxAngle",MaxAngle);
  m_variables.Get("MaxEntryPointRadius",MaxEntryPointRadius);
  
  m_data->Stores["ANNIEEvent"]->Header->Get("AnnieGeometry",geom);
  return true;
}


bool SimpleTankEnergyCalibrator::Execute(){

  //All right, so let's do a few things:
  //  - Access the MRDTracks.  return true if the following conditions hold:
  //      Not one track
  //      Angle and minimum track length not satisfied
  //      Entry point not within the circle of consideration
  //  - If we're good here, then Loop through the hits in the time window.  Convert 
  //    each hit's charge to PE count using a ChargeToPE map.  For now, just have the
  //    total PE and estimated Muon energy in MRD printed.
  
  int annieeventexists = m_data->Stores.count("ANNIEEvent");
  if(!annieeventexists){ cerr<<"no ANNIEEvent store!"<<endl;}
  
  std::map<unsigned long, std::vector<std::vector<ADCPulse>>> RecoADCHits;
  //First, get the hits from the Store
  m_data->Stores["ANNIEEvent"]->Get("EventNumber", evnum);
  bool got_recoadc = m_data->Stores["ANNIEEvent"]->Get("RecoADCHits",RecoADCHits);

  bool got_hits = m_data->Stores["ANNIEEvent"]->Get("Hits", Hits);
  if (!got_hits){
    std::cout << "No Hits store in ANNIEEvent! " << std::endl;
    return false;
  }
  
  //Get beam window hits 
  std::vector<Hit> BeamHits = this->GetInWindowHits();
  if(BeamHits.size() < TankNHitThreshold){
    if(verbosity>3) std::cout << "SimpleTankEnergyCalculator Tool: Not enough tank hits in defined beam window" << std::endl;
    return true;
  }

  //Check muon track



  return true;
}


bool SimpleTankEnergyCalibrator::Finalise(){

  std::cout << "SimpleTankEnergyCalibrator exitting" << std::endl;
  return true;
}

  m_variables.Get("TankBeamWindowStart",TankBeamWindowStart);
  m_variables.Get("TankBeamWindowEnd",TankBeamWindowEnd);

std::vector<Hit> SimpleTankEnergyCalibrator::GetInWindowHits(){
  std::vector<Hit> BeamHits;

  for(std::pair<unsigned long, std::vector<Hit>>&& apair : *Hits){
    unsigned long chankey = apair.first;
    std::vector<Hit> ThisTubeHits = apair.second;
    for(int i=0; i<ThisTubeHits.size(); i++){
      Hit ahit = ThisTubeHits.at(i);
      double hittime = ahit.GetTime();
      if((hittime > TankBeamWindowStart) && (hittime < TankBeamWindowEnd)) BeamHits.push_back(ahit);
    }
  }
  return BeamHits;
}
