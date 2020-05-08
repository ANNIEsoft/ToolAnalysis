#ifndef SimpleTankEnergyCalibrator_H
#define SimpleTankEnergyCalibrator_H

#include <string>
#include <iostream>

#include "TMath.h"

#include "Tool.h"
#include "Hit.h"
#include "ADCPulse.h"
#include "Position.h"
#include "Geometry.h"
#include <boost/algorithm/string.hpp>
#include "BoostStore.h"
#include "Store.h"

/**
 * \class SimpleTankEnergyCalibrator
 *
 * This is a blank template for a Tool used by the script to generate a new custom tool. Please fill out the description and author information.
*
* $Author: B.Richards $
* $Date: 2019/05/28 10:44:00 $
* Contact: b.richards@qmul.ac.uk
*/
class SimpleTankEnergyCalibrator: public Tool {


 public:

  SimpleTankEnergyCalibrator(); ///< Simple constructor
  bool Initialise(std::string configfile,DataModel &data); ///< Initialise Function for setting up Tool resources. @param configfile The path and name of the dynamic configuration file to read in. @param data A reference to the transient data class used to pass information between Tools.
  bool Execute(); ///< Execute function used to perform Tool purpose.
  bool Finalise(); ///< Finalise function used to clean up resources.

  std::vector<Hit> GetInWindowHits();
  double GetTotalQ(std::vector<Hit> AllHits);
  double GetTotalPE(std::vector<Hit> AllHits);

 private:

  std::map<int,double> ChannelKeyToSPEMap;

  Geometry *geom = nullptr;

  std::map<unsigned long, std::vector<std::vector<ADCPulse>>> RecoADCHits;
  std::map<unsigned long, std::vector<Hit>>* Hits = nullptr;
  std::map<unsigned long,vector<Hit>>* TDCData=nullptr;


  int evnum;
  int TankBeamWindowStart;
  int TankBeamWindowEnd;
  int TankNHitThreshold;
  int MinPenetrationDepth;
  double MaxAngle;
  int MaxEntryPointRadius;

  int numsubevs;
  int numtracksinev;
  int EventNumber;
  Position StartVertex;
  Position StopVertex;
  std::vector<int> PMTsHit;
  int numpmtshit;
  std::map<int,std::vector<int>> paddlesInTrackReco;

  std::string MRDTriggertype;
  std::vector<BoostStore>* theMrdTracks;   // the reconstructed tracks
  double TrackAngle;
  double TrackAngleError;
  double PenetrationDepth;
  Position MrdEntryPoint;
  int LayersHit;
  double EnergyLoss; //in MeV
  double EnergyLossError;

  //verbosity levels: if 'verbosity' < this level, the message type will be logged.
  int verbosity=1;
  int v_error=0;
  int v_warning=1;
  int v_message=2;
  int v_debug=3;


};


#endif
