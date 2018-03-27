/* vim:set noexpandtab tabstop=4 wrap */
#include "MrdPaddlePlot.h"
#include "TCanvas.h"
#include "TGeoManager.h"
#include "TEveLine.h"
 #include "TGLViewer.h"
#include <thread>         // std::this_thread::sleep_for
#include <chrono>         // std::chrono::seconds
#include <time.h>         // clock_t, clock, CLOCKS_PER_SEC

MrdPaddlePlot::MrdPaddlePlot():Tool(){}

bool MrdPaddlePlot::Initialise(std::string configfile, DataModel &data){

	if(verbose) cout<<"Initializing tool MrdPaddlePlot"<<endl;

	/////////////////// Usefull header ///////////////////////
	if(configfile!="")  m_variables.Initialise(configfile); //loading config file
	//m_variables.Print();

	m_data= &data; //assigning transient data pointer
	/////////////////////////////////////////////////////////////////
	
	// get configuration variables for this tool
	m_variables.Get("verbose",verbose);
	m_variables.Get("gdmlpath",gdmlpath);
	m_variables.Get("saveimages",saveimages);
	
	//////////////////////////////////////////////////////////////////
	
	numents=0;    // number of events (~triggers) analysed
	totnumtracks=0;
	numstopped=0;
	numpenetrated=0;
	numsideexit=0;
	numtankintercepts=0;
	numtankmisses=0;
	
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

	
	// Import the gdml geometry for the detector:
//	TGeoManager::Import(gdmlpath.c_str());
//	// make all the materials semi-transparent so we can see the tracks going through them:
//	TList* matlist = gGeoManager->GetListOfMaterials();
//	TIter nextmaterial(matlist);
//	while(TGeoMixture* amaterial=(TGeoMixture*)nextmaterial()) amaterial->SetTransparency(90);
//	cout<<"drawing top volume"<<endl;
//	gGeoManager->GetVolume("WORLD2_LV")->Draw("ogl");
//	gdmlcanv = (TCanvas*)gROOT->FindObject("c1");
	
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
	
	if(verbose) cout<<"Tool MrdPaddlePlot updating histograms"<<endl;
	
	// Get the ANNIE event and extract information
	// Maybe this information could be added to plots for reference
	m_data->Stores["ANNIEEvent"]->Get("MCFile",MCFile);
	m_data->Stores["ANNIEEvent"]->Get("RunNumber",RunNumber);
	m_data->Stores["ANNIEEvent"]->Get("SubrunNumber",SubrunNumber);
	m_data->Stores["ANNIEEvent"]->Get("EventNumber",EventNumber);
	m_data->Stores["ANNIEEvent"]->Get("TriggerNumber",MCTriggernum);
	m_data->Stores["ANNIEEvent"]->Get("MCEventNum",MCEventNum);
	
	// TODO: figure out how to number with 2 different event numberings 
	// (event / mcevent+trigger+mrdsubevent...)
	
	// Since the TClonesArray isn't cleared (tracks aren't deleted) between fillings
	// How do we know how many tracks were updated as they were in this event?
	// Perhaps the user needs to store it separately?
	m_data->Stores["MRDTracks"]->Get("NumMrdSubEvents",numsubevs);
	m_data->Stores["MRDTracks"]->Get("NumMrdTracks",numtracksinev);
	
//	// a version to plot tracks retrieved from the BoostStore would have only one level
//	// as tracks are not nested within subevents
//	for(int tracki=0; tracki<numtracksinev; tracki++){
//		
//		// Get the track details from the BoostStore
//		m_data->Stores["MRDTracks"]->Get("MRDTrackID",MRDTrackID);
//		m_data->Stores["MRDTracks"]->Get("MrdSubEventID",MrdSubEventID);
//		m_data->Stores["MRDTracks"]->Get("InterceptsTank",InterceptsTank);
//		m_data->Stores["MRDTracks"]->Get("StartTime",StartTime);
//		m_data->Stores["MRDTracks"]->Get("StartVertex",StartVertex);
//		m_data->Stores["MRDTracks"]->Get("StopVertex",StopVertex);
//		m_data->Stores["MRDTracks"]->Get("TrackAngle",TrackAngle);
//		m_data->Stores["MRDTracks"]->Get("TrackAngleError",TrackAngleError);
//		m_data->Stores["MRDTracks"]->Get("LayersHit",LayersHit);
//		m_data->Stores["MRDTracks"]->Get("TrackLength",TrackLength);
//		m_data->Stores["MRDTracks"]->Get("IsMrdPenetrating",IsMrdPenetrating);
//		m_data->Stores["MRDTracks"]->Get("IsMrdStopped",IsMrdStopped);
//		m_data->Stores["MRDTracks"]->Get("IsMrdSideExit",IsMrdSideExit);
//		m_data->Stores["MRDTracks"]->Get("PenetrationDepth",PenetrationDepth);
//		m_data->Stores["MRDTracks"]->Get("HtrackFitChi2",HtrackFitChi2);
//		m_data->Stores["MRDTracks"]->Get("HtrackFitCov",HtrackFitCov);
//		m_data->Stores["MRDTracks"]->Get("VtrackFitChi2",VtrackFitChi2);
//		m_data->Stores["MRDTracks"]->Get("VtrackFitCov",VtrackFitCov);
//		m_data->Stores["MRDTracks"]->Get("PMTsHit",PMTsHit);
//		
//		// Plot the track
//		// todoooooooo
//		// . . .
//	}
	
	
//	maybe we can use these:
//	TEveViewer *ev = gEve->GetDefaultViewer();
//	TGLViewer  *gv = ev->GetGLViewer();
//	gv->SetGuideState(TGLUtil::kAxesOrigin, kTRUE, kFALSE, 0);
//	gEve->Redraw3D(kTRUE);
//	gSystem->ProcessEvents();
//	gv->CurrentCamera().RotateRad(-0.5, 1.4);
//	gv->RequestDraw();
	
	// ##############################################################################
	// Actual drawing code
	// This works entirely with the TClonesArray created by FindMrdTracks Tool
	// (tracks are not retrieved from the BoostStore)
	// ##############################################################################
	
	std::vector<TEveLine*> thiseventstracks;
	// loop over events
	int numtracksdrawn=0;
	bool earlyexit=false; // could be used to trigger interesting events
	
	hnumsubevs->Fill(numsubevs);
	int numtracksrunningtot=0;
	
	int thetracki=-1;
	// loop over subevents (collections of hits on the MRD within a narrow time window)
	cout<<"scanning "<<numsubevs<<" subevents in event "<<EventNumber<<endl;
	cout<<"thesubeventarray="<<thesubeventarray<<endl;
	cout<<"thesubeventarray TClonesArray says it has "<<thesubeventarray->GetEntries()<<" entries"<<endl;
	if(thesubeventarray->GetEntries()!=numsubevs){
		cerr<<"mismatch between MRDTrack TClonesArray size and ANNIEEVENT recorded number of MRD Subevents!"<<endl;
		return false;
	}
	for(int subevi=0; subevi<numsubevs; subevi++){
		cMRDSubEvent* thesubevent = (cMRDSubEvent*)thesubeventarray->At(subevi);
		cout<<"printing subevent "<<subevi<<endl;
		thesubevent->Print();
		
		std::vector<cMRDTrack>* tracksthissubevent = thesubevent->GetTracks();
		// there are no subevents (since we can't nest boost stores
		// so we just retrieve next track and note when it's subeventid changes
		int numtracks = tracksthissubevent->size();
		hnumtracks->Fill(numtracks);
		numtracksrunningtot+=numtracks;
		totnumtracks+=numtracks;
		//cout<<"event "<<evi<<", subevent "<<subevi<<" had "<<numtracks<<" tracks"<<endl;
		// loop over tracks (collections of hits in a line within a subevent)
		cout<<"scanning "<<numtracks<<" tracks in subevent "<<subevi<<endl;
		for(auto&& thetrack : *tracksthissubevent){
			thetracki++;
			cout<<"printing next track"<<endl;
			thetrack.Print2();
			
			if(thetrack.ispenetrating) numpenetrated++;
			else if(thetrack.isstopped) numstopped++;
			else numsideexit++;
			if(thetrack.interceptstank) numtankintercepts++;
			else numtankmisses++;
			if(!thetrack.interceptstank) earlyexit=true;
			
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
			
			TVector3* sttv = &thetrack.trackfitstart;
			TVector3* stpv = &thetrack.trackfitstop;
			TVector3* pep = &thetrack.projectedtankexitpoint;
			htrackstart->Fill(sttv->X(),sttv->Z(),sttv->Y());
			htrackstop->Fill(stpv->X(),stpv->Z(),stpv->Y());
			//cout<<"track "<<thetracki<<" started at ("<<sttv->X()<<", "<<sttv->Y()<<", "<<sttv->Z()<<")"
			//	<<" and ended at ("<<stpv->X()<<", "<<stpv->Y()<<", "<<stpv->Z()<<")"<<endl;
			
			cout<<"adding track to event display"<<endl;
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
				cout<<"back projected point intercepts the tank at ("
					<<pep->X()<<", "<<pep->Y()<<", "<<pep->Z()<<")"<<endl;
				if(abs(pep->X())>450.) earlyexit=true;
			}
			thiseventstracks.push_back(evl);
			numtracksdrawn++;
			//cout<<"eveline constructed at "<<evl<<endl;
			//if(numtracksdrawn>100) earlyexit=true;
			
			
			/*
			// draw the track
			int trackcolourindex=thetrack.MRDtrackID+1; // element 0 is black
			while(trackcolourindex+1>=cMRDSubEvent::trackcolours.size()) 
				trackcolourindex-=cMRDSubEvent::trackcolours.size();
			EColor thistrackscolour = cMRDSubEvent::trackcolours.at(trackcolourindex);
			EColor fittrackscolour = cMRDSubEvent::trackcolours.at(trackcolourindex+1);
			thetrack->DrawReco(imgcanvas, trackarrows, thistrackscolour, paddlepointers);
			thetrack->DrawFit(imgcanvas, trackfitarrows, fittrackscolour);
			*/
			if(earlyexit) break;
		} // end loop over tracks
		
//		gdmlcanv->cd();
//		cout<<"drawing event display"<<endl;
//		for(auto&& aline : thiseventstracks){
//			//cout<<"drawing track at "<<aline<<" from ("<<aline->GetLineStart().fX
//			//<<", "<<aline->GetLineStart().fY<<", "<<aline->GetLineStart().fZ<<") to ("
//			//<<aline->GetLineEnd().fX<<", "<<aline->GetLineEnd().fY<<", "<<aline->GetLineEnd().fZ<<")"
//			//<<endl;
//			aline->Draw();
//		}
		
		
		thesubevent->DrawMrdCanvases();  // creates the canvas with the digits
		thesubevent->DrawTracks();       // adds the CA tracks and their fit
		thesubevent->DrawTrueTracks();   // draws true tracks over the event
		if(saveimages) 
			thesubevent->imgcanvas->SaveAs(TString::Format("checkmrdtracks_%d_%d.png",EventNumber,subevi));
		
		thesubevent->RemoveArrows();
		//if(earlyexit) break;
	} // end loop over subevents
	
	if(numtracksinev!=numtracksrunningtot){
		cerr<<"number of tracks in event does not correspond to sum of tracks in subevents!"<<endl;
	}
	
