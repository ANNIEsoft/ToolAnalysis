#include "LAPPDLocateHit.h"

LAPPDLocateHit::LAPPDLocateHit():Tool(){}


bool LAPPDLocateHit::Initialise(std::string configfile, DataModel &data){
  cout<<"start locateHit"<<endl;
  /////////////////// Useful header ///////////////////////
  if(configfile!="") m_variables.Initialise(configfile); // loading config file
  //m_variables.Print();

  m_data= &data; //assigning transient data pointer
  /////////////////////////////////////////////////////////////////
  TString IWL,OWL,RIWL;
  m_variables.Get("LocateInputWavLabel",IWL);
  InputWavLabel = IWL;
  m_variables.Get("LocateOutputWavLabel",OWL);
  OutputWavLabel = OWL;
  m_variables.Get("LocateRawInputWaveLabel",RIWL);
  RawInputWavLabel = RIWL;
  m_variables.Get("peakLowThreshold",peakLowThreshold);
  m_variables.Get("peakHighThreshold",peakHighThreshold);
  m_variables.Get("LAPPDLocateHitVerbosity",LAPPDLocateHitVerbosity);
  m_variables.Get("sigma",sigma);
  m_variables.Get("LocateHitLabel",LocateHitLabel);
  m_variables.Get("sampling_factor", nnlsTimeStep);
  m_variables.Get("Timebinsize2D",Timebinsize2D);
  TString positionFileName = "Position.root";
  m_variables.Get("positionFileName",positionFileName);
  TString positionTreeFileName = "Position.root";
  m_variables.Get("positionTreeFileName",positionTreeFileName);
  m_variables.Get("SingleEventPlot",SingleEventPlot);
  m_variables.Get("plotOption",plotOption);
  m_variables.Get("colorNumber",colorNumber);
  m_variables.Get("sizeNumber",sizeNumber);
  m_variables.Get("colorScaleN",colorScaleN);
  m_variables.Get("colorNumberstart",colorNumberstart);
  m_variables.Get("colorNumberend",colorNumberend);
  m_variables.Get("beamPlotOption",beamPlotOption);
  TString PlotSuffix = "plot";
  m_variables.Get("DisplayPDFSuffix",DisplayPDFSuffix);
  PlotSuffix = DisplayPDFSuffix;
  m_variables.Get("plotStripVert",plotStripVert);

  positionFile = new TFile(positionFileName, "RECREATE");

  m_data->Stores["ANNIEEvent"]->Header->Get("AnnieGeometry", geom);

  std::map<int, int> event0Channel;
  std::map<int, int> event1Channel;

  for(int i =0; i<60;i++){ //fill the two strip number map, for finding the same strip later
    Channel* c = geom->GetChannel(i);
    int strip = c->GetStripNum();
    int side = c->GetStripSide();
    if(side==0) event0Channel.insert(pair<int,int>(strip,i));
    if(side==1) event1Channel.insert(pair<int,int>(strip,i));
  }

  EventItr=0;
  m_variables.Get("StripLength",StripLength);
  m_variables.Get("signalSpeed",signalSpeed);
  positionHistOff = new TH2D("PositionsOff","PositionsOff",200,-0.05,20.05 ,30,-0.5,29.5);
  positionHistIn = new TH2D("PositionsIn","PositionsIn",200,-0.05,20.05 ,30,-0.5,29.5);
  timeEventsTotal = new TH2D("EventsTimeTotal","EventsTimeTotal",(int)40/Timebinsize2D,-0.05,20.05 ,30,-0.5,29.5);

  m_variables.Get("histoBinNumber",hBN);
  AmpVSnAreaS = new TH2D("AmpVSnAreaS","AmpVSnAreaS",hBN,0,10,hBN,0,300);
  AmpVSnAreaD = new TH2D("AmpVSnAreaD","AmpVSnAreaD",hBN,0,10,hBN,-50,50);
  AmpVSpAreaS = new TH2D("AmpVSpAreaS","AmpVSpAreaS",hBN,0,150,hBN,0,2000);
  AmpVSpAreaD = new TH2D("AmpVSpAreaD","AmpVSpAreaD",hBN,0,150,hBN,-200,200);
  nAmpVSpAmp = new TH2D("nAmpVSpAmp","nAmpVSpAmp",hBN,0,10,hBN,0,150);
  nAmpVSpAreaS = new TH2D("nAmpVSpAreaS","nAmpVSpAreaS",hBN,0,10,hBN,0,2000);
  nAmpVSpASoT = new TH2D("nAmpVSpASoT","nAmpVSpASoT",hBN,0,10,hBN,0,10);

/*
AmpVSnAreaS = new TH2D("AmpVSnAreaS","AmpVSnAreaS",100,-1000,1000,100,-2000,2000);
AmpVSnAreaD = new TH2D("AmpVSnAreaD","AmpVSnAreaD",100,-1000,1000,100,-2000,2000);
AmpVSpAreaS = new TH2D("AmpVSpAreaS","AmpVSpAreaS",100,-1000,1000,100,-2000,2000);
AmpVSpAreaD = new TH2D("AmpVSpAreaD","AmpVSpAreaD",100,-1000,1000,100,-2000,2000);
nAmpVSpAmp = new TH2D("nAmpVSpAmp","nAmpVSpAmp",100,-1000,1000,100,-2000,2000);
*/
  BeamTvsPIn = new TH3D("BeamTvsPIn","BeamTvsPIn",hBN,0,22,hBN,0,20,hBN,0,30);
  BeamTvsPOut = new TH3D("BeamTvsPOut","BeamTvsPOut",hBN,0,22,hBN,0,20,hBN,0,30);

  PositionTree = new TTree("PositionTree","PositionTree");
  positionTreeFile = new TFile(positionTreeFileName, "RECREATE");
  PositionTree->Branch("testRatio1",&testRatio1,"testRatio1/D");
  PositionTree->Branch("testRatio2",&testRatio2,"testRatio2/D");

  vector<double> nnlsAmp;
  vector<double> nnlsArea;
  vector<double> pulseGain;
  int zbin = int (peakHighThreshold - peakLowThreshold) +1;
  //positionHist3D = new TH3D("Position3D","Position3D",200,-0.05,20.05 ,30,-0.5,29.5, zbin,int(peakLowThreshold), int(peakHighThreshold)+1);
  cout<<"end locateHit"<<endl;

  AbnormalEvents = 0 ;
  return true;
}


