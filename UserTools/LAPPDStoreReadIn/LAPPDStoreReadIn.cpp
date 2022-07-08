#include "LAPPDStoreReadIn.h"
#include "PsecData.h"

LAPPDStoreReadIn::LAPPDStoreReadIn():Tool(){}


bool LAPPDStoreReadIn::Initialise(std::string configfile, DataModel &data){
    /////////////////// Useful header ///////////////////////
    if(configfile!="") m_variables.Initialise(configfile); // loading config file
    //m_variables.Print();

    m_data= &data; //assigning transient data pointer
    /////////////////////////////////////////////////////////////////

    //general variables are read from the config file, currently
    //Filename
    m_variables.Get("PSECinputfile", NewFileName);
    //Verbosity
    m_variables.Get("LAPPDStoreReadInVerbosity",LAPPDStoreReadInVerbosity);
    //Details on channels, samples and max vector sizes and trigger channel
    m_variables.Get("Nsamples", Nsamples);
    m_variables.Get("TrigChannel", TrigChannel);
    m_variables.Get("NUM_VECTOR_DATA",NUM_VECTOR_DATA);
    m_variables.Get("NUM_VECTOR_PPS",NUM_VECTOR_PPS);

    //Get the input and output labes for the store entries
    m_variables.Get("RawDataOutpuWavLabel",OutputWavLabel);
    m_variables.Get("RawDataInputWavLabel",InputWavLabel);
    m_variables.Get("BoardIndexLabel",BoardIndexLabel);

//Multi layer boost store setup
    //Create a multi-layer boost store and give it the file
    m_data->Stores["indata"] = new BoostStore(false,0);
    m_data->Stores["indata"]->Initialise(NewFileName);
    m_data->Stores["indata"]->Print(false);

    //Create store for the LAPPD data
    m_data->Stores["LAPPDData"] = new BoostStore(false,2);
    //BoostStore *LAPPDData = new BoostStore(false,2);

    m_data->Stores["indata"]->Get("LAPPDData",*m_data->Stores["LAPPDData"]);
    std::cout <<"Print LAPPDData:"<<std::endl;
    m_data->Stores["LAPPDData"]->Print(false);
    //m_data->Stores["LAPPDData"]->Header->Print(false);
/*Single layer boost store setup*/
//    m_data->Stores["LAPPDData"] = new BoostStore(false,2);
//    m_data->Stores["LAPPDData"]->Initialise(NewFileName);
//    if(LAPPDStoreReadInVerbosity>0) m_data->Stores["LAPPDData"]->Print(false);
//*/

    //Grab the amount of total entries in the LAPPDData Store
    m_data->Stores["LAPPDData"]->Header->Get("TotalEntries",entries);
    if(LAPPDStoreReadInVerbosity>0) cout << NewFileName << " got " << entries << endl;

    //since unsure if LAPPDFilter, LAPPDBaselineSubtract, or LAPPDcfd are being used, isFiltered, isBLsub, and isCFD are set to false
    bool isFiltered = false;
    m_data->Stores["ANNIEEvent"]->Set("isFiltered",isFiltered);
    bool isBLsub = false;
    m_data->Stores["ANNIEEvent"]->Set("isBLsubtracted",isBLsub);
    bool isCFD=false;
    m_data->Stores["ANNIEEvent"]->Set("isCFD",isCFD);

    //Grab all pedestal files and prepare the map channel|pedestal-vector for substraction
    PedestalValues = new std::map<unsigned long, vector<int>>;
    m_variables.Get("DoPedSubtraction", DoPedSubtract);
    m_variables.Get("Nboards", Nboards);
    m_variables.Get("Pedinputfile",PedFileName);
    m_data->Stores["PedestalFile"] = new BoostStore(false,2);
    if(DoPedSubtract==1)//extra work for multi ped files and stores
    {
        bool ret=false;
        if (FILE *file = fopen(PedFileName.c_str(), "r"))
        {
            fclose(file);
            ret = true;
            cout << "Using Store Pedestal File" << endl;
        }
        if(ret)
        {
            m_data->Stores["PedestalFile"]->Initialise(PedFileName);
            long Pedentries;
            m_data->Stores["PedestalFile"]->Header->Get("TotalEntries",Pedentries);
            if(LAPPDStoreReadInVerbosity>0) cout << PedFileName << " got " << Pedentries << endl;
            m_data->Stores["PedestalFile"]->Get("PedestalMap",PedestalValues);
        }else
        {
            m_variables.Get("PedinputfileTXT", PedFileNameTXT);
            for(int i=0; i<Nboards; i++)
            {
                ReadPedestals(i);
            }
        }
    }
    if(DoPedSubtract==1 && LAPPDStoreReadInVerbosity>1) cout<<"PEDSIZES: "<<PedestalValues->size()<<" "<<PedestalValues->at(0).size()<<" "<<PedestalValues->at(4).at(5)<<endl;

    //parameters (potentially) used by the whole ToolChain
    m_variables.Get("Nsamples", Nsamples);
    m_variables.Get("NChannels", NChannels);
    m_variables.Get("TrigChannel", TrigChannel);
    m_variables.Get("LAPPDchannelOffset", LAPPDchannelOffset);
    m_variables.Get("SampleSize", SampleSize);

    m_data->Stores["ANNIEEvent"]->Set("Nsamples", Nsamples);
    m_data->Stores["ANNIEEvent"]->Set("NChannels", NChannels);
    m_data->Stores["ANNIEEvent"]->Set("TrigChannel", TrigChannel);
    m_data->Stores["ANNIEEvent"]->Set("LAPPDchannelOffset", LAPPDchannelOffset);
    m_data->Stores["ANNIEEvent"]->Set("SampleSize", SampleSize);

    //Prepare to start with event 0
    eventNo=0;

    return true;
}


