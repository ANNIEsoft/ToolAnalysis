#include "LAPPDStoreReorder.h"

LAPPDStoreReorder::LAPPDStoreReorder() : Tool() {}

bool LAPPDStoreReorder::Initialise(std::string configfile, DataModel &data)
{
    /////////////////// Useful header ///////////////////////
    if (configfile != "")
        m_variables.Initialise(configfile); // loading config file
    // m_variables.Print();

    m_data = &data; // assigning transient data pointer
    /////////////////////////////////////////////////////////////////

    m_variables.Get("ReorderInputWavLabel", InputWavLabel);
    m_variables.Get("ReorderOutputWavLabel", OutputWavLabel);
    m_variables.Get("DelayOffset", delayoffset);
    m_variables.Get("GlobalShift", GlobalShift);
    m_variables.Get("LAPPDReorderVerbosityLevel", LAPPDReorderVerbosityLevel);
    m_variables.Get("NUM_VECTOR_METADATA", NUM_VECTOR_METADATA);
    m_variables.Get("LAPPDchannelOffset", LAPPDchannelOffset);
    bool gotGeomerty = m_data->Stores["ANNIEEvent"]->Header->Get("AnnieGeometry", _geom);
    if (!gotGeomerty)
    {
        Log("Error: LAPPDStoreReorder: Failed to get ANNIEGeometry from the ANNIEEvent store", 0, LAPPDReorderVerbosityLevel);
        return false;
    }
    LoadLAPPDMap = false;
    m_variables.Get("LoadLAPPDMap", LoadLAPPDMap);

    return true;
}

bool LAPPDStoreReorder::Execute()
{
    CleanDataObjects();
    if (LAPPDReorderVerbosityLevel > 3)
        cout << "LAPPDStoreReorder::Execute()" << endl;
    m_data->CStore.Get("LAPPDana", LAPPDana);
    if (!LAPPDana)
    {
        if (LAPPDReorderVerbosityLevel > 3)
            cout << "LAPPDStoreReorder::Execute() LAPPDana is false, returning" << endl;
        return true;
    }

    m_data->Stores["ANNIEEvent"]->Get("ACDCmetadata", acdcmetadata);
    m_data->Stores["ANNIEEvent"]->Get(InputWavLabel, lappddata);
    m_data->Stores["ANNIEEvent"]->Get("ACDCboards", NReadBoards);
    m_data->Stores["ANNIEEvent"]->Get("ACDCReadedLAPPDID;", ACDCReadedLAPPDID);

    // Loop over waveforms, reorder data
    DoReorder();
    // this reorder will change the channel number by adding an channel offset.

    m_data->Stores["ANNIEEvent"]->Set("TimingCounters", tcounters);
    m_data->Stores["ANNIEEvent"]->Set(OutputWavLabel, reordereddata);

    if (LAPPDReorderVerbosityLevel > 1)
    {
        cout << "LAPPDStoreReorder, reordered data size is " << reordereddata.size() << endl;
    }

    if (LAPPDReorderVerbosityLevel > 3)
        cout << "LAPPDStoreReorder::Execute() done" << endl;

    return true;
}

bool LAPPDStoreReorder::Finalise()
{
    return true;
}

void LAPPDStoreReorder::CleanDataObjects()
{
    reordereddata.clear();
    lappddata.clear();
    acdcmetadata.clear();
    NReadBoards.clear();
    ACDCReadedLAPPDID.clear();
    tcounters.clear();
}

