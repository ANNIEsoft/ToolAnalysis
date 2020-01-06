#ifndef MrdPaddleEfficiency_H
#define MrdPaddleEfficiency_H

#include <string>
#include <iostream>
#include <vector>
#include <map>
#include <fstream>

#include "Tool.h"
#include "Geometry.h"
#include "Paddle.h"
#include "Detector.h"
#include "Hit.h"
#include "MRDSubEventClass.hh"      // a class for defining subevents
#include "MRDTrackClass.hh"         // a class for defining MRD tracks

#include "TH1D.h"
#include "TFile.h"


/**
 * \class MrdPaddleEfficiency
 *
 * This is a blank template for a Tool used by the script to generate a new custom tool. Please fill out the description and author information.
*
* $Author: B.Richards $
* $Date: 2019/05/28 10:44:00 $
* Contact: b.richards@qmul.ac.uk
*/

class MrdPaddleEfficiency: public Tool {

 public:

  MrdPaddleEfficiency(); ///< Simple constructor
  bool Initialise(std::string configfile,DataModel &data); ///< Initialise Function for setting up Tool resources. @param configfile The path and name of the dynamic configuration file to read in. @param data A reference to the transient data class used to pass information between Tools.
  bool Execute(); ///< Execute function used to perform Tool purpose.
  bool Finalise(); ///< Finalise function used to clean up resources.
  bool FindPaddleIntersection(Position startpos, Position endpos, double &x, double &y, double z);



 private:

 	int verbosity;
 	std::vector<BoostStore>* theMrdTracks;                        // the reconstructed tracks
 	std::string MRDTriggertype;
 	Geometry *geom = nullptr;

 	int numsubevs;
 	int numtracksinev;
 	int EventNumber;
 	Position StartVertex;
 	Position StopVertex;
 	std::vector<int> PMTsHit;
 	int MrdTrackID;
 	double HtrackFitChi2;
 	double VtrackFitChi2;
 	std::vector<int> LayersHit;
 	std::map<int,std::vector<int>> paddlesInTrackReco;

 	ofstream property_file;

 	TH1D *hist_numtracks = nullptr;
 	TH1D *hist_pmtshit = nullptr;
 	TH1D *hist_htrackfitchi2 = nullptr;
 	TH1D *hist_vtrackfitchi2 = nullptr;
 	TH1D *hist_layershit = nullptr;
 	TH1D *hist_tracklength = nullptr;
 	TFile *hist_file = nullptr;

 	std::map<int,std::map<unsigned long,TH1D*>> observed_MRDHits;
 	std::map<int,TH1D*> expected_MRDHits;
 	std::map<int,double> zLayers;
 	std::map<int,int> orientationLayers;

 	int v_error = 0;
 	int v_warning = 1;
 	int v_message = 2;
 	int v_debug = 3;


 };

 #endif