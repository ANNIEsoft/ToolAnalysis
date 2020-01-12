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

class TH1F;
class TH2F;
class TH3D;

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
	std::map<unsigned long,std::vector<Hit>>* TDCData=nullptr;
	int numsubevs;
	int numtracksinev;
	
	std::string MCFile;     //-> currentfilestring
	uint32_t RunNumber;     // -> runnum     (keep as the type is different: TODO align types)
	uint32_t SubrunNumber;  // -> subrunnum  ( " " )
	uint32_t EventNumber;   // -> eventnum   ( " " )
	uint16_t MCTriggernum;  // -> triggernum ( " " )
	uint64_t MCEventNum;    // not yet in MRDTrackClass
	
	// Retrieved from ANNIEvent
	//std::map<int,std::map<unsigned long,double>>* ParticleId_to_MrdTubeIds;
	//std::vector<double>* ParticleId_to_MrdCharge;
	std::map<int,int> Reco_to_True_Id_Map;
	
	// Variables from MRDTracks BoostStore
	// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	std::vector<BoostStore>* theMrdTracks; // the actual tracks
	int MrdTrackID;
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
