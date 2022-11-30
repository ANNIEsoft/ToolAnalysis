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
  Position CalculateChargePointMC(std::vector<MCHit> cluster_hits, std::vector<unsigned long> cluster_detkeys);
  double CalculateChargeBalanceMC(std::vector<MCHit> cluster_hits, std::vector<unsigned long> cluster_detkeys);
  double CalculateMaxPEMC(std::vector<MCHit> cluster_hits, std::vector<unsigned long> cluster_detkeys);

 private:

  std::map<int,double> ChannelKeyToSPEMap;
  bool isData;

  std::map<double,std::vector<Hit>>* m_all_clusters = nullptr;  
  std::map<double,std::vector<MCHit>>* m_all_clusters_MC = nullptr;
  std::map<double,std::vector<unsigned long>>* m_all_clusters_detkey = nullptr;

  Geometry *geom = nullptr;

  std::map<int,unsigned long> pmtid_to_channelkey;
  std::map<unsigned long,int> channelkey_to_pmtid;

  /// \brief verbosity levels: if 'verbosity' < this level, the message type will be logged.
  int verbosity;
  int v_error=0;
  int v_warning=1;
  int v_message=2;
  int v_debug=3;
  std::string logmessage;
};


#endif
