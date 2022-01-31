#include "LAPPDFindT0.h"
#include "TGraph.h"

LAPPDFindT0::LAPPDFindT0():Tool(){}


bool LAPPDFindT0::Initialise(std::string configfile, DataModel &data){

  /////////////////// Useful header ///////////////////////
  if(configfile!="") m_variables.Initialise(configfile); // loading config file
  //m_variables.Print();

  m_data= &data; //assigning transient data pointer
  /////////////////////////////////////////////////////////////////

  TString IWL,OWL;
  m_variables.Get("FindT0InputWavLabel",IWL);
  InputWavLabel = IWL;
  m_variables.Get("FindT0OutputWavLabel",OWL);
  OutputWavLabel = OWL;

  m_variables.Get("FindT0Verbosity",FindT0VerbosityLevel);

  m_variables.Get("TrigEarlyCut",trigearlycut);
  m_variables.Get("TrigLateCut",triglatecut);

  m_variables.Get("T0channelNo",T0channelNo);
  m_variables.Get("T0signalmax",T0signalmax);
  m_variables.Get("T0signalthreshold",T0signalthreshold);

  m_variables.Get("T0offset",T0offset);
  m_variables.Get("GlobalShiftT0",globalshiftT0);


  return true;
}


bool LAPPDFindT0::Execute(){

  std::map<unsigned long,vector<Waveform<double>>> lappddata;
  m_data->Stores["ANNIEEvent"]->Get(InputWavLabel,lappddata);
  std::map<unsigned long,vector<Waveform<double>>> reordereddata;
  bool T0signalInWindow = false;
  double deltaT;

  if(FindT0VerbosityLevel>0) cout<<"In LAPPDFindT0, T0 channel:"<< T0channelNo<<" , InputWavlabel: "<<InputWavLabel<<" , LAPPDdata.size()="<<lappddata.size()<<endl;

  map <unsigned long, vector<Waveform<double>>> :: iterator itr;

  if(FindT0VerbosityLevel>1) cout<<"is the channel there? "<<lappddata.count((unsigned long) T0channelNo)<<endl;

  itr = lappddata.find((unsigned long) T0channelNo);
  Waveform<double> bwav = (itr->second).at(0);

  double thetime = Tfit(bwav.GetSamples());
  int switchbit = (int)(thetime/100.);
  deltaT = thetime - (100.*(double)switchbit);
  int Qvar=0;
  if( (switchbit>=trigearlycut) && (switchbit<=triglatecut) ) T0signalInWindow = true;

  if(FindT0VerbosityLevel>0) cout<<"Done finding the time, switchbit:" << switchbit << " , deltaT: "<<deltaT<<" , inwindow: "<<T0signalInWindow<<endl;


  if(FindT0VerbosityLevel>1) cout<<"ready to loop"<<endl;

  for (itr = lappddata.begin(); itr != lappddata.end(); ++itr){
    unsigned long channelno = itr->first;
    vector<Waveform<double>> Vwavs = itr->second;

    vector<Waveform<double>> Vrwav;

    //loop over all Waveforms
    for(int i=0; i<Vwavs.size(); i++){

        Waveform<double> bwav = Vwavs.at(i);
        Waveform<double> rwav;
        Waveform<double> rwavShift;

        int sb = switchbit + T0offset;
        if(sb<0) sb+=255;

        for(int j=0; j< bwav.GetSamples()->size(); j++){

            if(sb>255) sb=0;
            double nsamp = bwav.GetSamples()->at(sb);
            rwav.PushSample(nsamp);
            sb++;

        }

        for(int j=0; j< rwav.GetSamples()->size(); j++){
          int ibin = j + globalshiftT0;
          if(ibin > 255) ibin = ibin - 255;
          double nsamp = rwav.GetSamples()->at(ibin);
          rwavShift.PushSample(nsamp);
        }

        Vrwav.push_back(rwavShift);
    }

    reordereddata.insert(pair<unsigned long, vector<Waveform<double>>>(channelno,Vrwav));

  }

  if(FindT0VerbosityLevel>0) cout<<"End of LAPPDFindT0..."<<endl;

  m_data->Stores["ANNIEEvent"]->Set(OutputWavLabel,reordereddata);
  m_data->Stores["ANNIEEvent"]->Set("deltaT",deltaT);
  m_data->Stores["ANNIEEvent"]->Set("T0signalInWindow",T0signalInWindow);
  m_data->Stores["ANNIEEvent"]->Set("T0Bin",switchbit);


  return true;
}


bool LAPPDFindT0::Finalise(){

  return true;
}



double LAPPDFindT0::Tfit(std::vector<double>* wf){

  int nbin = wf->size();
  double pvol,ppre;

  bool firstcross=true;
  double ttime=0;

  for(int i=0;i<nbin;i++) {
    pvol = (wf->at(i));
    if(i>1) ppre = (wf->at(i-1));

    if(FindT0VerbosityLevel==2) cout<<i<<" "<<ppre<<" "<<pvol<<" "<<T0signalmax<<endl;

    if(firstcross && pvol < T0signalmax && ppre > T0signalmax && i>5 && i<250){

      if(FindT0VerbosityLevel>1) cout<<"t0 bin: "<<i<<endl;

      TGraph *edge = new TGraph();
      for(int j=-7; j<1; j++){
        edge->SetPoint(j+7,(i+j)*100.,(wf->at(i+j)));
      }

      bool firstthreshcross=true;
      for(int k=(i-7)*100; k<(i+1)*100; k+=10){

        if(FindT0VerbosityLevel==3) cout<<k<<" "<<(edge->Eval((double)k, 0, "S"))<<endl;

        if(((edge->Eval((double)k, 0, "S"))<T0signalthreshold) && firstthreshcross){
          if(FindT0VerbosityLevel>0) cout<<"time: "<<k<<endl;
          ttime=(double)k;
          firstthreshcross=false;
        }

      }

      firstcross=false;
    }


  }

  if(FindT0VerbosityLevel>0) cout<<"Done Finding T0"<<endl;

  return ttime;
}
