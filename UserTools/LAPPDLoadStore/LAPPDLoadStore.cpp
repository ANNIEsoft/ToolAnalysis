#include "LAPPDLoadStore.h"

LAPPDLoadStore::LAPPDLoadStore() : Tool() {}

bool LAPPDLoadStore::Initialise(std::string configfile, DataModel &data)
{
    /////////////////// Useful header ///////////////////////
    if (configfile != "")
        m_variables.Initialise(configfile); // loading config file
    // m_variables.Print();

    m_data = &data; // assigning transient data pointer
    /////////////////////////////////////////////////////////////////

    // Control variables used in this tool
    retval = 0;
    eventNo = 0;
    errorEventsNumber = 0;

    // Load Config Variables for this tool
    m_variables.Get("Nboards", Nboards);
    m_variables.Get("PedinputfileTXT", PedFileNameTXT);
    m_variables.Get("DoPedSubtraction", DoPedSubtract);
    m_variables.Get("LAPPDLoadStoreVerbosity", LAPPDLoadStoreVerbosity);
    m_variables.Get("num_vector_data", num_vector_data);
    m_variables.Get("num_vector_pps", num_vector_pps);
    m_variables.Get("SelectSingleLAPPD", SelectSingleLAPPD);
    m_variables.Get("SelectedLAPPD", SelectedLAPPD);
    
    mergingModeReadIn = false;
    m_variables.Get("mergingModeReadIn", mergingModeReadIn);
    MultiLAPPDMap = false;
    m_variables.Get("MultiLAPPDMap", MultiLAPPDMap);
    loadPSEC = true;
    m_variables.Get("loadPSEC", loadPSEC);
    loadPPS = false;
    m_variables.Get("loadPPS", loadPPS);
    loadOffsets = false;
    m_variables.Get("loadOffsets", loadOffsets);
    LoadBuiltPPSInfo = true;
    m_variables.Get("LoadBuiltPPSInfo", LoadBuiltPPSInfo);
    loadFromStoreDirectly = false;
    m_variables.Get("loadFromStoreDirectly", loadFromStoreDirectly);
    
    NonEmptyEvents = 0;
    NonEmptyDataEvents = 0;
    PPSnumber = 0;
    mergedEvent = false;
    isFiltered = false;
    isBLsub = false;
    isCFD = false;
    
    // Global Control variables that you get from the config file
    m_variables.Get("stopEntries", stopEntries);
    m_variables.Get("PsecReceiveMode", PsecReceiveMode);
    m_variables.Get("RawDataOutputWavLabel", OutputWavLabel);
    m_variables.Get("RawDataInputWavLabel", InputWavLabel);
    m_variables.Get("NChannels", NChannels);
    m_variables.Get("Nsamples", Nsamples);
    m_variables.Get("TrigChannel", TrigChannel);
    m_variables.Get("SampleSize", SampleSize);
    m_variables.Get("LAPPDchannelOffset", LAPPDchannelOffset);

    runNumber = 0;
    subRunNumber = 0;
    partFileNumber = 0;
    eventNumberInPF = 0;

    // Grab all pedestal files and prepare the map channel|pedestal-vector for substraction
    if (DoPedSubtract == 1)
    {
        PedestalValues = new std::map<unsigned long, vector<int>>;
        for (int i = 0; i < Nboards; i++) {
            if (LAPPDLoadStoreVerbosity > 0) {
                std::cout << "Reading Pedestal File " << PedFileNameTXT << " " << i << std::endl;
            }
            ReadPedestals(i);
        }
        if (LAPPDLoadStoreVerbosity > 0) {
            cout << "PEDSIZES: " << PedestalValues->size() << " " << PedestalValues->at(0).size() << " " << PedestalValues->at(4).at(5) << endl;
        }
    }

    // set some control variables for later tools
    m_data->CStore.Set("SelectSingleLAPPD", SelectSingleLAPPD);
    m_data->Stores["ANNIEEvent"]->Set("Nsamples", Nsamples);
    m_data->Stores["ANNIEEvent"]->Set("NChannels", NChannels);
    m_data->Stores["ANNIEEvent"]->Set("TrigChannel", TrigChannel);
    m_data->Stores["ANNIEEvent"]->Set("LAPPDchannelOffset", LAPPDchannelOffset);
    m_data->Stores["ANNIEEvent"]->Set("SampleSize", SampleSize);
    m_data->Stores["ANNIEEvent"]->Set("isFiltered", isFiltered);
    m_data->Stores["ANNIEEvent"]->Set("isBLsubtracted", isBLsub);
    m_data->Stores["ANNIEEvent"]->Set("isCFD", isCFD);

    LAPPDEventIndex_ID = {0, 0, 0, 0, 0}; // initialize for five LAPPDs
    
    if (loadOffsets)
        LoadOffsetsAndCorrections();
    
    if (LAPPDLoadStoreVerbosity > 5)
        debugLoadStore.open("debugLoadStore.txt");

    std::string ACCIDConfigFile;
    get_ok = m_variables.Get("ACCIDConfigFile", ACCIDConfigFile);
    if(!get_ok) {
	    Log("LAPPDLoadStore: Missing ACCIDConfigFile in config!",v_error,LAPPDLoadStoreVerbosity);
	    return false;
    }

    idConfigRecords = LoadIDConfig(ACCIDConfigFile);
    if (LAPPDLoadStoreVerbosity > 1)
    {
        // print the ACCID config records for debug
        cout << "Loaded LAPPD ID Config Records from " << ACCIDConfigFile << ":" << endl;
        for (const auto& record : idConfigRecords) {
            cout << "RunNumber: " << record.RunNumber
                 << ", ACCID: " << record.ACCID
                 << ", ManufacturerID: " << record.ManufacturerID
                 << ", Position: " << record.Position << endl;
        }

	int testRunNum = 5907;
        int testManuID = 39; // example ManufacturerID to query
        auto result= queryNearestACCID(idConfigRecords, testRunNum, testManuID);
        int nearestACCID = std::get<0>(result);
        std::string position = std::get<1>(result);
        cout << "Querying nearest ACCID for RunNumber: " << testRunNum << ", ManufacturerID: " << testManuID << endl;
        cout << "Nearest ACCID: " << nearestACCID << ", Position: " << position << endl;
    }

    return true;
}

void LAPPDLoadStore::CleanDataObjects()
{
    LAPPD_ID = -9999;
    Raw_buffer.clear();
    Parse_buffer.clear();
    ReadBoards.clear();
    data.clear();
    pps.clear();
    EventType = -9999;
    LAPPDana = false;
    ParaBoards.clear();
    meta.clear();
    LAPPDWaveforms.clear();
    data.clear();
    Parse_buffer.clear();
    // LAPPDDataMap.clear();
    // DataStreams.clear();
    runInfoLoaded = false;

    LAPPD_IDs.clear();

    LAPPDLoadedTimeStampsRaw_0.clear();
    LAPPDLoadedBeamgatesRaw_0.clear();
    LAPPDLoadedOffsets_0.clear();
    LAPPDLoadedTSCorrections_0.clear();
    LAPPDLoadedBGCorrections_0.clear();
    LAPPDLoadedOSInMinusPS_0.clear();
    LAPPDLoadedBG_PPSBefore_0.clear();
    LAPPDLoadedBG_PPSAfter_0.clear();
    LAPPDLoadedBG_PPSDiff_0.clear();
    LAPPDLoadedBG_PPSMissing_0.clear();
    LAPPDLoadedTS_PPSBefore_0.clear();
    LAPPDLoadedTS_PPSAfter_0.clear();
    LAPPDLoadedTS_PPSDiff_0.clear();
    LAPPDLoadedTS_PPSMissing_0.clear();

    LAPPDLoadedTimeStampsRaw_1.clear();
    LAPPDLoadedBeamgatesRaw_1.clear();
    LAPPDLoadedOffsets_1.clear();
    LAPPDLoadedTSCorrections_1.clear();
    LAPPDLoadedBGCorrections_1.clear();
    LAPPDLoadedOSInMinusPS_1.clear();
    LAPPDLoadedBG_PPSBefore_1.clear();
    LAPPDLoadedBG_PPSAfter_1.clear();
    LAPPDLoadedBG_PPSDiff_1.clear();
    LAPPDLoadedBG_PPSMissing_1.clear();
    LAPPDLoadedTS_PPSBefore_1.clear();
    LAPPDLoadedTS_PPSAfter_1.clear();
    LAPPDLoadedTS_PPSDiff_1.clear();
    LAPPDLoadedTS_PPSMissing_1.clear();
}

