/* vim:set noexpandtab tabstop=2 wrap */
#include "TotalLightMap.h"

// ROOT
//#include "TPointSet3D.h"
#include "TApplication.h"
#include "TSystem.h"
#include "TCanvas.h"
#include "TPolyMarker.h"
#include "TEllipse.h"
#include "TBox.h"
#include "TColor.h"
#include "TH2F.h"
#include "TMath.h"
#include "TPaveLabel.h"
#include "TMarker.h"

// std
#include <sys/types.h> // for stat() test to see if file or folder
#include <sys/stat.h>
#include <thread>      // for sleep_for
#include <chrono>      // for chrono::milliseconds
//#include <unistd.h>

/*
make scatter plot (TPolyMarker2D) of the projected tank exit of pion/gammas/muons based on their initial momentum
Then make plots of hits on walls from each species:
 o  scatter plot of unbinned true photon positions << Is this useful?
     -> we may be able to approximate this by using old sims with many LAPPDs
 o  binned true position of hits, then take bin differences and compare distributions
     -> we can probably do this more easily/meaningfully by using PMTs as bins
 o  PMT hit maps... could similarly take differences in distributions
*/

///////////////////////////////////////////////////////////////////////
// Much Code stolen / adapted from Michael Nieslony's EventDisplay tool
///////////////////////////////////////////////////////////////////////

TotalLightMap::TotalLightMap():Tool(){}

// ##############################################################
// ####################### Initialise ###########################
// ##############################################################

bool TotalLightMap::Initialise(std::string configfile, DataModel &data){
	
	/////////////////// Useful header ///////////////////////
	
	if(verbosity) cout<<"Initializing Tool TotalLightMap"<<endl;
	
	if(configfile!="") m_variables.Initialise(configfile); //loading config file
	//m_variables.Print();
	
	m_data= &data; //assigning transient data pointer
	/////////////////////////////////////////////////////////////////
	
	// Get the Tool configuration variables
	// ====================================
	m_variables.Get("verbosity",verbosity);
	m_variables.Get("plotDirectory",plotDirectory);
	m_variables.Get("drawHistos",drawHistos);
	m_variables.Get("ColourScaleParameter",mode);
	if(mode=="") mode = "Parent";
	
	// Make the output directory
	// =========================
	bool isdir=false, plotDirectoryExists=false;
	struct stat s;
	if(stat(plotDirectory.c_str(),&s)==0){
		plotDirectoryExists=true;
		if(s.st_mode & S_IFDIR){        // mask to extract if it's a directory
			isdir=true;                   //it's a directory
		} else if(s.st_mode & S_IFREG){ // mask to check if it's a file
			isdir=false;                  //it's a file
		} else {
			isdir=false;                  // neither file nor folder??
		}
	} else {
		plotDirectoryExists=false;
		//assert(false&&"stat failed on input path! Is it valid?"); // error
		// errors could also be because this is a file pattern: e.g. wcsim_0.4*.root
		isdir=false;
	}
	
	if(/*drawHistos &&*/ (!plotDirectoryExists || !isdir)){   // we need to either draw or save
		Log("TotalLightMap Tool: output directory "+plotDirectory+" does not exist or is not a writable directory; please check and re-run.",v_error,verbosity);
		return false;
	}
	
	// Make the TApplication
	// ======================
	// If we wish to show the histograms during running, we need a TApplication
	// There may only be one TApplication, so if another tool has already made one
	// register ourself as a user. Otherwise, make one and put a pointer in the CStore for other Tools
	if(drawHistos){
		// create the ROOT application to show histograms
		int myargc=0;
		//char *myargv[] = {(const char*)"mrdeff"};
		// get or make the TApplication
		intptr_t tapp_ptr=0;
		get_ok = m_data->CStore.Get("RootTApplication",tapp_ptr);
		if(not get_ok){
			Log("TotalLightMap Tool: Making global TApplication",v_error,verbosity);
			rootTApp = new TApplication("rootTApp",&myargc,0);
			tapp_ptr = reinterpret_cast<intptr_t>(rootTApp);
			m_data->CStore.Set("RootTApplication",tapp_ptr);
		} else {
			Log("TotalLightMap Tool: Retrieving global TApplication",v_error,verbosity);
			rootTApp = reinterpret_cast<TApplication*>(tapp_ptr);
		}
		int tapplicationusers;
		get_ok = m_data->CStore.Get("RootTApplicationUsers",tapplicationusers);
		if(not get_ok) tapplicationusers=1;
		else tapplicationusers++;
		m_data->CStore.Set("RootTApplicationUsers",tapplicationusers);
	}
	
	// Get the anniegeom
	// =================
	get_ok = m_data->Stores.at("ANNIEEvent")->Header->Get("AnnieGeometry",anniegeom);
	if(not get_ok){
		Log("TotalLightMap: Could not get the AnnieGeometry from ANNIEEvent!",v_error,verbosity);
		return false;
	}
	
	// Get properties of the detector we need
	tank_radius = anniegeom->GetTankRadius();
	tank_height = anniegeom->GetTankHalfheight();
	// in fact at time of writing these are incorrect, and we'll need to override them manually:
	//tank_radius = 1.277;     // [m]
	//tank_height = 3.134;  // [m]   actually the tank *half* height
	tank_radius = 1.5*MRDSpecs::tank_radius/100.;
	tank_height = 2.*MRDSpecs::tank_halfheight/100.;
	
	// Get the Detectors
	// =================
	Log("TotalLightMap Tool: Getting tank Detectors",v_debug,verbosity);
	// first the PMTs
	// FIXME better way of finding a collection
	int detectorcollection=0;
	do {
		if(detectorcollection>=anniegeom->GetDetectors()->size()) break;
		TankDetectors = anniegeom->GetDetectors()->at(detectorcollection);
		if(TankDetectors->size()>0){
			if(TankDetectors->begin()->second.GetDetectorElement()=="Tank"){ break; }
		}
		detectorcollection++;
	} while (detectorcollection<anniegeom->GetDetectors()->size());
	Log("TotalLightMap Tool: We have "+to_string(TankDetectors->size())+" PMTs",v_debug,verbosity);
	// then the LAPPDs
	detectorcollection=0;
	do {
		if(detectorcollection>=anniegeom->GetDetectors()->size()) break;
		LAPPDDetectors = anniegeom->GetDetectors()->at(detectorcollection);
		if(LAPPDDetectors->size()>0){
			if(LAPPDDetectors->begin()->second.GetDetectorElement()=="LAPPD"){ break; }
		}
		detectorcollection++;
	} while (detectorcollection<anniegeom->GetDetectors()->size());
	Log("TotalLightMap Tool: We have "+to_string(LAPPDDetectors->size())+" LAPPDs",v_debug,verbosity);
	
	// Make the event gui
	// ==================
	Log("TotalLightMap Tool: Making GUI",v_debug,verbosity);
	make_gui();
	
	// Other canvases
	// ==============
	// Canvases for unbinned light distributions
	Log("TotalLightMap Tool: Making canvases for polymarker plots",v_debug,verbosity);
	// + split by parent type: blue markers = muon hits, red markers = Pion daughter hits
	lightmap_by_parent_canvas = new TCanvas("lightmap_by_parent_canvas","",600,400);
	
	// unbinned hit plots
	// ====================
	Log("TotalLightMap Tool: Making polymarkers",v_debug,verbosity);
	// pmt hit patterns in CCQE events
	lightmapccqe = new TPolyMarker();
	// pmt hit patterns in CCNPI events
	lightmapcc1p = new TPolyMarker();
	
	// binned plots
	// ============
	// spherical polar which we will plot with mercator projection
	// this allows us to rotate the light map arbitrarily to place the muon exit point at the center
	Log("TotalLightMap Tool: Making histograms",v_debug,verbosity);
	lmccqe = new TH2F("lmccqe","LightMap CCQE",  25, -180, 180, 25, -80.5, 80.5); // can we do finer binning?
	lmcc1p = new TH2F("lmcc1p","LightMap CCNPi", 25, -180, 180, 25, -80.5, 80.5);
	lmdiff = new TH2F("lmdiff","LightMap CCQE-CCNPi",  25, -180, 180, 25, -80.5, 80.5);
	lmmuon = new TH2F("lmmuon","LightMap Primary Muon",  25, -180, 180, 25, -80.5, 80.5); // can we do finer binning?
	lmpigammas = new TH2F("lmpigammas","LightMap Pion Products", 25, -180, 180, 25, -80.5, 80.5);
	lmdiff2 = new TH2F("lmdiff2","LightMap PrimaryMuon - PionProducts",  25, -180, 180, 25, -80.5, 80.5);
	
	// debug plots
	vertexphihist = new TH1D("vertexphihist","Histogram of Phi of primary vertices",100,-2.*M_PI,2.*M_PI);
	vertexthetahist = new TH1D("vertexthetahist","Histogram of Theta of primary vertices",100,-2.*M_PI, 2.*M_PI);
	vertexyhist = new TH1D("vertexyhist","Histogram of Y of primary vertices",100,-2.*tank_height,2.*tank_height);
	
	Log("TotalLightMap Tool: Finished Intialize",v_debug,verbosity);
	return true;
}

