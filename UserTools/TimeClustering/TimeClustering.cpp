/* vim:set noexpandtab tabstop=4 wrap */
#include "TimeClustering.h"

// for sleeping
#include <thread>          // std::this_thread::sleep_for
#include <chrono>          // std::chrono::seconds

// for ROOT debug plot
#include "TROOT.h"
#include "TSystem.h"
#include "TH1D.h"
#include "TCanvas.h"
#include "TApplication.h"

TimeClustering::TimeClustering():Tool(){}


bool TimeClustering::Initialise(std::string configfile, DataModel &data){
	
	if(verbosity) cout<<"Initializing tool TimeClustering"<<endl;
	
	/////////////////// Useful header ///////////////////////
	if(configfile!="") m_variables.Initialise(configfile); //loading config file
	//m_variables.Print();
	
	m_data= &data; //assigning transient data pointer
	/////////////////////////////////////////////////////////////////
	
	m_variables.Get("verbosity",verbosity);
	m_variables.Get("MinDigitsForTrack",minimumdigits);
	m_variables.Get("MaxMrdSubEventDuration",maxsubeventduration);
	m_variables.Get("MinSubeventTimeSep",minimum_subevent_timeseparation);
	m_variables.Get("MakeMrdDigitTimePlot",MakeMrdDigitTimePlot);  /// XXX XXX remame
	
	if(MakeMrdDigitTimePlot){
		mrddigitts = new TH1D("mrddigitts","mrd digit times",500,-50,2000);
		canvwidth = 900;
		canvheight = 600;
		
		// Since we want to make event-wise debug plots, we need a TApplication to show them
		// ======================
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
	}
	
	return true;
}