bool LAPPDLoadStore::Execute()
{
    // 1. Clean data variables
    // 2. Decide loading data or not, load the data from PsecData data to tool
    // 3. Parse and pass data to later tools

    CleanDataObjects();
    m_data->CStore.Set("LAPPD_new_event", false);
    LoadRunInfo();

    if (MultiLAPPDMap)
    {
        bool gotDataStream = m_data->Stores["ANNIEEvent"]->Get("DataStreams", DataStreams);
        bool getMap = m_data->Stores["ANNIEEvent"]->Get("LAPPDDataMap", LAPPDDataMap);
        
        if (getMap && DataStreams["LAPPD"] == true && LAPPDDataMap.size() > 0)
        {
            bool gotBeamgates_ns_0 = m_data->Stores["ANNIEEvent"]->Get("LAPPDBeamgate_ns_0", LAPPDBeamgate_ns_0);
            bool gotTimeStamps_ns_0 = m_data->Stores["ANNIEEvent"]->Get("LAPPDTimeStamps_ns_0", LAPPDTimeStamps_ns_0);
            bool gotTimeStampsRaw_0 = m_data->Stores["ANNIEEvent"]->Get("LAPPDTimeStampsRaw_0", LAPPDTimeStampsRaw_0);
            bool gotBeamgatesRaw_0 = m_data->Stores["ANNIEEvent"]->Get("LAPPDBeamgatesRaw_0", LAPPDBeamgatesRaw_0);
            bool gotOffsets_0 = m_data->Stores["ANNIEEvent"]->Get("LAPPDOffsets_0", LAPPDOffsets_0);
            bool gotTSCorrection_0 = m_data->Stores["ANNIEEvent"]->Get("LAPPDTSCorrection_0", LAPPDTSCorrection_0);
            bool gotDBGCorrection_0 = m_data->Stores["ANNIEEvent"]->Get("LAPPDBGCorrection_0", LAPPDBGCorrection_0);
            bool gotOSInMinusPS_0 = m_data->Stores["ANNIEEvent"]->Get("LAPPDOSInMinusPS_0", LAPPDOSInMinusPS_0);

            bool gotBeamgates_ns_1 = m_data->Stores["ANNIEEvent"]->Get("LAPPDBeamgate_ns_1", LAPPDBeamgate_ns_1);
            bool gotTimeStamps_ns_1 = m_data->Stores["ANNIEEvent"]->Get("LAPPDTimeStamps_ns_1", LAPPDTimeStamps_ns_1);
            bool gotTimeStampsRaw_1 = m_data->Stores["ANNIEEvent"]->Get("LAPPDTimeStampsRaw_1", LAPPDTimeStampsRaw_1);
            bool gotBeamgatesRaw_1 = m_data->Stores["ANNIEEvent"]->Get("LAPPDBeamgatesRaw_1", LAPPDBeamgatesRaw_1);
            bool gotOffsets_1 = m_data->Stores["ANNIEEvent"]->Get("LAPPDOffsets_1", LAPPDOffsets_1);
            bool gotTSCorrection_1 = m_data->Stores["ANNIEEvent"]->Get("LAPPDTSCorrection_1", LAPPDTSCorrection_1);
            bool gotDBGCorrection_1 = m_data->Stores["ANNIEEvent"]->Get("LAPPDBGCorrection_1", LAPPDBGCorrection_1);
            bool gotOSInMinusPS_1 = m_data->Stores["ANNIEEvent"]->Get("LAPPDOSInMinusPS_1", LAPPDOSInMinusPS_1);

            if (LoadBuiltPPSInfo)
            {
                bool gotBG_PPSBefore_0 = m_data->Stores["ANNIEEvent"]->Get("LAPPDBG_PPSBefore_0", LAPPDBG_PPSBefore_0);
                bool gotBG_PPSAfter_0  = m_data->Stores["ANNIEEvent"]->Get("LAPPDBG_PPSAfter_0", LAPPDBG_PPSAfter_0);
                bool gotBG_PPSDiff_0   = m_data->Stores["ANNIEEvent"]->Get("LAPPDBG_PPSDiff_0", LAPPDBG_PPSDiff_0);
                bool gotBG_PPSMissing_0 = m_data->Stores["ANNIEEvent"]->Get("LAPPDBG_PPSMissing_0", LAPPDBG_PPSMissing_0); 
                bool gotTS_PPSBefore_0 = m_data->Stores["ANNIEEvent"]->Get("LAPPDTS_PPSBefore_0", LAPPDTS_PPSBefore_0);
                bool gotTS_PPSAfter_0  = m_data->Stores["ANNIEEvent"]->Get("LAPPDTS_PPSAfter_0", LAPPDTS_PPSAfter_0);
                bool gotTS_PPSDiff_0   = m_data->Stores["ANNIEEvent"]->Get("LAPPDTS_PPSDiff_0", LAPPDTS_PPSDiff_0);
                bool gotTS_PPSMissing_0 = m_data->Stores["ANNIEEvent"]->Get("LAPPDTS_PPSMissing_0", LAPPDTS_PPSMissing_0);

                bool gotBG_PPSBefore_1 = m_data->Stores["ANNIEEvent"]->Get("LAPPDBG_PPSBefore_1", LAPPDBG_PPSBefore_1);
                bool gotBG_PPSAfter_1  = m_data->Stores["ANNIEEvent"]->Get("LAPPDBG_PPSAfter_1", LAPPDBG_PPSAfter_1);
                bool gotBG_PPSDiff_1   = m_data->Stores["ANNIEEvent"]->Get("LAPPDBG_PPSDiff_1", LAPPDBG_PPSDiff_1);
                bool gotBG_PPSMissing_1 = m_data->Stores["ANNIEEvent"]->Get("LAPPDBG_PPSMissing_1", LAPPDBG_PPSMissing_1);
                bool gotTS_PPSBefore_1 = m_data->Stores["ANNIEEvent"]->Get("LAPPDTS_PPSBefore_1", LAPPDTS_PPSBefore_1);
                bool gotTS_PPSAfter_1  = m_data->Stores["ANNIEEvent"]->Get("LAPPDTS_PPSAfter_1", LAPPDTS_PPSAfter_1);
                bool gotTS_PPSDiff_1   = m_data->Stores["ANNIEEvent"]->Get("LAPPDTS_PPSDiff_1", LAPPDTS_PPSDiff_1);
                bool gotTS_PPSMissing_1 = m_data->Stores["ANNIEEvent"]->Get("LAPPDTS_PPSMissing_1", LAPPDTS_PPSMissing_1);

                if (LAPPDLoadStoreVerbosity > 3)
                {
                    std::cout << "LAPPDLoadStore: gotOffsets = " << gotOffsets_0 << std::endl;		 
                    std::cout << "Size of LAPPDDataMap = " << LAPPDDataMap.size() 
                              << ", LAPPDOffsets_0 = " << LAPPDOffsets_0.size() 
                              << ", LAPPDOffsets_1 = " << LAPPDOffsets_1.size()
                              << ", LAPPDBG_PPSBefore_0 = " << LAPPDBG_PPSBefore_0.size() 
                              << ", LAPPDTS_PPSBefore_0 = " << LAPPDTS_PPSBefore_0.size() 
                              << ", LAPPDBG_PPSBefore_1 = " << LAPPDBG_PPSBefore_1.size() 
                              << ", LAPPDTS_PPSBefore_1 = " << LAPPDTS_PPSBefore_1.size() << std::endl;

                    std::cout << "Board 0: gotBG_PPSBefore = " << gotBG_PPSBefore_0 << ", gotBG_PPSAfter = " << gotBG_PPSAfter_0 
                              << ", gotBG_PPSDiff = " << gotBG_PPSDiff_0 << ", gotBG_PPSMissing = " << gotBG_PPSMissing_0 << std::endl;
                    
                    std::cout << "Board 0: gotTS_PPSBefore = " << gotTS_PPSBefore_0 << ", gotTS_PPSAfter = " << gotTS_PPSAfter_0 
                              << ", gotTS_PPSDiff = " << gotTS_PPSDiff_0 << ", gotTS_PPSMissing = " << gotTS_PPSMissing_0 << std::endl;
                         
                    std::cout << "Board 1: gotBG_PPSBefore = " << gotBG_PPSBefore_1 << ", gotBG_PPSAfter = " << gotBG_PPSAfter_1 
                              << ", gotBG_PPSDiff = " << gotBG_PPSDiff_1 << ", gotBG_PPSMissing = " << gotBG_PPSMissing_1 << std::endl;
                    
                    std::cout << "Board 1: gotTS_PPSBefore = " << gotTS_PPSBefore_1 << ", gotTS_PPSAfter = " << gotTS_PPSAfter_1 
                              << ", gotTS_PPSDiff = " << gotTS_PPSDiff_1 << ", gotTS_PPSMissing = " << gotTS_PPSMissing_1 << std::endl;\
                }
            }

            // --- BACKWARD COMPATIBILITY BLOCK ---
            // If the _0 identifiers were not found, this is older processed data.
            // Fetch the legacy variables (no suffixes) and map them to ACDC 0.
            if (!gotBeamgates_ns_0) 
            {
                if (LAPPDLoadStoreVerbosity > 0)
                    std::cout << "LAPPDLoadStore: Legacy data detected. Mapping old variables to ACDC 0." << std::endl;
                
                bool gotLegacy = m_data->Stores["ANNIEEvent"]->Get("LAPPDBeamgate_ns", LAPPDBeamgate_ns_0);
                
                if (gotLegacy) 
                {
                    m_data->Stores["ANNIEEvent"]->Get("LAPPDTimeStamps_ns", LAPPDTimeStamps_ns_0);
                    m_data->Stores["ANNIEEvent"]->Get("LAPPDTimeStampsRaw", LAPPDTimeStampsRaw_0);
                    m_data->Stores["ANNIEEvent"]->Get("LAPPDBeamgatesRaw", LAPPDBeamgatesRaw_0);
                    m_data->Stores["ANNIEEvent"]->Get("LAPPDOffsets", LAPPDOffsets_0);
                    m_data->Stores["ANNIEEvent"]->Get("LAPPDTSCorrection", LAPPDTSCorrection_0);
                    m_data->Stores["ANNIEEvent"]->Get("LAPPDBGCorrection", LAPPDBGCorrection_0);
                    m_data->Stores["ANNIEEvent"]->Get("LAPPDOSInMinusPS", LAPPDOSInMinusPS_0);
                }

                if (LoadBuiltPPSInfo && gotLegacy) 
                {
                    m_data->Stores["ANNIEEvent"]->Get("LAPPDBG_PPSBefore", LAPPDBG_PPSBefore_0);
                    m_data->Stores["ANNIEEvent"]->Get("LAPPDBG_PPSAfter", LAPPDBG_PPSAfter_0);
                    m_data->Stores["ANNIEEvent"]->Get("LAPPDBG_PPSDiff", LAPPDBG_PPSDiff_0);
                    m_data->Stores["ANNIEEvent"]->Get("LAPPDBG_PPSMissing", LAPPDBG_PPSMissing_0);
                    m_data->Stores["ANNIEEvent"]->Get("LAPPDTS_PPSBefore", LAPPDTS_PPSBefore_0);
                    m_data->Stores["ANNIEEvent"]->Get("LAPPDTS_PPSAfter", LAPPDTS_PPSAfter_0);
                    m_data->Stores["ANNIEEvent"]->Get("LAPPDTS_PPSDiff", LAPPDTS_PPSDiff_0);
                    m_data->Stores["ANNIEEvent"]->Get("LAPPDTS_PPSMissing", LAPPDTS_PPSMissing_0);
                }
            }
            // --- END BACKWARD COMPATIBILITY BLOCK ---   
        }
        else
        {
            return true;
        }
    }

    // decide loading data or not, set to LAPPDana for later tools
    LAPPDana = LoadData();
    m_data->CStore.Set("LAPPDana", LAPPDana);
    if (LAPPDLoadStoreVerbosity > 0)
        cout << "LAPPDana for loading was set to " << LAPPDana << endl;
    if (!LAPPDana)
    {
        // not loading data, return
        return true;
    }

    if (!MultiLAPPDMap)
    {
        // parse and pass data to later tools
        int frametype = static_cast<int>(Raw_buffer.size() / ReadBoards.size());
        if (frametype != num_vector_data && frametype != num_vector_pps)
        {
            cout << "Problem identifying the frametype, size of raw vector was " << Raw_buffer.size() << endl;
            cout << "It was expected to be either " << num_vector_data * ReadBoards.size() << " or " << num_vector_pps * ReadBoards.size() << endl;
            cout << "Please check manually!" << endl;
            LAPPDana = false;
            m_data->CStore.Set("LAPPDana", LAPPDana);
            m_data->CStore.Set("LAPPDPPShere", LAPPDana);
            return true;
        }

        if (frametype == num_vector_pps && loadPPS)
        {
            // if it's PPS, don't to anything relate to merging
            m_data->CStore.Set("LAPPDanaData", false);
            // set LAPPDana to false
            LAPPDana = false;
            m_data->CStore.Set("LAPPDana", LAPPDana);
            ParsePPSData();
            m_data->CStore.Set("LAPPD_ID", LAPPD_ID);
            m_data->Stores["ANNIEEvent"]->Set("LAPPD_ID", LAPPD_ID);
            m_data->CStore.Set("LoadingPPS", true);
            if (LAPPDLoadStoreVerbosity > 0)
                cout << "LAPPDLoadStore: PPS data loaded, LAPPDanaData is false, set LAPPDana to false" << endl;
            return true;
        }

        if (frametype == num_vector_data && loadPSEC)
        {
            m_data->CStore.Set("LAPPDanaData", true);
            LoadRunInfo();
            bool parsData = ParsePSECData();
	    runInfoLoaded = true;
            LAPPDana = parsData;
            m_data->CStore.Set("LAPPDana", LAPPDana);
            m_data->CStore.Set("LoadingPPS", false);

            if (!parsData)
            {
                cout << "LAPPDLoadStore: PSEC data parsing failed, set LAPPDana to false and return" << endl;

                return true;
            }
            NonEmptyDataEvents += 1;
        }

        // parsing finished, do pedestal subtraction
        DoPedestalSubtract();
        // save some timestamps relate to this event, for later using
        SaveTimeStamps();

        vector<int> ReadedBoards;
        vector<int> ACDCReadedLAPPDID;
        for (auto it = ReadBoards.begin(); it != ReadBoards.end(); it++)
        {
            ReadedBoards.push_back(*it);
            ACDCReadedLAPPDID.push_back(LAPPD_ID);
            // cout << "ReadedBoards loaded with " << *it << endl;
        }

        if (LAPPDLoadStoreVerbosity > 0)
            cout << "*************************END LAPPDLoadStore************************************" << endl;
        m_data->CStore.Set("LAPPD_ID", LAPPD_ID);
        m_data->Stores["ANNIEEvent"]->Set("LAPPD_ID", LAPPD_ID);
        m_data->Stores["ANNIEEvent"]->Set("RawLAPPDData", LAPPDWaveforms); // leave this only for the merger tool
        m_data->Stores["ANNIEEvent"]->Set("MergeLAPPDPsec", LAPPDWaveforms);
        m_data->Stores["ANNIEEvent"]->Set("ACDCmetadata", meta);
        m_data->Stores["ANNIEEvent"]->Set("ACDCboards", ReadBoards);
        m_data->Stores["ANNIEEvent"]->Set("SortedBoards", ParaBoards);
        m_data->Stores["ANNIEEvent"]->Set("TriggerChannelBase", TrigChannel);
        m_data->Stores["ANNIEEvent"]->Set("ACDCReadedLAPPDID", ACDCReadedLAPPDID);
        m_data->Stores["ANNIEEvent"]->Set("ReadedBoards", ReadedBoards);

        m_data->CStore.Set("NewLAPPDDataAvailable", true);
        if (LAPPDLoadStoreVerbosity > 5)
            debugLoadStore << " Set NewLAPPDDataAvailable to true" << endl;

        NonEmptyEvents += 1;
        eventNo++;
        if (LAPPDLoadStoreVerbosity > 2)
        {
            cout << "Finish LAPPDLoadStore, Printing the ANNIEEvent" << endl;
            m_data->Stores["ANNIEEvent"]->Print(false);
        }
    }
    else
    {
        // if we are reading multiple LAPPD data from one ANNIEEvent
        // assume we only have PSEC data in ANNIEEvent, no PPS event.
        // loop the map, for each PSEC data, do the same loading and parsing.
        // load the waveform by using LAPPD_ID * board_number * channel_number as the key

        // data was already loaded in the LoadData()

        vector<int> ReadedBoards;
        vector<int> ACDCReadedLAPPDID;

        if (LAPPDLoadStoreVerbosity > 0)
            cout << "LAPPDLoadStore: LAPPDDataMap has " << LAPPDDataMap.size() << " LAPPD PSEC data " << endl;
        bool ValidDataLoaded = false;
        std::map<unsigned long, PsecData>::iterator it;
        for (it = LAPPDDataMap.begin(); it != LAPPDDataMap.end(); it++)
        {
            ParaBoards.clear();
            uint64_t time = it->first;
            PsecData dat = it->second;
            ReadBoards = dat.BoardIndex; // From the data, board index is not related to the LAPPD_ID! WHY use this way?
            Raw_buffer = dat.RawWaveform;
            LAPPD_ID = dat.LAPPD_ID;

	    if (LAPPD_ID>20) {
                tuple<int, string> queryResult = queryNearestACCID(idConfigRecords, runNumber, LAPPD_ID);
                if (LAPPDLoadStoreVerbosity > 2)
                    cout << "LAPPDLoadStore: Mapped ManufacturerID " << LAPPD_ID << " to ACCID " << get<0>(queryResult) << " for run " << runNumber << endl;
              
	  	LAPPD_ID = get<0>(queryResult);
            }

            if (LAPPD_ID != SelectedLAPPD && SelectSingleLAPPD)
                continue;

            if (Raw_buffer.size() == 0 || ReadBoards.size() == 0)
            {
                m_data->CStore.Set("LAPPDana", false);
                cout << "LAPPD Load Store, find Raw buffer size == 0 or ReadBoards size == 0" << endl;
                continue;
                // return true;
            }

            if (LAPPDLoadStoreVerbosity > 0)
            {
                // print ReadBoards
                cout << "LAPPD ID " << LAPPD_ID << " ReadBoards size is " << ReadBoards.size() << ", data: " << endl;
                for (auto it = ReadBoards.begin(); it != ReadBoards.end(); it++)
                {
                    cout << ", " << *it;
                }
                cout << endl;
            }

            // push all elements in ReadBoards to ReadedBoards
            for (auto it = ReadBoards.begin(); it != ReadBoards.end(); it++)
            {
                ReadedBoards.push_back(*it);
                ACDCReadedLAPPDID.push_back(LAPPD_ID);
                // cout << "ReadedBoards loaded with " << *it << endl;
            }

            int frametype = static_cast<int>(Raw_buffer.size() / ReadBoards.size());
            if (frametype != num_vector_data)
            {
                cout << "LAPPDLoadStore: For LAPPD_ID " << LAPPD_ID << " frametype is not num_vector_data, skip this LAPPD" << endl;
                continue;
            }
            m_data->CStore.Set("LAPPDanaData", true);
            if (LAPPDLoadStoreVerbosity > 3)
            {
                cout << "Before parsing data, printing size and element in ReadBoards, ReadedBoards, ParaBoards" << endl;
                cout << "ReadBoards size is " << ReadBoards.size() << endl;
                for (auto it = ReadBoards.begin(); it != ReadBoards.end(); it++)
                {
                    cout << ", " << *it;
                }
                cout << endl;
                cout << "ReadedBoards size is " << ReadedBoards.size() << endl;
                for (auto it = ReadedBoards.begin(); it != ReadedBoards.end(); it++)
                {
                    cout << ", " << *it;
                }
                cout << endl;
                cout << "ParaBoards size is " << ParaBoards.size() << endl;
                for (auto it = ParaBoards.begin(); it != ParaBoards.end(); it++)
                {
                    cout << ", " << *it;
                }
                cout << endl;
            }
            bool parsData = ParsePSECData(); // TODO: now assuming all boards just has 30 channels. Need to be changed for gen 2
            if (parsData)
            {
                ValidDataLoaded = true;
                if (LAPPDLoadStoreVerbosity > 2)
                    cout << "LAPPDLoadStore: Loaded LAPPD data for LAPPD_ID " << LAPPD_ID << " at time " << time << endl;
                LAPPDLoadedTimeStamps.push_back(time);
                LAPPD_IDs.push_back(LAPPD_ID);

                // print the size of LAPPDTimeStampsRaw for Board 0 and Board 1, print all keys in them
                if (LAPPDLoadStoreVerbosity > 0)
                {
                    std::cout << "LAPPDTimeStampsRaw_0 size is " << LAPPDTimeStampsRaw_0.size() << std::endl;
                    for (auto it = LAPPDTimeStampsRaw_0.begin(); it != LAPPDTimeStampsRaw_0.end(); it++)
                    {
                        std::cout << "LAPPDTimeStampsRaw_0 key is " << it->first << std::endl;
                    }
                    std::cout << "LAPPDTimeStampsRaw_1 size is " << LAPPDTimeStampsRaw_1.size() << std::endl;
                    for (auto it = LAPPDTimeStampsRaw_1.begin(); it != LAPPDTimeStampsRaw_1.end(); it++)
                    {
                        std::cout << "LAPPDTimeStampsRaw_1 key is " << it->first << std::endl;
                    }
                }
                
                // print the size of LAPPDOffsets for Board 0 and Board 1, print all keys in it
                if (LAPPDLoadStoreVerbosity > 0)
                {
                    std::cout << "LAPPDOffsets_0 size is " << LAPPDOffsets_0.size() << std::endl;
                    for (auto it = LAPPDOffsets_0.begin(); it != LAPPDOffsets_0.end(); it++)
                    {
                        std::cout << "LAPPDOffsets_0 key is " << it->first << std::endl;
                    }
                    std::cout << "LAPPDOffsets_1 size is " << LAPPDOffsets_1.size() << std::endl;
                    for (auto it = LAPPDOffsets_1.begin(); it != LAPPDOffsets_1.end(); it++)
                    {
                        std::cout << "LAPPDOffsets_1 key is " << it->first << std::endl;
                    }
                }

                // --- ACDC 0 Loading ---
                // (Legacy data was mapped to _0, so these maps are guaranteed to have data)
                LAPPDLoadedTimeStampsRaw_0.push_back(LAPPDTimeStampsRaw_0.at(time));
                LAPPDLoadedBeamgatesRaw_0.push_back(LAPPDBeamgatesRaw_0.at(time));
                LAPPDLoadedOffsets_0.push_back(LAPPDOffsets_0.at(time));
                LAPPDLoadedTSCorrections_0.push_back(LAPPDTSCorrection_0.at(time));
                LAPPDLoadedBGCorrections_0.push_back(LAPPDBGCorrection_0.at(time));
                LAPPDLoadedOSInMinusPS_0.push_back(LAPPDOSInMinusPS_0.at(time));

                // --- ACDC 1 Loading ---
                // Check whether ACDC 1 has data; if not, this is legacy data with no ACDC 1 identifier
                if (LAPPDTimeStampsRaw_1.count(time) > 0) 
                {
                    LAPPDLoadedTimeStampsRaw_1.push_back(LAPPDTimeStampsRaw_1.at(time));
                    LAPPDLoadedBeamgatesRaw_1.push_back(LAPPDBeamgatesRaw_1.at(time));
                    LAPPDLoadedOffsets_1.push_back(LAPPDOffsets_1.at(time));
                    LAPPDLoadedTSCorrections_1.push_back(LAPPDTSCorrection_1.at(time));
                    LAPPDLoadedBGCorrections_1.push_back(LAPPDBGCorrection_1.at(time));
                    LAPPDLoadedOSInMinusPS_1.push_back(LAPPDOSInMinusPS_1.at(time));
                } 
                else 
                {
                    // Legacy data fallback: safely fill ACDC 1 with dummy zeros
                    LAPPDLoadedTimeStampsRaw_1.push_back(-1);
                    LAPPDLoadedBeamgatesRaw_1.push_back(-1);
                    LAPPDLoadedOffsets_1.push_back(-1);
                    LAPPDLoadedTSCorrections_1.push_back(-1);
                    LAPPDLoadedBGCorrections_1.push_back(-1);
                    LAPPDLoadedOSInMinusPS_1.push_back(-1);
                }

                if (LAPPDLoadStoreVerbosity > 2)
                    cout << "parsing finished for LAPPD_ID " << LAPPD_ID << " at time " << time << endl;

                if (LoadBuiltPPSInfo)
                {
                    LAPPDLoadedBG_PPSBefore_0.push_back(LAPPDBG_PPSBefore_0.at(time));
                    LAPPDLoadedBG_PPSAfter_0.push_back(LAPPDBG_PPSAfter_0.at(time));
                    LAPPDLoadedBG_PPSDiff_0.push_back(LAPPDBG_PPSDiff_0.at(time));
                    LAPPDLoadedBG_PPSMissing_0.push_back(LAPPDBG_PPSMissing_0.at(time));
                    LAPPDLoadedTS_PPSBefore_0.push_back(LAPPDTS_PPSBefore_0.at(time));
                    LAPPDLoadedTS_PPSAfter_0.push_back(LAPPDTS_PPSAfter_0.at(time));
                    LAPPDLoadedTS_PPSDiff_0.push_back(LAPPDTS_PPSDiff_0.at(time));
                    LAPPDLoadedTS_PPSMissing_0.push_back(LAPPDTS_PPSMissing_0.at(time));

                    if (LAPPDBG_PPSBefore_1.count(time) > 0) 
                    {
                        LAPPDLoadedBG_PPSBefore_1.push_back(LAPPDBG_PPSBefore_1.at(time));
                        LAPPDLoadedBG_PPSAfter_1.push_back(LAPPDBG_PPSAfter_1.at(time));
                        LAPPDLoadedBG_PPSDiff_1.push_back(LAPPDBG_PPSDiff_1.at(time));
                        LAPPDLoadedBG_PPSMissing_1.push_back(LAPPDBG_PPSMissing_1.at(time));
                        LAPPDLoadedTS_PPSBefore_1.push_back(LAPPDTS_PPSBefore_1.at(time));
                        LAPPDLoadedTS_PPSAfter_1.push_back(LAPPDTS_PPSAfter_1.at(time));
                        LAPPDLoadedTS_PPSDiff_1.push_back(LAPPDTS_PPSDiff_1.at(time));
                        LAPPDLoadedTS_PPSMissing_1.push_back(LAPPDTS_PPSMissing_1.at(time));
                    } 
                    else 
                    {
                        LAPPDLoadedBG_PPSBefore_1.push_back(-1);
                        LAPPDLoadedBG_PPSAfter_1.push_back(-1);
                        LAPPDLoadedBG_PPSDiff_1.push_back(-1);
                        LAPPDLoadedBG_PPSMissing_1.push_back(-1);
                        LAPPDLoadedTS_PPSBefore_1.push_back(-1);
                        LAPPDLoadedTS_PPSAfter_1.push_back(-1);
                        LAPPDLoadedTS_PPSDiff_1.push_back(-1);
                        LAPPDLoadedTS_PPSMissing_1.push_back(-1);
                    }

                    // --- Board 0 Check ---
                    if (LAPPDTS_PPSMissing_0.at(time) != LAPPDBG_PPSMissing_0.at(time) && 
                        ((LAPPDTS_PPSMissing_0.at(time) > -100 && LAPPDTS_PPSMissing_0.at(time) < 100) || 
                         (LAPPDBG_PPSMissing_0.at(time) > -100 && LAPPDBG_PPSMissing_0.at(time) < 100)))
                    {
                        std::cout << "LAPPDLoadStore: [Board 0] PPS missing mismatch for LAPPD_ID " << LAPPD_ID 
                              << " at time " << time << ", BG: " << LAPPDBG_PPSMissing_0.at(time) 
                              << ", TS: " << LAPPDTS_PPSMissing_0.at(time) << std::endl;
                        std::cout << "LAPPDLoadStore: [Board 0] BG_PPSDiff: " << LAPPDBG_PPSDiff_0.at(time) 
                              << ", TS_PPSDiff: " << LAPPDTS_PPSDiff_0.at(time) << std::endl;
                    }

                    // --- Board 1 Check ---
                    if (LAPPDTS_PPSMissing_1.count(time) > 0) {
                        if (LAPPDTS_PPSMissing_1.at(time) != LAPPDBG_PPSMissing_1.at(time) && 
                            ((LAPPDTS_PPSMissing_1.at(time) > -100 && LAPPDTS_PPSMissing_1.at(time) < 100) || 
                             (LAPPDBG_PPSMissing_1.at(time) > -100 && LAPPDBG_PPSMissing_1.at(time) < 100)))
                        {
                            std::cout << "LAPPDLoadStore: [Board 1] PPS missing mismatch for LAPPD_ID " << LAPPD_ID 
                              << " at time " << time << ", BG: " << LAPPDBG_PPSMissing_1.at(time) 
                              << ", TS: " << LAPPDTS_PPSMissing_1.at(time) << std::endl;
                            std::cout << "LAPPDLoadStore: [Board 1] BG_PPSDiff: " << LAPPDBG_PPSDiff_1.at(time) 
                              << ", TS_PPSDiff: " << LAPPDTS_PPSDiff_1.at(time) << std::endl;
                        }
                    }
                }
            }
            NonEmptyEvents += 1;
            NonEmptyDataEvents += 1;
        }
        eventNo++;
        
        LAPPDana = ValidDataLoaded;
        m_data->CStore.Set("LAPPDana", LAPPDana);
        DoPedestalSubtract();

        m_data->Stores["ANNIEEvent"]->Set("RawLAPPDData", LAPPDWaveforms); // leave this only for the merger tool
        m_data->Stores["ANNIEEvent"]->Set("LAPPD_IDs", LAPPD_IDs);
        m_data->Stores["ANNIEEvent"]->Set("LAPPDLoadedTimeStamps", LAPPDLoadedTimeStamps);
        m_data->Stores["ANNIEEvent"]->Set("ACDCboards", ReadedBoards);
        m_data->Stores["ANNIEEvent"]->Set("ACDCReadedLAPPDID", ACDCReadedLAPPDID);
        m_data->Stores["ANNIEEvent"]->Set("ACDCmetadata", meta);

        m_data->Stores["ANNIEEvent"]->Set("LAPPDDataMap", LAPPDDataMap);

        m_data->Stores["ANNIEEvent"]->Set("LAPPDBeamgate_ns_0", LAPPDBeamgate_ns_0);
        m_data->Stores["ANNIEEvent"]->Set("LAPPDTimeStamps_ns_0", LAPPDTimeStamps_ns_0);
        m_data->Stores["ANNIEEvent"]->Set("LAPPDTimeStampsRaw_0", LAPPDTimeStampsRaw_0);
        m_data->Stores["ANNIEEvent"]->Set("LAPPDBeamgatesRaw_0", LAPPDBeamgatesRaw_0);
        m_data->Stores["ANNIEEvent"]->Set("LAPPDOffsets_0", LAPPDOffsets_0);
        m_data->Stores["ANNIEEvent"]->Set("LAPPDTSCorrection_0", LAPPDTSCorrection_0);
        m_data->Stores["ANNIEEvent"]->Set("LAPPDBGCorrection_0", LAPPDBGCorrection_0);
        m_data->Stores["ANNIEEvent"]->Set("LAPPDOSInMinusPS_0", LAPPDOSInMinusPS_0);

        m_data->Stores["ANNIEEvent"]->Set("LAPPDBeamgate_ns_1", LAPPDBeamgate_ns_1);
        m_data->Stores["ANNIEEvent"]->Set("LAPPDTimeStamps_ns_1", LAPPDTimeStamps_ns_1);
        m_data->Stores["ANNIEEvent"]->Set("LAPPDTimeStampsRaw_1", LAPPDTimeStampsRaw_1);
        m_data->Stores["ANNIEEvent"]->Set("LAPPDBeamgatesRaw_1", LAPPDBeamgatesRaw_1);
        m_data->Stores["ANNIEEvent"]->Set("LAPPDOffsets_1", LAPPDOffsets_1);
        m_data->Stores["ANNIEEvent"]->Set("LAPPDTSCorrection_1", LAPPDTSCorrection_1);
        m_data->Stores["ANNIEEvent"]->Set("LAPPDBGCorrection_1", LAPPDBGCorrection_1);
        m_data->Stores["ANNIEEvent"]->Set("LAPPDOSInMinusPS_1", LAPPDOSInMinusPS_1);

        if (LoadBuiltPPSInfo)
        {
            m_data->Stores["ANNIEEvent"]->Set("LAPPDBG_PPSBefore_0", LAPPDBG_PPSBefore_0);
            m_data->Stores["ANNIEEvent"]->Set("LAPPDBG_PPSAfter_0", LAPPDBG_PPSAfter_0);
            m_data->Stores["ANNIEEvent"]->Set("LAPPDBG_PPSDiff_0", LAPPDBG_PPSDiff_0);
            m_data->Stores["ANNIEEvent"]->Set("LAPPDBG_PPSMissing_0", LAPPDBG_PPSMissing_0);
            m_data->Stores["ANNIEEvent"]->Set("LAPPDTS_PPSBefore_0", LAPPDTS_PPSBefore_0);
            m_data->Stores["ANNIEEvent"]->Set("LAPPDTS_PPSAfter_0", LAPPDTS_PPSAfter_0);
            m_data->Stores["ANNIEEvent"]->Set("LAPPDTS_PPSDiff_0", LAPPDTS_PPSDiff_0);
            m_data->Stores["ANNIEEvent"]->Set("LAPPDTS_PPSMissing_0", LAPPDTS_PPSMissing_0);
        
            m_data->Stores["ANNIEEvent"]->Set("LAPPDBG_PPSBefore_1", LAPPDBG_PPSBefore_1);
            m_data->Stores["ANNIEEvent"]->Set("LAPPDBG_PPSAfter_1", LAPPDBG_PPSAfter_1);
            m_data->Stores["ANNIEEvent"]->Set("LAPPDBG_PPSDiff_1", LAPPDBG_PPSDiff_1);
            m_data->Stores["ANNIEEvent"]->Set("LAPPDBG_PPSMissing_1", LAPPDBG_PPSMissing_1);
            m_data->Stores["ANNIEEvent"]->Set("LAPPDTS_PPSBefore_1", LAPPDTS_PPSBefore_1);
            m_data->Stores["ANNIEEvent"]->Set("LAPPDTS_PPSAfter_1", LAPPDTS_PPSAfter_1);
            m_data->Stores["ANNIEEvent"]->Set("LAPPDTS_PPSDiff_1", LAPPDTS_PPSDiff_1);
            m_data->Stores["ANNIEEvent"]->Set("LAPPDTS_PPSMissing_1", LAPPDTS_PPSMissing_1);
        }

        // TODO: save other timestamps, variables and metadata for later use

        if (eventNo % 100 == 0) {
            std::cout << "LAPPDLoadStore: Loaded " << eventNo << " events, " << NonEmptyDataEvents << " non empty LAPPD PSEC data loaded from all LAPPDs" << std::endl;
        }
    }

    if (LAPPDLoadStoreVerbosity > 0)
        cout << "LAPPDLoadStore: Finished loading LAPPD data" << endl;

    return true;
}

