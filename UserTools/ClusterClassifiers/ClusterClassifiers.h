#ifndef ClusterClassifiers_H
#define ClusterClassifiers_H

#include <string>
#include <iostream>

#include "Tool.h"
#include "Direction.h"
#include "Position.h"
#include "Geometry.h"

/**
 * \class ClusterClassifiers
 *
 * This is a blank template for a Tool used by the script to generate a new custom tool. Please fill out the description and author information.
*
* $Author: B.Richards $
* $Date: 2019/05/28 10:44:00 $
* Contact: b.richards@qmul.ac.uk
*/
class ClusterClassifiers: public Tool {


 public:

  ClusterClassifiers(); ///< Simple constructor
  bool Initialise(std::string configfile,DataModel &data); ///< Initialise Function for setting up Tool resources. @param configfile The path and name of the dynamic configuration file to read in. @param data A reference to the transient data class used to pass information between Tools.
  bool Execute(); ///< Execute function used to perform Tool purpose.
  bool Finalise(); ///< Finalise function used to clean up resources.
  Position CalculateChargePoint(std::vector<Hit> cluster_hits);
  double CalculateChargeBalance(std::vector<Hit> cluster_hits);
  double CalculateMaxPE(std::vector<Hit> cluster_hits);

 private:

  std::map<int,double> ChannelKeyToSPEMap;

  std::map<double,std::vector<Hit>>* m_all_clusters = nullptr;  

  Geometry *geom = nullptr;

  /// \brief verbosity levels: if 'verbosity' < this level, the message type will be logged.
  int verbosity;
  int v_error=0;
  int v_warning=1;
  int v_message=2;
  int v_debug=3;
  std::string logmessage;
};


#endif