// ##############################################################
// ######################### Execute ############################
// ##############################################################

bool TotalLightMap::Execute(){
	Log("TotalLightMap Tool: Executing",v_debug,verbosity);
	
	// Get the event hits
	// ==================
	get_ok = m_data->Stores.at("ANNIEEvent")->Get("MCHits",MCHits);
	if(not get_ok){
		Log("TotalLightMap Tool: Error retrieving MCHits,true from ANNIEEvent!",v_error,verbosity);
		return false;
	}
	get_ok = m_data->Stores.at("ANNIEEvent")->Get("MCLAPPDHits",MCLAPPDHits);
	if(not get_ok){
		Log("TotalLightMap Tool: Error retrieving MCLAPPDHits,true from ANNIEEvent!",v_error,verbosity);
		return false;
	}
	
	// find any particles of interest in the MCParticles
	// =================================================
	get_ok = m_data->Stores.at("ANNIEEvent")->Get("MCParticles",MCParticles);
	if(not get_ok){
		Log("TotalLightMap Tool: Error retrieving MCParticles,true from ANNIEEvent!",v_error,verbosity);
		return false;
	}
	
	// we'll pull out a few particles in particular
	MCParticle primarymuon;   // primary muon
	primarymuon.SetParticleID(-1);
	// we'll compare the IDs later when looking at hits, so set the IDs to out of range dummy values
	// in case the particles don't exist in the event
	std::vector<MCParticle> particlesofinterest;
	
	// Find these particles
	Log("TotalLightMap Tool: Searching for MCParticles",v_debug,verbosity);
	bool mufound=false;
	if(MCParticles){
		Log("TotalLightMap Tool: Num MCParticles = "+to_string(MCParticles->size()),v_message,verbosity);
		for(int particlei=0; particlei<MCParticles->size(); particlei++){
			MCParticle aparticle = MCParticles->at(particlei);
			aparticle.SetParticleID(particlei);  // override WCSim 'TrackID' with index in MCParticles XXX
			// Selection criteria pick out particle(s) of interest
			if((aparticle.GetParentPdg()==0) && (aparticle.GetPdgCode()==13)){
				primarymuon = aparticle;
				mufound=true;
			} else if( (abs(aparticle.GetPdgCode())==211) || (aparticle.GetPdgCode()==111) ||   // a Pion
							  (((aparticle.GetPdgCode()==22)||(abs(aparticle.GetPdgCode())==13)) &&     // gamma or muon
			           ((aparticle.GetParentPdg()==111)||(abs(aparticle.GetParentPdg())==211))) // pion daughter
							 ){   // i.e. if it's either a charged pion, or a pion daughter of interest
							      // actually the pion parentage is passed down, so light is reported as
							      // from pions (event pi0's) not their daughters*
				// pi0 (111), pi+ (211), pi- (-211)
				particlesofinterest.push_back(aparticle);
			}
		}
	} else {
		Log("TotalLightMap Tool: No MCParticles in the event!",v_warning,verbosity);
	}
	
	// skip the event if not CC
	// ========================
	if(not mufound){
		Log("TotalLightMap Tool: No primary muon in this event",v_debug,verbosity);
		return true;
	}
	
	// note the interesting particle indices for comparing against MCHit parents
	// =========================================================================
	// particleidsofinterest[0] should always be the primary muon
	particleidsofinterest.clear();
	particleidsofinterest.push_back(primarymuon.GetParticleID());    // primary muon id must be first entry
	particlesofinterest.push_back(primarymuon);
	for(MCParticle& aparticle : particlesofinterest){
		if((aparticle.GetPdgCode()==111)||(abs(aparticle.GetPdgCode())==211)){
			particleidsofinterest.push_back(aparticle.GetParticleID());  // *for ids, only note pions, not daughters
		}
	}
	std::cout<<"Particles of interest are: {";
	for(auto&& ap : particleidsofinterest){ std::cout<<ap<<", "; }
	std::cout<<"}"<<std::endl;
	// to plot light from all particles, don't specify any particles of interest XXX
	
	// get the interaction type
	// ========================
	// if we had any pion daughters, we'll assume this is CCNPi for now XXX
	interaction_type = (particlesofinterest.size()>1) ? "CC1PI" : "CCQE";
	Log("TotalLightMap Tool: Calling this a "+interaction_type+" event",v_debug,verbosity);
	
	// ========================================================
	// End of Event Initialization: Move to processing the hits
	// ========================================================
	
	// Make markers for PMT hits
	// =========================
	Log("TotalLightMap Tool: Executing make_pmt_markers",v_debug,verbosity);
	make_pmt_markers(primarymuon);    // this also calls Fill() on the cumulative histograms
	
	// Make markers from LAPPD hits
	// ============================
	Log("TotalLightMap Tool: Executing make_lappd_markers",v_debug,verbosity);
	make_lappd_markers();  // this also calls Fill() on the cumulative histograms
	
	// Make markers from the particles
	// ===============================
	Log("TotalLightMap Tool: Making polymarkers for primary particles",v_debug,verbosity);
	// markers at the projected tank exit of true particles of interest
	// for the traditional event display and Wiener Triple cumulative display
	//Log("TotalLightMap Tool: Making polymarker for primary muon vertex",v_debug,verbosity);
	for(MCParticle& aparticle : particlesofinterest){
		make_vertex_markers(aparticle);
	}
	
	// ========================================================
	// End of Event Processing: Move to drawing the displays
	// ========================================================
	
	// Draw the markers
	// ================
	Log("TotalLightMap Tool: Drawing Polymarkers",v_debug,verbosity);
	DrawMarkers();
	
	// save histos or wait for viewer
	// XXX DEBUG ONLY XXX   V
//	while(gROOT->FindObject("canvas_ev_display")!=nullptr){
		Log("TotalLightMap Tool: Waiting for user to evaluate event display",v_debug,verbosity);
		gSystem->ProcessEvents();
		//std::this_thread::sleep_for(std::chrono::milliseconds(100));
		gPad->WaitPrimitive();
//	}
//	canvas_ev_display = new TCanvas("canvas_ev_display","Event Display",900,900);
//	top_circle->Draw();
//	bottom_circle->Draw();
//	box->Draw();
//	// XXX DEBUG ONLY XXX   ^
	
	// ========================================================
	// End of Event Drawing: Move to cleanup
	// ========================================================
	
	// cleanup: we make markers and their colours on an event-by-event basis
	Log("TotalLightMap Tool: Cleaning up polymarkers for the event view",v_debug,verbosity);
	for(TPolyMarker* amarker : marker_pmts_top)   { delete amarker; } marker_pmts_top.clear();
	for(TPolyMarker* amarker : marker_pmts_bottom){ delete amarker; } marker_pmts_bottom.clear();
	for(TPolyMarker* amarker : marker_pmts_wall)  { delete amarker; } marker_pmts_wall.clear();
	for(TPolyMarker* amarker : marker_lappds)     { delete amarker; } marker_lappds.clear();
	// clean up the projected exit point of the true particles
	// these aren't the same markers as the cumulative plots; their coordinate systems are different
	for(TPolyMarker* amarker : marker_vtxs)       { delete amarker; } marker_vtxs.clear();
	// clean up the legend colour dots
//	for(TMarker* amarker : legend_dots)           { delete amarker; } legend_dots.clear(); TODO
	
	// delete and re-create (clear) the event-wise Wiener TPolyMarker plots
	
	// clear the vectors for event-wise wiener tripel plots, but don't delete the markers
	// or TColors for any markers we want to keep for cumulative plots
	/* for(TPolyMarker* amarker : chargemapcc1p)     { delete amarker; } */ chargemapcc1p.clear();
	
	// reset: colours are scaled to the range of charges/times seen in the event - reset for next event
	Log("TotalLightMap Tool: Resetting event times and charges scale range",v_debug,verbosity);
	event_earliest_hit_time = 9999999;
	event_latest_hit_time = 0;
	maximum_charge_on_a_pmt = 0;
	
	return true;
}

// ##############################################################
// ######################### Finalise ###########################
// ##############################################################