bool LAPPDLoadStore::Finalise()
{
    cout << "\033[1;34mLAPPDLoadStore: Finalising\033[0m" << endl;
    cout << "LAPPDLoadStore: Got pps event in total: " << PPSnumber << endl;
    cout << "LAPPDLoadStore: Got error data or PPS events in total: " << errorEventsNumber << endl;
    cout << "LAPPDLoadStore: Got non empty data and PPS events in total: " << NonEmptyEvents << endl;
    cout << "LAPPDLoadStore: Got non empty data events in total: " << NonEmptyDataEvents << endl;
    cout << "LAPPDLoadStore: End at event number: " << eventNo << endl;
    return true;
}

bool LAPPDLoadStore::ReadPedestals(int boardNo)
{

    if (LAPPDLoadStoreVerbosity > 0)
        cout << "Getting Pedestals " << boardNo << endl;

    std::string LoadName = PedFileNameTXT;
    string nextLine; // temp line to parse
    double finalsampleNo;
    std::string ext = std::to_string(boardNo);
    ext += ".txt";
    LoadName += ext;
    PedFile.open(LoadName); // final name: PedFileNameTXT + boardNo + .txt
    if (!PedFile.is_open())
    {
        cout << "Failed to open " << LoadName << "!" << endl;
        return false;
    }
    if (LAPPDLoadStoreVerbosity > 0)
        cout << "Opened file: " << LoadName << endl;

    int sampleNo = 0; // sample number
    while (getline(PedFile, nextLine))
    {
        istringstream iss(nextLine);            // copies the current line in the file
        int location = -1;                      // counts the current perameter in the line
        string stempValue;                      // current string in the line
        int tempValue;                          // current int in the line
        unsigned long channelNo = boardNo * 30; // channel number
        // cout<<"NEW BOARD "<<channelNo<<" "<<sampleNo<<endl;
        // starts the loop at the beginning of the line
        while (iss >> stempValue)
        {
            location++;
            int tempValue = stoi(stempValue, 0, 10);
            if (sampleNo == 0)
            {
                vector<int> tempPed;
                tempPed.push_back(tempValue);
                // cout<<"First time: "<<channelNo<<" "<<tempValue<<endl;
                PedestalValues->insert(pair<unsigned long, vector<int>>(channelNo, tempPed));
                if (LAPPDLoadStoreVerbosity > 0)
                    cout << "Inserting pedestal at channelNo " << channelNo << endl;
                // newboard=false;
            }
            else
            {
                // cout<<"Following time: "<<channelNo<<" "<<tempValue<<"    F    "<<PedestalValues->count(channelNo)<<endl;
                (((PedestalValues->find(channelNo))->second)).push_back(tempValue);
            }

            channelNo++;
        }
        sampleNo++;
    }
    if (LAPPDLoadStoreVerbosity > 0)
        cout << "FINAL SAMPLE NUMBER: " << PedestalValues->size() << " " << (((PedestalValues->find(0))->second)).size() << endl;
    PedFile.close();
    return true;
}

