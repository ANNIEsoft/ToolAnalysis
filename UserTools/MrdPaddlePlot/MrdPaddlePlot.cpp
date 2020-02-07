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
	
	useTApplication = true;
	plotOnlyTracks = false;

	// get configuration variables for this tool
	m_variables.Get("verbosity",verbosity);
	m_variables.Get("gdmlpath",gdmlpath);
	m_variables.Get("saveimages",saveimages);
	m_variables.Get("saverootfile",saverootfile);
	m_variables.Get("plotDirectory",plotDirectoryString);
	plotDirectory = plotDirectoryString.c_str();
	m_variables.Get("drawPaddlePlot",drawPaddlePlot);
	m_variables.Get("drawGdmlOverlay",drawGdmlOverlay);
	m_variables.Get("drawStatistics",drawStatistics);
	m_variables.Get("printTClonesTracks",printTClonesTracks); // from the FindMrdTracks Tool
	m_variables.Get("useTApplication",useTApplication);
	m_variables.Get("OutputROOTFile",output_rootfile);
	m_variables.Get("PlotOnlyTracks",plotOnlyTracks);

	std::cout <<"plotDirectory: string = "<<plotDirectoryString<<", const char* "<<plotDirectory<<std::endl;
	

	// for gdml overlay
	double buildingoffsetx, buildingoffsety, buildingoffsetz;
	m_variables.Get("buildingoffsetx",buildingoffsetx);
	m_variables.Get("buildingoffsety",buildingoffsety);
	m_variables.Get("buildingoffsetz",buildingoffsetz);
	buildingoffset = Position(buildingoffsetx,buildingoffsety,buildingoffsetz);
	
	//////////////////////////////////////////////////////////////////
	
	if (useTApplication){
		// create the ROOT application to show histograms
		int myargc=0;
		//char *myargv[] = {(const char*)"somestring2"};
		// get or make the TApplication
		intptr_t tapp_ptr=0;
		get_ok = m_data->CStore.Get("RootTApplication",tapp_ptr);
		if(not get_ok){
			if(verbosity>2) cout<<"MrdPaddlePlot Tool: making global TApplication"<<endl;
			rootTApp = new TApplication("rootTApp",&myargc,0);
			tapp_ptr = reinterpret_cast<intptr_t>(rootTApp);
			m_data->CStore.Set("RootTApplication",tapp_ptr);
		} else {
			if(verbosity>2) cout<<"MrdPaddlePlot Tool: Retrieving global TApplication"<<std::endl;
			rootTApp = reinterpret_cast<TApplication*>(tapp_ptr);
		}
		int tapplicationusers;
		get_ok = m_data->CStore.Get("RootTApplicationUsers",tapplicationusers);
		if(not get_ok) tapplicationusers=1;
		else tapplicationusers++;
		m_data->CStore.Set("RootTApplicationUsers",tapplicationusers);
	}
	
	int rootfileusers=0;
	get_ok = m_data->CStore.Get("RootFileUsers",rootfileusers);
	if (get_ok){
		rootfileusers++;
		saverootfile=true;  //already other ROOT files open, need to associate the histograms in this tool with an own root file
		m_data->CStore.Set("RootFileUsers",rootfileusers);
	} else {
		if (saverootfile) {
			rootfileusers=1;
			m_data->CStore.Set("RootFileUsers",rootfileusers);
		}
	}

	if (saverootfile){
		std::stringstream ss_rootfilename;
		ss_rootfilename << plotDirectoryString << "/" << output_rootfile << ".root";
		Log("MrdPaddlePlot tool: Creating root file "+ss_rootfilename.str()+" to save paddle plots.",v_message,verbosity);
		mrdvis_file = new TFile(ss_rootfilename.str().c_str(),"RECREATE");
	}

	if(drawStatistics){
		hnumhclusters = new TH1D("hnumhclusters","Num track clusters in H view",10,0,10);
		hnumvclusters = new TH1D("hnumvclusters","Num track clusters in V view",10,0,10);
		hnumhcells = new TH1D("hnumhcells","Num track cells in H view",10,0,10);
		hnumvcells = new TH1D("hnumvcells","Num track cells in V view",10,0,10);
		hpaddleids = new TH1D("hpaddleids","Hits on Individual Paddles",400,0,400);
		hpaddleinlayeridsh = new TH1D("hpaddleinlayeridsh","Hits on Paddle Positions in H Layers",13,0,13);
		hpaddleinlayeridsv = new TH1D("hpaddleinlayeridsv","Hits on Paddle Positions in V Layers",17,0,17);
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
	
	// to show true tracks on the paddle plot we give hit paddles a border with a colour
	// unique for each true particle
	// But ParticleId_to_MrdTubeIds matches to paddle ChannelKeys, whereas the paddle plot
	// uses WCSim TubeIds. We need to map from one to the other.
	highlight_true_paddles = m_data->CStore.Get("channelkey_to_mrdpmtid",channelkey_to_mrdpmtid);
	
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
	
	// check which subevents had a track
	std::vector<int> track_subevs;
	m_data->CStore.Get("TracksSubEvs",track_subevs);

	if(verbosity>2) cout<<"Event "<<EventNumber<<" had "<<numtracksinev<<" tracks in "<<numsubevs<<" subevents"<<endl;

	std::cout <<"MrdPaddlePlot: Check at start of execute: plotDirectory = "<<plotDirectory<<std::endl;
	
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
	
	if(verbosity>3) cout<<"TClonesArray says it has "<<thesubeventarray->GetEntries()<<" entries"<<endl;
	if(thesubeventarray->GetEntries()!=numsubevs){
		cerr<<"mismatch between MRDTrack TClonesArray size and ANNIEEVENT recorded number of MRD Subevents!"<<endl;
		return false;
	}
	
	// loop over subevents (collections of hits on the MRD within a narrow time window)
	for(int subevi=0; subevi<numsubevs; subevi++){
		if (std::find(track_subevs.begin(),track_subevs.end(),subevi)==track_subevs.end() && plotOnlyTracks){
			Log("MrdPaddlePlot: No tracks in subev "+std::to_string(subevi)+", don't plot MRD paddle plot.",v_message,verbosity);
			continue;
		}
		cMRDSubEvent* thesubevent = (cMRDSubEvent*)thesubeventarray->At(subevi);
		if(printTClonesTracks){
			if(verbosity>3) cout<<"printing subevent "<<subevi<<endl;
			thesubevent->Print();
		}
		
		std::vector<cMRDTrack>* tracksthissubevent = thesubevent->GetTracks();
		int numtracks = tracksthissubevent->size();
		numtracksrunningtot+=numtracks;  // num tracks within all subevents in this event
		
		// loop over tracks within this subevent
		if(verbosity>2) cout<<"scanning "<<numtracks<<" tracks in subevent "<<subevi<<endl;
		for(auto&& thetrack : *tracksthissubevent){
			thetracki++;
			if(printTClonesTracks){
				if(verbosity>3) cout<<"printing next track"<<endl;
				thetrack.Print2(false); // false: do not re-print subevent info
			}
			
			//if(!thetrack.interceptstank) earlyexit=true;
			
			// most properties of the reconstructed tracks are plotted in MrdEfficiency Tool
			// but some a recorded here as the information is more readily accessible in reconstructed
			// track object members (mrdclusters, mrdcells)
			if(drawStatistics){
				hnumhclusters->Fill(thetrack.htrackclusters.size());
				hnumvclusters->Fill(thetrack.vtrackclusters.size());
				// loop over clusters on horizontal paddles
				for(auto&& acluster : thetrack.htrackclusters){
					// record a hit at this position (the cluster centre) within within the layer
					hpaddleinlayeridsh->Fill(acluster.GetCentreIndex());
					// for all paddles within the cluster, record hits on those paddles:
					// 1) get the id of the paddle at the top of the cluster
					Int_t uptubetopid = (2*acluster.xmaxid) + MRDSpecs::layeroffsets.at(acluster.layer);
					// 2) get the id of the paddle at the bottom of the cluster (if it spans multiple paddles)
					Int_t uptubebottomid = (2*acluster.xminid) + MRDSpecs::layeroffsets.at(acluster.layer);
					// 3) scan over the range of paddle ids and record that they were hit
					for(int i=uptubebottomid; i<=uptubebottomid; i++) hpaddleids->Fill(i);
				}
				// repeat above for clusters on vertical paddles
				for(auto&& acluster : thetrack.vtrackclusters){
					hpaddleinlayeridsv->Fill(acluster.GetCentreIndex());
					Int_t uptubetopid = (2*acluster.xmaxid) + MRDSpecs::layeroffsets.at(acluster.layer);
					Int_t uptubebottomid = (2*acluster.xminid) + MRDSpecs::layeroffsets.at(acluster.layer);
					for(int i=uptubebottomid; i<=uptubebottomid; i++) hpaddleids->Fill(i);
				}
				// fill histogram of the number of cells in h/v tracks
				hnumhcells->Fill(thetrack.htrackcells.size());
				hnumvcells->Fill(thetrack.vtrackcells.size());
			}
			
#ifdef GOT_EVE
			if(drawGdmlOverlay){
				// If gdml track overlay is being drawn, construct the TEveLine for this track
				TVector3* sttv = &thetrack.trackfitstart;
				TVector3* stpv = &thetrack.trackfitstop;
				TVector3* pep = &thetrack.projectedtankexitpoint;
				//cout<<"track "<<thetracki<<" started at ("<<sttv->X()<<", "<<sttv->Y()<<", "<<sttv->Z()<<")"
				//	<<" and ended at ("<<stpv->X()<<", "<<stpv->Y()<<", "<<stpv->Z()<<")"<<endl;
			
				// ONLY to overlay on gdml plot, we need to shift tracks to the same gdml coordinate system!
				// N.B. track positions are in cm!
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
			Log("MrdPaddlePlot tool: Draw paddle plot",v_message,verbosity);
			Log("MrdPaddlePlot tool: Drawing MrdCanvases for subevent",v_debug,verbosity);
			thesubevent->DrawMrdCanvases();  // creates the canvas with the digits TODO optimise this
			Log("MrdPaddlePlot tool: Drawing Tracks for subevent",v_debug,verbosity);
			thesubevent->DrawTracks();       // adds the CA tracks and their fit
			Log("MrdPaddlePlot tool: Drawing True Tracks for subevent",v_debug,verbosity);
			thesubevent->DrawTrueTracks();   // draws true tracks over the event
			
			// if we have the truth information, we can highlight paddles that were hit
			// by true particles, to compare the reconstruction
			std::map<int,std::map<unsigned long,double>>* ParticleId_to_MrdTubeIds;
			get_ok = m_data->Stores["ANNIEEvent"]->Get("ParticleId_to_MrdTubeIds", ParticleId_to_MrdTubeIds);
			if(highlight_true_paddles && get_ok){  // if this isn't a simulation chain, we won't have this info
				std::vector<std::vector<int>> paddlesToHighlight;
				for(std::pair<const int,std::map<unsigned long,double>>& aparticle : *ParticleId_to_MrdTubeIds){
					std::map<unsigned long,double>* pmtshit = &aparticle.second;
					std::vector<int> tempvector;
					for(std::pair<const unsigned long,double>& apmt : *pmtshit){
						unsigned long channelkey = apmt.first;
						int pmtsid = channelkey_to_mrdpmtid.at(channelkey);
						tempvector.push_back(pmtsid-1); // -1 to align with MrdTrackLib
					}
					paddlesToHighlight.push_back(tempvector);
				}
				thesubevent->HighlightPaddles(paddlesToHighlight);
			}
			
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
				thesubevent->imgcanvas->SaveAs(TString::Format("%s/%s_%d_%d.png",
												plotDirectory,output_rootfile.c_str(),EventNumber,subevi));
			}
			if (saverootfile){
				mrdvis_file->cd();
				Log("MrdPaddlePlot tool: Saving EvNumber "+std::to_string(EventNumber)+", subevnumber = "+std::to_string(subevi)+" to ROOT-file",v_message,verbosity);
				thesubevent->imgcanvas->SetName(TString::Format("mrdpaddles_ev_%d_%d",EventNumber,subevi));
				thesubevent->imgcanvas->Write();
			}
		}
		
		//gSystem->ProcessEvents();
		//gPad->WaitPrimitive();
		// only need to sleep when using the interactive process
		if (useTApplication) std::this_thread::sleep_for (std::chrono::seconds(2));
		
		//if(earlyexit) break;
	} // end loop over subevents
	
	if(numtracksinev!=numtracksrunningtot){
		cerr<<"number of tracks in event does not correspond to sum of tracks in subevents!"<<endl;
	}

        //only for debugging
        //std::cout <<"MRDPaddlePlot tool: List of objects (End of Execute): "<<std::endl;
        //gObjectTable->Print();
	
	return true;
}

