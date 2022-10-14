#include "LAPPDFindPeak.h"

LAPPDFindPeak::LAPPDFindPeak():Tool(){}


bool LAPPDFindPeak::Initialise(std::string configfile, DataModel &data){

  /////////////////// Usefull header ///////////////////////
  if(configfile!="")  m_variables.Initialise(configfile); //loading config file
  //m_variables.Print();

  m_data= &data; //assigning transient data pointer
  /////////////////////////////////////////////////////////////////

  m_data->Stores["ANNIEEvent"]->Header->Get("AnnieGeometry", _geom);

  TString FPIWL;
  m_variables.Get("FiltPeakInputWavLabel",FPIWL);
  FiltPeakInputWavLabel = FPIWL;
  TString RPIWL;
  m_variables.Get("RawPeakInputWavLabel",RPIWL);
  RawPeakInputWavLabel = RPIWL;
  TString BLSPIWL;
  m_variables.Get("BLSPeakInputWavLabel",BLSPIWL);
  BLSPeakInputWavLabel = BLSPIWL;
  m_variables.Get("TotThreshold", TotThreshold);
  m_variables.Get("MinimumTot", MinimumTot);
  m_variables.Get("Deltat", Deltat);
  m_variables.Get("FindPeakVerbosity",FindPeakVerbosity);

  cout<<"BLSLabel (findpeak): "<<BLSPeakInputWavLabel<<endl;

  return true;
}


