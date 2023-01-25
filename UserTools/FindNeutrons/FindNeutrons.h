#ifndef FindNeutrons_H
#define FindNeutrons_H

#include <string>
#include <iostream>

#include "Tool.h"


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
  bool FindNeutronsByCB(); ///< Neutron identification by Charge Balance cut
  bool FillRecoParticle(); ///< Fill reco particle object with neutron information

 private:

  //configuration variables
  int verbosity;
  std::string Method;

  //verbosity variables
  int v_error=0;
  int v_warning=1;
  int v_message=2;
  int v_debug=3;
  int vv_debug=4;

};


#endif
