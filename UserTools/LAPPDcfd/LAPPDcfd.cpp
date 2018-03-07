#include "LAPPDcfd.h"

LAPPDcfd::LAPPDcfd():Tool(){}


bool LAPPDcfd::Initialise(std::string configfile, DataModel &data){

  /////////////////// Usefull header ///////////////////////
  if(configfile!="")  m_variables.Initialise(configfile); //loading config file
  //m_variables.Print();

  m_data= &data; //assigning transient data pointer
  /////////////////////////////////////////////////////////////////

  return true;
}


bool LAPPDcfd::Execute(){

  std::cout<<"In CFD"<<std::endl;
  Waveform<double> bwav;

  // get raw lappd data
  std::map<int,vector<Waveform<double>>> rawlappddata;
  bool testval =  m_data->Stores["ANNIEEvent"]->Get("RawLAPPDData",rawlappddata);

  // get first-level pulse reco
  std::map<int,vector<LAPPDPulse>> SimpleRecoLAPPDPulses;
  m_data->Stores["ANNIEEvent"]->Set("SimpleRecoLAPPDPulses",SimpleRecoLAPPDPulses);


  map <int, vector<Waveform<double>>> :: iterator itr;
  for (itr = rawlappddata.begin(); itr != rawlappddata.end(); ++itr){
    int channelno = itr->first;
    vector<Waveform<double>> Vwavs = itr->second;
    // get the vector of pulses correseponding to the wavs
    vector<LAPPDPulse> Vpulses = SimpleRecoLAPPDPulses.find(channelno)->second;

    //loop over all Waveforms
    for(int i=0; i<Vwavs.size(); i++){

        Waveform<double> bwav = Vwavs.at(i);

        // loop over all candidate pulses on each waveform
        for(int j=0; j<Vpulses.size(); j++){

          CFD_Discriminator1(bwav.GetSamples(),Vpulses.at(j));


        }


      }
        std::cout<<" "<<std::endl;
    }




  return true;
}


bool LAPPDcfd::Finalise(){

  return true;
}


double	LAPPDcfd::CFD_Discriminator1(std::vector<double>* trace, LAPPDPulse pulse) {

  double amp = pulse.GetPeak();
  double time = 0;
  // this should be a global variable that gets set from a config...
  double Fraction_CFD = 0.5;
  double th = Fraction_CFD * amp;
  double FitWindow_min = pulse.GetLowRange()-5;
  double FitWindow_max = pulse.GetHiRange()+5;

  double PointsPerSpline = 5;

  // this is kludgy...I'm converting the trace to a histo
  // (many parameters are hard coded)
  int nbins = trace->size();
  double starttime=0.;
  double endtime = starttime + ((double)nbins)*100.;
  TH1D* hwav = new TH1D("temptrace","temptrace",nbins,starttime,endtime);

  for(int i=0; i<nbins; i++){
    hwav->SetBinContent(i+1,-trace->at(i));
  }
  TSplineFit* fsplinewav = new TSplineFit("SplineName", "SplineTitle", 20, PointsPerSpline, hwav, FitWindow_min, FitWindow_max);
//TSplineFit* fsplinewav = new TSplineFit("thespline", "thespline", 20, PointsPerSpline, &trace[0], &trace[0], FitWindow_min, FitWindow_max);

//		double th = -40;
		double eps = 1e-4;
		int bin = pulse.GetTpsec();
//		while(fsplinewav->V(hwav->GetBinCenter(bin))<=th) {bin--;}
		double xlow = FitWindow_min;
		double xhigh = hwav->GetBinCenter(bin);

		double xmid = (xlow+xhigh)/2;

/*
  	if(fsplinewav->V(xmid)-th==0) time = xmid;

		while ((xhigh-xlow) >= eps) {
			xmid = (xlow + xhigh) / 2;
			if (fsplinewav->V(xmid)-th == 0)
				time = xmid;
			if ((fsplinewav->V(xlow)-th)*(fsplinewav->V(xmid)-th) < 0)
				xhigh = xmid;
			else
				xlow = xmid;
		}
		time = xlow;
	}
  */
	return time;
}


double LAPPDcfd::CFD_Discriminator2(std::vector<double>* trace, LAPPDPulse pulse){

  double time=0;

//  sprintf(CFDSplineName, "CFDSpline_ch%d_%d", chno, evtno);
//  sprintf(CFDSplineTitle, "CFDSpline | CFDSpline_ch%d_%d", chno, evtno);


//  if(amp <= AmpThreshold || npeaks==0) { time = 0; } //TDC threshold
//	else {
//    if(IfDynamicWindow == 1) Calculate_fitrange();
/*

  int Delay_CFDBin = (int)(Delay_CFD/Deltat);
  for(int i=0;i<DimSize;i++) {hcfd->SetBinContent(i+1, hwav->GetBinContent(i+1));} //copy
  TH1D *hinv = new TH1D("hinv","hinv",DimSize,0,DimSize*Deltat);
  for(int i=0;i<DimSize;i++) {
  		if(i+Delay_CFDBin<DimSize) hinv->SetBinContent(i+1, hwav->GetBinContent(i+1+Delay_CFDBin)); //copy and delay
  		else hinv->SetBinContent(i+1,0);
  }
  hinv->Scale(-1. * Fraction_CFD); //inverse and attenuate
  hcfd->Add(hinv);
  fsplinecfd = new TSplineFit(CFDSplineName,CFDSplineTitle,50,PointsPerSpline, hcfd,FitWindow_min, FitWindow_max);
  //		fsplinecfd->UpdateFile(true); //SplineFit database:  Skip this line for fast analysis
  fsplinecfd->ReduceMemory();

  //solve equation
  double eps = 1e-4;
  double xhigh = hcfd->GetMinimumBin()*Deltat;
  //		double xlow = hcfd->GetMaximumBin()*Deltat;
  double xlow = FitWindow_min;
  //		cout<<xlow<<"\t"<<xhigh<<endl;
  double xmid = (xlow+xhigh)/2;

  if(fsplinecfd->V(xmid)==0) time = xmid;

  while ((xhigh-xlow) >= eps) {
  	xmid = (xlow + xhigh) / 2;
  	if (fsplinecfd->V(xmid) == 0)
  			time =  xmid;
  		if (fsplinecfd->V(xlow)*fsplinecfd->V(xmid) < 0)
  			xhigh = xmid;
  		else
  			xlow = xmid;
  	}
  	time = xlow;
  	delete hinv;
  	hinv = NULL;
  }
  */
  return time;
}
