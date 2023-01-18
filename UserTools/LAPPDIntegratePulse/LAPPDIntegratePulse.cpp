#include "LAPPDIntegratePulse.h"

LAPPDIntegratePulse::LAPPDIntegratePulse():Tool(){}

// THIS CODE INTEGRATES AN LAPPD WAVEFORM OVER A FIXED RANGE DEFINED IN THE CONFIG FILE

bool LAPPDIntegratePulse::Initialise(std::string configfile, DataModel &data){

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

  // Get Strip Numbers to Integrate
  m_variables.Get("IntegStrip1",IS1);
  m_variables.Get("IntegStrip2",IS2);
  m_variables.Get("IntegStrip3",IS3);
  m_variables.Get("IntegStrip4",IS4);

  // Get the Verbosity Variabl
  m_variables.Get("IntegVerbosity", LAPPDIntegVerbosity);

  return true;
}


bool LAPPDIntegratePulse::Execute(){

  // get raw lappd data
  std::map<unsigned long,vector<Waveform<double>>> lappddata;
  //bool testval =  m_data->Stores["ANNIEEvent"]->Get("BLsubtractedLAPPDData",lappddata);
  bool testval =  m_data->Stores["ANNIEEvent"]->Get("ABLSLAPPDData",lappddata);

  std::map<int,vector<double>> thecharge;
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
    vector<double> acharge;
    for(int i=0; i<(int)Vwavs.size(); i++){

        Waveform<double> bwav = Vwavs.at(i);

        // integrate the pulse from the low range to high range in units of mV*psec
        double Qmvpsec = CalcIntegral(bwav,lowR,hiR);
        // convert to coulomb
        double Qcoulomb = Qmvpsec/(1000.*50.*1e12);
        //convert to number of electrons
        double Qelectrons = Qcoulomb/(1.60217733e-19);

        acharge.push_back(Qelectrons);
        //cout<<channelno<<" "<<Qelectrons<<endl;

        if( stripno==IS1 || stripno==IS2 || stripno==IS3 || stripno==IS4 ){
          totCharge+=Qelectrons;
        }
        //get the maximum amplitude
        if(stripno==IS1&&stripside==0) MaxAmp0 = CalcAmp(bwav,lowR,hiR);
        if(stripno==IS1&&stripside==1) MaxAmp1 = CalcAmp(bwav,lowR,hiR);
      }

      // store the charge vector by channel
      thecharge.insert(pair <unsigned long,vector<double>> (channelno,acharge));
    }

    // add the charge information to the Boost Store
    m_data->Stores["ANNIEEvent"]->Set("theCharges",thecharge);
    m_data->Stores["ANNIEEvent"]->Set("TotCharge",-totCharge);
    m_data->Stores["ANNIEEvent"]->Set("MaxAmp0",MaxAmp0);
    m_data->Stores["ANNIEEvent"]->Set("MaxAmp1",MaxAmp1);

    if(LAPPDIntegVerbosity>2) cout<<"Integ end: "<<MaxAmp0<<" "<<MaxAmp1<<" "<<totCharge<<endl;

    //cout<<"DONE INTEGRATING. TOTAL CHARGE = "<<-totCharge<<endl;

  return true;
}


bool LAPPDIntegratePulse::Finalise(){

  return true;
}

double LAPPDIntegratePulse::CalcIntegral(Waveform<double> hwav, double lowR, double hiR){

  double sT=0.; // currently hard coded

  int lowb = (lowR - sT)/Deltat;
  int hib = (hiR - sT)/Deltat;

  if(LAPPDIntegVerbosity>1) cout<<"Integ: "<<lowb<<" "<<hib<<endl;

  double tQ=0.;

  if( (lowb>=0) && (hib<(int)hwav.GetSamples()->size()) ){
    for(int i=lowb; i<hib; i++){
      tQ+=((hwav.GetSample(i))*Deltat);
    }
  } else std::cout<<"OUT OF RANGE!!!!";

  return tQ;
}



double LAPPDIntegratePulse::CalcAmp(Waveform<double> hwav, double lowR, double hiR){

  double sT=0.; // currently hard coded

  int lowb = (lowR - sT)/Deltat;
  int hib = (hiR - sT)/Deltat;

  double tQ=0.;
  double tAmp=0;

  if(LAPPDIntegVerbosity>1) cout<<"Integ: "<<lowb<<" "<<hib<<endl;

  if( (lowb>=0) && (hib<hwav.GetSamples()->size()) ){
    for(int i=lowb; i<hib; i++){
      if(fabs(hwav.GetSample(i))>fabs(tAmp)) tAmp=hwav.GetSample(i);
    }
  } else std::cout<<"OUT OF RANGE!!!!";

  return tAmp;
}