bool TotalLightMap::Finalise(){
	
	Log("TotalLightMap Tool: Making Muon vs Pion Decay Light Distrbution Difference Histogram",v_debug,verbosity);
	// make the binned histogram showing the difference in distributions of light
	// from the muon and light from the pions
	for(int xbini=0; xbini<lmmuon->GetNbinsX(); xbini++){
		for(int ybini=0; ybini<lmmuon->GetNbinsY(); ybini++){
			double contentsdiff = lmmuon->GetBinContent(xbini,ybini)-lmpigammas->GetBinContent(xbini,ybini);
			lmdiff2->SetBinContent(xbini,ybini, contentsdiff);
		}
	}
	
	Log("TotalLightMap Tool: Making CCQE vs CCNPi Event Light Distrbution Difference Histogram",v_debug,verbosity);
	// make the binned histogram showing the difference in distribution of light for
	// CCQE events and CC1pi events
	for(int xbini=0; xbini<lmccqe->GetNbinsX(); xbini++){
		for(int ybini=0; ybini<lmccqe->GetNbinsY(); ybini++){
			double contentsdiff = lmccqe->GetBinContent(xbini,ybini)-lmcc1p->GetBinContent(xbini,ybini);
			lmdiff->SetBinContent(xbini,ybini, contentsdiff);
		}
	}
	
	// Draw and Save the 2D histos
	Log("TotalLightMap Tool: Making Mercator Projection Canvas",v_debug,verbosity);
	TCanvas* mercatorCanv = new TCanvas("mercatorCanv","Mercator Canvas", 900,700);
	gStyle->SetOptTitle(1);
	mercatorCanv->cd();
	
	Log("TotalLightMap Tool: Drawing Accumulated CCQE Histogram",v_debug,verbosity);
	lmccqe->Draw("mercator");
	mercatorCanv->SaveAs(TString::Format("%s/%s.png",plotDirectory.c_str(),lmccqe->GetTitle()).Data());
	mercatorCanv->Clear();
	
	Log("TotalLightMap Tool: Drawing Accumulated CCNPi Histogram",v_debug,verbosity);
	lmcc1p->Draw("mercator");
	mercatorCanv->SaveAs(TString::Format("%s/%s.png",plotDirectory.c_str(),lmcc1p->GetTitle()).Data());
	mercatorCanv->Clear();
	
	Log("TotalLightMap Tool: Drawing CCQE - CCNPi Light Difference Histogram",v_debug,verbosity);
	lmdiff->Draw("mercator");
	mercatorCanv->SaveAs(TString::Format("%s/%s.png",plotDirectory.c_str(),lmdiff->GetTitle()).Data());
	mercatorCanv->Clear();
	
	Log("TotalLightMap Tool: Drawing Accumulated Muon Light Map Histogram",v_debug,verbosity);
	lmmuon->Draw("mercator");
	mercatorCanv->SaveAs(TString::Format("%s/%s.png",plotDirectory.c_str(),lmmuon->GetTitle()).Data());
	mercatorCanv->Clear();
	
	Log("TotalLightMap Tool: Drawing Accumulated Daughter Light Map Histogram",v_debug,verbosity);
	lmpigammas->Draw("mercator");
	mercatorCanv->SaveAs(TString::Format("%s/%s.png",plotDirectory.c_str(),lmpigammas->GetTitle()).Data());
	mercatorCanv->Clear();
	
	Log("TotalLightMap Tool: Drawing Muon - Daughter Light Map Histogram",v_debug,verbosity);
	lmdiff2->Draw("mercator");
	mercatorCanv->SaveAs(TString::Format("%s/%s.png",plotDirectory.c_str(),lmdiff2->GetTitle()).Data());
	mercatorCanv->Clear();
	delete mercatorCanv; mercatorCanv=nullptr;
	
	// Draw the cumulative tpolymarker plots
	Log("TotalLightMap Tool: Drawing Cumulative PolyMarker Plots",v_debug,verbosity);
	DrawCumulativeMarkers();
	// TODO: LEGENDS. LEGENDS EVERYWHERE.
	
	// Free memory
	FinalCleanup();
	
	TCanvas* phicanv = new TCanvas();
	vertexphihist->Draw();
	TCanvas* thetacanv = new TCanvas();
	vertexthetahist->Draw();
	TCanvas* ycanv = new TCanvas();
	vertexyhist->Draw();
	
	// FIXME wait because ToolAnalysis is currently segfaulting after Finalise
	TCanvas* c1 = new TCanvas();
	c1->WaitPrimitive();
	
	return true;
}

// ##############################################################
// ##################### Process PMT Hits #######################
// ##############################################################

