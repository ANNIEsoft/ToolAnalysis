/* vim:set noexpandtab tabstop=4 wrap */
#ifndef MrdDistributions_H
#define MrdDistributions_H

#include <string>
#include <iostream>

#include "Tool.h"
#include "TApplication.h"
#include "TStyle.h"
#include "TCanvas.h"
#include "TROOT.h"
#include "Math/Vector3D.h"

class TH1F;
class TH2F;
class TH3D;
class TFile;
class TTree;

class MrdDistributions: public Tool {
	
	public:
	// from config file
	int verbosity=1;
	bool printTracks;
	std::string plotDirectory;  // where to save images and plots
	
	MrdDistributions();
	bool Initialise(std::string configfile,DataModel &data);
	bool Execute();
	bool Finalise();
	
	private:
	// retrieved from ANNIEEvent
	// ~~~~~~~~~~~~~~~~~~~~~~~~~
	std::map<unsigned long,std::vector<MCHit>>* TDCData=nullptr;
	std::map<unsigned long,int> channelkey_to_mrdpmtid;
	int numsubevs;
	int numtracksinev;
	
	std::string MCFile;     //-> currentfilestring
	uint32_t RunNumber;     // -> runnum     (keep as the type is different: TODO align types)
	uint32_t SubrunNumber;  // -> subrunnum  ( " " )
	uint32_t EventNumber;   // -> eventnum   ( " " )
	uint16_t MCTriggernum;  // -> triggernum ( " " )
	uint64_t MCEventNum;    // not yet in MRDTrackClass
	ULong64_t MCEventNumRoot;
	int NumHitMrdPMTsInEvent;
	int NumMrdHitsInEvent;
	int NumHitMrdPMTsInSubEvent;
	int NumMrdHitsInSubEvent;
	
	// track-wise
	int NumPmtsHit;
	
	// Retrieved from ANNIEvent
	std::map<int,std::map<unsigned long,double>>* ParticleId_to_MrdTubeIds;
	std::map<int,int> Reco_to_True_Id_Map;
	std::map<int,int>* trackid_to_mcparticleindex=nullptr;
	std::map<int,int> True_to_Reco_Id_Map;
	
	// Retrieved from RecoEvent store
	RecoVertex* theExtendedVertex=nullptr;
	ROOT::Math::XYZVector recoVtxOrigin;
	
	// Variables from MRDTracks BoostStore
	// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	std::vector<BoostStore>* theMrdTracks; // the actual tracks
	int MrdTrackID;
	int MCTruthParticleID;
	int MrdSubEventID;
	bool InterceptsTank;
	double StartTime;
	Position StartVertex;
	Position StopVertex;
	double TrackAngle;
	double TrackAngleError;
	double TrackAngleX;
	double TrackAngleY;
	double TrackLength;
	double EnergyLoss;
	double EnergyLossError;
	bool IsMrdPenetrating;
	bool IsMrdStopped;
	bool IsMrdSideExit;
	double PenetrationDepth;
	std::vector<int> LayersHit;
	int NumLayersHit;
	std::vector<int> PMTsHit;
	Position TankExitPoint;
	Position MrdEntryPoint;
	Position TrueTrackOrigin;
	Position RecoTrackOrigin;
	Position ClosestAppPoint;
	double ClosestAppDist;
	int LongTrack;
	
	double HtrackOrigin;
	double HtrackOriginError;
	double HtrackGradient;
	double HtrackGradientError;
	double VtrackOrigin;
	double VtrackOriginError;
	double VtrackGradient;
	double VtrackGradientError;
	double HtrackFitChi2;
	double HtrackFitCov;
	double VtrackFitChi2;
	double VtrackFitCov;
	
	std::vector<MCParticle>* MCParticles=nullptr;
	
	// scalers
	///////////////////
	int nummrdtracksthisevent=0;
	
	int numents=0;
	int totnumtracks=0;
	int numstopped=0;
	int numpenetrated=0;
	int numsideexit=0;
	int numtankintercepts=0;
	int numtankmisses=0;
	
