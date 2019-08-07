#include "MCHitToHitComparer.h"

MCHitToHitComparer::MCHitToHitComparer():Tool(){}


bool MCHitToHitComparer::Initialise(std::string configfile, DataModel &data){

  /////////////////// Useful header ///////////////////////
  if(configfile!="") m_variables.Initialise(configfile); // loading config file
  //m_variables.Print();

  m_data= &data; //assigning transient data pointer
  /////////////////////////////////////////////////////////////////
  m_variables.Get("verbosity",verbosity);
  
  return true;
}


bool MCHitToHitComparer::Execute(){
  // Get a pointer to the ANNIEEvent Store
  auto* annie_event = m_data->Stores["ANNIEEvent"];
  if (!annie_event) {
    Log("Error: The MCHitToHitComparer tool could not find the ANNIEEvent Store", v_error,
      verbosity);
    return false;
  }
 
  // Load the map containing WCSim's MCHits
  bool got_mchitmap = annie_event->Get("MCHits", mchit_map);

  // Check for problems
  if ( !got_mchitmap ) {
    Log("Error: The MCHitToHitComparer tool could not find a mchits map", v_error,
      verbosity);
    return false;
  }
  else if ( mchit_map->empty() ) {
    Log("Error: The MCHitToHitComparer tool found an empty mchits map", v_error,
      verbosity);
    return false;
  }
 

  // Load the map containing the ADC raw waveform data
  bool got_hitmap = annie_event->Get("Hits", hit_map);

  // Check for problems
  if ( !got_hitmap ) {
    Log("Error: The MCHitToHitComparer tool could not find a hits map", v_error,
      verbosity);
    return false;
  }
  else if ( hit_map->empty() ) {
    Log("Error: The MCHitToHitComparer tool found an empty hits map", v_error,
      verbosity);
    return false;
  }
  //Now, we loop through all the MCHits and Hits for each PMT.  Print out stuff to compare
  for(std::pair<unsigned long,std::vector<MCHit>>&& temp_pair : *mchit_map){
    unsigned long channel_key = temp_pair.first;
    std::vector<MCHit>& mchits_vector = temp_pair.second;
    for(MCHit& anmchit : mchits_vector){
      std::cout << "MCHIT'S ID,TIME, CHARGE: " << anmchit.GetTubeId() << "," <<
          anmchit.GetTime() << "," << anmchit.GetCharge() << std::endl;
    }
  }
  for(std::pair<unsigned long,std::vector<Hit>>&& temp_pair : *hit_map){
    unsigned long channel_key = temp_pair.first;
    std::vector<Hit>& hits_vector = temp_pair.second;
    for(Hit& ahit : hits_vector){
      std::cout << "HIT'S ID,TIME, CHARGE: " << ahit.GetTubeId() << "," <<
          ahit.GetTime() << "," << ahit.GetCharge() << std::endl;
    }
  }
  //TODO: Add some additional checks, 
  return true;
}


bool MCHitToHitComparer::Finalise(){

  return true;
}
