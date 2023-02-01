#include "LAPPDDataDecoder.h"

LAPPDDataDecoder::LAPPDDataDecoder():Tool(){}


bool LAPPDDataDecoder::Initialise(std::string configfile, DataModel &data){

  /////////////////// Useful header ///////////////////////
  if(configfile!="") m_variables.Initialise(configfile); // loading config file
  //m_variables.Print();

  m_data= &data; //assigning transient data pointer
  /////////////////////////////////////////////////////////////////

  verbosity = 0;
  EntriesPerExecute = 0;

  m_variables.Get("verbosity",verbosity);
  m_variables.Get("EntriesPerExecute",EntriesPerExecute);

  LAPPDDEntryNum = 0;

  CurrentRunNum = -1;
  CurrentSubrunNum = -1;

  m_data->CStore.Set("PauseLAPPDDecoding",false);

  LAPPDPPS = new std::vector<uint64_t>;
  LAPPDTimestamps = new std::vector<uint64_t>;
  LAPPDBeamgateTimestamps = new std::vector<uint64_t>;
  LAPPDPulses = new std::vector<PsecData>;

  std::cout <<"LAPPDDataDecoder Tool: Initialized successfully" << std::endl;

  return true;
}


bool LAPPDDataDecoder::Execute(){

  LAPPDDataBuilt = false;
  Log("LAPPDDataDecoderTool: Executing",v_debug,verbosity);
  //Set in CStore that there's currently no new LAPPD data avaialble
  m_data->CStore.Set("NewLAPPDDataAvailable",false);

  //Check if there's a problem in the LoadRawData tool

  bool NewEntryAvailable;
  m_data->CStore.Get("NewRawDataEntryAccessed",NewEntryAvailable);
  if(!NewEntryAvailable){ //Something went wrong processing raw data.  Stop and save what's left
    Log("LAPPDDataDecoder Tool: There's no new PMT data.  Things would crash if we continue.  Stopping at next loop to save what data has been built.",v_warning,verbosity);
    m_data->vars.Set("StopLoop",1);
    return true;
  }

  //Check if the LAPPD decoding was paused
  bool PauseLAPPDDecoding = false;
  m_data->CStore.Get("PauseLAPPDDecoding",PauseLAPPDDecoding);
  if (PauseLAPPDDecoding){
    std::cout << "LAPPDDataDecoder tool: Pausing LAPPD decoding to let other data catch up..." << std::endl;
    return true;
  }

  // Load Run Information to see if a new run or subrun has been encountered 
 Store RunInfoPostgress;
    m_data->CStore.Get("RunInfoPostgress",RunInfoPostgress);
    int RunNumber;
    int SubRunNumber;
    uint64_t StarTime;
    int RunType;
    RunInfoPostgress.Get("RunNumber",RunNumber);
    RunInfoPostgress.Get("SubRunNumber",SubRunNumber);
    RunInfoPostgress.Get("RunType",RunType);
    RunInfoPostgress.Get("StarTime",StarTime);

    if (CurrentRunNum == -1){ //Initializing current run number and subrun number
CurrentRunNum = RunNumber;
      CurrentSubrunNum = SubRunNumber;
    }
    else if (RunNumber != CurrentRunNum){ //New run has been encountered
      Log("LAPPDDataDecoder Tool: New run encountered.  Clearing event building maps",v_message,verbosity);
      CurrentRunNum = RunNumber;
    }
    else if (SubRunNumber != CurrentSubrunNum){ //New subrun has been encountered
      Log("LAPPDDataDecoder Tool: New subrun encountered.",v_message,verbosity);
      CurrentSubrunNum = SubRunNumber;
    }

  bool NewRawDataFile = false;
  m_data->CStore.Get("NewRawDataFileAccessed",NewRawDataFile);
  if(NewRawDataFile){
    //Do we need to clear something here?
  }

  Log("LAPPDDataDecoder Tool: Procesing LAPPDData Entry from CStore",v_debug, verbosity);
  m_data->CStore.Get("LAPPDData",Ldata);

  std::vector<unsigned short> Raw_Buffer = Ldata->RawWaveform;
  std::vector<int> BoardId_Buffer = Ldata->BoardIndex;

  if (Raw_Buffer.size() == 0) {
    std::cout <<"LAPPDDataDecoder: Encountered Raw Buffer size of 0! Abort!"<<std::endl;
    return true;
  }

  if (verbosity > 2) std::cout <<"LAPPDDataDecoder: Raw_Buffer.size(): "<<Raw_Buffer.size()<<", BoardId_Buffer.size(): "<<BoardId_Buffer.size()<<std::endl;
  int frametype = Raw_Buffer.size()/BoardId_Buffer.size();
  if (verbosity > 2) std::cout <<"LAPPDDataDecoder: Got frametype = "<<frametype<<std::endl;

  if(frametype!=NUM_VECTOR_DATA && frametype!=NUM_VECTOR_PPS)
  {   
            cout << "Problem identifying the frametype, size of raw vector was " << Raw_Buffer.size() << endl;
            cout << "It was expected to be either " << NUM_VECTOR_DATA*BoardId_Buffer.size() << " or " <<  NUM_VECTOR_PPS*BoardId_Buffer.size() << endl;
            cout << "Please check manually!" << endl;
            return false;
  }

  if(frametype==NUM_VECTOR_PPS)
  {
    if (verbosity > 2) std::cout <<"LAPPDDataDecoder: Encountered PPS frame"<<std::endl;
    pps = Raw_Buffer;
    LoopThroughPPSData();
    LAPPDDataBuilt = true;
  } else {
    if (verbosity > 2) std::cout <<"LAPPDDataDecoder: Encountered data frame"<<std::endl;
    
    //Create a vector of paraphrased board indices
    int nbi = BoardId_Buffer.size();
    vector<int> ParaBoards;
    if(nbi%2!=0)
    {
      cout << "Why is there an uneven number of boards! this is wrong!" << endl;
      if(nbi==1)
      { 
        ParaBoards.push_back(1);
      } else
      { 
        return false;
      }
    } else
    { 
      for(int cbi=0; cbi<nbi; cbi++)
      { 
        ParaBoards.push_back(cbi);
      }
    }

    for(int bi: ParaBoards)
    {
        Parse_Buffer.clear();
        if(verbosity>1) std::cout << "Starting with board " << BoardId_Buffer[bi] << std::endl;
        //Go over all ACDC board data frames by seperating them
        for(int c=bi*frametype; c<(bi+1)*frametype; c++)
        {   
            Parse_Buffer.push_back(Raw_Buffer[c]);
        }
        if(verbosity>1) std::cout << "Data for board " << BoardId_Buffer[bi] << " was grabbed!" << std::endl;

        //Meta data parsing

        int retval = getParsedMeta(Parse_Buffer,BoardId_Buffer[bi]);
        if(retval!=0)
        {   
          std::cout << "Meta parsing went wrong! " << retval << endl;
        } else
        {   
          if(verbosity>1) std::cout << "Meta for board " << BoardId_Buffer[bi] << " was parsed!" << std::endl;
        }
    }

    LoopThroughMetaData();
    meta.clear();

    LAPPDPulses->push_back(*Ldata);
    LAPPDDataBuilt = true;
  }

  if (verbosity > 1) std::cout <<"LAPPDDataBuilt: "<<LAPPDDataBuilt<<std::endl;

  m_data->CStore.Set("InProgressLAPPDEvents",LAPPDPulses);
  m_data->CStore.Set("InProgressLAPPDPPS",LAPPDPPS);
  m_data->CStore.Set("InProgressLAPPDTimestamps",LAPPDTimestamps);
  m_data->CStore.Set("InProgressLAPPDBeamgate",LAPPDBeamgateTimestamps);
  m_data->CStore.Set("NewLAPPDDataAvailable",LAPPDDataBuilt);

  Log("LAPPDDataDecoder Tool: Size of LAPPDPulses from this execution (# triggers with at least one wave fully):" +
            to_string(LAPPDPulses->size()),v_message, verbosity);

  return true;
}


