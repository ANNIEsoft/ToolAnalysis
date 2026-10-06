#ifndef LAPPDLoadStore_H
#define LAPPDLoadStore_H

#include <string>
#include <iostream>
#include <map>
#include <bitset>
#include <fstream>
#include "Tool.h"
#include "PsecData.h"
#include "TFile.h"
#include "TTree.h"

#define NUM_CH 30
#define NUM_PSEC 5
#define NUM_SAMP 256

using namespace std;

// class LAPPDLoadStore
// Load LAPPD PSEC data and PPS data from BoostStore.

class LAPPDLoadStore : public Tool
{

public:
    LAPPDLoadStore();                                         ///< Simple constructor
    bool Initialise(std::string configfile, DataModel &data); ///< Initialise Function for setting up Tool resources. 
    bool Execute();                                           ///< Execute function used to perform Tool purpose.
    bool Finalise();                                          ///< Finalise function used to clean up resources.
    
    bool ReadPedestals(int boardNo);                          ///< Read in the Pedestal Files
    bool MakePedestals();                                     ///< Make a Pedestal File

    int getParsedData(vector<unsigned short> buffer, int ch_start);
    int getParsedMeta(vector<unsigned short> buffer, int BoardId);

    void CleanDataObjects();
    bool LoadData();
    bool ParsePSECData();
    void ParsePPSData();
    bool DoPedestalSubtract();
    void SaveTimeStamps(); // save some timestamps relate to this event
    void LoadOffsetsAndCorrections();
    void LoadRunInfo();
    void SaveOffsets();

    vector<IDConfigRecord> LoadIDConfig(const string& filename);
    tuple<int, string> queryNearestACCID(const vector<IDConfigRecord>& data, int targetRun, int manufacturerID);

private:
    static constexpr double CLOCK_TO_NSEC = 3.125;
    static constexpr int ACDC_META_WORDS = 103;

    // Tool Control Variables (Loaded from config file)
    int ReadStore;         	// read data from a StoreFile (tool chain start with this tool rather than LoadANNIEEvent)
    int Nboards;           	// total number of boards to load pedestal file, LAPPD count * 2
    string PedFileNameTXT; 	// txt format pedestal file name
    int DoPedSubtract;     	// 1: do pedestal subtraction, 0: don't do pedestal subtraction
    int LAPPDLoadStoreVerbosity;
    int num_vector_data;
    int num_vector_pps;
    bool SelectSingleLAPPD;
    int SelectedLAPPD;
    bool mergingModeReadIn;
    bool ReadStorePdeFile;
    bool MultiLAPPDMap; 	// loading map of multiple LAPPDs from ANNIEEvent
    bool loadOffsets;
    bool LoadBuiltPPSInfo;
    bool loadFromStoreDirectly;

    // Internal Tool State Variables
    int retval; 		// track the data parsing and meta parsing status
    int eventNo;
    int errorEventsNumber; 
    int stopEntries;        	// stop tool chain after loading this number of PSEC data events
    int NonEmptyEvents;     	// count how many non empty data events were loaded
    int NonEmptyDataEvents; 	// count how many non empty data events were loaded
    bool runInfoLoaded;

    // LAPPD Tool Chain Variables (Shared across multiple LAPPD tools)
    // Variables that you get from the config file
    bool PsecReceiveMode;   	// Get PSEC data from CStore or Stores["ANNIEEvent"]. 1: CStore, 0: Stores["ANNIEEvent"]
    string OutputWavLabel;
    string InputWavLabel;
    int NChannels;
    int Nsamples;
    int TrigChannel;
    double SampleSize;
    int LAPPDchannelOffset;
    int PPSnumber;
    bool loadPPS;
    bool loadPSEC;
    bool mergedEvent;		// in some merged Event, the LAPPD events was merged to ANNIEEvent, but the data stream was not changed to be true. use this = 1 to read it
    bool LAPPDana;    		// run other LAPPD tools
    
    bool isCFD;
    bool isBLsub;
    bool isFiltered;
    int EventType; 		// 0: PSEC, 1: PPS
    
    // Store Data Variables (Written to / Read from BoostStore)
    int LAPPD_ID;
    vector<int> LAPPD_IDs;
    vector<uint64_t> LAPPDLoadedTimeStamps;
    