bool LAPPDStoreReorder::DoReorder()
{
    Log("LAPPDStoreReorder::DoReorder()", 1, LAPPDReorderVerbosityLevel);
    // For 30 channels change to 10
    vector<unsigned short> Smeta26;
    for (int meta26 = 0; meta26 < NReadBoards.size(); meta26++)
    {
        Smeta26.push_back(acdcmetadata.at((meta26 * NUM_VECTOR_METADATA) + 10));
        if (LAPPDReorderVerbosityLevel > 1)
        {
            cout << "Metaword entry " << meta26 << " is " << Smeta26[meta26] << endl;
            cout << "pushed meta26 = " << meta26 << ", with value at " << (meta26 * NUM_VECTOR_METADATA) + 10 << " is " << acdcmetadata.at((meta26 * NUM_VECTOR_METADATA) + 10) << endl;
        }
    }

    vector<int> Smeta_BG;
    for (int meta_bg = 0; meta_bg < NReadBoards.size(); meta_bg++)
    {
        unsigned short metaBG = acdcmetadata.at((meta_bg * NUM_VECTOR_METADATA) + 67);
        Smeta_BG.push_back((metaBG & 0x7));
        if (LAPPDReorderVerbosityLevel > 1)
        {
            cout << "Metaword entry " << meta_bg << " is " << Smeta_BG[meta_bg] << endl;
            cout << "pushed meta_bg = " << meta_bg << ", with value at " << (meta_bg * NUM_VECTOR_METADATA) + 67 << " is " << acdcmetadata.at((meta_bg * NUM_VECTOR_METADATA) + 67) << endl;
        }
    }

    std::map<int, vector<int>> Smeta_BG_map; // key is id, value is the switch bit for beamgate

    if (LAPPDReorderVerbosityLevel > 2)
        cout << "REORDER TIME!!!!   " << acdcmetadata.size() << " " << acdcmetadata.at(10) << " " << acdcmetadata.at(102) << " lappd data size is " << lappddata.size() << ", reordereddata size is " << reordereddata.size() << endl;
    map<unsigned long, vector<Waveform<double>>>::iterator itr;
    for (itr = lappddata.begin(); itr != lappddata.end(); ++itr)
    {
        if (LAPPDReorderVerbosityLevel > 4)
            cout << "reordering channelno= " << itr->first << endl;
        unsigned long channelno = itr->first;
        int channelHere = channelno;
        channelHere = channelHere % 1000 + 1000; // hmmm the offset in geometry is 1000
        Channel *chan = _geom->GetChannel(channelHere);
        int stripno = chan->GetStripNum();
        vector<Waveform<double>> Vwavs = itr->second;
        if (LAPPDReorderVerbosityLevel > 5)
        {
            cout << "this channel has " << Vwavs.size() << " waveforms" << endl;
            if (Vwavs.size() > 0)
                cout << "The first waveform has " << Vwavs.at(0).GetSamples()->size() << " samples" << endl;
        }
        int switchbit = 0;
        // Get the current board and the respective meta word
        int bi = (int)channelno / 30;
        int LAPPDID = static_cast<int>((channelno % 1000) / 60);
        //  if (LoadLAPPDMap)
        // {
        int beginningBoardIDofThisLAPPDID = NReadBoards.at(std::distance(ACDCReadedLAPPDID.begin(), std::find(ACDCReadedLAPPDID.begin(), ACDCReadedLAPPDID.end(), LAPPDID)));
        bi = beginningBoardIDofThisLAPPDID + bi % 2;
        // }else{
        //     bi = bi%2;
        //  }
        if (LAPPDReorderVerbosityLevel > 6)
        {
            // print the elements in NReadBoards, print LAPPDID;
            cout << "NReadBoards size is " << NReadBoards.size() << endl;
            for (int i = 0; i < NReadBoards.size(); i++)
            {
                cout << "NReadBoards[" << i << "] = " << NReadBoards[i] << endl;
            }
            cout << "LAPPDID = " << LAPPDID << endl;
        }
        unsigned short switchword = Smeta26[std::distance(NReadBoards.begin(), std::find(NReadBoards.begin(), NReadBoards.end(), bi))];
        int switchBitBG = Smeta_BG[std::distance(NReadBoards.begin(), std::find(NReadBoards.begin(), NReadBoards.end(), bi))];

        //check if key LAPPDID is already exist in Smeta_BG_map, if not, resize that vector to size 2
        if (Smeta_BG_map.find(LAPPDID) == Smeta_BG_map.end())
        {
            Smeta_BG_map[LAPPDID] = vector<int>(2);
        }
        Smeta_BG_map[LAPPDID][bi%2] = switchBitBG;

        // Smeta26, 0 or 1, so switchword is the first timestmap or the second
        // Set the switchbit
        switchbit = (switchword & 0x7) * 32;
        // insert the stripno and switchbit into the map

        // take the last 3 bits of PSEC0 timestamp (10 in meta words), then times 2^5, shift left by 5
        switchbit += delayoffset;

        vector<Waveform<double>> Vrwav;
        // loop over all Waveforms
        for (int i = 0; i < Vwavs.size(); i++)
        {

            Waveform<double> bwav = Vwavs.at(i);
            Waveform<double> rwav;
            Waveform<double> rwavCorr;

            for (int j = 0; j < bwav.GetSamples()->size(); j++)
            {

                if (switchbit > 255 || switchbit < 0)
                    switchbit = 0;
                double nsamp = bwav.GetSamples()->at(switchbit);
                rwav.PushSample(nsamp);
                switchbit++;
            }
            for (int j = 0; j < rwav.GetSamples()->size(); j++)
            {
                int ibin = j + GlobalShift;
                if (ibin > 255)
                    ibin = ibin - 255;
                double nsamp = rwav.GetSamples()->at(ibin);
                // cout << "ibin before shift is " << j << " value is " << rwav.GetSamples()->at(j) << ", after shift is " << ibin << " value is " << nsamp << endl;
                rwavCorr.PushSample(nsamp);
            }

            Vrwav.push_back(rwavCorr);
        }

        reordereddata.insert(pair<unsigned long, vector<Waveform<double>>>(LAPPDchannelOffset + channelno, Vrwav));
        if (LAPPDReorderVerbosityLevel > 4)
            cout << "inserted channelno: " << LAPPDchannelOffset + channelno << ", current reorded data size is " << reordereddata.size() << ", switchword = " << switchword << ", switchbit = " << switchbit << endl;
    }
    if (LAPPDReorderVerbosityLevel > 4)
    {
        cout << "LAPPDStoreReorder, reordered data size is " << reordereddata.size() << endl;
    }

    m_data->Stores["ANNIEEvent"]->Set("SwitchBitBG", Smeta_BG_map);
    if(LAPPDReorderVerbosityLevel>3)
    {
        cout << "LAPPDStoreReorder:: insert SwitchBitBG map size is " << Smeta_BG_map.size() << endl;
    }

    return true;
}