bool LAPPDFindPeak::Execute(){

    if(FindPeakVerbosity>0){ std::cout<<"in FindPeak...Begin"<<std::endl; }
    bool isBLsub;
    m_data->Stores["ANNIEEvent"]->Get("isBLsubtracted",isBLsub);
    bool isFiltered;
    m_data->Stores["ANNIEEvent"]->Get("isFiltered",isFiltered);
    //cout<<isBLsub<<" Please Work "<<isFiltered<<endl;
  //Waveform<double> bwav;
  //m_data->Stores["ANNIEEvent"]->Print();
  //bool testval =  m_data->Stores["ANNIEEvent"]->Get("LAPPDtrace",bwav);

  //std::cout<<"In Peak Finding Tool..............................."<<std::endl;

  // get raw lappd data
    std::map<unsigned long,vector<Waveform<double>>> lappddata;
    if(isBLsub==false && isFiltered==false){
        m_data->Stores["ANNIEEvent"]->Get(RawPeakInputWavLabel,lappddata);
        if(FindPeakVerbosity>0) cout<<"Getting "<<RawPeakInputWavLabel<<endl;
    }
    else if(isBLsub==true && isFiltered==false){
        m_data->Stores["ANNIEEvent"]->Get(BLSPeakInputWavLabel,lappddata);
        if(FindPeakVerbosity>0) cout<<"Getting "<<BLSPeakInputWavLabel<<endl;
    }
    else if(isFiltered==true){
        m_data->Stores["ANNIEEvent"]->Get(FiltPeakInputWavLabel,lappddata);
        if(FindPeakVerbosity>0) cout<<"Getting "<<FiltPeakInputWavLabel<<endl;
    }
  //bool testval =  m_data->Stores["ANNIEEvent"]->Get("RawLAPPDData",lappddata);
  //cout<<FiltPeakInputWavLabel<<" HElp "<<lappddata.size()<<endl;


  //cout<<"Channel Number: "<<5<<" is Strip number: "<<endl;

  // make reconstructed pulses
  std::map<unsigned long,vector<LAPPDPulse>> SimpleRecoLAPPDPulses;

  map <unsigned long, vector<Waveform<double>>> :: iterator itr;

  for (itr = lappddata.begin(); itr != lappddata.end(); ++itr){
    unsigned long channelno = itr->first;
    vector<Waveform<double>> Vwavs = itr->second;

    Channel* mychannel= _geom->GetChannel(channelno);
    int stripno = mychannel->GetStripNum();

    //if(channelno>1037 && channelno<1040) cout<<"channel= "<<channelno<<" "<<stripno<<endl;

    //loop over all Waveforms
    std::vector<LAPPDPulse> thepulses;
    for(int i=0; i < (int) Vwavs.size(); i++){

        Waveform<double> bwav = Vwavs.at(i);
        thepulses = FindPulses_TOT(bwav.GetSamples());


        if(FindPeakVerbosity>0){
            if(FindPeakVerbosity>1) std::cout<<"The number of peaks for channel "<<channelno
                    <<", stripnum: "<< stripno <<", wavform "<<i<<" is: "<<thepulses.size()<<"   ";
            for(int j=0; j<thepulses.size(); j++){
                std::cout<<"...for pulse #"<<j<<" (Q="<<(thepulses.at(j)).GetCharge()<<",LowRange="<<(thepulses.at(j)).GetLowRange()<<",HiRange="<<(thepulses.at(j)).GetHiRange()<<",time="<<(thepulses.at(j)).GetTime()<<") "<<std::endl;
            }
            if(FindPeakVerbosity>1) std::cout<<" "<<std::endl;
        }
    }
    if(thepulses.size()>0)
      {
          //cout<<channelno<<" Channel with pulses in FindPeak"<<endl;
        SimpleRecoLAPPDPulses.insert(pair <unsigned long,vector<LAPPDPulse>> (channelno,thepulses));
      }
  }
  m_data->Stores["ANNIEEvent"]->Set("SimpleRecoLAPPDPulses",SimpleRecoLAPPDPulses);

  if(FindPeakVerbosity>0){
      cout<<SimpleRecoLAPPDPulses.size()<<" In LAPPDFINDPEAK"<<endl;
      std::cout<<"Done Finding Peaks..............................."<<std::endl;
      //std::cout<<" "<<std::endl;
      //std::cout<<"did i get from store "<<testval<<std::endl;
      //int tracesize = bwav.GetSamples()->size();
      //std::cout<<"trace size: "<<tracesize<<std::endl;
  }
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

  int npeaks=0;
  int endbin=0;
  int peakbin=0;
  double Q=0.;
  double peak=0.;
  double low=0.;
  double hi=0.;
  double tc=0;
  bool pulsestarted=false;
  double threshold = TotThreshold;
  double pvol = 0, vollast = 0;
  double ppre = 0;
  int nbin = theWav->size();
  int length = 0;
  int MinimumTotBin = (int)(MinimumTot/Deltat);
  //std::cout<<"Min Tot Bin: "<<MinimumTotBin<<std::endl;

  if(FindPeakVerbosity>1) cout<<"findpulsesTOT parameters: "<<TotThreshold<<" "<<MinimumTotBin<<" "<<nbin<<endl;

	for(int i=0;i<nbin;i++) {
		pvol = TMath::Abs(theWav->at(i));
        if(i>1) ppre = TMath::Abs(theWav->at(i-1));
    if(FindPeakVerbosity>5) cout<<theWav->at(i)<<endl;

		if(pvol>threshold) {
            if(FindPeakVerbosity>4) cout<<theWav->at(i)<<endl;
            length++;
            if(pvol>peak) {peak = pvol; peakbin=i;}
            Q+=(((theWav->at(i))/50000.)*(1e-10));
            if(!pulsestarted) low=(double)i;
            pulsestarted=true;
        }
		else {
            //if(length>0){cout<<length<<" "<<MinimumTotBin<<endl;}
			if(length<MinimumTotBin) {length=0; Q=0; pulsestarted=false; low=0; hi=0; peak=0;}
			else {
                npeaks++; length = 0; hi=(double)i;
                tc= (low * Deltat)/1000.;
                LAPPDPulse apulse(0,peakbin,tc,Q,peak,low,hi);
                if(FindPeakVerbosity>2) cout<<"pulse parameters:  t="<<tc<<" Q="<<Q<<" peak="<<peak<<" low="<<low<<" hi="<<hi<<endl;
                thepulses.push_back(apulse);
                pulsestarted=false;
                peak=0; Q=0; low=0; hi=0;
            }
		}
	}
	return thepulses;
}


std::vector<LAPPDPulse> LAPPDFindPeak::FindPulses_Thresh(std::vector<double> *theWav) {

  std::vector<LAPPDPulse> thepulses;


	return thepulses;
}
