#include "NeutronMultiplicity.h"

NeutronMultiplicity::NeutronMultiplicity():Tool(){}


bool NeutronMultiplicity::Initialise(std::string configfile, DataModel &data){

  if(configfile!="") m_variables.Initialise(configfile); // loading config file
  m_data= &data; //assigning transient data pointer

  m_variables.Get("verbosity",verbosity);

  return true;
}


bool NeutronMultiplicity::Execute(){

  std::vector<Particle> Particles;
  bool get_ok = m_data->Stores["ANNIEEvent"]->Get("Particles",Particles);

  if (get_ok){
    for (int i_particle=0; i_particle < (int) Particles.size(); i_particle++){
      Log("NeutronMultiplicity tool: Retrieved particle # "+std::to_string(i_particle)+"...",v_message,verbosity);
      if (verbosity > 2) Particles.at(i_particle).Print();
    }
  }

  return true;
}


bool NeutronMultiplicity::Finalise(){

  return true;
}
