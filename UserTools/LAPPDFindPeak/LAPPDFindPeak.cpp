#include "LAPPDFindPeak.h"

LAPPDFindPeak::LAPPDFindPeak():Tool(){}


bool LAPPDFindPeak::Initialise(std::string configfile, DataModel &data){

  /////////////////// Usefull header ///////////////////////
  if(configfile!="")  m_variables.Initialise(configfile); //loading config file
  //m_variables.Print();

  m_data= &data; //assigning transient data pointer
  /////////////////////////////////////////////////////////////////

  return true;
}


bool LAPPDFindPeak::Execute(){

  //std::cout<<"in FindPeak"<<std::endl;
  //Waveform<double> bwav;
  //m_data->Stores["ANNIEEvent"]->Print();
  //bool testval =  m_data->Stores["ANNIEEvent"]->Get("LAPPDtrace",bwav);

  std::cout<<"In Peak Finding Tool..............................."<<std::endl;

  // get raw lappd data
  std::map<int,vector<Waveform<double>>> rawlappddata;
  bool testval =  m_data->Stores["ANNIEEvent"]->Get("RawLAPPDData",rawlappddata);

  // make reconstructed pulses
  std::map<int,vector<LAPPDPulse>> SimpleRecoLAPPDPulses;

  // loop over all of the lappd channels (number is temporarily hard coded)
  map <int, vector<Waveform<double>>> :: iterator itr;
  for (itr = rawlappddata.begin(); itr != rawlappddata.end(); ++itr){
    int channelno = itr->first;
    vector<Waveform<double>> Vwavs = itr->second;

    //loop over all Waveforms
    for(int i=0; i<Vwavs.size(); i++){

        Waveform<double> bwav = Vwavs.at(i);
        std::vector<LAPPDPulse> thepulses = FindPulses_TOT(bwav.GetSamples());
        std::cout<<"The number of peaks for channel "<<channelno
                 <<", wavform "<<i<<" is: "<<thepulses.size()<<"   ";
        for(int j=0; j<thepulses.size(); j++){
          std::cout<<"...for pulse #"<<j<<" (Q="<<(thepulses.at(j)).GetCharge()<<",LowRange="<<(thepulses.at(j)).GetLowRange()<<",HiRange="<<(thepulses.at(j)).GetHiRange()<<") ";
        }
        std::cout<<" "<<std::endl;
    }
  }

  m_data->Stores["ANNIEEvent"]->Set("SimpleRecoLAPPDPulses",SimpleRecoLAPPDPulses);

  std::cout<<"Done Finding Peaks..............................."<<std::endl;
  std::cout<<" "<<std::endl;
  //std::cout<<"did i get from store "<<testval<<std::endl;
  //int tracesize = bwav.GetSamples()->size();
  //std::cout<<"trace size: "<<tracesize<<std::endl;
/*
  double themax=0;
  int maxbin=0;

  double themin=55555.;
  int minbin = 0;

  // This function is defined in DataModel/ANNIEalgorithms.cpp
  FindPulseMax(bwav.GetSamples(),themax,maxbin,themin,minbin);
  int numpeaks = FindPulses_TOT(bwav.GetSamples());

  std::cout<<"the max value is: "<<themax<<" at sample #"<<maxbin
  <<"   the min value is: "<<themin<<" at sample #"<<minbin<<std::endl;
  std::cout<<"and the number of peaks is:  "<<numpeaks<<std::endl;

  // add the measured peak values to the Store
  m_data->Stores["ANNIEEvent"]->Set("peakmax",themax);
  m_data->Stores["ANNIEEvent"]->Set("maxbin",maxbin);
  m_data->Stores["ANNIEEvent"]->Set("peakmin",themin);
  m_data->Stores["ANNIEEvent"]->Set("minbin",minbin);
*/



  return true;
}


bool LAPPDFindPeak::Finalise(){

  return true;
}

std::vector<LAPPDPulse> LAPPDFindPeak::FindPulses_TOT(std::vector<double> *theWav) {

  std::vector<LAPPDPulse> thepulses;

  // temporarily hard coding these parameters (should be in a config)..
  double TotThreshold = 10.;
  double MinimumTot =500.;
  double Deltat=100.;

  int npeaks=0;
  int endbin=0;
  double Q=0.;
  double peak=0.;
  double low=0.;
  double hi=0.;
  TimeClass tc(0);
  double tp=0.;

  bool pulsestarted=false;
	double threshold = TotThreshold*2;
	double pvol = 0, vollast = 0;
  double ppre = 0;
	int nbin = theWav->size();
	int length = 0;
	int MinimumTotBin = (int)(MinimumTot/Deltat);
	for(int i=0;i<nbin;i++) {
		pvol = TMath::Abs(theWav->at(i));
    if(i>1) ppre = TMath::Abs(theWav->at(i-1));

		if(pvol>threshold) {
      length++;
      Q+=(theWav->at(i));
      if(!pulsestarted) low=(double)i;
      pulsestarted=true;
    }
		else {
			if(length<MinimumTotBin) {length=0; Q=0; pulsestarted=false; low=0; hi=0;}
			else {
        npeaks++; length = 0; hi=(double)i;
        LAPPDPulse apulse(0,0,tc,Q,tp,peak,low,hi);
        thepulses.push_back(apulse);
        pulsestarted=false;
      }
		}

	}
	return thepulses;
}


std::vector<LAPPDPulse> LAPPDFindPeak::FindPulses_Thresh(std::vector<double> *theWav) {

  std::vector<LAPPDPulse> thepulses;


	return thepulses;
}