bool LAPPDStoreReadIn::Execute(){

    if((long)eventNo==entries) m_data->vars.Set("StopLoop",1);

    if(eventNo%10==0) cout<<"Event: " << eventNo << endl;

    PsecData dat;
    int i_entry = eventNo;
    if(LAPPDStoreReadInVerbosity>2) cout << "Starting entry " << i_entry << endl;
    m_data->Stores["LAPPDData"]->GetEntry(i_entry);
    m_data->Stores["LAPPDData"]->Get("LAPPDData",dat);
    if(LAPPDStoreReadInVerbosity>2) cout << "Got entry " << i_entry << endl;

    ReadBoards = dat.BoardIndex;
    Raw_buffer = dat.RawWaveform;

    /*
    cout<<InputWavLabel<<" "<<BoardIndexLabel<<endl;
    bool test1 = m_data->Stores["LAPPDData"]->Get(InputWavLabel,Raw_buffer);
    bool test2 = m_data->Stores["LAPPDData"]->Get(BoardIndexLabel,ReadBoards);
    cout<<"find em?"<<test1<<" "<<test2<<" "<<BoardIndex.size()<<endl;
*/

    if(LAPPDStoreReadInVerbosity>2) cout << "Number of boards was " << ReadBoards.size() << endl;

    int frametype = Raw_buffer.size()/ReadBoards.size();
    //cout<<"FRAMETYPE: "<<frametype<<endl;
    if(frametype!=NUM_VECTOR_DATA && frametype!=NUM_VECTOR_PPS)
    {
        cout << "Problem identifying the frametype, size of raw vector was " << Raw_buffer.size() << endl;
        cout << "It was expected to be either " << NUM_VECTOR_DATA*ReadBoards.size() << " or " <<  NUM_VECTOR_PPS*ReadBoards.size() << endl;
        cout << "Please check manually!" << endl;
        return false;
    }

    if(frametype==NUM_VECTOR_PPS)
    {
        if(LAPPDStoreReadInVerbosity>2) std::cout << "PPS Frame was read! Please fix me!" << std::endl;
        //return all as is
        /*
        m_data->ToDataModel.DATA = EMPTY;
        m_data->ToDataModel.PPS = Raw_Buffer;
        m_data->ToDataModel.ACCINFOFRAME = PDATA.AccInfoFrame;
        m_data->ToDataModel.METADATA = EMPTY;
        */
        return true;
    }else if(frametype==NUM_VECTOR_DATA)
    {
        if(LAPPDStoreReadInVerbosity>1) std::cout << "Data Frame was read! Starting the parsing!" << std::endl;
    }

    //Create a vector of paraphrased board indices
    int nbi = ReadBoards.size();
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

    for(int bi: ParaBoards)
    {
        Parse_buffer.clear();
        if(LAPPDStoreReadInVerbosity>2) std::cout << "Starting with board " << ReadBoards[bi] << std::endl;
        //Go over all ACDC board data frames by seperating them
        for(int c=bi*frametype; c<(bi+1)*frametype; c++)
        {
            Parse_buffer.push_back(Raw_buffer[c]);
        }
        if(LAPPDStoreReadInVerbosity>2) std::cout << "Data for board " << ReadBoards[bi] << " was grabbed!" << std::endl;


        //Grab the parsed data and give it to a global variable 'data'
        retval = getParsedData(Parse_buffer,ReadBoards[bi]*NUM_CH);
        if(retval == 0)
        {
            if(LAPPDStoreReadInVerbosity>2) std::cout << "Data for board " << ReadBoards[bi] << " was parsed!" << std::endl;
            //Grab the parsed metadata and give it to a global variable 'meta'
            retval = getParsedMeta(Parse_buffer,ReadBoards[bi]);
            if(retval!=0)
            {
                std::cout << "Meta parsing went wrong! " << retval << endl;
            }else
            {
                if(LAPPDStoreReadInVerbosity>2) std::cout << "Meta for board " << ReadBoards[bi] << " was parsed!" << std::endl;
            }
        }else
        {
            std::cout << "Parsing went wrong! " << retval << endl;
            return false;
        }
    }

    if(LAPPDStoreReadInVerbosity>0) cout<<"BEGIN LAPPDStoreReadIn "<< endl;

    std::map<unsigned long, vector<Waveform<double>>> LAPPDWaveforms;
    Waveform<double> tmpWave;
    vector<Waveform<double>> VecTmpWave;
    int pedval,val;
    //Loop over data stream
    for(std::map<int, vector<unsigned short>>::iterator it=data.begin(); it!=data.end(); ++it)
    {
        for(int kvec=0; kvec<it->second.size(); kvec++)
        {
            if(DoPedSubtract==1)
            {
                pedval = ((PedestalValues->find(it->first))->second).at(kvec);
            }else
            {
                pedval = 0;
            }
            val = it->second.at(kvec);
            tmpWave.PushSample(0.3*(double)(val-pedval));
        }
        VecTmpWave.push_back(tmpWave);
        LAPPDWaveforms.insert(pair<unsigned long, vector<Waveform<double>>>((unsigned long)it->first,VecTmpWave));
        tmpWave.ClearSamples();
        VecTmpWave.clear();
    }



    if(LAPPDStoreReadInVerbosity>0) cout<<"*************************END LAPPDStoreReadIn************************************"<<endl;

    m_data->Stores["ANNIEEvent"]->Set(OutputWavLabel,LAPPDWaveforms);
    m_data->Stores["ANNIEEvent"]->Set("ACDCmetadata",meta);
    m_data->Stores["ANNIEEvent"]->Set("ACDCboards",ReadBoards);
    m_data->Stores["ANNIEEvent"]->Set("TriggerChannelBase",TrigChannel);

    meta.clear();
    LAPPDWaveforms.clear();
    data.clear();
    Raw_buffer.clear();
    ReadBoards.clear();
    Parse_buffer.clear();

    eventNo++;
    return true;
}