bool LAPPDLoadStore::MakePedestals()
{

    // Empty for now...
    // should be moved to ASCII readin?

    return true;
}

int LAPPDLoadStore::getParsedMeta(std::vector<unsigned short> buffer, int BoardId)
{
    // Catch empty buffers
    if (buffer.size() == 0)
    {
        std::cout << "You tried to parse ACDC data without pulling/setting an ACDC buffer" << std::endl;
        return -1;
    }

    // Prepare the Metadata vector
    // meta.clear();

    // Helpers
    int chip_count = 0;

    // Indicator words for the start/end of the metadata
    const unsigned short startword = 0xBA11;
    unsigned short endword = 0xFACE;
    unsigned short endoffile = 0x4321;

    // Empty metadata map for each Psec chip <PSEC #, vector with information>
    map<int, vector<unsigned short>> PsecInfo;

    // Empty trigger metadata map for each Psec chip <PSEC #, vector with trigger>
    map<int, vector<unsigned short>> PsecTriggerInfo;
    unsigned short CombinedTriggerRateCount;

    // Empty vector with positions of aboves startword
    vector<int> start_indices =
        {
            1539, 3091, 4643, 6195, 7747};

    // Fill the psec info map
    vector<unsigned short>::iterator bit;
    for (int i : start_indices)
    {
        // Write the first word after the startword
        bit = buffer.begin() + (i + 1);

        // As long as the endword isn't reached copy metadata words into a vector and add to map
        vector<unsigned short> InfoWord;
        while (*bit != endword && *bit != endoffile && InfoWord.size() < 14)
        {
            InfoWord.push_back(*bit);
            ++bit;
        }
        PsecInfo.insert(pair<int, vector<unsigned short>>(chip_count, InfoWord));
        chip_count++;
    }

    // Fill the psec trigger info map
    for (int chip = 0; chip < NUM_PSEC; chip++)
    {
        for (int ch = 0; ch < NUM_CH / NUM_PSEC; ch++)
        {
            if (LAPPDLoadStoreVerbosity > 10)
                cout << "parsing meta step1-1" << endl;
            // Find the trigger data at begin + last_metadata_start + 13_info_words + 1_end_word + 1
            bit = buffer.begin() + start_indices[4] + 13 + 1 + 1 + ch + (chip * (NUM_CH / NUM_PSEC));
            if (LAPPDLoadStoreVerbosity > 10)
                cout << "parsing meta step1-2" << endl;
            PsecTriggerInfo[chip].push_back(*bit);
        }
    }

    if (LAPPDLoadStoreVerbosity > 10)
        cout << "parsing meta step1.5" << endl;
    // Fill the combined trigger
    CombinedTriggerRateCount = buffer[7792];

    //----------------------------------------------------------
    // Start the metadata parsing

    meta.push_back(BoardId);
    for (int CHIP = 0; CHIP < NUM_PSEC; CHIP++)
    {
        meta.push_back((0xDCB0 | CHIP));
        // cout<<"size of info word is "<<PsecInfo[CHIP].size()<<endl;
        // cout<<"size of trigger word is "<<PsecTriggerInfo[CHIP].size()<<endl;
        for (int INFOWORD = 0; INFOWORD < 13; INFOWORD++)
        {
            if (LAPPDLoadStoreVerbosity > 10)
                cout << "parsing meta step2-1 infoword " << INFOWORD << endl;
            if (PsecInfo[CHIP].size() < 13)
            {
                NonEmptyEvents = NonEmptyEvents - 1;
                NonEmptyDataEvents = NonEmptyDataEvents - 1;
                cout << "meta data parsing wrong! PsecInfo[CHIP].size() < 13" << endl;
                m_data->CStore.Set("LAPPDana", false);
                return 1;
            }

            try
            {
                meta.push_back(PsecInfo[CHIP][INFOWORD]);
            }
            catch (...)
            {
                NonEmptyEvents = NonEmptyEvents - 1;
                NonEmptyDataEvents = NonEmptyDataEvents - 1;
                cout << "meta data parsing wrong! meta.push_back(PsecInfo[CHIP][INFOWORD]);" << endl;
                m_data->CStore.Set("LAPPDana", false);
                return 1;
            }
        }
        for (int TRIGGERWORD = 0; TRIGGERWORD < 6; TRIGGERWORD++)
        {
            if (LAPPDLoadStoreVerbosity > 10)
                cout << "parsing meta step2-2 trigger word" << endl;

            if (PsecTriggerInfo[CHIP].size() < 6)
            {
                NonEmptyEvents = NonEmptyEvents - 1;
                NonEmptyDataEvents = NonEmptyDataEvents - 1;
                cout << "meta data parsing wrong! PsecTriggerInfo[CHIP].size() < 6" << endl;
                m_data->CStore.Set("LAPPDana", false);
                return 1;
            }

            try
            {
                meta.push_back(PsecTriggerInfo[CHIP][TRIGGERWORD]);
            }
            catch (...)
            {
                NonEmptyEvents = NonEmptyEvents - 1;
                NonEmptyDataEvents = NonEmptyDataEvents - 1;
                cout << "meta data parsing wrong! meta.push_back(PsecTriggerInfo[CHIP][TRIGGERWORD]);" << endl;
                m_data->CStore.Set("LAPPDana", false);
                return 1;
            }
        }
    }

    meta.push_back(CombinedTriggerRateCount);
    meta.push_back(0xeeee);
    return 0;
}

int LAPPDLoadStore::getParsedData(std::vector<unsigned short> buffer, int ch_start)
{
    // Catch empty buffers
    if (buffer.size() == 0)
    {
        std::cout << "You tried to parse ACDC data without pulling/setting an ACDC buffer" << std::endl;
        return -1;
    }

    // Helpers
    int DistanceFromZero;
    int channel_count = 0;

    // Indicator words for the start/end of the metadata
    const unsigned short startword = 0xF005;
    unsigned short endword = 0xBA11;
    unsigned short endoffile = 0x4321;

    // Empty vector with positions of aboves startword
    vector<int> start_indices =
        {
            2, 1554, 3106, 4658, 6210};

    // Fill data map
    vector<unsigned short>::iterator bit;
    for (int i : start_indices)
    {
        // Write the first word after the startword
        bit = buffer.begin() + (i + 1);

        // As long as the endword isn't reached copy metadata words into a vector and add to map
        vector<unsigned short> InfoWord;
        while (*bit != endword && *bit != endoffile)
        {
            InfoWord.push_back((unsigned short)*bit);
            if (InfoWord.size() == NUM_SAMP)
            {
                data.insert(pair<int, vector<unsigned short>>(ch_start + channel_count, InfoWord));
                if (LAPPDLoadStoreVerbosity > 5)
                    cout << "inserted data to channel " << ch_start + channel_count << endl;
                InfoWord.clear();
                channel_count++;
            }
            ++bit;
        }
    }

    return 0;
}