bool LAPPDStoreReorder::ConstructTimestampsFromMeta()
{
    // get the appropriate metadata words from the meta structure
    unsigned short beamgate_63_48 = acdcmetadata.at(7);
    unsigned short beamgate_47_32 = acdcmetadata.at(27);
    unsigned short beamgate_31_16 = acdcmetadata.at(47);
    unsigned short beamgate_15_0 = acdcmetadata.at(67);

    std::bitset<16> bits_beamgate_63_48(beamgate_63_48);
    std::bitset<16> bits_beamgate_47_32(beamgate_47_32);
    std::bitset<16> bits_beamgate_31_16(beamgate_31_16);
    std::bitset<16> bits_beamgate_15_0(beamgate_15_0);

    // cout statements

    if (LAPPDReorderVerbosityLevel > 2)
        std::cout << "bits_beamgate_63_48: " << bits_beamgate_63_48 << std::endl;
    if (LAPPDReorderVerbosityLevel > 2)
        std::cout << "bits_beamgate_47_32: " << bits_beamgate_47_32 << std::endl;
    if (LAPPDReorderVerbosityLevel > 2)
        std::cout << "bits_beamgate_31_16: " << bits_beamgate_31_16 << std::endl;
    if (LAPPDReorderVerbosityLevel > 2)
        std::cout << "bits_beamgate_15_0: " << bits_beamgate_15_0 << std::endl;
    // construct the full 64-bit counter number
    unsigned long beamgate_63_0 = (static_cast<unsigned long>(beamgate_63_48) << 48) + (static_cast<unsigned long>(beamgate_47_32) << 32) + (static_cast<unsigned long>(beamgate_31_16) << 16) + (static_cast<unsigned long>(beamgate_15_0));
    // cout statement
    if (LAPPDReorderVerbosityLevel > 2)
        std::cout << "beamgate combined: " << beamgate_63_0 << std::endl;
    // binary digit
    std::bitset<64> bits_beamgate_63_0(beamgate_63_0);
    // cout the binary
    if (LAPPDReorderVerbosityLevel > 2)
        std::cout << "bits_beamgate_63_0: " << bits_beamgate_63_0 << std::endl;

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
    unsigned int meta27_1 = (unsigned int)strtol(hexstring, NULL, 16);
    hexstring = str_beamgate_31_16.str().c_str();
    unsigned int meta47_1 = (unsigned int)strtol(hexstring, NULL, 16);
    hexstring = str_beamgate_15_0.str().c_str();
    unsigned int meta67_1 = (unsigned int)strtol(hexstring, NULL, 16);
    meta7_1 = meta7_1 << 16;
    meta47_1 = meta47_1 << 16;

    // my two beam counter values
    unsigned int beamcounter = meta47_1 + meta67_1;
    unsigned int beamcounterL = meta7_1 + meta27_1;
    // as doubles
    double largetime = (double)beamcounterL * 13.1;
    double smalltime = ((double)beamcounter / 1E9) * 3.125;

    // bunch of couts
    if (LAPPDReorderVerbosityLevel > 2)
    {
        std::cout << "meta7_1: " << meta7_1 << std::endl;
        std::cout << "meta27_1: " << meta27_1 << std::endl;
        std::cout << "meta47_1: " << meta47_1 << std::endl;
        std::cout << "meta67_1: " << meta67_1 << std::endl;
        std::cout << "beamcounter: " << beamcounter << std::endl;
        std::cout << "beamcounterL: " << beamcounterL << std::endl;
        std::cout << "largetime: " << largetime << std::endl;
        std::cout << "smalltime: " << smalltime << std::endl;
        std::cout << "eventtime: " << (largetime + smalltime) << std::endl;
        std::cout << "beamgate old: " << ((beamgate_63_0 / 1E9) * 3.125) << std::endl;
    }

    // Build data timestamp
    unsigned short timestamp_63_48 = acdcmetadata.at(70);
    unsigned short timestamp_47_32 = acdcmetadata.at(50);
    unsigned short timestamp_31_16 = acdcmetadata.at(30);
    unsigned short timestamp_15_0 = acdcmetadata.at(10);
    std::bitset<16> bits_timestamp_63_48(timestamp_63_48);
    std::bitset<16> bits_timestamp_47_32(timestamp_47_32);
    std::bitset<16> bits_timestamp_31_16(timestamp_31_16);
    std::bitset<16> bits_timestamp_15_0(timestamp_15_0);
    if (LAPPDReorderVerbosityLevel > 2)
        std::cout << "bits_timestamp_63_48: " << bits_timestamp_63_48 << std::endl;
    if (LAPPDReorderVerbosityLevel > 2)
        std::cout << "bits_timestamp_47_32: " << bits_timestamp_47_32 << std::endl;
    if (LAPPDReorderVerbosityLevel > 2)
        std::cout << "bits_timestamp_31_16: " << bits_timestamp_31_16 << std::endl;
    if (LAPPDReorderVerbosityLevel > 2)
        std::cout << "bits_timestamp_15_0: " << bits_timestamp_15_0 << std::endl;
    // construct the full 64-bit counter number
    unsigned long timestamp_63_0 = (static_cast<unsigned long>(timestamp_63_48) << 48) + (static_cast<unsigned long>(timestamp_47_32) << 32) + (static_cast<unsigned long>(timestamp_31_16) << 16) + (static_cast<unsigned long>(timestamp_15_0));
    // cout statement
    if (LAPPDReorderVerbosityLevel > 2)
        std::cout << "timestamp combined: " << timestamp_63_0 << std::endl;
    std::bitset<64> bits_timestamp_63_0(timestamp_63_0);
    // cout the binary
    if (LAPPDReorderVerbosityLevel > 2)
        std::cout << "bits_timestamp_63_0: " << bits_timestamp_63_0 << std::endl;

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
    unsigned int meta50_1 = (unsigned int)strtol(hexstring, NULL, 16);
    hexstring = str_timestamp_31_16.str().c_str();
    unsigned int meta30_1 = (unsigned int)strtol(hexstring, NULL, 16);
    hexstring = str_timestamp_15_0.str().c_str();
    unsigned int meta10_1 = (unsigned int)strtol(hexstring, NULL, 16);
    meta70_1 = meta70_1 << 16;
    meta30_1 = meta30_1 << 16;

    // my two beam counter values
    unsigned int trigcounter = meta30_1 + meta10_1;
    unsigned int trigcounterL = meta50_1 + meta70_1;
    // as doubles

    tcounters.push_back(beamcounter);
    tcounters.push_back(beamcounterL);
    tcounters.push_back(trigcounter);
    tcounters.push_back(trigcounterL);

    return true;
}
