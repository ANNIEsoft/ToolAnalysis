/* vim:set noexpandtab tabstop=4 wrap */

#include "LoadWCSim.h"

LoadWCSim::LoadWCSim():Tool(){}

bool LoadWCSim::Initialise(std::string configfile, DataModel &data){
	
	/////////////////// Useful header ///////////////////////
	
	if(configfile!="") m_variables.Initialise(configfile); //loading config file
	//m_variables.Print();
	
	m_data= &data; //assigning transient data pointer
	
	// Get the Tool configuration variables
	// ====================================
	m_variables.Get("verbose",verbose);
	//verbose=10;
	m_variables.Get("InputFile",MCFile);
	
	// Short Stores README
	//////////////////////
	// n.b. m_data->vars is a Store (of ben's Store type) that is not saved to disk?
	//      m_data->CStore is a single entry binary BoostStore that is not saved to disk.
	//      m_data->Stores["StoreName"] is a map of binary BoostStores that are saved to disk.
	// If using Stores->BoostStore->Set("MyVariable") it will always be saved to disk 
	// Using Stores->BoostStore.Set("MyVariable",&myvar) if myvar is a pointer (to an object on 
	// the heap) puts myvar in the Store and it's deletion will be  handled by the Store.
	// (provided your class has a suitable destructor.)
	// Is 'BoostStore::Save' needed for single-entry stores?
	// ----------------
	// create a new BoostStore with key "ANNIEEvent" in the Stores std::map
	// BoostStore constructor args: typechecking (bool), m_format (0=binary, 1=ASCII, 2=multievent)
	// A BoostStore has a header where useful constants may be saved. The header is a BoostStore itself,
	// and can be accessed via: Store.Header->Get() and Store.Header->Set().
	// The method 'Store::Save()' writes everything 'Set' since the last 'Save' to the current entry.
	// Store::Clear clears the map of the current entry, to start building a new one.
	// Use 'Store::GetEntry(int entrynum)' to load an entry to then be able to 'Get' it's contents.
	// 'Store->Header->Get("TotalEntries",NumEvents)' will load the num entries into NumEvents
	// ------------------
	// When adding a BoostStore (or class object in general) to a BoostStore (such as ANNIEEvent)
	// you call BoostStore::Set("key",ObjectPointer) - BUT be aware that serialization happens when the
	// 'Set' method is called - although you pass it a pointer, any subsequent changes to the object
	// will NOT get saved! You must call 'Set' AFTER making ALL changes to your object!
	/////////////////////////////////////////////////////////////////
	
	// Make class private members; e.g. the WCSimT and WCSimRootGeom
	// =============================================================
	file= new TFile(MCFile.c_str(),"READ");
	wcsimtree= (TTree*) file->Get("wcsimT");
	NumEvents=wcsimtree->GetEntries();
	WCSimEntry= new wcsimT(wcsimtree);
	wcsimrootgeom = WCSimEntry->wcsimrootgeom;
	if(verbose>1) cout<<"wcsimrootgeom at "<<wcsimrootgeom<<endl;
	
	// put useful constants into the CStore
	// ====================================
	//m_data->CStore.Set("WCSimEntry",WCSimEntry,false); // pass on the WCSim entry - not possible
	
	// Make the ANNIEEvent Store if it doesn't exist
	// =============================================
	int annieeventexists = m_data->Stores.count("ANNIEEvent");
	if(annieeventexists==0) m_data->Stores["ANNIEEvent"] = new BoostStore(false,2);
	
	// construct the Geometry to go in the header from the WCSimRootGeom
	// =================================================================
	double WCSimGeometryVer = 1;                       // TODO pull this from some suitable variable
	// some variables are available from wcsimrootgeom.   TODO: put everything into wcsimrootgeom
	int numtankpmts = wcsimrootgeom->GetWCNumPMT();
	int numlappds = wcsimrootgeom->GetWCNumLAPPD();
	int nummrdpmts = wcsimrootgeom->GetWCNumMRDPMT();
	numvetopmts = wcsimrootgeom->GetWCNumFACCPMT();
	double tank_radius = wcsimrootgeom->GetWCCylRadius();
	double tank_halfheight = wcsimrootgeom->GetWCCylLength();
	// geometry variables not yet in wcsimrootgeom are in MRDSpecs.hh
	double mrd_width =  MRDSpecs::MRD_width;
	double mrd_height = MRDSpecs::MRD_height;
	double mrd_depth =  MRDSpecs::MRD_depth;
	double mrd_start =  MRDSpecs::MRD_start;
	if(verbose>1) cout<<"we have "<<numtankpmts<<" tank pmts, "<<nummrdpmts<<" mrd pmts... etc."<<endl;
	
	// loop over PMTs and make the map of Detectors
	std::map<ChannelKey,Detector> Detectors;
	// tank pmts
	for(int i=0; i<numtankpmts; i++){
		ChannelKey akey(subdetector::ADC, i);
		WCSimRootPMT apmt = wcsimrootgeom->GetPMT(i);
		Detector adet("Tank", Position(apmt.GetPosition(0),apmt.GetPosition(1),apmt.GetPosition(2)), 
		               Direction(apmt.GetOrientation(0),apmt.GetOrientation(1),apmt.GetOrientation(2)), 
		               i, apmt.GetName(), detectorstatus::ON, 0.);
		Detectors.emplace(akey,adet);
	}
	// mrd pmts
	for(int i=0; i<nummrdpmts; i++){
		ChannelKey akey(subdetector::TDC, i);
		WCSimRootPMT apmt = wcsimrootgeom->GetMRDPMT(i);
		Detector adet("MRD", Position(apmt.GetPosition(0),apmt.GetPosition(1),apmt.GetPosition(2)), 
		              Direction(apmt.GetOrientation(0),apmt.GetOrientation(1),apmt.GetOrientation(2)), 
		              i, apmt.GetName(), detectorstatus::ON, 0.);
		Detectors.emplace(akey,adet);
	}
	// veto pmts
	for(int i=0; i<numvetopmts; i++){
		ChannelKey akey(subdetector::TDC, i);
		WCSimRootPMT apmt = wcsimrootgeom->GetFACCPMT(i);
		Detector adet("Veto", Position(apmt.GetPosition(0),apmt.GetPosition(1),apmt.GetPosition(2)), 
		              Direction(apmt.GetOrientation(0),apmt.GetOrientation(1),apmt.GetOrientation(2)), 
		              i, apmt.GetName(), detectorstatus::ON, 0.);
		Detectors.emplace(akey,adet);
	}
	// lappds
	for(int i=0; i<numlappds; i++){
		ChannelKey akey(subdetector::LAPPD, i);
		WCSimRootPMT apmt = wcsimrootgeom->GetLAPPD(i);
		Detector adet("Tank", Position(apmt.GetPosition(0),apmt.GetPosition(1),apmt.GetPosition(2)), 
		              Direction(apmt.GetOrientation(0),apmt.GetOrientation(1),apmt.GetOrientation(2)), 
		              i, apmt.GetName(), detectorstatus::ON, 0.);
		Detectors.emplace(akey,adet);
	}
	
	// construct the goemetry 
	Geometry* anniegeom = new Geometry(Detectors, WCSimGeometryVer, tank_radius, tank_halfheight, 
	                                   mrd_width, mrd_height, mrd_depth, mrd_start, numtankpmts, 
	                                   nummrdpmts, numvetopmts, numlappds, detectorstatus::ON);
	if(verbose>1) cout<<"constructed anniegom at "<<anniegeom<<endl;
	m_data->Stores["ANNIEEvent"]->Header->Set("AnnieGeometry",anniegeom,true);
	
	// Set run-level information in the ANNIEEvent
	// ===========================================
	/*
	At time of writing ANNIEEvent has the following that need to be Set before each Save:
		RunNumber
		SubrunNumber
		EventNumber
		MCParticles
		RecoParticles
		MCHits
		TDCData
		RawADCData
		CalibratedADCData
		RawLAPPDData
		CalibratedLAPPDData
		TriggerData
		MCFlag
		EventTime
		MCEventNum
		MCFile
		BeamStatus
	*/
	
	MCEventNum=-1;
	MCTriggernum=0;
	// pull the first entry with a trigger and use it's Date for the BeamStatus last recorded time. TODO
	if(verbose>1) cout<<"getting Run start time"<<endl;
	do{
		MCEventNum++;
		WCSimEntry->GetEntry(MCEventNum);
	} while(WCSimEntry->wcsimrootevent->GetNumberOfEvents()==0);
	atrigt = WCSimEntry->wcsimrootevent->GetTrigger(0);
	TimeClass RunStartTime(atrigt->GetHeader()->GetDate());
	MCEventNum=0;
	
	// use nominal beam values TODO
	double beaminten=4.777e+12;
	double beampow=3.2545e+16;
	BeamStatus = new BeamStatusClass(RunStartTime, beaminten, beampow, "stable");
	
	// Construct the other objects we'll be setting at event level,
	// pass managed pointers to the ANNIEEvent Store
	MCParticles = new std::vector<Particle>;
	MCHits = new std::map<ChannelKey,std::vector<Hit>>;
	TDCData = new std::map<ChannelKey,std::vector<Hit>>;
	EventTime = new TimeClass();
	TriggerClass beamtrigger("beam",true,0);
	TriggerData = new std::vector<TriggerClass>{beamtrigger}; // FIXME ? one trigger and resetting time is ok?
	
	return true;
}


