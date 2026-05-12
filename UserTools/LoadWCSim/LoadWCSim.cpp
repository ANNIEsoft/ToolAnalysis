#include "LoadWCSim.h"

LoadWCSim::LoadWCSim():Tool(){}

bool LoadWCSim::Initialise(std::string configfile, DataModel &data)
{	
  /////////////////// Useful header ///////////////////////
  if (configfile!="") m_variables.Initialise(configfile); //loading config file
  //m_variables.Print();
	
  m_data = &data; //assigning transient data pointer
	
  // Get the Tool configuration variables and set defaults
  // ======================================================
  if (!m_variables.Get("verbose", verbosity)) verbosity = 1;
  logmessage = "LoadWCSim::Initialise: Initialising LoadWCSim!";
  Log(logmessage, v_warning, verbosity);	
	
  if (!m_variables.Get("MaxEntries", MaxEntries)) MaxEntries = -1;
  
  if (!m_variables.Get("InputFile", MCFile)) {
    logmessage = "LoadWCSim::Initialise: NO InputFile set in the config!";
    Log(logmessage, v_error, verbosity);
    return false;
  }

  if (!m_variables.Get("HistoricTriggeroffset", HistoricTriggeroffset)) {
    logmessage = "LoadWCSim::Initialise: NO HistoricTriggeroffset set in the config! Aborting!";
    Log(logmessage, v_error, verbosity);
    return false;
  }
  
  if (!m_variables.Get("WCSimVersion", WCSimVersion)) {
    logmessage = "LoadWCSim::Initialise: WCSimVersion not set in the config! Aborting!";
    Log(logmessage, v_error, verbosity);
    return false;
  }
  m_data->CStore.Set("WCSimVersion", WCSimVersion);
  
  if (!m_variables.Get("UseDigitSmearedTime", use_smeared_digit_time)) {
    use_smeared_digit_time = 1;
    logmessage  = "LoadWCSim::Initialise: UseDigitSmearedTime flag not set in the config.";
    logmessage += "Using default value: " + std::to_string(use_smeared_digit_time);
    Log(logmessage, v_warning, verbosity);
    
  }

  if (!m_variables.Get("LappdNumStrips", LappdNumStrips)) {
    LappdNumStrips = 56;
    logmessage  = "LoadWCSim::Initialise: LappdNumStrips not set in config. ";
    logmessage += "Using default value: " + std::to_string(LappdNumStrips);
    Log(logmessage, v_warning, verbosity);
  }
  
  if(!m_variables.Get("LappdStripLength", LappdStripLength)){
    LappdStripLength = 200;
    logmessage  = "LoadWCSim::Initialise: LappdStripLength not set in config. ";
    logmessage += "Using default value: " + std::to_string(LappdStripLength);
    Log(logmessage, v_warning, verbosity);
  }
  
  if (!m_variables.Get("LappdStripSeparation", LappdStripSeparation)) {
    LappdStripSeparation = 7.14;
    logmessage  = "LoadWCSim::Initialise: LappdStripSeparation not set in config. ";
    logmessage += "Using default value: " + std::to_string(LappdStripSeparation);
    Log(logmessage, v_warning, verbosity);
  }

  if (!m_variables.Get("RunStartDate", RunStartUser)) {
    RunStartUser = 0;
    logmessage  = "LoadWCSim::Initialise: RunStartUser not set in config. ";
    logmessage += "Using default value: " + std::to_string(LappdStripSeparation) + " ns since unix epoch";
    Log(logmessage, v_warning, verbosity);
  }

  if (!m_variables.Get("SplitSubTriggers", splitSubtriggers)) {
    splitSubtriggers = false;
    logmessage  = "LoadWCSim::Initialise: splitSubtriggers flag not set in config. ";
    logmessage += "Using default value: " + std::to_string(splitSubtriggers);
    Log(logmessage, v_warning, verbosity);
  }
  m_data->CStore.Set("SplitSubTriggers", splitSubtriggers);
  
  if (!m_variables.Get("TriggerType", TriggerType)) {
    TriggerType = "Beam";
    logmessage  = "LoadWCSim::Initialise: TriggerType not set in config. ";
    logmessage += "Using default value: " + TriggerType;
    Log(logmessage, v_warning, verbosity);
  }
	
  if (!m_variables.Get("TriggerWord", TriggerWord)) {
    TriggerWord = 5;
    logmessage  = "LoadWCSim::Initialise: TriggerWord not set in config. ";
    logmessage += "Using default value: " + std::to_string(TriggerWord) + " (ie. Beam)";
    Log(logmessage, v_warning, verbosity);
  }

  if (!m_variables.Get("RunType", RunType)) {
    RunType = 3;
    logmessage  = "LoadWCSim::Initialise: RunType not set in config. ";
    logmessage += "Using default value: " + std::to_string(RunType) + " (ie. Beam)";
    Log(logmessage, v_warning, verbosity);
  }

  if (!m_variables.Get("PMTMask", PMTMask)) {
    PMTMask = "None";
    logmessage  = "LoadWCSim::Initialise: RunType not set in config. ";
    logmessage += "Using default value: " + PMTMask;
    Log(logmessage, v_warning, verbosity);
  }
  if (PMTMask != "None") masked_ids = this->LoadPMTMask(PMTMask);

  if (!m_variables.Get("FileStartOffset", WCSimEntryNum)) {
    WCSimEntryNum = 0;
    logmessage  = "LoadWCSim::Initialise: FileStartOffset not set in config. ";
    logmessage += "Using default value: " + std::to_string(WCSimEntryNum);
    Log(logmessage, v_warning, verbosity);
  }

  // TODO: should not use relative path.
  // There should be an env var that points to the top directory
  if (!m_variables.Get("ChankeyToPMTIDMap", path_chankeymap)) {
    path_chankeymap = "./configfiles/LoadWCSim/Chankey_WCSimID_v7.txt";
    logmessage  = "LoadWCSim::Initialise: ChankeyToPMTIDMap not set in config. ";
    logmessage += "Using default path: " + path_chankeymap;
    Log(logmessage, v_warning, verbosity);
  } 
  ifstream file_pmtid(path_chankeymap.c_str());  
  if (file_pmtid.is_open()){
    // watch out: comment or empty lines not supported here
    while (!file_pmtid.eof()) {
      unsigned long chankey;
      int pmtid;
      file_pmtid >> chankey >> pmtid;
      channelkey_to_pmtid.emplace(chankey,pmtid);
      pmtid_to_channelkey.emplace(pmtid,chankey);
    }
		
    file_pmtid.close();
    m_data->CStore.Set("pmt_tubeid_to_channelkey_data", pmtid_to_channelkey);
    m_data->CStore.Set("channelkey_to_pmtid_data", channelkey_to_pmtid);
  } else {
    logmessage  = "LoadWCSim::Initialise: PMT ID Configuration file: " + path_chankeymap;
    logmessage += " could not be opened! Is the path valid? Aborting!";
    Log(logmessage, v_error, verbosity);
    return false;
  }

  // TODO: should not use relative path.
  // There should be an env var that points to the top directory
  if (!m_variables.Get("ChankeyToMRDIDMap", path_mrd_chankeymap)) {
    path_mrd_chankeymap = "./configfiles/LoadWCSim/MRD_Chankey_WCSimID.dat";
    logmessage  = "LoadWCSim::Initialise: ChankeyToMRDIDMap not set in config. ";
    logmessage += "Using default path: " + path_mrd_chankeymap;
    Log(logmessage, v_warning, verbosity);
  } 
  ifstream file_mrdid(path_mrd_chankeymap.c_str());
  if (file_mrdid.is_open()){
    // watch out: comment or empty lines not supported here
    while (!file_mrdid.eof()){
      unsigned long chankey;
      int mrdid;
      file_mrdid >> chankey >> mrdid;
      mrdid_to_channelkey.emplace(mrdid,chankey);
    }
    file_mrdid.close();
  } else {
    logmessage  = "LoadWCSim::Initialise: MRD ID Configuration file: " + path_mrd_chankeymap;
    logmessage += " could not be opened! Is the path valid? Aborting!";
    Log(logmessage, v_error, verbosity);
    return false;
  }
  
  // TODO: should not use relative path.
  // There should be an env var that points to the top directory
  if (!m_variables.Get("ChankeyToFMVIDMap", path_fmv_chankeymap)) {
    path_fmv_chankeymap = "./configfiles/LoadWCSim/FMV_Chankey_WCSimID.dat";
    logmessage  = "LoadWCSim::Initialise: ChankeyToFMVIDMap not set in config. ";
    logmessage += "Using default path: " + path_fmv_chankeymap;
    Log(logmessage, v_warning, verbosity);
  } 
  ifstream file_fmvid(path_fmv_chankeymap.c_str());
  if (file_fmvid.is_open()){
    // watch out: comment or empty lines not supported here
    while (!file_fmvid.eof()){
      unsigned long chankey;
      int fmvid;
      file_fmvid >> chankey >> fmvid;
      fmvid_to_channelkey.emplace(fmvid,chankey);
    }
    file_fmvid.close();
  } else {
    logmessage  = "LoadWCSim::Initialise: FMV ID Configuration file: " + path_fmv_chankeymap;
    logmessage += " could not be opened! Is the path valid? Aborting!";
    Log(logmessage, v_error, verbosity);
    return false;
  }
	
  // Short Stores README
  // ======================================================
  // n.b. m_data->vars is a Store (of ben's Store type) that is not saved to disk.
  //      m_data->CStore is a single entry binary BoostStore that is not saved to disk.
  //      m_data->Stores["StoreName"] is a map of binary BoostStores that are saved to disk.
  // With BoostStore::Set("MyVariable", myvar), if myvar is not a pointer it will always
  // be saved to disk, but if myvar is a pointer the persist flag (default true) controls
  // whether it will be saved to disk. In either case the BoostStore becomes the owner
  // of the object and will handle its deletion.
  // Is 'BoostStore::Save' needed for single-entry stores?
  // ----------------
  // BoostStore constructor args: typechecking (bool), m_format (0=binary, 1=ASCII, 2=multievent)
  // A BoostStore has a header where useful constants may be saved. The header is a BoostStore itself,
  // and can be accessed via: Store.Header->Get() and Store.Header->Set().
  // The method 'BoostStore::Save()' writes everything 'Set' since the last 'Save' to the current entry.
  // BoostStore::Clear clears the map of the current entry, to start building a new one.
  // Use 'BoostStore::GetEntry(int entrynum)' to load an entry to then be able to 'Get' it's contents.
  // 'BoostStore->Header->Get("TotalEntries",NumEvents)' will load the num entries into NumEvents
  // ------------------
  // When adding a BoostStore (or class object in general) to a BoostStore (such as ANNIEEvent)
  // you call BoostStore::Set("key",ObjectPointer) - BUT be aware that serialization happens when the
  // 'Set' method is called - although you pass it a pointer, any subsequent changes to the object
  // will NOT get saved! You must call 'Set' AFTER making ALL changes to your object!
  /////////////////////////////////////////////////////////
  
  // Make class private members; e.g. the WCSimT and WCSimRootGeom
  WCSimEntry = new wcsimT(MCFile.c_str(),verbosity);
  gROOT->cd();

  // Make the ANNIEEvent Store if it doesn't exist
  // =============================================
  if (m_data->Stores.count("ANNIEEvent") == 0)
    m_data->Stores["ANNIEEvent"] = new BoostStore(false, 2);

  MCFile = WCSimEntry->GetCurrentFile()->GetName();
  m_data->Stores.at("ANNIEEvent")->Set("MCFile", MCFile);
    
  // Grab the geom and record it for LAPPD reader to use later
  wcsimrootgeom = WCSimEntry->wcsimrootgeom;
  intptr_t geomptr = reinterpret_cast<intptr_t>(wcsimrootgeom);
  m_data->CStore.Set("WCSimRootGeom", geomptr);
  std::ostringstream ss_wcsimrootgeom;
  std::ostringstream ss_geomptr;
  ss_wcsimrootgeom << wcsimrootgeom;
  ss_geomptr << geomptr;
  logmessage  = "LoadWCSim::Initialise: WCSimRootGeom at: " + ss_wcsimrootgeom.str();
  logmessage += ", and geomptr at: " + ss_geomptr.str();
  Log(logmessage, v_message, verbosity);
  
  // Convert WCSimRootGeom into ToolChain Geometry class
  Geometry* anniegeom = ConstructToolChainGeometry();

  // Grab root options and record them
  wcsimrootopts = WCSimEntry->wcsimrootopts;
  int pretriggerwindow  = wcsimrootopts->GetNDigitsPreTriggerWindow();
  int posttriggerwindow = wcsimrootopts->GetNDigitsPostTriggerWindow();
  m_data->CStore.Set("WCSimPreTriggerWindow",  pretriggerwindow);
  m_data->CStore.Set("WCSimPostTriggerWindow", posttriggerwindow);

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
    WCSimEntryNum
    MCFile
    BeamStatus
  */

  EventNumber = 0;
  MCTriggerNum = 0;
  
  // pull the first entry so it's ready to look at during the first execute loop
  int nbytesread = WCSimEntry->GetEntry(WCSimEntryNum); 
  if (nbytesread <= 0) {
    logmessage  = "LoadWCSim::Initialise: Issue loading event number: ";
    logmessage += std::to_string(WCSimEntryNum) + ". Aborting!";
    Log(logmessage, v_error, verbosity);
    
    if (nbytesread == -4) {
      logmessage  = "LoadWCSim::Initialise: Overran the end of TChain! ";
      logmessage += "Have you specified more iterations than are available in ToolChainConfig?";
      Log(logmessage, v_error, verbosity);
    }

    if (nbytesread==0) {
      logmessage  = "LoadWCSim::Initialise: No TChain loaded! Is your filepath correct?";
      Log(logmessage, v_error, verbosity);
    }
   
    m_data->vars.Set("StopLoop", 1);
    return false;
  }

  // TODO: use nominal beam values 
  double beaminten = 4.777e+12;
  double beampow = 3.2545e+16;
  uint64_t beamtimestamp = 0;
  RunStartTime.SetNs(RunStartUser);
  beamstat.set_time(TimeClass(beamtimestamp));
  beamstat.set_pot(beampow);
  BeamCondition bc = BeamCondition::Ok;
  beamstat.set_condition(bc);

  
  // Construct the other objects we'll be setting at event level,
  // pass managed pointers to the ANNIEEvent Store
  // =============================================
  MCParticles = new std::vector<MCParticle>;
  MCHits = new std::map<unsigned long,std::vector<MCHit>>;
  TDCData = new std::map<unsigned long,std::vector<MCHit>>;
  EventTime = new TimeClass();

  // FIXME ? one trigger and resetting time is ok?
  // TriggerClass should have a lower case type
  std::string triggertype = TriggerType; 
  std::transform(triggertype.begin(), triggertype.end(), triggertype.begin(),
        [](unsigned char c){ return std::tolower(c); }); 

  TriggerClass beamtrigger(triggertype, TriggerWord, 0, true, 0);
  TriggerData = new std::vector<TriggerClass>{beamtrigger}; 
	
  // we'll put these in the CStore: so don't delete them in Finalise! It'll get handled by the Store
  ParticleId_to_TankTubeIds  = new std::map<int,std::map<unsigned long,double>>;
  ParticleId_to_MrdTubeIds   = new std::map<int,std::map<unsigned long,double>>;
  ParticleId_to_VetoTubeIds  = new std::map<int,std::map<unsigned long,double>>;
  ParticleId_to_TankCharge   = new std::map<int,double>;
  ParticleId_to_MrdCharge    = new std::map<int,double>;
  ParticleId_to_VetoCharge   = new std::map<int,double>;
  trackid_to_mcparticleindex = new std::map<int,int>;
  mapNeutronIsPrim = new std::map<int,bool>;

  // If true then other tools can select a specific event number
  m_data->CStore.Set("UserEvent", false);   
  trigsInEntry = 0;

  return true;
}

