#ifndef MCPropertiesToTree_H
#define MCPropertiesToTree_H

#include <string>
#include <iostream>

#include "TFile.h"
#include "TH1F.h"
#include "TTree.h"

#include "Tool.h"
#include "Geometry.h"
#include "Particle.h"
#include "TriggerClass.h"
#include "Geometry.h"
#include "TimeClass.h"

/**
 * \class MCPropertiesToTree
 *
 * 
* $Author: M.Nieslony $
* $Date: 2019/09/16 15:28:00 $
* Contact: mnieslon@uni-mainz.de
*/

class MCPropertiesToTree: public Tool {


 public:

  MCPropertiesToTree(); ///< Simple constructor
  bool Initialise(std::string configfile,DataModel &data); ///< Initialise Function for setting up Tool resources. @param configfile The path and name of the dynamic configuration file to read in. @param data A reference to the transient data class used to pass information between Tools.
  bool Execute(); ///< Execute function used to perform Tool purpose.
  bool Finalise(); ///< Finalise function used to clean up resources.


 private:

	int verbosity;
	std::string outfile_name;
	
	Geometry *geom = nullptr;
	std::vector<MCParticle>* mcparticles = nullptr;
	std::vector<TriggerClass>* TriggerData = nullptr;
	uint16_t MCTriggernum;
	std::map<unsigned long, std::vector<MCHit>>* MCHits=nullptr;
    std::map<unsigned long, std::vector<MCLAPPDHit>>* MCLAPPDHits=nullptr;
    std::map<unsigned long,vector<MCHit>>* TDCData;
    int evnum;
	int nrings;
	TimeClass TriggerTime;	

	double tank_center_x, tank_center_y, tank_center_z;

	TFile *f = nullptr;
  	TH1F *hE = nullptr;
  	TH1F *hPosX = nullptr;
  	TH1F *hPosY = nullptr;
  	TH1F *hPosZ = nullptr;
  	TH1F *hPosStopX = nullptr;
  	TH1F *hPosStopY = nullptr;
  	TH1F *hPosStopZ = nullptr;
  	TH1F *hDirX = nullptr;
  	TH1F *hDirY = nullptr;
  	TH1F *hDirZ = nullptr;
  	TH1F *hQ = nullptr;
  	TH1F *hT = nullptr;
	TH1F *hQtotal = nullptr;
 	TH1F *hQ_LAPPD = nullptr;
  	TH1F *hT_LAPPD = nullptr;
	TH1F *hQtotal_LAPPD = nullptr;
  	TH1F *hNumPrimaries = nullptr;
  	TH1F *hNumSecondaries = nullptr;
  	TH1F *hPDGPrimaries = nullptr;
  	TH1F *hPDGSecondaries = nullptr;
  	TH1F *hMRDPaddles = nullptr;
  	TH1F *hMRDLayers = nullptr;
	TH1F *hMRDClusters = nullptr;
  	TH1F *hPMTHits = nullptr;
  	TH1F *hLAPPDHits = nullptr;
	TH1F *hRings = nullptr;
	TH1F *hNoPiK = nullptr;
	TH1F *hMRDStop = nullptr;
	TH1F *hFV = nullptr;
	TH1F *hPMTVol = nullptr;
	TTree *t = nullptr;

	std::vector<double> *particleE = nullptr;
	std::vector<int> *particlePDG = nullptr;
	std::vector<int> *particlePDG_primaries = nullptr;
	std::vector<int> *particlePDG_secondaries = nullptr;
	std::vector<int> *particleParentPDG = nullptr;
	std::vector<int> *particleFlag = nullptr;
	bool is_prompt;
	ULong64_t trigger_time;
	int particleTriggers;
	int num_primaries;
	int num_secondaries;
	int pmtHits, lappdHits, mrdPaddles, mrdLayers, mrdClusters;
	bool mrd_stop, event_fv, event_pmtvol, no_pik;
	std::vector<double> *pmtQ = nullptr;
	std::vector<double> *pmtT = nullptr;
	std::vector<double> *lappdQ = nullptr;
	std::vector<double> *lappdT = nullptr;
	double pmtQtotal, lappdQtotal;
	std::vector<double> *particle_posX = nullptr;
	std::vector<double> *particle_posY = nullptr;
	std::vector<double> *particle_posZ = nullptr;
	std::vector<double> *particle_stopposX = nullptr;
	std::vector<double> *particle_stopposY = nullptr;
	std::vector<double> *particle_stopposZ = nullptr;
	std::vector<double> *particle_dirX = nullptr;
	std::vector<double> *particle_dirY = nullptr;
	std::vector<double> *particle_dirZ = nullptr;

	int v_error = 0;
	int v_warning = 1;
	int v_message = 2;
	int v_debug = 3;

};


#endif
