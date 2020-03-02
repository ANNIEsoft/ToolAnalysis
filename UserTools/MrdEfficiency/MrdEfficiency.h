/* vim:set noexpandtab tabstop=4 wrap */
#ifndef MrdEfficiency_H
#define MrdEfficiency_H

#include <string>
#include <iostream>

#include "Tool.h"
#include "TApplication.h"
#include "TCanvas.h"
#include "TH1F.h"
#include "TH3D.h"
#include <TStyle.h>
#include "TMath.h"
#include "TROOT.h"

class TGraphErrors;

class MrdEfficiency: public Tool {
	
	public:
	int verbosity=1;
	bool drawHistos;
	
	MrdEfficiency();
	bool Initialise(std::string configfile,DataModel &data);
	bool Execute();
	bool Finalise();
	
	private:
	std::map<int,std::map<unsigned long,double>>* ParticleId_to_MrdTubeIds; // the MC Truth information
	std::map<unsigned long,int> channelkey_to_mrdpmtid;           // 
	std::vector<BoostStore>* theMrdTracks;                        // the reconstructed tracks
	std::vector<MCParticle>* MCParticles=nullptr;                 // the true particles
	std::vector<int> PMTsHit;
	int MrdTrackID;
	int numsubevs;
	int numtracksinev;
	uint32_t EventNumber;
	
	// maps between true and reco particles
	std::map<int,int> Reco_to_True_Id_Map;
	std::map<int,int> True_to_Reco_Id_Map;
	
	// TApplication for making histograms
	TApplication* rootTApp=nullptr;
	TCanvas* mrdEffCanv=nullptr;
	double canvwidth, canvheight;
	std::string plotDirectory;
	TFile* fileout=nullptr; // save histograms/TGraphs to file
	
	// histograms
	///////////////////
	TH1F* hnumcorrectlymatched = nullptr;
	TH1F* hnumtruenotmatched = nullptr;
	TH1F* hnumreconotmatched = nullptr;
	
	// properties of primary muons whose tracks were successfully reconstructed
	TH1F* hhangle_recod = nullptr;
	TH1F* hvangle_recod = nullptr;
	TH1F* htotangle_recod = nullptr;
	TH1F* henergyloss_recod = nullptr;
	TH1F* htracklength_recod = nullptr;
	TH1F* htrackpen_recod = nullptr;
	TH1F* hnummrdpmts_recod = nullptr;
	TH1F* hq2_recod = nullptr;
	TH3D* htrackstart_recod = nullptr;
	TH3D* htrackstop_recod = nullptr;
	TH3D* hpep_recod = nullptr;
	TH3D* hmpep_recod = nullptr;
	
	// properties of primary muons whose tracks were not successfully reconstructed
	TH1F* hhangle_nrecod = nullptr;
	TH1F* hvangle_nrecod = nullptr;
	TH1F* htotangle_nrecod = nullptr;
	TH1F* henergyloss_nrecod = nullptr;
	TH1F* htracklength_nrecod = nullptr;
	TH1F* htrackpen_nrecod = nullptr;
	TH1F* hnummrdpmts_nrecod = nullptr;
	TH1F* hq2_nrecod = nullptr;
	TH3D* htrackstart_nrecod = nullptr;
	TH3D* htrackstop_nrecod = nullptr;
	TH3D* hpep_nrecod = nullptr;
	TH3D* hmpep_nrecod = nullptr;
	
	// more useful: efficiency vs various metrics
	TGraphErrors* heff_vs_mrdlength = nullptr;
	TGraphErrors* heff_vs_penetration = nullptr;
	TGraphErrors* heff_vs_q2 = nullptr;
	TGraphErrors* heff_vs_angle = nullptr;
	TGraphErrors* heff_vs_npmts = nullptr;
	
	int num_primary_muons_that_hit_MRD = 0;
	int num_primary_muons_that_missed_MRD = 0;
	int num_primary_muons_reconstructed = 0;
	int num_primary_muons_not_reconstructed = 0;
	
	// small function to calculate the error on an efficiency bin
	double Efficiency_Error(double recod_events, double nrecod_events);
	
	// verbosity level
	// if 'verbosity' < this level, the message type will be logged.
	int v_error=0;
	int v_warning=1;
	int v_message=2;
	int v_debug=3;
	std::string logmessage;
	int get_ok;
	
};


#endif
