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

    Trigdelay = new TH1D("trigger delay","trigger delay",2400,0.,600.);


  return true;
}

bool LAPPDReorderData::Execute(){

  // Loop over waveforms, reorder data

    vector<string> acdcmetadata;
    m_data->Stores["ANNIEEvent"]->Get("ACDCmetadata",acdcmetadata);
    std::map<unsigned long,vector<Waveform<double>>> lappddata;
    m_data->Stores["ANNIEEvent"]->Get(InputWavLabel,lappddata);
    std::map<unsigned long,vector<Waveform<double>>> reordereddata;

    int oldLaser=0;
    m_data->Stores["ANNIEEvent"]->Get("oldLaser", oldLaser);

    // Construct the relevant time-stamp/clock variables///////////////////////////////////

    //get the appropriate metadata words from the meta structure

    string sbeamgate_63_48 = acdcmetadata.at(14);   //meta data for board 1
    string sbeamgate_47_32 = acdcmetadata.at(54);   //meta data for board 1
    string sbeamgate_31_16 = acdcmetadata.at(94);   //meta data for board 1
    string sbeamgate_15_0 = acdcmetadata.at(134);   //meta data for board 1

    /*
      string sbeamgate_63_48 = acdcmetadata.at(15);   //meta data for board 2
      string sbeamgate_47_32 = acdcmetadata.at(55);   //meta data for board 2
      string sbeamgate_31_16 = acdcmetadata.at(95);   //meta data for board 2
      string sbeamgate_15_0 = acdcmetadata.at(135);   //meta data for board 2
    */

    const char *hexstring = sbeamgate_63_48.c_str();
    unsigned short beamgate_63_48 = (unsigned int)strtol(hexstring, NULL, 16);
    hexstring = sbeamgate_47_32.c_str();
    unsigned short beamgate_47_32 = (unsigned int)strtol(hexstring, NULL, 16);
    hexstring = sbeamgate_31_16.c_str();
    unsigned short beamgate_31_16 = (unsigned int)strtol(hexstring, NULL, 16);
    hexstring = sbeamgate_15_0.c_str();
    unsigned short beamgate_15_0 = (unsigned int)strtol(hexstring, NULL, 16);

    std::bitset<16> bits_beamgate_63_48(beamgate_63_48);
    std::bitset<16> bits_beamgate_47_32(beamgate_47_32);
    std::bitset<16> bits_beamgate_31_16(beamgate_31_16);
    std::bitset<16> bits_beamgate_15_0(beamgate_15_0);

    // cout statements

    if (VerbosityLevel > 2) std::cout << "bits_beamgate_63_48: " << bits_beamgate_63_48 << std::endl;
    if (VerbosityLevel > 2) std::cout << "bits_beamgate_47_32: " << bits_beamgate_47_32 << std::endl;
    if (VerbosityLevel > 2) std::cout << "bits_beamgate_31_16: " << bits_beamgate_31_16 << std::endl;
    if (VerbosityLevel > 2) std::cout << "bits_beamgate_15_0: " << bits_beamgate_15_0 << std::endl;
    //construct the full 64-bit counter number
    unsigned long beamgate_63_0 = (static_cast<unsigned long>(beamgate_63_48) << 48) + (static_cast<unsigned long>(beamgate_47_32) << 32) + (static_cast<unsigned long>(beamgate_31_16) << 16) + (static_cast<unsigned long>(beamgate_15_0));
    // cout statement
    if (VerbosityLevel > 2) std::cout << "beamgate combined: " << beamgate_63_0 << std::endl;
    // binary digit
    std::bitset<64> bits_beamgate_63_0(beamgate_63_0);
    // cout the binary
    if (VerbosityLevel > 2) std::cout << "bits_beamgate_63_0: " << bits_beamgate_63_0 << std::endl;

    // hex manipulations
    std::stringstream str_beamgate_15_0;
    str_beamgate_15_0 << std::hex << (beamgate_15_0);
    std::stringstream str_beamgate_31_16;
    str_beamgate_31_16 << std::hex << (beamgate_31_16);
    std::stringstream str_beamgate_47_32;
    str_beamgate_47_32 << std::hex << (beamgate_47_32);
    std::stringstream str_beamgate_63_48;
    str_beamgate_63_48 << std::hex << (beamgate_63_48);
    hexstring = str_beamgate_63_48.str().c_str();
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
    if (VerbosityLevel > 2) {
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
    string stimestamp_63_48 = acdcmetadata.at(140); // board 1
    string stimestamp_47_32 = acdcmetadata.at(100); // board 1
    string stimestamp_31_16 = acdcmetadata.at(60); // board 1
    string stimestamp_15_0 = acdcmetadata.at(20); // board 1

    /*
    string stimestamp_63_48 = acdcmetadata.at(141); // board 2
    string stimestamp_47_32 = acdcmetadata.at(101); // board 2
    string stimestamp_31_16 = acdcmetadata.at(61); // board 2
    string stimestamp_15_0 = acdcmetadata.at(21); // board 2
    */

    hexstring = stimestamp_63_48.c_str();
    unsigned short timestamp_63_48 = (unsigned int)strtol(hexstring, NULL, 16);
    hexstring = stimestamp_47_32.c_str();
    unsigned short timestamp_47_32 = (unsigned int)strtol(hexstring, NULL, 16);
    hexstring = stimestamp_31_16.c_str();
    unsigned short timestamp_31_16 = (unsigned int)strtol(hexstring, NULL, 16);
    hexstring = stimestamp_15_0.c_str();
    unsigned short timestamp_15_0 = (unsigned int)strtol(hexstring, NULL, 16);

    std::bitset<16> bits_timestamp_63_48(timestamp_63_48);
    std::bitset<16> bits_timestamp_47_32(timestamp_47_32);
    std::bitset<16> bits_timestamp_31_16(timestamp_31_16);
    std::bitset<16> bits_timestamp_15_0(timestamp_15_0);
    if (VerbosityLevel > 2) std::cout << "bits_timestamp_63_48: " << bits_timestamp_63_48 << std::endl;
    if (VerbosityLevel > 2) std::cout << "bits_timestamp_47_32: " << bits_timestamp_47_32 << std::endl;
    if (VerbosityLevel > 2) std::cout << "bits_timestamp_31_16: " << bits_timestamp_31_16 << std::endl;
    if (VerbosityLevel > 2) std::cout << "bits_timestamp_15_0: " << bits_timestamp_15_0 << std::endl;
    //construct the full 64-bit counter number
    unsigned long timestamp_63_0 = (static_cast<unsigned long>(timestamp_63_48) << 48) + (static_cast<unsigned long>(timestamp_47_32) << 32) + (static_cast<unsigned long>(timestamp_31_16) << 16) + (static_cast<unsigned long>(timestamp_15_0));
    // cout statement
    if (VerbosityLevel > 2) std::cout << "timestamp combined: " << timestamp_63_0 << std::endl;
    std::bitset<64> bits_timestamp_63_0(timestamp_63_0);
    // cout the binary
    if (VerbosityLevel > 2) std::cout << "bits_timestamp_63_0: " << bits_timestamp_63_0 << std::endl;

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

    m_data->Stores["ANNIEEvent"]->Set("TimingCounters",tcounters);

    Trigdelay->Fill(((double)((trigcounter-beamcounter))*3.125)/1E3);

    if (VerbosityLevel > 2) std::cout << "trigcounter: " << trigcounter << "  beamcounter: "<<beamcounter<<"  "<<((double)((trigcounter-beamcounter))*3.125)/1E3<<std::endl;


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


    if(VerbosityLevel>1){
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
    if(oldLaser==1){
      if(VerbosityLevel>1) cout<< "oldLaser!"<<endl;
      Smeta26_1 = acdcmetadata.at(52);
      Smeta26_2 = acdcmetadata.at(53);
      meta26_1 = stoi(Smeta26_1, 0, 16);
      meta26_2 = stoi(Smeta26_2, 0, 16);
    }

    map <unsigned long, vector<Waveform<double>>> :: iterator itr;

    for (itr = lappddata.begin(); itr != lappddata.end(); ++itr){
      unsigned long channelno = itr->first;
      vector<Waveform<double>> Vwavs = itr->second;


        int switchbit=0;
        if(channelno<30) switchbit = meta26_1*32;
        else switchbit = meta26_2*32;

        if(VerbosityLevel>1) cout<<"switch bit: "<<switchbit<<" "<<channelno<<endl;


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

  TFile *tf = new TFile("Tdelay.root","RECREATE");
  Trigdelay->Write();
  tf->Close();

  return true;
}
