/* vim:set noexpandtab tabstop=4 wrap */
#include "FindMrdTracks.h"
#include <numeric>      // std::iota
// for drawing
#include "TROOT.h"
#include "TApplication.h"
#include "TCanvas.h"
#include "TH1.h"
#include "TSystem.h"
// for sleeping
#include <thread>          // std::this_thread::sleep_for
#include <chrono>          // std::chrono::seconds

FindMrdTracks::FindMrdTracks():Tool(){}

bool FindMrdTracks::Initialise(std::string configfile, DataModel &data){
	
	if(verbosity) cout<<"Initializing tool FindMrdTracks"<<endl;
	
	/////////////////// Useful header ///////////////////////
	if(configfile!="") m_variables.Initialise(configfile); //loading config file
	//m_variables.Print();
	
	m_data= &data; //assigning transient data pointer
	/////////////////////////////////////////////////////////////////
	
	// get configuration variables for this tool
	m_variables.Get("OutputDirectory",outputdir);
	m_variables.Get("verbosity",verbosity);
	m_variables.Get("MinDigitsForTrack",minimumdigits);
	m_variables.Get("MaxMrdSubEventDuration",maxsubeventduration);
	m_variables.Get("MinSubeventTimeSep",minimum_subevent_timeseparation);
	m_variables.Get("WriteTracksToFile",writefile);
	m_variables.Get("DrawTruthTracks",DrawTruthTracks);
	m_variables.Get("MakeMrdDigitTimePlot",MakeMrdDigitTimePlot);
	
	// create a store for holding MRD tracks to pass to downstream Tools
	// will be a single entry BoostStore containing a vector of single entry BoostStores
	m_data->Stores["MRDTracks"] = new BoostStore(true,0);
	// the vector of MRD tracks found in the current event
	theMrdTracks = new std::vector<BoostStore>(5);
	
	m_data->Stores["ANNIEEvent"]->Header->Get("AnnieGeometry",geo);
	//numvetopmts = geo->GetNumVetoPMTs();
	
	// create clonesarray for storing the MRD Track details as they're found
	if(SubEventArray==nullptr) SubEventArray = new TClonesArray("cMRDSubEvent");  // string is class name
	// put the pointer in the CStore, so it can be retrieved by MrdTrackPlotter tool Init
	intptr_t subevptr = reinterpret_cast<intptr_t>(SubEventArray);
	m_data->CStore.Set("MrdSubEventTClonesArray",subevptr);
	m_data->CStore.Get("channelkey_to_mrdpmtid",channelkey_to_mrdpmtid);
	// pass this to TrackCombiner
	m_data->CStore.Set("DrawMrdTruthTracks",DrawTruthTracks);
	
	if(MakeMrdDigitTimePlot){
		// Make the TApplication
		// ======================
		// If we wish to show the histograms during running, we need a TApplication
		// There may only be one TApplication, so if another tool has already made one
		// register ourself as a user. Otherwise, make one and put a pointer in the CStore for other Tools
		// create the ROOT application to show histograms
		int myargc=0;
		//char *myargv[] = {(const char*)"Ahh shark!"};
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
		
		mrddigitts = new TH1D("mrddigitts","mrd digit times",500,-50,2000);
		canvwidth = 900;
		canvheight = 600;
	}
	
	return true;
}


