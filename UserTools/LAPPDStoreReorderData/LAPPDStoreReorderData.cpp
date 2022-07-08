#include "LAPPDStoreReorderData.h"

LAPPDStoreReorderData::LAPPDStoreReorderData():Tool(){}


bool LAPPDStoreReorderData::Initialise(std::string configfile, DataModel &data)
{
    /////////////////// Useful header ///////////////////////
    if(configfile!="") m_variables.Initialise(configfile); // loading config file
    //m_variables.Print();

    m_data= &data; //assigning transient data pointer
    /////////////////////////////////////////////////////////////////

    m_variables.Get("ReorderInputWavLabel",InputWavLabel);
    m_variables.Get("ReorderOutputWavLabel",OutputWavLabel);
    m_variables.Get("DelayOffset",delayoffset);
    m_variables.Get("GlobalShift",globalshift);
    m_variables.Get("ReorderVerbosityLevel",ReorderVerbosityLevel);
    m_variables.Get("NUM_VECTOR_METADATA",NUM_VECTOR_METADATA);
    m_variables.Get("LAPPDchannelOffset",LAPPDchannelOffset);
    //m_data->Stores["ANNIEEvent"]->Get("LAPPDchannelOffset",LAPPDchannelOffset);

    return true;
}

bool LAPPDStoreReorderData::Execute()
{
    vector<unsigned short> acdcmetadata;
    m_data->Stores["ANNIEEvent"]->Get("ACDCmetadata",acdcmetadata);
    std::map<unsigned long,vector<Waveform<double>>> lappddata;
    m_data->Stores["ANNIEEvent"]->Get(InputWavLabel,lappddata);
    vector<int> NReadBoards;
    m_data->Stores["ANNIEEvent"]->Get("ACDCboards",NReadBoards);
    std::map<unsigned long,vector<Waveform<double>>> reordereddata;

    // Loop over waveforms, reorder data
    // For 30 channels change to 10
    vector<unsigned short> Smeta26;
    for(int meta26=0; meta26<NReadBoards.size(); meta26++)
    {
        Smeta26.push_back(acdcmetadata.at((meta26*NUM_VECTOR_METADATA)+10));
        if(ReorderVerbosityLevel>1) cout << "Metaword entry " << meta26 << " is " << Smeta26[meta26]<< endl;
    }
    if(ReorderVerbosityLevel>2) cout<<"REORDER TIME!!!!   "<<acdcmetadata.size()<<" "<<acdcmetadata.at(10)<<" "<<acdcmetadata.at(102)<<endl;

    map <unsigned long, vector<Waveform<double>>> :: iterator itr;
    for (itr = lappddata.begin(); itr != lappddata.end(); ++itr)
    {
        unsigned long channelno = itr->first;
        vector<Waveform<double>> Vwavs = itr->second;
        int switchbit=0;

        //Get the current board and the respective meta word
        int bi = (int)channelno/30;
        unsigned short switchword = Smeta26[std::distance(NReadBoards.begin(),std::find(NReadBoards.begin(),NReadBoards.end(),bi))];

        //Set the switchbit
        switchbit = (switchword & 0x7)*32;
        switchbit+=delayoffset;

        if(ReorderVerbosityLevel>1) cout<<"channel= "<<channelno<<endl;
        vector<Waveform<double>> Vrwav;
        //loop over all Waveforms
        for(int i=0; i<Vwavs.size(); i++){

            Waveform<double> bwav = Vwavs.at(i);
            Waveform<double> rwav;
            Waveform<double> rwavCorr;

            for(int j=0; j< bwav.GetSamples()->size(); j++){

                if(switchbit>255 || switchbit<0) switchbit=0;
                double nsamp = bwav.GetSamples()->at(switchbit);
                rwav.PushSample(nsamp);
                switchbit++;

            }
            for(int j=0; j< rwav.GetSamples()->size(); j++){
                int ibin = j + globalshift;
                if(ibin > 255) ibin = ibin - 255;
                double nsamp = rwav.GetSamples()->at(ibin);
                rwavCorr.PushSample(nsamp);
            }

            Vrwav.push_back(rwavCorr);
        }

        reordereddata.insert(pair<unsigned long, vector<Waveform<double>>>(LAPPDchannelOffset+channelno,Vrwav));

    }


    // Construct the relevant time-stamp/clock variables///////////////////////////////////

    //get the appropriate metadata words from the meta structure
    unsigned short beamgate_63_48 = acdcmetadata.at(7);
    unsigned short beamgate_47_32 = acdcmetadata.at(27);
    unsigned short beamgate_31_16 = acdcmetadata.at(47);
    unsigned short beamgate_15_0 = acdcmetadata.at(67);


    std::bitset<16> bits_beamgate_63_48(beamgate_63_48);
    std::bitset<16> bits_beamgate_47_32(beamgate_47_32);
    std::bitset<16> bits_beamgate_31_16(beamgate_31_16);
    std::bitset<16> bits_beamgate_15_0(beamgate_15_0);

    // cout statements

    if (ReorderVerbosityLevel > 2) std::cout << "bits_beamgate_63_48: " << bits_beamgate_63_48 << std::endl;
    if (ReorderVerbosityLevel > 2) std::cout << "bits_beamgate_47_32: " << bits_beamgate_47_32 << std::endl;
    if (ReorderVerbosityLevel > 2) std::cout << "bits_beamgate_31_16: " << bits_beamgate_31_16 << std::endl;
    if (ReorderVerbosityLevel > 2) std::cout << "bits_beamgate_15_0: " << bits_beamgate_15_0 << std::endl;
    //construct the full 64-bit counter number
    unsigned long beamgate_63_0 = (static_cast<unsigned long>(beamgate_63_48) << 48) + (static_cast<unsigned long>(beamgate_47_32) << 32) + (static_cast<unsigned long>(beamgate_31_16) << 16) + (static_cast<unsigned long>(beamgate_15_0));
    // cout statement
    if (ReorderVerbosityLevel > 2) std::cout << "beamgate combined: " << beamgate_63_0 << std::endl;
    // binary digit
    std::bitset<64> bits_beamgate_63_0(beamgate_63_0);
    // cout the binary
    if (ReorderVerbosityLevel > 2) std::cout << "bits_beamgate_63_0: " << bits_beamgate_63_0 << std::endl;

    // hex manipulations
    std::stringstream str_beamgate_15_0;
    str_beamgate_15_0 << std::hex << (beamgate_15_0);
    std::stringstream str_beamgate_31_16;
    str_beamgate_31_16 << std::hex << (beamgate_31_16);
    std::stringstream str_beamgate_47_32;
    str_beamgate_47_32 << std::hex << (beamgate_47_32);
    std::stringstream str_beamgate_63_48;
    str_beamgate_63_48 << std::hex << (beamgate_63_48);
    const char *hexstring = str_beamgate_63_48.str().c_str();
    unsigned int meta7_1 = (unsigned int)strtol(hexstring, NULL, 16);
    hexstring = str_beamgate_47_32.str().c_str();
    unsigned int meta27_1 = (unsigned int) strtol(hexstring, NULL, 16);
    hexstring = str_beamgate_31_16.str().c_str();
    unsigned int meta47_1 = (unsigned int) strtol(hexstring, NULL, 16);
    hexstring = str_beamgate_15_0.str().c_str();
    unsigned int meta67_1 = (unsigned int) strtol(hexstring, NULL, 16);
    meta7_1 = meta7_1 << 16;
    meta47_1 = meta47_1 << 16;

    // my two beam counter values
    unsigned int beamcounter = meta47_1 + meta67_1;
    unsigned int beamcounterL = meta7_1 + meta27_1;
    // as doubles
    double largetime = (double)beamcounterL*13.1;
    double smalltime = ((double)beamcounter/1E9)*3.125;

    // bunch of couts
    if (ReorderVerbosityLevel > 2) {
    std::cout <<"meta7_1: "<<meta7_1<<std::endl;
    std::cout <<"meta27_1: "<<meta27_1<<std::endl;
    std::cout <<"meta47_1: "<<meta47_1<<std::endl;
    std::cout <<"meta67_1: "<<meta67_1<<std::endl;
    std::cout <<"beamcounter: "<<beamcounter<<std::endl;
    std::cout <<"beamcounterL: "<<beamcounterL<<std::endl;
    std::cout <<"largetime: "<<largetime<<std::endl;
    std::cout <<"smalltime: "<<smalltime<<std::endl;
    std::cout <<"eventtime: "<<(largetime+smalltime)<<std::endl;
    std::cout <<"beamgate old: "<<((beamgate_63_0/1E9)*3.125)<<std::endl;
    }

    //Build data timestamp
    unsigned short timestamp_63_48 = acdcmetadata.at(70);
    unsigned short timestamp_47_32 = acdcmetadata.at(50);
    unsigned short timestamp_31_16 = acdcmetadata.at(30);
    unsigned short timestamp_15_0 = acdcmetadata.at(10);
    std::bitset<16> bits_timestamp_63_48(timestamp_63_48);
    std::bitset<16> bits_timestamp_47_32(timestamp_47_32);
    std::bitset<16> bits_timestamp_31_16(timestamp_31_16);
    std::bitset<16> bits_timestamp_15_0(timestamp_15_0);
    if (ReorderVerbosityLevel > 2) std::cout << "bits_timestamp_63_48: " << bits_timestamp_63_48 << std::endl;
    if (ReorderVerbosityLevel > 2) std::cout << "bits_timestamp_47_32: " << bits_timestamp_47_32 << std::endl;
    if (ReorderVerbosityLevel > 2) std::cout << "bits_timestamp_31_16: " << bits_timestamp_31_16 << std::endl;
    if (ReorderVerbosityLevel > 2) std::cout << "bits_timestamp_15_0: " << bits_timestamp_15_0 << std::endl;
    //construct the full 64-bit counter number
    unsigned long timestamp_63_0 = (static_cast<unsigned long>(timestamp_63_48) << 48) + (static_cast<unsigned long>(timestamp_47_32) << 32) + (static_cast<unsigned long>(timestamp_31_16) << 16) + (static_cast<unsigned long>(timestamp_15_0));
    // cout statement
    if (ReorderVerbosityLevel > 2) std::cout << "timestamp combined: " << timestamp_63_0 << std::endl;
    std::bitset<64> bits_timestamp_63_0(timestamp_63_0);
    // cout the binary
    if (ReorderVerbosityLevel > 2) std::cout << "bits_timestamp_63_0: " << bits_timestamp_63_0 << std::endl;

    // hex manipulations
    std::stringstream str_timestamp_63_48;
    str_timestamp_63_48 << std::hex << (timestamp_63_48);
    std::stringstream str_timestamp_47_32;
    str_timestamp_47_32 << std::hex << (timestamp_47_32);
    std::stringstream str_timestamp_31_16;
    str_timestamp_31_16 << std::hex << (timestamp_31_16);
    std::stringstream str_timestamp_15_0;
    str_timestamp_15_0 << std::hex << (timestamp_15_0);

    hexstring = str_timestamp_63_48.str().c_str();
    unsigned int meta70_1 = (unsigned int)strtol(hexstring, NULL, 16);
    hexstring = str_timestamp_47_32.str().c_str();
    unsigned int meta50_1 = (unsigned int) strtol(hexstring, NULL, 16);
    hexstring = str_timestamp_31_16.str().c_str();
    unsigned int meta30_1 = (unsigned int) strtol(hexstring, NULL, 16);
    hexstring = str_timestamp_15_0.str().c_str();
    unsigned int meta10_1 = (unsigned int) strtol(hexstring, NULL, 16);
    meta70_1 = meta70_1 << 16;
    meta30_1 = meta30_1 << 16;

    // my two beam counter values
    unsigned int trigcounter = meta30_1 + meta10_1;
    unsigned int trigcounterL = meta50_1 + meta70_1;
    // as doubles
    //double largetime = (double)beamcounterL*13.1;
    //double smalltime = ((double)beamcounter/1E9)*3.125;

    vector<unsigned int> tcounters;
    tcounters.push_back(beamcounter);
    tcounters.push_back(beamcounterL);
    tcounters.push_back(trigcounter);
    tcounters.push_back(trigcounterL);

    /*
      	beamgate_timestamp.push_back(beamgate_63_0);
    		data_timestamp.push_back(timestamp_63_0);

    		//for (int i=0; i<first_entry.size(); i++) std::cout << first_entry.at(i)<<std::endl;

    		data_beamgate_lastfile.at(board_idx).push_back(timestamp_63_0 - beamgate_63_0);
    		if (first_entry.at(vector_idx) == true) {
    			//std::cout <<"true"<<std::endl;
    			first_beamgate_timestamp.at(vector_idx) = beamgate_63_0;
    			first_timestamp.at(vector_idx) = timestamp_63_0;
    			first_entry.at(vector_idx) = false;
    		}
    		//Just overwrite "last" entry every time
    		last_beamgate_timestamp.at(vector_idx) = beamgate_63_0;
    		last_timestamp.at(vector_idx) = timestamp_63_0;
    */

    m_data->Stores["ANNIEEvent"]->Set("TimingCounters",tcounters);
    m_data->Stores["ANNIEEvent"]->Set(OutputWavLabel,reordereddata);

    return true;
}


bool LAPPDStoreReorderData::Finalise()
{
    return true;
}
