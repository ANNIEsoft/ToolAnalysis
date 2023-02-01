#include "LAPPDClusterTree.h"

LAPPDClusterTree::LAPPDClusterTree():Tool(){}


bool LAPPDClusterTree::Initialise(std::string configfile, DataModel &data)
{

  /////////////////// Useful header ///////////////////////
  if(configfile!="") m_variables.Initialise(configfile); // loading config file
  //m_variables.Print();

  m_data= &data; //assigning transient data pointer
  /////////////////////////////////////////////////////////////////
  nottf = new TFile("Analysis.root","RECREATE");
  fMyTree = new TTree("ffmytree","ffmytree");

  m_variables.Get("LAPPDClusterTreeVerbosity",LAPPDClusterTreeVerbosity);
  m_variables.Get("getSimpleClusters",simpleClusters);

  TString SCL;
  m_variables.Get("SimpleClusterLabel",SCL);
  SimpleClusterLabel = SCL;
  TString CFDCL;
  m_variables.Get("CFDClusterLabel",CFDCL);
  CFDClusterLabel = CFDCL;

  // zero out variables //////////////////////////

  WraparoundBin=0; QualityVar=0; TrigDeltaT1=0.; TrigDeltaT2=0.; PulseHeight=0.; MaxAmp0=0.; MaxAmp1=0.; BeamTime=0.; EventTime=0.; TotalCharge=0.; Npulses_cfd=0; Npulses_simp=0; T0Bin=0;
  NHits=0; NHits_simp=0; Npulses_cfd=0; Npulses_simp=0;
  Nchannels=60;

  for(int i=0; i<60; i++){
      hQ[i]=0;  hxpar[i]=0; hxperp[i]=0; htime[i]=0;  hdeltime[i]=0; hvpeak[i]=0;
      hQ_simp[i]=0;  hxpar_simp[i]=0; hxperp_simp[i]=0; htime_simp[i]=0;

      pulsestart_cfd[i]=0;  pulsestrip_cfd[i]=0;  pulseside_cfd[i]=0; pulseamp_cfd[i]=0; pulseQ_cfd[i]=0;

      pulsestart_simp[i]=0; pulseend_simp[i]=0; pulseamp_simp[i]=0;  pulsepeakbin_simp[i]=0;
      pulseQ_simp[i]=0; pulsestrip_simp[i]=0; pulseside_simp[i]=0;

      SelectedAmp0[i]=0; SelectedAmp1[i]=0; SelectedTime0[i]=0; SelectedTime1[i]=0;

      StripPeak[i]=0;  StripPeak_Sm[i]=0;  StripPeakT[i]=0;  StripPeakT_Sm[i] =0;
      StripQ[i]=0;  StripQ_Sm[i]=0;

  }

  // set the branches ///////////////////////////

  //global parameters
  fMyTree->Branch("T0Bin",                    &T0Bin,                     "T0Bin/I"                    );
  fMyTree->Branch("WraparoundBin",            &WraparoundBin,             "WraparoundBin/I"            );
  fMyTree->Branch("QualityVar",               &QualityVar,                "QualityVar/I"               );
//  fMyTree->Branch("TrigDeltaT",               &TrigDeltaT,                "TrigDeltaT/D"               );
  fMyTree->Branch("TrigDeltaT1",              &TrigDeltaT1,               "TrigDeltaT1/D"              );
  fMyTree->Branch("TrigDeltaT2",              &TrigDeltaT2,               "TrigDeltaT2/D"              );

  fMyTree->Branch("PulseHeight",              &PulseHeight,               "PulseHeight/D"              );
  fMyTree->Branch("MaxAmp0",                  &MaxAmp0,                   "MaxAmp0/D"                  );
  fMyTree->Branch("MaxAmp1",                  &MaxAmp1,                   "MaxAmp1/D"                  );


  fMyTree->Branch("BeamTime",                 &BeamTime,                  "BeamTime/D"                 );
  fMyTree->Branch("EventTime",                &EventTime,                 "EventTime/D"                );
  fMyTree->Branch("TotalCharge",              &TotalCharge,               "TotalCharge/D"              );


  //Hit parameters (from CFD)
  fMyTree->Branch("NHits",            &NHits,             "NHits/I"               );
  fMyTree->Branch("Q",                hQ,                 "Q[NHits]/D"            );
  fMyTree->Branch("xpar",             hxpar,              "Xpar[NHits]/D"         );
  fMyTree->Branch("xperp",            hxperp,             "Xperp[NHits]/D"        );
  fMyTree->Branch("time",             htime,              "time[NHits]/D"         );
  fMyTree->Branch("deltime",          hdeltime,           "deltime[NHits]/D"      );
  fMyTree->Branch("Vpeak",            hvpeak,             "Vpeak[NHits]/D"        );

  //Strip parameters (from TraceMax)

  fMyTree->Branch("Nchannels",        &Nchannels,         "Nchannels/I"               );
  fMyTree->Branch("StripPeak",        StripPeak,          "StripPeak[Nchannels]/D"    );
  fMyTree->Branch("StripPeak_Sm",     StripPeak_Sm,       "StripPeak_Sm[Nchannels]/D" );
  fMyTree->Branch("StripPeakT",       StripPeakT,         "StripPeakT[Nchannels]/D"   );
  fMyTree->Branch("StripPeakT_Sm",    StripPeakT_Sm,      "StripPeakT_Sm[Nchannels]/D");
  fMyTree->Branch("StripQ",           StripQ,             "StripQ[Nchannels]/D"       );
  fMyTree->Branch("StripQ_Sm",        StripQ_Sm,          "StripQ_Sm[Nchannels]/D"    );


  //Hit parameters (from simple FindPeak)
  fMyTree->Branch("NHits_simp",            &NHits_simp,             "NHits_simp/I"               );
  fMyTree->Branch("Q_simp",                hQ_simp,                 "Q_simp[NHits_simp]/D"            );
  fMyTree->Branch("xpar_simp",             hxpar_simp,              "Xpar_simp[NHits_simp]/D"         );
  fMyTree->Branch("xperp_simp",            hxperp_simp,             "Xperp_simp[NHits_simp]/D"        );
  fMyTree->Branch("time_simp",             htime_simp,              "time_simp[NHits_simp]/D"         );

  //Hit parameters (from nnls fitting and matching)
  fMyTree->Branch("nnlsParallelP",         nnlsParallelP,           "nnlsParallelP/D");
  fMyTree->Branch("nnlsTransverseP",       nnlsTransverseP,         "nnlsTransverseP/D");
  fMyTree->Branch("nnlsArrivalTime",       nnlsArrivalTime,         "nnlsArrivalTime/D");
  fMyTree->Branch("nnlsAmplitude",         nnlsAmplitude,           "nnlsAmplitude/D");

  //Simple Distribution Analysis
  fMyTree->Branch("SelectedAmp0",           SelectedAmp0,             "SelectedAmp0/D");
  fMyTree->Branch("SelectedAmp1",           SelectedAmp1,             "SelectedAmp1/D");
  fMyTree->Branch("SelectedTime0",          SelectedTime0,            "SelectedTime0/D");
  fMyTree->Branch("SelectedTime1",          SelectedTime1,            "SelectedTime1/D");
  fMyTree->Branch("GMaxOn0",                GMaxOn0,                  "GMaxOn0/D");
  fMyTree->Branch("GMaxOn1",                GMaxOn1,                  "GMaxOn1/D");

  //Pulse parameters
  fMyTree->Branch("Npulses_simp",            &Npulses_simp,             "Npulses_simp/I"                  );
  fMyTree->Branch("pulsestart_simp",         pulsestart_simp,           "pulsestart_simp[Npulses_simp]/D" );
  fMyTree->Branch("pulseend_simp",           pulseend_simp,             "pulseend_simp[Npulses_simp]/D"   );
  fMyTree->Branch("pulsepeakbin_simp",       pulsepeakbin_simp,         "pulsepeakbin_simp[Npulses_simp]/D");
  fMyTree->Branch("pulseamp_simp",           pulseamp_simp,             "pulseamp_simp[Npulses_simp]/D"   );
  fMyTree->Branch("pulseQ_simp",             pulseQ_simp,               "pulseQ_simp[Npulses_simp]/D"     );
  fMyTree->Branch("pulsestrip_simp",         pulsestrip_simp,           "pulsestrip_simp[Npulses_simp]/I" );
  fMyTree->Branch("pulseside_simp",          pulseside_simp,            "pulseside_simp[Npulses_simp]/I"  );

  fMyTree->Branch("Npulses_cfd",             &Npulses_cfd,              "Npulses_cfd/I"                   );
  fMyTree->Branch("pulsestart_cfd",          pulsestart_cfd,            "pulsestart_simp[Npulses_cfd]/D"  );
  fMyTree->Branch("pulsestrip_cfd",          pulsestrip_cfd,            "pulsestrip_simp[Npulses_cfd]/I"  );
  fMyTree->Branch("pulseside_cfd",           pulseside_cfd,             "pulseside_simp[Npulses_cfd]/I"   );
  fMyTree->Branch("pulseamp_cfd",            pulseamp_cfd,              "pulseamp_simp[Npulses_cfd]/D"   );
  fMyTree->Branch("pulseQ_cfd",              pulseQ_cfd,                "pulseQ_simp[Npulses_cfd]/D"     );


  m_data->Stores["ANNIEEvent"]->Header->Get("AnnieGeometry",_geom);


  //Pulse parameters (from each hit)
  //fMyTree->Branch("Npulses_cfd",            &Npulses_cfd,             "Npulses_cfd/I"               );


  return true;
}


