#include "LAPPDCluster.h"

LAPPDCluster::LAPPDCluster():Tool(),_geom(nullptr){}


bool LAPPDCluster::Initialise(std::string configfile, DataModel &data){

  /////////////////// Useful header ///////////////////////
  if(configfile!="") m_variables.Initialise(configfile); // loading config file
  //m_variables.Print();

  m_data= &data; //assigning transient data pointer
  /////////////////////////////////////////////////////////////////

  m_data->Stores["ANNIEEvent"]->Header->Get("AnnieGeometry", _geom);
    //cout<<"loaded Geom"<<endl;
  TString SCL;
  m_variables.Get("SimpleClusterLabel",SCL);
    //cout<<"GKLDAJGAFSFD"<<CL<<endl;
    SimpleClusterLabel = SCL;
    TString CFDCL;
    m_variables.Get("CFDClusterLabel",CFDCL);
    CFDClusterLabel = CFDCL;

    TString HOL;
    m_variables.Get("HitOutLabel",HOL);
    HitOutLabel = HOL;

    m_variables.Get("ClusterVerbosity",ClusterVerbosity);


    //cout<<ClusterLabel<<endl;

  return true;
}


bool LAPPDCluster::Execute(){

  if(ClusterVerbosity>0) cout<<"executing lappdcluster!!!"<<endl;
  bool isCFD;
  m_data->Stores["ANNIEEvent"]->Get("isCFD",isCFD);
  std::map <unsigned long, vector<LAPPDPulse>> RecoLAPPDPulses;
  //cout<< "ClusterLabel  "<<ClusterLabel<<endl;
  //cout<<"hmmm "<<isCFD<<" "<<HitOutLabel<<endl;
  if(isCFD==true){
    m_data->Stores["ANNIEEvent"]->Get(CFDClusterLabel,RecoLAPPDPulses);
    if(ClusterVerbosity>0) cout<<"CFD "<<CFDClusterLabel<<" "<<HitOutLabel<<endl;
  }
  else if(isCFD==false){
    m_data->Stores["ANNIEEvent"]->Get(SimpleClusterLabel,RecoLAPPDPulses);
    if(ClusterVerbosity>0) cout<<"Simple "<<SimpleClusterLabel<<" "<<HitOutLabel<<endl;
  }

  if(ClusterVerbosity>0) cout<<"Number of Pulses: "<<RecoLAPPDPulses.size()<<endl;
  //m_data->Stores["ANNIEEvent"]->Get("RecoLAPPDPulses",RecoLAPPDPulses);
    //cout<<"We got "<< RecoLAPPDPulses.size()<< " Pulses" << endl;
  std::map <unsigned long, vector<LAPPDPulse>> :: iterator pulseitr;


  //cout<<"Grabbed RecoLAPPDPulses "<<RecoLAPPDPulses.size()<<endl;

  vector<unsigned long> chanhand;
  std::map <unsigned long, vector<LAPPDHit>> Hits;



  //cout<<"!!!That is all pulses we have!!!"<<endl;
  for (pulseitr = RecoLAPPDPulses.begin(); pulseitr != RecoLAPPDPulses.end(); ++pulseitr){
    vector<LAPPDHit> thehits;
    vector<double> localposition;
    double sition=-5555;
    double PerpPosition=-5555;
    double ParaPosition=-5555;


    unsigned long chankey = pulseitr->first;

    bool handled = false;
    for(int i=0; i<chanhand.size(); ++i)
      {
        unsigned long key = chanhand[i];
        if(chankey==key)
        {
          handled = true;
        }
      }
      if (handled)
      {
        //cout<<chankey<<" is looped."<<endl;
        continue;
      }

    //cout<<"!@##^&&%%$pulseitr chankey is "<<chankey<<endl;

    vector<LAPPDPulse> vPulse = pulseitr->second;

    // cout<<"iterating!!!   "<<chankey<<" "<<vPulse.size()<<endl;

    //if(vPulse.size()>1) {cout<<"VPULSE HAS A SIZE OF: "<<vPulse.size()<<endl;}
    for(int jj=0; jj<vPulse.size(); jj++){
      LAPPDPulse apulse = vPulse.at(jj);
      // cout<<"the charge of this pulse is: "<<apulse.GetCharge()<<endl;
      //cout<< "The Time of this Pulse is " <<apulse.GetTime() <<endl;
    }

    Channel* mychannel= _geom->GetChannel(chankey);
   // cout<<"the strip number is :"<<mychannel->GetStripNum()<<" and the side is: "<<mychannel->GetStripSide()<<endl;
    std::map<unsigned long , LAPPDPulse> cPulse;
    std::map <unsigned long, vector<LAPPDPulse>> :: iterator oppoitr;

    for (oppoitr = RecoLAPPDPulses.begin(); oppoitr != RecoLAPPDPulses.end(); ++oppoitr){

      unsigned long oppochankey = oppoitr->first;

      //cout<<"oppochankey is "<<oppochankey<<endl;

      vector<LAPPDPulse> oppovPulse = oppoitr->second;
      Channel* oppochannel = _geom->GetChannel(oppochankey);
      int mystripnum = mychannel->GetStripNum();
      int oppostripnum = oppochannel->GetStripNum();

      if(ClusterVerbosity>1) cout<<"mystripnum is "<<mystripnum<<endl;
      if(ClusterVerbosity>1) cout<<"oppostripnum is "<<oppostripnum<<endl;



      if( (oppochankey != chankey) && (mystripnum == oppostripnum) ){
        if(ClusterVerbosity>2) cout<<"channel "<<chankey<<" and "<<oppochankey<<" are on the same strip."<<endl;
        cPulse.insert(pair <unsigned long,LAPPDPulse> (chankey,vPulse.at(0)));
        cPulse.insert(pair <unsigned long,LAPPDPulse> (oppochankey,oppovPulse.at(0)));
        //chanhand.push_back(oppochankey);
      }
      if ( (oppochankey != chankey) && (std::abs(oppostripnum-mystripnum)==1) ){
        if (mychannel->GetStripSide() == oppochannel->GetStripSide()){
          if(ClusterVerbosity>2) cout<<"channel "<<chankey<<" and "<<oppochankey<<" are on the same sides of adjacent strips."<<endl;
          cPulse.insert(pair <unsigned long,LAPPDPulse> (oppochankey,oppovPulse.at(0)));
          //chanhand.push_back(oppochankey);
        }
        else {
          if(ClusterVerbosity>2) cout<<"channel "<<chankey<<" and "<<oppochankey<<" are on the opposite sides of adjacent strips."<<endl;
          cPulse.insert(pair <unsigned long,LAPPDPulse> (oppochankey,oppovPulse.at(0)));
          //chanhand.push_back(oppochankey);
        }
      }

    }

    //Finding the Maxpulse
    //cout<<"Finding Maxpulse!!!"<<endl;
    double maxcharge = 0;
    unsigned long maxchankey=0;
    LAPPDPulse maxpulse;

    std::map<unsigned long , LAPPDPulse> :: iterator itr;

    //cout<<"What is in cPulse???"<<endl;
    //for (itr = cPulse.begin(); itr != cPulse.end(); ++itr){
      //cout<< itr->first <<endl;
      //cout<< itr->second.GetTime() <<endl;
    //}

    for (itr = cPulse.begin(); itr != cPulse.end(); ++itr){
      unsigned long mychankey = itr->first;
      LAPPDPulse mypulse = itr->second;
      //cout<<"!!!mychankey is "<<mychankey<<" !!!"<<endl;
      if (maxcharge>mypulse.GetCharge()){ // Pulses are negative
        maxcharge=mypulse.GetCharge();
        //cout<<"maxcharge "<<mypulse.GetCharge()<<endl;
        maxchankey = mychankey;
        maxpulse = mypulse;
      }
    }

    //cout<<maxchankey<<" is the maxchankey!!!!!!"<<endl;
    //cout<<"chankey is "<<chankey<<" !!!!!!"<<endl;



    if (maxchankey == chankey){
      //cout<<chankey<<" is the maxchankey!!!!!!"<<endl;
      for (itr = cPulse.begin(); itr != cPulse.end(); ++itr){
        chanhand.push_back(itr->first);
      }
    }
    else {
      //cout<<chankey<<" is not the maxchankey LOOP BREAKS"<<endl;
      continue;
    }


    maxcharge = maxpulse.GetCharge();

    //cout<<"the maxchankey is "<<maxchankey<<endl;
    //cout<<"Maxpulse is at "<<maxpulse.GetChannelID()<<endl;

    Channel* maxchannel= _geom->GetChannel(maxchankey);
    std::map<int,double> neighbourpulses;

    for (itr = cPulse.begin(); itr != cPulse.end(); ++itr){
      unsigned long thechankey = itr->first;
      LAPPDPulse mypulse = itr->second;

      Channel* mychannel = _geom->GetChannel(thechankey);
      if(ClusterVerbosity>2) cout<<"thechankey "<<thechankey<<" maxchankey "<<maxchankey<<endl;
      if(ClusterVerbosity>2) cout<<"MYStrip "<<mychannel->GetStripNum()<<" MAXStrip "<<maxchannel->GetStripNum()<<endl;
      if(ClusterVerbosity>2) cout<<" "<<endl;
      if( (thechankey != maxchankey) && (mychannel->GetStripNum() == maxchannel->GetStripNum()) ) {
        if(ClusterVerbosity>2) cout<<"WERTWER "<<maxpulse.GetTime()<<" "<<mypulse.GetTime()<<endl;
        if(ClusterVerbosity>2)cout<<mychannel->GetStripSide()<<" "<<maxchannel->GetStripSide()<<endl;
        if(ClusterVerbosity>2) cout<<" "<<endl;
        if ( (mychannel->GetStripSide()==0) && (maxchannel->GetStripSide()==1) ){
          if(ClusterVerbosity>2) cout<<"case 1"<<endl;
          ParaPosition = ((mypulse.GetTime() - maxpulse.GetTime()) * 0.53 * (299.792458))/2.0;
        }
        if ( (mychannel->GetStripSide()==1) && (maxchannel->GetStripSide()==0) ){
          if(ClusterVerbosity>2) cout<<"case 2"<<endl;
          ParaPosition = ((maxpulse.GetTime() - mypulse.GetTime()) * 0.53 * (299.792458))/2.0;
        }
        //cout<<leftpulse.GetTime()<<" "<<rightpulse.GetTime()<<endl;
      }
      if( (thechankey != maxchankey) && (abs(mychannel->GetStripNum() - maxchannel->GetStripNum()) == 1) && (mychannel->GetStripSide() == maxchannel->GetStripSide()) ) {
        neighbourpulses.insert(pair <int,double> (mychannel->GetStripNum(),mypulse.GetPeak()));
      }
    }
    neighbourpulses.insert(pair <int,double> (maxchannel->GetStripNum(),maxpulse.GetPeak()));

    double SumAbove=0.;
    double SumBelow=0.;

    if(neighbourpulses.size()>1){
      std::map<int,double>::iterator neighbouritr;
      for (neighbouritr = neighbourpulses.begin(); neighbouritr != neighbourpulses.end(); ++neighbouritr){
        int Strip = neighbouritr->first;
        double Peak = neighbouritr->second;
        //cout<<"!!!!!!prepcalculation "<<Strip<<" "<<Peak<<endl;
        SumAbove += ((double)Strip*Peak);
        SumBelow += (Peak);
      }
      if(SumBelow>0) {PerpPosition = (SumAbove / SumBelow);}
    }
    else {
      PerpPosition = (double) maxchannel->GetStripNum();
    }

    //cout<<"Positions: "<<ParaPosition<<" "<<PerpPosition<<endl;
    localposition.push_back(ParaPosition);
    localposition.push_back(PerpPosition);


    //Putting information into LAPPDHit
    LAPPDHit myhit;
    myhit.SetTubeId(maxpulse.GetTubeId());
    myhit.SetTime(maxpulse.GetTime());
    myhit.SetCharge(maxcharge);
    myhit.SetLocalPosition(localposition);
    thehits.push_back(myhit);
    Hits.insert(pair <unsigned long,vector<LAPPDHit>> (chankey,thehits));
    chanhand.push_back(chankey);
    //cout<< "HANDLED" << endl;



  }
  if(ClusterVerbosity>0) cout << "Ending LAPPDCluster: " << Hits.size()<< endl;
  //m_data->Stores["ANNIEEvent"]->Set("Clusters",Hits);
  m_data->Stores["ANNIEEvent"]->Set(HitOutLabel,Hits);

  return true;
}


bool LAPPDCluster::Finalise(){

  return true;
}