// Overview of process to extract WCSim files:
// The wcsimT object is a tree that we load into WCSimEntry.
//   It contains WCSimRootEvent branches for the tank, MRD, and veto
//   (along with geom. and root options).
// Each G4 "event" is obtained by using wcsimT::GetEntry(),
//   which loads the specified entry from the WCSimRootEvent branches.
//   We loaded the first entry in the initialize stage so we're ready
//   at the first execute. We then load the next entry at the end of
//   Execute (depending on the splitSubtriggers flag (see below))
// WCSimRootEvent objects contain "triggers" 
//   The first trigger contains all the particles initially passed G4.
//   Subsequent triggers contain delayed particles (eg. from decays).
// If splitSubtriggers is set to True then each Execute loop will
//  add one subtrigger as a single event.
// If splitSubtriggers is set to False then each Execute loop will
//  add all subtriggers as a single event.
// In this way the WCSimEntryNum may not match the EventNumber assigned by LoadWCSim.
bool LoadWCSim::Execute()
{
  // Check if another tool has specified an entry and trigger number to load.
  // Currently e.g. the EventDisplay has the ability to do that
  // =============================================
  bool userEvent;
  m_data->CStore.Get("UserEvent", userEvent);
  if (userEvent) {
    // Grab the user-specified entry
    if (m_data->CStore.Get("LoadEvNr", WCSimEntryNum)) {
      logmessage = "LoadWCSim::Execute: Going to the user-defined event number: ";
      logmessage += std::to_string(WCSimEntryNum);
      Log(logmessage, v_message, verbosity);
    } else {
      logmessage  = "LoadWCSim::Execute: Requested to a user-defined event number, ";
      logmessage += "but LoadEvNr is not set in the CStore.";
      Log(logmessage, v_error, verbosity);
      return false;
    }
      
    // Reset the flag and go to the first trigger in that event
    m_data->CStore.Set("UserEvent", false);
    MCTriggerNum = 0;

    // Check if an additional trigger is available and which trigger was requested
    bool checkFurtherTriggers = false;
    uint16_t currentTriggerNum;
    m_data->CStore.Get("CheckFurtherTriggers", checkFurtherTriggers);
    m_data->CStore.Get("CurrentTriggernum", currentTriggerNum);
    ++currentTriggerNum;

    logmessage = "LoadWCSim::Execute: Check further triggers: " + std::to_string(checkFurtherTriggers);
    logmessage += ", current trigger num: " + std::to_string(currentTriggerNum);
    logmessage += "\n total number triggers: " + std::to_string(trigsInEntry);
    Log(logmessage, v_debug, verbosity);

    // If there is a further trigger in the previous event then we'll load that entry
    if (checkFurtherTriggers && currentTriggerNum != trigsInEntry) {      
      --WCSimEntryNum;
      MCTriggerNum = currentTriggerNum;
    }
    
    // Load the desired entry
    // If this is the last one in the chain then stop the loop
    if ((int)WCSimEntryNum >= MaxEntries && MaxEntries > 0) {
      logmessage = "LoadWCSim::Execute: Reached max entries specified in the config file. Terminating ToolChain";
      Log(logmessage, v_error, verbosity);
      m_data->vars.Set("StopLoop", 1);
    } else {
      int nBytesRead = WCSimEntry->GetEntry(WCSimEntryNum);  // <0 if out of file

      logmessage  = "LoadWCSim::Execute: Trying to get next event, WCSimEntryNum: " + std::to_string(WCSimEntryNum);
      logmessage += ", N bytes read: " + std::to_string(nBytesRead);
      Log(logmessage, v_debug, verbosity);

      if (nBytesRead <= 0) {
		logmessage = "LoadWCSim Tool: Reached last entry of WCSim input file, terminating ToolChain";
		Log(logmessage, v_warning, verbosity);
		m_data->vars.Set("StopLoop", 1);
		return true;
      }
    }
  }// endif userEvent

  logmessage = "LoadWCSim::Execute: Executing with MC entry: " + std::to_string(WCSimEntryNum);
  logmessage += " and trigger: " + std::to_string(MCTriggerNum);
  Log(logmessage, v_warning, verbosity);

  // setting StopLoop doesn't terminate the ToolChain if the number of iterations
  // is specified manually in the ToolChainConfig.
  // This is almost certainly going to result in a segfault somewhere,
  // (e.g. if this tool set it in the last loop iteration because it ran out of entries)
  // but let's do what we can
  int loopStopped = 0;
  get_ok = m_data->vars.Get("StopLoop",loopStopped);
  if (get_ok && loopStopped){
    logmessage = "LoadWCSim::Execute: WARNING: STOPLOOP HAS BEEN SET. RETURNING";
    Log(logmessage, v_warning, verbosity);
    return false;
  }

  // Start extracting the things from the WCSim file
  // =============================================
  MCFile = WCSimEntry->GetCurrentFile()->GetName();
    
  // Clean slate
  TDCData->clear();
  MCHits->clear();
  MCNeutCap.clear();
  MCNeutCapGammas.clear();
  //MCHitsToParticles.clear();
  mrd_firstlayer = false;
  mrd_lastlayer  = false;	
  
  // CherenkovHit(Times) are all in first trigger so just grab them once per execute
  WCSimRootTrigger* firstTrigTank = WCSimEntry->wcsimrootevent     ->GetTrigger(0);
  WCSimRootTrigger* firstTrigMRD  = WCSimEntry->wcsimrootevent_mrd ->GetTrigger(0);
  WCSimRootTrigger* firstTrigVeto = WCSimEntry->wcsimrootevent_facc->GetTrigger(0);
 
  // Grab GENIE info and record it
  std::string genieFilename = firstTrigTank->GetHeader()->GetGenieFileName().Data();
  int genieEntry = firstTrigTank->GetHeader()->GetGenieEntryNum();
  m_data->CStore.Set("GenieFile",genieFilename);
  m_data->CStore.Set("GenieEntry",genieEntry);
  
  logmessage = "LoadWCSim::Execute: GENIE file is " + genieFilename;
  logmessage += ", GENIE evt num is: " + std::to_string(genieEntry);
  Log(logmessage, v_message, verbosity);

  
  // How many triggers to loop over in this Execute?
  // If splitSubtriggers is True then we will "loop" over a single trigger
  //  defined by previously set MCTriggerNum
  // If splitSubtriggers is False then we will loop from 0 up to the total
  //  number of triggers
  trigsInEntry = WCSimEntry->wcsimrootevent->GetNumberOfEvents();
  MCTriggerNum = splitSubtriggers ? MCTriggerNum : 0;    
  int MaxEventNr = splitSubtriggers ? MCTriggerNum + 1 : trigsInEntry;

  logmessage  = "LoadWCSim::Execute: There are " + std::to_string(trigsInEntry);
  logmessage += " triggers in this entry";
  Log(logmessage, v_warning, verbosity);

  int nMRDTriggers  = WCSimEntry->wcsimrootevent_mrd->GetNumberOfEvents();
  int nVetoTriggers = WCSimEntry->wcsimrootevent_facc->GetNumberOfEvents();
  
  // Loop over over the triggers
  // =============================================
  while (MCTriggerNum < MaxEventNr) {
    logmessage  = "LoadWCSim::Execute: Getting trigger " + std::to_string(MCTriggerNum);
	logmessage += " of " + std::to_string(trigsInEntry); 
    Log(logmessage, v_message, verbosity);

    WCSimRootTrigger* aTrigTank = WCSimEntry->wcsimrootevent->GetTrigger(MCTriggerNum);
    WCSimRootTrigger* aTrigMRD  = ( (MCTriggerNum < nMRDTriggers)
									? WCSimEntry->wcsimrootevent_mrd->GetTrigger(MCTriggerNum)
									: nullptr );
    WCSimRootTrigger* aTrigVeto = ( (MCTriggerNum < nVetoTriggers)
									? WCSimEntry->wcsimrootevent_facc->GetTrigger(MCTriggerNum)
									: nullptr );

    std::ostringstream ssEventTank, ssEventMRD, ssEventVeto;
    std::ostringstream ssTrigTank,  ssTrigMRD,  ssTrigVeto;
    ssEventTank << WCSimEntry->wcsimrootevent;
    ssEventMRD  << WCSimEntry->wcsimrootevent_mrd;
    ssEventVeto << WCSimEntry->wcsimrootevent_facc;
    ssTrigTank << aTrigTank; ssTrigMRD  << aTrigMRD; ssTrigVeto << aTrigVeto;
    logmessage  = "LoadWCSim::Execute: memory locations\n";
    logmessage += " wcsimrootevent: " + ssEventTank.str();
    logmessage += ", wcsimrootevent_mrd: " + ssEventMRD.str();
    logmessage += ", wcsimrootevent_facc: " + ssEventVeto.str() + "\n";
    logmessage += " aTrigTank: " + ssTrigTank.str();
    logmessage += ", aTrigMRD: " + ssTrigMRD.str();
    logmessage += ", aTrigVeto: " + ssTrigVeto.str();
    Log(logmessage, v_debug, verbosity);


    logmessage = "LoadWCSim::Execute: Getting event date";
    Log(logmessage, v_message, verbosity);
    
    RunNumber = aTrigTank->GetHeader()->GetRun();
    SubrunNumber = 0;
    EventTimeNs = aTrigTank->GetHeader()->GetDate();
    EventTime->SetNs(EventTimeNs);

    logmessage = "LoadWCSim::Execute: EventTime is: " + std::to_string(EventTimeNs);
    Log(logmessage, v_debug, verbosity);

    // Load everything
    // LoadHits returns a bool that we could use, but don't
    // =============================================
    LoadMCParticles(firstTrigTank);
    LoadNeutronCaptures(aTrigTank);
    LoadHits(aTrigTank, firstTrigTank, "Tank");
    LoadHits(aTrigMRD,  firstTrigMRD,  "MRD" );
    LoadHits(aTrigVeto, firstTrigVeto, "Veto");

    // Set the event time
    logmessage = "LoadWCSim::Execute: Setting triggerdata time to " + std::to_string(EventTimeNs) + " ns";
    Log(logmessage, v_message, verbosity);
    TriggerData->front().SetTime(EventTimeNs);
		
		
    // update the information about tracks and which tank/mrd/veto PMTs they hit
    // this needs updating with each MC trigger, as digits are grouped into MC trigger
    // so these maps will then only contain the digits respective particles create
    // in the active trigger
    // 
    // ParticleId_to_TankTubeIds is a std::map<ParticleId,std::map<ChannelKey,TotalCharge>>
    // where TotalCharge is the total charge from that particle on that tube
    // (in the event that the particle generated several hits on the tube)
    // ParticleId_to_TankCharge is a std::map<ParticleId,TotalCharge> 
    // where TotalCharge is summed over all digits, on all pmts, which contained
    // light from that particle
    ParticleId_to_TankTubeIds->clear();
    ParticleId_to_MrdTubeIds->clear();
    ParticleId_to_VetoTubeIds->clear();
    ParticleId_to_TankCharge->clear();
    ParticleId_to_MrdCharge->clear();
    ParticleId_to_VetoCharge->clear();
    MakeParticleToPmtMap(aTrigTank, firstTrigTank, ParticleId_to_TankTubeIds,
						 ParticleId_to_TankCharge, pmt_tubeid_to_channelkey);
    MakeParticleToPmtMap(aTrigMRD, firstTrigMRD, ParticleId_to_MrdTubeIds,
						 ParticleId_to_MrdCharge, mrd_tubeid_to_channelkey);
    MakeParticleToPmtMap(aTrigVeto, firstTrigVeto, ParticleId_to_VetoTubeIds,
						 ParticleId_to_VetoCharge, facc_tubeid_to_channelkey);
    MCTriggerNum++;
  } //End of MCTriggerNum loop
	
	
  // Save things to the store
  // =============================================
  logmessage  = "LoadWCSim::Execute: Setting the store variables";
  Log(logmessage, v_message, verbosity);

  m_data->Stores.at("ANNIEEvent")->Set("RunNumber", RunNumber);
  m_data->Stores.at("ANNIEEvent")->Set("SubrunNumber", SubrunNumber);
  m_data->Stores.at("ANNIEEvent")->Set("RunType", RunType);
  m_data->Stores.at("ANNIEEvent")->Set("EventNumber", EventNumber);

  logmessage = "LoadWCSim::Execute: Setting the MCParticles";
  Log(logmessage, v_debug, verbosity);
  m_data->Stores.at("ANNIEEvent")->Set("MCParticles", MCParticles, true);

  // Set up Particles object for reconstructed particles
  std::vector<Particle> Particles_Reco;
  m_data->Stores.at("ANNIEEvent")->Set("Particles", Particles_Reco);

  logmessage = "LoadWCSim::Execute: Setting the MCHits";
  Log(logmessage, v_debug, verbosity);
  m_data->Stores.at("ANNIEEvent")->Set("MCHits", MCHits, true);

  logmessage = "LoadWCSim::Execute: Setting the TDCData";
  Log(logmessage, v_debug, verbosity);
  m_data->Stores.at("ANNIEEvent")->Set("TDCData", TDCData, true);

  // TODO?
  // right now we have three Time variables:
  // 1. "RunStartTime" which stores just a user passed start time.
  // 2. "TriggerData", a one-element vector of TriggerClass objects, each of which has a time member.
  //    The one used entry has its time member set to the time between MC event start (always 0)
  //     and the MC trigger time.
  // 3. "EventTime" which stores this same time difference between MC Trigger and MC event start
  // This means we simulate many events all happening ~ the unix epoch (time 0).
  // If we want to simulate a 'run' of events, we need to throw some cumulative running time and
  // add this to RunStartTime, and store the Event and Trigger times separately.
  // This is done by PulseSimulation tool (timefileout_Time), but maybe should be here...
  logmessage = "LoadWCSim::Execute: Setting the TriggerData";
  Log(logmessage, v_debug, verbosity);
  m_data->Stores.at("ANNIEEvent")->Set("TriggerData", TriggerData, true);

  logmessage = "LoadWCSim::Execute: Setting the EventTime";
  Log(logmessage, v_debug, verbosity);
  long runstarttime = RunStartTime.GetNs();
  m_data->Stores.at("ANNIEEvent")->Set("RunStartTime", runstarttime);
  m_data->Stores.at("ANNIEEvent")->Set("EventTimeTank", runstarttime);
  m_data->Stores.at("ANNIEEvent")->Set("EventTimeMRD", RunStartTime);
  m_data->Stores.at("ANNIEEvent")->Set("EventTime", EventTime, true);
  m_data->Stores.at("ANNIEEvent")->Set("MCEventNum", WCSimEntryNum);
  
  // If we merged the subtriggers, report an MCTriggerNum of 0
  // so downstream tools know it's a new event
  int reported_triggernum = splitSubtriggers ? MCTriggerNum-1 : 0;
  m_data->Stores.at("ANNIEEvent")->Set("MCTriggernum", reported_triggernum);
  m_data->Stores.at("ANNIEEvent")->Set("MCFile", MCFile);
  m_data->Stores.at("ANNIEEvent")->Set("MCFlag", true);
  m_data->Stores.at("ANNIEEvent")->Set("BeamStatus", beamstat);
  m_data->Stores.at("ANNIEEvent")->Set("MCNeutCap", MCNeutCap);
  m_data->Stores.at("ANNIEEvent")->Set("MCNeutCapGammas", MCNeutCapGammas);
  m_data->CStore.Set("NumTriggersThisMCEvt", trigsInEntry);

  // auxilliary information about MC Truth particles
  m_data->Stores.at("ANNIEEvent")->Set("ParticleId_to_TankTubeIds", ParticleId_to_TankTubeIds, false);
  m_data->Stores.at("ANNIEEvent")->Set("ParticleId_to_TankCharge", ParticleId_to_TankCharge, false);
  m_data->Stores.at("ANNIEEvent")->Set("ParticleId_to_MrdTubeIds", ParticleId_to_MrdTubeIds, false);
  m_data->Stores.at("ANNIEEvent")->Set("ParticleId_to_MrdCharge", ParticleId_to_MrdCharge, false);
  m_data->Stores.at("ANNIEEvent")->Set("ParticleId_to_VetoTubeIds", ParticleId_to_VetoTubeIds, false);
  m_data->Stores.at("ANNIEEvent")->Set("ParticleId_to_VetoCharge", ParticleId_to_VetoCharge, false);
  m_data->Stores.at("ANNIEEvent")->Set("TrackId_to_MCParticleIndex",trackid_to_mcparticleindex,false);

  //Change MRD TriggerType if first and last layer saw a hit (Hardware Cosmic Trigger)
  if (mrd_lastlayer && mrd_firstlayer) TriggerType = "Cosmic";
  m_data->Stores.at("ANNIEEvent")->Set("MRDTriggerType", TriggerType);
  m_data->Stores.at("ANNIEEvent")->Set("PrimaryMuonIndex", primaryMuonIndex);
  m_data->Stores.at("ANNIEEvent")->Set("TriggerWord", TriggerWord);	
	
  std::map<std::string, bool> DataStreams;
  DataStreams.emplace(std::make_pair("Tank", true));
  DataStreams.emplace(std::make_pair("MRD", true));
  DataStreams.emplace(std::make_pair("Trigger", true));
  DataStreams.emplace(std::make_pair("LAPPD", true));
  m_data->Stores.at("ANNIEEvent")->Set("DataStreams", DataStreams);

  // We have an extended readout for all MC events, set to 1
  m_data->Stores.at("ANNIEEvent")->Set("TriggerExtended", 1);

  logmessage = "LoadWCSim::Execute: Done loading event";
  Log(logmessage, v_message, verbosity);

  ++EventNumber;

  // Things that will be set by later tools:
  //  RawADCData
  //  CalibratedADCData
  //  RawLAPPDData
  //  CalibratedLAPPDData
  //  RecoParticles

  logmessage = "LoadWCSim::Execute: Checking if we're done with trigs from this event";
  Log(logmessage, v_debug, verbosity);

  bool newentry = false;
  if (MCTriggerNum == trigsInEntry) {
    MCTriggerNum = 0;
    ++WCSimEntryNum;
    newentry = true;

    logmessage  = "LoadWCSim::Execute: This is the last trigger in the event. ";
    logmessage += "Next loop with process a new event";
    Log(logmessage, v_debug, verbosity);
  } else {
    logmessage  = "LoadWCSim::Execute: There are additional trigger in this event. ";
    logmessage += "Next loop with process trigger: " + std::to_string(MCTriggerNum);
    logmessage += "/" + std::to_string(trigsInEntry);
    
  }
  
  // Load the next entry to get it ready for the next Execute loop
  // If this is the last one in the chain then stop the loop
  if (newentry) {
    // if next loop is processing the next trigger in the same entry, no need to re-load it
    if ((int)WCSimEntryNum >= MaxEntries && MaxEntries > 0) {
      logmessage  = "LoadWCSim::Execute: Reached the max number of entries specified in the config. ";
      logmessage += "Terminating the ToolChain";
      Log(logmessage, v_warning, verbosity);

      m_data->vars.Set("StopLoop", 1);
    } else {
      int nBytesRead = WCSimEntry->GetEntry(WCSimEntryNum);  // <0 if out of file

      logmessage  = "LoadWCSim::Execute: Trying to get the next event. ";
      logmessage += "WCSimEntryNum: " + std::to_string(WCSimEntryNum);
      logmessage += ", nBytesRead: " + std::to_string(nBytesRead);
      Log(logmessage, v_debug, verbosity);


      // Check if we've reached the end of the file
      if (nBytesRead <= 0){
		logmessage = "LoadWCSim::Execute: Reached the last entry of the WCSim input file. ";
		logmessage += "Terminating the ToolChain";
		Log(logmessage, v_warning, verbosity);

		m_data->vars.Set("StopLoop", 1);
      }
    }
  }
	
  return true;
}

