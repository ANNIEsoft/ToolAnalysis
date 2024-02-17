#ifndef FindNeutrons_H
#define FindNeutrons_H

#include <string>
#include <iostream>

#include "Tool.h"
#include "Particle.h"

/**
 * \class FindNeutrons
 *
 * This is a blank template for a Tool used by the script to generate a new custom tool. Please fill out the description and author information.
*
* $Author: M. Nieslony $
* $Date: 2023/01/23 10:44:00 $
* Contact: mnieslon@uni-mainz.de
*/
class FindNeutrons: public Tool {


 public:

  FindNeutrons(); ///< Simple constructor
  bool Initialise(std::string configfile,DataModel &data); ///< Initialise Function for setting up Tool resources. @param configfile The path and name of the dynamic configuration file to read in. @param data A reference to the transient data class used to pass information between Tools.
  bool Execute(); ///< Execute function used to perform Tool purpose.
  bool Finalise(); ///< Finalise function used to clean up resources.

  bool FindNeutronCandidates(std::string method); ///< Neutron identification
  bool FindNeutronsByCB(bool strict); ///< Neutron identification by Charge Balance cut
  bool FindNeutronsByNHits(int nhits_thr); /// < Neutron identification by NHits
  bool FillRecoParticles(); ///< Fill reco particle object with neutron information
  bool LoadNeutronEfficiencyMap(); ///< Load neutron efficiency map (from calibration/simulation)

 private:

  //configuration variables
  int verbosity;
  std::string Method;
  std::string EfficiencyMapPath;

  //vectors storing neutron candidate properties
  std::vector<int> cluster_neutron;
  std::vector<double> cluster_times_neutron;
  std::vector<double> cluster_charges_neutron;
  std::vector<double> cluster_cb_neutron;
  std::vector<int> cluster_nhits_neutron;
  std::vector<double> cluster_times;
  std::vector<double> cluster_charges;
  std::vector<double> cluster_cb;
  std::vector<int> cluster_nhits;

  //vector storing neutron particles
  std::vector<Particle> vec_neutrons;

  //map storing the efficiency map for a given cut (from calibration / simulation)
  std::map<std::vector<double>,double> eff_map;  

  //verbosity variables
  int v_error=0;
  int v_warning=1;
  int v_message=2;
  int v_debug=3;
  int vv_debug=4;

};


#endif
