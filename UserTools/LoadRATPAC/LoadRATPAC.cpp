#include "LoadRATPAC.h"

LoadRATPAC::LoadRATPAC():Tool(){}


bool LoadRATPAC::Initialise(std::string configfile, DataModel &data){

  /////////////////// Usefull header ///////////////////////
  if(configfile!="")  m_variables.Initialise(configfile); //loading config file
	
  if(verbose) std::cout<<"Initializing Tool LoadRATPAC"<<std::endl;

  m_data= &data; //assigning transient data pointer
  /////////////////////////////////////////////////////////////////
	m_variables.Get("InputFile",filename_ratpac);
 

  // Initialize the TChain reading event info from our file
  //chain = new TChain("T");
  //chain->Add(filename_ratpac);
  //Initialize the DSReader
  dsReader = new RAT::DSReader(filename_ratpac.c_str()); // cuz stupid dsReader needs to be initialize with a filename..
  
  //Initialize run info
  std::cout<<"Initializing the run information"<<std::endl;
  
  //tri = new TChain("T");
  runtri = new TChain("runT");

  TFile *ftemp = TFile::Open(filename_ratpac.c_str());
  if (ftemp->Get("runT")) {
    runtri->Add(filename_ratpac.c_str());
    RAT::DS::RunStore::SetReadTree(runtri);
  } else {
      std::cout<<"File does not have a run tree!  Not the right format file"<<std::endl;
     return false; // else, no runT, so don't register runtri with RunStore
  }
  delete ftemp;
  //RAT::DS::Root *branchDS = new RAT::DS::Root();
  //tri->SetBranchAddress("ds", &branchDS);
  //RAT::DS::RunStore::GetRun(branchDS);
  
  ds = dsReader->GetEvent(0);
  run = new RAT::DS::Run;
  run = RAT::DS::RunStore::Get()->GetRun(ds);
  RunNumber = static_cast<uint32_t>(run->GetID());
  SubrunNumber = 0; 
  pmtInfo = new RAT::DS::PMTInfo;
  pmtInfo = run->GetPMTInfo();
  lappdInfo = new RAT::DS::LAPPDInfo;
  lappdInfo = run->GetLAPPDInfo();
  
  //Total number of events available to be read
  NbEntries = dsReader->GetTotal();
	
  // things to be saved to the ANNIEEvent Store
	MCParticles = new std::vector<MCParticle>;
  MCLAPPDHits = new std::map<ChannelKey,std::vector<LAPPDHit>>;
	MCHits = new std::map<ChannelKey,std::vector<Hit>>;
  
  this->LoadANNIEGeometry();

	//FIXME: No triggers in RAT-PAC yet, but should be fine without for now
  //TriggerData = new std::vector<TriggerClass>{beamtrigger}; 
  
  //Counter corresponding to Store number
	EventNumber=0;

  //Counter corresponding to event number in MC
  //Without tracking any detector triggers, is the same as EventNum
	MCEventNum=0;

  //Currently, RAT-PAC has no trigger really; just per event.  Keep this zero
	MCTriggernum=0;

  //FIXME: Fill in the run start time as GetUTC from first event?  Or is in runT?
  time_t runstarttime = run->GetStartTime();
  uint64_t runstart = static_cast<uint64_t>(runstarttime);
  TimeClass RunStartTime(runstart);
	
  // use nominal beam values for now.  Eventually, will be loaded in data
	double beaminten=4.777e+12;
	double beampow=3.2545e+16;
	BeamStatus = new BeamStatusClass(RunStartTime, beaminten, beampow, "stable");
  
  if(verbose) std::cout<<"Initialization of Tool LoadRATPAC Complete"<<std::endl;
  return true;
}


