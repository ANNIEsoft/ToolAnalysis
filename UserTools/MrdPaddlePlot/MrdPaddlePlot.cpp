/* vim:set noexpandtab tabstop=4 wrap */
#include "MrdPaddlePlot.h"
#include "TCanvas.h"
#ifdef GOT_EVE
#include "TGeoManager.h"
#include "TEveLine.h"
#include "TGLViewer.h"
#endif
#include <thread>         // std::this_thread::sleep_for
#include <chrono>         // std::chrono::seconds
#include <time.h>         // clock_t, clock, CLOCKS_PER_SEC
#include <algorithm>      // for std::replace

MrdPaddlePlot::MrdPaddlePlot():Tool(){}

bool MrdPaddlePlot::Initialise(std::string configfile, DataModel &data){

	if(verbosity) cout<<"Initializing tool MrdPaddlePlot"<<endl;

	/////////////////// Usefull header ///////////////////////
	if(configfile!="")  m_variables.Initialise(configfile); //loading config file
	//m_variables.Print();

	m_data= &data; //assigning transient data pointer
	/////////////////////////////////////////////////////////////////
	
	// get configuration variables for this tool
	m_variables.Get("verbosity",verbosity);
	m_variables.Get("gdmlpath",gdmlpath);
	m_variables.Get("saveimages",saveimages);
	std::string plotDirectoryString;
	m_variables.Get("plotDirectory",plotDirectoryString);
	plotDirectory = plotDirectoryString.c_str();
	m_variables.Get("drawCanvases",enableTApplication);
	if(enableTApplication){
		m_variables.Get("drawPaddlePlot",drawPaddlePlot);
		m_variables.Get("drawGdmlOverlay",drawGdmlOverlay);
		
		// for gdml overlay
		double buildingoffsetx, buildingoffsety, buildingoffsetz;
		m_variables.Get("buildingoffsetx",buildingoffsetx);
		m_variables.Get("buildingoffsety",buildingoffsety);
		m_variables.Get("buildingoffsetz",buildingoffsetz);
		buildingoffset = Position(buildingoffsetx,buildingoffsety,buildingoffsetz);
	} else {
		drawPaddlePlot=false;
		drawGdmlOverlay=false;
	}
	m_variables.Get("printTracks",printTracks);  // from the BoostStore
	m_variables.Get("printTClonesTracks",printTClonesTracks); // from the FindMrdTracks Tool
	
	//////////////////////////////////////////////////////////////////
	
	numents=0;    // number of events (~triggers) analysed
	totnumtracks=0;
	numstopped=0;
	numpenetrated=0;
	numsideexit=0;
	numtankintercepts=0;
	numtankmisses=0;
	
	// TODO fix ranges
	hnumsubevs = new TH1F("numsubevs","Number of MRD SubEvents",20,0,10);
	hnumtracks = new TH1F("numtracks","Number of MRD Tracks",10,0,10);
	hrun = new TH1F("run","Run Number Histogram",20,0,9);
	hevent = new TH1F("event","Event Number Histogram",100,0,2000);
	hmrdsubev = new TH1F("subevent","MRD SubEvent Number Histogram",20,0,19);
	htrigger = new TH1F("trigg","Trigger Number Histogram",10,0,9);
	hnumhclusters = new TH1F("numhclusts","Number of H Clusters in MRD Track",20,0,10);
	hnumvclusters = new TH1F("numvclusts","Number of V Clusters in MRD Track",20,0,10);
	hnumhcells = new TH1F("numhcells","Number of H Cells in MRD Track",20,0,10);
	hnumvcells = new TH1F("numvcells","Number of V Cells in MRD Track",20,0,10);
	hpaddleids = new TH1F("paddleids","IDs of MRD Paddles Hit",500,0,499);
	hpaddleinlayeridsh = new TH1F("inlayerpaddleidsh","IDs of MRD Paddles Hit Within H Layer",60,0,30);
	hpaddleinlayeridsv = new TH1F("inlayerpaddleidsv","IDs of MRD Paddles Hit Within V Layer",60,0,30);
	hdigittimes = new TH1D("digittimes","Digit Times",500,900,1300);
	hhangle = new TH1F("hangle","Track Angle in Top View",100,-TMath::Pi(),TMath::Pi());
	hhangleerr = new TH1F("hangleerr","Error in Track Angle in Top View",100,0,TMath::Pi());
	hvangle = new TH1F("vangle","Track Angle in Side View",100,-TMath::Pi(),TMath::Pi());
	hvangleerr = new TH1F("vangleerr","Error in Track Angle in Side View",100,0,TMath::Pi());
	htotangle = new TH1F("totangle","Track Angle from Beam Axis",100,0,TMath::Pi());
	htotangleerr = new TH1F("totangleerr","Error in Track Angle from Beam Axis",100,0,TMath::Pi());
	henergyloss = new TH1F("energyloss","Track Energy Loss in MRD",100,0,1000);
	henergylosserr = new TH1F("energylosserr","Error in Track Energy Loss in MRD",100,0,2000);
	htracklength = new TH1F("tracklen","Total Track Length in MRD",100,0,220);
	htrackpen = new TH1F("trackpen","Track Penetration in MRD",100,0,200);
	htrackpenvseloss = new TH2F("trackpenvseloss","Track Penetration vs E Loss",100,0,220,100,0,1000);
	htracklenvseloss = new TH2F("tracklenvseloss","Track Length vs E Loss",100,0,220,100,0,1000);
	htrackstart = new TH3D("trackstart","MRD Track Start Vertices",100,-170,170,100,300,480,100,-230,220);
	htrackstop = new TH3D("trackstop","MRD Track Stop Vertices",100,-170,170,100,300,480,100,-230,220);
	hpep = new TH3D("pep","Back Projected Tank Exit",100,-500,500,100,0,480,100,-330,320);
	
	htrackstart->SetMarkerStyle(20);
	htrackstart->SetMarkerColor(kRed);
	htrackstop->SetMarkerStyle(20);
	htrackstop->SetMarkerColor(kBlue);
	hpep->SetMarkerStyle(20);
	
	Double_t canvwidth = 700;
	Double_t canvheight = 600;
	if(enableTApplication){
		// create the ROOT application to show histograms
		if(verbosity>3) cout<<"Tool MrdPaddlePlot: constructing TApplication"<<endl;
		int myargc=0;
		char *myargv[] = {(const char*)"somestring2"};
		mrdPaddlePlotApp = new TApplication("mrdPaddlePlotApp",&myargc,myargv);
	}
	
#ifdef GOT_GEO
	if(drawGdmlOverlay){
		// Import the gdml geometry for the detector:
		TGeoManager::Import(gdmlpath.c_str());
		// make all the materials semi-transparent so we can see the tracks going through them:
		TList* matlist = gGeoManager->GetListOfMaterials();
		TIter nextmaterial(matlist);
		while(TGeoMixture* amaterial=(TGeoMixture*)nextmaterial()) amaterial->SetTransparency(90);
		gdmlcanv = new TCanvas("gdmlcanv","gdmlcanv",canvwidth,canvheight);
		gdmlcanv->cd();
		gGeoManager->GetVolume("WORLD2_LV")->Draw("ogl");
		
		// A canvas for other MRD track stats plots: TODO decouple from gdml
		mrdTrackCanv = new TCanvas("mrdTrackCanv","mrdTrackCanv",canvwidth,canvheight);
	}
#endif
	
	// Get a pointer to the TClonesArray filled by FindMrdTracks
	//m_data->CStore.Get("MrdSubEventTClonesArray",thesubeventarray);
	intptr_t subevptr;
	bool returnval = m_data->CStore.Get("MrdSubEventTClonesArray",subevptr);
	if(!returnval){
		cerr<<"Failed to get MrdSubEventTClonesArray from CStore!!"<<endl;
		return false;
	}
	thesubeventarray = reinterpret_cast<TClonesArray*>(subevptr);
	
	return true;
}