bool LoadWCSim::Finalise()
{
  WCSimEntry->GetCurrentFile()->Close();
  delete WCSimEntry;
	
  return true;
}

Geometry* LoadWCSim::ConstructToolChainGeometry()
{
  // Pull details from the WCSimRootGeom
  // ======================================================
  double WCSimGeometryVer = 1;                      // TODO: pull this from some suitable variable
  // some variables are available from wcsimrootgeom.  TODO: put everything into wcsimrootgeom
  numtankpmts = wcsimrootgeom->GetWCNumPMT();
  numlappds = wcsimrootgeom->GetWCNumLAPPD();
  nummrdpmts = wcsimrootgeom->GetWCNumMRDPMT();
  numvetopmts = wcsimrootgeom->GetWCNumFACCPMT();

  Position tank_centre(wcsimrootgeom->GetWCOffset(0),
					   wcsimrootgeom->GetWCOffset(1),
					   wcsimrootgeom->GetWCOffset(2));
  tank_centre.UnitToMeter();

  // GetWCCylRadius() returns the black sheet radius not the tank radius
  double tank_radius = (wcsimrootgeom->GetWCCylRadius()) / 100.;

  // GetWCCylLength() returns the main annulus height not the tank height
  double tank_halfheight = 0.5*(wcsimrootgeom->GetWCCylLength()) / 100.; 

  //Currently hard-coded; estimated with a tape measure on the ANNIE frame :)
  double pmt_enclosed_radius = 1.0;
  double pmt_enclosed_halfheight = 1.45;
  
  // geometry variables not yet in wcsimrootgeom are in MRDSpecs.hh
  double mrd_width  =  (MRDSpecs::MRD_width)  / 100.;
  double mrd_height =  (MRDSpecs::MRD_height) / 100.;
  double mrd_depth  =  (MRDSpecs::MRD_depth)  / 100.;
  double mrd_start  =  (MRDSpecs::MRD_start)  / 100.;

  logmessage  = "LoadWCSim::ConstructToolChainGeometry: We have ";
  logmessage += std::to_string(numtankpmts) + " tank PMTs, ";
  logmessage += std::to_string(nummrdpmts) + " MRD PMTs, and";
  logmessage += std::to_string(numlappds) + " LAPPDs";
  Log(logmessage, v_message, verbosity);
	
  // construct the ToolChain Goemetry
  // ======================================================
  auto* anniegeom = new Geometry(WCSimGeometryVer,
								 tank_centre,
								 tank_radius,
								 tank_halfheight,
								 pmt_enclosed_radius,
								 pmt_enclosed_halfheight,
								 mrd_width,
								 mrd_height,
								 mrd_depth,
								 mrd_start,
								 numtankpmts,
								 nummrdpmts,
								 numvetopmts,
								 numlappds,
								 geostatus::FULLY_OPERATIONAL);

  logmessage  = "LoadWCSim::ConstructToolChainGeometry: Constructed anniegeom ";  
  logmessage += "with tank origin (x, y, z): (" + std::to_string(tank_centre.X());
  logmessage += ", " + std::to_string(tank_centre.Y());
  logmessage += ", " + std::to_string(tank_centre.Z()) + ")";
  Log(logmessage, v_message, verbosity);

  m_data->Stores.at("ANNIEEvent")->Header->Set("AnnieGeometry", anniegeom, true);
	
  // Construct the Detectors and Channels
  // ======================================================
  ConstructDetectors(anniegeom, numtankpmts, "Tank" );
  ConstructDetectors(anniegeom, nummrdpmts,  "MRD"  );
  ConstructDetectors(anniegeom, numvetopmts, "Veto" );
  ConstructDetectors(anniegeom, numlappds,   "LAPPD"); 
  
  // for other WCSim tools that may need the WCSim Tube IDs
  m_data->CStore.Set("lappd_tubeid_to_detectorkey", lappd_tubeid_to_detectorkey);
  m_data->CStore.Set("pmt_tubeid_to_channelkey",    pmt_tubeid_to_channelkey   );
  m_data->CStore.Set("mrd_tubeid_to_channelkey",    mrd_tubeid_to_channelkey   );
  m_data->CStore.Set("facc_tubeid_to_channelkey",   facc_tubeid_to_channelkey  );
  // inverse
  m_data->CStore.Set("detectorkey_to_lappdid",  detectorkey_to_lappdid );
  m_data->CStore.Set("channelkey_to_pmtid",     channelkey_to_pmtid    );
  m_data->CStore.Set("channelkey_to_mrdpmtid",  channelkey_to_mrdpmtid );
  m_data->CStore.Set("channelkey_to_faccpmtid", channelkey_to_faccpmtid);
	
  return anniegeom;
}

