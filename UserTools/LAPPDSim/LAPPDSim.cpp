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

  m_data->Stores["ANNIEEvent"]= new BoostStore(false,2);

  bool isSim = true;
  m_data->Stores["ANNIEEvent"]->Header->Set("isSim",isSim);

  // initialize the ROOT random number generator
  myTR = new TRandom3();

  m_variables.Get("SimInput", SimInput);

  iter=0;

  return true;
}


bool LAPPDSim::Execute(){

  std::map<int,vector<Waveform<double>>> RawLAPPDData;

  if(iter%100==0) cout<<"iteration: "<<iter<<endl;

  // get the MC Hits
  std::map<int,vector<LAPPDHit>> lappdmchits;
  bool testval =  m_data->Stores["ANNIEEvent"]->Get("MCLAPPDHit",lappdmchits);

  map <int, vector<LAPPDHit>> :: iterator itr;

  // loop over the number of lappds
  for (itr = lappdmchits.begin(); itr != lappdmchits.end(); ++itr){
    int tubeno = itr->first;

    vector<LAPPDHit> mchits = itr->second;

    std::vector<double> pulsetimes;
    //LAPPDresponse* response = new LAPPDresponse();  //SD
    LAPPDresponse response;

    // loop over the pulses on each lappd
    for(int j=0; j<mchits.size(); j++){
      // Here we would input these pulses into our lappd model
      // and extract the signals on each of 60 channels...
      // For now we just extract the 2 Tpsec times
      // and input them in 5 channels

      LAPPDHit ahit = mchits.at(j);
      double atime = ahit.GetTime()*1000.;
      pulsetimes.push_back(atime) ;
      vector<double> localpos = ahit.GetLocalPosition();  //SD
      double trans = localpos.at(1);         //SD
      double para = localpos.at(0);               //SD
      response.AddSinglePhotonTrace(trans, para, atime);       //SD
    }
    int ic=0;
    for(int i=-30; i<31; i++){
      if(i==0){
        continue;
      }
      Waveform<double> awav = response.GetTrace(i, 0.0, 100, 256, 1.0);
      vector<Waveform<double>> Vwavs;
      Vwavs.push_back(awav);
      RawLAPPDData.insert(pair <int,vector<Waveform<double>>> (ic,Vwavs));
      ic++;
    }
    //loop over 5 channels, populate with pulses
    //this part of the code is totally made up
    //as a place holder
    //  for(int i=0; i<5; i++){

      // make the waveform
      //    Waveform<double> awav = SimpleGenPulse(pulsetimes);

      // stuff the waveform into a vector of Waveforms
      // vector<Waveform<double>> Vwavs;
      // Vwavs.push_back(awav);
      //    RawLAPPDData.insert(pair <int,vector<Waveform<double>>> (i,Vwavs));
      //}

    // put the vector of Waveforms into the LAPPDData Map with a channel FindPulseMax

  }
  //put the fake LAPPD pulse into the ANNIEEvent Store, call it "LAPPDtrace"
  //m_data->Stores["ANNIEEvent"]->Set("LAPPDtrace",mwav);

  m_data->Stores["ANNIEEvent"]->Set("RawLAPPDData",RawLAPPDData);

  iter++;
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
      double theamp = fabs(myTR->Gaus(30.,30.)); // 30 mV mean, 30 mV sigma

      aGauss[i]->SetParameter(0,theamp); // amplitude of the pulse
      aGauss[i]->SetParameter(1,pulsetimes.at(i)); // peak location (in samples)
      aGauss[i]->SetParameter(2,8.); // width (sigma) of the pulse
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
      thewav.PushSample(-thevoltage);
    }

    return thewav;
}