bool LAPPDDataDecoder::Finalise(){

  Log("LAPPDDataDecoder tool exitting",v_warning,verbosity);

  return true;
}

int LAPPDDataDecoder::getParsedMeta(std::vector<unsigned short> buffer, int BoardId)
{
    //Catch empty buffers
    if(buffer.size() == 0)
    {   
        std::cout << "You tried to parse ACDC data without pulling/setting an ACDC buffer" << std::endl; 
        return -1;
    }

    //Prepare the Metadata vector 
    //meta.clear();

    //Helpers
    int chip_count = 0;

    //Indicator words for the start/end of the metadata
    const unsigned short startword = 0xBA11;
    unsigned short endword = 0xFACE;
    unsigned short endoffile = 0x4321;

    //Empty metadata map for each Psec chip <PSEC #, vector with information>
    map<int, vector<unsigned short>> PsecInfo;

    //Empty trigger metadata map for each Psec chip <PSEC #, vector with trigger>
    map<int, vector<unsigned short>> PsecTriggerInfo;
    unsigned short CombinedTriggerRateCount;

    //Empty vector with positions of aboves startword
    vector<int> start_indices=
    {   
        1539, 3091, 4643, 6195, 7747
    };

    //Fill the psec info map
    vector<unsigned short>::iterator bit;
    for(int i: start_indices)
    {   
        //Write the first word after the startword
        bit = buffer.begin() + (i+1);
        
        //As long as the endword isn't reached copy metadata words into a vector and add to map
        vector<unsigned short> InfoWord; 
        while(*bit != endword && *bit != endoffile && InfoWord.size() < 14)
{
            InfoWord.push_back(*bit);
            ++bit;
        }
        PsecInfo.insert(pair<int, vector<unsigned short>>(chip_count, InfoWord));
        chip_count++;
    }

    //Fill the psec trigger info map
    for(int chip=0; chip<NUM_PSEC; chip++)
    {
        for(int ch=0; ch<NUM_CH/NUM_PSEC; ch++)
        {
            //Find the trigger data at begin + last_metadata_start + 13_info_words + 1_end_word + 1 
            bit = buffer.begin() + start_indices[4] + 13 + 1 + 1 + ch + (chip*(NUM_CH/NUM_PSEC));
            PsecTriggerInfo[chip].push_back(*bit);
        }
    }

    //Fill the combined trigger
    CombinedTriggerRateCount = buffer[7792];

    //----------------------------------------------------------
    //Start the metadata parsing 

    if (verbosity > 2) std::cout <<"meta push back board id "<<BoardId<<std::endl;
    meta.push_back(BoardId);
    for(int CHIP=0; CHIP<NUM_PSEC; CHIP++)
    {
        meta.push_back((0xDCB0 | CHIP));
        for(int INFOWORD=0; INFOWORD<13; INFOWORD++)
        {
            meta.push_back(PsecInfo[CHIP][INFOWORD]);
        }
        for(int TRIGGERWORD=0; TRIGGERWORD<6; TRIGGERWORD++)
        {
            meta.push_back(PsecTriggerInfo[CHIP][TRIGGERWORD]);
        }
    }
    meta.push_back(CombinedTriggerRateCount);
    meta.push_back(0xeeee);
    return 0;
}