bool LAPPDLocateHit::Execute(){
  EventItr++;
  positionFile->cd();


  TString timeEName;
  timeEName += "TimeEvent";
  timeEName += EventItr;
  m_data->Stores["ANNIEEvent"]->Get("BeamTime", BeamTime);
  if(BeamTime>7.5&&BeamTime<9.5) timeEName += "_inBeam";
  TH2D *timeEvents = new TH2D(timeEName,timeEName,(int)40/Timebinsize2D,-0.05,20.05 ,30,-0.5,29.5);
  //cout<<"start execute"<<endl;
  std::map<unsigned long, vector<Waveform<double>>> nnlsResultInWaveform; // nnls result read from LAPPDnnlsPeak
  std::map<unsigned long, vector<Waveform<double>>> RawnnlsWaveform;
  m_data->Stores["ANNIEEvent"]->Get(InputWavLabel, nnlsResultInWaveform);
  m_data->Stores["ANNIEEvent"]->Get(RawInputWavLabel, RawnnlsWaveform);
  //cout<<"nnls size "<<nnlsResultInWaveform.size()<<endl;
  // loop over all channels
  std::map<unsigned long, vector<Waveform<double>>> :: iterator itr;

  std::map<int, vector<vector<double>>> PeaksOnSide0; //pairs of pulses on int-strip number, vector [time, height]
  std::map<int, vector<vector<double>>> PeaksOnSide1;
  std::map<int,int> eventsOnChannel;
  eventsOnChannel.insert(pair<int,int>(1000,0)); //fill a ramdom pair, incase there is no signal on this channel

  for (itr = nnlsResultInWaveform.begin(); itr != nnlsResultInWaveform.end(); ++itr){

      unsigned long channelno = itr->first;
      vector<Waveform<double>> Vwavs = itr->second;
      vector<Waveform<double>> RVwavs = RawnnlsWaveform.find(channelno)->second;

      Channel* mychannel = geom->GetChannel(channelno);
      //figure out the stripline number
      int stripno1 = mychannel->GetStripNum();
      //figure out the side of the stripline
      int stripside1 = mychannel->GetStripSide();
      vector<double> PeakHeight;
      vector<int> PeakTimeLoc;
      vector<double> PeakArea;
      vector<double> PulseArea;
      vector<double> PulseHeight;

      //loop over all waveforms in this channel, find the local peaks
      for (int i = 0; i<Vwavs.size(); i++){
        //vector<double> LocalPeak = FindlocalPeak(Vwavs.at(i));
        if(channelno == 1005 || channelno == 1035) break;
        int eventN = 0;
        bool loopR = FindlocalPeaks(Vwavs.at(i),RVwavs.at(i),PeakHeight,PeakTimeLoc,PeakArea,PulseArea,PulseHeight,stripno1,peakLowThreshold,peakHighThreshold,nnlsTimeStep);
        if (LAPPDLocateHitVerbosity>1) cout<<"finding on "<<channelno<<endl;
        if(loopR == true){ //normal means no peak higher than high threshold, some unknown error from nnls. should be removed in the future
          if(stripside1 == 0){  // push samples to side 0
            if(LAPPDLocateHitVerbosity>1) cout<<"side0 "<<endl;
              vector<vector<double>> x;
              vector<double> loc;
              vector<double> hei;
              vector<double> nnlsA;
              vector<double> pulseA;
              vector<double> phei;
              //cout<<"peakheight size "<<PeakHeight.size()<<endl;
              for (int i=0;i<PeakHeight.size();i++) {
                  //x = [peak time on this strip, peak heights on this strip]

                loc.push_back(PeakTimeLoc.at(i));
                hei.push_back(PeakHeight.at(i));
                nnlsA.push_back(PeakArea.at(i));
                pulseA.push_back(PulseArea.at(i));
                phei.push_back(PulseHeight.at(i));
              }
                x.push_back(loc);
                x.push_back(hei);
                x.push_back(nnlsA);
                x.push_back(pulseA);
                x.push_back(phei);
                PeaksOnSide0.insert(pair<int,vector<vector<double>>>(stripno1,x));
                //for (int kkk=0;kkk<loc.size();kkk++) cout<<"0element "<<loc.at(kkk)<<endl;
                eventN += loc.size();
                loc.clear();
                hei.clear();
                nnlsA.clear();
                pulseA.clear();
                phei.clear();
                x.clear();

                if(LAPPDLocateHitVerbosity>1) cout<<"In side 0 strip "<<stripno1<<" "<<eventsOnChannel.count(stripno1)<<" size "<<PeakHeight.size()<<endl;

          }else if(stripside1 == 1){ // push samples to side 1
            if(LAPPDLocateHitVerbosity>1) cout<<"side1"<<endl;
            vector<vector<double>> x;
            vector<double> loc;
            vector<double> hei;
            vector<double> nnlsA;
            vector<double> pulseA;
            vector<double> phei;
            for (int i=0;i<PeakHeight.size();i++) {
                //x = [peak time on this strip, peak heights on this strip]
              loc.push_back(PeakTimeLoc.at(i));
              hei.push_back(PeakHeight.at(i));
              nnlsA.push_back(PeakArea.at(i));
              pulseA.push_back(PulseArea.at(i));
              phei.push_back(PulseHeight.at(i));
            }
              x.push_back(loc);
              x.push_back(hei);
              x.push_back(nnlsA);
              x.push_back(pulseA);
              x.push_back(phei);
              PeaksOnSide1.insert(pair<int,vector<vector<double>>>(stripno1,x));
              //for (int kkk=0;kkk<loc.size();kkk++) cout<<"1element "<<loc.at(kkk)<<endl;
              eventN += loc.size();
              loc.clear();
              hei.clear();
              nnlsA.clear();
              pulseA.clear();
              phei.clear();
              x.clear();

              if(LAPPDLocateHitVerbosity>1) cout<<"In side 1 strip "<<stripno1<<" "<<eventsOnChannel.count(stripno1)<<" size "<<PeakHeight.size()<<endl;
          }else{cout<<"pulse not fill in side 0 or 1 at channel "<<channelno<<" on event "<<EventItr<<endl;}

        }
        if(LAPPDLocateHitVerbosity>0) cout<<"strip "<<stripno1<<" "<<eventsOnChannel.count(stripno1)<<" pulse number "<<PeakHeight.size()<<endl;
        if (eventsOnChannel.count(stripno1)==0){
          eventsOnChannel.insert(pair<int,int>(stripno1,eventN));
        }else{
          int prev = eventsOnChannel.find(stripno1)->second;
          if (prev > eventN) eventsOnChannel.find(stripno1)->second = eventN;
        }
      }//end looping all waveforms
      //cout<<"finish looping"<<endl;


  }

  //now we have fill the peaks on side 1 and side 0. start to match them
    map<int, vector<vector<double>>> OrderedPeaks; //use to save all matched peaks as pair (strip number, vector of all position)

  if(PeaksOnSide0.size()!=PeaksOnSide1.size()){
    if(LAPPDLocateHitVerbosity>0) cout<<"find "<<PeaksOnSide0.size()<<"on side0, "<<PeaksOnSide1.size()<<"on side1."<<endl;
  }

  //looping all strips
  //**************!!!really stupid factorial, need to be optimized

  for (int strip = 1; strip<28 ;strip++){
    if(LAPPDLocateHitVerbosity>1) cout<<"strip "<<strip<<", event "<<eventsOnChannel.find(strip)->second<<endl;
    if (eventsOnChannel.find(strip)->second !=0){

    vector<vector<double>> e0 = PeaksOnSide0.find(strip)->second;
    vector<vector<double>> e1 = PeaksOnSide1.find(strip)->second;
    if(LAPPDLocateHitVerbosity>0) cout<<"ordering strip "<<strip<<" side0 "<<e0.at(1).size()<<" side1 "<<e1.at(1).size()<<endl;
    vector<int> permute0;
    vector<int> permute1;
    for(int k = 0;k<e0.at(1).size();k++){  //we already have e0 size = e1 size
        permute0.push_back(k);}
    for(int k = 0;k<e1.at(1).size();k++){
        permute1.push_back(k);}
    vector<int> MaxOrder0 = permute0;
    vector<int> MaxOrder1 = permute1;
    double Maxsum = 0.;
    int MaxLoop = permute0.size();
    if (permute1.size()<permute0.size()) {MaxLoop = permute1.size();}

    if(LAPPDLocateHitVerbosity>0) cout<<"MaxLoop "<<MaxLoop<<endl;
    if (MaxLoop > 0){
//cout<<"problem here 0"<<endl;
    if (permute1.size()<permute0.size()) MaxLoop = permute1.size();
  for(int j=0;j<MaxLoop;j++){  //1st
    double pr = MatchingProduct(e0.at(1).at(j), e1.at(1).at(j), sigma); //vector [time, height]
    Maxsum = Maxsum + pr;
  }
//cout<<"problem here 1"<<endl;
  do {
    do{
    double Sum = 0.;
    for(int j=0;j<MaxLoop;j++){
      Sum+=MatchingProduct(e0.at(1).at(permute0.at(j)), e1.at(1).at(permute1.at(j)), sigma);
    }
    if(Sum>Maxsum){
      MaxOrder0 = permute0;
      MaxOrder1 = permute1;
      Maxsum = Sum;
    }
    //cout<<"problem here in0"<<endl;
  }while(next_permutation(permute0.begin(),permute0.end()));
  //cout<<"problem here in1"<<endl;
}while(next_permutation(permute1.begin(),permute1.end()));
//  //now, use the max order to pair pulse;
  vector<double> Position;
  vector<double> Height;
  vector<double> AbsTime;
  vector<double> nnlsAreaS;
  vector<double> nnlsAreaD;
  vector<double> pulseAreaS;
  vector<double> pulseAreaD;
  vector<double> pulseHeight;
  vector<vector<double>> PandH;
//int Num = MaxOrder0.size();
//if (MaxOrder1.size()<MaxOrder0.size()) Num = MaxOrder1.size();
//cout<<"position "<<Position.size()<<" "<<MaxOrder0.size()<<" "<<MaxOrder1.size()<<endl;

for (int p = 0; p<MaxLoop;p++){
  double dT = e0.at(0).at(MaxOrder0.at(p)) - e1.at(0).at(MaxOrder1.at(p));    //**** add height number here
  double SumHeight = e0.at(1).at(MaxOrder0.at(p)) + e1.at(1).at(MaxOrder1.at(p));
  double absTimeIn = (e0.at(0).at(MaxOrder0.at(p)) + e1.at(0).at(MaxOrder1.at(p)))/2/1000*nnlsTimeStep - 0.5 ;
  double nnlsArS = e0.at(2).at(MaxOrder0.at(p)) + e1.at(2).at(MaxOrder1.at(p));
  double nnlsArD = e0.at(2).at(MaxOrder0.at(p)) - e1.at(2).at(MaxOrder1.at(p));
  double pulseArS = e0.at(3).at(MaxOrder0.at(p)) + e1.at(3).at(MaxOrder1.at(p));
  double pulseArD = e0.at(3).at(MaxOrder0.at(p)) - e1.at(3).at(MaxOrder1.at(p));
  double SumpulseHei = e0.at(4).at(MaxOrder0.at(p)) + e1.at(4).at(MaxOrder1.at(p));
  dT = dT * nnlsTimeStep / 1000;
  if(LAPPDLocateHitVerbosity>1) cout<<"dT= "<<dT<<", StripLength "<<StripLength<<", signalspeed"<<signalSpeed<<endl;
  double pos = (dT * signalSpeed + StripLength)/2*100;
  Position.push_back(pos);
  Height.push_back(SumHeight);
  AbsTime.push_back(absTimeIn);
  nnlsAreaS.push_back(nnlsArS);
  nnlsAreaD.push_back(nnlsArD);
  pulseAreaS.push_back(pulseArS);
  pulseAreaD.push_back(pulseArD);
  pulseHeight.push_back(SumpulseHei);
  if (LAPPDLocateHitVerbosity>0) cout<<"saving positions on "<<strip<<" position "<<pos<<endl;
}
  PandH.push_back(Position);
  PandH.push_back(Height);
  PandH.push_back(AbsTime);
  PandH.push_back(nnlsAreaS);
  PandH.push_back(nnlsAreaD);
  PandH.push_back(pulseAreaS);
  PandH.push_back(pulseAreaD);
  PandH.push_back(pulseHeight);
  OrderedPeaks.insert(pair<int,vector<vector<double>>>(strip,PandH));

e0.clear();
e1.clear();
permute0.clear();
permute1.clear();

}
}
}
  //end else
if (LAPPDLocateHitVerbosity>0) cout<<"Itr "<<OrderedPeaks.size()<<endl;

  std::map <int,vector<vector<double>>> :: iterator itrPlot;
vector<double> DISstripNO;
vector<double> DISposition;
vector<double> DISamp;
vector<double> DISarvTime;
vector<double> DISnnlsAreaS;
vector<double> DISnnlsAreaD;
vector<double> DISpulseAreaS;
vector<double> DISpulseAreaD;
vector<double> DISpulseHeight;


  for (itrPlot = OrderedPeaks.begin(); itrPlot != OrderedPeaks.end(); ++itrPlot){
    int stripNo = itrPlot->first;
    vector<double> position = itrPlot->second.at(0);
    vector<double> height = itrPlot->second.at(1);
    vector<double> time = itrPlot->second.at(2);
    vector<double> nnlsAS = itrPlot->second.at(3);
    vector<double> nnlsAD = itrPlot->second.at(4);
    vector<double> pulseAS = itrPlot->second.at(5);
    vector<double> pulseAD = itrPlot->second.at(6);
    vector<double> rawheight = itrPlot->second.at(7);
    for (int f=0; f<position.size();f++){
      if (BeamTime> 7.5 && BeamTime< 9.5){
        positionHistIn->Fill(position.at(f),stripNo);
        BeamTvsPIn->Fill(BeamTime,position.at(f),stripNo);
      }else{
        positionHistOff->Fill(position.at(f),stripNo);
        BeamTvsPOut->Fill(BeamTime,position.at(f),stripNo);
      }
      timeEvents->Fill(position.at(f),stripNo,time.at(f));
      timeEventsTotal->Fill(position.at(f),stripNo,time.at(f));
      if(LAPPDLocateHitVerbosity>0) cout<<"nh "<<height.at(f)<<", rh "<<rawheight.at(f)<<", nAS "<<nnlsAS.at(f)<<", nAD "<<nnlsAD.at(f)<<", pAS "<<pulseAS.at(f)<<", pAD "<<pulseAD.at(f)<<endl;

/*      AmpVSnAreaS->Fill(-height.at(f), -nnlsAS.at(f));
      AmpVSnAreaD->Fill(-height.at(f), -nnlsAD.at(f));
      AmpVSpAreaS->Fill(-rawheight.at(f), -pulseAS.at(f));
      AmpVSpAreaD->Fill(-rawheight.at(f), -pulseAD.at(f));
      nAmpVSpAmp->Fill(-height.at(f), -rawheight.at(f));
      nAmpVSpAreaS->Fill(-height.at(f),-pulseAS.at(f));
      nAmpVSpASoT->Fill(-height.at(f),-pulseAS.at(f)/700);*/

      if(position.at(f)>20 || position.at(f)<0) AbnormalEvents++;
      if (LAPPDLocateHitVerbosity>1) cout<<"position at "<<position.at(f)<<endl;
      if(LAPPDLocateHitVerbosity>0) cout<<"event "<<EventItr<< ", saving pulse at strip "<<stripNo<<" position "<<position.at(f)<<endl;

      DISstripNO.push_back(stripNo);
      DISposition.push_back(position.at(f));
      DISamp.push_back(height.at(f));
      DISarvTime.push_back(time.at(f));
    }
    //fill the event display TCanvas
  }

int a=0;  //remove points outside of LAPPD to display
for (int i=0;i<DISposition.size();i++){
    if(DISposition.at(i-a) < 0 || DISposition.at(i-a)>20){
        DISstripNO.erase(DISstripNO.begin()+i-a);
        DISposition.erase(DISposition.begin()+i-a);
        DISamp.erase(DISamp.begin()+i-a);
        DISarvTime.erase(DISarvTime.begin()+i-a);
        a++;
    }
}
double peak1, peak2;

// insert the reconstructed positions to the map
std::map<unsigned long,vector<double>> NNLSLocatedHitsMap;
for(int i =0; i <DISposition.size();i++){
  vector<double> info = {DISposition.at(i),DISstripNO.at(i),DISarvTime.at(i),-DISamp.at(i)};
  NNLSLocatedHitsMap.insert(pair<unsigned long,vector<double>>(i,info));

}



for (int i =0; i<DISposition.size(); i++){

  if(DISposition.at(i)>9 && DISposition.at(i)< 11){
    if(LAPPDLocateHitVerbosity>0) cout<<"strip nnn"<<DISstripNO.at(i)<<endl;
    if(DISstripNO.at(i) == 11) peak1 = DISamp.at(i);
    if(DISstripNO.at(i) == 12) peak2 = DISamp.at(i);
  }
}
testRatio1 = peak1/(peak2+peak1);
testRatio2 = peak2/(peak2+peak1);
PositionTree->Fill();

if(DISposition.size()>3){
  if(LAPPDLocateHitVerbosity>0) cout<<"many events here: "<<endl;
  for (int i = 0; i <DISstripNO.size();i++) cout<<DISstripNO.at(i)<<",";
  if(LAPPDLocateHitVerbosity>0) cout<<" DISstripNO "<<endl;
  for (int i = 0;i<DISstripNO.size();i++) cout<<DISposition.at(i)<<",";
  if(LAPPDLocateHitVerbosity>0) cout<<" DISposition "<<endl;
  for (int i = 0;i<DISstripNO.size();i++) cout<<DISamp.at(i)<<",";
  if(LAPPDLocateHitVerbosity>0) cout<<" DISamp "<<endl;
  for (int i = 0;i<DISstripNO.size();i++) cout<<DISarvTime.at(i)<<",";
  if(LAPPDLocateHitVerbosity>0) cout<<" DISarvTime "<<endl;
}

if(beamPlotOption > 0){
  if(BeamTime>7.5&&BeamTime<9.5){
    plotOption = 0;
  }
}




if (plotOption>0 && OrderedPeaks.size()>0){

  double maxT = *max_element(DISarvTime.begin(), DISarvTime.end());
    double minT = *min_element(DISarvTime.begin(), DISarvTime.end());
    double timeScaleC = (colorNumberend-colorNumberstart)/(maxT-minT);

    auto *g0 = new TGraphErrors(); //all x means along strip line, y across strip line
    int pointNo = DISstripNO.size();
    for (int i =0;i<pointNo;i++){
      if (plotStripVert == 1){
        g0->SetPoint(i,((DISstripNO.at(i)-1)*(2.29+4.62)+14.4+2.31)/10,DISposition.at(i)); //resolution position
      }else{
        g0->SetPoint(i,DISposition.at(i),((DISstripNO.at(i)-1)*(2.29+4.62)+14.4+2.31)/10); //resolution position
      }
    g0->SetPointError(i,0.5*0.691,0.43); //resolution size, half width of one stripline + stand deviation of laser spot.
    }
    TString name = "Event_";
    name += EventItr;
    TCanvas *c = new TCanvas(name,name); //draw resolutions on a canvas

      int z = 300;
    c->SetCanvasSize(z*2,z*3);

      TPad *pad1 = new TPad("pad1", "The pad 80% of the height",0.0,0.4,1.0,1.0,-1);
      TPad *pad2 = new TPad("pad2", "The pad 20% of the height",0.0,0.0,1.0,0.4,-1);
      pad1->Draw();
      pad2->Draw();
      pad1->cd();


      g0->GetXaxis()->SetLimits(0,20);

    g0->SetMinimum(0);
    g0->SetMaximum(20);

    g0->Draw("ap");
    if (plotStripVert ==1 ){
      g0->GetXaxis()->SetTitle("AcrossStripLine(cm)");
      g0->GetYaxis()->SetTitle("AlongStripLine(cm)");
    }else{
      g0->GetYaxis()->SetTitle("AcrossStripLine(cm)");
      g0->GetXaxis()->SetTitle("AlongStripLine(cm)");
    }



      TText *tx = new TText();
      tx->SetTextFont(42);
      //tx->SetTextSize(0.05);
      int p1dBins = 10;
      TH1D *profile1d = new TH1D("timeProfile","",p1dBins,minT-0.2*(maxT-minT),maxT+0.2*(maxT-minT));
    for (int i =0;i<pointNo;i++){ //draw markers
      auto *m = new TMarker();
          if (plotStripVert ==1 ){
            m->SetX(((DISstripNO.at(i)-1)*(2.29+4.62)+14.4+2.31)/10);  //marker x position
            m->SetY(DISposition.at(i));  //marker y position
          }else{
            m->SetY(((DISstripNO.at(i)-1)*(2.29+4.62)+14.4+2.31)/10);  //marker x position
            m->SetX(DISposition.at(i));  //marker y position
          }

      m->SetMarkerSize(-DISamp.at(i)*sizeNumber);  //set marker size
      m->SetMarkerStyle(20);  //solid circle
      m->SetMarkerColorAlpha(colorNumberstart+(DISarvTime.at(i)-minT)*timeScaleC, 0.5);  //set marker color, prop to min~max time scale
      m->Draw("P");
      if (plotStripVert ==1 ){
        tx->DrawText(((DISstripNO.at(i)-1)*(2.29+4.62)+14.4+2.31)/10+0.5, DISposition.at(i)-0.5,Form("%i",i));
      }else{
        tx->DrawText(DISposition.at(i)-0.5, ((DISstripNO.at(i)-1)*(2.29+4.62)+14.4+2.31)/10+0.5, Form("%i",i));
      }

        profile1d->Fill(DISarvTime[i]);

    }


      pad2->cd();
      profile1d->SetStats(0);
      profile1d->GetXaxis()->SetTitle("Arrival time(ns)");
      profile1d->GetYaxis()->SetTitle("Count");
      if(LAPPDLocateHitVerbosity>0) cout<<"size "<<g0->GetXaxis()->GetTitleSize()<<endl;

      profile1d->GetXaxis()->SetTitleSize((g0->GetXaxis()->GetTitleSize())*1.5);
      profile1d->GetYaxis()->SetTitleSize((g0->GetXaxis()->GetTitleSize())*1.5);

      profile1d->Draw();
      for (int i=1;i<p1dBins+1;i++){

          int n = profile1d->GetBinContent(i);

          auto *m = new TMarker();
          m->SetX(profile1d->GetBinCenter(i));
          m->SetY(0.1);
          double colorIndex = colorNumberstart+(profile1d->GetBinCenter(i)-minT)*timeScaleC;
          if(colorIndex > colorNumberstart && colorIndex <100 ){
              m->SetMarkerColorAlpha(colorIndex, 0.5);
          }else if( colorIndex < colorNumberstart){
              m->SetMarkerColorAlpha(colorNumberstart, 0.5);
          }else if (colorIndex > colorNumberend){
              m->SetMarkerColorAlpha(colorNumberend, 0.5);
          }
          m->SetMarkerSize(3);
          m->SetMarkerStyle(21);
          m->Draw();
      }


    name += DisplayPDFSuffix;
    name += ".pdf";
    cout<<"Saving pdf "<<name<<" for event "<<EventItr<<endl;
    c->SaveAs(name);

  delete c;
  delete profile1d;

  positionFile->cd();
  /*
  AmpVSnAreaS->Write();
  AmpVSnAreaD->Write();
  AmpVSpAreaS->Write();
  AmpVSpAreaD->Write();
  nAmpVSpAmp->Write();
  nAmpVSpAreaS->Write();
  nAmpVSpASoT->Write();
  positionHistOff->Write();
  positionHistIn->Write();
  BeamTvsPIn->Write();
  BeamTvsPOut->Write();
  */
PositionTree->Write();



}


if (SingleEventPlot >0 ) timeEvents->Write();

m_data->Stores["ANNIEEvent"]->Set(LocateHitLabel,OrderedPeaks);
m_data->Stores["ANNIEEvent"]->Set("NNLSLocatedHits",NNLSLocatedHitsMap);


  return true;
}