    vector<uint64_t> LAPPDLoadedTimeStampsRaw_0;
    vector<uint64_t> LAPPDLoadedBeamgatesRaw_0;
    vector<uint64_t> LAPPDLoadedOffsets_0;
    vector<int> LAPPDLoadedTSCorrections_0;
    vector<int> LAPPDLoadedBGCorrections_0;
    vector<int> LAPPDLoadedOSInMinusPS_0;
    vector<uint64_t> LAPPDLoadedBG_PPSBefore_0;
    vector<uint64_t> LAPPDLoadedBG_PPSAfter_0;
    vector<uint64_t> LAPPDLoadedBG_PPSDiff_0;
    vector<int> LAPPDLoadedBG_PPSMissing_0;
    vector<uint64_t> LAPPDLoadedTS_PPSBefore_0;
    vector<uint64_t> LAPPDLoadedTS_PPSAfter_0;
    vector<uint64_t> LAPPDLoadedTS_PPSDiff_0;
    vector<int> LAPPDLoadedTS_PPSMissing_0;

    vector<uint64_t> LAPPDLoadedTimeStampsRaw_1;
    vector<uint64_t> LAPPDLoadedBeamgatesRaw_1;
    vector<uint64_t> LAPPDLoadedOffsets_1;
    vector<int> LAPPDLoadedTSCorrections_1;
    vector<int> LAPPDLoadedBGCorrections_1;
    vector<int> LAPPDLoadedOSInMinusPS_1;
    vector<uint64_t> LAPPDLoadedBG_PPSBefore_1;
    vector<uint64_t> LAPPDLoadedBG_PPSAfter_1;
    vector<uint64_t> LAPPDLoadedBG_PPSDiff_1;
    vector<int> LAPPDLoadedBG_PPSMissing_1;
    vector<uint64_t> LAPPDLoadedTS_PPSBefore_1;
    vector<uint64_t> LAPPDLoadedTS_PPSAfter_1;
    vector<uint64_t> LAPPDLoadedTS_PPSDiff_1;
    vector<int> LAPPDLoadedTS_PPSMissing_1;

    vector<int> ParaBoards; 	// save the board index for this PsecData
    std::map<unsigned long, vector<Waveform<double>>> LAPPDWaveforms;

    ifstream PedFile;   	// stream for reading in the Pedestal Files
    std::map<unsigned long, vector<int>> *PedestalValues;
    std::vector<unsigned short> Raw_buffer;
    std::vector<unsigned short> Parse_buffer;
    std::vector<int> ReadBoards;
    std::map<int, vector<unsigned short>> data;
    vector<unsigned short> meta;
    vector<unsigned short> pps;
    
    vector<int> LAPPD_ID_Channel;     // for each LAPPD, how many channels on it's each board
    vector<int> LAPPD_ID_BoardNumber; // for each LAPPD, how many boards with it
    
    std::map<uint64_t, PsecData> LAPPDDataMap;
    std::map<std::string, bool> DataStreams;
    
    std::map<uint64_t, uint64_t> LAPPDBeamgate_ns_0;
    std::map<uint64_t, uint64_t> LAPPDTimeStamps_ns_0;
    std::map<uint64_t, uint64_t> LAPPDTimeStampsRaw_0;
    std::map<uint64_t, uint64_t> LAPPDBeamgatesRaw_0;
    std::map<uint64_t, uint64_t> LAPPDOffsets_0;
    std::map<uint64_t, int> LAPPDTSCorrection_0;
    std::map<uint64_t, int> LAPPDBGCorrection_0;
    std::map<uint64_t, int> LAPPDOSInMinusPS_0;

    std::map<uint64_t, uint64_t> LAPPDBeamgate_ns_1;
    std::map<uint64_t, uint64_t> LAPPDTimeStamps_ns_1;
    std::map<uint64_t, uint64_t> LAPPDTimeStampsRaw_1;
    std::map<uint64_t, uint64_t> LAPPDBeamgatesRaw_1;
    std::map<uint64_t, uint64_t> LAPPDOffsets_1;
    std::map<uint64_t, int> LAPPDTSCorrection_1;
    std::map<uint64_t, int> LAPPDBGCorrection_1;
    std::map<uint64_t, int> LAPPDOSInMinusPS_1;
    
