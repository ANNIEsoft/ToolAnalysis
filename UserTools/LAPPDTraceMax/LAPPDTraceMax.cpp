#include "LAPPDTraceMax.h"

LAPPDTraceMax::LAPPDTraceMax():Tool(){}

// THIS CODE INTEGRATES AN LAPPD WAVEFORM OVER A FIXED RANGE DEFINED IN THE CONFIG FILE

bool LAPPDTraceMax::Initialise(std::string configfile, DataModel &data){

  /////////////////// Usefull header ///////////////////////
  if(configfile!="")  m_variables.Initialise(configfile); //loading config file
  //m_variables.Print();

  m_data= &data; //assigning transient data pointer
  /////////////////////////////////////////////////////////////////

  // Make note that the pulse has been integrated
  bool isIntegrated = true;
  m_data->Stores["ANNIEEvent"]->Header->Set("isIntegrated",isIntegrated);

  // load in the geomety
  m_data->Stores["ANNIEEvent"]->Header->Get("AnnieGeometry", _geom);

  // Get from the configuration file the parameters of the waveform
  m_variables.Get("Nsamples", DimSize);
  m_variables.Get("SampleSize",Deltat);

  // Get from the config file, the range over which to integrate
  m_variables.Get("IntegLow",lowR);
  m_variables.Get("IntegHi",hiR);
  m_variables.Get("Nsmooth",Nsmooth);

  // Get Strip Numbers to Integrate
  //m_variables.Get("IntegStrip1",IS1);
  //m_variables.Get("IntegStrip2",IS2);
  //m_variables.Get("IntegStrip3",IS3);
  //m_variables.Get("IntegStrip4",IS4);

  // Get the Verbosity Variabl
  m_variables.Get("TMVerbosity", LAPPDTMVerbosity);

  return true;
}


bool LAPPDTraceMax::Execute(){

  // get raw lappd data
  std::map<unsigned long,vector<Waveform<double>>> lappddata;
  //bool testval =  m_data->Stores["ANNIEEvent"]->Get("BLsubtractedLAPPDData",lappddata);
  bool testval =  m_data->Stores["ANNIEEvent"]->Get("ABLSLAPPDData",lappddata);

  std::map<int, vector<vector<double>>> TML;
  std::map<int, vector<vector<double>>> TMR;

  map <unsigned long, vector<Waveform<double>>> :: iterator itr;

  double totCharge=0;
  double MaxAmp0=0;
  double MaxAmp1=0;
  for (itr = lappddata.begin(); itr != lappddata.end(); ++itr){

    unsigned long channelno = itr->first;
    vector<Waveform<double>> Vwavs = itr->second;

    Channel* mychannel = _geom->GetChannel(channelno);
    //figure out the stripline number
    int stripno = mychannel->GetStripNum();
    int stripside = mychannel->GetStripSide();

    //loop over all Waveforms
    //vector<double> acharge;
    //vector<double> acharge_sm;
    //vector<double> ampvect;
    //vector<double> ampvect_sm;

    vector<vector<double>> vastripsumL;
    vector<vector<double>> vastripsumR;

    for(int i=0; i<(int)Vwavs.size(); i++){

        Waveform<double> bwav = Vwavs.at(i);

        // find the peak and max amplitude
        vector<double> ampvect = CalcAmp(bwav,lowR,hiR);
        vector<double> ampvect_sm = CalcAmpSmoothed(bwav,lowR,hiR);

        // integrate the pulse from the low range to high range in units of mV*psec
        double Qmvpsec = CalcIntegral(bwav,lowR,hiR);
        // convert to coulomb
        double Qcoulomb = Qmvpsec/(1000.*50.*1e12);
        //convert to number of electrons
        double Qelectrons = Qcoulomb/(1.60217733e-19);

        double slowR = (ampvect_sm.at(1)*100) - 2000;
        double shiR = (ampvect_sm.at(1)*100) + 2000;
        //cout<<slowR<<" "<<shiR<<" "<<lowR<<" "<<hiR<<endl;
        double sQmvpsec = CalcIntegral(bwav,slowR,shiR);
        // convert to coulomb
        double sQcoulomb = sQmvpsec/(1000.*50.*1e12);
        //convert to number of electrons
        double sQelectrons = sQcoulomb/(1.60217733e-19);

        //cout<<"sQelectrons: "<<sQelectrons<<" "<<sQcoulomb<<" "<<sQmvpsec<<endl;

        vector<double> astripsumL;
        vector<double> astripsumR;

        if(stripside==0){
          astripsumL.push_back(ampvect.at(0));
          astripsumL.push_back(ampvect.at(1));
          astripsumL.push_back(ampvect_sm.at(0));
          astripsumL.push_back(ampvect_sm.at(1));
          astripsumL.push_back(Qelectrons);
          astripsumL.push_back(sQelectrons);
        }

        if(stripside==1){
          astripsumR.push_back(ampvect.at(0));
          astripsumR.push_back(ampvect.at(1));
          astripsumR.push_back(ampvect_sm.at(0));
          astripsumR.push_back(ampvect_sm.at(1));
          astripsumR.push_back(Qelectrons);
          astripsumR.push_back(sQelectrons);
        }

        vastripsumL.push_back(astripsumL);
        vastripsumR.push_back(astripsumR);

        //cout<<channelno<<" "<<Qelectrons<<endl;

      }


      // store the charge vector by channel
      //cout<<stripside<<" "<<stripno<<endl;
      if(stripside==0) TML.insert(pair <int,vector<vector<double>>> (stripno,vastripsumL));
      if(stripside==1) TMR.insert(pair <int,vector<vector<double>>> (stripno,vastripsumR));

    }

    // add the charge information to the Boost Store
    m_data->Stores["ANNIEEvent"]->Set("TML",TML);
    m_data->Stores["ANNIEEvent"]->Set("TMR",TMR);

    //m_data->Stores["ANNIEEvent"]->Set("theCharges",thecharge);
    //m_data->Stores["ANNIEEvent"]->Set("TotCharge",-totCharge);
    //m_data->Stores["ANNIEEvent"]->Set("MaxAmp0",MaxAmp0);
    //m_data->Stores["ANNIEEvent"]->Set("MaxAmp1",MaxAmp1);

    //if(LAPPDTMVerbosity>2) cout<<"Integ end: "<<MaxAmp0<<" "<<MaxAmp1<<" "<<totCharge<<endl;

    //cout<<"DONE INTEGRATING. TOTAL CHARGE = "<<-totCharge<<endl;

  return true;
}


