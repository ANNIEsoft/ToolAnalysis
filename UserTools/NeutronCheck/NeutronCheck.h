#ifndef NeutronCheck_H
#define NeutronCheck_H

#include <string>
#include <iostream>
#include <cmath>
#include <vector>

#include "Tool.h"
#include "Particle.h"
#include "Position.h"
#include "RecoVertex.h"
#include "RecoCluster.h"

#include "TTree.h"
#include "TFile.h"
#include "TH1.h"
#include "TH2.h"
#include "TH3.h"
#include "TGraph.h"
#include "TGraphErrors.h"
#include "TMath.h"



/**
 * \class NeutronCheck
 *
 * This is a blank template for a Tool used by the script to generate a new custom tool. Please fill out the description and author information.
*
* $Author: F. A. Lemmons $
* $Date: 2025/02/28 10:44:00 $
* Contact: franklin.lemmons@mines.sdsmt.edu
*/
class NeutronCheck: public Tool {


 public:

  NeutronCheck(); ///< Simple constructor
  bool Initialise(std::string configfile,DataModel &data); ///< Initialise Function for setting up Tool resources. @param configfile The path and name of the dynamic configuration file to read in. @param data A reference to the transient data class used to pass information between Tools.
  bool Execute(); ///< Execute function used to perform Tool purpose.
  bool Finalise(); ///< Finalise function used to clean up resources.


 private:

	 bool GetClusterInformation();
	 double ASCheck(std::vector<RecoDigit>*& digits,int mode);
	 void CSCheck();
	 void ResetVariables();

	 int verbosity;
	 std::string outfile;
	 bool useCleanEvent=0;
	 bool useCleanCluster=0;
	 bool useCleanDigit=0;
	 std::string EventStore;
	 bool fFinderCompare;
	 bool fParticleInfo;
	 double fDelayThreshold=10000;
	 bool fVertexInfo;
	 bool fIsData;

	 TTree* NeutCheckTree = nullptr;

	 /*
	 -EventNumber
	 -Clusternumber
	 -number of clusters
	 -neutron multiplicity


	 -Cluster PDG
	 -Cluster Charge
	 -Cluster Time
	 -CB
	 -AS 0,1,2

	 */

	 std::vector<int> cluster_neutron;
	 std::vector<double> cluster_times_neutron;
	 std::vector<double> cluster_charges_neutron;
	 std::vector<double> cluster_cb_neutron;
	 std::vector<double> cluster_times;
	 std::vector<double> cluster_charges;
	 std::vector<double> cluster_cb;

	 std::vector<double> fDigitCharges;
	 std::vector<double> fDigitTimes;

	 std::map<double, int>* fClusterToBestParticleID = nullptr;
	 std::map<double, int>* fClusterToBestParticlePDG = nullptr;
	 std::map<double, double>* fClusterEfficiency = nullptr;
	 std::map<double, double>* fClusterPurityMap = nullptr;
	 std::map<double, double>* fClusterTotalCharge = nullptr;
	 std::vector<RecoDigit>* fDigitList=nullptr;
	 vector<RecoCluster>* fRecoClusters;

	 std::vector<MCParticle>* fMCParticles;
	 std::vector<double> fMCNeutCapTimes;
	 std::vector<double> fMCNeutCapX;
	 std::vector<double> fMCNeutCapY;
	 std::vector<double> fMCNeutCapZ;
	 std::vector<double> fMCNeutCapNucleus;


	 std::map<double,std::vector<MCHit>>* m_all_clusters;

	 Geometry* fGeometry = nullptr;    ///< ANNIE Geometry

	 /// \brief MC entry number
	 uint32_t fMCEventNum;
	 vector<int> fClusterNum;
	 int fClusterCount;

	 vector<int> fClusterNDigits;
	 vector<int> fClusterPDG;
	 vector<int> fClusterParentPDG;
	 vector<int> fClusterCoincPDG;
	 vector<double> fClusterParticleEnergy;
	 vector<double> fClusterCharge;
	 vector<double> fClusterPurity;
	 vector<double> fClusterCB;
	 vector<double> fClusterTime;
	 vector<double> fClusterAS0;
	 vector<double> fClusterAS1;
	 vector<double> fClusterAS2;
	 vector<double> fClusterASC;
	 vector<double> fClusterAMD;
	 vector<double> fClusterAW;
	 vector<double> fClusterB1,fClusterB2,fClusterB3,fClusterB4,fClusterB5;
	 vector<double> fClusterTRT,fClusterTRC,fClusterTRQ,fClusterTRRTQ,fClusterTRRTC,fClusterTRRQC;
	 //vector<double> fClusterSA;
	 //vector<Position> fClusterCV;
	 vector<double> fClusterCVX,fClusterCVY,fClusterCVZ,fClusterCVR;
	 vector<double> fClusterSphericity, fClusterPlanarity;

	 vector<int> fClusterMode;
	 vector<int> fClusterHits;

	 vector<int> fParticleNumber;
	 vector<int> fParticlePDG;
	 vector<int> fParticleParent;
	 vector<double> fParticleStartEnergy;
	 vector<double> fParticleStartTime;
	 vector<double> fParticleStopTime;

	 vector<int> fFinderClusterNum;
	 int fFinderClusterCount;
	 vector<int> fFinderPDG;
	 vector<int> fFinderParentPDG;
	 vector<double> fFinderCharge;
	 vector<double> fFinderPurity;
	 vector<double> fFinderCB;
	 vector<double> fFinderTime;

	 
	 int fTrueNeutronMult;
	 int fTrueNeutronDelayed;
	 int fNeutronMult;
	 double true_Emu;
	 double true_Enu;
	 double TrueQ2;
	 double CalcQ2;

	 double classicEmu;
	 double classicPt;

	 double trueVtxX, trueVtxY, trueVtxZ;
	 double recoVtxX, recoVtxY, recoVtxZ;
	 double deltaVtxX, deltaVtxY, deltaVtxZ, deltaVtxR;
	 double trueDirX,trueDirY,trueDirZ;
	 double recoDirX,recoDirY,recoDirZ;
	 double recoVtxFOM;
	 int vtxRecoStatus;

	 /// \brief trigger number
	 uint16_t fMCTriggerNum;

	 /// \brief ANNIE event number
	 uint32_t fEventNumber;

	 int fEventStatusApplied;
	 int fEventStatusFlagged;

	 

	 int reco_Clusters;

	 TFile* fOutput_tfile;

	 //verbosity variables
	 int v_error = 0;
	 int v_warning = 1;
	 int v_message = 2;
	 int v_debug = 3;
	 int vv_debug = 4;


};


#endif
