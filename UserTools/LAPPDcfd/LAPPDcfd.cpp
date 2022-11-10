#include "LAPPDcfd.h"

LAPPDcfd::LAPPDcfd():Tool(){}


bool LAPPDcfd::Initialise(std::string configfile, DataModel &data){

  /////////////////// Usefull header ///////////////////////
  if(configfile!="")  m_variables.Initialise(configfile); //loading config file
  //m_variables.Print();

  m_data= &data; //assigning transient data pointer
  /////////////////////////////////////////////////////////////////

  TString FCIWL;
  m_variables.Get("FiltCFDInputWavLabel",FCIWL);
  FiltCFDInputWavLabel = FCIWL;
  TString RCIWL;
  m_variables.Get("RawCFDInputWavLabel",RCIWL);
  RawCFDInputWavLabel = RCIWL;
  TString BLSCIWL;
  m_variables.Get("BLSCFDInputWavLabel",BLSCIWL);
  BLSCFDInputWavLabel = BLSCIWL;
  m_variables.Get("CFDVerbosity",CFDVerbosity);

  //cout<<"INITIALIZING CFD "<<CFDInputWavLabel<<endl;

  // Get the CFD threshold from the config file
  m_variables.Get("Fraction_CFD", Fraction_CFD);
  //std::cout<<"Fraction_CFD="<<Fraction_CFD<<std::endl;

  isSim=false;
  // Check in the Boost Store whether this is a simulated event or not
  m_data->Stores["ANNIEEvent"]->Header->Get("isSim",isSim);

  return true;
}


bool LAPPDcfd::Execute(){
  bool isCFD=true;
  m_data->Stores["ANNIEEvent"]->Set("isCFD",isCFD);
  bool isBLsub;
  m_data->Stores["ANNIEEvent"]->Get("isBLsubtracted",isBLsub);
  bool isFiltered;
  m_data->Stores["ANNIEEvent"]->Get("isFiltered",isFiltered);
  Waveform<double> bwav;

  //cout<<"in execute "<<CFDInputWavLabel<<endl;

  // get raw lappd data from the Boost Store
  std::map<unsigned long,vector<Waveform<double>>> lappddata;
    bool testval;
    if(isBLsub==false && isFiltered==false){
       testval = m_data->Stores["ANNIEEvent"]->Get(RawCFDInputWavLabel,lappddata);
    }
    else if(isBLsub==true && isFiltered==false){
        testval = m_data->Stores["ANNIEEvent"]->Get(BLSCFDInputWavLabel,lappddata);
    }
    else if(isFiltered==true){
        testval = m_data->Stores["ANNIEEvent"]->Get(FiltCFDInputWavLabel,lappddata);
        //cout<<"Im inside the if statement cfd"<<endl;
    }

  //cout<<CFDInputWavLabel<<" here 0: "<<lappddata.size()<<endl;

  // get first-level pulse reco from the Boost Store
  std::map<unsigned long,vector<LAPPDPulse>> SimpleRecoLAPPDPulses;
  m_data->Stores["ANNIEEvent"]->Get("SimpleRecoLAPPDPulses",SimpleRecoLAPPDPulses);

  //cout<<"here 1"<<endl;


  // get the sim-level information

  std::map<unsigned long,vector<LAPPDHit>> lappdmchits;
  if(isSim){
    m_data->Stores["ANNIEEvent"]->Get("MCLAPPDHits",lappdmchits);
  }

  //cout<<"here now"<<endl;

  // Place to store the reconstructed pulses
  std::map<unsigned long,vector<LAPPDPulse>> CFDRecoLAPPDPulses;

  // Loop over all channels
  map<unsigned long, vector<LAPPDPulse>>:: iterator p;

  for (p = SimpleRecoLAPPDPulses.begin(); p != SimpleRecoLAPPDPulses.end(); ++p){
   // cout<<"In loop of channels"<<endl;
    // Get the channel number and a vector of pulses
    unsigned long channelno = p->first;
    vector<LAPPDPulse> Vpulses = p->second;
     // cout<<"still in loop "<<SimpleRecoLAPPDPulses.size() <<" " << channelno<<endl;
    // get the vector of waveforms correseponding to the channel
    map <unsigned long, vector<Waveform<double>>> :: iterator itr;
    itr = lappddata.find(channelno);
    //cout<<"Is it here? "<<endl;
    vector<Waveform<double>> Vwavs = itr->second;
    //cout<<"or here?"<<endl;
    if(CFDVerbosity>0){
        std::cout<<"************************************************"<<std::endl;
        std::cout<<"IN LAPPDCFD:: channel: "<<channelno<<std::endl;
    }

    if(isSim){
      // If the data is simulated data, we loop over the true hits
      // and print them to screen
/*    std::cout<<"simulated pulse: ";
      map<unsigned long, vector<LAPPDHit>>::iterator p;
      p = lappdmchits.find(0);
      vector<LAPPDHit> Vhits = p->second;

      std::cout<<Vhits.size()<<" simulated pulses. ";
      for(int np=0; np<Vhits.size(); np++){
        std::cout<<"p"<<np<<"=(t="<<(Vhits.at(np)).GetTpsec()<<") ";
      }
      std::cout<<" "<<std::endl;
*/
    }

    std::vector<LAPPDPulse> thepulses;

    //loop over all Waveforms
    for(int i=0; i<(int)Vwavs.size(); i++){

        Waveform<double> bwav = Vwavs.at(i);

        //std::cout<<"reconstructed pulses: "<<std::endl;;

        // loop over all candidate pulses on each waveform, as determined by the LAPPDFindPeak Tool
        for(int j=0; j<(int)Vpulses.size(); j++){

          // for each pulse on the Waveform find the time using CFD1 algorithm
          double cfdtime = CFD_Discriminator1(bwav.GetSamples(),Vpulses.at(j));
          if(CFDVerbosity>0){
              std::cout<<"for pulse #"<<j<<" (Q="<<(Vpulses.at(j)).GetCharge()<<",Amp="<<(Vpulses.at(j)).GetPeak()<<",LowRange="<<(Vpulses.at(j)).GetLowRange()<<",HiRange="<<(Vpulses.at(j)).GetHiRange()<<") "<<"  cfd_time="<<cfdtime<<std::endl;
          }
          // Store the reconstructed time in a new LAPPDPulse
          LAPPDPulse apulse(0,channelno,(cfdtime/1000.),(Vpulses.at(j)).GetCharge(),(Vpulses.at(j)).GetPeak(),(Vpulses.at(j)).GetLowRange(),(Vpulses.at(j)).GetHiRange());
          thepulses.push_back(apulse);
        }
      }

        //std::cout<<" "<<std::endl;
        // Put the newly reconsructed LAPPDPulses into a map, by channel
        CFDRecoLAPPDPulses.insert(pair <unsigned long,vector<LAPPDPulse>> (channelno,thepulses));
    }
    //cout<<"There are "<< CFDRecoLAPPDPulses.size()<< " Pulses" << endl;
    // Add the CFD reconstructed information to the Boost Store
    m_data->Stores["ANNIEEvent"]->Set("CFDRecoLAPPDPulses",CFDRecoLAPPDPulses);

    //cout<<"gGJDKLJ"<<endl;

  return true;
}