bool FindMrdTracks::Execute(){
	
	if(verbosity) cout<<"Tool FindMrdTracks finding tracks in next event."<<endl;
	
	// ensure the previous tracks are cleared so we don't have any carry over
	SubEventArray->Clear("C");
	//m_data->Stores["MRDTracks"]->Delete(); // delete the previous vector of tracks - just clear and refill
	m_data->Stores["MRDTracks"]->Set("NumMrdSubEvents",0);
	m_data->Stores["MRDTracks"]->Set("NumMrdTracks",0);
	
	int lastrunnum=runnum;
	int lastsubrunnum=subrunnum;
	
	// Get the ANNIE event and extract information
	if(verbosity>4) cout<<"FindMrdTracks getting event info from store"<<endl;
	m_data->Stores["ANNIEEvent"]->Get("MCFile",MCFile);
	m_data->Stores["ANNIEEvent"]->Get("RunNumber",RunNumber);
	m_data->Stores["ANNIEEvent"]->Get("SubrunNumber",SubrunNumber);
	m_data->Stores["ANNIEEvent"]->Get("EventNumber",EventNumber);
	m_data->Stores["ANNIEEvent"]->Get("TriggerNumber",MCTriggernum);
	m_data->Stores["ANNIEEvent"]->Get("MCEventNum",MCEventNum);
	m_data->Stores["ANNIEEvent"]->Get("TDCData",TDCData);  // a std::map<unsigned long,vector<TDCHit>>
	m_data->Stores["ANNIEEvent"]->Get("MCParticles",MCParticles);
	
	// FIXME align types until we update MRDTrackClass/MRDSubEventClass
	currentfilestring=MCFile;
	runnum=(int)RunNumber;
	subrunnum=(int)SubrunNumber;
	eventnum=(int)EventNumber;
	triggernum=(int)MCTriggernum;
	// TODO: add MCEventNum to MRDTrackClass/MRDSubEventClass
	
	// make the tree if we want to save the Tracks to file
	if( writefile && (mrdtrackfile==nullptr || (lastrunnum!=runnum) || (lastsubrunnum!=subrunnum)) ){
		if(verbosity>2) cout<<"creating new file to write MRD tracks to"<<endl;
		StartNewFile();  // open a new file for each run / subrun
	}
	
	// extract the digits from the annieevent and put them into separate vectors used by the track finder
	mrddigittimesthisevent.clear();
	mrddigitpmtsthisevent.clear();
	mrddigitchargesthisevent.clear();
	if(!TDCData){
		if(verbosity) cout<<"no TDC data to find MRD tracks in."<<endl;
		return true;
	}
	if(TDCData->size()==0){
		if(verbosity) cout<<"no TDC hits to find tracks in."<<endl;
		return true;
	}
	
	if(verbosity>3) cout<<"retrieving digit info from "<<TDCData->size()<<" hit pmts"<<endl;
	for(auto&& anmrdpmt : (*TDCData)){
		// retrieve the digit information
		// ============================
		//WCSimRootCherenkovDigiHit* digihit =
		//	(WCSimRootCherenkovDigiHit*)atrigm->GetCherenkovDigiHits()->At(i);
		/*
		digihit is of type std::pair<unsigned long,vector<TDCHit>>,
		the unsigned long is a unique number allocated to all signal channels
		*/
		
		unsigned long chankey = anmrdpmt.first;
		Detector* thedetector = geo->ChannelToDetector(chankey);
		int wcsimtubeid = channelkey_to_mrdpmtid.at(chankey);
		assert(wcsimtubeid>0&&"FindMrdTracks WCSimTubeId==0!");
		
		if(thedetector->GetDetectorElement()!="MRD") continue; // this is a veto hit, not an MRD hit.
		for(auto&& hitsonthismrdpmt : anmrdpmt.second){
			mrddigitpmtsthisevent.push_back(wcsimtubeid-1);
			//cout<<"recording MRD hit at time "<<hitsonthismrdpmt.GetTime()<<endl;
			mrddigittimesthisevent.push_back(hitsonthismrdpmt.GetTime());
			mrddigitchargesthisevent.push_back(hitsonthismrdpmt.GetCharge());
			if(MakeMrdDigitTimePlot){
				// fill the histogram if we're checking
				mrddigitts->Fill(hitsonthismrdpmt.GetTime());
			}
		}
	}
	int numdigits = mrddigittimesthisevent.size();
	
	///////////////////////////
	// now do the track finding
	
	if(verbosity>1) cout<<"Searching for MRD tracks in event "<<eventnum<<endl;
	if(verbosity>2) cout<<"mrddigittimesthisevent.size()="<<numdigits<<endl;
/*
if your class contains pointers, use TrackArray.Clear("C"). You MUST then provide a Clear() method in your class that properly performs clearing and memory freeing. (or "implements the reset procedure for pointer objects")
 see https://root.cern.ch/doc/master/classTClonesArray.html#a025645e1e80ea79b43a08536c763cae2
*/
	int mrdeventcounter=0;
	if(numdigits<minimumdigits){
		// =====================================================================================
		// NO DIGITS IN THIS EVENT
		// ======================================
		//new((*SubEventArray)[0]) cMRDSubEvent();
		nummrdsubeventsthisevent=0;
		nummrdtracksthisevent=0;
		if(writefile){
			nummrdsubeventsthiseventb->Fill();
			nummrdtracksthiseventb->Fill();
			subeventsinthiseventb->Fill();
			//mrdtree->Fill();                // fill the branches so the entries align.
			mrdtrackfile->cd();
			mrdtree->SetEntries(nummrdtracksthiseventb->GetEntries());
			mrdtree->Write("",TObject::kOverwrite);
			gROOT->cd();
		}
		if(verbosity) cout<<"No MRD digits in this event; FindMrdTracks tool returning"<<endl;
		return true;
		// XXX Note for other tools: we've written files and updated BoostStore SubEvent/Track counts to 0.
		// XXX Other tools should check these before processing the MrdTracks BoostStore or MrdSubEventTClonesArray
		// XXX since those are not necessarily cleared!
		// skip remainder
		// ======================================================================================
	}
	
	// MEASURE EVENT DURATION TO DETERMINE IF THERE IS MORE THAN ONE MRD SUB EVENT
	// ===========================================================================
	// first check: are all hits within a 30ns window (maxsubeventduration) If so, just one subevent.
	double eventendtime = *std::max_element(mrddigittimesthisevent.begin(),mrddigittimesthisevent.end());
	double eventstarttime = *std::min_element(mrddigittimesthisevent.begin(),mrddigittimesthisevent.end());
	double eventduration = (eventendtime - eventstarttime);
	if(verbosity>2){
		cout<<"mrd event start: "<<eventstarttime<<", end : "
		    <<eventendtime<<", duration : "<<eventduration<<endl;
	}
	
	if((eventduration<maxsubeventduration)&&(numdigits>=minimumdigits)){
	// JUST ONE SUBEVENT
	// =================
		if(verbosity>1){
			cout<<"all hits this event within one subevent."<<endl;
		}
		std::vector<int> digitidsinasubevent(numdigits);
		std::iota(digitidsinasubevent.begin(),digitidsinasubevent.end(),1);
		// these aren't recorded for true data. Could be obtained for MC, but not essential for now
		std::vector<double> digitqsinasubevent(numdigits,0);
		std::vector<int> digitnumtruephots(numdigits,0);
		std::vector<int> particleidsinasubevent(numdigits,0);
		std::vector<double> photontimesinasubevent(numdigits,0);
		
		// used to pull information from the WCSim trigger on true photons, parents, charges etc.
		// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//		// loop over digits and extract info
//		for(int thisdigit=0;thisdigit<numdigits;thisdigit++){
//			int thisdigitsq = thedigihit->GetQ();
//			digitqsinasubevent.push_back(thisdigitsq);
//			// add all the unique parent ID's for digits contributing to this subevent (truth level info)
			// XXX we need this to be able to draw True tracks by true parentid! XXX
			// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//			std::vector<int> truephotonindices = thedigihit->GetPhotonIds();
//			digitnumtruephots.push_back(truephotonindices.size());
//			for(int truephoton=0; truephoton<truephotonindices.size(); truephoton++){
//				int thephotonsid = truephotonindices.at(truephoton);
//				WCSimRootCherenkovHitTime *thehittimeobject = (WCSimRootCherenkovHitTime*)atrigm->GetCherenkovHitTimes()->At(thephotonsid);
//				int thephotonsparentsubevent = thehittimeobject->GetParentID();
//				particleidsinasubevent.push_back(thephotonsparentsubevent);
//				double thephotonstruetime = thehittimeobject->GetTruetime();
//				photontimesinasubevent.push_back(thephotonstruetime);
//			}
//		}
		
		// pull truth tracks to overlay on reconstructed tracks
		// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//		std::vector<WCSimRootTrack*> truetrackpointers;               // removed from cMRDSubEvent
		std::vector<std::pair<TVector3,TVector3>> truetrackvertices;  // replacement of above
		std::vector<Int_t> truetrackpdgs;                             // replacement of above
		if(DrawTruthTracks){
			int numtracks = MCParticles->size();
			for(int truetracki=0; truetracki<numtracks; truetracki++){
				MCParticle nextrack = MCParticles->at(truetracki);
				if(true&&nextrack.GetPdgCode()!=11){
					Position startvp = nextrack.GetStartVertex();
					TVector3 startv(startvp.X()*100.,startvp.Y()*100.,startvp.Z()*100.);
					Position stopvp = nextrack.GetStopVertex();
					TVector3 stopv(stopvp.X()*100.,stopvp.Y()*100.,stopvp.Z()*100.);
					truetrackvertices.push_back({startv,stopv});
					truetrackpdgs.push_back(nextrack.GetPdgCode());
				}
			}
		}
		// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
		
		// construct the subevent from all the digits
		if(verbosity>1){
			cout<<"constructing a single subevent for this event"<<endl;
		}
		cMRDSubEvent* currentsubevent = new((*SubEventArray)[0]) cMRDSubEvent(0, currentfilestring, runnum, eventnum, triggernum, digitidsinasubevent, mrddigitpmtsthisevent, digitqsinasubevent, mrddigittimesthisevent, digitnumtruephots, photontimesinasubevent, particleidsinasubevent, truetrackvertices, truetrackpdgs);
		mrdeventcounter++;
		// can also use 'cMRDSubEvent* = (cMRDSubEvent*)SubEventArray.ConstructedAt(0);' followed by a bunch of
		// 'Set' calls to set all relevant fields. This bypasses the constructor, calling it only when
		// necessary, saving time. In that case, we do not need to call SubEventArray.Clear();
		
		int tracksthissubevent=currentsubevent->GetTracks()->size();
		if(verbosity>1){
			cout<<"the only subevent this event found "<<tracksthissubevent<<" tracks"<<endl;
		}
		nummrdsubeventsthisevent=1;
		nummrdtracksthisevent=tracksthissubevent;
		if(writefile){
			nummrdsubeventsthiseventb->Fill();
			nummrdtracksthiseventb->Fill();
			subeventsinthiseventb->Fill();
		}
		
	} else {
	// MORE THAN ONE MRD SUBEVENT
	// ===========================
		std::vector<int> digitidsinasubevent;
		std::vector<int> tubeidsinasubevent;
		std::vector<double> digitqsinasubevent;
		std::vector<double> digittimesinasubevent;
		std::vector<int> digitnumtruephots;
		std::vector<int> particleidsinasubevent;
		std::vector<double> photontimesinasubevent;
		
		// this event has multiple subevents. Need to split hits into which subevent they belong to.
		// scan over the times and look for gaps where no digits lie, using these to delimit 'subevents'
		std::vector<float> subeventhittimesv;   // a vector of the starting times of a given subevent
		std::vector<double> sorteddigittimes(mrddigittimesthisevent);
		std::sort(sorteddigittimes.begin(), sorteddigittimes.end());
		subeventhittimesv.push_back(sorteddigittimes.at(0));
		for(int i=0;i<sorteddigittimes.size()-1;i++){
			float timetonextdigit = sorteddigittimes.at(i+1)-sorteddigittimes.at(i);
			if(timetonextdigit>minimum_subevent_timeseparation){
				subeventhittimesv.push_back(sorteddigittimes.at(i+1));
				if(verbosity>2){
					cout<<"Setting subevent time threshold at "<<subeventhittimesv.back()<<endl;
//					cout<<"this digit is at "<<sorteddigittimes.at(i)<<endl;
//					cout<<"next digit is at "<<sorteddigittimes.at(i+1)<<endl;
//					try{
//						cout<<"next next digit is at "<<sorteddigittimes.at(i+2)<<endl;
//					} catch (...) { int i=0; }
				}
			}
		}
		if(verbosity>1){
			cout<<subeventhittimesv.size()<<" subevents this event"<<endl;
		}
		// for checking the timing splitting of subevents, draw a histogram of the times
		if(MakeMrdDigitTimePlot){
			// remake the canvas: close it when done viewing (deleted by ROOT)
			if(gROOT->FindObject("findMrdRootCanvas")) delete findMrdRootCanvas;
			findMrdRootCanvas = new TCanvas("findMrdRootCanvas","findMrdRootCanvas",canvwidth,canvheight);
			findMrdRootCanvas->SetWindowSize(canvwidth,canvheight);
			findMrdRootCanvas->cd();
			mrddigitts->Draw();
			findMrdRootCanvas->Modified();
			findMrdRootCanvas->Update();
			gSystem->ProcessEvents();
			//rootTApp->Run();
			//std::this_thread::sleep_for (std::chrono::seconds(5));
			while(gROOT->FindObject("findMrdRootCanvas")!=nullptr){
				gSystem->ProcessEvents();
				std::this_thread::sleep_for (std::chrono::milliseconds(500));
			}
		}
		
		// a vector to record the subevent number for each hit, to know if we've allocated it yet.
		std::vector<int> subeventnumthisevent(numdigits,-1);
		// another for truth tracks used in drawing
		int numtracks = MCParticles->size();
		std::vector<int> subeventnumthisevent2(numtracks,-1);
		
		// now we need to sort the digits into the subevents they belong to:
		// loop over subevents
		int mrdtrackcounter=0;   // not all the subevents will have a track
		for(int thissubevent=0; thissubevent<subeventhittimesv.size(); thissubevent++){
			if(verbosity>2){
				cout<<"Digits in MRD at = "<<subeventhittimesv.at(thissubevent)<<"ns in event "<<eventnum<<endl;
			}
			// don't need to worry about lower bound as we start from lowest t peak and
			// exclude already allocated hits
			
			float endtime = (thissubevent<(subeventhittimesv.size()-1)) ?
				subeventhittimesv.at(thissubevent+1) : (eventendtime+1.);
			if(verbosity>2){
				cout<<"endtime for subevent "<<thissubevent<<" is "<<endtime<<endl;
			}
			// times are not ordered, so scan through all digits for each subevent
			for(int thisdigit=0;thisdigit<numdigits;thisdigit++){
				if(subeventnumthisevent.at(thisdigit)<0 && mrddigittimesthisevent.at(thisdigit)< endtime ){
					// thisdigit is in thissubevent
					if(verbosity>5){
						cout<<"adding digit at "<<mrddigittimesthisevent.at(thisdigit)<<" to subevent "<<thissubevent<<endl;
					}
					digitidsinasubevent.push_back(thisdigit);
					subeventnumthisevent.at(thisdigit)=thissubevent;
					tubeidsinasubevent.push_back(mrddigitpmtsthisevent.at(thisdigit));
					digittimesinasubevent.push_back(mrddigittimesthisevent.at(thisdigit));
					digitqsinasubevent.push_back(mrddigitchargesthisevent.at(thisdigit));
//					// add all the unique parent ID's for digits contributing to this subevent (truth level info)
					// XXX we need this to be able to draw True tracks by true parentid! XXX
					// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//					std::vector<int> truephotonindices = thedigihit->GetPhotonIds();
//					digitnumtruephots.push_back(truephotonindices.size());
//					for(int truephoton=0; truephoton<truephotonindices.size(); truephoton++){
//						int thephotonsid = truephotonindices.at(truephoton);
//						WCSimRootCherenkovHitTime *thehittimeobject = (WCSimRootCherenkovHitTime*)atrigm->GetCherenkovHitTimes()->At(thephotonsid);
//						int thephotonsparentsubevent = thehittimeobject->GetParentID();
//						particleidsinasubevent.push_back(thephotonsparentsubevent);
//						double thephotonstruetime = thehittimeobject->GetTruetime();
//						photontimesinasubevent.push_back(thephotonstruetime);
//					}
					// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
				}
			}
			digitnumtruephots.assign(digittimesinasubevent.size(),0);      // FIXME replacement of above
			photontimesinasubevent.assign(digittimesinasubevent.size(),0); // FIXME replacement of above
			particleidsinasubevent.assign(digittimesinasubevent.size(),0); // FIXME replacement of above
			
			// construct the subevent from all the digits
			if(digitidsinasubevent.size()>=minimumdigits){  // must have enough for a subevent
				// pull true tracks in window to draw overlaid on reconstructed tracks
				// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
				//std::vector<WCSimRootTrack*> truetrackpointers;               // removed from cMRDSubEvent
				std::vector<std::pair<TVector3,TVector3>> truetrackvertices;    // replacement of above
				std::vector<Int_t> truetrackpdgs;                               // replacement of above
				if(DrawTruthTracks){
					for(int truetracki=0; truetracki<numtracks; truetracki++){
						MCParticle nextrack = MCParticles->at(truetracki);
						if((subeventnumthisevent2.at(truetracki)<0)&&(nextrack.GetStartTime()<endtime)
							&&(nextrack.GetPdgCode()!=11)){
							Position startvp = nextrack.GetStartVertex();
							TVector3 startv(startvp.X()*100.,startvp.Y()*100.,startvp.Z()*100.);
							Position stopvp = nextrack.GetStopVertex();
							TVector3 stopv(stopvp.X()*100.,stopvp.Y()*100.,stopvp.Z()*100.);
							truetrackvertices.push_back({startv,stopv});
							truetrackpdgs.push_back(nextrack.GetPdgCode());
							subeventnumthisevent2.at(truetracki)=thissubevent;
						}
					}
				}
				// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
				
				if(verbosity>3){
					cout<<"constructing subevent "<<mrdeventcounter<<" with "<<digitidsinasubevent.size()<<" digits"<<endl;
				}
				cMRDSubEvent* currentsubevent = new((*SubEventArray)[mrdeventcounter]) cMRDSubEvent(mrdeventcounter, currentfilestring, runnum, eventnum, triggernum, digitidsinasubevent, tubeidsinasubevent, digitqsinasubevent, digittimesinasubevent, digitnumtruephots, photontimesinasubevent, particleidsinasubevent, truetrackvertices, truetrackpdgs);
				mrdeventcounter++;
				mrdtrackcounter+=currentsubevent->GetTracks()->size();
				if(verbosity>2){
					cout<<"subevent "<<thissubevent<<" found "<<currentsubevent->GetTracks()->size()<<" tracks"<<endl;
				}
			}
			
			// clear the vectors and loop to the next subevent
			digitidsinasubevent.clear();
			tubeidsinasubevent.clear();
			digitqsinasubevent.clear();
			digittimesinasubevent.clear();
			particleidsinasubevent.clear();
			photontimesinasubevent.clear();
			digitnumtruephots.clear();
			
		}
		
		// quick scan to check for any unallocated hits
		for(int k=0;k<subeventnumthisevent.size();k++){
			if(subeventnumthisevent.at(k)==-1 && verbosity>2 ){cout<<"*****unbinned hit!"<<k<<" "<<mrddigittimesthisevent.at(k)<<endl;}
		}
		
		nummrdsubeventsthisevent=mrdeventcounter;
		nummrdtracksthisevent=mrdtrackcounter;
		if(writefile){
			nummrdsubeventsthiseventb->Fill();
			nummrdtracksthiseventb->Fill();
			subeventsinthiseventb->Fill();
			//mrdtree->Fill();
		}
		
	}	// end multiple subevents case
	
	//if(eventnum==735){ assert(false); }
	//if(nummrdtracksthisevent) std::this_thread::sleep_for (std::chrono::seconds(5));
	
	// WRITE+CLOSE OUTPUT FILES
	// ========================
	if(verbosity){
		std::cout<<"Found "<<nummrdtracksthisevent<<" MRD tracks in this event, in "<<nummrdsubeventsthisevent
			<<" sub-events. End of finding MRD SubEvents and Tracks in this event"<<std::endl;
	}
	if(writefile){
		if(verbosity>2) std::cout<<"Writing update to mrdtree in mrdtrackfile"<<std::endl;
		mrdtrackfile->cd();
		mrdtree->SetEntries(nummrdtracksthiseventb->GetEntries());
		mrdtree->Write("",TObject::kOverwrite);
	}
	if(cMRDSubEvent::imgcanvas) cMRDSubEvent::imgcanvas->Update();
	gROOT->cd();
	
	// save MRD tracks to the store for downstream tools
	if(verbosity>3) std::cout<<"Writing MRD tracks to ANNIEEVENT"<<std::endl;
	m_data->Stores["MRDTracks"]->Set("NumMrdSubEvents",nummrdsubeventsthisevent);
	m_data->Stores["MRDTracks"]->Set("NumMrdTracks",nummrdtracksthisevent);
	if(nummrdtracksthisevent>theMrdTracks->size()){
		if(verbosity>3) std::cout<<"Growing theMrdTracks vector to size "<<nummrdtracksthisevent<<std::endl;
		theMrdTracks->resize(nummrdtracksthisevent);
	}
	
	for(int subevi=0; subevi<nummrdsubeventsthisevent; subevi++){
		cMRDSubEvent* asubev = (cMRDSubEvent*)SubEventArray->At(subevi);
		// let's not save the SubEvent information. It's not much use.
		// N.B. BOOSTSTORE "MRDSubEvents" IS NOT CREATED
//		m_data->Stores["MRDSubEvents"]->Set("SubEventID",asubev->GetSubEventID());
//		m_data->Stores["MRDSubEvents"]->Set("NumDigits",asubev->GetNumDigits());
//		m_data->Stores["MRDSubEvents"]->Set("NumLayersHit",asubev->GetNumLayersHit());
//		m_data->Stores["MRDSubEvents"]->Set("NumPmtsHit",asubev->GetNumPMTsHit());
//		m_data->Stores["MRDSubEvents"]->Set("LayersHit",asubev->GetLayersHit());
//		m_data->Stores["MRDSubEvents"]->Set("LayerEdeps",asubev->GetEdeps());
//		m_data->Stores["MRDSubEvents"]->Set("PMTsHit",asubev->GetPMTsHit());
//		//m_data->Stores["MRDSubEvents"]->Set("TrueTracks",asubev->GetTrueTracks()); // WCSimRootTracks
//		m_data->Stores["MRDSubEvents"]->Set("DigitIds",asubev->GetDigitIds());       // WCSimRootCkvDigi indices
//		m_data->Stores["MRDSubEvents"]->Set("DigitQs",asubev->GetDigitQs());
//		m_data->Stores["MRDSubEvents"]->Set("DigitTs",asubev->GetDigitTs());
//		m_data->Stores["MRDSubEvents"]->Set("NumPhotonsInDigits",asubev->GetDigiNumPhots());
//		// since each digit may have mutiple photons the following are concatenated.
//		// scan through and keep a running total of NumPhots to identify the subarray start/ends
//		m_data->Stores["MRDSubEvents"]->Set("PhotTsInDigits",asubev->GetDigiPhotTs());
//		m_data->Stores["MRDSubEvents"]->Set("PhotParentsInDigits",asubev->GetDigiPhotParents());
		
		// can't nest multi-entry BoostStores
		//m_data->Stores["MRDSubEvents"]->Set("RecoTracks",asubev->GetTracks());       // cMRDTracks
		std::vector<cMRDTrack>* thetracks = asubev->GetTracks();
		for(int tracki=0; tracki<thetracks->size(); tracki++){
			cMRDTrack* atrack = &thetracks->at(tracki);
			
			// get the BoostStore to hold this track
			BoostStore* thisTrackAsBoostStore = &(theMrdTracks->at(subevi+tracki));
			// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
			// If no tracks are found to match the reconstructed event vertex, the TrackCombiner Tool
			// may perform a cruder reconstruction and append an Mrd Track to this vector.
			// Since we don't remove and re-create the BoostStores each Execute, any members that are
			// not updated by both tools could result in spurious track properties being propagated!
			// !!!!!!!!!!!!! It is therefore crucial to keep these two Tools in sync! !!!!!!!!!!!!!!!
			// XXX XXX XXX XXX XXX XXX XXX XXX XXX XXX XXX XXX XXX XXX XXX XXX XXX XXX XXX XXX XXX XXX
			// IF ADDING MEMBERS TO THE MRDTRACK BOOSTSTORE HERE, ADD THEM TO THE TRACKCOMBINER TOO!
			// XXX XXX XXX XXX XXX XXX XXX XXX XXX XXX XXX XXX XXX XXX XXX XXX XXX XXX XXX XXX XXX XXX
			// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
			
			// fill with this track's data
			thisTrackAsBoostStore->Set("MrdTrackID",atrack->GetTrackID());
			thisTrackAsBoostStore->Set("MrdSubEventID",atrack->GetMrdSubEventID());
			thisTrackAsBoostStore->Set("InterceptsTank",atrack->GetInterceptsTank());
			thisTrackAsBoostStore->Set("StartTime",atrack->GetStartTime());
			// convert start posn from TVector3 to ANNIEEVENT Position class
			Position startpos(atrack->GetStartVertex().X() / 100.,
							  atrack->GetStartVertex().Y() / 100.,
							  atrack->GetStartVertex().Z() / 100.);
			// same for endpos
			Position endpos(  atrack->GetStopVertex().X() / 100.,
							  atrack->GetStopVertex().Y() / 100.,
							  atrack->GetStopVertex().Z() / 100.);
			thisTrackAsBoostStore->Set("StartVertex",startpos);
			thisTrackAsBoostStore->Set("StopVertex",endpos);
			thisTrackAsBoostStore->Set("TrackAngle",atrack->GetTrackAngle());
			thisTrackAsBoostStore->Set("TrackAngleError",atrack->GetTrackAngleError()); // TODO!!!
			thisTrackAsBoostStore->Set("LayersHit",atrack->GetLayersHit());
			thisTrackAsBoostStore->Set("TrackLength",atrack->GetTrackLength() / 100.);
			thisTrackAsBoostStore->Set("IsMrdPenetrating",atrack->GetIsPenetrating());
			thisTrackAsBoostStore->Set("EnergyLoss",atrack->GetEnergyLoss());
			thisTrackAsBoostStore->Set("EnergyLossError",atrack->GetEnergyLossError());
			thisTrackAsBoostStore->Set("IsMrdStopped",atrack->GetIsStopped());
			thisTrackAsBoostStore->Set("IsMrdSideExit",atrack->GetIsSideExit());
			thisTrackAsBoostStore->Set("PenetrationDepth",atrack->GetPenetrationDepth() / 100.);
			thisTrackAsBoostStore->Set("HtrackFitChi2",atrack->GetHtrackFitChi2());
			thisTrackAsBoostStore->Set("HtrackFitCov",atrack->GetHtrackFitCov());
			thisTrackAsBoostStore->Set("VtrackFitChi2",atrack->GetVtrackFitChi2());
			thisTrackAsBoostStore->Set("VtrackFitCov",atrack->GetVtrackFitCov());
			thisTrackAsBoostStore->Set("PMTsHit",atrack->GetPMTsHit());
			
			thisTrackAsBoostStore->Set("HtrackOrigin",atrack->GetHtrackOrigin());
			thisTrackAsBoostStore->Set("HtrackOriginError",atrack->GetHtrackOriginError());
			thisTrackAsBoostStore->Set("HtrackGradient",atrack->GetHtrackGradient());
			thisTrackAsBoostStore->Set("HtrackGradientError",atrack->GetHtrackGradientError());
			thisTrackAsBoostStore->Set("VtrackOrigin",atrack->GetVtrackOrigin());
			thisTrackAsBoostStore->Set("VtrackOriginError",atrack->GetVtrackOriginError());
			thisTrackAsBoostStore->Set("VtrackGradient",atrack->GetVtrackGradient());
			thisTrackAsBoostStore->Set("VtrackGradientError",atrack->GetVtrackGradientError());
			
			// convert back projections to Position class
			Position TankExitPoint( atrack->GetTankExitPoint().X() / 100.,
									atrack->GetTankExitPoint().Y() / 100.,
									atrack->GetTankExitPoint().Z() / 100.);
			Position MrdEntryPoint( atrack->GetMrdEntryPoint().X() / 100.,
									atrack->GetMrdEntryPoint().Y() / 100.,
									atrack->GetMrdEntryPoint().Z() / 100.);
			thisTrackAsBoostStore->Set("TankExitPoint",TankExitPoint);
			thisTrackAsBoostStore->Set("MrdEntryPoint",MrdEntryPoint);
			
			// this stuff either isn't important or isn't yet implemented, don't store:
//			thisTrackAsBoostStore->Set("NumPMTsHit",atrack->GetNumPMTsHit());
//			thisTrackAsBoostStore->Set("KEStart",atrack->GetKEStart());
//			thisTrackAsBoostStore->Set("KEEnd",atrack->GetKEEnd());
//			thisTrackAsBoostStore->Set("ParticlePID",atrack->GetParticlePID());
//			thisTrackAsBoostStore->Set("NumLayersHit",atrack->GetNumLayersHit());
//			thisTrackAsBoostStore->Set("LayerEdeps",atrack->GetEdeps());
//			thisTrackAsBoostStore->Set("NumDigits",atrack->GetNumDigits());
//			thisTrackAsBoostStore->Set("MrdEntryBoundsX",atrack->GetMrdEntryBoundsX());
//			thisTrackAsBoostStore->Set("MrdEntryBoundsY",atrack->GetMrdEntryBoundsY());
//			//thisTrackAsBoostStore->Set("TrueTrackID",atrack->GetTrueTrackID());
//			//thisTrackAsBoostStore->Set("TrueTrack",atrack->GetTrueTrack());
//			thisTrackAsBoostStore->Set("DigitIds",atrack->GetDigitIds());
//			thisTrackAsBoostStore->Set("DigitQs",atrack->GetDigitQs());
//			thisTrackAsBoostStore->Set("DigitTs",atrack->GetDigitTs());
//			thisTrackAsBoostStore->Set("DigiNumPhots",atrack->GetDigiNumPhots());
//			thisTrackAsBoostStore->Set("DigiPhotTs",atrack->GetDigiPhotTs());
//			thisTrackAsBoostStore->Set("DigiPhotParents",atrack->GetDigiPhotParents());
		}
	}
	m_data->Stores["MRDTracks"]->Set("MRDTracks",theMrdTracks,false); // the address may be changed by resize()
	//m_data->Stores["MRDTracks"]->Save();   // write to file?
	
	// the TClonesArray doesn't get deleted (for efficiency reasons) so shove a pointer
	// into the CStore in case something wants to use it (e.g. MrdPaddlePlot tool)
	//m_data->CStore.Set("MrdSubEventTClonesArray",SubEventArray,false);
	// XXX shouldn't be necessary as SubEventArray is re-used, so pointer has not changed
	intptr_t subevptr = reinterpret_cast<intptr_t>(SubEventArray);
	m_data->CStore.Set("MrdSubEventTClonesArray",subevptr);
	
	return true;
}