    // For each LAPPD ID, count the index of current loaded event.
    // (e.g. part file has 3 ID=0 events and 4 ID=1 events -> LAPPDEventIndex_ID = {3, 4})
    std::vector<int> LAPPDEventIndex_ID;
    
    vector<IDConfigRecord> idConfigRecords;

    // save PPS info for the second order correction
    // save PPS info for the second order correction
    std::map<uint64_t, uint64_t> LAPPDBG_PPSBefore_0;
    std::map<uint64_t, uint64_t> LAPPDBG_PPSAfter_0;
    std::map<uint64_t, uint64_t> LAPPDBG_PPSDiff_0;
    std::map<uint64_t, int> LAPPDBG_PPSMissing_0;
    std::map<uint64_t, uint64_t> LAPPDTS_PPSBefore_0;
    std::map<uint64_t, uint64_t> LAPPDTS_PPSAfter_0;
    std::map<uint64_t, uint64_t> LAPPDTS_PPSDiff_0;
    std::map<uint64_t, int> LAPPDTS_PPSMissing_0;

    std::map<uint64_t, uint64_t> LAPPDBG_PPSBefore_1;
    std::map<uint64_t, uint64_t> LAPPDBG_PPSAfter_1;
    std::map<uint64_t, uint64_t> LAPPDBG_PPSDiff_1;
    std::map<uint64_t, int> LAPPDBG_PPSMissing_1;
    std::map<uint64_t, uint64_t> LAPPDTS_PPSBefore_1;
    std::map<uint64_t, uint64_t> LAPPDTS_PPSAfter_1;
    std::map<uint64_t, uint64_t> LAPPDTS_PPSDiff_1;
    std::map<uint64_t, int> LAPPDTS_PPSMissing_1;

    // data variables don't need to be cleared in each loop
    // these are loaded offset for event building
    std::map<string, vector<uint64_t>> Offsets_0;
    std::map<string, vector<int>> Offsets_minus_ps_0;
    std::map<string, vector<int>> BGCorrections_0;
    std::map<string, vector<int>> TSCorrections_0;
    std::map<string, vector<uint64_t>> BG_PPSBefore_loaded_0;
    std::map<string, vector<uint64_t>> BG_PPSAfter_loaded_0;
    std::map<string, vector<uint64_t>> BG_PPSDiff_loaded_0;
    std::map<string, vector<int>> BG_PPSMissing_loaded_0;
    std::map<string, vector<uint64_t>> TS_PPSBefore_loaded_0;
    std::map<string, vector<uint64_t>> TS_PPSAfter_loaded_0;
    std::map<string, vector<uint64_t>> TS_PPSDiff_loaded_0;
    std::map<string, vector<int>> TS_PPSMissing_loaded_0;

    std::map<string, vector<uint64_t>> Offsets_1;
    std::map<string, vector<int>> Offsets_minus_ps_1;
    std::map<string, vector<int>> BGCorrections_1;
    std::map<string, vector<int>> TSCorrections_1;
    std::map<string, vector<uint64_t>> BG_PPSBefore_loaded_1;
    std::map<string, vector<uint64_t>> BG_PPSAfter_loaded_1;
    std::map<string, vector<uint64_t>> BG_PPSDiff_loaded_1;
    std::map<string, vector<int>> BG_PPSMissing_loaded_1;
    std::map<string, vector<uint64_t>> TS_PPSBefore_loaded_1;
    std::map<string, vector<uint64_t>> TS_PPSAfter_loaded_1;
    std::map<string, vector<uint64_t>> TS_PPSDiff_loaded_1;
    std::map<string, vector<int>> TS_PPSMissing_loaded_1;

    int runNumber;
    int subRunNumber;
    int partFileNumber;
    int eventNumberInPF;
    std::ofstream debugLoadStore;

    // verbosity levels: if 'verbosity' < this level, the message type will be logged.
    int v_error=0;
    int v_warning=1;
    int v_message=2;
    int v_debug=3;
    std::string logmessage;
    int get_ok;
};

#endif