bool LAPPDTraceMax::Finalise(){

  return true;
}

double LAPPDTraceMax::CalcIntegral(Waveform<double> hwav, double lowR, double hiR){

  double sT=0.; // currently hard coded

  int lowb = (lowR - sT)/Deltat;
  int hib = (hiR - sT)/Deltat;

  if(LAPPDTMVerbosity>1) cout<<"Integ: "<<lowb<<" "<<hib<<endl;

  double tQ=0.;

  if( (lowb>=0) && (hib<(int)hwav.GetSamples()->size()) ){
    for(int i=lowb; i<hib; i++){
      tQ+=((hwav.GetSample(i))*Deltat);
    }
  } else std::cout<<"OUT OF RANGE!!!!"<<std::flush;

  return tQ;
}



std::vector<double> LAPPDTraceMax::CalcAmp(Waveform<double> hwav, double lowR, double hiR){

  double sT=0.; // currently hard coded

  int lowb = (lowR - sT)/Deltat;
  int hib = (hiR - sT)/Deltat;

  double tQ=0.;
  double tAmp=0;
  double tTime=0;

  if(LAPPDTMVerbosity>1) cout<<"CalcAmp: "<<lowb<<" "<<hib<<endl;

  if( (lowb>=0) && (hib<hwav.GetSamples()->size()) ){
    for(int i=lowb; i<hib; i++){
      if(fabs(hwav.GetSample(i))>fabs(tAmp)) {
        tAmp=hwav.GetSample(i);
        tTime=(double)i;
      }
    }
  } else std::cout<<"OUT OF RANGE!!!!"<<std::flush;

  vector<double> tAmpVect;
  tAmpVect.push_back(tAmp);
  tAmpVect.push_back(tTime);

  return tAmpVect;
}


std::vector<double> LAPPDTraceMax::CalcAmpSmoothed(Waveform<double> hwav, double lowR, double hiR){

    double sT=0.; // currently hard coded

    int lowb = (lowR - sT)/Deltat;
    int hib = (hiR - sT)/Deltat;

    double tQ=0.;
    double tAmp=0;
    double tTime=0;

    if(LAPPDTMVerbosity>1) cout<<"CalcAmp: "<<lowb<<" "<<hib<<endl;

    if( ((lowb-Nsmooth)>=0) && ((hib-Nsmooth)<hwav.GetSamples()->size()) ){
      for(int i=lowb; i<hib; i++){

        double smoothsamp=0;
        for(int j=0; j<Nsmooth; j++){
          smoothsamp = hwav.GetSample(i-Nsmooth+j);
        }
        if(Nsmooth>0) smoothsamp=smoothsamp/Nsmooth;
        else cout<<"Nsmooth = 0 !!!!"<<endl;

        if(fabs(smoothsamp)>fabs(tAmp)) {
          tAmp=smoothsamp;
          tTime=(double)i - (Nsmooth/2.) ;
        }
      }
    } else std::cout<<"OUT OF RANGE!!!!"<<std::flush;

    vector<double> tAmpVect;
    tAmpVect.push_back(tAmp);
    tAmpVect.push_back(tTime);

    return tAmpVect;
}


double LAPPDTraceMax::CalcIntegralSmoothed(Waveform<double> hwav, double lowR, double hiR){

  double sT=0.; // currently hard coded

  int lowb = (lowR - sT)/Deltat;
  int hib = (hiR - sT)/Deltat;

  if(LAPPDTMVerbosity>1) cout<<"Integ: "<<lowb<<" "<<hib<<endl;

  double tQ=0.;

  if( (lowb>=0) && (hib<(int)hwav.GetSamples()->size()) ){
    for(int i=lowb; i<hib; i++){
      tQ+=((hwav.GetSample(i))*Deltat);
    }
  } else std::cout<<"OUT OF RANGE!!!!";

  return tQ;
}
