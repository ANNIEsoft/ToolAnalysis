#include "LAPPDPlotWaveForms2D.h"

LAPPDPlotWaveForms2D::LAPPDPlotWaveForms2D():Tool(){}


bool LAPPDPlotWaveForms2D::Initialise(std::string configfile, DataModel &data){

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

    cout<<"InputWavLabel: "<<InputWavLabel<<endl;

    //tf->mkdir("filteredwavs");
    //tf->mkdir("blswavs");

    isFiltered=false;
    isBLsub=false;

    // have the pulses been filtered? integrated? is this real or simulated data?
    bool ifilt = m_data->Stores["ANNIEEvent"]->Get("isFiltered",isFiltered);
    bool ibl = m_data->Stores["ANNIEEvent"]->Get("isBLsubtracted",isBLsub);
    bool iI = m_data->Stores["ANNIEEvent"]->Get("isIntegrated",isIntegrated);
    bool isim = m_data->Stores["ANNIEEvent"]->Get("isSim",isSim);

    // keep count of the loop number (starting from 0)
    miter=0;

    int TrigChannel,LAPPDchannelOffset;
    m_data->Stores["ANNIEEvent"]->Get("SampleSize",Deltat);
    m_data->Stores["ANNIEEvent"]->Get("TrigChannel",TrigChannel);
    m_data->Stores["ANNIEEvent"]->Get("LAPPDchannelOffset",LAPPDchannelOffset);

    trigno = TrigChannel + LAPPDchannelOffset;

    m_variables.Get("NHistos", NHistos);
    m_variables.Get("SaveByChannel",SaveByChannel);
    m_variables.Get("SaveSingleStrip",SaveSingleStrip);
    m_variables.Get("SingleStripNo",psno);
    m_variables.Get("requireT0signal",requireT0signal);


    // setup the output files
    TString OutFile = "lapptraces.root";     //default input file
    m_variables.Get("outfile2D", OutFile);
    mtf = new TFile(OutFile,"RECREATE");
    if(!SaveSingleStrip) mtf->mkdir("wavs");

    mtf->mkdir("in_beam");
    mtf->mkdir("off_beam");

    cout<<"SAVE SINGLE STRIP: "<<SaveSingleStrip<<" "<<psno<<" "<<trigno<<endl;

    PHD = new TH1D("PHD","PHD",1000,-1e5,1e7);


  return true;
}


bool LAPPDPlotWaveForms2D::Execute(){

    // get raw lappd data
    std::map<unsigned long,vector<Waveform<double>>> lappddata;

    //m_data->Stores["ANNIEEvent"]->Get("RawLAPPDData",rawlappddata);
    bool work = m_data->Stores["ANNIEEvent"]->Get(InputWavLabel,lappddata);
    //cout<<"IN PLOT WAVES 2D "<<InputWavLabel<<" "<<work<<endl;
    bool T0signalInWindow;
    m_data->Stores["ANNIEEvent"]->Get("T0signalInWindow",T0signalInWindow);
    if(requireT0signal && !T0signalInWindow) {
    	miter++;
    	return true;
    }

    // Get the beam timing information
    vector<unsigned int> tcounters;
    m_data->Stores["ANNIEEvent"]->Get("TimingCounters",tcounters);
    unsigned int beamcounter = tcounters.at(0);
    unsigned int beamcounterL = tcounters.at(1);
    unsigned int trigcounter = tcounters.at(2);
    unsigned int trigcounterL = tcounters.at(3);
    double largetime = (double)beamcounterL*13.1;
    double smalltime = ((double)beamcounter/1E9)*3.125;
    double BeamTime=((double)((trigcounter-beamcounter))*3.125)/1E3;

    //cout<<"po "<<BeamTime<<endl;

    double totcharge;
    m_data->Stores["ANNIEEvent"]->Get("TotCharge",totcharge);
    PHD->Fill(totcharge);


    // Setup 2D hist
    //cout<<"ope"<<endl;
    Waveform<double> fwav = (lappddata.begin()->second).at(0);
    int nbins = fwav.GetSamples()->size();
    //cout<<"nbins: "<<nbins<<endl;
    //cout<<"nope"<<endl;
    double starttime=0.;
    double endtime = starttime + ((double)nbins)*100.;
    TString h2name;
    h2name+="event0_";
    h2name+=miter;
    TH2D* hEvent_0 = new TH2D(h2name,h2name,nbins,starttime/1000.,endtime/1000.,30,-0.5,29.5);
    TString h3name;
    h3name+="event1_";
    h3name+=miter;
    TH2D* hEvent_1 = new TH2D(h3name,h3name,nbins,starttime/1000.,endtime/1000.,30,-0.5,29.5);

    //cout<<"done?"<<endl;

    double eventcharge = 0.;
    double eventamplitude = 0.;

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

        if(stripside==0){
          for(int i=0; i<Vwavs.size(); i++){
              Waveform<double> bwav = Vwavs.at(i);

              for(int i=0; i<nbins; i++){
                if(stripno>1 && stripno<29 && i>150) eventcharge+=(((bwav.GetSamples()->at(i))/1000.)*100)/50.;
                if((bwav.GetSamples()->at(i))<eventamplitude) eventamplitude = (bwav.GetSamples()->at(i));
                hEvent_0->SetBinContent(i+1,stripno+1,-bwav.GetSamples()->at(i));
              }
            }
          }

          if(stripside==1){
            for(int i=0; i<Vwavs.size(); i++){
                Waveform<double> bwav = Vwavs.at(i);
                for(int i=0; i<nbins; i++){
                  hEvent_1->SetBinContent(i+1,stripno+1,-bwav.GetSamples()->at(i));
                }
              }
            }




        } //cout<<"WRITTEN!"<<endl; }

  if(BeamTime>7.5&&BeamTime<9.5) mtf->cd("in_beam");
  else mtf->cd("off_beam");
  if(miter<NHistos) hEvent_0->Write();
  if(miter<NHistos) hEvent_1->Write();
  delete hEvent_0;
  delete hEvent_1;

  //cout<<BeamTime<<" "<<eventamplitude<<" "<<eventcharge<<endl;
  m_data->Stores["ANNIEEvent"]->Set("EventCharge",eventcharge);

  miter++;


  return true;
}


bool LAPPDPlotWaveForms2D::Finalise(){

    mtf->cd();
    mtf->Write();
    mtf->Close();
    delete mtf;

  return true;
}