//	gdmlcanv->Update();
	
	c2.cd();
	hnumsubevs->Draw();
	c3.cd();
	hnumtracks->Draw();
	c4.cd();
	hrun->Draw();
	c5.cd();
	hevent->Draw();
	c6.cd();
	hmrdsubev->Draw();
	c7.cd();
	htrigger->Draw();
	c8.cd();
	hnumhclusters->Draw();
	c9.cd();
	hnumvclusters->Draw();
	c10.cd();
	hnumhcells->Draw();
	c11.cd();
	hnumvcells->Draw();
	c12.cd();
	hpaddleids->Draw();
	c13.cd();
	hdigittimes->Draw();
	c14.cd();
	hhangle->Draw();
	c15.cd();
	hhangleerr->Draw();
	c16.cd();
	hvangle->Draw();
	c17.cd();
	hvangleerr->Draw();
	c18.cd();
	htotangle->Draw();
	c19.cd();
	htotangleerr->Draw();
	c20.cd();
	henergyloss->Draw();
	c21.cd();
	henergylosserr->Draw();
	c22.cd();
	htracklength->Draw();
	c23.cd();
	htrackpen->Draw();
	c24.cd();
	htrackpenvseloss->Draw();
	c25.cd();
	htracklenvseloss->Draw();
	// should put these on the same canvas V
	c26.cd();
	htrackstart->Draw();
	c27.cd();
	htrackstop->Draw();
	c28.cd();
	hpep->Draw();
	
	//gPad->WaitPrimitive();
	//std::this_thread::sleep_for (std::chrono::seconds(5));
	
//	struct timespec timeOut,remains;
//	
//	timeOut.tv_sec = 0;
//	timeOut.tv_nsec = 500000000; /* 50 milliseconds */
//	nanosleep(&timeOut, &remains);
	
	numents++;
	
	return true;
}


bool MrdPaddlePlot::Finalise(){
	
	cout<<"Analysed "<<numents<<" events, found "<<totnumtracks<<" MRD tracks, of which "
		<<numstopped<<" stopped in the MRD, "<<numpenetrated<<" fully penetrated and the remaining "
		<<numsideexit<<" exited the side."<<endl
		<<"Back projection suggests "<<numtankintercepts<<" tracks would have originated in the tank, while "
		<<numtankmisses<<" would not intercept the tank through back projection."<<endl;
	
	return true;
}