////////////////////////////////////////////////////////////////////////////////
void LoadWCSim::ConstructDetectors(Geometry* anniegeom, int numDets, std::string system)
{
  // Check the "system" string. Only Tank, MRD, and Veto are allowed
  if (system != "Tank" && system != "MRD" && system != "Veto" && system != "LAPPD") {
    logmessage  = "LoadWCSim::ConstructDetectors: \"system\" must be Tank, MRD, Veto, or LAPPD; but you passed: ";
    logmessage += system;
    Log(logmessage, v_error, verbosity);
    return;
  }

  // To fill up crates, cards, and channels monotonically
  // they're arbitrary for simulation
  int lvl1_crt = 0;
  int lvl1_crd = 0;
  int lvl1_chn = 0;
  int lvl2_crt = 0;
  int lvl2_crd = 0;
  int lvl2_chn = 0;
  int hv_crt = 0;
  int hv_crd = 0;
  int hv_chn = 0;

  // Loop and build the detectors
  // ======================================================
  for (int detIdx = 0; detIdx < numDets; ++detIdx) {
    WCSimRootPMT apmt;
    if (system == "Tank")  apmt = wcsimrootgeom->GetPMT(detIdx);
    if (system == "MRD")   apmt = wcsimrootgeom->GetMRDPMT(detIdx);
    if (system == "Veto")  apmt = wcsimrootgeom->GetFACCPMT(detIdx);
    if (system == "LAPPD") apmt = wcsimrootgeom->GetLAPPD(detIdx);
		
    // Construct the detector associated with this PMT
    // ======================================================
    unsigned long uniquedetectorkey;
    if (system == "Tank")  uniquedetectorkey = pmtid_to_channelkey[detIdx + 1];
    if (system == "MRD")   uniquedetectorkey = mrdid_to_channelkey[detIdx];
    if (system == "Veto")  uniquedetectorkey = fmvid_to_channelkey[detIdx];
    if (system == "LAPPD") uniquedetectorkey = anniegeom->ConsumeNextFreeDetectorKey();

    std::string CylLocString;
    int cylloc = apmt.GetCylLoc();

    // FIXME set OD pmts in WCSim
    if (apmt.GetName().find("EMI9954KB") != std::string::npos) cylloc = 6; 

    switch (cylloc) {
    case 0:  CylLocString = "TopCap";    break;
    case 2:  CylLocString = "BottomCap"; break;
    case 1:  CylLocString = "Barrel";    break;
    case 4:  CylLocString = "MRD";       break;  // TODO set this as H or V paddle? And layer?
    case 5:  CylLocString = "Veto";      break;  // TODO set layer?
    case 6:  CylLocString = "OD";        break;
    default: CylLocString = "NA";        break;  // unknown
    }

    Detector adet(uniquedetectorkey, system, CylLocString,
				  Position( apmt.GetPosition(0)/100.,
							apmt.GetPosition(1)/100.,
							apmt.GetPosition(2)/100.),
				  Direction(apmt.GetOrientation(0),
							apmt.GetOrientation(1),
							apmt.GetOrientation(2)),
				  apmt.GetName(), detectorstatus::ON, 0.);

    // Construct the channel associated with this PMT
    // Tank, MRD, and Veto have one channel per detector
    // LAPPDs have two channels per strip 
    // ======================================================
    unsigned long uniquechannelkey;
    if (system != "LAPPD") {
      // Tank, MRD, or Veto
      uniquechannelkey = uniquedetectorkey; 
      ++lvl1_chn;
      ++hv_chn;

      if (system == "Tank") {
		pmt_tubeid_to_channelkey.emplace(apmt.GetTubeNo(), uniquechannelkey);
		channelkey_to_pmtid.emplace(uniquechannelkey, apmt.GetTubeNo());

		if (lvl1_chn >= ADC_CHANNELS_PER_CARD)   { lvl1_chn = 0; ++lvl1_crd; ++lvl2_chn; }
		if (lvl1_crd >= ADC_CARDS_PER_CRATE)     { lvl1_crd = 0; ++lvl1_crt; }
		if (lvl2_chn >= MT_CHANNELS_PER_CARD)    { lvl2_chn = 0; ++lvl2_crd; }
		if (lvl2_crd >= MT_CARDS_PER_CRATE)      { lvl2_crd = 0; ++lvl2_crt; }
		if (hv_chn >= CAEN_HV_CHANNELS_PER_CARD) { hv_chn = 0; ++hv_crd; }
		if (hv_crd >= CAEN_HV_CARDS_PER_CRATE)   { hv_crd = 0; ++hv_crt; }
      }

      // Technically the LeCroy is for both MRD and Veto. Thus the veto HV
      //  channel/card/crate should pickup from where the MRD left off
      // But these things elements are never called in practice and these
      //  numbers are largely aribitrary for simulation anyway. 

      if (system == "MRD") {
		mrd_tubeid_to_channelkey.emplace(apmt.GetTubeNo(), uniquechannelkey);
		channelkey_to_mrdpmtid.emplace(uniquechannelkey, apmt.GetTubeNo());

		if (lvl1_chn >= TDC_CHANNELS_PER_CARD) { lvl1_chn = 0; ++lvl1_crd; }
		if (lvl1_crd >= TDC_CARDS_PER_CRATE)   { lvl1_crd = 0; ++lvl1_crt; }
		if (hv_chn >= LECROY_HV_CHANNELS_PER_CARD) { hv_chn = 0; ++hv_crd; }
		if (hv_crd >= LECROY_HV_CARDS_PER_CRATE)   { hv_crd = 0; ++hv_crt; }
		lvl2_crt = -1; lvl2_crd = -1; lvl2_chn = -1;
      }

      if (system == "Veto") {
		facc_tubeid_to_channelkey.emplace(apmt.GetTubeNo(), uniquechannelkey);
		channelkey_to_faccpmtid.emplace(uniquechannelkey, apmt.GetTubeNo());
	
		if (lvl1_chn >= TDC_CHANNELS_PER_CARD) { lvl1_chn = 0; ++lvl1_crd; }
		if (lvl1_crd >= TDC_CARDS_PER_CRATE)   { lvl1_crd = 0; ++lvl1_crt; }
		if (hv_chn >= LECROY_HV_CHANNELS_PER_CARD) { hv_chn = 0; ++hv_crd; }
		if (hv_crd >= LECROY_HV_CARDS_PER_CRATE)   { hv_crd = 0; ++hv_crt; }
		lvl2_crt = -1; lvl2_crd = -1; lvl2_chn = -1;
      }

      Channel channel(uniquechannelkey, Position(0, 0, 0.),
					  0, 0,
					  lvl1_crt, lvl1_crd, lvl1_chn,
					  lvl2_crt, lvl2_crd, lvl2_chn,
					  hv_crt, hv_crd, hv_chn,
					  channelstatus::ON);

      // Add this channel to the geometry
      logmessage  = "LoadWCSim::ConstructDetectors: " + system ;
      logmessage += ", WCSim ID: " + std::to_string(apmt.GetTubeNo());
      logmessage += " Adding channel: " + std::to_string(uniquechannelkey);
      logmessage += " to detector: "  + std::to_string(uniquedetectorkey);
      Log(logmessage, v_debuggier, verbosity);
      
      adet.AddChannel(channel);
    } else {
      // LAPPDs get one channel per strip
      lappd_tubeid_to_detectorkey.emplace(apmt.GetTubeNo(), uniquedetectorkey);
      detectorkey_to_lappdid.emplace(uniquedetectorkey, apmt.GetTubeNo());

      for (int stripIdx = 0; stripIdx < LappdNumStrips; ++stripIdx) {
		uniquechannelkey = anniegeom->ConsumeNextFreeChannelKey();

		// Calc strip info
		// StripSide == 0 for LHS, StripSide == 1 for RHS
		int stripside = ((stripIdx % 2) == 0);   
		int stripnum = (int)(stripIdx / 2);
		double xpos = (stripside) ? -LappdStripLength : LappdStripLength;
		double ypos = (stripnum * LappdStripSeparation) - ((LappdNumStrips * LappdStripSeparation) / 2.);
		++lvl1_chn;
		++hv_chn;
		if (lvl1_chn >= ACDC_CHANNELS_PER_CARD)   { lvl1_chn = 0; ++lvl1_crd; ++lvl2_chn; }
		if (lvl1_crd >= ACDC_CARDS_PER_CRATE)     { lvl1_crd = 0; ++lvl1_crt; }
		if (lvl2_chn >= ACC_CHANNELS_PER_CARD)    { lvl2_chn = 0; ++lvl2_crd; }
		if (lvl2_crd >= ACC_CARDS_PER_CRATE)      { lvl2_crd = 0; ++lvl2_crt; }
		if (hv_chn >= LAPPD_HV_CHANNELS_PER_CARD) { hv_chn = 0; ++hv_crd; }
		if (hv_crd >= LAPPD_HV_CARDS_PER_CRATE)   { hv_crd = 0; ++hv_crt; }

		Channel channel(uniquechannelkey, Position(xpos, ypos, 0.),
						stripside, stripnum,
						lvl1_crt, lvl1_crd, lvl1_chn,
						lvl2_crt, lvl2_crd, lvl2_chn,
						hv_crt, hv_crd, hv_chn,
						channelstatus::ON);

		// Add this channel to the geometry
		logmessage  = "LoadWCSim::ConstructDetectors: " + system ;
		logmessage += ", WCSim ID: " + std::to_string(apmt.GetTubeNo());
		logmessage += " Adding channel: " + std::to_string(uniquechannelkey);
		logmessage += " to detector: "  + std::to_string(uniquedetectorkey);
		Log(logmessage, v_debuggier, verbosity);
	
		adet.AddChannel(channel);
      }// end loop over LAPPD strips

    }// end creating channels

    // Add the detector to the geometry
    logmessage  = "LoadWCSim::ConstructDetectors: " + system;
    logmessage += " Adding detector: " + std::to_string(uniquedetectorkey);
    logmessage += " to geometry";
    Log(logmessage, v_debuggier, verbosity);

    anniegeom->AddDetector(adet);

    if (verbosity >= v_debuggier) anniegeom->PrintChannels();

    // Finished for the Tank and LAPPDs
    // Gotta create paddles for the MRD and Veto
    // ======================================================
    if (system == "MRD" || system == "Veto") {

      // Things to fill for paddles
      int pad_x, pad_y, pad_z;
      int orientation = 0;
      Position origin;
      std::pair<double,double> ext_x;
      std::pair<double,double> ext_y;
      std::pair<double,double> ext_z;
    
      if (system == "MRD") {
		if (detIdx != (apmt.GetTubeNo()-1) ) {
		  logmessage  = "LoadWCSim::ConstructDetectors: " + system;
		  logmessage += "detIdx != pmt.GetTubeNo-1! ";
		  logmessage += "detIdx = " + std::to_string(detIdx);
		  logmessage += ", pmt.GetTubeNo = " + std::to_string(apmt.GetTubeNo());
		  assert(false);
		}

		// calculate MRD_x_y_z ... MRDSpecs doesn't provide a nice way to do this
		int layernum = 0;
		while ((detIdx + 1) > MRDSpecs::layeroffsets.at(layernum + 1)) ++layernum;

		Mrd_Chankey_Layer.emplace(uniquechannelkey, layernum);
		int in_layer_pmtnum = detIdx - MRDSpecs::layeroffsets.at(layernum);
      
		// paddles in each layer alternate on sides; i.e. paddles 0 and 1 are on opposite sides
		int side = in_layer_pmtnum % 2;
		in_layer_pmtnum = std::floor(in_layer_pmtnum / 2);
		orientation = MRDSpecs::paddle_orientations.at(detIdx);
		pad_x = (orientation) ? in_layer_pmtnum  : side;
		pad_y = (orientation) ? side : in_layer_pmtnum;
		pad_z = layernum + 2;  // first MRD z layer num is 2 (veto are 0,1)

		origin = Position(MRDSpecs::paddle_originx.at(detIdx),
						  MRDSpecs::paddle_originy.at(detIdx),
						  MRDSpecs::paddle_originz.at(detIdx));
      
		origin *= 1./1000.;

		ext_x = std::make_pair(MRDSpecs::paddle_extentsx.at(detIdx).first/1000.,
							   MRDSpecs::paddle_extentsx.at(detIdx).second/1000.);

		ext_y = std::make_pair(MRDSpecs::paddle_extentsy.at(detIdx).first/1000.,
							   MRDSpecs::paddle_extentsy.at(detIdx).second/1000.);

		ext_z = std::make_pair(MRDSpecs::paddle_extentsz.at(detIdx).first/1000.,
							   MRDSpecs::paddle_extentsz.at(detIdx).second/1000.);
      }// endif MRD

      else { // system == "Veto"
		pad_z = (uniquedetectorkey>12); // 13 paddles per layer
		pad_x = pad_z;                  // i believe PMTs are on LHS for layer 0, RHS for layer 1
		pad_y = uniquedetectorkey - (13*pad_z);

		// numbers from geofile.txt
		origin = Position(0, facc_paddle_yorigins.at(uniquedetectorkey)/100., (pad_z) ? 0.0728 : 0.0508);

		// numbers from WCSim source files / measurements
		ext_x = std::make_pair(-1.6,1.6);
		ext_y = std::make_pair(origin.Y() - 0.1525, origin.Y() + 0.1525);
		ext_z = std::make_pair(origin.Z() - 0.01,   origin.Z() + 0.01  );
      }// endif Veto

      Paddle apaddle(uniquedetectorkey, pad_x, pad_y, pad_z,
					 orientation, origin, ext_x, ext_y, ext_z);

      logmessage  = "LoadWCSim::ConstructDetectors: " + system;
      logmessage += " Setting paddle for detector: " + std::to_string(uniquedetectorkey);
      Log(logmessage, v_debuggier, verbosity);

      anniegeom->SetDetectorPaddle(uniquedetectorkey,apaddle);
    }// end paddle things for MRD/Veto
  }// end loop over detectors
}
////////////////////////////////////////////////////////////////////////////////
void LoadWCSim::LoadMCParticles(WCSimRootTrigger* firstTrig)
{
  // MCParticles will only be loaded during trigger 0 of a given entry
  // This means that particles from subtriggers are loaded as well
  // When we reach those subtriggers then we'll simply update the relevant info
  if (MCTriggerNum == 0) {
    // Reset the things
    MCParticles->clear();
    trackid_to_mcparticleindex->clear();
    mapNeutronIsPrim->clear();
    primaryMuonIndex = -1;

    // Loop over ALL triggers so we can load all MCParticles
    for (int trigIdx = 0; trigIdx < WCSimEntry->wcsimrootevent->GetNumberOfEvents(); ++trigIdx) {
	  
      WCSimRootTrigger* aTrigTank = WCSimEntry->wcsimrootevent->GetTrigger(trigIdx);

      // Loop over tracks within this trigger and extract MCParticles
      logmessage = "LoadWCSim::LoadMCParticles: Getting " + std::to_string(aTrigTank->GetNtrack());
      logmessage += " tracks from trigger # " + std::to_string(trigIdx);
      Log(logmessage, v_message, verbosity);	

      for (int trackIdx = 0; trackIdx < aTrigTank->GetNtrack(); trackIdx++) {
		logmessage = "LoadWCSim::LoadMCParticles: Getting WCSim track # " + std::to_string(trackIdx);
		Log(logmessage, v_message, verbosity);	

		auto* nextTrack = (WCSimRootTrack*)aTrigTank->GetTracks()->At(trackIdx);
					
		tracktype startStopType = tracktype::UNDEFINED;

		// Extract the neutrino information
		if (nextTrack->GetFlag() == -1) {
		  double startTime = AdjustTime((double)nextTrack->GetTime());
		  double stopTime  = AdjustTime((double)nextTrack->GetStopTime());
		  Position startPos(nextTrack->GetStart(0), nextTrack->GetStart(1), nextTrack->GetStart(2));
		  Position stopPos(nextTrack->GetStop(0), nextTrack->GetStop(1), nextTrack->GetStop(2));
		  startPos.UnitToMeter();
		  stopPos.UnitToMeter();
		  double length = (stopPos-startPos).Mag();

		  MCParticle neutrino(nextTrack->GetIpnu(), nextTrack->GetE(), nextTrack->GetEndE(),
							  startPos, stopPos, startTime, stopTime,
							  Direction(nextTrack->GetDir(0), nextTrack->GetDir(1), nextTrack->GetDir(2)),
							  length, startStopType,
							  nextTrack->GetId(),
							  nextTrack->GetParenttype(),
							  nextTrack->GetFlag(),
							  trigIdx);
							
		  // Save the neutrino own particle in the store
		  m_data->Stores["ANNIEEvent"]->Set("NeutrinoParticle", neutrino);

		  // Don't also create a particle for the neutrino
		  continue;
		  
		}// done extracting the neutrino information
	  
		// Now load the other particles
		double startTime = AdjustTime((double)nextTrack->GetTime());
		double stopTime  = AdjustTime((double)nextTrack->GetStopTime());
		Position startPos(nextTrack->GetStart(0), nextTrack->GetStart(1), nextTrack->GetStart(2));
		Position stopPos(nextTrack->GetStop(0), nextTrack->GetStop(1), nextTrack->GetStop(2));
		startPos.UnitToMeter();
		stopPos.UnitToMeter();
		double length = (stopPos-startPos).Mag();
	
		logmessage = "LoadWCSim::LoadMCParticles: Loaded particle with PDG: " + std::to_string(nextTrack->GetIpnu());
		logmessage += ", stop time: " + std::to_string(stopTime);
		logmessage += ", end process: " + nextTrack->GetEndProcess();
		Log(logmessage, v_debug, verbosity);

		// Record neutron primary/secondary
		if (nextTrack->GetIpnu() == 2112)
		  mapNeutronIsPrim->emplace(nextTrack->GetId(), (nextTrack->GetParenttype() == 0));
	
		MCParticle thisparticle(nextTrack->GetIpnu(), nextTrack->GetE(), nextTrack->GetEndE(),
								startPos, stopPos, startTime, stopTime,
								Direction(nextTrack->GetDir(0), nextTrack->GetDir(1), nextTrack->GetDir(2)),
								length, startStopType,
								nextTrack->GetId(),
								nextTrack->GetParenttype(),
								nextTrack->GetFlag(),
								trigIdx);

		// Exit point is not currently in constructor call so set it separately
		// Older WCSim files do not recor this info. This breaks backward compatibility
		Position exitPoint(nextTrack->GetTankExitPoint(0),
						   nextTrack->GetTankExitPoint(1),
						   nextTrack->GetTankExitPoint(2));
		exitPoint.UnitToMeter();
		thisparticle.SetTankExitPoint(exitPoint);

		// Check if this is a primary muon. Only record the first one
		if (nextTrack->GetIpnu() == 13 && nextTrack->GetParenttype() == 0 &&
			nextTrack->GetFlag() == 0  && primaryMuonIndex < 0 ) 
		  primaryMuonIndex = MCParticles->size();

		// Some print outs for "interesting" particles
		if (abs(nextTrack->GetIpnu()) == 13 || abs(nextTrack->GetIpnu()) == 211 || nextTrack->GetIpnu() == 111){
		  logmessage = "LoadWCSim::LoadMCParticles: Found " + std::to_string(nextTrack->GetIpnu());
		  logmessage += " with flag: " + std::to_string(nextTrack->GetFlag());
		  logmessage += ", parent type " + std::to_string(nextTrack->GetParenttype());
		  logmessage += ", Id " + std::to_string(nextTrack->GetId());
		  logmessage += ", start vertex (" + std::to_string(nextTrack->GetStart(0)/100.);
		  logmessage += ", " + std::to_string(nextTrack->GetStart(1)/100.);
		  logmessage += ", " + std::to_string(nextTrack->GetStart(2)/100.);
		  logmessage += "), and end vertex (" + std::to_string(nextTrack->GetStop(0)/100.);
		  logmessage += ", " + std::to_string(nextTrack->GetStop(1)/100.);
		  logmessage += ", " + std::to_string(nextTrack->GetStop(2)/100.) + ")";
		  Log(logmessage, v_debug, verbosity);
		}
	  
		trackid_to_mcparticleindex->emplace(nextTrack->GetId(),MCParticles->size());
		MCParticles->push_back(thisparticle);
      }// end loop over tracks

      logmessage = "LoadWCSim::LoadMCParticles: Loaded " + std::to_string(MCParticles->size()) + " MCParticles";
      Log(logmessage, v_debug, verbosity);
    } // end loop over events
  }// endif MCTriggerNum == 0
  else {
    // if MCTrigger > 0 we need to update all the particle times
    // since particle times are relative to the trigger time
    double timeDiff = EventTimeNs - firstTrig->GetHeader()->GetDate();
    for (MCParticle& aparticle : *MCParticles) {
      if (splitSubtriggers) {
		aparticle.SetStartTime(aparticle.GetStartTime() - timeDiff);
		aparticle.SetStopTime (aparticle.GetStopTime()  - timeDiff);
      } 
    }
  } // end updating particle times
}

