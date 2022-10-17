#ifndef ClusterDummy_H
#define ClusterDummy_H

#include <string>
#include <iostream>

#include "Tool.h"
#include "LAPPDHit.h"
#include "LAPPDPulse.h"
#include "Geometry.h"
#include "Detector.h"
#include "ANNIEalgorithms.h"
#include "LAPPDPulse.h"
#include "TVector3.h"




/**
 * \class ClusterDummy
 *
 * This is a blank template for a Tool used by the script to generate a new custom tool. Please fill out the description and author information.
*
* $Author: B.Richards $
* $Date: 2019/05/28 10:44:00 $
* Contact: b.richards@qmul.ac.uk
*/
class ClusterDummy: public Tool {


 public:

  ClusterDummy(); ///< Simple constructor
  bool Initialise(std::string configfile,DataModel &data); ///< Initialise Function for setting up Tool resources. @param configfile The path and name of the dynamic configuration file to read in. @param data A reference to the transient data class used to pass information between Tools.
  bool Execute(); ///< Execute function used to perform Tool purpose.
  bool Finalise(); ///< Finalise function used to clean up resources.


 private:





};


#endif
