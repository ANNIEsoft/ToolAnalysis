/* vim:set noexpandtab tabstop=4 wrap */

#include "LoadWCSimLAPPD.h"
#include "LAPPDTree.h"

// for drawing
#include "TROOT.h"
#include "TApplication.h"
#include "TCanvas.h"
#include "TH1.h"
#include "TPolyMarker3D.h"
#include "TSystem.h"

#include <numeric>  // iota

#include <thread>          // std::this_thread::sleep_for
#include <chrono>          // std::chrono::seconds
#include <time.h>          // clock_t, clock, CLOCKS_PER_SEC

LoadWCSimLAPPD::LoadWCSimLAPPD():Tool(){}

bool LoadWCSimLAPPD::Initialise(std::string configfile, DataModel &data){
	
	/////////////////// Useful header ///////////////////////
	
	if(verbose) cout<<"Initializing Tool LoadWCSimLAPPD"<<endl;
	
	if(configfile!="") m_variables.Initialise(configfile); //loading config file
	//m_variables.Print();
	
	m_data= &data; //assigning transient data pointer
	/////////////////////////////////////////////////////////////////
	
	int returnval=0; // All BoostStore 'Get' calls must have their return value checked!!!
	// 0 means the entry was not found, value is NOT set! 1 means OK. No compile time checking!!
	
	// Get the Tool configuration variables
	// ====================================
	m_variables.Get("verbose",verbose);
	//verbose=10;
	m_variables.Get("InputFile",MCFile);
	m_variables.Get("InnerStructureRadius",Rinnerstruct);
	m_variables.Get("DrawDebugGraphs",DEBUG_DRAW_LAPPD_HITS);
	m_variables.Get("FileVersion",FILE_VERSION);
	
	// Make class private members; e.g. the LAPPDTree
	// ==============================================
	if(verbose>2) cout<<"LoadWCSimLAPPD Tool: loading file "<<MCFile<<endl;
	file= new TFile(MCFile.c_str(),"READ");
	lappdtree= (TTree*) file->Get("LAPPDTree");
	NumEvents=lappdtree->GetEntries();
	LAPPDEntry= new LAPPDTree(lappdtree);
	
	// Make the ANNIEEvent Store if it doesn't exist
	// =============================================
	int annieeventexists = m_data->Stores.count("ANNIEEvent");
	if(annieeventexists==0){
		cerr<<"LoadWCSimLAPPD tool need to run after LoadWCSim tool!"<<endl;
		return false; // TODO we need to read various things from main the files...
	}

	// Get trigger window parameters from CStore
	// =========================================
	m_data->CStore.Get("WCSimPreTriggerWindow",pretriggerwindow);
	m_data->CStore.Get("WCSimPostTriggerWindow",posttriggerwindow);
	if(verbose>2) cout<<"WCSimPreTriggerWindow="<<pretriggerwindow
					  <<", WCSimPostTriggerWindow="<<posttriggerwindow<<endl;
	// so far all simulations have used the SK defaults: pre: -400ns, post: 950ns
	
	// Get WCSimRootGeom so we can obtain CylLoc for calculating global pos
	// ====================================================================
	intptr_t geomptr;
	returnval = m_data->CStore.Get("WCSimRootGeom",geomptr);
	geo = reinterpret_cast<WCSimRootGeom*>(geomptr);
//	int wcsimgoemexists = m_data->Stores.count("WCSimRootGeomStore");
//	if(wcsimgoemexists==0){
//		m_data->Stores.at("WCSimRootGeomStore")->Header->Get("WCSimRootGeom",geo);
//	} else {
//		cerr<<"No WCSimRootGeom needed by LoadWCSimLAPPD tool!"<<endl;
//	}
	if(verbose>3){
		cout<<"Retrieved WCSimRootGeom from geo="<<geo<<", geomptr="<<geomptr<<endl;
		int numlappds = geo->GetWCNumLAPPD();
		cout<<"we have "<<numlappds<<" LAPPDs"<<endl;
	}
	
	// things to be saved to the ANNIEEvent Store
	MCLAPPDHits = new std::map<ChannelKey,std::vector<LAPPDHit>>;
	
	if(DEBUG_DRAW_LAPPD_HITS){
		// create the ROOT application to show histograms
		int myargc=0;
		char *myargv[] = {(const char*)"somestring"};
		lappdRootDrawApp = new TApplication("lappdRootDrawApp",&myargc,myargv);
		lappdhitshist = new TPolyMarker3D();
		digixpos = new TH1D("digixpos","digixpos",100,-2,2);
		digiypos = new TH1D("digiypos","digiypos",100,-2.5,2.5);
		digizpos = new TH1D("digizpos","digizpos",100,0.,4.);
		digits = new TH1D("digits","digits",100,-50,100);
	}
	
	return true;
}