bool MrdPaddlePlot::Execute(){
	
	if(verbosity) cout<<"executing tool MrdPaddlePlot"<<endl;
	
	// Get the ANNIE event and extract information
	// Maybe this information could be added to plots for reference
	m_data->Stores["ANNIEEvent"]->Get("MCFile",MCFile);
	m_data->Stores["ANNIEEvent"]->Get("RunNumber",RunNumber);
	m_data->Stores["ANNIEEvent"]->Get("SubrunNumber",SubrunNumber);
	m_data->Stores["ANNIEEvent"]->Get("EventNumber",EventNumber);
	m_data->Stores["ANNIEEvent"]->Get("TriggerNumber",MCTriggernum);
	m_data->Stores["ANNIEEvent"]->Get("MCEventNum",MCEventNum);
	
	// demonstrate retrieving tracks from the BoostStore
	m_data->Stores["MRDTracks"]->Get("NumMrdSubEvents",numsubevs);
	m_data->Stores["MRDTracks"]->Get("NumMrdTracks",numtracksinev);
	
	cout<<"Event "<<EventNumber<<" had "<<numtracksinev<<" tracks in "<<numsubevs<<" subevents"<<endl;
	int got_ok = m_data->Stores["MRDTracks"]->Get("MRDTracks", theMrdTracks);
	if(not got_ok){
		cerr<<"No MRDTracks member of the MRDTracks BoostStore!"<<endl;
		cout<<"MRDTracks store contents:"<<endl;
		m_data->Stores["MRDTracks"]->Print(false);
		return false;
	}
	if(theMrdTracks->size()<numtracksinev){
		cerr<<"Too few entries in MRDTracks vector relative to NumMrdTracks!"<<endl;
		// more is fine as we don't shrink for efficiency
	}
	
	if(verbosity>2) cout<<"looping over theMrdTracks"<<endl;
	for(int tracki=0; tracki<numtracksinev; tracki++){
		BoostStore* thisTrackAsBoostStore = &(theMrdTracks->at(tracki));
		if(verbosity>3) cout<<"track "<<tracki<<" at "<<thisTrackAsBoostStore<<endl;
		// Get the track details from the BoostStore
		thisTrackAsBoostStore->Get("MrdTrackID",MrdTrackID);
		thisTrackAsBoostStore->Get("MrdSubEventID",MrdSubEventID);
		thisTrackAsBoostStore->Get("InterceptsTank",InterceptsTank);
		thisTrackAsBoostStore->Get("StartTime",StartTime);
		thisTrackAsBoostStore->Get("StartVertex",StartVertex);
		thisTrackAsBoostStore->Get("StopVertex",StopVertex);
		thisTrackAsBoostStore->Get("TrackAngle",TrackAngle);
		thisTrackAsBoostStore->Get("TrackAngleError",TrackAngleError);
		thisTrackAsBoostStore->Get("LayersHit",LayersHit);
		thisTrackAsBoostStore->Get("TrackLength",TrackLength);
		thisTrackAsBoostStore->Get("IsMrdPenetrating",IsMrdPenetrating);
		thisTrackAsBoostStore->Get("IsMrdStopped",IsMrdStopped);
		thisTrackAsBoostStore->Get("IsMrdSideExit",IsMrdSideExit);
		thisTrackAsBoostStore->Get("PenetrationDepth",PenetrationDepth);
		thisTrackAsBoostStore->Get("HtrackFitChi2",HtrackFitChi2);
		thisTrackAsBoostStore->Get("HtrackFitCov",HtrackFitCov);
		thisTrackAsBoostStore->Get("VtrackFitChi2",VtrackFitChi2);
		thisTrackAsBoostStore->Get("VtrackFitCov",VtrackFitCov);
		thisTrackAsBoostStore->Get("PMTsHit",PMTsHit);
		
		// Print the track info
		if(printTracks){
			cout<<"MrdTrack "<<MrdTrackID<<" in subevent "<<MrdSubEventID<<" was reconstructed from "
				<<PMTsHit.size()<<" MRD PMTs hit in "<<LayersHit.size()
				<<" layers for a total penetration depth of "<<PenetrationDepth<<" [m]."<<endl;
			cout<<"The track went from ";
			StartVertex.Print(false);
			cout<<" [m] to ";
			StopVertex.Print(false);
			cout<<" [m] starting at time "<<StartTime<<" [ns], travelling "<<TrackLength
				<<" [m] through the MRD at an angle of "<<TrackAngle
				<<" +/- "<<abs(TrackAngleError)<<" [rads] to the beam axis, before ";
			if(IsMrdStopped) cout<<"stopping in the MRD";
			if(IsMrdPenetrating) cout<<"penetrating the MRD";
			if(IsMrdSideExit) cout<<"leaving through the side of the MRD";
			cout<<endl<<"Back-projection suggests this track ";
			if(InterceptsTank) cout<<"intercepted"; else cout<<"did not intercept";
			cout<<" the tank."<<endl;
		}
	}
	
	// ##############################################################################
	// Track drawing and Histogram Filling code
	// This works entirely with the TClonesArray created by FindMrdTracks Tool
	// since the MRDTrack classes have plotting tools built-in
	// TODO: move the histogramming to use BoostStore to separate the two
	// ##############################################################################
	
//	// we may need to re-retrieve the pointer??... shouldn't do, as SubEventArray gets reused
//	intptr_t subevptr;
//	bool returnval = m_data->CStore.Get("MrdSubEventTClonesArray",subevptr);
//	if(!returnval){
//		cerr<<"Failed to get MrdSubEventTClonesArray from CStore!!"<<endl;
//		return false;
//	}
//	thesubeventarray = reinterpret_cast<TClonesArray*>(subevptr);
	
	bool earlyexit=false; // could be used to trigger interesting events
	int thetracki=-1;
	int numtracksdrawn=0;
	int numtracksrunningtot=0;
	// clear the track lines from the last event before adding new ones
#ifdef GOT_EVE
	if(verbosity>2) cout<<"deleting TEveLines"<<endl;
	for(TEveLine* aline : thiseventstracks){ delete aline; }
#endif
	thiseventstracks.clear();
	
	if(verbosity>2) cout<<"Printing "<<numsubevs<<" MRD subevents in event "<<EventNumber<<endl;
	if(verbosity>3) cout<<"TClonesArray says it has "<<thesubeventarray->GetEntries()<<" entries"<<endl;
	if(thesubeventarray->GetEntries()!=numsubevs){
		cerr<<"mismatch between MRDTrack TClonesArray size and ANNIEEVENT recorded number of MRD Subevents!"<<endl;
		return false;
	}
	
	hnumsubevs->Fill(numsubevs);
	
	// loop over subevents (collections of hits on the MRD within a narrow time window)
	for(int subevi=0; subevi<numsubevs; subevi++){
		cMRDSubEvent* thesubevent = (cMRDSubEvent*)thesubeventarray->At(subevi);
		if(printTClonesTracks){
			if(verbosity>3) cout<<"printing subevent "<<subevi<<endl;
			thesubevent->Print();
		}
		
		std::vector<cMRDTrack>* tracksthissubevent = thesubevent->GetTracks();
		int numtracks = tracksthissubevent->size();
		hnumtracks->Fill(numtracks);     // num tracks in a given MRD subevent - (a short time window)
		numtracksrunningtot+=numtracks;  // num tracks within all subevents in this event
		totnumtracks+=numtracks;         // total number of tracks analysed in this ToolChain run
		
		// loop over tracks within this subevent
		if(verbosity>2) cout<<"scanning "<<numtracks<<" tracks in subevent "<<subevi<<endl;
		for(auto&& thetrack : *tracksthissubevent){
			thetracki++;
			if(printTClonesTracks){
				if(verbosity>3) cout<<"printing next track"<<endl;
				thetrack.Print2(false); // false: do not re-print subevent info
			}
			
			if(thetrack.ispenetrating) numpenetrated++;
			else if(thetrack.isstopped) numstopped++;
			else numsideexit++;
			if(thetrack.interceptstank) numtankintercepts++;
			else numtankmisses++;
			//if(!thetrack.interceptstank) earlyexit=true;
			
			hrun->Fill(thetrack.run_id);
			hevent->Fill(thetrack.event_id);
			hmrdsubev->Fill(thetrack.mrdsubevent_id);
			htrigger->Fill(thetrack.trigger);
			
			hnumhclusters->Fill(thetrack.htrackclusters.size());
			hnumvclusters->Fill(thetrack.vtrackclusters.size());
			for(auto&& acluster : thetrack.htrackclusters){
				hpaddleinlayeridsh->Fill(acluster.GetCentreIndex());
				Int_t uptubetopid = (2*acluster.xmaxid) + MRDSpecs::layeroffsets.at(acluster.layer);
				Int_t uptubebottomid = (2*acluster.xminid) + MRDSpecs::layeroffsets.at(acluster.layer);
				for(int i=uptubebottomid; i<=uptubebottomid; i++) hpaddleids->Fill(i);
				for(auto&& adigitime : acluster.digittimes) hdigittimes->Fill(adigitime);
			}
			for(auto&& acluster : thetrack.vtrackclusters){
				hpaddleinlayeridsv->Fill(acluster.GetCentreIndex());
				Int_t uptubetopid = (2*acluster.xmaxid) + MRDSpecs::layeroffsets.at(acluster.layer);
				Int_t uptubebottomid = (2*acluster.xminid) + MRDSpecs::layeroffsets.at(acluster.layer);
				for(int i=uptubebottomid; i<=uptubebottomid; i++) hpaddleids->Fill(i);
				for(auto&& adigitime : acluster.digittimes) hdigittimes->Fill(adigitime);
			}
			hnumhcells->Fill(thetrack.htrackcells.size());
			hnumvcells->Fill(thetrack.vtrackcells.size());
			
			hhangle->Fill(thetrack.htrackgradient);
			hhangleerr->Fill(thetrack.htrackgradienterror);
			hvangle->Fill(thetrack.htrackgradient);
			hvangleerr->Fill(thetrack.vtrackgradienterror);
			htotangle->Fill(thetrack.trackangle);
			htotangleerr->Fill(thetrack.trackangleerror);
			henergyloss->Fill(thetrack.EnergyLoss);
			henergylosserr->Fill(thetrack.EnergyLossError);
			htracklength->Fill(thetrack.mutracklengthinMRD);
			htrackpen->Fill(thetrack.penetrationdepth);
			htrackpenvseloss->Fill(thetrack.penetrationdepth,thetrack.EnergyLoss);
			htracklenvseloss->Fill(thetrack.mutracklengthinMRD,thetrack.EnergyLoss);
			
			// N.B. track positions are in cm!
			TVector3* sttv = &thetrack.trackfitstart;
			TVector3* stpv = &thetrack.trackfitstop;
			TVector3* pep = &thetrack.projectedtankexitpoint;
			htrackstart->Fill(sttv->X(),sttv->Z(),sttv->Y());
			htrackstop->Fill(stpv->X(),stpv->Z(),stpv->Y());
			//cout<<"track "<<thetracki<<" started at ("<<sttv->X()<<", "<<sttv->Y()<<", "<<sttv->Z()<<")"
			//	<<" and ended at ("<<stpv->X()<<", "<<stpv->Y()<<", "<<stpv->Z()<<")"<<endl;
			
#ifdef GOT_EVE
			// If gdml track overlay is being drawn, construct the TEveLine for this track
			if(drawGdmlOverlay){
				// ONLY to overlay on gdml plot, we need to shift tracks to the same gdml coordinate system!
				(*sttv) =  TVector3(sttv->X()-buildingoffset.X(),
									sttv->Y()-buildingoffset.Y(),
									sttv->Z()-buildingoffset.Z());
				(*stpv) =  TVector3(stpv->X()-buildingoffset.X(),
									stpv->Y()-buildingoffset.Y(),
									stpv->Z()-buildingoffset.Z());
				(*pep)  =  TVector3( pep->X()-buildingoffset.X(),
									 pep->Y()-buildingoffset.Y(),
									 pep->Z()-buildingoffset.Z());
				
				if(verbosity>2) cout<<"adding track "<<thetracki<<" to event display"<<endl;
				TEveLine* evl = new TEveLine("track1",2);
				evl->SetLineWidth(4);
				evl->SetLineStyle(1);
				evl->SetMarkerColor(kRed);
				evl->SetRnrPoints(kTRUE);  // enable rendering of points
				//int icolour = thetracki;
				//while(icolour>=cMRDSubEvent::trackcolours.size()) icolour-=cMRDSubEvent::trackcolours.size();
				//evl->SetLineColor(cMRDSubEvent::trackcolours.at(icolour));
				evl->SetPoint(0,sttv->X(),sttv->Y(),sttv->Z());
				evl->SetPoint(1,stpv->X(),stpv->Y(),stpv->Z());
				if(!(pep->X()==0&&pep->Y()==0&&pep->Z()==0)){
					evl->SetPoint(2,pep->X(),pep->Y(),pep->Z());
					hpep->Fill(pep->X(),pep->Z(),pep->Y());
					if(verbosity>2) cout<<"back projected point intercepts the tank at ("
						<<pep->X()<<", "<<pep->Y()<<", "<<pep->Z()<<")"<<endl;
					//if(abs(pep->X())>450.) earlyexit=true;
				}
				thiseventstracks.push_back(evl);
				numtracksdrawn++;
				//if(numtracksdrawn>100) earlyexit=true;
				
//				// draw the track -- either need to make trackarrows public,
//				// or make cMRDSubEvent method to call draw a given track... 
//				// or we can draw all tracks via DrawTracks().
//				int trackcolourindex=thetrack.MRDtrackID+1; // element 0 is black
//				while(trackcolourindex+1>=cMRDSubEvent::trackcolours.size()) 
//					trackcolourindex-=cMRDSubEvent::trackcolours.size();
//				EColor thistrackscolour = cMRDSubEvent::trackcolours.at(trackcolourindex);
//				EColor fittrackscolour = cMRDSubEvent::trackcolours.at(trackcolourindex+1);
//				thetrack.DrawReco(thesubevent->imgcanvas, thesubevent->trackarrows, thistrackscolour, thesubevent->paddlepointers);
//				thetrack.DrawFit(thesubevent->imgcanvas, thesubevent->trackfitarrows, fittrackscolour);
			}
#endif
			
			if(earlyexit) break;
		} // end loop over tracks
		
		// if gdml overlay is being drawn, draw it here
#ifdef GOT_EVE
		if(drawGdmlOverlay){
			gdmlcanv->cd();
			for(TEveLine* aline : thiseventstracks){
				//cout<<"drawing track at "<<aline<<" from ("<<aline->GetLineStart().fX
				//<<", "<<aline->GetLineStart().fY<<", "<<aline->GetLineStart().fZ<<") to ("
				//<<aline->GetLineEnd().fX<<", "<<aline->GetLineEnd().fY<<", "<<aline->GetLineEnd().fZ<<")"
				//<<endl;
				aline->Draw();
			}
			gdmlcanv->Update();
			//gEve->Redraw3D(kTRUE);
		}
#endif
		
		if(drawPaddlePlot){
			if(verbosity>2) cout<<"Drawing paddle plot"<<endl;
			thesubevent->DrawMrdCanvases();  // creates the canvas with the digits TODO optimise this
			thesubevent->DrawTracks();       // adds the CA tracks and their fit
			thesubevent->DrawTrueTracks();   // draws true tracks over the event
			
//			// FIXME for some reason when both plots are drawn the Reco Fit arrows aren't shown???
			thesubevent->imgcanvas->Modified();
			for(int subpad=1; subpad<3; subpad++){
				thesubevent->imgcanvas->GetPad(subpad)->Modified();
				thesubevent->imgcanvas->GetPad(subpad)->Update();
			}
//			gSystem->ProcessEvents();

//			for(int subpad=1; subpad<3; subpad++){ // loop over the top and side views
//				TList* imgcanvasprimaries = thesubevent->imgcanvas->GetPad(subpad)->GetListOfPrimitives();
//				//imgcanvasprimaries->ls();
//				TIter primaryiterator(imgcanvasprimaries);
//				int numarrows=0;
//				while(TObject* aprimary = primaryiterator()){
//					TString primarytype = aprimary->ClassName();
//					if(primarytype=="TArrow"){
//						numarrows++;
//					}
//				}
//				cout<<"Found "<<numarrows<<" arrows drawn on the subevent canvas"<<endl;
			
			if(saveimages){
				thesubevent->imgcanvas->SaveAs(TString::Format("%s/checkmrdtracks_%d_%d.png",
												plotDirectory,EventNumber,subevi));
			}
		}
		
		//gSystem->ProcessEvents();
		//gPad->WaitPrimitive();
		//if(enableTApplication) std::this_thread::sleep_for (std::chrono::seconds(2));
		
		//if(earlyexit) break;
	} // end loop over subevents
	
	if(numtracksinev!=numtracksrunningtot){
		cerr<<"number of tracks in event does not correspond to sum of tracks in subevents!"<<endl;
	}
	
	numents++;
	
	return true;
}

