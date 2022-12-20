#include "parseLAPPDData.h"

parseLAPPDData::parseLAPPDData():Tool(){}


bool parseLAPPDData::Initialise(std::string configfile, DataModel &data){

  /////////////////// Useful header ///////////////////////
  if(configfile!="") m_variables.Initialise(configfile); // loading config file
  //m_variables.Print();

  m_data= &data; //assigning transient data pointer
  /////////////////////////////////////////////////////////////////

  return true;
}


bool parseLAPPDData::Execute(){

  int boardindex, retval;

  bool has_lappd = false;
  m_data->CStore.Get("LAPPD_HasData",has_lappd);

  if (has_lappd){ 
    uint64_t time_since_beamgate;
    bool in_beam_window;

    PsecData psec;
    m_data->CStore.Get("LAPPD_Psec",psec);

    std::vector<unsigned short> Raw_Buffer = psec.RawWaveform;
    std::vector<int> BoardId_Buffer = psec.BoardIndex;
    
    if (Raw_Buffer.size() == 0) {
      std::cout <<"Encountered Raw Buffer size of 0! Abort!"<<std::endl;
      return true;
    }

    if (verbosity > 2) std::cout <<"ParseDataMonitoring: Raw_Buffer.size(): "<<Raw_Buffer.size()<<", BoardId_Buffer.size(): "<<BoardId_Buffer.size()<<std::endl;
    int frametype = Raw_Buffer.size()/BoardId_Buffer.size();
    if (verbosity > 2) std::cout <<"ParseDataMonitoring: Got frametype = "<<frametype<<std::endl;

    if(frametype!=NUM_VECTOR_DATA && frametype!=NUM_VECTOR_PPS)
    {
      cout << "Problem identifying the frametype, size of raw vector was " << Raw_Buffer.size() << endl;
      cout << "It was expected to be either " << NUM_VECTOR_DATA*BoardId_Buffer.size() << " or " <<  NUM_VECTOR_PPS*BoardId_Buffer.size() << endl;
      cout << "Please check manually!" << endl;
      return false;
    }

    if (verbosity > 2) std::cout <<"ParseDataMonitoring: frametype: "<<frametype<<", NUM_VECTOR_PPS: "<<NUM_VECTOR_PPS<<", NUM_VECTOR_DATA: "<<NUM_VECTOR_DATA<<std::endl;

    if(frametype==NUM_VECTOR_PPS)
    {
      if (verbosity > 2) std::cout <<"PPS frame, skip"<<std::endl;
    }
    else  {			//NOT A PPS ENTRY --> DATA ENTRY
    	//Create a vector of paraphrased board indices
        int nbi = BoardId_Buffer.size();
    	vector<int> ParaBoards;
    	if(nbi%2!=0)
    	{  
          cout << "Why is there an uneven number of boards! this is wrong!" << endl;
          if(nbi==1)
          {
            ParaBoards.push_back(1);
          }else
          {
            return false;
          }
    	}else
    	{
          for(int cbi=0; cbi<nbi; cbi++)
          {
            ParaBoards.push_back(cbi);
          }
      }
    data.clear();
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


        //Grab the parsed data and give it to a global variable 'data'
	if (verbosity > 2) std::cout <<"bi: "<<bi<<", BoardId_Buffer[bi: "<<BoardId_Buffer[bi]<<std::endl;
        retval = getParsedData(Parse_Buffer,BoardId_Buffer[bi]*NUM_CH);
        if(retval == 0)
        {
            if(verbosity>1) std::cout << "Data for board " << BoardId_Buffer[bi] << " was parsed!" << std::endl;
            //Grab the parsed metadata and give it to a global variable 'meta'
            retval = getParsedMeta(Parse_Buffer,BoardId_Buffer[bi]);
            if(retval!=0)
            {
                std::cout << "Meta parsing went wrong! " << retval << endl;
            }else
            {
                if(verbosity>1) std::cout << "Meta for board " << BoardId_Buffer[bi] << " was parsed!" << std::endl;
            }
        }else
        {
            std::cout << "Parsing went wrong! " << retval << endl;
            return false;
        }
    } 
 
    //Loop over meta data
    int offset = 0;
    unsigned short beamgate_63_48 = meta.at(7 - offset);	//Shift everything by 1 for the test file
    unsigned short beamgate_47_32 = meta.at(27 - offset);	//Shift everything by 1 for the test file
    unsigned short beamgate_31_16 = meta.at(47 - offset);	//Shift everything by 1 for the test file
    unsigned short beamgate_15_0 = meta.at(67 - offset);	//Shift everything by 1 for the test file
    std::bitset < 16 > bits_beamgate_63_48(beamgate_63_48);
    std::bitset < 16 > bits_beamgate_47_32(beamgate_47_32);
    std::bitset < 16 > bits_beamgate_31_16(beamgate_31_16);
    std::bitset < 16 > bits_beamgate_15_0(beamgate_15_0);
    unsigned long beamgate_63_0 = (static_cast<unsigned long>(beamgate_63_48) << 48) + (static_cast<unsigned long>(beamgate_47_32) << 32) + (static_cast<unsigned long>(beamgate_31_16) << 16) + (static_cast<unsigned long>(beamgate_15_0));
    std::bitset < 64 > bits_beamgate_63_0(beamgate_63_0);
    double beamgate_nsec = beamgate_63_0*CLOCK_to_NSEC;
    //Build data timestamp
    unsigned short timestamp_63_48 = meta.at(70 - offset);	//Shift everything by 1 for the test file
    unsigned short timestamp_47_32 = meta.at(50 - offset);	//Shift everything by 1 for the test file
    unsigned short timestamp_31_16 = meta.at(30 - offset);	//Shift everything by 1 for the test file
    unsigned short timestamp_15_0 = meta.at(10 - offset);		//Shift everything by 1 for the test file
    std::bitset < 16 > bits_timestamp_63_48(timestamp_63_48);
    std::bitset < 16 > bits_timestamp_47_32(timestamp_47_32);
    std::bitset < 16 > bits_timestamp_31_16(timestamp_31_16);
    std::bitset < 16 > bits_timestamp_15_0(timestamp_15_0);
    //std::cout << "bits_timestamp_63_48: " << bits_timestamp_63_48 << std::endl;
    //std::cout << "bits_timestamp_47_32: " << bits_timestamp_47_32 << std::endl;
    //std::cout << "bits_timestamp_31_16: " << bits_timestamp_31_16 << std::endl;
    //std::cout << "bits_timestamp_15_0: " << bits_timestamp_15_0 << std::endl;
    unsigned long timestamp_63_0 = (static_cast<unsigned long>(timestamp_63_48) << 48) + (static_cast<unsigned long>(timestamp_47_32) << 32) + (static_cast<unsigned long>(timestamp_31_16) << 16) + (static_cast<unsigned long>(timestamp_15_0));
    if (verbosity > 2) std::cout << "timestamp combined: " << timestamp_63_0 << std::endl;
    std::bitset < 64 > bits_timestamp_63_0(timestamp_63_0);
    double timestamp_nsec = timestamp_63_0*CLOCK_to_NSEC;

    time_since_beamgate = timestamp_nsec - beamgate_nsec;
    in_beam_window = false;
    if (time_since_beamgate > 7000 && time_since_beamgate < 10000) in_beam_window = true;

    m_data->CStore.Set("LAPPD_TimeSinceBeam",time_since_beamgate);
    m_data->CStore.Set("LAPPD_InBeamWindow",in_beam_window);
    m_data->CStore.Set("LAPPD_ParsedData",data);
    m_data->CStore.Set("LAPPD_ParsedMeta",meta);
    }
  }

  return true;
}