bool LAPPDLoadStore::LoadData()
{
    // TODO: when looping in Stores["ANNIEEvent"], the multiple PSEC data will be saved in std::map<double,PsecData> LAPPDDatas;
    // so we need to loop the map to get all data and waveforms, using the LAPPD_ID, board number, channel number to form a global channel-waveform map
    // then loop all waveforms based on channel number

    if (loadFromStoreDirectly)
        m_data->Stores["ANNIEEvent"]->GetEntry(eventNo);
    if (LAPPDLoadStoreVerbosity > 2)
        cout << "Got eventNo " << eventNo << endl;

    // if loaded enough events, stop the loop and return false
    if (NonEmptyEvents == stopEntries || NonEmptyEvents > stopEntries || NonEmptyDataEvents == stopEntries || NonEmptyDataEvents > stopEntries)
    {
        if (LAPPDLoadStoreVerbosity > 0)
            cout << "LAPPDLoadStore: NonEmptyEvents is " << NonEmptyEvents << ", NonEmptyDataEvents is " << NonEmptyDataEvents << ", stopEntries is " << stopEntries << ", stop the loop" << endl;
        m_data->vars.Set("StopLoop", 1);
        return false;
    }

    // if (mergedEvent)
    //     DataStreams["LAPPD"] = true;

    // print the load information: DataStreams["LAPPD"] value, PsecReceiveMode, MultiLAPPDMap
    if (LAPPDLoadStoreVerbosity > 0)
    {
        cout << "LAPPDLoadStore: DataStreams[LAPPD] is " << DataStreams["LAPPD"] << ", PsecReceiveMode is " << PsecReceiveMode << ", MultiLAPPDMap is " << MultiLAPPDMap << endl;
    }

    if (loadPSEC || loadPPS)
    { // if load any kind of data
        // if there is no LAPPD data in event store, and not getting data from CStore, return false, don't load
        if (!DataStreams["LAPPD"] && PsecReceiveMode == 0 && !MultiLAPPDMap) // if doesn't have datastream, not reveive data from raw data store (PsecReceiveMode), not loading multiple LAPPD map from processed data
        {
            return false;
        }
        else if (PsecReceiveMode == 1 && !MultiLAPPDMap) // no LAPPD data in event store, but load from CStore (for merging LAPPD to ANNIEEvent)
        {                                                // only get PSEC object from CStore
            // if loading from raw data, and the loading was set to pause, return false
            bool LAPPDRawLoadingPaused = false;
            m_data->CStore.Get("PauseLAPPDDecoding", LAPPDRawLoadingPaused);
            if (LAPPDRawLoadingPaused)
            {
                m_data->CStore.Set("NewLAPPDDataAvailable", false);
                return false;
            }

            PsecData dat;
            bool getData = m_data->CStore.Get("LAPPDData", dat);
            if (getData)
            {
                m_data->CStore.Set("StoreLoadedLAPPDData", dat);
            }
            if (LAPPDLoadStoreVerbosity > 0)
                cout << "LAPPDLoadStore: getting LAPPDData from CStore" << endl;
            bool mergingLoad;
            // if in merging mode, but no LAPPD data in CStore, return false, don't load
            m_data->CStore.Get("LAPPDanaData", mergingLoad);
            if (!mergingLoad && mergingModeReadIn)
            {
                if (LAPPDLoadStoreVerbosity > 0)
                    cout << "LAPPDLoadStore: mergingMode is true but LAPPDanaData is false, set LAPPDana to false" << endl;
                return false;
            }
            if (getData)
            {
                vector<unsigned int> errorcodes = dat.errorcodes;
                if (errorcodes.size() == 1 && errorcodes[0] == 0x00000000)
                {
                    if (LAPPDLoadStoreVerbosity > 1)
                        printf("No errorcodes found all good: 0x%08x\n", errorcodes[0]);
                }
                else
                {
                    printf("When Loading PPS: Errorcodes found: %li\n", errorcodes.size());
                    for (unsigned int k = 0; k < errorcodes.size(); k++)
                    {
                        printf("Errorcode: 0x%08x\n", errorcodes[k]);
                    }
                    errorEventsNumber++;
                    return false;
                }
                ReadBoards = dat.BoardIndex;
                Raw_buffer = dat.RawWaveform;
                if (Raw_buffer.size() == 0 || ReadBoards.size() == 0)
                {
                    cout << "LAPPD Load Store, find Raw buffer size == 0 or ReadBoards size == 0" << endl;
                    return false;
                }
                LAPPD_ID = dat.LAPPD_ID;

		if (LAPPD_ID>20) {
                    tuple<int, string> queryResult = queryNearestACCID(idConfigRecords, runNumber, LAPPD_ID);
                    if (LAPPDLoadStoreVerbosity > 2)
                        cout << "LAPPDLoadStore: Mapped ManufacturerID  " << LAPPD_ID << " to ACCID " << get<0>(queryResult) << " for run " << runNumber << endl;
                  
	      	    LAPPD_ID = get<0>(queryResult);
                }

                if (LAPPD_ID != SelectedLAPPD && SelectSingleLAPPD)
                    return false;
                m_data->CStore.Set("PsecTimestamp", dat.Timestamp);
                if (LAPPDLoadStoreVerbosity > 2)
                {
                    cout << " Got Data " << endl;
                    dat.Print();
                }
                int frameType = static_cast<int>(Raw_buffer.size() / ReadBoards.size());
                if (LAPPDLoadStoreVerbosity > 0)
                    cout << "LAPPDLoadStore: got Data from CStore, frame type is " << frameType << endl;
                if (loadPSEC)
                {
                    if (frameType == num_vector_data)
                    {
                        m_data->CStore.Set("LoadingPPS", false);
                        return true;
                    }
                }
                if (loadPPS)
                {
                    if (frameType == num_vector_pps)
                    {
                        m_data->CStore.Set("LoadingPPS", true);
                        return true;
                    }
                }
                return false;
            }
            else
            {
                return false;
            }
        }
        else if (DataStreams["LAPPD"] && PsecReceiveMode == 0 && !MultiLAPPDMap) // if load single lappd data at ID 0, require Datastream, not receive from cstore, not in multiLAPPDMap mode
        {
            PsecData dat;
            m_data->Stores["ANNIEEvent"]->Get("LAPPDData", dat);
            ReadBoards = dat.BoardIndex;
            Raw_buffer = dat.RawWaveform;
            LAPPD_ID = dat.LAPPD_ID;

	    if (LAPPD_ID>20) {
                tuple<int, string> queryResult = queryNearestACCID(idConfigRecords, runNumber, LAPPD_ID);
                if (LAPPDLoadStoreVerbosity > 2)
                    cout << "LAPPDLoadStore: Mapped ManufacturerID  " << LAPPD_ID << " to ACCID " << get<0>(queryResult) << " for run " << runNumber << endl;
                    
		LAPPD_ID = get<0>(queryResult);
            }

            if (LAPPD_ID != SelectedLAPPD && SelectSingleLAPPD)
                return false;
            m_data->CStore.Set("PsecTimestamp", dat.Timestamp);

            if (Raw_buffer.size() != 0 || ReadBoards.size() != 0)
            {
                if (LAPPDLoadStoreVerbosity > 0)
                {
                    cout << "Getting data length format" << static_cast<int>(Raw_buffer.size() / ReadBoards.size()) << ", psec timestamp is " << dat.Timestamp << endl;
                    cout << "ReadBoards size " << ReadBoards.size() << " Raw_buffer size " << Raw_buffer.size() << " LAPPD_ID " << LAPPD_ID << endl;
                }
            }
            else
            {
                cout << "LAPPDLoadStore: loading data with raw buffer size 0 or ReadBoards size 0, skip loading" << endl;
                cout << "ReadBoards size " << ReadBoards.size() << " Raw_buffer size " << Raw_buffer.size() << " LAPPD_ID " << LAPPD_ID << endl;

                return false;
            }
            return true;
        }
        else if (DataStreams["LAPPD"] && PsecReceiveMode == 0 && MultiLAPPDMap) // if not receive from cstore, and load multi lappd map
        {
            if (LAPPDLoadStoreVerbosity > 0)
                cout << "LAPPDLoadStore: Loading multiple LAPPD data from ANNIEEvent" << "Inside, size of LAPPDDatamap = " << LAPPDDataMap.size() << endl;

            if (LAPPDDataMap.size() == 0)
            {
                cout << "what happened?" << endl;
                return false;
            }

            return true;
        }
    }
    return false; // if not any of the above, return false
}

void LAPPDLoadStore::ParsePPSData()
{
    if (LAPPDLoadStoreVerbosity > 0)
        cout << "Loading PPS frame size " << pps.size() << endl;
    std::vector<unsigned short> pps = Raw_buffer;
    std::vector<unsigned long> pps_vector;
    std::vector<unsigned long> pps_count_vector;

    unsigned long pps_timestamp = 0;
    unsigned long ppscount = 0;
    for (int s = 0; s < ReadBoards.size(); s++)
    {
        unsigned short pps_63_48 = pps.at(2 + 16 * s);
        unsigned short pps_47_32 = pps.at(3 + 16 * s);
        unsigned short pps_31_16 = pps.at(4 + 16 * s);
        unsigned short pps_15_0 = pps.at(5 + 16 * s);
        std::bitset<16> bits_pps_63_48(pps_63_48);
        std::bitset<16> bits_pps_47_32(pps_47_32);
        std::bitset<16> bits_pps_31_16(pps_31_16);
        std::bitset<16> bits_pps_15_0(pps_15_0);
        unsigned long pps_63_0 = (static_cast<unsigned long>(pps_63_48) << 48) + (static_cast<unsigned long>(pps_47_32) << 32) + (static_cast<unsigned long>(pps_31_16) << 16) + (static_cast<unsigned long>(pps_15_0));
        if (LAPPDLoadStoreVerbosity > 0)
            std::cout << "pps combined: " << pps_63_0 << std::endl;
        std::bitset<64> bits_pps_63_0(pps_63_0);
        // pps_timestamp = pps_63_0 * (CLOCK_to_NSEC); // NOTE: Don't do convert to ns because of the precision, do this in later tools
        pps_timestamp = pps_63_0;
        // LAPPDPPS->push_back(pps_timestamp);
        if (LAPPDLoadStoreVerbosity > 0)
            std::cout << "Adding timestamp " << pps_timestamp << " to LAPPDPPS" << std::endl;
        pps_vector.push_back(pps_timestamp);

        unsigned short ppscount_31_16 = pps.at(8 + 16 * s);
        unsigned short ppscount_15_0 = pps.at(9 + 16 * s);
        std::bitset<16> bits_ppscount_31_16(ppscount_31_16);
        std::bitset<16> bits_ppscount_15_0(ppscount_15_0);
        unsigned long ppscount_31_0 = (static_cast<unsigned long>(ppscount_31_16) << 16) + (static_cast<unsigned long>(ppscount_15_0));
        if (LAPPDLoadStoreVerbosity > 0)
            std::cout << "pps count combined: " << ppscount_31_0 << std::endl;
        std::bitset<32> bits_ppscount_31_0(ppscount_31_0);
        ppscount = ppscount_31_0;
        pps_count_vector.push_back(ppscount);

        if (LAPPDLoadStoreVerbosity > 8)
        {
            // Print the bitsets
            cout << "******************************" << endl;
            std::cout << "printing ACDC " << s << ": " << endl;
            std::cout << "bits_pps_63_48: " << bits_pps_63_48 << std::endl;
            std::cout << "bits_pps_47_32: " << bits_pps_47_32 << std::endl;
            std::cout << "bits_pps_31_16: " << bits_pps_31_16 << std::endl;
            std::cout << "bits_pps_15_0: " << bits_pps_15_0 << std::endl;
            // Print the unsigned shorts
            std::cout << "pps_63_48: " << pps_63_48 << std::endl;
            std::cout << "pps_47_32: " << pps_47_32 << std::endl;
            std::cout << "pps_31_16: " << pps_31_16 << std::endl;
            std::cout << "pps_15_0: " << pps_15_0 << std::endl;
            std::cout << "pps_63_0: " << pps_63_0 << std::endl;
            std::cout << "pps_63_0 after conversion in double: " << pps_timestamp << endl;

            for (int x = 0; x < 16; x++)
            {
                std::bitset<16> bit_pps_here(pps.at(x + 16 * s));
                cout << "unsigned short at " << x << " : " << pps.at(x + 16 * s) << ", bit at " << x << " is: " << bit_pps_here << endl;
                ;
            }
        }
    }

    // double ppsDiff = static_cast<double>(pps_vector.at(0)) - static_cast<double>(pps_vector.at(1));
    unsigned long ppsDiff = pps_vector.at(0) - pps_vector.at(1);
    m_data->CStore.Set("LAPPDPPScount0", pps_count_vector.at(0));
    m_data->CStore.Set("LAPPDPPScount1", pps_count_vector.at(1));
    m_data->CStore.Set("LAPPDPPScount", pps_count_vector);
    m_data->CStore.Set("LAPPDPPSDiff0to1", ppsDiff);
    m_data->CStore.Set("LAPPDPPSVector", pps_vector);

    m_data->CStore.Set("LAPPDPPStimestamp0", pps_vector.at(0));
    m_data->CStore.Set("LAPPDPPStimestamp1", pps_vector.at(1));
    m_data->CStore.Set("LAPPDPPShere", true);
    m_data->CStore.Set("LAPPD_ID", LAPPD_ID);

    PPSnumber++;
}