int LAPPDDataDecoder::LoopThroughMetaData(){
  
  int offset = 0;
  int board_index = meta.at(0);
  if (board_index == 56496){
    std::cout <<"LAPPDDataDecoder: Board index missing from meta data. Abort"<<std::endl;
    return 1;
  }  

  //Beamgate timestamp
  unsigned short beamgate_63_48 = meta.at(7);
  unsigned short beamgate_47_32 = meta.at(27);
  unsigned short beamgate_31_16 = meta.at(47);
  unsigned short beamgate_15_0 = meta.at(67);
 
  std::bitset < 16 > bits_beamgate_63_48(beamgate_63_48);
  std::bitset < 16 > bits_beamgate_47_32(beamgate_47_32);
  std::bitset < 16 > bits_beamgate_31_16(beamgate_31_16);
  std::bitset < 16 > bits_beamgate_15_0(beamgate_15_0);
  unsigned long beamgate_63_0 = (static_cast<unsigned long>(beamgate_63_48) << 48) + (static_cast<unsigned long>(beamgate_47_32) << 32) + (static_cast<unsigned long>(beamgate_31_16) << 16) + (static_cast<unsigned long>(beamgate_15_0));
  std::bitset < 64 > bits_beamgate_63_0(beamgate_63_0);
  unsigned long beamgate_timestamp = beamgate_63_0 * (CLOCK_to_NSEC);
  LAPPDBeamgateTimestamps->push_back(beamgate_timestamp);

  //Data timestamp
  unsigned short timestamp_63_48 = meta.at(70);
  unsigned short timestamp_47_32 = meta.at(50);
  unsigned short timestamp_31_16 = meta.at(30);
  unsigned short timestamp_15_0 = meta.at(10);

  std::bitset < 16 > bits_timestamp_63_48(timestamp_63_48);
  std::bitset < 16 > bits_timestamp_47_32(timestamp_47_32);
  std::bitset < 16 > bits_timestamp_31_16(timestamp_31_16);
  std::bitset < 16 > bits_timestamp_15_0(timestamp_15_0);
  unsigned long timestamp_63_0 = (static_cast<unsigned long>(timestamp_63_48) << 48) + (static_cast<unsigned long>(timestamp_47_32) << 32) + (static_cast<unsigned long>(timestamp_31_16) << 16) + (static_cast<unsigned long>(timestamp_15_0));
  unsigned long lappd_timestamp = timestamp_63_0 * (CLOCK_to_NSEC);

  LAPPDTimestamps->push_back(lappd_timestamp);

  return 0; 
}