void TotalLightMap::make_pmt_markers(MCParticle primarymuon){
	
	/* Loops over all tank PMTs and draws a marker for each, with colour based on either time or charge. */
	
	std::vector<TPolyMarker*>* theset=nullptr;
	short color_marker=0; // should be careful we don't overflow this...
	
	// loop over PMTs with a hit
	// =========================
	Log("TotalLightMap Tool: Looping over PMTs with a hit",v_debug,verbosity);
	for(std::pair<const unsigned long,std::vector<MCHit>>& nextpmt : *MCHits ){
		// if it's not a tank PMT, ignore it
		Detector* thepmt = anniegeom->ChannelToDetector(nextpmt.first);
		if(thepmt->GetDetectorElement()!="Tank") continue;
		if(thepmt->GetTankLocation()=="OD") continue;  // don't plot OD pmts
		
		// calculate the total charge on this PMT, and find the time of the earliest photon on it
		// these will be used for colouring the marker
		double total_pmt_charge = 0;
		double first_pmt_light = 999999;
		// alternatively, to colour light by parent, we'll set th RGB components based on
		// fractional charge from each parent of interest. Limits us to 3 parents of interst at most.
		std::vector<float> chargesfromparents(3,0);
		
		// loop over hits on this PMT
		// ==========================
		// ... and calculate the colour metrics
		//Log("TotalLightMap Tool: Looping over hits on tank PMT "+to_string(nextpmt.first),v_debug,verbosity);
		for(MCHit& nexthit : nextpmt.second){
			
			// if we're only plotting the light from specific particles, check the hit parents
			// (if we've not specified any particles of interest, include all hits)
			if(particleidsofinterest.size()){
				const std::vector<int>* parentindices = nexthit.GetParents();
				bool hitofinterest=false;
				for(const int& aparent : *parentindices){
					std::vector<int>::iterator parentofinterest = 
						std::find(particleidsofinterest.begin(), particleidsofinterest.end(), aparent);
					if(parentofinterest!=particleidsofinterest.end()){
						int parentofinterestnum = std::distance(particleidsofinterest.begin(), parentofinterest);
						chargesfromparents.at(parentofinterestnum)+= nexthit.GetCharge();
						hitofinterest=true;
					}
//					if(parentofinterest==particleidsofinterest.begin()){
//					if(aparent==particleidsofinterest.at(0)){
//						chargesfromparents.at(0)+= nexthit.GetCharge();  // this hit was from primary muon
//					} else {
//						chargesfromparents.at(1)+= nexthit.GetCharge();  // all other light
//					}
//					hitofinterest=true;
				}
				if(not hitofinterest){ continue; } // ignore hits on this PMT not from a particle of interest
			}
			
			total_pmt_charge += nexthit.GetCharge();
			double hit_time = nexthit.GetTime();
			if(hit_time<first_pmt_light) first_pmt_light = hit_time;
			
			// keep track of the total event duration for colour scale range
			if(hit_time<event_earliest_hit_time) event_earliest_hit_time = hit_time;
			if(hit_time>event_latest_hit_time) event_latest_hit_time = hit_time;
		}
		
		// keep track of the maximum charge on a PMT for colour scale range
		if(maximum_charge_on_a_pmt<total_pmt_charge){ maximum_charge_on_a_pmt = total_pmt_charge; }
		
		// Skip if uninteresting
		// =====================
		// if none of the hits on the PMT were from particles of interest, do not make any marker
		if(total_pmt_charge==0){ continue; }
		//Log("TotalLightMap Tool: This PMT had hits from particles of interest",v_debug,verbosity);
		
		// Calculate the marker position on the canvas
		// ===========================================
		// First get PMT position relative the tank centre
		Position PMT_position = thepmt->GetPositionInTank();
		
		// calculate marker position on the appropriate standard event display pad
		//Log("TotalLightMap Tool: Calculating marker position",v_debug,verbosity);
		double markerx=0.,markery=0.;
		if (thepmt->GetTankLocation()=="TopCap"){
			// top cap
			markerx=0.5-size_top_drawing*PMT_position.X()/tank_radius;
			markery=0.5+(tank_height/tank_radius+1)*size_top_drawing-size_top_drawing*PMT_position.Z()/tank_radius;
			theset = &marker_pmts_top;
		} else if (thepmt->GetTankLocation()=="BottomCap"){
			// bottom cap
			markerx=0.5-size_top_drawing*PMT_position.X()/tank_radius;
			markery=0.5-(tank_height/tank_radius+1)*size_top_drawing+size_top_drawing*PMT_position.Z()/tank_radius;
			theset = &marker_pmts_bottom;
		} else if (thepmt->GetTankLocation()=="Barrel"){
			// wall
			double phi = -PMT_position.GetPhi();  // not sure why it needs the -1 for the display....
			markerx=0.5+phi*size_top_drawing;
			markery=0.5+2.*PMT_position.Y()/tank_height*tank_height/tank_radius*size_top_drawing;
			theset = &marker_pmts_wall;
		} else {
			Log("TotalLightMap Tool: Unrecognised cylinder location of tank PMT: "
					+(thepmt->GetTankLocation()), v_warning, verbosity);
		}
		//logmessage = "TotalLightMap Tool: The PMT marker position is ("+to_string(markerx)
		//						+", "+to_string(markery)+")";
		//Log(logmessage,v_debug,verbosity);
		
		// Calculate the Polymarker colour
		// ===============================
		// set the colour based on the time / charge of the hit
		// we want to scale these to the full range of the event,
		// but we don't know that range until we've scanned the whole event.
		// So for now, set the colour to the absolute charge/time
		// and then we'll then scale it before drawing
		//Log("TotalLightMap Tool: Calculating PMT marker colour using mode "+mode,v_debug,verbosity);
		if (mode == "Charge"){
			color_marker = int(total_pmt_charge);
		} else if( mode == "Time"){
			color_marker = int(first_pmt_light);
		} else if( mode == "Parent"){
			// determine the colour based on the parent
			// If a PMT saw light from multiple parent particles,
			// Use the relative charges to set the RGB components
			// Since this means creating (and keeping, for cumulative plots)
			// a TColor on the heap, only do this if it's necessary
			if((int(chargesfromparents.at(0)>0)+int(chargesfromparents.at(1)>0)+int(chargesfromparents.at(2)>0))>1){
				// this PMT saw light from multiple parents
				// ok, so to make a new TColor we need a free unique index for it
				// (that index (a 'Color_t') is actually the number we pass to 'SetMarkerColor')
				color_marker = startingcolournum + eventcolours.size();
				TColor* thetcolor = 
					new TColor(color_marker,chargesfromparents.at(0),chargesfromparents.at(1),chargesfromparents.at(2));
				// fortunately as we're specifying RGB components no scaling will be needed
				// but keep the TColor pointers so we can free them when we're done
				eventcolours.push_back(thetcolor);
			} else {
				if(chargesfromparents.at(0)>0){
					color_marker = 2; // Red
				} else if(chargesfromparents.at(1)>0){
					color_marker = 3; // Green
				} else {
					color_marker = 4; // Blue
				}
			}
		}
		//Log("TotalLightMap Tool: Marker colour is: "+to_string(color_marker),v_debug,verbosity);
		
		// Make the Polymarker
		TPolyMarker* marker = new TPolyMarker(1,&markerx,&markery,"");
		
		// Set the marker properties
		marker->SetMarkerColor(color_marker);
		marker->SetMarkerStyle(8);  // or 20?
		if((mode=="Charge")||(mode=="Time")){
			marker->SetMarkerSize(1);
		} else {
			marker->SetMarkerSize(int(total_pmt_charge));
		}
		// Add to the relevant set
		theset->push_back(marker);
		//logmessage = "TotalLightMap Tool: Adding the marker; we now have "+to_string(theset->size())+" markers";
		//Log(logmessage,v_debug,verbosity);
		
		// =================================
		// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
		// =================================
/*
		
		// Calculate the Winkel Triple Coordinates
		// =======================================
		// this plot will show the whole 360° view of the tank, with the views rotated to place
		// the primary muon trajectory along (0,0), to align all events
		double phi = PMT_position.GetPhi() - primarymuon.GetStartDirection().GetPhi();
		double theta = PMT_position.GetTheta() - primarymuon.GetStartDirection().GetTheta();
		logmessage="TotalLightMap Tool: Polar position is (Phi="+to_string(phi)+", Theta="+to_string(theta)+")";
		Log(logmessage,v_debug,verbosity);
		
		double x_winkel, y_winkel;
		DoTheWinkelTripel(theta, phi, x_winkel, y_winkel);
		logmessage="TotalLightMap Tool: Wiener position is ("+to_string(x_winkel)+", "+to_string(y_winkel)+")";
		Log(logmessage,v_debug,verbosity);
		
		// Add to the cumulative polymarker plots
		// ======================================
		// first cumulative hit maps for each event type
		if(interaction_type=="CCQE"){
			Log("TotalLightMap Tool: Adding Winkel marker to CCQE plot",v_debug,verbosity);
			lightmapccqe->SetNextPoint(x_winkel, y_winkel); //, total_pmt_charge);
			std::cout<<"We now have "<<lightmapcc1p->GetN()<<" points in the CCQE plot"<<std::endl;
		}
		if(interaction_type=="CC1PI"){
			Log("TotalLightMap Tool: Adding Winkel marker to CC1Pi plot",v_debug,verbosity);
			lightmapcc1p->SetNextPoint(x_winkel, y_winkel); //, total_pmt_charge);
			std::cout<<"We now have "<<lightmapcc1p->GetN()<<" points in the CCNPi plot"<<std::endl;
		}
		
		// Then, for CCNPi events, we use a collection of TPolyMarkers, because the colour
		// is based on the ratio of charge from parent particle
		if(interaction_type=="CC1PI"){
			// Make the Polymarker
			TPolyMarker* markerwinkel = new TPolyMarker(1,&x_winkel,&y_winkel,"");
			
			// set the marker properties
			markerwinkel->SetMarkerColor(color_marker);
			markerwinkel->SetMarkerStyle(8);
			markerwinkel->SetMarkerSize(1);
		
			// Add to the appropriate PolyMarker(s), both event-wise and cumulative
			Log("TotalLightMap Tool: Adding marker cc1pi charge map",v_debug,verbosity);
			chargemapcc1p.push_back(markerwinkel);
			chargemapcc1p_cum.push_back(markerwinkel);
			std::cout<<"We now have "<<chargemapcc1p.size()<<" CC1Pi PolyMarkers in this event and "
							 <<chargemapcc1p_cum.size()<<" accumlated CC1Pi polymarkers over all events"<<std::endl;
		}
*/
		
/*
		// Also fill the standard Mercator 2D histograms
		Log("TotalLightMap Tool: Filling 2D histos",v_debug,verbosity);
		// first tank charge from the muon (weight the PMT fill by it's charge from the muon)
		if(chargesfromparents.at(0)>0) lmmuon->Fill(x_winkel, y_winkel, chargesfromparents.at(0));
		// same for light from the pion daughters
		if((chargesfromparents.at(1)+chargesfromparents.at(2))>0){
			lmpigammas->Fill(x_winkel, y_winkel, chargesfromparents.at(1)+chargesfromparents.at(2));
		}
		
		// lastly fill total light hisotgrams for CCQE and CC1PI events
		if(interaction_type=="CCQE"){
			lmccqe->Fill(x_winkel, y_winkel, chargesfromparents.at(0));
		}
		if(interaction_type=="CC1PI"){
			lmcc1p->Fill(x_winkel, y_winkel, chargesfromparents.at(1)+chargesfromparents.at(2));
		}
		
*/
		
	} // end loop over PMTs
	
}