bool LAPPDLoadStore::ParsePSECData()
{
    if (LAPPDLoadStoreVerbosity > 0)
        std::cout << "PSEC Data Frame was read! Starting the parsing!" << std::endl;

    // while loading single PsecData Object, parse the data by LAPPDID and number of boards on each LAPPD and channel on each board
    //  Create a vector of paraphrased board indices
    //  the board indices goes with LAPPD ID. For example, LAPPD ID = 2, we will have board = 4,5
    //  this need to be converted to 0,1
    int nbi = ReadBoards.size();
    if (LAPPDLoadStoreVerbosity > 0 && nbi != 2)
        cout << "Number of board is " << nbi << endl;
    if (nbi == 0)
    {
        cout << "LAPPDLoadStore: error here! number of board is 0" << endl;
        errorEventsNumber++;
        return false;
    }
    if (nbi % 2 != 0)
    {
        errorEventsNumber++;
        cout << "LAPPDLoadStore: uneven number of boards in this event" << endl;
        if (nbi == 1)
        {
            ParaBoards.push_back(ReadBoards[0]);
        }
        else
        {
            return false;
        }
    }
    else
    {
        for (int cbi = 0; cbi < nbi; cbi++)
        {
            ParaBoards.push_back(cbi);
            if (LAPPDLoadStoreVerbosity > 2)
                cout << "Board " << cbi << " is added to the list of boards to be parsed!" << endl;
        }
    }
    // loop all boards, 0, 1
    if (LAPPDLoadStoreVerbosity > 2)
    {
        cout << "ParaBoards size is " << ParaBoards.size() << endl;
        for (int i = 0; i < ParaBoards.size(); i++)
        {
            cout << "ParaBoards " << i << " is " << ParaBoards[i] << endl;
        }
    }
    for (int i = 0; i < ParaBoards.size(); i++)
    {
        int bi = ParaBoards.at(i) % 2;
        Parse_buffer.clear();
        if (LAPPDLoadStoreVerbosity > 2)
            std::cout << "Parsing board with ReadBoards ID" << ReadBoards[bi] << std::endl;
        // Go over all ACDC board data frames by seperating them
        int frametype = static_cast<int>(Raw_buffer.size() / ReadBoards.size());
        for (int c = bi * frametype; c < (bi + 1) * frametype; c++)
        {
            Parse_buffer.push_back(Raw_buffer[c]);
        }
        if (LAPPDLoadStoreVerbosity > 2)
            std::cout << "Data for " << i << "_th board with board number = " << ReadBoards[bi] << " was grabbed!" << std::endl;

        // Grab the parsed data and give it to a global variable 'data'
        // insert the data start with channel number 30*ReadBoards[bi]
        // for instance, when bi=0 , LAPPD ID = 2, ReadBoards[bi] = 4, insert to channel number start with 120, to 150
        int channelShift = bi * NUM_CH + LAPPD_ID * NUM_CH * 2;
	if (LAPPDLoadStoreVerbosity > 2) 
	    std::cout << "bi= " << bi << ", LAPPD_ID= " << LAPPD_ID << ", NUM_CH= " << NUM_CH << ", channelShift= " << channelShift << std::endl;

	retval = getParsedData(Parse_buffer, channelShift); //(because there are only 2 boards, so it's 0*30 or 1*30). Inserting the channel number start from this then ++ to 30
        if (retval == 0)
        {
            if (LAPPDLoadStoreVerbosity > 2)
                std::cout << "Data for board with number = " << ReadBoards[bi] << " was parsed with channel shift " << channelShift << endl;
            // Grab the parsed metadata and give it to a global variable 'meta'
            retval = getParsedMeta(Parse_buffer, bi + LAPPD_ID * 2);
            if (retval != 0)
            {
                std::cout << "Meta parsing went wrong! " << retval << endl;
                return false;
            }
            else
            {
                if (LAPPDLoadStoreVerbosity > 2)
                    std::cout << "Meta for board " << ReadBoards[bi] << " was parsed!" << std::endl;
            }
        }
        else
        {
            std::cout << "Parsing went wrong! " << retval << endl;
            return false;
        }
    }

    LAPPDEventIndex_ID[LAPPD_ID] += 1;
    if (LAPPDLoadStoreVerbosity > 1)
    {
        cout << "Adding one new event with LAPPD_ID = " << LAPPD_ID << " to the LAPPDEventIndex_ID, now it is: " << endl;
        for (int i = 0; i < LAPPDEventIndex_ID.size(); i++)
        {
            cout <<  LAPPDEventIndex_ID[i] << ", " << endl;
        }
        cout << endl;
    }

    if (LAPPDLoadStoreVerbosity > 2)
        cout << "Parsed all boards for this event finished" << endl;
    return true;
}

bool LAPPDLoadStore::DoPedestalSubtract()
{
    if (LAPPDLoadStoreVerbosity > 0)
        cout << "LAPPDLoadStore::DoPedestalSubtract()" << endl;
    if (DoPedSubtract == 0)
        return true;
    Waveform<double> tmpWave;
    vector<Waveform<double>> VecTmpWave;
    int pedval, val;
    if (LAPPDLoadStoreVerbosity > 3)
    {
        // print the size of data and all keys, and the size of PedestalValues and all keys
        cout << "Size of data is " << data.size() << endl;
        for (std::map<int, vector<unsigned short>>::iterator it = data.begin(); it != data.end(); ++it) // looping over the data map by channel number, from 0 to 60
        {
            cout << it->first << ", ";
        }
        cout << endl;
        cout << "Size of PedestalValues is " << PedestalValues->size() << endl;
        for (auto it = PedestalValues->begin(); it != PedestalValues->end(); ++it) // looping over the data map by channel number, from 0 to 60
        {
            cout << it->first << ", ";
        }
        cout << endl;
    }
    // Loop over data stream
    for (std::map<int, vector<unsigned short>>::iterator it = data.begin(); it != data.end(); ++it) // looping over the data map by channel number, from 0 to 60
    {
        int wrongPedChannel = 0;
        if (LAPPDLoadStoreVerbosity > 5)
            cout << "Do Pedestal sub at Channel " << it->first;

        for (int kvec = 0; kvec < it->second.size(); kvec++)
        { // loop all data point in this channel
            if (DoPedSubtract == 1)
            {
                auto iter = PedestalValues->find((it->first));
                if (kvec == 0 && LAPPDLoadStoreVerbosity > 5)
                    cout << std::fixed << ", found PedestalValues for channel " << it->first << " with value = " << iter->second.at(0);
                if (iter != PedestalValues->end() && iter->second.size() > kvec)
                {
                    pedval = iter->second.at(kvec);
                }
                else
                {
                    pedval = 0;
                    wrongPedChannel = (it->first);
                }
            }
            else
            {
                pedval = 0;
            }
            val = it->second.at(kvec);
            tmpWave.PushSample(0.3 * (double)(val - pedval));
            if (LAPPDLoadStoreVerbosity > 5 && kvec < 10)
                cout << ", " << val << "-" << pedval << "=" << 0.3 * (double)(val - pedval);
        }
        if (wrongPedChannel != 0)
            cout << "Pedestal value not found for channel " << wrongPedChannel << "with it->first channel" << it->first << ", LAPPD channel shift " << LAPPD_ID * 60 << endl;

        VecTmpWave.push_back(tmpWave);

        unsigned long pushChannelNo = (unsigned long)it->first;
        LAPPDWaveforms.insert(pair<unsigned long, vector<Waveform<double>>>(pushChannelNo, VecTmpWave));
        // cout<<", Pushed to LAPPDWaveforms with channel number "<<pushChannelNo<<endl;

        tmpWave.ClearSamples();
        VecTmpWave.clear();
    }
    return true;
}

void LAPPDLoadStore::SaveTimeStamps()
{

    // PROTECTION: We expect exactly two boards (ACDC 0 and ACDC 1) for each LAPPD. 
    // Each board produces 103 metadata words (total 206).
    // If either ACDC 0 OR ACDC 1 is missing, the offset fitting script ignored this event.
    // We must abort and roll back the index to prevent desynchronization from the ROOT tree.
    if (meta.size() < 206) 
    {
        std::cout << "LAPPDLoadStore::SaveTimeStamps, WARNING: Event " << eventNo 
                  << " is missing a board (meta size: " << meta.size() 
                  << "). Expected = 206. Skipping to maintain offset index sync!" << std::endl;
        
        // Roll back the index that was incremented in ParsePSECData
        if (LAPPDEventIndex_ID[LAPPD_ID] > 0) {
            LAPPDEventIndex_ID[LAPPD_ID] -= 1;
        }
        return; 
    }

    // Reconstructing beamgate from ACDC 0
    unsigned short beamgate0_63_48 = meta.at(7);
    unsigned short beamgate0_47_32 = meta.at(27);
    unsigned short beamgate0_31_16 = meta.at(47);
    unsigned short beamgate0_15_0 = meta.at(67);
    
    unsigned long beamgate0_63_0 = (static_cast<unsigned long>(beamgate0_63_48) << 48) 
        + (static_cast<unsigned long>(beamgate0_47_32) << 32) 
        + (static_cast<unsigned long>(beamgate0_31_16) << 16) 
        + (static_cast<unsigned long>(beamgate0_15_0));
    
    unsigned long beamgate0_timestamp = beamgate0_63_0 * CLOCK_TO_NSEC;
    
    // Reconstructing beamgate from ACDC 1
    // There are 103 metadata words per ACDC, so the ACDC 1 beamgate is located 103 words after the corresponding ACDC 0 beamgate words.
    unsigned short beamgate1_63_48 = meta.at(7 + ACDC_META_WORDS);
    unsigned short beamgate1_47_32 = meta.at(27 + ACDC_META_WORDS);
    unsigned short beamgate1_31_16 = meta.at(47 + ACDC_META_WORDS);
    unsigned short beamgate1_15_0  = meta.at(67 + ACDC_META_WORDS);

    unsigned long beamgate1_63_0 =
        (static_cast<unsigned long>(beamgate1_63_48) << 48) +
        (static_cast<unsigned long>(beamgate1_47_32) << 32) +
        (static_cast<unsigned long>(beamgate1_31_16) << 16) +
        static_cast<unsigned long>(beamgate1_15_0);

    unsigned long beamgate1_timestamp = beamgate1_63_0 * CLOCK_TO_NSEC;

    m_data->CStore.Set("LAPPDbeamgate0", beamgate0_timestamp);
    m_data->CStore.Set("LAPPDBeamgate0_Raw", beamgate0_63_0);
    m_data->CStore.Set("LAPPDbeamgate1", beamgate1_timestamp);
    m_data->CStore.Set("LAPPDBeamgate1_Raw", beamgate1_63_0);

    unsigned long BG0_Truncation = beamgate0_63_0 % 8;
    unsigned long BG0_Truncated = beamgate0_63_0 - BG0_Truncation;
    unsigned long BG0_Int = BG0_Truncated / 8 * 25;
    unsigned long BG0_IntTruncation = BG0_Truncation * 3;
    double BG0_Float = BG0_Truncation * 0.125;
    unsigned long BG0_IntCombined = BG0_Int + BG0_IntTruncation;

    unsigned long BG1_Truncation = beamgate1_63_0 % 8;
    unsigned long BG1_Truncated = beamgate1_63_0 - BG1_Truncation;
    unsigned long BG1_Int = BG1_Truncated / 8 * 25;
    unsigned long BG1_IntTruncation = BG1_Truncation * 3;
    double BG1_Float = BG1_Truncation * 0.125;
    unsigned long BG1_IntCombined = BG1_Int + BG1_IntTruncation;

    m_data->CStore.Set("LAPPDBG0_IntCombined", BG0_IntCombined);
    m_data->CStore.Set("LAPPDBG0_Float", BG0_Float);
    m_data->CStore.Set("LAPPDBG1_IntCombined", BG1_IntCombined);
    m_data->CStore.Set("LAPPDBG1_Float", BG1_Float);

    // Reconstructing timestamp from ACDC 0
    unsigned short timestamp0_63_48 = meta.at(70);
    unsigned short timestamp0_47_32 = meta.at(50);
    unsigned short timestamp0_31_16 = meta.at(30);
    unsigned short timestamp0_15_0 = meta.at(10);
    
    unsigned long timestamp0_63_0 = 
        (static_cast<unsigned long>(timestamp0_63_48) << 48) + 
        (static_cast<unsigned long>(timestamp0_47_32) << 32) + 
        (static_cast<unsigned long>(timestamp0_31_16) << 16) + 
        (static_cast<unsigned long>(timestamp0_15_0));

    unsigned long lappd_timestamp0 = timestamp0_63_0 * CLOCK_TO_NSEC;

    // Reconstructing timestamp from ACDC 1
    unsigned short timestamp1_63_48 = meta.at(70 + ACDC_META_WORDS);
    unsigned short timestamp1_47_32 = meta.at(50 + ACDC_META_WORDS);
    unsigned short timestamp1_31_16 = meta.at(30 + ACDC_META_WORDS);
    unsigned short timestamp1_15_0 = meta.at(10 + ACDC_META_WORDS);
    
    unsigned long timestamp1_63_0 = 
        (static_cast<unsigned long>(timestamp1_63_48) << 48) + 
        (static_cast<unsigned long>(timestamp1_47_32) << 32) + 
        (static_cast<unsigned long>(timestamp1_31_16) << 16) + 
        (static_cast<unsigned long>(timestamp1_15_0));

    unsigned long lappd_timestamp1 = timestamp1_63_0 * CLOCK_TO_NSEC;

    m_data->CStore.Set("LAPPDtimestamp0", lappd_timestamp0);
    m_data->CStore.Set("LAPPDTimestamp0_Raw", timestamp0_63_0);
    m_data->CStore.Set("LAPPDtimestamp1", lappd_timestamp1);
    m_data->CStore.Set("LAPPDTimestamp1_Raw", timestamp1_63_0);

    unsigned long TS0_Truncation = timestamp0_63_0 % 8;
    unsigned long TS0_Truncated = timestamp0_63_0 - TS0_Truncation;
    unsigned long TS0_Int = TS0_Truncated / 8 * 25;
    unsigned long TS0_IntTruncation = TS0_Truncation * 3;
    double TS0_Float = TS0_Truncation * 0.125;
    unsigned long TS0_IntCombined = TS0_Int + TS0_IntTruncation;

    unsigned long TS1_Truncation = timestamp1_63_0 % 8;
    unsigned long TS1_Truncated = timestamp1_63_0 - TS1_Truncation;
    unsigned long TS1_Int = TS1_Truncated / 8 * 25;
    unsigned long TS1_IntTruncation = TS1_Truncation * 3;
    double TS1_Float = TS1_Truncation * 0.125;
    unsigned long TS1_IntCombined = TS1_Int + TS1_IntTruncation;

    m_data->CStore.Set("LAPPDTS0_IntCombined", TS0_IntCombined);
    m_data->CStore.Set("LAPPDTS0_Float", TS0_Float);
    m_data->CStore.Set("LAPPDTS1_IntCombined", TS1_IntCombined);
    m_data->CStore.Set("LAPPDTS1_Float", TS1_Float);

    m_data->Stores["ANNIEEvent"]->Set("LAPPDbeamgate0", beamgate0_timestamp); // in ns
    m_data->Stores["ANNIEEvent"]->Set("LAPPDtimestamp0", lappd_timestamp0);   // in ns
    m_data->Stores["ANNIEEvent"]->Set("LAPPDBeamgate0_Raw", beamgate0_63_0);
    m_data->Stores["ANNIEEvent"]->Set("LAPPDTimestamp0_Raw", timestamp0_63_0);
    m_data->Stores["ANNIEEvent"]->Set("LAPPDbeamgate1", beamgate1_timestamp); // in ns
    m_data->Stores["ANNIEEvent"]->Set("LAPPDtimestamp1", lappd_timestamp1);   // in ns
    m_data->Stores["ANNIEEvent"]->Set("LAPPDBeamgate1_Raw", beamgate1_63_0);
    m_data->Stores["ANNIEEvent"]->Set("LAPPDTimestamp1_Raw", timestamp1_63_0);

    if (LAPPDLoadStoreVerbosity > 5) {
        debugLoadStore
            << "Event number: " << eventNo
            << ", LAPPD ID: " << LAPPD_ID
            << ", PSEC index: " << LAPPDEventIndex_ID[LAPPD_ID]
            << ", ACC: " << meta[0]
            << ", PPS index: " << PPSnumber
            << ", Timestamp0: " << timestamp0_63_0
            << ", Beamgate0: " << beamgate0_63_0
            << ", Timestamp1: " << timestamp1_63_0
            << ", Beamgate1: " << beamgate1_63_0
            << std::endl;
    }

    if (loadOffsets && runInfoLoaded)
    {
        // run number + sub run number + partfile number + LAPPD_ID to make a string key
        // TODO: need to add a reset number here after got everything work
        // search it in the maps, then load
        SaveOffsets();
    }
    m_data->CStore.Set("LAPPD_new_event", true);
}

