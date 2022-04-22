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

  WraparoundBin=0; QualityVar=0; TrigDeltaT=0.; PulseHeight=0.; BeamTime=0.; EventTime=0.; TotalCharge=0.; Npulses_cfd=0; Npulses_simp=0; T0Bin=0;
  NHits=0; NHits_simp=0; Npulses_cfd=0; Npulses_simp=0;
  for(int i=0; i<60; i++){
      hQ[i]=0;  hxpar[i]=0; hxperp[i]=0; htime[i]=0;  hdeltime[i]=0; hvpeak[i]=0;
      hQ_simp[i]=0;  hxpar_simp[i]=0; hxperp_simp[i]=0; htime_simp[i]=0;

      pulsestart_cfd[i]=0;  pulsestrip_cfd[i]=0;  pulseside_cfd[i]=0; pulseamp_cfd[i]=0; pulseQ_cfd[i]=0;

      pulsestart_simp[i]=0; pulseend_simp[i]=0; pulseamp_simp[i]=0;  pulsepeakbin_simp[i]=0;
      pulseQ_simp[i]=0; pulsestrip_simp[i]=0; pulseside_simp[i]=0;
  }

  // set the branches ///////////////////////////

  //global parameters
  fMyTree->Branch("T0Bin",                    &T0Bin,                     "T0Bin/I"                    );
  fMyTree->Branch("WraparoundBin",            &WraparoundBin,             "WraparoundBin/I"            );
  fMyTree->Branch("QualityVar",               &QualityVar,                "QualityVar/I"               );
  fMyTree->Branch("TrigDeltaT",               &TrigDeltaT,                "TrigDeltaT/D"               );
  fMyTree->Branch("PulseHeight",              &PulseHeight,               "PulseHeight/D"              );


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

  //Hit parameters (from simple FindPeak)

  fMyTree->Branch("NHits_simp",            &NHits_simp,             "NHits_simp/I"               );
  fMyTree->Branch("Q_simp",                hQ_simp,                 "Q_simp[NHits_simp]/D"            );
  fMyTree->Branch("xpar_simp",             hxpar_simp,              "Xpar_simp[NHits_simp]/D"         );
  fMyTree->Branch("xperp_simp",            hxperp_simp,             "Xperp_simp[NHits_simp]/D"        );
  fMyTree->Branch("time_simp",             htime_simp,              "time_simp[NHits_simp]/D"         );

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
  m_data->Stores["ANNIEEvent"]->Get("deltaT",TrigDeltaT);
  m_data->Stores["ANNIEEvent"]->Get("TotCharge",PulseHeight);
  m_data->Stores["ANNIEEvent"]->Get("T0Bin",T0Bin);
  m_data->Stores["ANNIEEvent"]->Get("T0signalInWindow",T0signalInWindow);
  //m_data->Stores["ANNIEEvent"]->Get("WraparoundBin",WraparoundBin);

  vector<unsigned int> tcounters;
  m_data->Stores["ANNIEEvent"]->Get("TimingCounters",tcounters);
  m_data->Stores["ANNIEEvent"]->Get("EventCharge",TotalCharge);

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