// ##############################################################
// ##################### Process LAPPD Hits #####################
// ##############################################################

	void TotalLightMap::make_lappd_markers(){
	
	/* Loops over all LAPPD Hits and draws a marker for each, with colour based on either time or charge. */
	/* In contrast to PMTs, we make one marker per hit, not per LAPPD, since each has a unique position   */
	
	std::cout <<"Drawing LAPPD hits..."<<std::endl;
	
	// Loop over LAPPDs with a hit
	for(std::pair<const unsigned long, std::vector<MCLAPPDHit>>& nextlappd : *MCLAPPDHits){
		// Loop over hits on this LAPPD
		for(MCLAPPDHit& nexthit : nextlappd.second){
			
			// if we're only plotting the light from specific particles, check the hit parents
			// (if we've not specified any particles of interest, include all hits)
			std::vector<float> chargesfromparents(3,0);
			if(particleidsofinterest.size()){
				const std::vector<int>* parentindices = nexthit.GetParents();
				bool hitofinterest=false;
				for(const int& aparent : *parentindices){
					std::vector<int>::iterator parentofinterest = 
						std::find(particleidsofinterest.begin(), particleidsofinterest.end(), aparent);
					if(parentofinterest!=particleidsofinterest.end()){
						int parentofinterestnum = std::distance(particleidsofinterest.begin(), parentofinterest);
						chargesfromparents.at(parentofinterestnum)+= nexthit.GetCharge();
						hitofinterest=true;
					}
//					if(aparent==particleidsofinterest.at(0)){
//						chargesfromparents.at(0)+= nexthit.GetCharge();  // this hit was from primary muon
//					} else {
//						chargesfromparents.at(1)+= nexthit.GetCharge();  // all other light
//					}
//					hitofinterest=true;
				}
				if(not hitofinterest){ continue; } // ignore hits on this PMT not from a particle of interest
			}
			
			double hit_time = nexthit.GetTime();
			double hit_charge = nexthit.GetCharge();
			
			// keep track of the total event duration for colour scale range
			if(hit_time<event_earliest_hit_time) event_earliest_hit_time = hit_time;
			if(hit_time>event_latest_hit_time) event_latest_hit_time = hit_time;
			// do we want LAPPD charge scale to match PMT scale? Moot for now, LAPPD hit charge is a dummy value
			//if(maximum_charge_on_a_pmt<hit_charge) maximum_charge_on_a_pmt = hit_charge;
			
			// skip drawing this LAPPD hit if the time/charge is outside the plot range
			// (reduces # markers, prevents black markers obscuring others)
			if( (hit_charge<threshold_lappd) ||
					(threshold_time_high !=-999 && hit_time>threshold_time_high) ||
					(threshold_time_low  !=-999 && hit_time<threshold_time_low )  ) continue;
			
			// get global position of the hit
			std::vector<double> lappdposv = nexthit.GetPosition();
			Position lappdhitglobalpos(lappdposv.at(0), lappdposv.at(1), lappdposv.at(2));
			lappdhitglobalpos -= anniegeom->GetTankCentre();
			double phi = -lappdhitglobalpos.GetPhi();
			double y_lappd = lappdhitglobalpos.Y();
			
			// calculate marker position on canvas
			double markerx=0.,markery=0.;
			markerx=0.5+phi*size_top_drawing;
			markery=0.5+2.*y_lappd/tank_height*tank_height/tank_radius*size_top_drawing;
			
			// make the polymarker
			TPolyMarker *marker_lappd = new TPolyMarker(1,&markerx,&markery,"");
			
			// set the colour based on the time / charge of the hit. We'll scale to the event range before drawing
			if (mode == "Charge"){
				color_marker = hit_charge;
			} else if(mode == "Time"){
				color_marker = hit_time;
			} else if( mode == "Parent"){
				// determine the colour based on the parent
				// If a PMT saw light from multiple parent particles,
				// Use the relative charges to set the RGB components
				// Since this means creating (and keeping, for cumulative plots)
				// a TColor on the heap, only do this if it's necessary
				if((int(chargesfromparents.at(0)>0)+int(chargesfromparents.at(1)>0)+int(chargesfromparents.at(2)>0))>1){
					// this PMT saw light from multiple parents
					// ok, so to make a new TColor we need a free unique index for it
					// (that index (a 'Color_t') is actually the number we pass to 'SetMarkerColor')
					color_marker = startingcolournum + eventcolours.size();
					TColor* thetcolor = 
						new TColor(color_marker,chargesfromparents.at(0),chargesfromparents.at(1),chargesfromparents.at(2));
					// fortunately as we're specifying RGB components no scaling will be needed
					// but keep the TColor pointers so we can free them when we're done
					eventcolours.push_back(thetcolor);
				} else {
					if(chargesfromparents.at(0)>0){
						color_marker = 2; // Red
					} else if(chargesfromparents.at(1)>0){
						color_marker = 3; // Green
					} else {
						color_marker = 4; // Blue
					}
				}
			}
			
			// Set the marker properties
			marker_lappd->SetMarkerColor(color_marker);
			marker_lappd->SetMarkerStyle(2);  //8: small circle, 2: +
			marker_lappd->SetMarkerSize(0.4);
			// Add to the relevant set
			marker_lappds.push_back(marker_lappd);
			
		} // loop over hits on this LAPPD
	} // loop over LAPPDs
}

// ##############################################################
// ################## Process True Vertices #####################
// ##############################################################

void TotalLightMap::make_vertex_markers(MCParticle aparticle){
	
	// get true or projected tank exit point of particle in tank centred coordinates
	Position exitpoint = aparticle.GetTankExitPoint() - anniegeom->GetTankCentre();
	
				Position truevtx = aparticle.GetStartVertex() - anniegeom->GetTankCentre();
				Direction truedir = aparticle.GetStartDirection();
				std::cout<<"vertex marker for particle starting at: "; truevtx.Print(false);
				std::cout<<", going in direction: "; truedir.Print();
				std::cout<<", with tank exit: "; exitpoint.Print();
		
				vertexphihist->Fill(truedir.GetPhi());
				vertexthetahist->Fill(truedir.GetTheta());
				vertexyhist->Fill(exitpoint.Y());
	
	double vtxproj_x = exitpoint.X();  // We may not have one if the particle didn't start in and exit the tank
	double vtxproj_y = exitpoint.Y();  // this could be fixed in MCParticleProperties
	double vtxproj_z = exitpoint.Z();  //
	std::cout <<"projected vtx on wall: ( "<< vtxproj_x<<" , "<<vtxproj_y<<" , "<<vtxproj_z<< " )"<<std::endl;
	//std::cout <<"drawing vertex on EventDisplay.... "<<std::endl;
	
	double xTemp, yTemp;
	translate_xy(vtxproj_x,vtxproj_y,vtxproj_z,xTemp,yTemp);
	std::cout<<"translated_xy = ("<<xTemp<<", "<<yTemp<<")"<<std::endl;
	
	// colour the marker based on the particle type and parentage
	Color_t thecolour=0;
	std::string particletype="";
	if(aparticle.GetPdgCode()==13 && abs(aparticle.GetParentPdg())==0){
		particletype = "PrimaryMuon";
		thecolour = kMagenta+2;  // dark purple
	} else if(aparticle.GetPdgCode()==13 && aparticle.GetParentPdg()==-211){
		particletype = "MuonFromPiMinus";
		thecolour = kGreen+1;  // green
	} else if(aparticle.GetPdgCode()==-13 && abs(aparticle.GetParentPdg())==211){
		particletype = "AntiMuonFromPiPlus";
		thecolour = kOrange+7; // dark orange
	} else if(aparticle.GetPdgCode()==22 && aparticle.GetParentPdg()==111){
		particletype = "GammaFromPiZero";
		thecolour = kAzure+7;  // dark turquoise
	} else if(aparticle.GetPdgCode()==211){
		particletype = "Pi+";
		thecolour = kPink+9;  // very pink/magenta
	} else if(aparticle.GetPdgCode()==-211){
		particletype = "Pi-";
		thecolour = kViolet+5;   // like Lilac
	} else if(aparticle.GetPdgCode()==111){
		particletype = "Pi0";
		thecolour = kSpring-9;   // spring green
	} else {
		particletype = "Other";
		thecolour = 11; // Grey
	}
	std::string infotype = (particletype=="Other") ? to_string(aparticle.GetPdgCode()) : particletype;
	Log("TotalLightMap Tool: Making vertex marker for "+infotype,v_debug,verbosity);
	std::cout<<"marker vertex is ("<<xTemp<<", "<<yTemp<<")"<<std::endl;
	
	TPolyMarker* marker_vtx = new TPolyMarker(1,&xTemp,&yTemp,"");
	marker_vtx->SetMarkerStyle(22);
	marker_vtx->SetMarkerSize(1.);
	marker_vtx->SetMarkerColor(thecolour);
	// add it to the set of current event true vertices
	marker_vtxs.push_back(marker_vtx);
	
	// to add this true vertex to the cumulative displays
	// we need to convert it into Winkel Triple coordinates first
	double x_winkel, y_winkel;
	DoTheWinkelTripel(vtxproj_x,vtxproj_y,vtxproj_z, x_winkel, y_winkel);
	
	// we'll accumulate markers in a map with one TPolymarker per particle type
	// we'll actually use the colour code as a key, as it distinguishes primary muons vs pi- decay muons
	if(marker_vtxs_map.count(thecolour)==0){
		// key isnt' found (new particle type), so make a new marker set
		TPolyMarker* thepolymarkers = new TPolyMarker(1,&x_winkel, &y_winkel,"");
		thepolymarkers->SetMarkerColor(thecolour);
		marker_vtxs_map.emplace(thecolour, thepolymarkers);
	} else {
		// add this point to an existing marker set
		marker_vtxs_map.at(thecolour)->SetNextPoint(x_winkel, y_winkel);
	}
	
}

// ##############################################################
// ######################## Draw Markers ########################
// ##############################################################

