#include "LAPPDOtherSimp.h"

LAPPDOtherSimp::LAPPDOtherSimp():Tool(){}


bool LAPPDOtherSimp::Initialise(std::string configfile, DataModel &data){

  /////////////////// Useful header ///////////////////////
  if(configfile!="") m_variables.Initialise(configfile); // loading config file
  //m_variables.Print();

  m_data= &data; //assigning transient data pointer
  /////////////////////////////////////////////////////////////////

  TString IWL;
  m_variables.Get("otherSimpInputWavLabel",IWL);
  InputWavLabel = IWL;

  TString OWL;
  m_variables.Get("otherSimpOutputWavLabel",OWL);
  OutputWavLabel = OWL;

  m_data->Stores["ANNIEEvent"]->Header->Get("AnnieGeometry", geom);

  m_variables.Get("OSrange1",range1);
  m_variables.Get("OSrange2",range2);
  m_variables.Get("OSstrip1",rangeStrip1);
  m_variables.Get("OSstrip2",rangeStrip2);
  m_variables.Get("OSstrip3",rangeStrip3);
  m_variables.Get("OSstrip4",rangeStrip4);
  m_variables.Get("OSstrip5",rangeStrip5);

  nnlsOption = 0;
  m_variables.Get("OSnnlsOption",nnlsOption);

  m_variables.Get("baselineOption",baselineOption);
  if(baselineOption==1){
  m_variables.Get("OtherSimpFileName",TFname);
  OtherSimpFile = new TFile(TFname,"RECREATE");
  OSTree= new TTree("OSTree","OSTree");
  OSTree->Branch("stripN",            &stripN,             "stripN/I");
  OSTree->Branch("sAmp",            &sAmp,             "sAmp/D");
  OSTree->Branch("sTime",            &sTime,             "sTime/D");
  BSHistoCombine = new TH2D("th2C","th2C",60,0,60,40,-20,20);
  }

  return true;
}


bool LAPPDOtherSimp::Execute(){



  std::map<unsigned long, vector<Waveform<double>>> WaveData;
  m_data->Stores["ANNIEEvent"]->Get(InputWavLabel, WaveData);
  vector<vector<double>> Vec0(30 , vector<double>(256));
  vector<vector<double>> Vec1(30 , vector<double>(256));
  ConvertMapToVec(WaveData,Vec0,Vec1,geom);
  vector<double> rangeAmp0, rangeAmp1, rangeTime0, rangeTime1;



/*
  if(nnlsOption==1){
    std::map<unsigned long, vector<Waveform<double>>> nnlsData;
    m_data->Stores["ANNIEEvent"]->Get(nnlsInputWavLabel, nnlsData);
    vector<vector<double>> Vec0nnls(30 , vector<double>(256));
    vector<vector<double>> Vec1nnls(30 , vector<double>(256));
    ConvertMapToVec(nnlsData,Vec0nnls,Vec1nnls,geom);
    vector<double> pulseAmp0, pulseAmp1, pulseTime0, pulseTime1;
  }*/

    //max raw amp in a range
    //FindMaxInRange(Vec0, Vec1, range1, range2, rangeStrip1, rangeAmp0, rangeAmp1, rangeTime0, rangeTime1 );
    //FindMaxInRange(Vec0, Vec1, range1, range2, rangeStrip2, rangeAmp0, rangeAmp1, rangeTime0, rangeTime1 );
    //FindMaxInRange(Vec0, Vec1, range1, range2, rangeStrip3, rangeAmp0, rangeAmp1, rangeTime0, rangeTime1 );
    //FindMaxInRange(Vec0, Vec1, range1, range2, rangeStrip4, rangeAmp0, rangeAmp1, rangeTime0, rangeTime1 );
    //FindMaxInRange(Vec0, Vec1, range1, range2, rangeStrip5, rangeAmp0, rangeAmp1, rangeTime0, rangeTime1 );
    double maxv1, maxv2;
    GetGMax(Vec0, maxv1);
    GetGMax(Vec1, maxv2);


    if(baselineOption==1){

    TH2D* BaselineHisto = new TH2D("th2","th2",60,0,60,40,-20,20);
    map<unsigned long, vector<Waveform<double>>> :: iterator itr;
    for (itr = WaveData.begin(); itr != WaveData.end(); ++itr){
      int channelno = itr->first;
      if(channelno == 1005 ||channelno == 1035  )continue;
      Channel* chan = geom->GetChannel(channelno);
      int side = chan->GetStripSide();
      int strip = chan->GetStripNum();
      Waveform<double> wav = itr->second.at(0);

      for(int i =0; i<wav.GetSamples()->size();i++){
        BaselineHisto->Fill(strip+side*30, wav.GetSample(i) );
        stripN=strip;
        sAmp=wav.GetSample(i);
        sTime=i/10;
        OSTree->Fill();
      }
    }

    for (int i = 0;i<BaselineHisto->GetNbinsX()*BaselineHisto->GetNbinsY();i++){
      BSHistoCombine->SetBinContent(i,BaselineHisto->GetBinContent(i)+BSHistoCombine->GetBinContent(i));
    }

    delete BaselineHisto;
    }



    //overall amp

  vector<double> trigMax = {maxv1,maxv2};
  vector<vector<double>> info = {rangeAmp0, rangeAmp1, rangeTime0, rangeTime1};
  m_data->Stores["ANNIEEvent"]->Set("OtherSimpVec",info);
  m_data->Stores["ANNIEEvent"]->Set("OtherSimpTrigMax",trigMax);


  return true;
}


