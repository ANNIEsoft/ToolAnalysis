#include "LAPPDPlotWaveForms.h"

LAPPDPlotWaveForms::LAPPDPlotWaveForms():Tool(){}


bool LAPPDPlotWaveForms::Initialise(std::string configfile, DataModel &data){

  /////////////////// Useful header ///////////////////////
  if(configfile!="") m_variables.Initialise(configfile); // loading config file
  //m_variables.Print();

  m_data= &data; //assigning transient data pointer
  /////////////////////////////////////////////////////////////////

    m_data->Stores["ANNIEEvent"]->Header->Get("AnnieGeometry", _geom);

    TString IWL;
    //RawInputWavLabel;
    m_variables.Get("PlotWavLabel",IWL);
    InputWavLabel = IWL;

    //tf->mkdir("filteredwavs");
    //tf->mkdir("blswavs");

    isFiltered=false;
    isBLsub=false;

    // have the pulses been filtered? integrated? is this real or simulated data?
    bool ifilt = m_data->Stores["ANNIEEvent"]->Header->Get("isFiltered",isFiltered);
    bool ibl = m_data->Stores["ANNIEEvent"]->Header->Get("isBLsubtracted",isBLsub);
    bool iI = m_data->Stores["ANNIEEvent"]->Header->Get("isIntegrated",isIntegrated);
    bool isim = m_data->Stores["ANNIEEvent"]->Header->Get("isSim",isSim);

    // keep count of the loop number (starting from 0)
    miter=0;

    m_variables.Get("NHistos", NHistos);
    m_variables.Get("SampleSize",Deltat);
    m_variables.Get("SaveByChannel",SaveByChannel);
    m_variables.Get("SaveSingleStrip",SaveSingleStrip);
    m_variables.Get("SingleStripNo",psno);
    m_variables.Get("TrigNo",trigno);
    m_variables.Get("requireT0signal",requireT0signal);


    // setup the output files
    TString OutFile = "lapptraces.root";     //default input file
    m_variables.Get("outfile", OutFile);
    mtf = new TFile(OutFile,"RECREATE");
    if(!SaveSingleStrip) mtf->mkdir("wavs");

    cout<<"SAVE SINGLE STRIP: "<<SaveSingleStrip<<" "<<psno<<" "<<trigno<<endl;

    PHD = new TH1D("PHD","PHD",1000,-1e5,1e7);

    // declare the histograms
    /*
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
      hQ[i] = new TH1D(QName,QName,8800,-1e7,10e7);

      TString TimeName;
      TimeName+="Time_CH";
      TimeName+=i;
      hTime[i] = new TH1D(TimeName,TimeName,10000,0.,100000.);
    }*/

  return true;
}


bool LAPPDPlotWaveForms::Execute(){

    // get raw lappd data
    std::map<unsigned long,vector<Waveform<double>>> lappddata;

    //m_data->Stores["ANNIEEvent"]->Get("RawLAPPDData",rawlappddata);
    m_data->Stores["ANNIEEvent"]->Get(InputWavLabel,lappddata);

    bool T0signalInWindow;
    m_data->Stores["ANNIEEvent"]->Get("T0signalInWindow",T0signalInWindow);
    if(requireT0signal && !T0signalInWindow) {
    	miter++;
    	return true;
    }

    double totcharge;
    m_data->Stores["ANNIEEvent"]->Get("TotCharge",totcharge);
    PHD->Fill(totcharge);


    // loop over all channels
    std::map<unsigned long, vector<Waveform<double>>> :: iterator itr;
    for (itr = lappddata.begin(); itr != lappddata.end(); ++itr){

        unsigned long channelno = itr->first;
        vector<Waveform<double>> Vwavs = itr->second;

        Channel* mychannel = _geom->GetChannel(channelno);
        //figure out the stripline number
        int stripno = mychannel->GetStripNum();
        //figure out the side of the stripline
        int stripside = mychannel->GetStripSide();

        for(int i=0; i<Vwavs.size(); i++){
            Waveform<double> bwav = Vwavs.at(i);

            int nbins = bwav.GetSamples()->size();
            //cout<<"IN PLOT WAVES "<<channelno<<" "<<nbins<<endl;
            double starttime=0.;
            double endtime = starttime + ((double)nbins)*100.;

            TString hname;

            TH1D* hwav;

            if(!SaveSingleStrip){

              if(SaveByChannel){
                hname+="wav_ch";
                hname+=channelno;
                hname+="_wav";
                hname+=i;
                hname+="_evt";
                hname+=miter;
              } else{
                hname+="wav_strip_";
                hname+=stripno;
                hname+="_";
                if(stripside==0) hname+="L";
                else hname+="R";
                hname+="_evt";
                hname+=miter;
              }

              hwav = new TH1D(hname,hname,nbins,starttime,endtime);

              for(int i=0; i<nbins; i++){
                hwav->SetBinContent(i+1,bwav.GetSamples()->at(i));
              }

              mtf->cd("wavs");
              if(miter<NHistos) hwav->Write();
              delete hwav;
            } else{

              mtf->cd();
              if(stripno==psno){
                hname+="wav_strip_";
                hname+=stripno;
                hname+="_";
                if(stripside==0) hname+="L";
                else hname+="R";
                hname+="_evt";
                hname+=miter;
                hwav = new TH1D(hname,hname,nbins,starttime,endtime);
                for(int i=0; i<nbins; i++){
                  hwav->SetBinContent(i+1,bwav.GetSamples()->at(i));
                }
                if(miter<NHistos) hwav->Write();
                delete hwav;
              }

              if((int)channelno==trigno){
                hname+="trig_";
                hname+=miter;
                hwav = new TH1D(hname,hname,nbins,starttime,endtime);
                for(int i=0; i<nbins; i++){
                  hwav->SetBinContent(i+1,bwav.GetSamples()->at(i));
                }
                if(miter<NHistos) hwav->Write();
                delete hwav;
              }
            }
          } //cout<<"WRITTEN!"<<endl; }
      }



  miter++;


  return true;
}


bool LAPPDPlotWaveForms::Finalise(){

    mtf->cd();
    mtf->Write();
    mtf->Close();
    delete mtf;

  return true;
}