bool TimeClustering::Execute(){
	
	Log("Tool TimeClustering: finding time clusters in next event",v_message,verbosity);
	
	MrdTimeClusters.clear(); // do not carry over any subevents
	m_data->Stores.at("ANNIEEvent")->Set("NumMrdTimeClusters",0);
	int mrdeventcounter=0;
	
	// extract the digits from the annieevent and put them into separate vectors used by the track finder
	mrddigitpmtsthisevent.clear();
	mrddigittimesthisevent.clear();
	
	get_ok = m_data->Stores.at("ANNIEEvent")->Get("TDCData",TDCData);  // a std::map<unsigned long,vector<MCHit>>
	if(not get_ok){
		Log("TimeClustering Tool: No TDC data in ANNIEEvent!",v_error,verbosity);
		return false;
	}
	get_ok = m_data->Stores.at("ANNIEEvent")->Get("MCHits",MCHits);    // a std::map<unsigned long,vector<MCHit>>
	if(not get_ok){
		Log("TimeClustering Tool: No MCHits in ANNIEEvent!",v_error,verbosity);
		return false;
	}
	
	// check if we have any hits to process
	if(TDCData->size()==0){
		if(verbosity) cout<<"no TDC hits to find clusters in."<<endl;
		return true; // XXX XXX XXX XXX XXX XXX XXX
	}
	
	Log("TimeClustering Tool: Retrieving digit info from "+to_string(TDCData->size())+" hit pmts",v_debug,verbosity);
	// just dump all the hit times in this event into a vector. Allows us to sort hit times and search for clusters.
	for(auto&& anmrdpmt : (*TDCData)){
		unsigned long chankey = anmrdpmt.first;
		// if(thedetector->GetDetectorElement()!="MRD") continue; // this is a veto hit, not an MRD hit. XXX keep this cut?
		for(auto&& hitsonthismrdpmt : anmrdpmt.second){
			mrddigitpmtsthisevent.push_back(chankey);
			mrddigittimesthisevent.push_back(hitsonthismrdpmt.GetTime());
			if(MakeMrdDigitTimePlot){  // XXX XXX XXX rename
				// fill the histogram if we're checking
				mrddigitts->Fill(hitsonthismrdpmt.GetTime());
			}
		}
	}
	int numdigits = mrddigittimesthisevent.size();
	Log("TimeClustering Tool: Searching "+to_string(numdigits)+" (MRD) hits this event for clusters",v_debug,verbosity);
	
	///////////////////////////
	// now do the time clustering
	
	// First the easy check; do we have enough hits to make a cluster
	if(numdigits<minimumdigits){
		// =====================================================================================
		// NOT ENOUGH DIGITS IN THIS EVENT
		// ======================================
		Log("TimeClustering Tool: Insufficient digits in this event to make any clusters; returning",v_debug,verbosity);
		return true;
		// ======================================================================================
	}
	
	// MEASURE TOTAL EVENT DURATION
	// ============================
	double eventendtime = *std::max_element(mrddigittimesthisevent.begin(),mrddigittimesthisevent.end());
	double eventstarttime = *std::min_element(mrddigittimesthisevent.begin(),mrddigittimesthisevent.end());
	double eventduration = (eventendtime - eventstarttime);
	Log("TimeClustering Tool: mrd event start: "+to_string(eventstarttime)
			+", end : "+to_string(eventendtime)+", duration : "+to_string(eventduration),v_debug,verbosity);
	
	// If all hits within maxsubeventduration[ns], just one subevent.
	if((eventduration<maxsubeventduration)&&(numdigits>=minimumdigits)){
	// JUST ONE SUBEVENT
	// =================
		Log("TimeClustering Tool: All hits this event within one subevent.",v_debug,verbosity);
		std::vector<int> digitidsinasubevent(numdigits);                     // a vector of indices of the digits in this subevent
		std::iota(digitidsinasubevent.begin(),digitidsinasubevent.end(),1);  // fill with 1-N, as all digits are are in this subevent
		//MrdTimeClusters.Add(mrdeventcounter, digitidsinasubevent, mrddigitpmtsthisevent, mrddigittimesthisevent);
		MrdTimeClusters.push_back(digitidsinasubevent);
		mrdeventcounter++;
		
	} else {
	// MORE THAN ONE SUBEVENT
	// ======================
		// COUNT SUBEVENTS
		// ---------------
		// this event has multiple subevents. We need to split hits into which subevent they belong to.
		// first scan over the times and look for gaps where no digits lie, using these to delimit 'subevents'
		std::vector<float> subeventhittimesv;   // a vector of the starting times of a given subevent
		std::vector<double> sorteddigittimes(mrddigittimesthisevent);
		std::sort(sorteddigittimes.begin(), sorteddigittimes.end());
		subeventhittimesv.push_back(sorteddigittimes.at(0));
		for(int i=0;i<sorteddigittimes.size()-1;i++){
			float timetonextdigit = sorteddigittimes.at(i+1)-sorteddigittimes.at(i);
			if(timetonextdigit>minimum_subevent_timeseparation){
				subeventhittimesv.push_back(sorteddigittimes.at(i+1));
				Log("TimeClustering Tool: Setting subevent time threshold at "+to_string(subeventhittimesv.back()),v_debug,verbosity);
			}
		}
		Log("TimeClustering Tool: Found "+to_string(subeventhittimesv.size())+" subevents this event",v_debug,verbosity);
		
		// DEBUG CHECK
		// -----------
		// debug check of the timing splitting: draw a histogram of the times
		if(MakeMrdDigitTimePlot){
			// remake the canvas: close it when done viewing (deleted by ROOT)
			if(gROOT->FindObject("timeClusterCanvas")) delete timeClusterCanvas;
			timeClusterCanvas = new TCanvas("timeClusterCanvas","timeClusterCanvas",canvwidth,canvheight);
			timeClusterCanvas->SetWindowSize(canvwidth,canvheight);
			timeClusterCanvas->cd();
			mrddigitts->Draw();
			timeClusterCanvas->Modified();
			timeClusterCanvas->Update();
			gSystem->ProcessEvents();
			//rootTApp->Run();
			//std::this_thread::sleep_for (std::chrono::seconds(5));
			while(gROOT->FindObject("timeClusterCanvas")!=nullptr){
				gSystem->ProcessEvents();
				std::this_thread::sleep_for (std::chrono::milliseconds(500));
			}
		}
		
		// SORT HITS INTO SUBEVENTS
		// ------------------------
		// now we need to sort the digits into the subevents they belong to:
		// temporary storage vectors to hold the digits in the present subevent
		std::vector<int> digitidsinasubevent;
		std::vector<int> tubeidsinasubevent;
		std::vector<double> digittimesinasubevent;
		// a vector to record the subevent number for each hit, to know if we've allocated it yet.
		std::vector<int> subeventnumthisevent(numdigits,-1);
		// LOOP OVER SUBEVENTS
		// -------------------
		for(int thissubevent=0; thissubevent<subeventhittimesv.size(); thissubevent++){
			float endtime = (thissubevent<(subeventhittimesv.size()-1)) ?
								subeventhittimesv.at(thissubevent+1) : (eventendtime+1.);
			Log("TimeClustering Tool: Endtime for subevent "+to_string(thissubevent)+" is "+to_string(endtime), v_debug,verbosity);
			// don't need to worry about search start time as we start from earliest hit and exclude already allocated hits
			// times are not ordered, so scan through all digits for each subevent
			// FIND HITS IN THIS SUBEVENT
			// --------------------------
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
				}
			}
			
			// CONSTRUCT THE SUBEVENT
			// -----------------------
			if(digitidsinasubevent.size()>=minimumdigits){  // must have enough for a subevent
				Log("TimeClustering Tool: Constructing subevent "+to_string(mrdeventcounter)
					+" with "+to_string(digitidsinasubevent.size())+" digits",v_debug,verbosity);
				//MrdTimeClusters.Add(mrdeventcounter, digitidsinasubevent, tubeidsinasubevent, digittimesinasubevent);
				MrdTimeClusters.push_back(digitidsinasubevent);
				mrdeventcounter++;
			}
			
			// clear the vectors and loop to the next subevent
			digitidsinasubevent.clear();
			tubeidsinasubevent.clear();
			digittimesinasubevent.clear();
		}
		
		// quick scan to check for any unallocated hits
		for(int k=0;k<subeventnumthisevent.size();k++){
			if(subeventnumthisevent.at(k)==-1){
				Log("TimeClustering Tool: Found unbinned hit "+to_string(k)+" at "+to_string(mrddigittimesthisevent.at(k)),v_error,verbosity);
			}
		}
		
	}  // end multiple subevents case
	
	// pass the found clusters to the ANNIEEvent
	m_data->Stores.at("ANNIEEvent")->Set("MrdTimeClusters",MrdTimeClusters);
	m_data->Stores.at("ANNIEEvent")->Set("NumMrdTimeClusters",mrdeventcounter);
	
	return true;
}


bool TimeClustering::Finalise(){
	
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
	
	return true;
}
