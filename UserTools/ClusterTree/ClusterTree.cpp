#include "ClusterTree.h"

ClusterTree::ClusterTree():Tool(){}


bool ClusterTree::Initialise(std::string configfile, DataModel &data)
{

  /////////////////// Useful header ///////////////////////
  if(configfile!="") m_variables.Initialise(configfile); // loading config file
  //m_variables.Print();

  m_data= &data; //assigning transient data pointer
  /////////////////////////////////////////////////////////////////
  nottf = new TFile("Analysis.root","RECREATE");
  fMyTree = new TTree("ffmytree","ffmytree");

  m_variables.Get("ClusterTreeVerbosity",ClusterTreeVerbosity);
  m_variables.Get("getSimpleClusters",simpleClusters);

  TString SCL;
  m_variables.Get("SimpleClusterLabel",SCL);
  SimpleClusterLabel = SCL;
  TString CFDCL;
  m_variables.Get("CFDClusterLabel",CFDCL);
  CFDClusterLabel = CFDCL;

  // zero out variables //////////////////////////

  WraparoundBin=0; QualityVar=0; TrigDeltaT=0.; PulseHeight=0.; Npulses_cfd=0; Npulses_simp=0; T0Bin=0;
  NHits=0; NHits_simp=0;
  for(int i=0; i<40; i++){
      hQ[i]=0;  hxpar[i]=0; hxperp[i]=0; htime[i]=0;  hdeltime[i]=0; hvpeak[i]=0;
      hQ_simp[i]=0;  hxpar_simp[i]=0; hxperp_simp[i]=0; htime_simp[i]=0;

      pulsestart_cfd[i]=0;  pulsestrip_cfd[i]=0;  pulseside_cfd[i]=0;
      pulsestart_simp[i]=0; pulseend_simp[i]=0; pulseamp_simp[i]=0;  pulsepeakbin_simp[i]=0;
      pulseQ_simp[i]=0; pulsestrip_simp[i]=0; pulseside_simp[i]=0;
  }

  // set the branches ///////////////////////////

  //global parameters
  fMyTree->Branch("T0Bin",                    &T0Bin,                     "T0Bin/I"                   );
  fMyTree->Branch("WraparoundBin",            &WraparoundBin,             "WraparoundBin/I"            );
  fMyTree->Branch("QualityVar",               &QualityVar,                "QualityVar/I"               );
  fMyTree->Branch("TrigDeltaT",               &TrigDeltaT,                "TrigDeltaT/D"               );
  fMyTree->Branch("PulseHeight",              &PulseHeight,               "PulseHeight/D"              );

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



  m_data->Stores["ANNIEEvent"]->Header->Get("AnnieGeometry",_geom);


  //Pulse parameters (from each hit)
  //fMyTree->Branch("Npulses_cfd",            &Npulses_cfd,             "Npulses_cfd/I"               );


  return true;
}


