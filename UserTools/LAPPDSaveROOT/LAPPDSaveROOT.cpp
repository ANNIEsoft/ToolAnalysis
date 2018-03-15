#include "LAPPDSaveROOT.h"

LAPPDSaveROOT::LAPPDSaveROOT():Tool(){}


bool LAPPDSaveROOT::Initialise(std::string configfile, DataModel &data){

  /////////////////// Usefull header ///////////////////////
  if(configfile!="")  m_variables.Initialise(configfile); //loading config file
  //m_variables.Print();

  m_data= &data; //assigning transient data pointer
  /////////////////////////////////////////////////////////////////

  // setup the output files
  TString OutFile = "lappdout.root";	 //default input file
  m_variables.Get("outfile", OutFile);
  tf = new TFile(OutFile,"RECREATE");
  tf->mkdir("wavs");
  tf->mkdir("filteredwavs");

  // have the pulses been filtered? integrated? is this real or simulated data?
  m_data->Stores["ANNIEEvent"]->Header->Get("isFiltered",isFiltered);
  m_data->Stores["ANNIEEvent"]->Header->Get("isIntegrated",isIntegrated);
  m_data->Stores["ANNIEEvent"]->Header->Get("isIntegrated",isSim);

  // from the config file, get the number of channels, the trigger channel, num histos, samplesize
  m_variables.Get("NChannels", NChannel);
  m_variables.Get("TrigChannel", TrigChannel);
  m_variables.Get("NHistos", NHistos);
  m_variables.Get("SampleSize",Deltat);

  // keep count of the loop number (starting from 0)
  miter=0;

  // set up the branches of the ROOT tree
  outtree = new TTree("LAPPDdata","LAPPDdata");
  outtree->Branch("iteration",&miter);
  outtree->Branch("channel",&chno);
  outtree->Branch("cfdtime",&cfdtime);
  outtree->Branch("amplitude",&amp);
  outtree->Branch("Twidth",&twidth);

  // declare the histograms
  hAmp = new TH1D*[NChannel];
  hTime = new TH1D*[NChannel];
  hQ = new TH1D*[NChannel];

  // initialize the histograms
  for(int i=0; i<NChannel; i++){
    TString AmpName;
    AmpName+="Amplitudes_CH";
    AmpName+=i;
    hAmp[i] = new TH1D(AmpName,AmpName,1000,0.,50.);

    TString QName;
    QName+="Charge_CH";
    QName+=i;
    hQ[i] = new TH1D(QName,QName,4400,-1e7,10e7);

    TString TimeName;
    TimeName+="Time_CH";
    TimeName+=i;
    hTime[i] = new TH1D(TimeName,TimeName,10000,0.,100000.);
  }

  return true;
}


bool LAPPDSaveROOT::Execute(){

  // get raw lappd data
  std::map<int,vector<Waveform<double>>> rawlappddata;
  m_data->Stores["ANNIEEvent"]->Get("RawLAPPDData",rawlappddata);
  // get filtered data
  std::map<int,vector<Waveform<double>>> filteredlappddata;
  if(isFiltered) m_data->Stores["ANNIEEvent"]->Get("FiltLAPPDData",filteredlappddata);
  // get charge information
  std::map<int, vector<double>> TheCharges;
  if(isIntegrated) m_data->Stores["ANNIEEvent"]->Get("theCharges",TheCharges);
  // get reconstructed LAPPD pulses
  std::map<int,vector<LAPPDPulse>> CFDRecoLAPPDPulses;
  m_data->Stores["ANNIEEvent"]->Get("CFDRecoLAPPDPulses",CFDRecoLAPPDPulses);

  // loop over all channels
  std::map<int, vector<Waveform<double>>> :: iterator itr;
  for (itr = rawlappddata.begin(); itr != rawlappddata.end(); ++itr){
    int channelno = itr->first;
    vector<Waveform<double>> Vwavs = itr->second;

    vector<Waveform<double>> Vfwavs;
    if(isFiltered){
      std::map<int, vector<Waveform<double>>>::iterator fp;
      fp = filteredlappddata.find(channelno);
      Vfwavs = fp->second;
    }

    // get the pulses for each channel
    map<int, vector<LAPPDPulse>>::iterator p;
    p = CFDRecoLAPPDPulses.find(channelno);
    vector<LAPPDPulse> Vpulses = p->second;

    // get the vector of charges for each channel
    map<int, vector<double>>::iterator pq;
    pq = TheCharges.find(channelno);
    vector<double> Vqs = pq->second;

    // loop over the charges and store to a hist
    for(int pqi=0; pqi<Vqs.size(); pqi++){
      double theq = Vqs.at(pqi);
      if(channelno!=TrigChannel) hQ[channelno]->Fill(theq);
    }

    //loop over all pulses fill histos and fill tree with pulse information
    for(int i=0; i<Vpulses.size(); i++){
      LAPPDPulse thepulse = Vpulses.at(i);
      if(channelno!=TrigChannel){

        hAmp[channelno]->Fill(thepulse.GetPeak());
        hTime[channelno]->Fill(thepulse.GetTpsec());

        chno=channelno;
        cfdtime=thepulse.GetTpsec();
        amp=thepulse.GetPeak();
        twidth=(thepulse.GetHiRange()-thepulse.GetLowRange())*Deltat;
        outtree->Fill();
      }

    }

    //loop over all Waveforms, make histrograms
    std::vector<LAPPDPulse> thepulses;
    for(int i=0; i<Vwavs.size(); i++){

        Waveform<double> bwav = Vwavs.at(i);
        Waveform<double> bfwav;
        if(isFiltered) bfwav = Vfwavs.at(i);


        int nbins = bwav.GetSamples()->size();
        double starttime=0.;
        double endtime = starttime + ((double)nbins)*100.;
        TH1D* hwav = new TH1D("hwav","hwav",nbins,starttime,endtime);
        TH1D* fwav = new TH1D("fwav","fwav",nbins,starttime,endtime);


        for(int i=0; i<nbins; i++){
          hwav->SetBinContent(i+1,-bwav.GetSamples()->at(i));
          if(isFiltered) fwav->SetBinContent(i+1,-bfwav.GetSamples()->at(i));
        }

        TString hname;
        hname+="wav_ch";
        hname+=channelno;
        hname+="_wav";
        hname+=i;
        hname+="_evt";
        hname+=miter;

        // sotre wavform hists in the "wav" folder
        tf->cd("wavs");
        if(miter<NHistos) hwav->Write(hname);

        if(isFiltered){
          tf->cd("filteredwavs");
          if(miter<NHistos) fwav->Write("filtered"+hname);
        }

        delete hwav;
        delete fwav;

    }
  }

  miter++;
  return true;
}


bool LAPPDSaveROOT::Finalise(){

  // go to the top level of the output file
  tf->cd();

  // write the summary histos to file
  for(int i=0; i<NChannel; i++){
    hAmp[i]->Write();
    hTime[i]->Write();
    hQ[i]->Write();
  }
  // write the output tree to file
  outtree->Write();

  tf->Close();

  return true;
}
