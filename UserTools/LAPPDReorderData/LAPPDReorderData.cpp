#include "LAPPDReorderData.h"

LAPPDReorderData::LAPPDReorderData():Tool(){}


bool LAPPDReorderData::Initialise(std::string configfile, DataModel &data){

  /////////////////// Useful header ///////////////////////
  if(configfile!="") m_variables.Initialise(configfile); // loading config file
  //m_variables.Print();

  m_data= &data; //assigning transient data pointer
  /////////////////////////////////////////////////////////////////

    std::string IWL,OWL;
    m_variables.Get("ReorderInputWavLabel",IWL);
    InputWavLabel = IWL;
    m_variables.Get("ReorderOutputWavLabel",OWL);
    OutputWavLabel = OWL;

    m_variables.Get("DelayOffset",delayoffset);
    m_variables.Get("GlobalShift",globalshift);
    m_variables.Get("ReorderVerbosityLevel",VerbosityLevel);



  return true;
}

bool LAPPDReorderData::Execute(){

    vector<string> acdcmetadata;
    m_data->Stores["ANNIEEvent"]->Get("ACDCmetadata",acdcmetadata);
    std::map<unsigned long,vector<Waveform<double>>> lappddata;
    m_data->Stores["ANNIEEvent"]->Get(InputWavLabel,lappddata);
    std::map<unsigned long,vector<Waveform<double>>> reordereddata;

    // Loop over waveforms, reorder data

    // For 30 channels change to 10
    string Smeta26_1 = acdcmetadata.at(20);
    string Smeta26_2 = acdcmetadata.at(21);
    
    // Reading the hexadecimal metadata then bitmask to get the
    // last 3 bit for PSEC0 timestamp [15:0]  
    // https://github.com/lappd-daq/acdc-daq-revc

    //In order to bitmask the last 3 bit, we use the below
    std::bitset<16> bs1("0000000000000111");

    const char *hexstring_1 = Smeta26_1.c_str();
    int meta26_1_0 = (int)strtol(hexstring_1, NULL, 16);

    const char *hexstring_2 = Smeta26_2.c_str();
    int meta26_2_0 = (int)strtol(hexstring_2, NULL, 16);

    std::string to_binary_1 = std::bitset<16>(meta26_1_0).to_string();
    std::bitset<16> bs2_1(to_binary_1);

    std::string to_binary_2 = std::bitset<16>(meta26_2_0).to_string();
    std::bitset<16> bs2_2(to_binary_2);

    int meta26_1 = std::bitset<16>((bs1 & bs2_1)).to_ulong();
    int meta26_2 = std::bitset<16>((bs1 & bs2_2)).to_ulong();

    
    if(VerbosityLevel>0){
          cout<<"REORDER TIME!!!!   "<<acdcmetadata.size()<<" "<<Smeta26_1<<" "<<Smeta26_2<<" "<<meta26_1_0<<" "<<meta26_2<<endl;
	  cout << "bs1           : " << bs1 << endl;
	  cout <<"------------------------------"<<endl;
	  cout << "bs2_1         : " << bs2_1 << endl;
	  cout << "bs1 AND bs2_1 : " << (bs1 & bs2_1) << endl;
	  std::string binary2_1 = std::bitset<16>((bs1 & bs2_1)).to_string();
	  cout<< "string 1 :"<< binary2_1 <<" decimal: " << meta26_1 << endl;
	  cout <<"------------------------------"<<endl;
	  cout << "bs2_2         : " << bs2_2 << endl;
	  cout << "bs1 AND bs2_2 : " << (bs1 & bs2_2) << endl;
	  std::string binary2_2 = std::bitset<16>((bs1 & bs2_2)).to_string();
	  cout<< "string 2 :"<< binary2_2 <<" decimal: " << meta26_2 << endl;
	  cout <<"------------------------------------------------------------"<<endl;
    }
    
    //cout<<"REORDER TIME!!!!   "<<acdcmetadata.size()<<" "<<acdcmetadata.at(52)<<" "<<acdcmetadata.at(53)<<" "<<meta26_1<<" "<<meta26_2<<endl;

    map <unsigned long, vector<Waveform<double>>> :: iterator itr;

    for (itr = lappddata.begin(); itr != lappddata.end(); ++itr){
      unsigned long channelno = itr->first;
      vector<Waveform<double>> Vwavs = itr->second;


        int switchbit=0;
        if(channelno<30) switchbit = meta26_1*32;
        else switchbit = meta26_2*32;

        switchbit+=delayoffset;

        // cout<<"channel= "<<channelno<<endl;

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

        reordereddata.insert(pair<unsigned long, vector<Waveform<double>>>(1000+channelno,Vrwav));

    }


    m_data->Stores["ANNIEEvent"]->Set(OutputWavLabel,reordereddata);


  return true;
}


bool LAPPDReorderData::Finalise(){

  return true;
}