bool LAPPDOtherSimp::Finalise(){
  if(baselineOption==1){
  OtherSimpFile->cd();
  BSHistoCombine->Write();
  OSTree->Write();
  OtherSimpFile->Close();
  }

  return true;
}

bool LAPPDOtherSimp::FindMaxInRange(vector<vector<double>> &Vec0, vector<vector<double>> &Vec1, int range1, int range2, int rangeStrip, vector<double> &rangeAmp0, vector<double> &rangeAmp1, vector<double> &rangeTime0, vector<double> &rangeTime1 ){
cout<<"break a"<<endl;
  int i = rangeStrip;
  double maxA0 = -50;//Vec0.at(i).at(range1);
  double timeA0 = range1;
cout<<"break g"<<endl;
  for (int j = range1;j<range2;j++){
    if(Vec0.at(i).at(j)>maxA0) {
      maxA0=Vec1.at(i).at(j);
      timeA0=j;}
  }
cout<<"break b"<<endl;
  rangeAmp0.push_back(maxA0);
  rangeTime0.push_back(timeA0/10);
cout<<"break c"<<endl;
  double maxA1 = -50; //Vec1.at(i).at(range1);
  double timeA1 = range1;
cout<<"break d"<<endl;
  for (int j = range1;j<range2;j++){
    if(Vec1.at(i).at(j)>maxA1) {
      maxA1=Vec1.at(i).at(j);
      timeA1=j;}
  }
cout<<"break e"<<endl;
  rangeAmp1.push_back(maxA1);
  rangeTime1.push_back(timeA1/10);
cout<<"break f"<<endl;
  return true;
}


bool LAPPDOtherSimp::ConvertMapToVec( map<unsigned long, vector<Waveform<double>>> &WaveData, vector<vector<double>> &Vec0, vector<vector<double>> &Vec1, Geometry *geom){
  map<unsigned long, vector<Waveform<double>>> :: iterator itr;

  //fill vec, original nnls
  for (itr = WaveData.begin(); itr != WaveData.end(); ++itr){
    int channelno = itr->first;
    if(channelno == 1005 ||channelno == 1035  )continue;

    Channel* chan = geom->GetChannel(channelno);
    int side = chan->GetStripSide();
    int strip = chan->GetStripNum();
    Waveform<double> wav = itr->second.at(0);

    if(side==0){
      for (int i =0; i<wav.GetSamples()->size();i++){
        int j = strip;
        Vec0.at(j).at(i) = -(wav.GetSample(i));
      }}else{
      for ( int i =0; i<wav.GetSamples()->size();i++){
        int j = strip;
        Vec1.at(j).at(i) = -(wav.GetSample(i));
      }}
    }//end channel seletc

    return true;
}//end func

bool LAPPDOtherSimp::GetGMax(vector<vector<double>> &Vec0, double &maxv){
  vector<double>maxs;
  for (int i =0;i<Vec0.size();i++){
    vector<double>::iterator r;
    r = std::max_element(Vec0.at(i).begin(),Vec0.at(i).end());
    maxs.push_back(*r);
  }

  vector<double>::iterator rs;
  rs = std::max_element(maxs.begin(),maxs.end());
  maxv = *rs;
  return true;
}