	int totnumtrackstrue=0;
	int numpenetratedtrue=0;
	int numstoppedtrue=0;
	int numsideexittrue=0;
	int numtankinterceptstrue=0;
	int numtankmissestrue=0;
	
	// histograms
	///////////////////
	TH1F* hnumsubevs=nullptr;
	TH1F* hnumtracks=nullptr;
	TH1F* hrun=nullptr;
	TH1F* hevent=nullptr;
	TH1F* hmrdsubev=nullptr;
	TH1F* htrigger=nullptr;
	TH1F* hhangle=nullptr;
	TH1F* hhangleerr=nullptr;
	TH1F* hvangle=nullptr;
	TH1F* hvangleerr=nullptr;
	TH1F* htotangle=nullptr;
	TH1F* htotangleerr=nullptr;
	TH1F* henergyloss=nullptr;
	TH1F* henergylosserr=nullptr;
	TH1F* htracklength=nullptr;
	TH1F* htrackpen=nullptr;
	TH2F* htrackpenvseloss=nullptr;
	TH2F* htracklenvseloss=nullptr;
	TH3D* htrackstart=nullptr;
	TH3D* htrackstop=nullptr;
	TH3D* hpep=nullptr;
	TH3D* hmpep=nullptr;
	
	// truth copies
	TH1F* hnumtrackstrue=nullptr;
	TH1F* hnumsubevstrue=nullptr;
	TH1F* hhangletrue=nullptr;
	TH1F* hvangletrue=nullptr;
	TH1F* htotangletrue=nullptr;
	TH1F* henergylosstrue=nullptr;
	TH1F* htracklengthtrue=nullptr;
	TH1F* htrackpentrue=nullptr;
	TH2F* htrackpenvselosstrue=nullptr;
	TH2F* htracklenvselosstrue=nullptr;
	TH3D* htrackstarttrue=nullptr;
	TH3D* htrackstoptrue=nullptr;
	TH3D* hpeptrue=nullptr;
	TH3D* hmpeptrue=nullptr;
	
	// Output ROOT file
	std::string outfilename;
	TFile* outfile=nullptr;
	
	// function to enclose making it and setting branch addresses etc
	TFile* MakeRootFile();
	void ClearBranchVectors();
	TTree* recotree=nullptr;
	TTree* truthtree=nullptr;
	
	// vectors for track-wise information in ROOT file
	std::vector<int> fileout_MrdSubEventID;
	std::vector<double> fileout_HtrackAngle;
	std::vector<double> fileout_HtrackAngleError;
	std::vector<double> fileout_VtrackAngle;
	std::vector<double> fileout_VtrackAngleError;
	std::vector<double> fileout_TrackAngle;
	std::vector<double> fileout_TrackAngleError;
	std::vector<double> fileout_EnergyLoss;
	std::vector<double> fileout_EnergyLossError;
	std::vector<double> fileout_TrackLength;
	std::vector<double> fileout_PenetrationDepth;
	std::vector<int> fileout_NumLayersHit;
	std::vector<int> fileout_NumPmtsHit;
	std::vector<ROOT::Math::XYZVector> fileout_StartVertex;
	std::vector<ROOT::Math::XYZVector> fileout_StopVertex;
	std::vector<ROOT::Math::XYZVector> fileout_TankExitPoint;
	std::vector<ROOT::Math::XYZVector> fileout_MrdEntryPoint;
	std::vector<int> fileout_LongTrackReco;
	// truth variables
	std::vector<int> fileout_MCTruthParticleID;
	std::vector<double> fileout_TrueTrackAngleX;
	std::vector<double> fileout_TrueTrackAngleY;
	std::vector<double> fileout_TrueTrackAngle;
	std::vector<double> fileout_TrueEnergyLoss;
	std::vector<double> fileout_TrueTrackLength;
	std::vector<double> fileout_TruePenetrationDepth;
	std::vector<int> fileout_TrueNumLayersHit;
	std::vector<int> fileout_TrueNumPmtsHit;
	std::vector<ROOT::Math::XYZVector> fileout_TrueStopVertex;
	std::vector<ROOT::Math::XYZVector> fileout_TrueTankExitPoint;
	std::vector<ROOT::Math::XYZVector> fileout_TrueMrdEntryPoint;
	std::vector<uint64_t> fileout_TrueStartTime;
	std::vector<int> fileout_TrueInterceptsTank;
	std::vector<int> fileout_TrueIsMrdStopped;
	std::vector<int> fileout_TrueIsMrdPenetrating;
	std::vector<int> fileout_TrueIsMrdSideExit;
	//
	std::vector<int> fileout_RecoParticleID;
	std::vector<ROOT::Math::XYZVector> fileout_TrueOriginVertex;
	std::vector<ROOT::Math::XYZVector> fileout_RecoOriginVertex;
	std::vector<ROOT::Math::XYZVector> fileout_TrueClosestApproachPoint;
	std::vector<double> fileout_TrueClosestApproachDist;
	
