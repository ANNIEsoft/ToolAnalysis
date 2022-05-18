#include "ParseDataMonitoring.h"

ParseDataMonitoring::ParseDataMonitoring():Tool(){}


bool ParseDataMonitoring::Initialise(std::string configfile, DataModel &data){

    if(configfile!="")  m_variables.Initialise(configfile);
    //m_variables.Print();

    m_data= &data;

    if(!m_variables.Get("verbose",verbosity)) verbosity=1;
    m_variables.Get("MaxEntriesPerLoop",max_entries);

    LAPPDData = new BoostStore(false,2);

    return true;
}

bool ParseDataMonitoring::Execute()
{
    int boardindex, retval;
 
    TempData = new BoostStore(false,2);
   
    bool has_lappd;
    std::string state;
    m_data->CStore.Get("State",state);

    if (state == "Wait") {
      Log("ParseDataMonitoringMonitoring: Received State is "+state+", abort",2,verbosity);
      return true;
    }

    m_data->CStore.Get("HasLAPPDData",has_lappd);
    /*if (!has_lappd) {
      Log("ParseDataMonitoringMonitoring: No LAPPD data present! Abort",2,verbosity);
      return true;
    }*/

    if (state == "DataFile" && has_lappd){

    m_data->Stores["LAPPDData"]->Get("LAPPDData",LAPPDData);
    if (verbosity > 1) LAPPDData->Print(false);

    long entries;
    LAPPDData->Header->Get("TotalEntries",entries);
    if (verbosity > 1) std::cout <<"ParseDataMonitoring: TotalEntries: "<<entries<<std::endl;
    
    int entries_loop = (entries > max_entries)? max_entries : entries;

    for (int i_entry=0; i_entry < entries_loop; i_entry++)
    {

        if (verbosity > 2) cout << "Looping through entry " << i_entry << endl;
        
	LAPPDData->GetEntry(i_entry);
        PsecData PDATA;
        LAPPDData->Get("LAPPDData",PDATA);
        if (verbosity > 1) PDATA.Print();

        std::vector<unsigned short> Raw_Buffer = PDATA.RawWaveform;
        std::vector<int> BoardId_Buffer = PDATA.BoardIndex;
    
	if (Raw_Buffer.size() == 0) {
		std::cout <<"Encountered Raw Buffer size of 0! Abort!"<<std::endl;
		continue;
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
	    if (verbosity > 2) std::cout <<"PPS frame"<<std::endl;

	    TempData->Set("PPS",Raw_Buffer);
	    TempData->Set("ACC",PDATA.AccInfoFrame);
	    std::string str_pps = "PPS";
	    TempData->Set("Type",str_pps);
	    TempData->Save("LAPPDTemp");
	    TempData->Delete();

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

/* old version
        for(int bi: BoardId_Buffer)
        {
            //Go over all ACDC board data frames by seperating them
            //for(int c=bi*frametype; c<(bi+1)*frametype; c++)
	    for(int c=0*frametype; c<BoardId_Buffer.size()*frametype; c++)
            {
                Parse_Buffer.push_back(Raw_Buffer[c]);
            }
            //Grab the parsed data and give it to a global variable 'data'
            retval = getParsedData(Parse_Buffer,bi*NUM_CH);
            if(retval == 0)
            {
                //Grab the parsed metadata and give it to a global variable 'meta'
                retval = getParsedMeta(Parse_Buffer,bi);
                if(retval!=0)
                {
                    cout << "Meta parsing went wrong! " << retval << endl;
                }
            }else
            {
                cout << "Parsing went wrong! " << retval << endl;
                return false;
            }
        }
*/

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

        Waveform<double> tmpWave;
        vector<Waveform<double>> VecTmpWave;
        //Loop over data stream
        for(std::map<int, vector<unsigned short>>::iterator it=data.begin(); it!=data.end(); ++it)
        {
	    bool first=true;
            for(unsigned short k: it->second)
            {
                tmpWave.PushSample((double)k);
		if (verbosity > 4 && first) std::cout <<"Chankey "<<it->first<<", ADC value "<<k<<std::endl;
		first = false;
            }
            VecTmpWave.push_back(tmpWave);
            LAPPDWaveforms.insert(pair<unsigned long, vector<Waveform<double>>>((unsigned long)it->first,VecTmpWave));
            tmpWave.ClearSamples();
            VecTmpWave.clear();
        }

	if (verbosity > 4){
		std::cout <<"META DATA"<<std::endl;
		for (int i_meta=0; i_meta < (int) meta.size(); i_meta++){std::cout <<meta.at(i_meta)<<std::endl;}
		std::cout <<"ACC INFO DATA"<<std::endl;
		for (int i_acc=0; i_acc < (int) PDATA.AccInfoFrame.size(); i_acc++){std::cout <<PDATA.AccInfoFrame.at(i_acc)<<std::endl;}
	}

	if (verbosity > 2) std::cout <<"Data entry"<<std::endl;


	TempData->Set("Data",LAPPDWaveforms);
        TempData->Set("ACC",PDATA.AccInfoFrame);
	TempData->Set("Meta",meta);
	std::string str_data="Data";
        TempData->Set("Type",str_data);
        TempData->Save("LAPPDTemp");
        TempData->Delete();

        meta.clear(); 
        LAPPDWaveforms.clear(); 
        data.clear(); 
        Raw_Buffer.clear(); 
        BoardId_Buffer.clear(); 
        Parse_Buffer.clear(); 
	}
    }

        if (verbosity > 2) cout << i_entry << " of " << entries << " done" << endl;
    }

      if (TempData!=0){
        TempData->Close();
        delete TempData;
        TempData=0;
      }
    } else if (state == "LAPPDSingle"){

    PsecData psec;
    m_data->Stores["LAPPDData"]->Get("Single",psec);

    std::vector<unsigned short> Raw_Buffer = psec.RawWaveform;
    std::vector<int> BoardId_Buffer = {psec.BoardIndex};
    
    int frametype = Raw_Buffer.size()/BoardId_Buffer.size();
    if(frametype!=NUM_VECTOR_DATA && frametype!=NUM_VECTOR_PPS)
    {
      cout << "Problem identifying the frametype, size of raw vector was " << Raw_Buffer.size() << endl;
      cout << "It was expected to be either " << NUM_VECTOR_DATA*BoardId_Buffer.size() << " or " <<  NUM_VECTOR_PPS*BoardId_Buffer.size() << endl;
      cout << "Please check manually!" << endl;
      return false;
    }

    if(frametype==NUM_VECTOR_PPS)
    {
      if (verbosity > 2) std::cout <<"PPS frame"<<std::endl;

        m_data->CStore.Set("PPS",Raw_Buffer);
        m_data->CStore.Set("ACC",psec.AccInfoFrame);
        m_data->CStore.Set("Type","PPS");
         
        return true;
      }

      for(int bi: BoardId_Buffer)
      {
        //Go over all ACDC board data frames by seperating them
        //for(int c=bi*frametype; c<(bi+1)*frametype; c++)
	for(int c=0*frametype; c<BoardId_Buffer.size()*frametype; c++)
        {
          Parse_Buffer.push_back(Raw_Buffer[c]);
        }
        //Grab the parsed data and give it to a global variable 'data'
        retval = getParsedData(Parse_Buffer,bi*NUM_CH);
        if(retval == 0)
        {
           //Grab the parsed metadata and give it to a global variable 'meta'
           retval = getParsedMeta(Parse_Buffer,bi);
           if(retval!=0)
           {
             cout << "Meta parsing went wrong! " << retval << endl;
           }
         }else
         {
           cout << "Parsing went wrong! " << retval << endl;
           return false;
          }
      }

      Waveform<double> tmpWave;
      vector<Waveform<double>> VecTmpWave;
      //Loop over data stream
      for(std::map<int, vector<unsigned short>>::iterator it=data.begin(); it!=data.end(); ++it)
      {
         bool first=true;
         for(unsigned short k: it->second)
         {
           tmpWave.PushSample((double)k);
           if (verbosity > 4 && first) std::cout <<"Chankey "<<it->first<<", ADC value "<<k<<std::endl;
	   first = false;
         }
         VecTmpWave.push_back(tmpWave);
         LAPPDWaveforms.insert(pair<unsigned long, vector<Waveform<double>>>((unsigned long)it->first,VecTmpWave));
         tmpWave.ClearSamples();
         VecTmpWave.clear();
      }

      if (verbosity > 4){
        std::cout <<"META DATA"<<std::endl;
	for (int i_meta=0; i_meta < (int) meta.size(); i_meta++){std::cout <<meta.at(i_meta)<<std::endl;}
	std::cout <<"ACC INFO DATA"<<std::endl;
        for (int i_acc=0; i_acc < (int) psec.AccInfoFrame.size(); i_acc++){std::cout <<psec.AccInfoFrame.at(i_acc)<<std::endl;}
      }

      if (verbosity > 2) std::cout <<"Data entry"<<std::endl;

      m_data->CStore.Set("Data",LAPPDWaveforms);
      m_data->CStore.Set("ACC",psec.AccInfoFrame);
      m_data->CStore.Set("Meta",meta);
      std::string str_data="Data";
      m_data->CStore.Set("Type",str_data);

      meta.clear(); 
      LAPPDWaveforms.clear(); 
      data.clear(); 
      Raw_Buffer.clear(); 
      BoardId_Buffer.clear(); 

   }
    

    return true;
}

bool ParseDataMonitoring::Finalise()
{
    return true;
}


int ParseDataMonitoring::getParsedMeta(std::vector<unsigned short> buffer, int BoardId)
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


int ParseDataMonitoring::getParsedData(std::vector<unsigned short> buffer, int ch_start)
{
    //Catch empty buffers
    if(buffer.size() == 0)
    {
        std::cout << "You tried to parse ACDC data without pulling/setting an ACDC buffer" << std::endl;
        return -1;
    }

    //Prepare the Metadata vector 
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
