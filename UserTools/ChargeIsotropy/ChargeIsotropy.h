#ifndef ChargeIsotropy_H
#define ChargeIsotropy_H

#include <string>
#include <iostream>
#include <cmath>

#include "Tool.h"


/**
 * \class ChargeIsotropy
 *
 * This is a blank template for a Tool used by the script to generate a new custom tool. Please fill out the description and author information.
*
* $Author: James Minock $
* $Date: 2026/03/13 $
* Contact: jmm1018@physics.rutgers.edu
*/
class ChargeIsotropy: public Tool {


 public:

  ChargeIsotropy(); ///< Simple constructor
  bool Initialise(std::string configfile,DataModel &data); ///< Initialise Function for setting up Tool resources. @param configfile The path and name of the dynamic configuration file to read in. @param data A reference to the transient data class used to pass information between Tools.
  bool Execute(); ///< Execute function used to perform Tool purpose.
  bool Finalise(); ///< Finalise function used to clean up resources.

  double LawOfCosines(double vtxx, double vtxy, double vtxz, double pmt1x, double pmt1y, double pmt1z,
    double pmt2x, double pmt2y, double pmt2z); //< Law of Cosines. Returns angle between 2 tank PMT hits with respect to the reconstructed vertex



 private:

  //configuration variables
  int verbosity;  
  double time_window;
  bool isData;
  bool MCWaveform;

  //CStore/BoostStore variables
  Geometry *geom = nullptr;
  std::map<unsigned long, int> channelkey_to_pmtid;
  std::map<int, unsigned long> pmtid_to_channelkey;
  std::map<int,double> ChannelKeyToSPEMap;

  //reconstruction variables
  double Qij; //< Charge Isotropy value
  Position SimpleRecoVtx;

  //verbosity variables
  int v_error=0;
  int v_warning=1;
  int v_message=2;
  int v_debug=3;
  int vv_debug=4;

};


#endif