////////////////////////////////////////////////////////////////////////////////
void LoadWCSim::LoadNeutronCaptures(WCSimRootTrigger* aTrig)
{
  int numCaptures = aTrig ? aTrig->GetNcaptures() : 0;

  logmessage = "LoadWCSim::LoadNeutronCaptures: looping over ";
  logmessage += std::to_string(numCaptures) + "neutron captures";
  Log(logmessage, v_message, verbosity);
    
  for (int captIdx = 0; captIdx < numCaptures; ++captIdx) {
    logmessage = "LoadWCSim::Execute: getting capture # " + std::to_string(captIdx);
    Log(logmessage, v_debug, verbosity);

    auto* capt = (WCSimRootCapture*)aTrig->GetCaptures()->At(captIdx);
			
    int captParent    = capt->GetCaptureParent();
    double captVtxX   = capt->GetCaptureVtx(0);
    double captVtxY   = capt->GetCaptureVtx(1);
    double captVtxZ   = capt->GetCaptureVtx(2);
    int captNGamma    = capt->GetNGamma();
    double captTotalE = capt->GetTotalGammaE();
    double captT      = capt->GetCaptureT();
    int captNucleus   = capt->GetCaptureNucleus();

    // Adjust the time if we're splitting subtriggers
    if (splitSubtriggers) captT -= EventTimeNs;
      
    // Check if capture is from a primary
    double doubleIsPrimary = -9999;
    if (mapNeutronIsPrim->count(captParent) > 0) {
      bool isPrimary = mapNeutronIsPrim->at(captParent);
	
      logmessage = "LoadWCSim::Execute: NeutCap object at " + std::to_string(captT);
      logmessage += " is from a";
      logmessage += isPrimary ? " primary" : " secondary" ;
      logmessage += " neutron parent with ID: " + std::to_string(captParent);
      logmessage += " on nucleus: " + std::to_string(captNucleus);
      Log(logmessage, v_debug, verbosity);
    } else {
      // The parent neutron to this capture was not recorded by WCSim
      logmessage = "LoadWCSim::Execute: NeutCap object at time " + std::to_string(captT);
      logmessage += " is from an unrecorded neutron parent";
      logmessage += " with ID: " + std::to_string(captParent);
      logmessage += " on nucleus: " + std::to_string(captNucleus);
      Log(logmessage, v_message, verbosity);	
    }

    // Record the photons from the neutron capture
    std::vector<double> gammaEnergies;
    for (int gammaIdx = 0; gammaIdx < captNGamma; ++gammaIdx) {
      auto* captGamma = (WCSimRootCaptureGamma*) capt->GetGammas()->At(gammaIdx);
      gammaEnergies.push_back(captGamma->GetE());
    }

    // Push back the things
    if (MCNeutCap.size()==0){
      MCNeutCap.emplace("CaptParent",std::vector<double>{double(captParent)});
      MCNeutCap.emplace("CaptVtxX",std::vector<double>{captVtxX});
      MCNeutCap.emplace("CaptVtxY",std::vector<double>{captVtxY});
      MCNeutCap.emplace("CaptVtxZ",std::vector<double>{captVtxZ});
      MCNeutCap.emplace("CaptNGamma",std::vector<double>{double(captNGamma)});
      MCNeutCap.emplace("CaptTotalE",std::vector<double>{captTotalE});
      MCNeutCap.emplace("CaptTime",std::vector<double>{captT});
      MCNeutCap.emplace("CaptNucleus",std::vector<double>{double(captNucleus)});
      MCNeutCapGammas.emplace("CaptGammas",std::vector<std::vector<double>>{gammaEnergies});
      MCNeutCap.emplace("CaptPrimary",std::vector<double>{doubleIsPrimary});
    } else {
      MCNeutCap.at("CaptParent").push_back(captParent);
      MCNeutCap.at("CaptVtxX").push_back(captVtxX);
      MCNeutCap.at("CaptVtxY").push_back(captVtxY);
      MCNeutCap.at("CaptVtxZ").push_back(captVtxZ);
      MCNeutCap.at("CaptNGamma").push_back(captNGamma);
      MCNeutCap.at("CaptTotalE").push_back(captTotalE);
      MCNeutCap.at("CaptTime").push_back(captT);
      MCNeutCap.at("CaptNucleus").push_back(captNucleus);
      MCNeutCapGammas.at("CaptGammas").push_back(gammaEnergies);
      MCNeutCap.at("CaptPrimary").push_back(doubleIsPrimary);
    }
  }// end loop over neutron captures
}