void LAPPDLoadStore::SaveOffsets()
{
    int LoadingOffsetID = LAPPD_ID;
    if(LoadingOffsetID + 1 > LAPPDEventIndex_ID.size())
    {
        LAPPDEventIndex_ID.resize(LoadingOffsetID + 1);
    }
    
    int GetOffsetIndex_byID = LAPPDEventIndex_ID[LoadingOffsetID] - 1;

    if(LAPPDLoadStoreVerbosity > 0) {
        cout << "LAPPDLoadStore::SaveOffsets, Loading offset for LAPPD_ID: " << LoadingOffsetID << ", OffsetIndex of this event: " << GetOffsetIndex_byID << endl;
    }

    std::string key = std::to_string(runNumber) + "_" + std::to_string(subRunNumber) + "_" + std::to_string(partFileNumber) + "_" + std::to_string(LAPPD_ID);

    // Variables for Board 0
    int LAPPDBGCorrection_0 = 0, LAPPDTSCorrection_0 = 0; 
    int LAPPDOffset_minus_ps_0 = 0;
    uint64_t LAPPDOffset_0 = 0;
    uint64_t BG_PPSBefore_0 = 0, BG_PPSAfter_0 = 0, BG_PPSDiff_0 = 0;
    uint64_t TS_PPSBefore_0 = 0, TS_PPSAfter_0 = 0, TS_PPSDiff_0 = 0;
    int BG_PPSMissing_0 = 0, TS_PPSMissing_0 = 0;

    // Variables for Board 1
    int LAPPDBGCorrection_1 = 0, LAPPDTSCorrection_1 = 0;
    int LAPPDOffset_minus_ps_1 = 0;
    uint64_t LAPPDOffset_1 = 0;
    uint64_t BG_PPSBefore_1 = 0, BG_PPSAfter_1 = 0, BG_PPSDiff_1 = 0;
    uint64_t TS_PPSBefore_1 = 0, TS_PPSAfter_1 = 0, TS_PPSDiff_1 = 0;
    int BG_PPSMissing_1 = 0, TS_PPSMissing_1 = 0;

    // Check if the key exists and the index is within range for loaded offsets and corrections
    auto fetchValue = [&](const auto& mapObj, const std::string& mapName, auto& targetVar) {
        auto it = mapObj.find(key);
        if (it != mapObj.end() && GetOffsetIndex_byID < it->second.size()) {
            targetVar = it->second[GetOffsetIndex_byID];
        } else {
            if (it == mapObj.end()) {
                std::cerr << "Error: Key not found in " << mapName << ": " << key << std::endl;
            } else {
                std::cerr << "Error: Index out of range for " << mapName << " with key: " << key << std::endl;
            }
        }
    };

    // Fetch Board 0 variables
    fetchValue(BGCorrections_0, "BGCorrections_0", LAPPDBGCorrection_0);
    fetchValue(TSCorrections_0, "TSCorrections_0", LAPPDTSCorrection_0);
    fetchValue(Offsets_minus_ps_0, "Offsets_minus_ps_0", LAPPDOffset_minus_ps_0);
    fetchValue(Offsets_0, "Offsets_0", LAPPDOffset_0);
    fetchValue(BG_PPSBefore_loaded_0, "BG_PPSBefore_loaded_0", BG_PPSBefore_0);
    fetchValue(BG_PPSAfter_loaded_0, "BG_PPSAfter_loaded_0", BG_PPSAfter_0);
    fetchValue(BG_PPSDiff_loaded_0, "BG_PPSDiff_loaded_0", BG_PPSDiff_0);
    fetchValue(BG_PPSMissing_loaded_0, "BG_PPSMissing_loaded_0", BG_PPSMissing_0);
    fetchValue(TS_PPSBefore_loaded_0, "TS_PPSBefore_loaded_0", TS_PPSBefore_0);
    fetchValue(TS_PPSAfter_loaded_0, "TS_PPSAfter_loaded_0", TS_PPSAfter_0);
    fetchValue(TS_PPSDiff_loaded_0, "TS_PPSDiff_loaded_0", TS_PPSDiff_0);
    fetchValue(TS_PPSMissing_loaded_0, "TS_PPSMissing_loaded_0", TS_PPSMissing_0);

    // Fetch Board 1 variables
    fetchValue(BGCorrections_1, "BGCorrections_1", LAPPDBGCorrection_1);
    fetchValue(TSCorrections_1, "TSCorrections_1", LAPPDTSCorrection_1);
    fetchValue(Offsets_minus_ps_1, "Offsets_minus_ps_1", LAPPDOffset_minus_ps_1);
    fetchValue(Offsets_1, "Offsets_1", LAPPDOffset_1);
    fetchValue(BG_PPSBefore_loaded_1, "BG_PPSBefore_loaded_1", BG_PPSBefore_1);
    fetchValue(BG_PPSAfter_loaded_1, "BG_PPSAfter_loaded_1", BG_PPSAfter_1);
    fetchValue(BG_PPSDiff_loaded_1, "BG_PPSDiff_loaded_1", BG_PPSDiff_1);
    fetchValue(BG_PPSMissing_loaded_1, "BG_PPSMissing_loaded_1", BG_PPSMissing_1);
    fetchValue(TS_PPSBefore_loaded_1, "TS_PPSBefore_loaded_1", TS_PPSBefore_1);
    fetchValue(TS_PPSAfter_loaded_1, "TS_PPSAfter_loaded_1", TS_PPSAfter_1);
    fetchValue(TS_PPSDiff_loaded_1, "TS_PPSDiff_loaded_1", TS_PPSDiff_1);
    fetchValue(TS_PPSMissing_loaded_1, "TS_PPSMissing_loaded_1", TS_PPSMissing_1);

    // Push Board 0 to CStore
    m_data->CStore.Set("LAPPDBGCorrection_0", LAPPDBGCorrection_0);
    m_data->CStore.Set("LAPPDTSCorrection_0", LAPPDTSCorrection_0);
    m_data->CStore.Set("LAPPDOffset_0", LAPPDOffset_0);
    m_data->CStore.Set("LAPPDOffset_minus_ps_0", LAPPDOffset_minus_ps_0);
    m_data->CStore.Set("BG_PPSBefore_0", BG_PPSBefore_0);
    m_data->CStore.Set("BG_PPSAfter_0", BG_PPSAfter_0);
    m_data->CStore.Set("BG_PPSDiff_0", BG_PPSDiff_0);
    m_data->CStore.Set("BG_PPSMissing_0", BG_PPSMissing_0);
    m_data->CStore.Set("TS_PPSBefore_0", TS_PPSBefore_0);
    m_data->CStore.Set("TS_PPSAfter_0", TS_PPSAfter_0);
    m_data->CStore.Set("TS_PPSDiff_0", TS_PPSDiff_0);
    m_data->CStore.Set("TS_PPSMissing_0", TS_PPSMissing_0);

    // Push Board 1 to CStore
    m_data->CStore.Set("LAPPDBGCorrection_1", LAPPDBGCorrection_1);
    m_data->CStore.Set("LAPPDTSCorrection_1", LAPPDTSCorrection_1);
    m_data->CStore.Set("LAPPDOffset_1", LAPPDOffset_1);
    m_data->CStore.Set("LAPPDOffset_minus_ps_1", LAPPDOffset_minus_ps_1);
    m_data->CStore.Set("BG_PPSBefore_1", BG_PPSBefore_1);
    m_data->CStore.Set("BG_PPSAfter_1", BG_PPSAfter_1);
    m_data->CStore.Set("BG_PPSDiff_1", BG_PPSDiff_1);
    m_data->CStore.Set("BG_PPSMissing_1", BG_PPSMissing_1);
    m_data->CStore.Set("TS_PPSBefore_1", TS_PPSBefore_1);
    m_data->CStore.Set("TS_PPSAfter_1", TS_PPSAfter_1);
    m_data->CStore.Set("TS_PPSDiff_1", TS_PPSDiff_1);
    m_data->CStore.Set("TS_PPSMissing_1", TS_PPSMissing_1);

    if (TS_PPSMissing_0 != BG_PPSMissing_0 || TS_PPSMissing_1 != BG_PPSMissing_1) {
        std::cout << "LAPPDLoadStore::SaveOffsets, PPS Missing mismatch detected. BG_0: " << BG_PPSMissing_0 
             << " TS_0: " << TS_PPSMissing_0 << " | BG_1: " << BG_PPSMissing_1 
             << " TS_1: " << TS_PPSMissing_1 << std::endl;
    }
    
    if (LAPPDLoadStoreVerbosity > 5) {
        debugLoadStore
            << "Event number: " << eventNo
            << ", LAPPDLoadStore, Saving offsets, key: " << key
            << ", LAPPD_ID: " << LAPPD_ID
            << ", idx: " << GetOffsetIndex_byID
            << std::endl;
    
        debugLoadStore
            << ", TS_PPSBefore_0: " << TS_PPSBefore_0
            << ", BG_PPSBefore_0: " << BG_PPSBefore_0
            << ", TS_PPSAfter_0: " << TS_PPSAfter_0
            << ", BG_PPSAfter_0: " << BG_PPSAfter_0
            << ", LAPPDOffset_0: " << LAPPDOffset_0
	    << ", LAPPDOffset_minus_ps_0: " << LAPPDOffset_minus_ps_0
	    << ", LAPPDTSCorrection_0: " << LAPPDTSCorrection_0
	    << ", LAPPDBGCorrection_0: " << LAPPDBGCorrection_0
            << std::endl;

        debugLoadStore
            << ", TS_PPSBefore_1: " << TS_PPSBefore_1
            << ", BG_PPSBefore_1: " << BG_PPSBefore_1
            << ", TS_PPSAfter_1: " << TS_PPSAfter_1
            << ", BG_PPSAfter_1: " << BG_PPSAfter_1
            << ", LAPPDOffset_1: " << LAPPDOffset_1
            << ", LAPPDOffset_minus_ps_1: " << LAPPDOffset_minus_ps_1
            << ", LAPPDTSCorrection_1: " << LAPPDTSCorrection_1
            << ", LAPPDBGCorrection_1: " << LAPPDBGCorrection_1
            << std::endl;
    }
}