bool ClusterTree::Execute()
{
  if(ClusterTreeVerbosity>0) cout<<"Cluster Tree Execute"<<endl;
  nottf->cd();
  std::map<unsigned long,vector<LAPPDHit>> Hits;
  m_data->Stores["ANNIEEvent"]->Get("Clusters",Hits);
  std::map<unsigned long,vector<LAPPDHit>> SimpleHits;
  if(simpleClusters) m_data->Stores["ANNIEEvent"]->Get("SimpleClusters",SimpleHits);
  map <unsigned long, vector<LAPPDHit>> :: iterator itr;
  vector<LAPPDHit> :: iterator itrr;
  // cout<<"SETUP COMPLETE  HELLO  "<< Hits.size() << endl;

  //get the global variables for the TREE
  m_data->Stores["ANNIEEvent"]->Get("deltaT",TrigDeltaT);
  m_data->Stores["ANNIEEvent"]->Get("TotCharge",PulseHeight);
  m_data->Stores["ANNIEEvent"]->Get("T0Bin",T0Bin);
  //m_data->Stores["ANNIEEvent"]->Get("WraparoundBin",WraparoundBin);


  bool T0signalInWindow;
  m_data->Stores["ANNIEEvent"]->Get("T0signalInWindow",T0signalInWindow);
  if(T0signalInWindow) QualityVar=1;
  else QualityVar=0;

  for (itr = Hits.begin(); itr != Hits.end(); ++itr)
    {
    //  cout<<"HANDLING A HIT YO"<<endl;
      unsigned long  channel= itr->first;
      vector<LAPPDHit> hitvect = itr->second;
      int numberhits = itr->second.size();
      NHits = numberhits;
      //cout<<"Size of numberhits:  "<< numberhits << endl;
      int m=0;
      for (itrr = itr->second.begin(); itrr!= itr->second.end(); ++itrr)
      {
          hT[m]=hitvect[m].GetTime();
          hQ[m]=hitvect[m].GetCharge();
          vector<double> localposition;
          localposition=hitvect[m].GetLocalPosition();
          if(ClusterTreeVerbosity>1) cout<<"Size of localposition "<< localposition.size()<<endl;
          if(localposition.size()>0)
          {
              hxpar[m]=localposition[0];
              hxperp[m]=localposition[1];
              if(ClusterTreeVerbosity>1) cout<<"Position in ClusterTree "<<hxpar[m]<<" "<<hxperp[m]<<endl;
          }
          htime[m]=hitvect[m].GetTime();
          m++;
      }

      if(simpleClusters){
        map <unsigned long, vector<LAPPDHit>> :: iterator sitr;
        vector<LAPPDHit> :: iterator sitrr;

        for (sitr = SimpleHits.begin(); sitr != SimpleHits.end(); ++sitr)
          {
          //  cout<<"HANDLING A HIT YO"<<endl;
            unsigned long  channel= sitr->first;
            vector<LAPPDHit> hitvect = sitr->second;
            int numberhits = sitr->second.size();
            NHits_simp = numberhits;
            //cout<<"Size of numberhits:  "<< numberhits << endl;
            int m=0;
            for (sitrr = sitr->second.begin(); sitrr!= sitr->second.end(); ++sitrr)
            {
                hT_simp[m]=hitvect[m].GetTime();
                hQ_simp[m]=hitvect[m].GetCharge();
                vector<double> localposition;
                localposition=hitvect[m].GetLocalPosition();
                if(ClusterTreeVerbosity>1) cout<<"Size of localposition "<< localposition.size()<<endl;
                if(localposition.size()>0)
                {
                    hxpar_simp[m]=localposition[0];
                    hxperp_simp[m]=localposition[1];
                    if(ClusterTreeVerbosity>1) cout<<"Position in ClusterTree "<<hxpar[m]<<" "<<hxperp[m]<<endl;
                }
                htime_simp[m]=hitvect[m].GetTime();
                m++;
            }
          }
      }

      // Get the Pulse Information for the ANNIEEvent
      bool isCFD;
      m_data->Stores["ANNIEEvent"]->Get("isCFD",isCFD);
      std::map <unsigned long, vector<LAPPDPulse>> RecoLAPPDPulses;
      std::map <unsigned long, vector<LAPPDPulse>> SimpleLAPPDPulses;
      if(isCFD==true){
        m_data->Stores["ANNIEEvent"]->Get(CFDClusterLabel,RecoLAPPDPulses);
        m_data->Stores["ANNIEEvent"]->Get(SimpleClusterLabel,SimpleLAPPDPulses);
      }
      else if(isCFD==false){
        m_data->Stores["ANNIEEvent"]->Get(SimpleClusterLabel,RecoLAPPDPulses);
      }

      Npulses_cfd=0;
      Npulses_simp=0;
      std::map <unsigned long, vector<LAPPDPulse>> :: iterator pulseitr;
      std::map <unsigned long, vector<LAPPDPulse>> :: iterator pulseitrsimp;

      for (pulseitr = RecoLAPPDPulses.begin(); pulseitr != RecoLAPPDPulses.end(); ++pulseitr){
        vector<LAPPDHit> thehits;
        vector<double> localposition;
        unsigned long chankey = pulseitr->first;
        vector<LAPPDPulse> vPulse = pulseitr->second;

        vector<LAPPDPulse> vPulseSimp;

        if(isCFD){
          pulseitrsimp = SimpleLAPPDPulses.find(chankey);
          vPulseSimp = pulseitrsimp->second;
        }


        Channel* mychannel = _geom->GetChannel(chankey);
        int mystripnum = mychannel->GetStripNum();
        int mystripside = mychannel->GetStripSide();

        for(int jj=0; jj<vPulse.size(); jj++){
          LAPPDPulse apulse = vPulse.at(jj);
          //cout<<"the charge of this pulse is: "<<apulse.GetCharge()<<endl;
          //cout<< "The Time of this Pulse is " <<apulse.GetTime() <<endl;

          pulsestrip_simp[Npulses_cfd]=mystripnum;
          pulseside_simp[Npulses_cfd]=mystripside;
          pulsestrip_cfd[Npulses_cfd]=mystripnum;
          pulseside_cfd[Npulses_cfd]=mystripside;
          pulsestart_cfd[Npulses_cfd]= apulse.GetTime();


          if(isCFD){
            LAPPDPulse apulsesimp = vPulseSimp.at(jj);
            pulsestart_simp[Npulses_cfd]= apulsesimp.GetLowRange();
            pulseend_simp[Npulses_cfd]= apulsesimp.GetHiRange();
            pulsepeakbin_simp[Npulses_cfd] = (Double_t)apulsesimp.GetChannelID();
            pulseamp_simp[Npulses_cfd]= apulsesimp.GetPeak();
            pulseQ_simp[Npulses_cfd]=apulsesimp.GetCharge();
          }


          Npulses_cfd++;
          Npulses_simp++;
        }


      }


    if(ClusterTreeVerbosity>1)cout<<"FILLING TREE WITH HITS"<<endl;
    fMyTree->Fill();

    }
    if(ClusterTreeVerbosity>0) cout<<"End ClusterTree..............................."<<endl;

  return true;
}


bool ClusterTree::Finalise(){

  nottf->cd();
  fMyTree->Write();



  nottf->Close();
  return true;
}