////////////////////////////////////////////////////////////////////////////////
bool LoadWCSim::LoadHits(WCSimRootTrigger* thisTrig, WCSimRootTrigger* firstTrig, std::string system)
{
  // Check the "system" string. Only Tank, MRD, and Veto are allowed
  if (system != "Tank" && system != "MRD" && system != "Veto") {
    logmessage  = "LoadWCSim::LoadHits: \"system\" must be Tank, MRD, or Veto; but you passed: ";
    logmessage += system;
    Log(logmessage, v_error, verbosity);
    return false;
  }

  int numDigiHits = thisTrig ? thisTrig->GetCherenkovDigiHits()->GetEntries() : 0;
  logmessage  = "LoadWCSim::LoadHits: Looping over " + std::to_string(numDigiHits);
  logmessage += " " + system + " digi. hits";
  Log(logmessage, v_message, verbosity);

  for (int hitIdx = 0; hitIdx < numDigiHits; ++hitIdx) {
    logmessage = "LoadWCSim::LoadHits: Getting hit: " + std::to_string(hitIdx);
    Log(logmessage, v_debug, verbosity);

    auto* digiHit = (WCSimRootCherenkovDigiHit*)thisTrig->GetCherenkovDigiHits()->At(hitIdx);
    int tubeID = digiHit->GetTubeId();

    if ((system == "Tank" && pmt_tubeid_to_channelkey.count(tubeID)  == 0) ||
		(system == "MRD"  && mrd_tubeid_to_channelkey.count(tubeID)  == 0) ||
		(system == "Veto" && facc_tubeid_to_channelkey.count(tubeID) == 0) ) {
      logmessage = "LoadWCSim::LoadHits: NO PMT ASSOCIATED WITH ID: " + std::to_string(tubeID);
      Log(logmessage, v_error, verbosity);
      return false;
    }

    uint64_t key;
    if (system == "Tank") key = pmt_tubeid_to_channelkey.at(tubeID);
    if (system == "MRD")  key = mrd_tubeid_to_channelkey.at(tubeID);
    if (system == "Veto") key = facc_tubeid_to_channelkey.at(tubeID);

    logmessage = "LoadWCSim::LoadHits: tubeid: " + std::to_string(tubeID);
    logmessage += " has ChannelKey: " + std::to_string(key);
    Log(logmessage, v_debug, verbosity);

    // Omit masked tank PMTs
    if (system == "Tank" && PMTMask != "None" &&
		std::find(masked_ids.begin(), masked_ids.end(), tubeID) != masked_ids.end()) {
      logmessage = "LoadWCSim::LoadHits: Skipping masked PMT: " + std::to_string(tubeID);      
      Log(logmessage, v_debug, verbosity);
      continue;
    }

    // Grab the hit charge and time
    float digiQ = digiHit->GetQ();
    double digiTime;
    if (use_smeared_digit_time) {
      // relative to trigger
      digiTime = static_cast<double>(digiHit->GetT()-HistoricTriggeroffset); 
    } else {
      // Use the true time of the first photon in the hit
      double earliestTime = 999999999999;

      // Loop over indices of the digit's photons not "IDs"
      std::vector<int> photonIdxs = digiHit->GetPhotonIds(); 
      for (int& photonIdx : photonIdxs) {
		auto* theHitTimeObject = (WCSimRootCherenkovHitTime*)firstTrig->GetCherenkovHitTimes()->At(photonIdx);

		if (theHitTimeObject == nullptr) {
		  logmessage = "LoadWCSim::LoadHits: Retrieval of photon from digi. hit returned nullptr!";
		  Log(logmessage, v_error, verbosity);
		  continue;
		}
	  
		double thisTime = static_cast<double>(theHitTimeObject->GetTruetime());
		if (thisTime < earliestTime) earliestTime = thisTime; 
      }// end loop over photons
      digiTime = earliestTime;
    }// endif use_smeared_digit_time

    // Adjust hit time as necessary based on settings and system
    if ( (system == "Tank" && !splitSubtriggers && use_smeared_digit_time) ||
		 (system == "MRD"  && !splitSubtriggers) ||
		 (system == "Veto") ) {
      digiTime += EventTimeNs;
    }
    
    logmessage = "LoadWCSim::LoadHits: digi. hit time is " + std::to_string(digiTime);
    logmessage +=" [ns] from the start of the Trigger";
    logmessage += " and charge is: " + std::to_string(digiQ);
    Log(logmessage, v_debug, verbosity);
    
    // Create the hit and put it in the correct map
    MCHit nextHit(key, digiTime, digiQ, GetHitParentIdxs(digiHit, firstTrig));

    if (system == "Tank") {
      if (MCHits->count(key) == 0) MCHits->emplace(key, std::vector<MCHit>{nextHit});
      else                         MCHits->at(key).push_back(nextHit);
    }

    if (system == "MRD") {
      if (TDCData->count(key) == 0) TDCData->emplace(key, std::vector<MCHit>{nextHit});
      else                          TDCData->at(key).push_back(nextHit);

      if (Mrd_Chankey_Layer.at(key) == 0)  mrd_firstlayer = true;
      if (Mrd_Chankey_Layer.at(key) == 10) mrd_lastlayer  = true;
    }

    if (system == "Veto") {
      if (TDCData->count(key) == 0) TDCData->emplace(key, std::vector<MCHit>{nextHit});
      else                          TDCData->at(key).push_back(nextHit);
    }

    logmessage = "LoadWCSim::LoadHits: Tank hit added for " + system;
    Log(logmessage, v_debug, verbosity);

  }// end loop over hits

  logmessage = "LoadWCSim::LoadHits: Finished loading " + system + " hits";
  Log(logmessage, v_message, verbosity);

  return true;
}