bool LoadWCSim::Execute(){
	
	// probably not necessary, clears the map for this entry. We're going to re-Set the event entry anyway...
	//m_data->Stores["ANNIEEvent"]->Clear();
	
	if(verbose>1) cout<<"getting entry "<<MCEventNum<<", trigger "<<MCTriggernum<<endl;
	WCSimEntry->GetEntry(MCEventNum);
	MCFile = wcsimtree->GetCurrentFile()->GetName();
	
	MCParticles->clear();
	MCHits->clear();
	TDCData->clear();
	
	//for(int MCTriggernum=0; MCTriggernum<WCSimEntry->wcsimrootevent->GetNumberOfEvents(); MCTriggernum++){
		if(verbose>1) cout<<"getting triggers"<<endl;
		atrigt = WCSimEntry->wcsimrootevent->GetTrigger(MCTriggernum);
		atrigm = WCSimEntry->wcsimrootevent_mrd->GetTrigger(MCTriggernum);
		atrigv = WCSimEntry->wcsimrootevent_facc->GetTrigger(MCTriggernum);
		if(verbose>2) cout<<"wcsimrootevent="<<WCSimEntry->wcsimrootevent<<endl;
		if(verbose>2) cout<<"wcsimrootevent_mrd="<<WCSimEntry->wcsimrootevent_mrd<<endl;
		if(verbose>2) cout<<"wcsimrootevent_facc="<<WCSimEntry->wcsimrootevent_facc<<endl;
		if(verbose>2) cout<<"atrigt="<<atrigt<<", atrigm="<<atrigm<<", atrigv="<<atrigv<<endl;
		
		if(verbose>1) cout<<"getting event date"<<endl;
		RunNumber = atrigt->GetHeader()->GetRun();
		SubrunNumber = 0;
		EventTimeNs = atrigt->GetHeader()->GetDate();
		EventTime->SetNs(EventTimeNs);
		if(verbose>2) cout<<"EventTime is "<<EventTimeNs<<"ns"<<endl;
		
		if(verbose>1) cout<<"getting "<<atrigt->GetNtrack()<<" tracks"<<endl;
		for(int tracki=0; tracki<atrigt->GetNtrack(); tracki++){
			if(verbose>2) cout<<"getting track "<<tracki<<endl;
			WCSimRootTrack* nextrack = (WCSimRootTrack*)atrigt->GetTracks()->At(tracki);
			/* a WCSimRootTrack has methods: 
			Int_t     GetIpnu()             pdg
			Int_t     GetFlag()             -1: neutrino primary, -2: neutrino target, 0: other
			Float_t   GetM()                mass
			Float_t   GetP()                momentum magnitude
			Float_t   GetE()                energy (inc rest mass^2)
			Float_t   GetEndE()             energy on stopping of particle tracking
			Float_t   GetEndP()             momentum on stopping of particle tracking
			Int_t     GetStartvol()         starting volume: 10 is tank, 20 is facc, 30 is mrd
			Int_t     GetStopvol()          stopping volume: but these may not be set.
			Float_t   GetDir(Int_t i=0)     momentum unit vector
			Float_t   GetPdir(Int_t i=0)    momentum vector
			Float_t   GetPdirEnd(Int_t i=0) direction vector on stop tracking
			Float_t   GetStop(Int_t i=0)    stopping vertex x,y,z for i=0-2, in cm
			Float_t   GetStart(Int_t i=0)   starting vertex x,y,z for i=0-2, in cm
			Int_t     GetParenttype()       parent pdg, 0 for primary.
			Float_t   GetTime()             trj->GetGlobalTime(); starting time of particle
			Float_t   GetStopTime()
			Int_t     GetId()               wcsim trackid
			*/
			
			// this is a method in Particle now
//			double tankcentredzstart = nextrack->GetStart(2)-(tank_start+tank_radius);
//			double tankcentredzend   = nextrack->GetStop (2) -(tank_start+tank_radius);
//			double trackstartradius  = sqrt(pow(nextrack->GetStart(0),2.)+pow(tankcentredzstart,2.));
//			double trackendradius    = sqrt(pow(nextrack->GetStop (0),2.)+pow(tankcentredzend,  2.));
//			bool tankcontained =     (MRDSpecs::trackstartradius<MRDSpecs::tank_radius) 
//								  && (nextrack->GetStart(1)<MRDSpecs::tank_halfheight)
//								  && (trackendradius  <MRDSpecs::tank_radius) 
//								  && (nextrack->GetStop (1)<MRDSpecs::tank_halfheight);
//			bool mrdcontained  =     (nextrack->GetStop(2)>(MRDSpecs::MRD_start)) 
//								 &&  (nextrack->GetStop(2)<(MRDSpecs::MRD_start+MRDSpecs::MRD_depth)) 
//								 &&  (abs(nextrack->GetStop(0))<MRDSpecs::MRD_width ) 
//								 &&  (abs(nextrack->GetStop(1))<MRDSpecs::MRD_height));
			
			tracktype startstoptype;
			bool startinbounds = (    nextrack->GetFlag()!=-1
								   && (abs(nextrack->GetStart(0))<550) 
								   && (abs(nextrack->GetStart(0))<550) 
								   && (abs(nextrack->GetStart(0))<550) );
			bool stopinbounds = (     (abs(nextrack->GetStop(0))<550) 
								   && (abs(nextrack->GetStop(0))<550) 
								   && (abs(nextrack->GetStop(0))<550) );
			
			if(startinbounds && stopinbounds) startstoptype = tracktype::CONTAINED;
			else if( startinbounds ) startstoptype = tracktype::STARTONLY;
			else if( stopinbounds  ) startstoptype = tracktype::ENDONLY;
			else startstoptype = tracktype::UNCONTAINED;
			
			Particle thisparticle(
				nextrack->GetIpnu(), nextrack->GetE(), nextrack->GetEndE(),
				Position(nextrack->GetStart(0), nextrack->GetStart(1), nextrack->GetStart(2)),
				Position(nextrack->GetStop(0), nextrack->GetStop(1), nextrack->GetStop(2)),
				TimeClass(nextrack->GetTime()), TimeClass(nextrack->GetStopTime()),
				Direction(nextrack->GetDir(0), nextrack->GetDir(1), nextrack->GetDir(2)),
				sqrt(pow(nextrack->GetStop(0)-nextrack->GetStart(0),2.)+
					 pow(nextrack->GetStop(1)-nextrack->GetStart(1),2.)+
					 pow(nextrack->GetStop(2)-nextrack->GetStart(2),2.)),
					 startstoptype);
			
			MCParticles->push_back(thisparticle);
		}
		if(verbose>2) cout<<"MCParticles has "<<MCParticles->size()<<" entries"<<endl;
		
		// n.b. ChannelKey is currently typedef'd as an int: it's just a unique PMT id for MRD+Tank+FACC
		int numtankdigits = atrigt ? atrigt->GetCherenkovDigiHits()->GetEntries() : 0;
		if(verbose>1) cout<<"looping over "<<numtankdigits<<" tank digits"<<endl;
		for(int digiti=0; digiti<numtankdigits; digiti++){
			if(verbose>2) cout<<"getting digit "<<digiti<<endl;
			WCSimRootCherenkovDigiHit* digihit = 
				(WCSimRootCherenkovDigiHit*)atrigt->GetCherenkovDigiHits()->At(digiti);
			//WCSimRootChernkovDigiHit has methods GetTubeId(), GetT(), GetQ()
			if(verbose>2) cout<<"next digihit at "<<digihit<<endl;
			int tubeid = digihit->GetTubeId();
			if(verbose>2) cout<<"tubeid="<<tubeid<<endl;
			TimeClass digittime(digihit->GetT()+EventTimeNs); // add trigger time to make absolute
			if(verbose>2){ cout<<"digittime is "; digittime.Print(); }
			float digiq = digihit->GetQ();
			if(verbose>2) cout<<"digit Q is "<<digiq<<endl;
			
			ChannelKey key(subdetector::ADC,tubeid);
			Hit nexthit(tubeid, digittime, digiq);
			if(MCHits->count(key)==0) MCHits->emplace(key, std::vector<Hit>{nexthit});
			else MCHits->at(key).push_back(nexthit);
			if(verbose>2) cout<<"digit added"<<endl;
		}
		if(verbose>2) cout<<"done with tank digits"<<endl;
		
		// FIXME implement LAPPD hits
		
		//MRD Hits
		int nummrddigits = atrigm ? atrigm->GetCherenkovDigiHits()->GetEntries() : 0;
		if(verbose>1) cout<<"adding "<<nummrddigits<<" mrd digits"<<endl;
		for(int digiti=0; digiti<nummrddigits; digiti++){
			if(verbose>2) cout<<"getting digit "<<digiti<<endl;
			WCSimRootCherenkovDigiHit* digihit = 
				(WCSimRootCherenkovDigiHit*)atrigm->GetCherenkovDigiHits()->At(digiti);
			if(verbose>2) cout<<"next digihit at "<<digihit<<endl;
			int tubeid = digihit->GetTubeId() + numvetopmts;
			if(verbose>2) cout<<"tubeid="<<tubeid<<endl;
			TimeClass digittime(digihit->GetT()+EventTimeNs); // add trigger time to make absolute
			if(verbose>2){ cout<<"digittime is "; digittime.Print(); }
			float digiq = digihit->GetQ();
			if(verbose>2) cout<<"digit Q is "<<digiq<<endl;
			
			ChannelKey key(subdetector::TDC,tubeid);
			Hit nexthit(tubeid, digittime, digiq);
			if(TDCData->count(key)==0) TDCData->emplace(key, std::vector<Hit>{nexthit});
			else TDCData->at(key).push_back(nexthit);
			if(verbose>2) cout<<"digit added"<<endl;
		}
		if(verbose>2) cout<<"done with mrd digits"<<endl;
		
		// Veto Hits
		int numvetodigits = atrigv ? atrigv->GetCherenkovDigiHits()->GetEntries() : 0;
		if(verbose>1) cout<<"adding "<<numvetodigits<<" veto digits"<<endl;
		for(int digiti=0; digiti<numvetodigits; digiti++){
			if(verbose>2) cout<<"getting digit "<<digiti<<endl;
			WCSimRootCherenkovDigiHit* digihit = 
				(WCSimRootCherenkovDigiHit*)atrigv->GetCherenkovDigiHits()->At(digiti);
			if(verbose>2) cout<<"next digihit at "<<digihit<<endl;
			int tubeid = digihit->GetTubeId();
			if(verbose>2) cout<<"tubeid="<<tubeid<<endl;
			TimeClass digittime(digihit->GetT()+EventTimeNs); // add trigger time to make absolute
			if(verbose>2){ cout<<"digittime is "; digittime.Print(); }
			float digiq = digihit->GetQ();
			if(verbose>2) cout<<"digit Q is "<<digiq<<endl;
			
			ChannelKey key(subdetector::TDC,tubeid);
			Hit nexthit(tubeid, digittime, digiq);
			if(TDCData->count(key)==0) TDCData->emplace(key, std::vector<Hit>{nexthit});
			else TDCData->at(key).push_back(nexthit);
			if(verbose>2) cout<<"digit added"<<endl;
		}
		if(verbose>2) cout<<"done with veto digits"<<endl;
		
		if(verbose>2) cout<<"setting triggerdata time to "<<EventTimeNs<<"ns"<<endl;
		TriggerData->front().SetTime(EventTimeNs);
		
	//}
	
	// Load the corresponding entry from the LAPPD file.
	// Loop over the triggers in this entry
	// fill LAPPDData properly. XXX XXX XXX XXX XXX XX XX XX XXX
	
	//int mrdentries;
	//m_data->Stores["TDCData"]->Get("TotalEntries",mrdentries); // ??
//	m_data->Stores["WCSimEntries"]->Set("wcsimrootevent",WCSimEntry->wcsimrootevent);
//	m_data->Stores["WCSimEntries"]->Set("wcsimrootevent_mrd",WCSimEntry->wcsimrootevent_mrd);
//	m_data->Stores["WCSimEntries"]->Set("wcsimrootevent_facc",*(WCSimEntry->wcsimrootevent_facc));
	
	
	// set event level variables
	if(verbose>1) cout<<"setting the store variables"<<endl;
	m_data->Stores["ANNIEEvent"]->Set("RunNumber",RunNumber);
	m_data->Stores["ANNIEEvent"]->Set("SubrunNumber",SubrunNumber);
	m_data->Stores["ANNIEEvent"]->Set("EventNumber",MCEventNum);
	if(verbose>2) cout<<"particles"<<endl;
	m_data->Stores["ANNIEEvent"]->Set("MCParticles",MCParticles,true);
	if(verbose>2) cout<<"hits"<<endl;
	m_data->Stores["ANNIEEvent"]->Set("MCHits",MCHits,true);
	if(verbose>2) cout<<"tdcdata"<<endl;
	m_data->Stores["ANNIEEvent"]->Set("TDCData",TDCData,true);
	if(verbose>2) cout<<"triggerdata"<<endl;
	m_data->Stores["ANNIEEvent"]->Set("TriggerData",TriggerData,true);  // FIXME
	if(verbose>2) cout<<"eventtime"<<endl;
	m_data->Stores["ANNIEEvent"]->Set("EventTime",EventTime,true);
	m_data->Stores["ANNIEEvent"]->Set("MCEventNum",MCEventNum);
	m_data->Stores["ANNIEEvent"]->Set("MCTriggernum",MCTriggernum);
	m_data->Stores["ANNIEEvent"]->Set("MCFile",MCFile);
	m_data->Stores["ANNIEEvent"]->Set("MCFlag",true);                   // constant 
	m_data->Stores["ANNIEEvent"]->Set("BeamStatus",BeamStatus,true);
	//Things that need to be set by later tools:
	//RawADCData
	//CalibratedADCData
	//RawLAPPDData
	//CalibratedLAPPDData
	//RecoParticles
	
	// this should be everything. save the entry to the BoostStore
	if(verbose>2) cout<<"saving"<<endl;
	m_data->Stores["ANNIEEvent"]->Save();
	
	MCTriggernum++;
	if(verbose>2) cout<<"checking if we're done on trigs in this event"<<endl;
	if(MCTriggernum==WCSimEntry->wcsimrootevent->GetNumberOfEvents()){
		MCTriggernum=0;
		MCEventNum++;
		if(verbose>2) cout<<"new event"<<endl;
	}
	if(MCEventNum==NumEvents) m_data->vars.Set("StopLoop",1);
	/*if(verbose>1)*/ cout<<"done loading event"<<endl;
	return true;
}


bool LoadWCSim::Finalise(){
	
	
	
	return true;
}