bool LoadWCSimLAPPD::Execute(){
	
	// WCSim splits "events" (a set of G4Primaries) into "triggers" (a set of time-distinct tank events)
	// To match real data, we flatten events out, creating one ToolAnalysis event per trigger.
	// But LAPPD hits are truth info and aren't tied to a WCSim trigger;
	// we need to scan through the LAPPD hits and check for hits within the trigger time window.
	// If you wish, you can bypass this easily enough, but in that case bear in mind the same
	// LAPPD hits may appear in multiple ToolAnalysis 'events' unless you otherwise handle it.
	if(verbose) cout<<"Executing tool LoadWCSimLAPPD"<<endl;
	
	int storegetret=666; // return 1 success
	storegetret = m_data->Stores.at("ANNIEEvent")->Get("MCEventNum",MCEventNum);
	storegetret = m_data->Stores.at("ANNIEEvent")->Get("MCTriggernum",MCTriggernum);
	storegetret = m_data->Stores.at("ANNIEEvent")->Get("EventTime",EventTime);
	if(EventTime==nullptr){
		cerr<<"Failed to load WCSim trigger time with code "<<storegetret<<"!"<<endl;
		return false;
	}
	
	double wcsimtriggertime = static_cast<double>(EventTime->GetNs()); // returns a uint64_t
	if(verbose>2) cout<<"LAPPDTool: Trigger time for event "<<MCEventNum<<", trigger "
					  <<MCTriggernum<<" was "<<wcsimtriggertime<<"ns"<<endl;
	
	if(MCTriggernum>0){
		if(unassignedhits.size()==0){
			if(verbose>2) cout<<"no LAPPD hits to add to this trigger"<<endl;
			return true;
		} else {
			if(verbose>2) cout<<"looping over "<<unassignedhits.size()
							  <<" LAPPD hits that weren't in the first trigger window"<<endl;
			for(int hiti=0; hiti<unassignedhits.size(); hiti++){
				LAPPDHit nexthit = unassignedhits.at(hiti);
				double relativedigitst = nexthit.GetTime(); // relative to Trigger time
				if( (relativedigitst)>(pretriggerwindow) &&
					(relativedigitst)<(posttriggerwindow) ){
					// this lappd hit is within the trigger window; note it
					ChannelKey key(subdetector::LAPPD,nexthit.GetTubeId());
					if(MCLAPPDHits->count(key)==0) MCLAPPDHits->emplace(key, std::vector<LAPPDHit>{nexthit});
					else MCLAPPDHits->at(key).push_back(nexthit);
					if(verbose>3) cout<<"new lappd digit added"<<endl;
				}
			}
		}
	} else {  // MCTriggernum == 0
		MCLAPPDHits->clear();
		if(verbose>3) cout<<"loading LAPPDEntry"<<MCEventNum<<endl;
		LAPPDEntry->GetEntry(MCEventNum);
		
		/////////////////////////////////
		// Scan through all lappd hits in the event
		// Create LAPPDHit objects for those within the trigger window
		// Store the results of calculations temporarily for future triggers in this event
		/////////////////////////////////
		if(verbose>2) cout<<"There were "<<LAPPDEntry->lappd_numhits<<" LAPPDs hit in this event"<<endl;
		int runningcount=0;
		for(int lappdi=0; lappdi<LAPPDEntry->lappd_numhits; lappdi++){
			// loop over LAPPDs that had at least one hit
			int LAPPDID = LAPPDEntry->lappdhit_objnum[lappdi];
			double pmtx = (LAPPDEntry->lappdhit_x[lappdi]) / 1000.;  // pos of LAPPD in global coords, [mm] to [m]
			double pmty = (LAPPDEntry->lappdhit_y[lappdi]) / 1000.;
			double pmtz = (LAPPDEntry->lappdhit_z[lappdi]) / 1000.;
			
			double tileangle;
			int theoctagonside;
			int lappdloc;
			if(FILE_VERSION<3){  // manual calculation of global hit position part 1
				WCSimRootPMT lappdobj = geo->GetLAPPD(LAPPDID-1);
				lappdloc = lappdobj.GetCylLoc();
				double Rthresh=Rinnerstruct*pow(2.,-0.5);
				switch (lappdloc){
					case 0: break; // top cap
					case 2: break; // bottom cap
					case 1: // wall
						// we need to account for the angle of the LAPPD within the tank
						// determine the angle based on it's position
						double octangle1=TMath::Pi()*(3./8.);
						double octangle2=TMath::Pi()*(1./8.);
						double pmtztankorigin = pmtz -((MRDSpecs::tank_start+MRDSpecs::tank_radius)/100.);
							 if(pmtx<-Rthresh&&pmtztankorigin<0)         {tileangle=-octangle1; theoctagonside=0;}
						else if(-Rthresh<pmtx&&pmtx<0&&pmtztankorigin<0) {tileangle=-octangle2; theoctagonside=1;}
						else if(0<pmtx&&pmtx<Rthresh&&pmtztankorigin<0)  {tileangle= octangle2; theoctagonside=2;}
						else if(Rthresh<pmtx&&pmtztankorigin<0)          {tileangle= octangle1; theoctagonside=3;}
						else if(pmtx<-Rthresh&&pmtztankorigin>0)         {tileangle= octangle1; theoctagonside=4;}
						else if(-Rthresh<pmtx&&pmtx<0&&pmtztankorigin>0) {tileangle= octangle2; theoctagonside=5;}
						else if(0<pmtx&&pmtx<Rthresh&&pmtztankorigin>0)  {tileangle=-octangle2; theoctagonside=6;}
						else if(Rthresh<pmtx&&pmtztankorigin>0)          {tileangle=-octangle1; theoctagonside=7;}
						break;
				}
			} // FILE_VERSION<3
			
			int numhitsthislappd=LAPPDEntry->lappdhit_edep[lappdi];
			if(verbose>3) cout<<"LAPPD "<<LAPPDID<<" had "<<numhitsthislappd<<" hits in total"<<endl;
			int lastrunningcount=runningcount;
			// loop over all the hits on this lappd
			for(;runningcount<(lastrunningcount+numhitsthislappd); runningcount++){
				double peposx = (LAPPDEntry->lappdhit_stripcoorx->at(runningcount)) / 1000.;  // pos on tile
				double peposy = (LAPPDEntry->lappdhit_stripcoory->at(runningcount)) / 1000.;  // [mm] to [m]
				
				double digitsx, digitsy, digitsz;
				if(FILE_VERSION<3){     // manual calculation of global hit position part 2
					switch (lappdloc){
						case 0: // top cap
							digitsx  = pmtx - peposx;
							digitsy  = pmty;
							digitsz  = pmtz - peposy;
							break;
						case 2: // bottom cap
							digitsx  = pmtx + peposx;
							digitsy  = pmty;
							digitsz  = pmtz - peposy;
							break;
						case 1: // wall
							digitsy  = pmty + peposx;
							if(theoctagonside<4){
									digitsx  = pmtx - peposy*TMath::Cos(tileangle);
									digitsz  = pmtz - peposy*TMath::Sin(tileangle);
							} else {
									digitsx  = pmtx + peposy*TMath::Cos(tileangle);
									digitsz  = pmtz + peposy*TMath::Sin(tileangle);
							}
					}
				} else {  // if FILE_VERSION>=3
					digitsx= (LAPPDEntry->lappdhit_globalcoorx->at(runningcount)) / 1000.;  // global WCSim coords
					digitsy= (LAPPDEntry->lappdhit_globalcoory->at(runningcount)) / 1000.;  //   [mm] to [m]
					digitsz= (LAPPDEntry->lappdhit_globalcoorz->at(runningcount)) / 1000.;
				} // if FILE_VERSION<3
				
				// calculate relative time within trigger
				double digitst  = LAPPDEntry->lappdhit_stripcoort->at(runningcount);
				double relativedigitst=digitst-wcsimtriggertime;
				
				float digiq = 0; // N/A
				ChannelKey key(subdetector::LAPPD,LAPPDID);
				std::vector<double> globalpos{digitsx,digitsy,digitsz};
				std::vector<double> localpos{peposx, peposy};
				//LAPPDHit nexthit(LAPPDID, digitst, digiq, globalpos, localpos);
				if(DEBUG_DRAW_LAPPD_HITS){
					lappdhitshist->SetNextPoint(digitsx,digitsz, digitsy);
					digixpos->Fill(digitsx);
					digiypos->Fill(digitsy);
					digizpos->Fill(digitsz);
					digits->Fill(relativedigitst);
				}
				
				// we now have all the necessary info about this LAPPD hit:
				// check if it falls within the current trigger window
				if(verbose>4){
					cout<<"LAPPD hit time is "<<relativedigitst<<" [ns], triggertime is "<<wcsimtriggertime
						<<", giving window ("<<(wcsimtriggertime+pretriggerwindow)
						<<"), ("<<(wcsimtriggertime+posttriggerwindow)<<")"<<endl;
				}
				if( relativedigitst>(pretriggerwindow) && 
					relativedigitst<(posttriggerwindow) ){
					if(MCLAPPDHits->count(key)==0){
						LAPPDHit nexthit(LAPPDID, relativedigitst, digiq, globalpos, localpos);
						MCLAPPDHits->emplace(key, std::vector<LAPPDHit>{nexthit});
					} else {
						MCLAPPDHits->at(key).emplace_back(LAPPDID, relativedigitst, digiq,
															globalpos, localpos);
					}
					if(verbose>3) cout<<"new lappd digit added"<<endl;
				} else { // store it for checking against future triggers in this event
					unassignedhits.emplace_back(LAPPDID, relativedigitst, digiq, globalpos, localpos);
				}
			} // end loop over photons on this lappd
		}     // end loop over lappds hit in this event
	}         // end if MCTriggernum == 0
	
	if(verbose>3) cout<<"Saving MCLAPPDHits to ANNIEEvent"<<endl;
	m_data->Stores.at("ANNIEEvent")->Set("MCLAPPDHits",MCLAPPDHits,true);
	
	return true;
}