bool MrdPaddlePlot::Finalise(){
	
	cout<<"Analysed "<<numents<<" events, found "<<totnumtracks<<" MRD tracks, of which "
		<<numstopped<<" stopped in the MRD, "<<numpenetrated<<" fully penetrated and the remaining "
		<<numsideexit<<" exited the side."<<endl
		<<"Back projection suggests "<<numtankintercepts<<" tracks would have originated in the tank, while "
		<<numtankmisses<<" would not intercept the tank through back projection."<<endl;
	
	// make summary plots
#ifdef GOT_EVE
	if(drawGdmlOverlay){  // TODO separate this from GDML overlay
		// FIXME: Why not being saved???
		std::string imgname;
		mrdTrackCanv->cd();
		hnumsubevs->Draw();
		imgname=hnumsubevs->GetTitle();
		std::replace(imgname.begin(), imgname.end(), ' ', '_');
		mrdTrackCanv->SaveAs(TString::Format("%s/%s.png",plotDirectory,imgname.c_str()));
		hnumtracks->Draw();
		imgname=hnumtracks->GetTitle();
		std::replace(imgname.begin(), imgname.end(), ' ', '_');
		mrdTrackCanv->SaveAs(TString::Format("%s/%s.png",plotDirectory,imgname.c_str()));
		hrun->Draw();
		imgname=hrun->GetTitle();
		std::replace(imgname.begin(), imgname.end(), ' ', '_');
		mrdTrackCanv->SaveAs(TString::Format("%s/%s.png",plotDirectory,imgname.c_str()));
		hevent->Draw();
		imgname=hevent->GetTitle();
		std::replace(imgname.begin(), imgname.end(), ' ', '_');
		mrdTrackCanv->SaveAs(TString::Format("%s/%s.png",plotDirectory,imgname.c_str()));
		hmrdsubev->Draw();
		imgname=hmrdsubev->GetTitle();
		std::replace(imgname.begin(), imgname.end(), ' ', '_');
		mrdTrackCanv->SaveAs(TString::Format("%s/%s.png",plotDirectory,imgname.c_str()));
		htrigger->Draw();
		imgname=htrigger->GetTitle();
		std::replace(imgname.begin(), imgname.end(), ' ', '_');
		mrdTrackCanv->SaveAs(TString::Format("%s/%s.png",plotDirectory,imgname.c_str()));
		hnumhclusters->Draw();
		imgname=hnumhclusters->GetTitle();
		std::replace(imgname.begin(), imgname.end(), ' ', '_');
		mrdTrackCanv->SaveAs(TString::Format("%s/%s.png",plotDirectory,imgname.c_str()));
		hnumvclusters->Draw();
		imgname=hnumvclusters->GetTitle();
		std::replace(imgname.begin(), imgname.end(), ' ', '_');
		mrdTrackCanv->SaveAs(TString::Format("%s/%s.png",plotDirectory,imgname.c_str()));
		hnumhcells->Draw();
		imgname=hnumhcells->GetTitle();
		std::replace(imgname.begin(), imgname.end(), ' ', '_');
		mrdTrackCanv->SaveAs(TString::Format("%s/%s.png",plotDirectory,imgname.c_str()));
		hnumvcells->Draw();
		imgname=hnumvcells->GetTitle();
		std::replace(imgname.begin(), imgname.end(), ' ', '_');
		mrdTrackCanv->SaveAs(TString::Format("%s/%s.png",plotDirectory,imgname.c_str()));
		hpaddleids->Draw();
		imgname=hpaddleids->GetTitle();
		std::replace(imgname.begin(), imgname.end(), ' ', '_');
		mrdTrackCanv->SaveAs(TString::Format("%s/%s.png",plotDirectory,imgname.c_str()));
		hdigittimes->Draw();
		imgname=hdigittimes->GetTitle();
		std::replace(imgname.begin(), imgname.end(), ' ', '_');
		mrdTrackCanv->SaveAs(TString::Format("%s/%s.png",plotDirectory,imgname.c_str()));
		hhangle->Draw();
		imgname=hhangle->GetTitle();
		std::replace(imgname.begin(), imgname.end(), ' ', '_');
		mrdTrackCanv->SaveAs(TString::Format("%s/%s.png",plotDirectory,imgname.c_str()));
		hhangleerr->Draw();
		imgname=hhangleerr->GetTitle();
		std::replace(imgname.begin(), imgname.end(), ' ', '_');
		mrdTrackCanv->SaveAs(TString::Format("%s/%s.png",plotDirectory,imgname.c_str()));
		hvangle->Draw();
		imgname=hvangle->GetTitle();
		std::replace(imgname.begin(), imgname.end(), ' ', '_');
		mrdTrackCanv->SaveAs(TString::Format("%s/%s.png",plotDirectory,imgname.c_str()));
		hvangleerr->Draw();
		imgname=hvangleerr->GetTitle();
		std::replace(imgname.begin(), imgname.end(), ' ', '_');
		mrdTrackCanv->SaveAs(TString::Format("%s/%s.png",plotDirectory,imgname.c_str()));
		htotangle->Draw();
		imgname=htotangle->GetTitle();
		std::replace(imgname.begin(), imgname.end(), ' ', '_');
		mrdTrackCanv->SaveAs(TString::Format("%s/%s.png",plotDirectory,imgname.c_str()));
		htotangleerr->Draw();
		imgname=htotangleerr->GetTitle();
		std::replace(imgname.begin(), imgname.end(), ' ', '_');
		mrdTrackCanv->SaveAs(TString::Format("%s/%s.png",plotDirectory,imgname.c_str()));
		henergyloss->Draw();
		imgname=henergyloss->GetTitle();
		std::replace(imgname.begin(), imgname.end(), ' ', '_');
		mrdTrackCanv->SaveAs(TString::Format("%s/%s.png",plotDirectory,imgname.c_str()));
		henergylosserr->Draw();
		imgname=henergylosserr->GetTitle();
		std::replace(imgname.begin(), imgname.end(), ' ', '_');
		mrdTrackCanv->SaveAs(TString::Format("%s/%s.png",plotDirectory,imgname.c_str()));
		htracklength->Draw();
		imgname=htracklength->GetTitle();
		std::replace(imgname.begin(), imgname.end(), ' ', '_');
		mrdTrackCanv->SaveAs(TString::Format("%s/%s.png",plotDirectory,imgname.c_str()));
		htrackpen->Draw();
		imgname=htrackpen->GetTitle();
		std::replace(imgname.begin(), imgname.end(), ' ', '_');
		mrdTrackCanv->SaveAs(TString::Format("%s/%s.png",plotDirectory,imgname.c_str()));
		htrackpenvseloss->Draw();
		imgname=htrackpenvseloss->GetTitle();
		std::replace(imgname.begin(), imgname.end(), ' ', '_');
		mrdTrackCanv->SaveAs(TString::Format("%s/%s.png",plotDirectory,imgname.c_str()));
		htracklenvseloss->Draw();
		imgname=htracklenvseloss->GetTitle();
		std::replace(imgname.begin(), imgname.end(), ' ', '_');
		mrdTrackCanv->SaveAs(TString::Format("%s/%s.png",plotDirectory,imgname.c_str()));
		// should put these on the same canvas V
		htrackstart->Draw();
		imgname=htrackstart->GetTitle();
		std::replace(imgname.begin(), imgname.end(), ' ', '_');
		mrdTrackCanv->SaveAs(TString::Format("%s/%s.png",plotDirectory,imgname.c_str()));
		htrackstop->Draw();
		imgname=htrackstop->GetTitle();
		std::replace(imgname.begin(), imgname.end(), ' ', '_');
		mrdTrackCanv->SaveAs(TString::Format("%s/%s.png",plotDirectory,imgname.c_str()));
		hpep->Draw();
		imgname=hpep->GetTitle();
		std::replace(imgname.begin(), imgname.end(), ' ', '_');
		mrdTrackCanv->SaveAs(TString::Format("%s/%s.png",plotDirectory,imgname.c_str()));
	}
#endif
	
	if(enableTApplication){
		// cleanup track drawing TApplication
		gSystem->ProcessEvents();
		if(mrdTrackCanv) delete mrdTrackCanv;
		if(gdmlcanv) delete gdmlcanv;
		std::vector<TH1*> histos {hnumsubevs, hnumtracks, hrun, hevent, hmrdsubev, htrigger, hnumhclusters, hnumvclusters, hnumhcells, hnumvcells, hpaddleids, hpaddleinlayeridsh, hpaddleinlayeridsv, hdigittimes, hhangle, hhangleerr, hvangle, hvangleerr, htotangle, htotangleerr, henergyloss, henergylosserr, htracklength, htrackpen, htrackpenvseloss, htracklenvseloss, htrackstart, htrackstop, hpep};
		for(TH1* ahisto : histos){ if(ahisto) delete ahisto; ahisto=0; }
		delete mrdPaddlePlotApp;
	}
	
	return true;
}

//	maybe we can use these:
//	TEveViewer *ev = gEve->GetDefaultViewer();
//	TGLViewer  *gv = ev->GetGLViewer();
//	gv->SetGuideState(TGLUtil::kAxesOrigin, kTRUE, kFALSE, 0);
//	gEve->Redraw3D(kTRUE);
//	gSystem->ProcessEvents();
//	gv->CurrentCamera().RotateRad(-0.5, 1.4);
//	gv->RequestDraw();

//	struct timespec timeOut,remains;
//	timeOut.tv_sec = 0;
//	timeOut.tv_nsec = 500000000; /* 50 milliseconds */
//	nanosleep(&timeOut, &remains);