bool LAPPDClusterTree::Execute()
{
  if(LAPPDClusterTreeVerbosity>0) cout<<"Cluster Tree Execute"<<endl;
  nottf->cd();
  std::map<unsigned long,vector<LAPPDHit>> Hits;
  std::map<unsigned long,vector<LAPPDHit>> SimpleHits;
  std::map <unsigned long, vector<LAPPDPulse>> RecoLAPPDPulses;
  std::map <unsigned long, vector<LAPPDPulse>> SimpleLAPPDPulses;

  bool isCFD;
  bool T0signalInWindow;

  m_data->Stores["ANNIEEvent"]->Get("isCFD",isCFD);

  if(isCFD==true){
    m_data->Stores["ANNIEEvent"]->Get("Clusters",Hits);
    m_data->Stores["ANNIEEvent"]->Get(CFDClusterLabel,RecoLAPPDPulses);

  }
  if(simpleClusters){
    m_data->Stores["ANNIEEvent"]->Get("SimpleClusters",SimpleHits);
    m_data->Stores["ANNIEEvent"]->Get(SimpleClusterLabel,SimpleLAPPDPulses);
  }

  //get the global variables for the TREE
  vector<double> TrigDeltaT;
  m_data->Stores["ANNIEEvent"]->Get("deltaT",TrigDeltaT);
  TrigDeltaT1 = TrigDeltaT.at(0);
  TrigDeltaT2 = TrigDeltaT.at(1);

  m_data->Stores["ANNIEEvent"]->Get("TotCharge",PulseHeight);
  m_data->Stores["ANNIEEvent"]->Get("MaxAmp0",MaxAmp0);
  m_data->Stores["ANNIEEvent"]->Get("MaxAmp1",MaxAmp1);

  m_data->Stores["ANNIEEvent"]->Get("T0Bin",T0Bin);
  m_data->Stores["ANNIEEvent"]->Get("T0signalInWindow",T0signalInWindow);
  //m_data->Stores["ANNIEEvent"]->Get("WraparoundBin",WraparoundBin);

  vector<unsigned int> tcounters;
  m_data->Stores["ANNIEEvent"]->Get("TimingCounters",tcounters);
  m_data->Stores["ANNIEEvent"]->Get("EventCharge",TotalCharge);


  std::map<int,vector<vector<double>>> TML;
  std::map<int,vector<vector<double>>> TMR;
  m_data->Stores["ANNIEEvent"]->Get("TML",TML);
  m_data->Stores["ANNIEEvent"]->Get("TMR",TMR);



  map <int, vector<vector<double>>> :: iterator TMitr;
  for (TMitr = TML.begin(); TMitr != TML.end(); ++TMitr){
    int stripno = TMitr->first;
    vector<vector<double>> vaTML = TMitr->second;
    vector<double> aTML = vaTML.at(0);
    StripPeak[stripno] = aTML.at(0);
    StripPeakT[stripno] = aTML.at(1);
    StripPeak_Sm[stripno] = aTML.at(2);
    StripPeakT_Sm[stripno] = aTML.at(3);
    StripQ[stripno] = aTML.at(4);
    StripQ_Sm[stripno] = aTML.at(5);

    //cout<<"Trace on Strip in ClusterTree "<<(int) stripno<<" amplitude:"<<aTML.at(0)<<" peaktime: "<<aTML.at(1)<<" charge:"<<aTML.at(4)<<" charge(smoothe peak):"<<aTML.at(5)<<endl;
  }

  for (TMitr = TMR.begin(); TMitr != TMR.end(); ++TMitr){
    int stripno = TMitr->first;
    vector<vector<double>> vaTML = TMitr->second;
    vector<double> aTML = vaTML.at(0);

    StripPeak[stripno+30] = aTML.at(0);
    StripPeakT[stripno+30] = aTML.at(1);
    StripPeak_Sm[stripno+30] = aTML.at(2);
    StripPeakT_Sm[stripno+30] = aTML.at(3);
    StripQ[stripno+30] = aTML.at(4);
    StripQ_Sm[stripno+30] = aTML.at(5);
    //cout<<"Trace on Strip in ClusterTree "<<(int) channelno<<" amplitude:"<<aTML.at(0)<<" peaktime: "<<aTML.at(1)<<" charge:"<<aTML.at(4)<<" charge(smoothe peak):"<<aTML.at(5)<<endl;
  }


  //nnls hits
  std::map<unsigned long,vector<double>> NNLSLocatedHits;
  m_data->Stores["ANNIEEvent"]->Get("NNLSLocatedHits",NNLSLocatedHits);
  std::map <unsigned long, vector<double>> :: iterator nnlsItr;

    for (nnlsItr = NNLSLocatedHits.begin(); nnlsItr != NNLSLocatedHits.end(); ++nnlsItr){
      unsigned long i = nnlsItr->first;
      vector<double> info = nnlsItr->second;

      nnlsParallelP[i] = info[0];
      nnlsTransverseP[i] = info[1];
      nnlsArrivalTime[i] = info[2];
      nnlsAmplitude[i] = info[3];
    }

    //other pulse simple analysis
    vector<vector<double>> OtherSimp;
    m_data->Stores["ANNIEEvent"]->Get("OtherSimpVec",OtherSimp);
    vector<double> trigMax;
    m_data->Stores["ANNIEEvent"]->Get("OtherSimpTrigMax",trigMax);

    for(int i =0;i<OtherSimp.at(0).size();i++){
      SelectedAmp0[i] = OtherSimp.at(0).at(i);
      SelectedAmp1[i] = OtherSimp.at(1).at(i);
      SelectedTime0[i] = OtherSimp.at(2).at(i);
      SelectedTime1[i] = OtherSimp.at(3).at(i);
    }
    GMaxOn0[0] = trigMax.at(0);
    GMaxOn1[0] = trigMax.at(1);


  //cout<<"Cluster Tree Yo:  "<<tcounters.at(0)<<" "<<tcounters.at(1)<<" "<<tcounters.at(2)<<" "<<tcounters.at(3)<<endl;

  unsigned int beamcounter = tcounters.at(0);
  unsigned int beamcounterL = tcounters.at(1);
  unsigned int trigcounter = tcounters.at(2);
  unsigned int trigcounterL = tcounters.at(3);

  double largetime = (double)beamcounterL*13.1;
  double smalltime = ((double)beamcounter/1E9)*3.125;

  BeamTime=((double)((trigcounter-beamcounter))*3.125)/1E3;
  //cout<<"TotCharge in Tree: "<<TotalCharge;
  //cout<<"Beam Time "<<BeamTime<<" LargeTime "<<largetime<<" smalltime: "<<smalltime<<endl;
  EventTime=largetime+smalltime;

  if(T0signalInWindow) QualityVar=1;
  else QualityVar=0;

  if(LAPPDClusterTreeVerbosity>1) cout<<"SETUP COMPLETE  HELLO"<<endl;
  if(LAPPDClusterTreeVerbosity>1&&isCFD) cout<<"HITS SIZE: "<< Hits.size() << endl;
  if(LAPPDClusterTreeVerbosity>1&&simpleClusters) cout<<simpleClusters<<" SIMPLE HITS SIZE: "<<SimpleHits.size()<<"  ASDFAS"<<endl;
  if(LAPPDClusterTreeVerbosity>1&&simpleClusters) cout<<"SIMPLE PULSES SIZE: "<<SimpleLAPPDPulses.size()<<endl;

  map <unsigned long, vector<LAPPDHit>> :: iterator itr;
  vector<LAPPDHit> :: iterator itrr;

  if(simpleClusters){
    for (itr = Hits.begin(); itr != Hits.end(); ++itr)
      {
        int m=0;
        //  cout<<"HANDLING A HIT YO"<<endl;
        unsigned long  channel= itr->first;
        vector<LAPPDHit> hitvect = itr->second;
        int numberhits = itr->second.size();
        NHits = numberhits;
        //cout<<"Size of numberhits:  "<< numberhits << endl;
        for (itrr = itr->second.begin(); itrr!= itr->second.end(); ++itrr)
        {
            hT[m]=hitvect[m].GetTime();
            hQ[m]=hitvect[m].GetCharge();
            vector<double> localposition;
            localposition=hitvect[m].GetLocalPosition();
            if(LAPPDClusterTreeVerbosity>2) cout<<"Size of localposition "<< localposition.size()<<endl;
            if(localposition.size()>0)
            {
                hxpar[m]=localposition[0];
                hxperp[m]=localposition[1];
                if(LAPPDClusterTreeVerbosity>2) cout<<"Position in LAPPDClusterTree "<<hxpar[m]<<" "<<hxperp[m]<<endl;
            }
            htime[m]=hitvect[m].GetTime();
            m++;
            if(m>=59) { cout<<"MORE THAN 60 HITS!!!!!!"<<endl; break; }
          }
        }
    }

    if(simpleClusters){
        map <unsigned long, vector<LAPPDHit>> :: iterator sitr;
        vector<LAPPDHit> :: iterator sitrr;
        //cout<<"In here"<<endl;
        for (sitr = SimpleHits.begin(); sitr != SimpleHits.end(); ++sitr)
          {
            int m=0;
            //cout<<"HANDLING A HIT YO"<<endl;
            unsigned long  channel= sitr->first;
            vector<LAPPDHit> hitvect = sitr->second;
            int numberhits = sitr->second.size();
            NHits_simp = numberhits;
            //cout<<"Size of numberhits:  "<< numberhits << endl;
            for (sitrr = sitr->second.begin(); sitrr!= sitr->second.end(); ++sitrr)
            {
                hT_simp[m]=hitvect[m].GetTime();
                hQ_simp[m]=hitvect[m].GetCharge();
                vector<double> localposition;
                localposition=hitvect[m].GetLocalPosition();
                if(LAPPDClusterTreeVerbosity>2) cout<<"Size of localposition "<< localposition.size()<<endl;
                if(localposition.size()>0)
                {
                    hxpar_simp[m]=localposition[0];
                    hxperp_simp[m]=localposition[1];
                    if(LAPPDClusterTreeVerbosity>2) cout<<"Position in LAPPDClusterTree "<<hxpar[m]<<" "<<hxperp[m]<<endl;
                }
                htime_simp[m]=hitvect[m].GetTime();
                m++;
                if(m>=50) { cout<<"MORE THAN 60 SIMPLE HITS!!!!!!"<<endl; break; }
            }
          }
      }

      // Get the Pulse Information for the ANNIEEvent
      if(LAPPDClusterTreeVerbosity>1) cout<<"Pulse Information"<<endl;

      std::map <unsigned long, vector<LAPPDPulse>> :: iterator pulseitr;
      std::map <unsigned long, vector<LAPPDPulse>> :: iterator pulseitrsimp;
      Npulses_cfd=0;

      if(isCFD==true){
        Npulses_cfd=0;

        if(LAPPDClusterTreeVerbosity>1) cout<<"cfd pulses..."<<endl;

        for (pulseitr = RecoLAPPDPulses.begin(); pulseitr != RecoLAPPDPulses.end(); ++pulseitr){

          unsigned long chankey = pulseitr->first;
          vector<LAPPDPulse> vPulse = pulseitr->second;
          Channel* mychannel = _geom->GetChannel(chankey);
          int mystripnum = mychannel->GetStripNum();
          int mystripside = mychannel->GetStripSide();

          if(LAPPDClusterTreeVerbosity>1) cout<<"in cfd pulse loop "<<Npulses_cfd<<" "<<vPulse.size()<<endl;

          for(int jj=0; jj<vPulse.size(); jj++){
            if(Npulses_cfd>=59) { cout<<"MORE THAN 60 CFD PULSES!!!!!! "<<Npulses_cfd<<endl; break; }
            LAPPDPulse apulse = vPulse.at(jj);
            if(LAPPDClusterTreeVerbosity>1) cout<<"the charge of this pulse is: "<<apulse.GetCharge()<<endl;
            if(LAPPDClusterTreeVerbosity>1) cout<<"The Time of this Pulse is " <<apulse.GetTime() <<endl;

            pulsestrip_cfd[Npulses_cfd]=mystripnum;
            pulseside_cfd[Npulses_cfd]=mystripside;
            pulsestart_cfd[Npulses_cfd]= apulse.GetTime();
            pulseamp_cfd[Npulses_cfd]=apulse.GetPeak();
            pulseQ_cfd[Npulses_cfd]=apulse.GetCharge();
            //pulsepeakbin_cfd[Npulses_simp] = (Double_t)apulse.GetChannelID();
            //pulseamp_cfd[Npulses_simp]= apulse.GetPeak();
            //pulseQ_cfd[Npulses_simp]=apulse.GetCharge();

            Npulses_cfd++;
          }
        }
      }

      if(LAPPDClusterTreeVerbosity>1) cout<<"Between cfd and simple pulses"<<endl;

      if(simpleClusters){
        Npulses_simp=0;

        if(LAPPDClusterTreeVerbosity>1) cout<<"simple pulses..."<<endl;
        if(LAPPDClusterTreeVerbosity>2) cout<<"S size: "<<SimpleLAPPDPulses.size()<<endl;

        for (pulseitrsimp = SimpleLAPPDPulses.begin(); pulseitrsimp != SimpleLAPPDPulses.end(); ++pulseitrsimp){

          unsigned long chankey = pulseitrsimp->first;
          vector<LAPPDPulse> vPulse = pulseitrsimp->second;
          Channel* mychannel = _geom->GetChannel(chankey);
          int mystripnum = mychannel->GetStripNum();
          int mystripside = mychannel->GetStripSide();

          if(LAPPDClusterTreeVerbosity>2) cout<<"vPsize: "<<vPulse.size()<<" "<<mystripnum<<" "<<mystripside<<endl;;

          for(int jj=0; jj<vPulse.size(); jj++){
            if(Npulses_simp>=59) { cout<<"MORE THAN 60 SIMPLE PULSES!!!!!!"<<endl; break; }
            LAPPDPulse apulse = vPulse.at(jj);
            if(LAPPDClusterTreeVerbosity>2) cout<<"the charge of this pulse is: "<<apulse.GetCharge()<<" "<<Npulses_simp<<" "<<mystripnum<<" "<<mystripside<<endl;
            if(LAPPDClusterTreeVerbosity>2) cout<< "The Time of this Pulse is " <<apulse.GetTime() <<endl;

                //pulsestart_simp[Npulses_simp]= apulse.GetLowRange();
           pulsestrip_simp[Npulses_simp]=mystripnum;
           pulseside_simp[Npulses_simp]=mystripside;
           pulsestart_simp[Npulses_simp]= apulse.GetTime();
            //pulsestart_simp[Npulses_simp]= apulse.GetLowRange();
           pulseend_simp[Npulses_simp]= apulse.GetHiRange();
           pulsepeakbin_simp[Npulses_simp] = (Double_t)apulse.GetChannelID();
           pulseamp_simp[Npulses_simp]= apulse.GetPeak();
           pulseQ_simp[Npulses_simp]=apulse.GetCharge();
           Npulses_simp++;
              //cout<<Npulses_simp<<"    sdfsdf"<<endl;
          }
        }
        if(LAPPDClusterTreeVerbosity>1) cout<<"DONE WITH SIMP Npulses: "<<Npulses_simp<<endl;
    }



    if(LAPPDClusterTreeVerbosity>1) cout<<"FILLING TREE WITH HITS"<<endl;
    fMyTree->Fill();


    if(LAPPDClusterTreeVerbosity>0) cout<<"End LAPPDClusterTree..............................."<<endl;

  return true;
}


bool LAPPDClusterTree::Finalise(){

  cout<<"here at the end of ClusterTree"<<endl;
  nottf->cd();
  cout<<"writing the tree"<<endl;
  fMyTree->Write();
  nottf->Close();
  cout<<"Done closing the file"<<endl;
  return true;
}
