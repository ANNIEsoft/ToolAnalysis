#include "LAPPDSim.h"

LAPPDSim::LAPPDSim():Tool(){}


bool LAPPDSim::Initialise(std::string configfile, DataModel &data){

  /////////////////// Usefull header ///////////////////////
  if(configfile!="")  m_variables.Initialise(configfile); //loading config file
  //m_variables.Print();

  m_data= &data; //assigning transient data pointer
  /////////////////////////////////////////////////////////////////

  // here I would open the input file

  // here I would also store relevant geometry information

  std::string test = "hello";
  m_data->Stores["ANNIEEvent"]= new BoostStore(false,2);
  m_data->Stores["ANNIEEvent"]->Header->Set("test",test);

  // initialize the ROOT random number generator
  myTR = new TRandom3();

  return true;
}


bool LAPPDSim::Execute(){

  std::map<int,vector<Waveform<double>>> RawLAPPDData;

  //loop over 5 channels, populate
  for(int i=0; i<5; i++){

    // fill every event with exactly two pulses at 20 samples and 70 samples (lame/boring)
    std::vector<double> pulsetimes;
    pulsetimes.push_back(20);
    pulsetimes.push_back(70);
    // make the waveform
    Waveform<double> awav = SimpleGenPulse(pulsetimes);

    // stuff the waveform into a vector of Waveforms
    vector<Waveform<double>> Vwavs;
    Vwavs.push_back(awav);

    // put the vector of Waveforms into the LAPPDData Map with a channel FindPulseMax
    RawLAPPDData.insert(pair <int,vector<Waveform<double>>> (i,Vwavs));
  }

  //put the fake LAPPD pulse into the ANNIEEvent Store, call it "LAPPDtrace"
  //m_data->Stores["ANNIEEvent"]->Set("LAPPDtrace",mwav);

  m_data->Stores["ANNIEEvent"]->Set("RawLAPPDData",RawLAPPDData);

  return true;
}


bool LAPPDSim::Finalise(){

  return true;
}


Waveform<double> LAPPDSim::SimpleGenPulse(vector<double> pulsetimes){

    int npulses = pulsetimes.size();

    // generate gaussian TF1s for each pulse
    TF1** aGauss = new TF1*[npulses];
    for(int i=0; i<npulses; i++){
      TString gname;
      gname+="gaus";
      gname+=i;
      aGauss[i] = new TF1(gname,"gaus",0,256);

      // random pulse amplitude chosen with from a gaussian distribution
      double theamp = fabs(myTR->Gaus(70.,50.)); // 50 mV mean, 50 mV sigma

      aGauss[i]->SetParameter(0,theamp); // amplitude of the pulse
      aGauss[i]->SetParameter(1,pulsetimes.at(i)); // peak location (in samples)
      aGauss[i]->SetParameter(2,10.); // width (sigma) of the pulse
    }

    // loop over 256 samples
    // generate the trace, populated with the fake pulses
    Waveform<double> thewav;
    for(int i=0; i<256; i++){

      double noise = myTR->Gaus(0.,2.0); //add in random baseline noise (2 mV sig)
      double signal = 0;

      //now add all the pulses to the signal
      for(int j=0; j<npulses; j++){
        signal+=aGauss[j]->Eval(i,0,0,0);
      }

      double thevoltage = signal+noise;
      thewav.PushSample(thevoltage);
    }

    return thewav;
}