////////////////////////////////////////////////////////////////////////////////
void LoadWCSim::MakeParticleToPmtMap(WCSimRootTrigger* thistrig,
									 WCSimRootTrigger* firstTrig,
									 std::map<int,std::map<unsigned long,double>>* ParticleId_to_TubeIds,
									 std::map<int,double>* ParticleId_to_Charge,
									 std::map<int,unsigned long> tubeid_to_channelkey)
{
  if (thistrig==nullptr) return;
  ParticleId_to_TubeIds->clear();
  ParticleId_to_Charge->clear();
  // scan through the parents IDs of the photons contributing to each digit
  // make note of which parent contributes to which digit, and which digits are associated with each parent
  Log("Making Particle to PMT Map",v_message, verbosity);
  // technically the charge will be a lower limit as this sums the charge from all digits
  // that a given particle contributed to, but not all this digit's charge may have been
  // from this particle.
	
  // To match digits to their parent particles we need the corresponding CherenkovHitTimes
  // both CherenkovHits and CherenkovHitTimes are stored in the first trigger
  //--------------------------------------------------------------------------------------
	
  // Loop over all digits 
  int numdigits = thistrig->GetCherenkovDigiHits()->GetEntries();
  for (int digitIdx=0; digitIdx<numdigits; digitIdx++) {
    auto* digihit = (WCSimRootCherenkovDigiHit*)thistrig->GetCherenkovDigiHits()->At(digitIdx);
    int tubeID = digihit->GetTubeId();
    double hitQ = digihit->GetQ();
    
    
    // loop over the photons in this digit
    std::vector<int> truephotonindices = digihit->GetPhotonIds();
    for (int thephotonsid : truephotonindices) {

      // get the index of the photon CherenkovHit object in the TClonesArray
      if (WCSimVersion < 2) {
		if (!timeArrayOffsetMap.size()) BuildTimeArrayOffsetMap(firstTrig);
		thephotonsid += timeArrayOffsetMap.at(tubeID);
      }
      
      // Get the CherenkovHitTime object that records the photon's Parent ID
      auto* thehittimeobject = (WCSimRootCherenkovHitTime*)(firstTrig->GetCherenkovHitTimes()->At(thephotonsid));

      // get the parent ID from the CherenkovHitTime
      Int_t parentID = (thehittimeobject) ? thehittimeobject->GetParentID() : -1;

      // We'll want a map of particle ID to channel keys, so convert WCSim TubeID to channelkey
      int chankey = tubeid_to_channelkey.at(tubeID);
			
      // Finally we can record the things
      // Check for parentID not in outer map, place it there empty
      auto outerIt = ParticleId_to_TubeIds->find(parentID);
      if (outerIt == ParticleId_to_TubeIds->end())
		outerIt = ParticleId_to_TubeIds->emplace(parentID, std::map<unsigned long,double>()).first;

      // Now emplace the charge value
      // Will succeed (result.second == true) if tubeID is not present
      // Will fail if tubeID is present. In which case we'll increment the charge
      auto result_PartToTubeIDs = outerIt->second.emplace(tubeID, hitQ);
      if (!result_PartToTubeIDs.second)
		result_PartToTubeIDs.first->second += hitQ;

      // Similar for the total charge map, but this is not nested
      auto result_PartToCharge = ParticleId_to_Charge->emplace(parentID, hitQ);
      if (!result_PartToCharge.second)
		result_PartToCharge.first->second += hitQ;
      
    }// end loop over photons
  }// end loop over digits
}