void LAPPDLoadStore::LoadOffsetsAndCorrections()
{
    // Load Offsets from both ACDCs here from the offsetFitResult.root output.
    /*
    std::map<string, vector<uint64_t>> Offsets; //Loaded offset, use string = run number + sub run number + partfile number as key.
    std::map<string, vector<int>> Offsets_minus_ps; //offset in ps, use offset - this/1e3 as the real offset
    std::map<string, vector<int>> BGCorrections; //Loaded BGcorrections, same key as Offsets, but offset saved on event by event basis in that part file, in unit of ticks
    std::map<string, vector<int>> TSCorrections; //TS corrections, in unit of ticks
    */

    TFile *file = new TFile("offsetFitResult.root", "READ");
    TTree *tree;
    file->GetObject("Events", tree);

    if (!tree)
    {
        std::cerr << "LAPPDLoadStore Loading offsets, Tree not found!" << std::endl;
        return;
    }

    int runNumber, subRunNumber, partFileNumber, LAPPD_ID;
    ULong64_t EventIndex;

    // ACDC 0 variables
    ULong64_t final_offset_ns_0, final_offset_ps_negative_0;
    ULong64_t BGCorrection_tick_0, TSCorrection_tick_0;
    ULong64_t BG_PPSBefore_tick_0, BG_PPSAfter_tick_0, BG_PPSDiff_tick_0, BG_PPSMissing_tick_0;
    ULong64_t TS_PPSBefore_tick_0, TS_PPSAfter_tick_0, TS_PPSDiff_tick_0, TS_PPSMissing_tick_0;
    ULong64_t TS_driftCorrection_ns_0, BG_driftCorrection_ns_0;

    // ACDC 1 variables
    ULong64_t final_offset_ns_1, final_offset_ps_negative_1;
    ULong64_t BGCorrection_tick_1, TSCorrection_tick_1;
    ULong64_t BG_PPSBefore_tick_1, BG_PPSAfter_tick_1, BG_PPSDiff_tick_1, BG_PPSMissing_tick_1;
    ULong64_t TS_PPSBefore_tick_1, TS_PPSAfter_tick_1, TS_PPSDiff_tick_1, TS_PPSMissing_tick_1;
    ULong64_t TS_driftCorrection_ns_1, BG_driftCorrection_ns_1;

    tree->SetBranchAddress("runNumber", &runNumber);
    tree->SetBranchAddress("subRunNumber", &subRunNumber);
    tree->SetBranchAddress("partFileNumber", &partFileNumber);
    tree->SetBranchAddress("LAPPD_ID", &LAPPD_ID);
    tree->SetBranchAddress("EventIndex", &EventIndex);

    // Map ACDC 0 branches
    tree->SetBranchAddress("final_offset_ns_0", &final_offset_ns_0);
    tree->SetBranchAddress("final_offset_ps_negative_0", &final_offset_ps_negative_0);
    tree->SetBranchAddress("BGCorrection_tick_0", &BGCorrection_tick_0);
    tree->SetBranchAddress("TSCorrection_tick_0", &TSCorrection_tick_0);
    tree->SetBranchAddress("BG_PPSBefore_tick_0", &BG_PPSBefore_tick_0);
    tree->SetBranchAddress("BG_PPSAfter_tick_0", &BG_PPSAfter_tick_0);
    tree->SetBranchAddress("BG_PPSDiff_tick_0", &BG_PPSDiff_tick_0);
    tree->SetBranchAddress("BG_PPSMissing_tick_0", &BG_PPSMissing_tick_0);
    tree->SetBranchAddress("TS_PPSBefore_tick_0", &TS_PPSBefore_tick_0);
    tree->SetBranchAddress("TS_PPSAfter_tick_0", &TS_PPSAfter_tick_0);
    tree->SetBranchAddress("TS_PPSDiff_tick_0", &TS_PPSDiff_tick_0);
    tree->SetBranchAddress("TS_PPSMissing_tick_0", &TS_PPSMissing_tick_0);
    tree->SetBranchAddress("TS_driftCorrection_ns_0", &TS_driftCorrection_ns_0);
    tree->SetBranchAddress("BG_driftCorrection_ns_0", &BG_driftCorrection_ns_0);

    // Map ACDC 1 branches
    tree->SetBranchAddress("final_offset_ns_1", &final_offset_ns_1);
    tree->SetBranchAddress("final_offset_ps_negative_1", &final_offset_ps_negative_1);
    tree->SetBranchAddress("BGCorrection_tick_1", &BGCorrection_tick_1);
    tree->SetBranchAddress("TSCorrection_tick_1", &TSCorrection_tick_1);
    tree->SetBranchAddress("BG_PPSBefore_tick_1", &BG_PPSBefore_tick_1);
    tree->SetBranchAddress("BG_PPSAfter_tick_1", &BG_PPSAfter_tick_1);
    tree->SetBranchAddress("BG_PPSDiff_tick_1", &BG_PPSDiff_tick_1);
    tree->SetBranchAddress("BG_PPSMissing_tick_1", &BG_PPSMissing_tick_1);
    tree->SetBranchAddress("TS_PPSBefore_tick_1", &TS_PPSBefore_tick_1);
    tree->SetBranchAddress("TS_PPSAfter_tick_1", &TS_PPSAfter_tick_1);
    tree->SetBranchAddress("TS_PPSDiff_tick_1", &TS_PPSDiff_tick_1);
    tree->SetBranchAddress("TS_PPSMissing_tick_1", &TS_PPSMissing_tick_1);
    tree->SetBranchAddress("TS_driftCorrection_ns_1", &TS_driftCorrection_ns_1);
    tree->SetBranchAddress("BG_driftCorrection_ns_1", &BG_driftCorrection_ns_1);

    Long64_t nentries = tree->GetEntries();
    std::cout << "LAPPDLoadStore::LoadOffsetsAndCorrections, total entries: " << nentries << std::endl;
    for (Long64_t i = 0; i < nentries; ++i)
    {
        tree->GetEntry(i);
        std::string key = std::to_string(runNumber) + "_" + std::to_string(subRunNumber) + "_" + std::to_string(partFileNumber) + "_" + std::to_string(LAPPD_ID);

        // Resize vectors dynamically for EventIndex
        if (Offsets_0[key].size() <= EventIndex)
        {
            Offsets_0[key].resize(EventIndex + 1);
            Offsets_minus_ps_0[key].resize(EventIndex + 1);
            BGCorrections_0[key].resize(EventIndex + 1);
            TSCorrections_0[key].resize(EventIndex + 1);
            BG_PPSBefore_loaded_0[key].resize(EventIndex + 1);
            BG_PPSAfter_loaded_0[key].resize(EventIndex + 1);
            BG_PPSDiff_loaded_0[key].resize(EventIndex + 1);
            BG_PPSMissing_loaded_0[key].resize(EventIndex + 1);
            TS_PPSBefore_loaded_0[key].resize(EventIndex + 1);
            TS_PPSAfter_loaded_0[key].resize(EventIndex + 1);
            TS_PPSDiff_loaded_0[key].resize(EventIndex + 1);
            TS_PPSMissing_loaded_0[key].resize(EventIndex + 1);
        
            Offsets_1[key].resize(EventIndex + 1);
            Offsets_minus_ps_1[key].resize(EventIndex + 1);
            BGCorrections_1[key].resize(EventIndex + 1);
            TSCorrections_1[key].resize(EventIndex + 1);
            BG_PPSBefore_loaded_1[key].resize(EventIndex + 1);
            BG_PPSAfter_loaded_1[key].resize(EventIndex + 1);
            BG_PPSDiff_loaded_1[key].resize(EventIndex + 1);
            BG_PPSMissing_loaded_1[key].resize(EventIndex + 1);
            TS_PPSBefore_loaded_1[key].resize(EventIndex + 1);
            TS_PPSAfter_loaded_1[key].resize(EventIndex + 1);
            TS_PPSDiff_loaded_1[key].resize(EventIndex + 1);
            TS_PPSMissing_loaded_1[key].resize(EventIndex + 1);
        }

        // Now using EventIndex to place each event correctly
        Offsets_0[key][EventIndex] = final_offset_ns_0 + TS_driftCorrection_ns_0;
        Offsets_minus_ps_0[key][EventIndex] = static_cast<int>(final_offset_ps_negative_0);
        BGCorrections_0[key][EventIndex] = static_cast<int>(BGCorrection_tick_0) - 1000;
        TSCorrections_0[key][EventIndex] = static_cast<int>(TSCorrection_tick_0) - 1000;
        BG_PPSBefore_loaded_0[key][EventIndex] = BG_PPSBefore_tick_0;
        BG_PPSAfter_loaded_0[key][EventIndex] = BG_PPSAfter_tick_0;
        BG_PPSDiff_loaded_0[key][EventIndex] = BG_PPSDiff_tick_0;
        BG_PPSMissing_loaded_0[key][EventIndex] = static_cast<int>(BG_PPSMissing_tick_0) - 1000;
        TS_PPSBefore_loaded_0[key][EventIndex] = TS_PPSBefore_tick_0;
        TS_PPSAfter_loaded_0[key][EventIndex] = TS_PPSAfter_tick_0;
        TS_PPSDiff_loaded_0[key][EventIndex] = TS_PPSDiff_tick_0;
        TS_PPSMissing_loaded_0[key][EventIndex] = static_cast<int>(TS_PPSMissing_tick_0) - 1000;

        Offsets_1[key][EventIndex] = final_offset_ns_1 + TS_driftCorrection_ns_1;
        Offsets_minus_ps_1[key][EventIndex] = static_cast<int>(final_offset_ps_negative_1);
        BGCorrections_1[key][EventIndex] = static_cast<int>(BGCorrection_tick_1) - 1000;
        TSCorrections_1[key][EventIndex] = static_cast<int>(TSCorrection_tick_1) - 1000;
        BG_PPSBefore_loaded_1[key][EventIndex] = BG_PPSBefore_tick_1;
        BG_PPSAfter_loaded_1[key][EventIndex] = BG_PPSAfter_tick_1;
        BG_PPSDiff_loaded_1[key][EventIndex] = BG_PPSDiff_tick_1;
        BG_PPSMissing_loaded_1[key][EventIndex] = static_cast<int>(BG_PPSMissing_tick_1) - 1000;        
        TS_PPSBefore_loaded_1[key][EventIndex] = TS_PPSBefore_tick_1;
        TS_PPSAfter_loaded_1[key][EventIndex] = TS_PPSAfter_tick_1;
        TS_PPSDiff_loaded_1[key][EventIndex] = TS_PPSDiff_tick_1;
        TS_PPSMissing_loaded_1[key][EventIndex] = static_cast<int>(TS_PPSMissing_tick_1) - 1000;

        if (nentries > 10 && i % (static_cast<int>(nentries / 10)) == 0)
        {
            std::cout << "LAPPDLoadStore::LoadOffsetsAndCorrections, " << i << " entries loaded" << std::endl;
        }
    }

    file->Close();
    delete file;

    // The data structures are now correctly filled and can be used as needed.
}

void LAPPDLoadStore::LoadRunInfo()
{
    if (LAPPDLoadStoreVerbosity > 0)
        cout << "LAPPDLoadStore, Loading run info" << endl;
    int PFNumberBeforeGet = partFileNumber;
    m_data->CStore.Get("rawFileNumber", partFileNumber);
    m_data->CStore.Get("runNumber", runNumber);
    m_data->CStore.Get("subrunNumber", subRunNumber);

    if (partFileNumber != PFNumberBeforeGet)
    {
        eventNumberInPF = 0;
        // also set all value of LAPPDEventIndex_ID to be 0
        for (int i = 0; i < LAPPDEventIndex_ID.size(); i++)
        {
            LAPPDEventIndex_ID[i] = 0;
        }
    }
    else
    {
        eventNumberInPF++;
    }
    if (LAPPDLoadStoreVerbosity > 0)
        cout << "LAPPDLoadStore, Loaded run info, runNumber: " << runNumber << ", subRunNumber: " << subRunNumber << ", partFileNumber: " << partFileNumber << ", eventNumberInPF: " << eventNumberInPF << endl;

    if (runNumber<1) {
        if (LAPPDLoadStoreVerbosity > 1)
            cout << "LAPPDLoadStore, runNumber is "<< runNumber << ", trying to get from ANNIEEvent Store" << endl;
       
       	m_data->Stores["ANNIEEvent"]->Get("RunNumber", runNumber);
        m_data->Stores["ANNIEEvent"]->Get("SubRunNumber", subRunNumber);
        m_data->Stores["ANNIEEvent"]->Get("PartNumber", partFileNumber);
       
       	if (LAPPDLoadStoreVerbosity > 1)
            cout << "LAPPDLoadStore, Got run info from ANNIEEvent Store, runNumber: " << runNumber << ", subRunNumber: " << subRunNumber << ", partFileNumber: " << partFileNumber << endl;
    }
}

vector<IDConfigRecord> LAPPDLoadStore::LoadIDConfig(const string& filename) {
    vector<IDConfigRecord> data;
    ifstream file(filename);
    if (!file.is_open()) {
        cerr << "LAPPDLoadStore::LoadIDConfig: Can't open file: " << filename << endl;
        return data;
    }

    string line;
    getline(file, line); // skip header

    while (getline(file, line)) {
        if (line.empty()) continue;
        replace(line.begin(), line.end(), '\t', ','); // support tab or comma
        stringstream ss(line);
        string token;
        IDConfigRecord r;
        
	getline(ss, token, ','); r.RunNumber = stoi(token);
	getline(ss, token, ','); r.ACCID = stoi(token);
        getline(ss, token, ','); r.ManufacturerID = stoi(token);
        getline(ss, token, ','); r.Position = token;
        
	data.push_back(r);
    }

    return data;
}

tuple<int, string> LAPPDLoadStore::queryNearestACCID(const vector<IDConfigRecord>& data, int targetRun, int manufacturerID) {
    int bestRun = -1;
    int bestACCID = -1;
    string bestPosition;

    for (const auto& r : data) {
        if (r.ManufacturerID == manufacturerID && r.RunNumber <= targetRun) {
            if (r.RunNumber > bestRun) { // find the nearest run not exceeding targetRun
                bestRun = r.RunNumber;
                bestACCID = r.ACCID;
                bestPosition = r.Position;
            }
        }
    }

    if (bestRun == -1)
        return {-1, ""};
    return {bestACCID, bestPosition};
}
