#ifndef LAPPDCluster_H
#define LAPPDCluster_H

#include <string>
#include <iostream>

#include "Tool.h"
#include "Geometry.h"
#include "LAPPDFindPeak.h"
#include "Channel.h"
#include "Detector.h"


/**
 * \class LAPPDCluster
 *
 * This is a blank template for a Tool used by the script to generate a new custom tool. Please fill out the description and author information.
*
* $Author: B.Richards $
* $Date: 2019/05/28 10:44:00 $
* Contact: b.richards@qmul.ac.uk
*/
class LAPPDCluster: public Tool {


 public:

  LAPPDCluster(); ///< Simple constructor
  bool Initialise(std::string configfile,DataModel &data); ///< Initialise Function for setting up Tool resources. @param configfile The path and name of the dynamic configuration file to read in. @param data A reference to the transient data class used to pass information between Tools.
  bool Execute(); ///< Execute function used to perform Tool purpose.
  bool Finalise(); ///< Finalise function used to clean up resources.


 private:

    Geometry* _geom;

    string HitOutLabel;
    string SimpleClusterLabel;
    string CFDClusterLabel;
    int ClusterVerbosity;


};


#endif