////////////////////////////////////////////////////////////////////////////////
// Get the ID of the primary MCParticle(s) that produced this digi. hit
std::vector<int> LoadWCSim::GetHitParentIDs(WCSimRootCherenkovDigiHit* digiHit, WCSimRootTrigger* firstTrig)
{
  std::vector<int> parentIDs; // a hit could technically have more than one contrbuting particle
	
  // loop over the photons in this digit
  std::vector<int> photonIdxs = digiHit->GetPhotonIds();
  for (int photonIdx : photonIdxs) {
    // Special offset for older WCSim
    if (WCSimVersion < 2) {
      if (timeArrayOffsetMap.size() == 0) BuildTimeArrayOffsetMap(firstTrig);
      photonIdx += timeArrayOffsetMap.at(digiHit->GetTubeId());
    }
    
    // Get the CherenkovHitTime objects themselves, which contain the primary parent IDs
    auto* theHitTimeObject = (WCSimRootCherenkovHitTime*)(firstTrig->GetCherenkovHitTimes()->At(photonIdx));

    if (theHitTimeObject == nullptr) {
      logmessage = "LoadWCSim::GetHitParentIDs: HitTime object is NULL!!";
      Log(logmessage, v_error, verbosity);
    }
    else 
      parentIDs.push_back(theHitTimeObject->GetParentID());
  
  }// end loop over photons  
  return parentIDs;
}

////////////////////////////////////////////////////////////////////////////////
// Get the index within the the MCParticle vector of the primaries that produced this digi. hit
std::vector<int> LoadWCSim::GetHitParentIdxs(WCSimRootCherenkovDigiHit* digiHit, WCSimRootTrigger* firstTrig)
{
  std::vector<int> parentIDs = GetHitParentIDs(digiHit, firstTrig);
  std::vector<int> parentIdxs;

  // Check if the parent was recorded, and if so then translate ID to index
  for (int parentID : parentIDs) {
    if (trackid_to_mcparticleindex->count(parentID))
      parentIdxs.push_back(trackid_to_mcparticleindex->at(parentID));
  }

  return parentIdxs;
}

////////////////////////////////////////////////////////////////////////////////
void LoadWCSim::BuildTimeArrayOffsetMap(WCSimRootTrigger* firstTrig)
{
  if (WCSimVersion < 2) {
    // The CherenkovHitTimes is a flattened array (over PMTs) of arrays (over photons)
    // For WCSimVersion<2, the PhotonIds available from a digit are the indices 
    // *within the subarray for that PMT*
    // we therefore need we need to offset these indices by the start of the pmt's subarray.
    // This offset may be found by scanning the CherenkovHits array (over PMTs),
    // finding the correct TubeID, and retrieving the 'GetTotalPe(0)' member for this entry.
    int ncherenkovhits = firstTrig->GetCherenkovHits()->GetEntries();

    // each WCSimRootCherenkovHit represents a hit PMT
    for (int hitIdx = 0; hitIdx < ncherenkovhits; ++hitIdx){
      auto* hitobject = (WCSimRootCherenkovHit*)firstTrig->GetCherenkovHits()->At(hitIdx);
      if (hitobject == nullptr) {
		logmessage = "LoadWCSim::BuildTimeArrayOffsetMap: Hit object is NULL!!";
		Log(logmessage, v_error, verbosity);
      }
      
      int tubeNumber = hitobject->GetTubeID();
      int timeArrayOffset = hitobject->GetTotalPe(0);
      timeArrayOffsetMap.emplace(tubeNumber, timeArrayOffset);
    }
  } else {
    logmessage  = "LoadWCSim::BuildTimeArrayOffsetMap: Called with WCSimVersion < 2. ";
    logmessage += "This is not needed!?";
    Log(logmessage, v_error, verbosity);
  }
}

////////////////////////////////////////////////////////////////////////////////
std::vector<int> LoadWCSim::LoadPMTMask(std::string path_to_pmtmask)
{
	
  std::vector<int> mask_vector;
  int temp_id;
  ifstream maskfile(path_to_pmtmask.c_str());
  while (!maskfile.eof()){
    maskfile >> temp_id;
    mask_vector.push_back(temp_id);
    if (maskfile.eof()) break;
  }
	
  return mask_vector;
}

////////////////////////////////////////////////////////////////////////////////
double LoadWCSim::AdjustTime(double time)
{
  if (splitSubtriggers) return time - EventTimeNs;
  else return time;   
}