void FindMrdTracks::StartNewFile(){
	TString filenameout = TString::Format("%s/mrdtrackfile.%d.%d.root",outputdir.c_str(),runnum,subrunnum);
	if(verbosity) cout<<"creating mrd output file "<<filenameout.Data()<<endl;
	if(mrdtrackfile) mrdtrackfile->Close();
	mrdtrackfile = new TFile(filenameout.Data(),"RECREATE","MRD Tracks file");
	mrdtrackfile->cd();
	mrdtree = new TTree("mrdtree","Tree for reconstruction data");
	mrdeventnumb = mrdtree->Branch("EventID",&eventnum);
	mrdtriggernumb = mrdtree->Branch("TriggerID",&triggernum);
	nummrdsubeventsthiseventb = mrdtree->Branch("nummrdsubeventsthisevent",&nummrdsubeventsthisevent);
	subeventsinthiseventb = mrdtree->Branch("subeventsinthisevent",&SubEventArray, nummrdsubeventsthisevent);
	nummrdtracksthiseventb = mrdtree->Branch("nummrdtracksthisevent",&nummrdtracksthisevent);
	gROOT->cd();
}


bool FindMrdTracks::Finalise(){
	// close output file
	if(mrdtrackfile){
		mrdtrackfile->Close();
		delete mrdtrackfile;
		mrdtrackfile=nullptr;
	}
	
	// clean up any BoostStore items
	m_data->Stores["MRDTracks"]->Delete();
	
	// clean up the space from the no longer needed TClonesArray
	//SubEventArray->Clear("C");
	if(verbosity>0) cout<<"FindMrdTracks Tool Calling SubEventArray->Delete()"<<endl;
	if(SubEventArray){ SubEventArray->Delete(); delete SubEventArray; SubEventArray=0;}
	
	if(MakeMrdDigitTimePlot){
		if(mrddigitts) delete mrddigitts;
		if(gROOT->FindObject("findMrdRootCanvas")) delete findMrdRootCanvas;
		
		// see if we're the last user of the TApplication and release it if so,
		// otherwise de-register us as a user since we're done
		int tapplicationusers=0;
		get_ok = m_data->CStore.Get("RootTApplicationUsers",tapplicationusers);
		if(not get_ok || tapplicationusers==1){
			if(rootTApp){
				std::cout<<"MrdPaddlePlot Tool: Deleting global TApplication"<<std::endl;
				delete rootTApp;
				rootTApp=nullptr;
			}
		} else if(tapplicationusers>1){
			m_data->CStore.Set("RootTApplicationUsers",tapplicationusers-1);
		}
	}
	
	if(verbosity>0) cout<<"FindMrdTracks exiting"<<endl;
	return true;
}