void TotalLightMap::DrawMarkers(){
	
	// now that we've scanned the whole event we can find the maximum charge / hit time,
	// with which to calculate the scaling for the full range of marker colours
	if (mode == "Charge"){
		// colour based on charge
		colour_full_scale = maximum_charge_on_a_pmt;  // XXX do we want to do PMTs and LAPPDs separately?
		colour_offset = 0;
		size_full_scale = 3.;
		size_offset = 0;
	} else if (mode == "Time" && threshold_time_high == -999 && threshold_time_low == -999){
		// if we have no time window in effect, colour based on time within event
		colour_full_scale = event_latest_hit_time-event_earliest_hit_time;
		colour_offset = event_earliest_hit_time;
		size_full_scale = 3.;
		size_offset = 0;
	} else if (mode == "Time"){
		// else if we have a time window, colour based on time within window
		colour_full_scale = threshold_time_high-threshold_time_low;
		colour_offset = threshold_time_low;
		size_full_scale = 3.;
		size_offset = 0;
	} else if (mode == "Parent"){
		colour_full_scale = 254.;
		colour_offset = 0;
		size_full_scale = maximum_charge_on_a_pmt;
		size_offset = 0;
	}
	logmessage = "TotalLightMap Tool: Event colour scale will use mode "+mode
							+", offset "+to_string(colour_offset)
							+", and maximum "+to_string(colour_full_scale);
	Log(logmessage,v_debug,verbosity);
	
	// Select the Event display canvas
	canvas_ev_display->cd();
	
	// draw the pmt markers
	Log("TotalLightMap Tool: drawing "+to_string(marker_pmts_top.size())+" top cap PMT markers",v_debug,verbosity);
	for (int i_marker=0;i_marker<marker_pmts_top.size();i_marker++){
		TPolyMarker* marker = marker_pmts_top.at(i_marker);
		double colour_marker_temp = 254.*((double(marker->GetMarkerColor())-colour_offset)/colour_full_scale);
		color_marker = int(colour_marker_temp);
		if((mode=="Time")||(mode=="Charge")){
			color_marker+=Bird_Idx;
		} else {
			double size_marker_temp = 3.*(double(marker->GetMarkerSize()))/size_full_scale;
			marker->SetMarkerSize(size_marker_temp);
		}
		marker->SetMarkerColor(color_marker);
		marker->Draw();
	}
	Log("TotalLightMap Tool: drawing "+to_string(marker_pmts_bottom.size())+" bottom cap PMT markers",v_debug,verbosity);
	for (int i_marker=0;i_marker<marker_pmts_bottom.size();i_marker++){
		TPolyMarker* marker = marker_pmts_bottom.at(i_marker);
		double colour_marker_temp = 254.*((double(marker->GetMarkerColor())-colour_offset)/colour_full_scale);
		color_marker = int(colour_marker_temp);
		if((mode=="Time")||(mode=="Charge")){
			color_marker+=Bird_Idx;
		} else {
			double size_marker_temp = 3.*(double(marker->GetMarkerSize()))/size_full_scale;
			marker->SetMarkerSize(size_marker_temp);
		}
		marker->SetMarkerColor(color_marker);
		marker->Draw();
	}
	Log("TotalLightMap Tool: drawing "+to_string(marker_pmts_wall.size())+" barrel PMT markers",v_debug,verbosity);
	for (int i_marker=0;i_marker<marker_pmts_wall.size();i_marker++){
		TPolyMarker* marker = marker_pmts_wall.at(i_marker);
		double colour_marker_temp = 254.*((double(marker->GetMarkerColor())-colour_offset)/colour_full_scale);
		color_marker = int(colour_marker_temp);
		if((mode=="Time")||(mode=="Charge")){
			color_marker+=Bird_Idx;
		} else {
			double size_marker_temp = 3.*(double(marker->GetMarkerSize()))/size_full_scale;
			marker->SetMarkerSize(size_marker_temp);
		}
		marker->SetMarkerColor(color_marker);
		marker->Draw();
	}
	
	// draw the lappd markers
	Log("TotalLightMap Tool: drawing "+to_string(marker_lappds.size())+" LAPPD markers",v_debug,verbosity);
	for (int i_marker=0; i_marker<marker_lappds.size();i_marker++){
		TPolyMarker* marker = marker_lappds.at(i_marker);
		double colour_marker_temp = 254.*((double(marker->GetMarkerColor())-colour_offset)/colour_full_scale);
		color_marker = int(colour_marker_temp);
		if((mode=="Time")||(mode=="Charge")) color_marker+=Bird_Idx;
		marker->SetMarkerColor(color_marker);
		marker->Draw();
	}
	
	// draw the true vertex markers
	for(int i_marker=0; i_marker<marker_vtxs.size();i_marker++){
		marker_vtxs.at(i_marker)->Draw();  // colour is already set based on particle type
	}
	
	canvas_ev_display->Modified();
	canvas_ev_display->Update();
	
	// now the unbinned Wiener Triple plot - only one makes sense for a single event
	// and only if it's a CC1PI event
	lightmap_by_parent_canvas->cd();
	
	for(int i_marker=0; i_marker<chargemapcc1p.size();i_marker++){
		chargemapcc1p.at(i_marker)->Draw();  // colour is already set based on particle type
	}
	
	// draw the true vertex markers
	for(int i_marker=0; i_marker<marker_vtxs.size();i_marker++){
		marker_vtxs.at(i_marker)->Draw();  // colour is already set based on particle type
	}
	
	lightmap_by_parent_canvas->Modified();
	lightmap_by_parent_canvas->Update();
	
}

// ##############################################################
// ################### Draw Cumulative Markers ##################
// ##############################################################

void TotalLightMap::DrawCumulativeMarkers(){
	
	// Make the Canvas for the plot of cumulative light distribution coloured by event type:
	// blue markers = CCQE events, red markers = CCNPi events
	lightmap_by_eventtype_canvas = new TCanvas("lightmap_by_eventtype_canvas","",900,1000);
	lightmap_by_eventtype_canvas->cd();
	
	// draw unbinned light from CCQE events
	lightmapccqe->Draw();
	// draw unbinned light from CCNPi events
	lightmapcc1p->Draw();  // seems like "same" isn't needed
	
	// Draw all the true particle projected exit vertices
	for(std::pair<const int,TPolyMarker*>& amarkerset : marker_vtxs_map){
		amarkerset.second->Draw();
	}
	
	lightmap_by_eventtype_canvas->Modified();
	lightmap_by_eventtype_canvas->Update();
	
	// Select Canvas
	lightmap_by_parent_canvas->cd();
	
	for(TPolyMarker* amarker : chargemapcc1p_cum){
		amarker->Draw();
	}
	
	// Draw all the true particle projected exit vertices
	for(std::pair<const int,TPolyMarker*>& amarkerset : marker_vtxs_map){
		amarkerset.second->Draw();
	}
	
	lightmap_by_parent_canvas->Modified();
	lightmap_by_parent_canvas->Update();
	
}

// ##############################################################
// ########################## Make GUI ##########################
// ##############################################################

void TotalLightMap::make_gui(){   // make the canvas, the tank unrolled outline, etc
	
	canvas_ev_display=new TCanvas("canvas_ev_display","Event Display",700,700);
	
	//draw top circle
	TPad* p1 = (TPad*) canvas_ev_display->cd(1);
	canvas_ev_display->Draw();
	gPad->SetFillColor(0);
	p1->Range(0,0,1,1);
	
	top_circle = new TEllipse(0.5,0.5+(tank_height/tank_radius+1)*size_top_drawing,size_top_drawing,size_top_drawing);
	top_circle->SetFillColor(1);
	top_circle->SetLineColor(1);
	top_circle->SetLineWidth(1);
	top_circle->Draw();
	//draw bulk
	box = new TBox(0.5-TMath::Pi()*size_top_drawing,0.5-tank_height/tank_radius*size_top_drawing,0.5+TMath::Pi()*size_top_drawing,0.5+tank_height/tank_radius*size_top_drawing);
	box->SetFillColor(1);
	box->SetLineColor(1);
	box->SetLineWidth(1);
	box->Draw();
	//draw lower circle
	bottom_circle = new TEllipse(0.5,0.5-(tank_height/tank_radius+1)*size_top_drawing,size_top_drawing,size_top_drawing);
	bottom_circle->SetFillColor(1);
	bottom_circle->SetLineColor(1);
	bottom_circle->SetLineWidth(1);
	bottom_circle->Draw();
	
	canvas_ev_display->Modified();
	canvas_ev_display->Update();
	
	if((mode=="Charge")||(mode=="Time")){
		// gStyle->SetPalette(kBird); // doesn't seem to apply to markers....
		//calculate the numbers of the color palette
		Double_t stops[9] = { 0.0000, 0.1250, 0.2500, 0.3750, 0.5000, 0.6250, 0.7500, 0.8750, 1.0000};
		Double_t red[9]   = { 0.2082, 0.0592, 0.0780, 0.0232, 0.1802, 0.5301, 0.8186, 0.9956, 0.9764};
		Double_t green[9] = { 0.1664, 0.3599, 0.5041, 0.6419, 0.7178, 0.7492, 0.7328, 0.7862, 0.9832};
		Double_t blue[9]  = { 0.5293, 0.8684, 0.8385, 0.7914, 0.6425, 0.4662, 0.3499, 0.1968, 0.0539};
		Double_t alpha=1.;  //make colors opaque, not transparent
		Bird_Idx = TColor::CreateGradientColorTable(9, stops, red, green, blue, 255, alpha);
	}
	
}

// ##############################################################
// ######################## Cleanup #############################
// ##############################################################