bool LoadRATPAC::Execute(){
  if(verbose) std::cout<<"Executing Tool LoadRATPAC"<<std::endl;
  // First, reset all counters kept track of per event
  this->Reset();
  
  // Starts the timer to load this event
  start = clock();

  TH1::SetDefaultSumw2(kTRUE);
  if (EventNumber >= NbEntries){
    std::cout << "LoadRATPAC tool: end of file reached.  Returning true" << std::endl;
    return true;
  }
  //chain->GetEntry(EventNumber); 
  ds = dsReader->GetEvent(EventNumber);

  //FIXME: WCSim gives in trigger data, but we don't have triggers in RAT-PAC yet
  uint64_t  EventTimeNs = 0; //FIXME: Should be called with chain->GetUTC() when we do have evnet times?
	EventTime->SetNs(EventTimeNs);

  //PMT loop
  for( size_t iPMT = 0; iPMT < ds->GetMC()->GetMCPMTCount(); iPMT++ ){
    if(verbose>2) std::cout<<"getting PMT "<<iPMT<<std::endl;
    RAT::DS::MCPMT *aPMT = ds->GetMC()->GetMCPMT(iPMT);
    int tubeid = aPMT->GetID();
	  if(verbose>2) cout<<"tubeid="<<tubeid<<endl;
    //Loop over photons that hit the PMT for digits
    ChannelKey key(subdetector::ADC, tubeid);
    for(long iPhot = 0; iPhot < aPMT->GetMCPhotonCount(); iPhot++){
      float hitcharge = aPMT->GetMCPhoton(iPhot)->GetCharge();
      float hittime = aPMT->GetMCPhoton(iPhot)->GetHitTime(); //Time relative to event start (ns)
      Hit thishit(tubeid, hittime, hitcharge);
      if(MCHits->count(key)==0) MCHits->emplace(key, std::vector<Hit>{thishit});
      else MCHits->at(key).push_back(thishit);
      if(verbose>2) cout<<"digit added"<<endl;
    }
  }
  std::cout << "Done loading event's PMT hits" << std::endl;

  //LAPPD loop
  for( size_t iLAPPD = 0; iLAPPD < ds->GetMC()->GetMCLAPPDCount(); iLAPPD++ ){
    if(verbose>2) std::cout<<"getting LAPPD "<<iLAPPD<<std::endl;
    RAT::DS::MCLAPPD *aLAPPD = ds->GetMC()->GetMCLAPPD(iLAPPD);
    int lappdid = aLAPPD->GetID();
	  if(verbose>2) cout<<"tubeid="<<lappdid<<endl;
    //Loop over photons that hit the LAPPD for digits
    ChannelKey key(subdetector::ADC, lappdid);
    for(long iPhot = 0; iPhot < aLAPPD->GetMCPhotonCount(); iPhot++){
      float hitcharge = aLAPPD->GetMCPhoton(iPhot)->GetCharge();
      float hittime = aLAPPD->GetMCPhoton(iPhot)->GetHitTime(); //Time relative to event start (ns)
      std::vector<double> lappdPosition;
      double lappdx = lappdInfo->GetPosition(lappdid).X();
      double lappdy = lappdInfo->GetPosition(lappdid).Y();
      double lappdz = lappdInfo->GetPosition(lappdid).Z();
      lappdPosition.push_back(lappdx);
      lappdPosition.push_back(lappdy);
      lappdPosition.push_back(lappdz);
      std::vector<double> localPosition;
      double localx = aLAPPD->GetMCPhoton(iPhot)->GetPosition().X();
      double localy = aLAPPD->GetMCPhoton(iPhot)->GetPosition().Y();
      double localz = aLAPPD->GetMCPhoton(iPhot)->GetPosition().Z();
      localPosition.push_back(localx);
      localPosition.push_back(localy);
      localPosition.push_back(localz);
      LAPPDHit thishit(lappdid, hittime, hitcharge, lappdPosition, localPosition);
      if(MCLAPPDHits->count(key)==0) MCLAPPDHits->emplace(key, std::vector<LAPPDHit>{thishit});
      else MCLAPPDHits->at(key).push_back(thishit);
      if(verbose>2) cout<<"LAPPD hit added"<<endl;
    }
  }
  std::cout << "Done loading event's LAPPD hits" << std::endl;

  if(verbose>1) cout<<"getting "<<ds->GetMC()->GetMCTrackCount()<<" tracks"<<endl;
  //Loop through all the tracks and get particle information.  We'll load it
  //Load MC Parent Particles
	tracktype startstoptype = tracktype::UNDEFINED;
  for (long iTrack=0; iTrack<ds->GetMC()->GetMCTrackCount(); iTrack++){
    RAT::DS::MCTrack  *thistrack = ds->GetMC()->GetMCTrack(iTrack);
    Int_t parentid = thistrack->GetParentID();
    Int_t pdgcode = thistrack->GetPDGCode();
    std::string particlename = thistrack->GetParticleName();
    if(particlename=="opticalphoton") continue; //Do not save optical photons
    Float_t tracklength = thistrack->GetLength();
    RAT::DS::MCTrackStep* firstStep = thistrack->GetMCTrackStep(0);
    RAT::DS::MCTrackStep* lastStep = thistrack->GetMCTrackStep(thistrack->GetMCTrackStepCount()-1); 
    TVector3 endpoint = lastStep->GetEndpoint();
    Float_t EndKE = lastStep->GetKE();
    Double_t endtime = lastStep->GetGlobalTime();
    TVector3 startpoint = firstStep->GetEndpoint();
    Double_t starttime = firstStep->GetGlobalTime();
    Float_t StartKE = firstStep->GetKE();
    TVector3 particledir = firstStep->GetMomentum();
    particledir = particledir.Unit();
    //Load information into MCParticle class used in ToolAnalysis
    MCParticle thisparticle(
            pdgcode, StartKE, EndKE, Position(startpoint.X()/1000., 
            startpoint.Y(), startpoint.Z()), Position(endpoint.X()/1000.,
            endpoint.Y()/1000., endpoint.Z()/1000.), starttime-EventTimeNs,
            endtime-EventTimeNs, Direction(particledir.X(), particledir.Y(),
            particledir.Z()), tracklength, startstoptype, iTrack, parentid);
    MCParticles->push_back(thisparticle);
  }


	// set event level variables
	if(verbose>1) std::cout<<"setting the store variables"<<std::endl;
	m_data->Stores.at("ANNIEEvent")->Set("RunNumber",RunNumber);
	m_data->Stores.at("ANNIEEvent")->Set("SubrunNumber",SubrunNumber);
	m_data->Stores.at("ANNIEEvent")->Set("EventNumber",EventNumber);
	if(verbose>2) cout<<"particles"<<endl;
	m_data->Stores.at("ANNIEEvent")->Set("MCParticles",MCParticles,true);
	if(verbose>2) cout<<"hits"<<endl;
	m_data->Stores.at("ANNIEEvent")->Set("MCHits",MCHits,true);
	if(verbose>2) cout<<"tdcdata"<<endl;
	//m_data->Stores.at("ANNIEEvent")->Set("TDCData",TDCData,true);
	//if(verbose>2) cout<<"triggerdata"<<endl;
	//m_data->Stores.at("ANNIEEvent")->Set("TriggerData",TriggerData,true);  // FIXME
	if(verbose>2) cout<<"eventtime"<<endl;
	m_data->Stores.at("ANNIEEvent")->Set("EventTime",EventTime,true);
	m_data->Stores.at("ANNIEEvent")->Set("MCEventNum",MCEventNum);
	m_data->Stores.at("ANNIEEvent")->Set("MCTriggernum",MCTriggernum);
	m_data->Stores.at("ANNIEEvent")->Set("MCFile",filename_ratpac);
	m_data->Stores.at("ANNIEEvent")->Set("MCFlag",true);           // constant
	m_data->Stores.at("ANNIEEvent")->Set("BeamStatus",BeamStatus,true);

  EventNumber+=1;
  MCEventNum+=1;
  // Ends the timer for tool's execute
  duration = ( clock() - start ) / (double) CLOCKS_PER_SEC;
  cout << "Execution time: " << duration << " seconds\n";
  return true;
}