bool LAPPDStoreReadIn::Finalise(){
    m_data->Stores["LAPPDData"]->Close();
    //m_data->Stores["indata"]->Close();
    return true;
}


bool LAPPDStoreReadIn::ReadPedestals(int boardNo){

    if(LAPPDStoreReadInVerbosity>0) cout<<"Getting Pedestals "<<boardNo<<endl;

    std::string LoadName = PedFileNameTXT;
    string nextLine; //temp line to parse
    double finalsampleNo;
    std::string ext = std::to_string(boardNo);
    ext += ".txt";
    LoadName += ext;
    PedFile.open(LoadName);
    if(!PedFile.is_open())
    {
        cout<<"Failed to open "<<LoadName<<"!"<<endl;
        return false;
    }
    if(LAPPDStoreReadInVerbosity>0) cout<<"Opened file: "<<LoadName<<endl;

    int sampleNo=0; //sample number
    while(getline(PedFile, nextLine))
    {
        istringstream iss(nextLine); //copies the current line in the file
        int location=-1; //counts the current perameter in the line
        string stempValue; //current string in the line
        int tempValue; //current int in the line

        unsigned long channelNo=boardNo*30; //channel number
        //cout<<"NEW BOARD "<<channelNo<<" "<<sampleNo<<endl;

        //starts the loop at the beginning of the line
        while(iss >> stempValue)
        {
            location++;

            int tempValue = stoi(stempValue, 0, 10);

            if(sampleNo==0){
                vector<int> tempPed;
                tempPed.push_back(tempValue);
                //cout<<"First time: "<<channelNo<<" "<<tempValue<<endl;
                PedestalValues->insert(pair <unsigned long,vector<int>> (channelNo,tempPed));
                //newboard=false;
            }
            else{
                //cout<<"Following time: "<<channelNo<<" "<<tempValue<<"    F    "<<PedestalValues->count(channelNo)<<endl;
                (((PedestalValues->find(channelNo))->second)).push_back(tempValue);
            }

            channelNo++;
        }

        sampleNo++;
        //when the sampleNo gets to ff
        //it is at sample 256 and needs to go to the next event
        //if(sampleNo=='ff')
        //if(sampleNo==255)
        //{
            //cout<<"DID THIS HAPPEN "<<sampleNo<<" "<<eventNo<<endl;
        //    eventNo++;
        //    break;
        //}

        //cout<<"at the end"<<endl;
    }

    if(LAPPDStoreReadInVerbosity>0) cout<<"FINAL SAMPLE NUMBER: "<<PedestalValues->size()<<" "<<(((PedestalValues->find(0))->second)).size()<<endl;
    PedFile.close();
  return true;
}


bool LAPPDStoreReadIn::MakePedestals(){

  //Empty for now...

  return true;
}

int LAPPDStoreReadIn::getParsedMeta(std::vector<unsigned short> buffer, int BoardId)
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


int LAPPDStoreReadIn::getParsedData(std::vector<unsigned short> buffer, int ch_start)
{
    //Catch empty buffers
    if(buffer.size() == 0)
    {
        std::cout << "You tried to parse ACDC data without pulling/setting an ACDC buffer" << std::endl;
        return -1;
    }

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
                data.insert(pair<int, vector<unsigned short>>(ch_start + channel_count, InfoWord));
                InfoWord.clear();
                channel_count++;
            }
            ++bit;
        }
    }

    return 0;
}