void TotalLightMap::FinalCleanup(){
	Log("TotalLightMap Tool: Doing cleanup",v_debug,verbosity);
	// Cleanup event accumulative markers
	// the two light pattern TPolyMarker sets for CCQE vs CCNPi comprisons
	delete lightmapccqe;
	delete lightmapcc1p;
	
	// the TPolyMarkers for true particle vertices
	for(std::pair<const int,TPolyMarker*>& amarkerset : marker_vtxs_map){
		delete amarkerset.second;
	}
	marker_vtxs_map.clear();
	
	// cumulative hits of CCNPi events where marker colouring reflects the hit parentage
	for(TPolyMarker* amarker : chargemapcc1p_cum){
		delete amarker;
	}
	chargemapcc1p_cum.clear();
	
	// clean up all custom colours
	for(TColor* acolor : eventcolours){
		delete acolor;
	}
	eventcolours.clear();
	
	// clean up binned histograms
	if(lmccqe) delete lmccqe; lmccqe=nullptr;
	if(lmcc1p) delete lmcc1p; lmcc1p=nullptr;
	if(lmdiff) delete lmdiff; lmdiff=nullptr;
	if(lmmuon) delete lmmuon; lmmuon=nullptr;
	if(lmpigammas) delete lmpigammas; lmpigammas=nullptr;
	if(lmdiff2) delete lmdiff2; lmdiff2=nullptr;
	
	if(lightmap_by_eventtype_canvas) delete lightmap_by_eventtype_canvas; lightmap_by_eventtype_canvas=nullptr;
	if(lightmap_by_parent_canvas) delete lightmap_by_parent_canvas; lightmap_by_parent_canvas=nullptr;
	
	// final event display gui cleanup
	if(top_circle) delete top_circle; top_circle=nullptr;
	if(bottom_circle) delete bottom_circle; bottom_circle=nullptr;
	if(box) delete box; box=nullptr;
	if(gROOT->FindObject("canvas_ev_display")) delete canvas_ev_display; canvas_ev_display=nullptr;
}

// ################################################################
// ########################## Draw Legend #########################
// ################################################################

/*
void TotalLightMap::MakeLegend(){
	// the full scale colour range is constant, so this only needs to be drawn once
	for (int co=0; co<255; co++){
		// what the heck XXX XXX XXX
		 float yc = 0.53+((1.4*co/255)-1.)*tank_height/tank_radius*size_top_drawing;
		TMarker *colordot = new TMarker(0.08,yc,21);
		colordot->SetMarkerColor(Bird_Idx+co);
		colordot->SetMarkerSize(3.);
		colordot->Draw();
		legend_dots.push_back(colordot);
	}
}

void TotalLightMap::UpdateLegend(){
	// TODO: update (make if not existant, edit otherwise) 
	// the TPaveLabel text that displays above the maximum and minimum
	// charges / times of the event
	TPaveLabel* colourscalemax;
	TPaveLabel* colourscalemin;
	
}

// ############# Michael vers
void TotalLightMap::draw_pmt_legend(){
    
    //draw pmt legend on the left side
    TPaveLabel *pmt_title = new TPaveLabel(0.05,0.5+tank_height/tank_radius*size_top_drawing-0.03,0.15,0.5+tank_height/tank_radius*size_top_drawing,mode.c_str(),"l");
    pmt_title->SetTextFont(40);
    pmt_title->SetFillColor(0);
    pmt_title->SetTextColor(1);
    pmt_title->SetBorderSize(0);
    pmt_title->SetTextAlign(11);
    pmt_title->Draw();
    for (int co=0; co<255; co++){
        float yc = 0.53+((1.4*co/255)-1.)*tank_height/tank_radius*size_top_drawing;
        TMarker *colordot = new TMarker(0.08,yc,21);
        colordot->SetMarkerColor(co);
        colordot->SetMarkerSize(3.);
        colordot->Draw();
    }

    std::string max_charge_pre = std::to_string(int(maximum_pmts));
    std::string pe_string = " p.e.";
    std::string max_charge = max_charge_pre+pe_string;
    std::string max_time_pre = std::to_string(int(maximum_time_overall));
    std::string time_string = " ns";
    std::string max_time;
    if (threshold_time_high==-999) max_time = max_time_pre+time_string;
    else max_time = std::to_string(int(threshold_time_high))+time_string;
    //std::cout <<"max time: "<<max_time<<std::endl;
    TPaveLabel *max_text = nullptr;
    if (mode == "Charge") max_text = new TPaveLabel(0.10,0.5-tank_height/tank_radius*size_top_drawing+0.03+0.7*2*tank_height/tank_radius*size_top_drawing*0.95,0.17,0.5-tank_height/tank_radius*size_top_drawing+0.03+0.7*2*tank_height/tank_radius*size_top_drawing*1.05,max_charge.c_str(),"L");
    else if (mode == "Time") max_text = new TPaveLabel(0.10,0.5-tank_height/tank_radius*size_top_drawing+0.03+0.7*2*tank_height/tank_radius*size_top_drawing*0.95,0.17,0.5-tank_height/tank_radius*size_top_drawing+0.03+0.7*2*tank_height/tank_radius*size_top_drawing*1.05,max_time.c_str(),"L");
    max_text->SetFillColor(0);
    max_text->SetTextColor(1);
    max_text->SetTextFont(40);
    max_text->SetBorderSize(0);
    max_text->SetTextAlign(11);
    max_text->Draw();

    std::string min_charge_pre = (threshold==-999)? "0" : std::to_string(int(threshold));
    std::string min_charge = min_charge_pre+pe_string;
    std::string min_time_pre = std::to_string(int(min_time_overall));
    std::string min_time;
    if (threshold_time_low==-999) min_time = min_time_pre+time_string;
    else min_time = std::to_string(int(threshold_time_low))+time_string;
    TPaveLabel *min_text = nullptr;
    if (mode == "Charge") min_text = new TPaveLabel(0.10,0.5-tank_height/tank_radius*size_top_drawing+0.03+0.7*2*tank_height/tank_radius*size_top_drawing*(-0.05),0.17,0.5-tank_height/tank_radius*size_top_drawing+0.03+0.7*2*tank_height/tank_radius*size_top_drawing*0.05,min_charge.c_str(),"L");
    else if (mode == "Time") min_text = new TPaveLabel(0.10,0.5-tank_height/tank_radius*size_top_drawing+0.03+0.7*2*tank_height/tank_radius*size_top_drawing*(-0.05),0.17,0.5-tank_height/tank_radius*size_top_drawing+0.03+0.7*2*tank_height/tank_radius*size_top_drawing*0.05,min_time.c_str(),"L");
    min_text->SetFillColor(0);
    min_text->SetTextColor(1);
    min_text->SetTextFont(40);
    min_text->SetBorderSize(0);
    min_text->SetTextAlign(11);
    min_text->Draw();
    
}
  
  void EventDisplay::draw_lappd_legend(){

    //draw LAPPD legend on the right side of the event display

    TPaveLabel *lappd_title = nullptr;
    if (mode == "Charge") lappd_title = new TPaveLabel(0.85,0.5+tank_height/tank_radius*size_top_drawing-0.03,0.95,0.5+tank_height/tank_radius*size_top_drawing,"charge [LAPPDs]","l");
    else if (mode == "Time") lappd_title = new TPaveLabel(0.85,0.5+tank_height/tank_radius*size_top_drawing-0.03,0.95,0.5+tank_height/tank_radius*size_top_drawing,"time [LAPPDs]","l");
    lappd_title->SetTextFont(40);
    lappd_title->SetFillColor(0);
    lappd_title->SetTextColor(1);
    lappd_title->SetBorderSize(0);
    lappd_title->SetTextAlign(11);
    lappd_title->Draw();
    for (int co=0; co<255; co++)
    {
        float yc = 0.5-tank_height/tank_radius*size_top_drawing+0.03+0.7*2*tank_height/tank_radius*size_top_drawing*co/255;
        TMarker *colordot = new TMarker(0.85,yc,21);
        colordot->SetMarkerColor(Bird_Idx+co);
        colordot->SetMarkerSize(3.);
        colordot->Draw();
    }
    
    std::string max_time_pre = std::to_string(int(maximum_time_overall));
    std::string time_string = " ns";
    std::string max_time_lappd;
    if (threshold_time_high==-999) max_time_lappd = max_time_pre+time_string;
    else max_time_lappd = std::to_string(int(threshold_time_high))+time_string;
    //std::cout <<"max time lappd: "<<max_time_lappd<<std::endl;
    TPaveLabel *max_lappd = nullptr;
    if (mode == "Charge") max_lappd = new TPaveLabel(0.88,0.5-tank_height/tank_radius*size_top_drawing+0.03+0.7*2*tank_height/tank_radius*size_top_drawing*0.95,0.95,0.5-tank_height/tank_radius*size_top_drawing+0.03+0.7*2*tank_height/tank_radius*size_top_drawing*1.05,"1 hit","L");
    else if (mode == "Time") max_lappd = new TPaveLabel(0.88,0.5-tank_height/tank_radius*size_top_drawing+0.03+0.7*2*tank_height/tank_radius*size_top_drawing*0.95,0.95,0.5-tank_height/tank_radius*size_top_drawing+0.03+0.7*2*tank_height/tank_radius*size_top_drawing*1.05,max_time_lappd.c_str(),"L");
    max_lappd->SetFillColor(0);
    max_lappd->SetTextColor(1);
    max_lappd->SetTextFont(40);
    max_lappd->SetBorderSize(0);
    max_lappd->SetTextAlign(11);
    max_lappd->Draw();

    //std::string min_time_pre = std::to_string(int(min_time_lappds));
    std::string min_time_lappd;
    std::string min_time_pre = std::to_string(int(min_time_overall));
    if (threshold_time_low==-999) min_time_lappd = min_time_pre+time_string;
    else min_time_lappd = std::to_string(int(threshold_time_low))+time_string; 
    TPaveLabel *min_lappd = nullptr;
    if (mode == "Charge") min_lappd = new TPaveLabel(0.88,0.5-tank_height/tank_radius*size_top_drawing+0.03+0.7*2*tank_height/tank_radius*size_top_drawing*(-0.05),0.95,0.5-tank_height/tank_radius*size_top_drawing+0.03+0.7*2*tank_height/tank_radius*size_top_drawing*0.05,"0 hits","L");
    else if (mode == "Time") min_lappd = new TPaveLabel(0.88,0.5-tank_height/tank_radius*size_top_drawing+0.03+0.7*2*tank_height/tank_radius*size_top_drawing*(-0.05),0.95,0.5-tank_height/tank_radius*size_top_drawing+0.03+0.7*2*tank_height/tank_radius*size_top_drawing*0.05,min_time_lappd.c_str(),"L");
    min_lappd->SetFillColor(0);
    min_lappd->SetTextColor(1);
    min_lappd->SetTextFont(40);
    min_lappd->SetBorderSize(0);
    min_lappd->SetTextAlign(11);
    min_lappd->Draw();

  }

*/
// ########################################