bool LoadRATPAC::Finalise(){
  save.Clear();
  log.Clear();
  
  delete chain;
  delete dsReader;
  
  // Ends the timer
  duration = ( clock() - start ) / (double) CLOCKS_PER_SEC;
  std::cout << "Execution time: " << duration << " seconds\n";
  return true;
}

void LoadRATPAC::Reset(){
  // Initialization time
	MCParticles->clear();
	MCHits->clear();
  MCLAPPDHits->clear();
}

void LoadRATPAC::LoadANNIEGeometry(){	
  // Make the ANNIEEvent Store if it doesn't exist
	// =============================================
	int annieeventexists = m_data->Stores.count("ANNIEEvent");
	if(annieeventexists==0) m_data->Stores["ANNIEEvent"] = new BoostStore(false,2);

  std::cout<<"LoadRATPAC tool: Loading the ANNIE geometry now"<<std::endl;  
	// construct the Geometry to go in the header Based on the RATPAC Geometry
	// =================================================================
	double RATPACGeometryVersion = 1;                       // TODO pull this from some suitable variable
  
  int numtankpmts = pmtInfo->GetPMTCount();
  int numlappds = lappdInfo->GetLAPPDCount();
	int nummrdpmts = 0;
  int numvetopmts = 0;
  
  //TODO: Have these values saved to the RATDS? Hard-coded for now
  //
  double tank_xcenter = (0.0) / 1000.;  // convert [mm] to [m]
	double tank_ycenter = (0.0) / 1000.;
	double tank_zcenter = (1724.0) / 1000.;
	Position tank_center(tank_xcenter, tank_ycenter, tank_zcenter);
	double tank_radius = (1524.0) / 1000.;
	double tank_halfheight = (1981.2) / 1000.;
	//geometry variables not yet in RATPAC grabbed from MRDSpecs.hh
	double mrd_width =  (MRDSpecs::MRD_width) / 100.; // convert [cm] to [m]
	double mrd_height = (MRDSpecs::MRD_height) / 100.;
	double mrd_depth =  (MRDSpecs::MRD_depth) / 100.;
	double mrd_start =  (MRDSpecs::MRD_start) / 100.;
	if(verbose>1) cout<<"we have "<<numtankpmts<<" tank pmts, and "<<numlappds<<" lappds"<<endl;
	
	// loop over PMTs and make the map of Detectors
	std::map<ChannelKey,Detector> Detectors;
	// tank pmts
	for(int i=0; i<numtankpmts; i++){
		ChannelKey akey(subdetector::ADC, i);
    TVector3 pmtpos = pmtInfo->GetPosition(i);
    TVector3 pmtdir = pmtInfo->GetDirection(i);
    int modelType = pmtInfo->GetModel(i);
		Detector adet("Tank", Position(pmtpos.X()/1000.,pmtpos.Y()/1000.,pmtpos.Z()/1000.),
		               Direction(pmtdir.X(),pmtdir.Y(),pmtdir.Z()),
		               i, pmtInfo->GetModelName(modelType) , detectorstatus::ON, 0.);
		Detectors.emplace(akey,adet);
	}
	// lappds
	for(int i=0; i<numlappds; i++){
		ChannelKey akey(subdetector::LAPPD, i);
    TVector3 lappdpos = lappdInfo->GetPosition(i);
    TVector3 lappddir = lappdInfo->GetDirection(i);
    int modelType = lappdInfo->GetModel(i);
		Detector adet("Tank", Position(lappdpos.X()/1000.,lappdpos.Y()/1000.,lappdpos.Z()/1000.),
		               Direction(lappddir.X(),lappddir.Y(),lappddir.Z()),
		               i, lappdInfo->GetModelName(modelType) , detectorstatus::ON, 0.);
		Detectors.emplace(akey,adet);
	}

	// construct the goemetry 
	Geometry* anniegeom = new Geometry(Detectors, RATPACGeometryVersion, tank_center, tank_radius,
	                           tank_halfheight, mrd_width, mrd_height, mrd_depth, mrd_start,
	                           numtankpmts, nummrdpmts, numvetopmts, numlappds, detectorstatus::ON);
	if(verbose>1){ cout<<"constructed anniegom at "<<anniegeom<<" with tank origin "; tank_center.Print(); }
	m_data->Stores.at("ANNIEEvent")->Header->Set("AnnieGeometry",anniegeom,true);
}	
