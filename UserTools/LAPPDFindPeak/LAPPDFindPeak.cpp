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

  // loop over all of the lappd channels (number is temporarily hard coded)
  map <int, vector<Waveform<double>>> :: iterator itr;
  for (itr = rawlappddata.begin(); itr != rawlappddata.end(); ++itr){
    int channelno = itr->first;
    vector<Waveform<double>> Vwavs = itr->second;

    //loop over all Waveforms
    for(int i=0; i<Vwavs.size(); i++){

        Waveform<double> bwav = Vwavs.at(i);
        int numpeaks = FindPulses_TOT(bwav.GetSamples());
        std::cout<<"The number of peaks for channel "<<channelno
                 <<", wavform "<<i<<" is:   "<<numpeaks<<std::endl;
    }
  }

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

int LAPPDFindPeak::FindPulses_TOT(std::vector<double> *theWav) {

  // temporarily hard coding these parameters (should be in a config)..
  double TotThreshold = 10.;
  double MinimumTot =500.;
  double Deltat=100.;

	int npeaks=0;
	double threshold = TotThreshold*2;
	double pvol = 0, vollast = 0;
	int nbin = theWav->size();
	int length = 0;
	int MinimumTotBin = (int)(MinimumTot/Deltat);
	for(int i=0;i<nbin;i++) {
		pvol = TMath::Abs(theWav->at(i));
		if(pvol>threshold) {length++;}
		else {
			if(length<MinimumTotBin) {length=0;}
			else {npeaks++; length = 0;}
		}

	}
	return npeaks;
}


int LAPPDFindPeak::FindPulses_Thresh(std::vector<double> *theWav) {
	int npeaks=0;

	return npeaks;
}