bool LAPPDLocateHit::Finalise(){
  positionFile->cd();
/*
  AmpVSnAreaS->Write();
  AmpVSnAreaD->Write();
  AmpVSpAreaS->Write();
  AmpVSpAreaD->Write();
  nAmpVSpAmp->Write();
  nAmpVSpAreaS->Write();
    nAmpVSpASoT->Write();

  positionHistOff->Write();
  positionHistIn->Write();
  timeEventsTotal->Write();
  //positionHist3D->Write();
  */

  PositionTree->Write();

  positionFile->Close();
  if(LAPPDLocateHitVerbosity>0) cout<<"LAPPD LocateHit Low threshold: "<<peakLowThreshold<<", High threshold: "<<peakHighThreshold<<endl;
  if(LAPPDLocateHitVerbosity>0) cout<<"LAPPD LocateHit has "<<AbnormalEvents<<" hits outside of the range"<<endl;
  return true;
}

double LAPPDLocateHit::MatchingProduct(double a, double b, double sigma){
  double result = -1./sigma *exp(pow(a-b,2)/2/pow(sigma,2));
  return result;
}


bool LAPPDLocateHit::FindlocalPeaks(Waveform<double> &wav,Waveform<double> &pwav, vector<double> &peakHeight, vector<int> &peakTimeLoc, vector<double> &peakArea ,vector<double> &pulseArea, vector<double> &pulseHeight, int strip, double lowT, double highT,int samplingFactor){
  bool normal = true;
  bool nonzero = false;
  for (int i=3;i<(wav.GetSamples()->size())-3;i++){//ignore the first bin to avoid possible errorflags
    //cout<<wav.GetSample(i)<<" at "<<i<<endl;
    //cout<<i<<endl;
    if(wav.GetSample(i)<wav.GetSample(i-1) && wav.GetSample(i)<wav.GetSample(i+1) && wav.GetSample(i)<wav.GetSample(i-2) && wav.GetSample(i)<wav.GetSample(i+2) && wav.GetSample(i)<lowT){  //less than lowT becaue the nnls output is negative
      if (LAPPDLocateHitVerbosity > 2) cout<<"peak at "<<i<<endl;
      peakHeight.push_back(wav.GetSample(i));
      peakTimeLoc.push_back(i);
      double Area=0.;

      for (int j=i; j>3; j--){
        if (wav.GetSample(j)>wav.GetSample(i) && wav.GetSample(j<0)) Area+=wav.GetSample(j);
        if (wav.GetSample(j) == 0) break;
      }
      for (int j=i+1; j<(wav.GetSamples()->size())-3; j++){
        if (wav.GetSample(j)>wav.GetSample(i) && wav.GetSample(j<0)) Area+=wav.GetSample(j);
        if (wav.GetSample(j) == 0) break;
      }
      peakArea.push_back(Area);
      //find pulse in raw data
      double pArea;
      int pIndex = (int) ((i*samplingFactor/1000)*10);
      int pStart = pIndex;
      int pEnd = pIndex;
      for (int j = pIndex; j>3; j--){
        if(pwav.GetSample(j)>-0.4) {
          pStart = j;
          break;}
      }
      for (int j =pIndex; j<pwav.GetSamples()->size(); j++){
        if(pwav.GetSample(j)>-0.4) {
          pEnd = j;
          break;}
      }
      double pHeight = pwav.GetSample(pStart);
      for (int j = pStart; j<pEnd; j++){
        pArea+=pwav.GetSample(j);
        if(pwav.GetSample(j)<pHeight) pHeight = pwav.GetSample(j);
      }
      pulseArea.push_back(pArea);
      pulseHeight.push_back(pHeight);
      nonzero = true;
      if(wav.GetSample(i)< highT){  //less than highT becaue the nnls output is negative
        normal=false;}
    }
  }
  bool r = normal && nonzero;
  //cout<<"peak height size in is "<<peakHeight.size()<<endl;

  return r;
}