// ##############################################################
// ##################### Support Functions ######################
// ##############################################################

void TotalLightMap::find_projected_xyz(double vtxX, double vtxY, double vtxZ, double dirX, double dirY, double dirZ, double &projected_x, double &projected_y, double &projected_z){
	
	// projected y position of tank exit... 
	double max_y = tank_height;
	double min_y = -tank_height;
	vtxY*=2.;
	
	double time_top = (dirY > 0)? (max_y-vtxY)/dirY : (min_y - vtxY)/dirY;
	double a = dirX*dirX + dirZ*dirZ;
	double b = 2*vtxX*dirX + 2*vtxZ*dirZ;
	double c = vtxX*vtxX+vtxZ*vtxZ-tank_radius*tank_radius;
	double time_wall1 = (-b+sqrt(b*b-4*a*c))/(2*a);
	double time_wall2 = (-b-sqrt(b*b-4*a*c))/(2*a);
	double time_wall=0;
	if (time_wall2>=0 && time_wall1>=0) time_wall = (time_wall1<time_wall2) ? time_wall1 : time_wall2;
	else if (time_wall2 < 0 ) time_wall = time_wall1;
	else if (time_wall1 < 0 ) time_wall = time_wall2;
	
	double time_min = (time_wall < time_top && time_wall >=0 )? time_wall : time_top;
	
	projected_x = vtxX+dirX*time_min;
	projected_y = vtxY+dirY*time_min;
	projected_z = vtxZ+dirZ*time_min;
}

//////////////////////////////////////////

void TotalLightMap::translate_xy(double vtxX, double vtxY, double vtxZ, double &xWall, double &yWall){
	
	double max_y = tank_height;
	double min_y = -tank_height;
	vtxY*=2.;
	//if (cylloc=="TopCap"){                   //draw vtx projection on the top of tank
	if (fabs(vtxY-max_y)<0.01){ 
		Log("TotalLightMap Tool: translate_xy placing marker on top cap",v_debug,verbosity);
		xWall=0.5-size_top_drawing*vtxX/tank_radius;
		yWall=0.5+(tank_height/tank_radius+1)*size_top_drawing-size_top_drawing*vtxZ/tank_radius;
		
	//} else if (cylloc=="BottomCap"){         //draw vtx projection on the bottom of tank
	} else if (fabs(vtxY-min_y)<0.01){ 
		Log("TotalLightMap Tool: translate_xy placing marker on bottom cap",v_debug,verbosity);
		xWall=0.5-size_top_drawing*vtxX/tank_radius;
		yWall=0.5-(tank_height/tank_radius+1)*size_top_drawing+size_top_drawing*vtxZ/tank_radius;
		
	} else {
		Log("TotalLightMap Tool: translate_xy placing marker on barrel",v_debug,verbosity);
		Position vtxpos(vtxX, vtxY, vtxZ);
		double phi=-vtxpos.GetPhi();
		xWall=0.5+phi*size_top_drawing;
		yWall=0.5+vtxY/tank_height*tank_height/tank_radius*size_top_drawing;
		std::cout<<"vtxY="<<vtxY<<", (vtxY/tank_height)="<<(vtxY/tank_height)
						 <<", (tank_height/tank_radius)="<<(tank_height/tank_radius)
						 <<", size_top_drawing="<<size_top_drawing
						 <<", (vtxY/tank_height*tank_height/tank_radius*size_top_drawing)="
						 <<vtxY/tank_height*tank_height/tank_radius*size_top_drawing<<endl;
	}
	
}

/////////////////////////////////////////

void TotalLightMap::DoTheWinkelTripel(double x, double y, double z, double& x_winkel, double& y_winkel){
	// convert x,y,z to longtitude and latitude
	double R, Phi, Theta;
	anniegeom->CartesianToPolar(Position(x,y,z), R, Phi, Theta, true); // true: x,y,z are already tank centered
	// then do the winkel tripel
	DoTheWinkelTripel(Theta, Phi, x_winkel, y_winkel);
}

void TotalLightMap::DoTheWinkelTripel(double latitude, double longitude, double& x_winkel, double& y_winkel){
	// latitude and longitude in RADIANS: those are theta and phi, angle from x-z plane and y-z plane respectively
	double R=1.0;   // scaling... not sure what it scales.
	
	// first we need the cylindrical equidistant projection plotting coordinates
	// for which we need to define:
	double standard_parallel = 50. + (28./60.); // 50°28'. Apparently it needs to be this value for WikelTripel...
	// we can then calculate the projection coordinates as:
	double x_ce = R*longitude*cos(standard_parallel);                 /* 3. */
	double y_ce = R*latitude;                                         /* 4. */
	
	// next we need the Aitoff projection coordinates as well
	double x_aitoff, y_aitoff;
	// for which we need to define:
	double D = acos(cos(latitude)*cos(longitude/2.));                 /* 5. */
	if(D==0){  // protect against division by 0
		x_aitoff = 0;                                                   /* 6. */
		y_aitoff = 0;                                                   /* 7. */
	} else {
		double C = sin(latitude)/sin(D);                                /* 8. */
		x_aitoff = 2.*R*D*sqrt(1.-pow(C,2.));                           /* 9. */
		y_aitoff = R*D*C;                                               /* 10.*/
	}
	
	// finally the Winkel Tripel is simply the average:
	x_winkel = (x_ce + x_aitoff)/2.;                                  /* 1. */
	y_winkel = (y_ce + y_aitoff)/2.;                                  /* 2. */
}

void TotalLightMap::DoTheMercator(double latitude, double longitude, double& x_mercater, double& y_mercater, double mapHeight, double mapWidth){
	// latitude + longitude to Mercator projection X,Y
	x_mercater = (longitude+M_PI)*(mapWidth/(2*M_PI));
	double mercN = log(tan((M_PI/4.)+(latitude/2.)));   // 'log' is ln, log10 is log10
	y_mercater = (mapHeight/2.)-mercN*(mapHeight/(2.*M_PI));
}

void TotalLightMap::InverseMercator(double x_mercater, double y_mercater, double& latitude, double& longitude, double mapHeight, double mapWidth){
	longitude = ((360.*x_mercater)/mapWidth) - 180.;
	latitude = 90.*(-1. + (4.*atan(pow(exp(1.), (M_PI - (2.*M_PI*y_mercater)/mapHeight)))) / M_PI);
}

void TotalLightMap::CartesianToStereoGraphic(double x, double y, double z, double& xout, double& yout){
	/* convert the locations on a unit sphere (x,y,z) to the projected location on a plane XY
	   The sphere is projected from it's top pole (0,0,1) through a given point on the sphere
	   to the plane (*,*,-1). The projected position in the projected plane is given by: */
	xout = 2.*x / (1.-z);
	yout = 2.*y / (1.-z);
}