	// addresses for ROOT file
	std::vector<int>* pfileout_MrdSubEventID;
	std::vector<double>* pfileout_HtrackAngle;
	std::vector<double>* pfileout_HtrackAngleError;
	std::vector<double>* pfileout_VtrackAngle;
	std::vector<double>* pfileout_VtrackAngleError;
	std::vector<double>* pfileout_TrackAngle;
	std::vector<double>* pfileout_TrackAngleError;
	std::vector<double>* pfileout_EnergyLoss;
	std::vector<double>* pfileout_EnergyLossError;
	std::vector<double>* pfileout_TrackLength;
	std::vector<double>* pfileout_PenetrationDepth;
	std::vector<int>* pfileout_NumLayersHit;
	std::vector<int>* pfileout_NumPmtsHit;
	std::vector<ROOT::Math::XYZVector>* pfileout_StartVertex;
	std::vector<ROOT::Math::XYZVector>* pfileout_StopVertex;
	std::vector<ROOT::Math::XYZVector>* pfileout_TankExitPoint;
	std::vector<ROOT::Math::XYZVector>* pfileout_MrdEntryPoint;
	std::vector<int>* pfileout_LongTrackReco;
	// truth variables
	std::vector<int>* pfileout_MCTruthParticleID;
	std::vector<double>* pfileout_TrueTrackAngleX;
	std::vector<double>* pfileout_TrueTrackAngleY;
	std::vector<double>* pfileout_TrueTrackAngle;
	std::vector<double>* pfileout_TrueEnergyLoss;
	std::vector<double>* pfileout_TrueTrackLength;
	std::vector<double>* pfileout_TruePenetrationDepth;
	std::vector<int>* pfileout_TrueNumLayersHit;
	std::vector<int>* pfileout_TrueNumPmtsHit;
	std::vector<ROOT::Math::XYZVector>* pfileout_TrueStopVertex;
	std::vector<ROOT::Math::XYZVector>* pfileout_TrueTankExitPoint;
	std::vector<ROOT::Math::XYZVector>* pfileout_TrueMrdEntryPoint;
	std::vector<uint64_t>* pfileout_TrueStartTime;
	std::vector<int>* pfileout_TrueInterceptsTank;
	std::vector<int>* pfileout_TrueIsMrdStopped;
	std::vector<int>* pfileout_TrueIsMrdPenetrating;
	std::vector<int>* pfileout_TrueIsMrdSideExit;
	//
	std::vector<int>* pfileout_RecoParticleID;
	std::vector<ROOT::Math::XYZVector>* pfileout_TrueOriginVertex;
	std::vector<ROOT::Math::XYZVector>* pfileout_RecoOriginVertex;
	std::vector<ROOT::Math::XYZVector>* pfileout_TrueClosestApproachPoint;
	std::vector<double>* pfileout_TrueClosestApproachDist;
	
	// TApplication for making histograms
	bool drawHistos;
	TApplication* rootTApp=nullptr;
	TCanvas* mrdDistCanv=nullptr;
	double canvwidth, canvheight;
	
	// verbosity levels: if 'verbosity' < this level, the message type will be logged.
	int v_error=0;
	int v_warning=1;
	int v_message=2;
	int v_debug=3;
	std::string logmessage;
	int get_ok;
	
};


#endif