int LAPPDDataDecoder::LoopThroughPPSData(){
  if (pps.size()==16){
    unsigned short pps_63_48 = pps.at(2);
    unsigned short pps_47_32 = pps.at(3);
    unsigned short pps_31_16 = pps.at(4);
    unsigned short pps_15_0 = pps.at(5);
    std::bitset < 16 > bits_pps_63_48(pps_63_48);
    std::bitset < 16 > bits_pps_47_32(pps_47_32);
    std::bitset < 16 > bits_pps_31_16(pps_31_16);
    std::bitset < 16 > bits_pps_15_0(pps_15_0);
    unsigned long pps_63_0 = (static_cast<unsigned long>(pps_63_48) << 48) + (static_cast<unsigned long>(pps_47_32) << 32) + (static_cast<unsigned long>(pps_31_16) << 16) + (static_cast<unsigned long>(pps_15_0));
    if (verbosity > 2) std::cout << "pps combined: " << pps_63_0 << std::endl;
    std::bitset < 64 > bits_pps_63_0(pps_63_0);
    unsigned long pps_timestamp = pps_63_0 * (CLOCK_to_NSEC);
    LAPPDPPS->push_back(pps_timestamp);
    if (verbosity > 2) std::cout <<"Adding timestamp "<<pps_timestamp<<" to LAPPDPPS"<<std::endl;
  } else if (pps.size() == 32){
    unsigned short pps_63_48 = pps.at(2);
    unsigned short pps_47_32 = pps.at(3);
    unsigned short pps_31_16 = pps.at(4);
    unsigned short pps_15_0 = pps.at(5);
    std::bitset < 16 > bits_pps_63_48(pps_63_48);
    std::bitset < 16 > bits_pps_47_32(pps_47_32);
    std::bitset < 16 > bits_pps_31_16(pps_31_16);
    std::bitset < 16 > bits_pps_15_0(pps_15_0);
    unsigned long pps_63_0 = (static_cast<unsigned long>(pps_63_48) << 48) + (static_cast<unsigned long>(pps_47_32) << 32) + (static_cast<unsigned long>(pps_31_16) << 16) + (static_cast<unsigned long>(pps_15_0));
    if (verbosity > 2) std::cout << "pps combined: " << pps_63_0 << std::endl;
    std::bitset < 64 > bits_pps_63_0(pps_63_0);
    unsigned long pps_timestamp = pps_63_0 * (CLOCK_to_NSEC);
    LAPPDPPS->push_back(pps_timestamp);
    if (verbosity > 2) std::cout <<"Adding timestamp "<<pps_timestamp<<" to LAPPDPPS"<<std::endl;
    pps_63_48 = pps.at(18);
    pps_47_32 = pps.at(19);
    pps_31_16 = pps.at(20);
    pps_15_0 = pps.at(21);
    std::bitset < 16 > Bits_pps_63_48(pps_63_48);
    std::bitset < 16 > Bits_pps_47_32(pps_47_32);
    std::bitset < 16 > Bits_pps_31_16(pps_31_16);
    std::bitset < 16 > Bits_pps_15_0(pps_15_0);
    pps_63_0 = (static_cast<unsigned long>(pps_63_48) << 48) + (static_cast<unsigned long>(pps_47_32) << 32) + (static_cast<unsigned long>(pps_31_16) << 16) + (static_cast<unsigned long>(pps_15_0));
    if (verbosity > 2) std::cout << "pps combined: " << pps_63_0 << std::endl;
    std::bitset < 64 > Bits_pps_63_0(pps_63_0);
    unsigned long pps_timestamp_2 = pps_63_0 * (CLOCK_to_NSEC);
  } else {
    //Some error because the size of the pps data frame does not match 16 or 32
    std::cout <<"LAPPDDataDecoder: PPS vector does not have size 16 or 32! Abort"<<std::endl;
    std::cout <<"Size: "<<pps.size()<<std::endl;
    for (int i_pps=0; i_pps < pps.size(); i_pps++){std::cout << pps.at(i_pps)<<std::endl;}
    return 1;
  }
  return 0;
}