bool MrdPaddlePlot::Finalise(){
	
	// make summary plots
	if(drawStatistics){
		
		// A canvas for other MRD track stats plots
		mrdTrackCanv = new TCanvas("mrdTrackCanv","mrdTrackCanv",canvwidth,canvheight);
		
		std::string imgname;
		mrdTrackCanv->cd();
		hnumhclusters->Draw();
		imgname=hnumhclusters->GetTitle();
		std::replace(imgname.begin(), imgname.end(), ' ', '_');
		
		if (saveimages) mrdTrackCanv->SaveAs(TString::Format("%s/%s.png",plotDirectory,imgname.c_str()));
		if (saverootfile) {
			mrdvis_file->cd();
			hnumhclusters->Write();
		}
		hnumvclusters->Draw();
		imgname=hnumvclusters->GetTitle();
		std::replace(imgname.begin(), imgname.end(), ' ', '_');
		if (saveimages) mrdTrackCanv->SaveAs(TString::Format("%s/%s.png",plotDirectory,imgname.c_str()));
		if (saverootfile) {
			mrdvis_file->cd();
			hnumvclusters->Write();
		}
		hnumhcells->Draw();
		imgname=hnumhcells->GetTitle();
		std::replace(imgname.begin(), imgname.end(), ' ', '_');
		if (saveimages) mrdTrackCanv->SaveAs(TString::Format("%s/%s.png",plotDirectory,imgname.c_str()));
		if (saverootfile) {
			mrdvis_file->cd();
			hnumhcells->Write();
		}
		hnumvcells->Draw();
		imgname=hnumvcells->GetTitle();
		std::replace(imgname.begin(), imgname.end(), ' ', '_');
		if (saveimages) mrdTrackCanv->SaveAs(TString::Format("%s/%s.png",plotDirectory,imgname.c_str()));
		if (saverootfile){
			mrdvis_file->cd();
			hnumvcells->Write();
		}
		hpaddleids->Draw();
		imgname=hpaddleids->GetTitle();
		std::replace(imgname.begin(), imgname.end(), ' ', '_');
		if (saveimages) mrdTrackCanv->SaveAs(TString::Format("%s/%s.png",plotDirectory,imgname.c_str()));
		if (saverootfile){
			mrdvis_file->cd();
			hpaddleids->Write();
		}

		delete mrdTrackCanv;
                mrdTrackCanv=nullptr;
	}
	
	// cleanup
	
	std::vector<TH1*> histos {hnumhclusters, hnumvclusters, hnumhcells, hnumvcells, hpaddleids, hpaddleinlayeridsh, hpaddleinlayeridsv};
	if (!saverootfile) { 
		for(TH1* ahisto : histos){ if(ahisto) delete ahisto; ahisto=0; }
	}
	/*if(gROOT->FindObject("mrdTrackCanv")){
		delete mrdTrackCanv;
		mrdTrackCanv=nullptr;
	}*/
	
	if(drawGdmlOverlay && gdmlcanv) delete gdmlcanv;
	
	if (saverootfile) delete mrdvis_file;

	if (useTApplication){
		int tapplicationusers=0;
		get_ok = m_data->CStore.Get("RootTApplicationUsers",tapplicationusers);
		if(not get_ok || tapplicationusers==1){
			if(rootTApp){
				std::cout<<"MrdPaddlePlot Tool: Deleting global TApplication"<<std::endl;
				//delete rootTApp;
				//rootTApp=nullptr;
			}
		} else if(tapplicationusers>1){
			m_data->CStore.Set("RootTApplicationUsers",tapplicationusers-1);
		}
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