bool LAPPDcfd::Finalise(){

  return true;
}


double	LAPPDcfd::CFD_Discriminator1(std::vector<double>* trace, LAPPDPulse pulse) {

  double deltaT;
  m_data->Stores["ANNIEEvent"]->Get("deltaT",deltaT);

  double amp = pulse.GetPeak();
  double time = 0;
  // this should be a global variable that gets set from a config...
  double th = Fraction_CFD * amp;
  double eps = 1e-2;
  double FitWindow_min = (pulse.GetLowRange()-5)*100.;
  double FitWindow_max = (pulse.GetHiRange()+5)*100.;

  if(CFDVerbosity>0) cout<<"Low Range: "<<pulse.GetLowRange()<<" Hi Range: "<<pulse.GetHiRange()<<" FitWindow_min: "<<FitWindow_min<<" FitWindow_max: "<<FitWindow_max<<endl;

  double PointsPerSpline = 5;

  // this is kludgy...I'm converting the trace to a histo
  // (many parameters are hard coded)
  int nbins = trace->size();
  double starttime=0.;
  double endtime = starttime + ((double)nbins)*100.;
  TH1D* hwav = new TH1D("hwav","hwav",nbins,starttime,endtime);
  TH1D* hwav_range = new TH1D("hwav_range","hwav_range",nbins,starttime,endtime);
  for(int i=0; i<nbins; i++){
    hwav->SetBinContent(i+1,-trace->at(i));
    if( (((double)i*100.)>FitWindow_min) && (((double)i*100.)<FitWindow_max) ) hwav_range->SetBinContent(i+1,-trace->at(i));
    else hwav_range->SetBinContent(i+1,0.);
  }

//	int bin = pulse.GetTpsec();
//		while(fsplinewav->V(hwav->GetBinCenter(bin))<=th) {bin--;}
	double xlow = FitWindow_min;
  //double xhigh = hwav->GetBinCenter(bin);
  double xhigh = hwav->GetBinCenter(hwav->GetMaximumBin());
  //double xhigh = FitWindow_max;
	double xmid = (xlow+xhigh)/2;

  if(hwav->Interpolate(xmid)-th==0) time = xmid;
  //std::cout<<"^^^ "<<xhigh<<" "<<xlow<<" "<<th<<" "<<hwav->Interpolate(xmid)<<std::endl;

  // gradually moving the high and low range of the search towards the point where the
  // pulse crosses the threshold. Stop when the low and high range are closer than eps
  while ((xhigh-xlow) >= eps ){

    xmid = (xlow + xhigh)/2.;
    if(hwav->Interpolate(xmid)-th==0) time = xmid;
    if ((hwav->Interpolate(xmid)-th) > 0)
    xhigh = xmid;
    else
    xlow = xmid;
  }

  time = xlow;
  delete hwav;
  delete hwav_range;
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