bool parseLAPPDData::Finalise(){

  return true;
}

int parseLAPPDData::getParsedMeta(std::vector<unsigned short> buffer, int BoardId)
{
    //Catch empty buffers
    if(buffer.size() == 0)
    {
        std::cout << "You tried to parse ACDC data without pulling/setting an ACDC buffer" << std::endl;
        return -1;
    }

    //Prepare the meta vector 
    meta.clear();

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

int parseLAPPDData::getParsedData(std::vector<unsigned short> buffer, int ch_start)
{
    //Catch empty buffers
    if(buffer.size() == 0)
    {
        std::cout << "You tried to parse ACDC data without pulling/setting an ACDC buffer" << std::endl;
        return -1;
    }

    //Prepare the meta vector 
    //data.clear();

    //Helpers
    int DistanceFromZero;
    channel_count=0;

    //Indicator words for the start/end of the metadata
    const unsigned short startword = 0xF005; 
    unsigned short endword = 0xBA11;
    unsigned short endoffile = 0x4321;

    //Empty vector with positions of aboves startword
    vector<int> start_indices=
    {
        2, 1554, 3106, 4658, 6210
    }; 

    //Fill data map
    vector<unsigned short>::iterator bit;
    for(int i: start_indices)
    {
        //Write the first word after the startword
        bit = buffer.begin() + (i+1);

        //As long as the endword isn't reached copy metadata words into a vector and add to map
        vector<unsigned short> InfoWord;
        while(*bit != endword && *bit != endoffile)
        {
            InfoWord.push_back((unsigned short)*bit);
            if(InfoWord.size()==NUM_SAMP)
            {
		if (verbosity > 4) std::cout << "ch_start: "<<ch_start<<", channel_count: "<<channel_count<<std::endl;
                data.insert(pair<int, vector<unsigned short>>(ch_start + channel_count, InfoWord));
                InfoWord.clear();
                channel_count++;
            }
            ++bit;
        } 
    }

    return 0;
}