bool LoadWCSimLAPPD::Finalise(){
	
	if(DEBUG_DRAW_LAPPD_HITS){
		Double_t canvwidth = 700;
		Double_t canvheight = 600;
		lappdRootCanvas = new TCanvas("lappdRootCanvas","lappdRootCanvas",canvwidth,canvheight);
		lappdRootCanvas->SetWindowSize(canvwidth,canvheight);
		lappdRootCanvas->cd();
		
		digixpos->Draw();
		lappdRootCanvas->Update();
		lappdRootCanvas->SaveAs("digixpos.png");
		
		digiypos->Draw();
		lappdRootCanvas->Update();
		lappdRootCanvas->SaveAs("digiypos.png");
		
		digizpos->Draw();
		lappdRootCanvas->Update();
		lappdRootCanvas->SaveAs("digizpos.png");
		
		digits->Draw();
		lappdRootCanvas->Update();
		lappdRootCanvas->SaveAs("digits.png");
		
		lappdRootCanvas->Clear();
		gStyle->SetOptStat(0);
		// need to create axes to add to the TPolyMarker3D
		TH3F *frame3d = new TH3F("frame3d","frame3d",10,-1.5,1.5,10,0,3.3,10,-2.5,2.5);
		frame3d->Draw();
		
		lappdhitshist->Draw();
		frame3d->SetTitle("LAPPD Hits - Isometric View");
		lappdRootCanvas->Modified();
		lappdRootCanvas->Update();
		lappdRootCanvas->SaveAs("lappdhits_isometricview.png");
		
		frame3d->SetTitle("LAPPD Hits - Top View");
		frame3d->GetXaxis()->SetLabelOffset(-0.1);
		lappdRootCanvas->SetPhi(0);
		lappdRootCanvas->SetTheta(90);
		lappdRootCanvas->Modified();
		lappdRootCanvas->Update();
		lappdRootCanvas->SaveAs("lappdhits_topview.png");
		
		lappdRootCanvas->SetWindowSize(canvheight, canvwidth);
		frame3d->SetTitle("LAPPD Hits - Side View");
		//frame3d->GetXaxis()->SetLabelOffset(-0.1);
		lappdRootCanvas->SetPhi(90);
		lappdRootCanvas->SetTheta(0.); //-(45./2.)
		lappdRootCanvas->Modified();
		lappdRootCanvas->Update();
		lappdRootCanvas->SaveAs("lappdhits_sideview.png");
		gSystem->ProcessEvents();
		//lappdRootDrawApp->Run();
		//std::this_thread::sleep_for (std::chrono::seconds(5));
		//lappdRootDrawApp->Terminate(0);
		
		delete digixpos;
		delete digiypos;
		delete digizpos;
		delete frame3d;
		delete lappdhitshist;
		delete lappdRootCanvas;
		delete lappdRootDrawApp;
	}
	
	return true;
}
