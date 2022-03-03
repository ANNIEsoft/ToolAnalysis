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

    m_data->Stores["ANNIEEvent"]->Set(OutputWavLabel,reordereddata);

    return true;
}


bool LAPPDStoreReorderData::Finalise()
{
    return true;
}
