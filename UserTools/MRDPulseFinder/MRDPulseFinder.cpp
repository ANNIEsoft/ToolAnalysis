#include "MRDPulseFinder.h"

MRDPulseFinder::MRDPulseFinder():Tool(){}


bool MRDPulseFinder::Initialise(std::string configfile, DataModel &data){

  /////////////////// Usefull header ///////////////////////
  if(configfile!="")  m_variables.Initialise(configfile); //loading config file
  //m_variables.Print();

  m_data= &data; //assigning transient data pointer
  /////////////////////////////////////////////////////////////////

  m_variables.Get("Threshold", thresh);
  m_variables.Get("MinPulseWidth", deltat);
//  m_variables.Get("Baseline", bline);
//  m_variables.Get("SigmaBaseline", sigbline);

  return true;
}


bool MRDPulseFinder::Execute(){

  m_data->Stores["ANNIEEvent"]->Get("RawADCData", rawadcdata);
  m_data->Stores["ANNIEEvent"]->Get("EventNumber", evnum);
  m_data->Stores["ANNIEEvent"]->Get("CalibratedADCData", caladc);

  map<int,map<int,std::vector<ADCPulse>>> MiniBufferPulses;

  map<unsigned long,std::vector<Waveform<unsigned short>>> :: iterator itr;
  //map<unsigned long,std::vector<CalibratedADCWaveform<double>>> :: iterator itrr;
  int vectsize = rawadcdata.begin()->second.size();

  for(int aaa=0; aaa<vectsize; aaa++){

    std::vector<ADCPulse> somepulses;
    map<int,std::vector<ADCPulse>> ChannelPulses;
    channelno = 0;
    //for(itr=rawadcdata.begin(),itrr=caladc.begin(); itr!=rawadcdata.end(),itrr!=caladc.end(); ++itr,++itrr){
    for(itr=rawadcdata.begin(); itr!=rawadcdata.end(); ++itr){

      unsigned long ck = itr->first;
      std::vector<Waveform<unsigned short>> TheWaveforms = itr->second;
      //unsigned long ckey = itrr->first;
      //std::vector<CalibratedADCWaveform<double>> somecalwavs = itrr->second;
      //CalibratedADCWaveform<double> onecalwav = somecalwavs.at(aaa);
      //bline = onecalwav.GetBaseline();
      //sigbline = onecalwav.GetSigmaBaseline();
      Waveform<unsigned short> aWaveform = TheWaveforms.at(aaa);
      somepulses = FindPulse(aWaveform.GetSamples());
      ChannelPulses.insert(pair <int,std::vector<ADCPulse>> (channelno,somepulses));

      channelno++;
    }
    minibuffernum=aaa;
    MiniBufferPulses.insert(pair <int,map<int,std::vector<ADCPulse>>> (minibuffernum,ChannelPulses));
  }

  m_data->Stores["ANNIEEvent"]->Set("TrigEvents",MiniBufferPulses);

  return true;
}


bool MRDPulseFinder::Finalise(){

  return true;
}

std::vector<ADCPulse> MRDPulseFinder::FindPulse(vector<short unsigned int>* someWave) {

  std::vector<ADCPulse> somepulses;

  int numpeaks=0;
  int binfin=0;
  double Q=0.;
  double peak=0.;
  double peaktime=0;
  double low=0.;
  double hi=0.;
  double pktime=0.;

  bool pulsebeg=false;
  double curval=0.;
  double preval=0.;
  int numbin = someWave->size();
  int length = 0;
  int minbin = (int)(deltat/2);
  double pulsewidth = 0.;

  for(int bbb=0; bbb<numbin; bbb++){
    //96-189 establish the dynamic baseline to subdue the rf noise seen in longer (non-Hefty) time windows
    if(bbb==0){
      bline = (*someWave)[bbb];
    }

    if(bbb==1
    &&fabs((*someWave)[bbb]-bline)<5
    &&fabs((*someWave)[bbb-1]-bline)<5){
      bline=((*someWave)[bbb]+(*someWave)[bbb-1])/2;
    }

    if(bbb==2
    &&fabs((*someWave)[bbb]-bline)<5
    &&fabs((*someWave)[bbb-1]-bline)<5
    &&fabs((*someWave)[bbb-2]-bline)<5){
      bline=((*someWave)[bbb]+(*someWave)[bbb-1]+(*someWave)[bbb-2])/3;
    }

    if(bbb==3
    &&fabs((*someWave)[bbb]-bline)<5
    &&fabs((*someWave)[bbb-1]-bline)<5
    &&fabs((*someWave)[bbb-2]-bline)<5
    &&fabs((*someWave)[bbb-3]-bline)<5){
      bline=((*someWave)[bbb]+(*someWave)[bbb-1]+(*someWave)[bbb-2]+(*someWave)[bbb-3])/4;
    }

    if(bbb==4
    &&fabs((*someWave)[bbb]-bline)<5
    &&fabs((*someWave)[bbb-1]-bline)<5
    &&fabs((*someWave)[bbb-2]-bline)<5
    &&fabs((*someWave)[bbb-3]-bline)<5
    &&fabs((*someWave)[bbb-4]-bline)<5){
      bline=((*someWave)[bbb]+(*someWave)[bbb-1]+(*someWave)[bbb-2]+(*someWave)[bbb-3]+(*someWave)[bbb-4])/5;
    }

    if(bbb==5
    &&fabs((*someWave)[bbb]-bline)<5
    &&fabs((*someWave)[bbb-1]-bline)<5
    &&fabs((*someWave)[bbb-2]-bline)<5
    &&fabs((*someWave)[bbb-3]-bline)<5
    &&fabs((*someWave)[bbb-4]-bline)<5
    &&fabs((*someWave)[bbb-5]-bline)<5){
      bline=((*someWave)[bbb]+(*someWave)[bbb-1]+(*someWave)[bbb-2]+(*someWave)[bbb-3]+(*someWave)[bbb-4]+(*someWave)[bbb-5])/6;
    }

    if(bbb==6
    &&fabs((*someWave)[bbb]-bline)<5
    &&fabs((*someWave)[bbb-1]-bline)<5
    &&fabs((*someWave)[bbb-2]-bline)<5
    &&fabs((*someWave)[bbb-3]-bline)<5
    &&fabs((*someWave)[bbb-4]-bline)<5
    &&fabs((*someWave)[bbb-5]-bline)<5
    &&fabs((*someWave)[bbb-6]-bline)<5){
      bline=((*someWave)[bbb]+(*someWave)[bbb-1]+(*someWave)[bbb-2]+(*someWave)[bbb-3]+(*someWave)[bbb-4]+(*someWave)[bbb-5]+(*someWave)[bbb-6])/7;
    }

    if(bbb==7
    &&fabs((*someWave)[bbb]-bline)<5
    &&fabs((*someWave)[bbb-1]-bline)<5
    &&fabs((*someWave)[bbb-2]-bline)<5
    &&fabs((*someWave)[bbb-3]-bline)<5
    &&fabs((*someWave)[bbb-4]-bline)<5
    &&fabs((*someWave)[bbb-5]-bline)<5
    &&fabs((*someWave)[bbb-6]-bline)<5
    &&fabs((*someWave)[bbb-7]-bline)<5){
      bline=((*someWave)[bbb]+(*someWave)[bbb-1]+(*someWave)[bbb-2]+(*someWave)[bbb-3]+(*someWave)[bbb-4]+(*someWave)[bbb-5]+(*someWave)[bbb-6]+(*someWave)[bbb-7])/8;
    }

    if(bbb==8
    &&fabs((*someWave)[bbb]-bline)<5
    &&fabs((*someWave)[bbb-1]-bline)<5
    &&fabs((*someWave)[bbb-2]-bline)<5
    &&fabs((*someWave)[bbb-3]-bline)<5
    &&fabs((*someWave)[bbb-4]-bline)<5
    &&fabs((*someWave)[bbb-5]-bline)<5
    &&fabs((*someWave)[bbb-6]-bline)<5
    &&fabs((*someWave)[bbb-7]-bline)<5
    &&fabs((*someWave)[bbb-8]-bline)<5){
      bline=((*someWave)[bbb]+(*someWave)[bbb-1]+(*someWave)[bbb-2]+(*someWave)[bbb-3]+(*someWave)[bbb-4]+(*someWave)[bbb-5]+(*someWave)[bbb-6]+(*someWave)[bbb-7]+(*someWave)[bbb-8])/9;
    }

    if(bbb>=9
    &&fabs((*someWave)[bbb]-bline)<5
    &&fabs((*someWave)[bbb-1]-bline)<5
    &&fabs((*someWave)[bbb-2]-bline)<5
    &&fabs((*someWave)[bbb-3]-bline)<5
    &&fabs((*someWave)[bbb-4]-bline)<5
    &&fabs((*someWave)[bbb-5]-bline)<5
    &&fabs((*someWave)[bbb-6]-bline)<5
    &&fabs((*someWave)[bbb-7]-bline)<5
    &&fabs((*someWave)[bbb-8]-bline)<5
    &&fabs((*someWave)[bbb-9]-bline)<5){
      bline=((*someWave)[bbb]+(*someWave)[bbb-1]+(*someWave)[bbb-2]+(*someWave)[bbb-3]+(*someWave)[bbb-4]+(*someWave)[bbb-5]+(*someWave)[bbb-6]+(*someWave)[bbb-7]+(*someWave)[bbb-8]+(*someWave)[bbb-9])/10;
    }

    curval = fabs((*someWave)[bbb]);
    if(bbb>1) preval = fabs((*someWave)[bbb-1]);

    if((curval-bline)>thresh){

      length++;
      if((curval-bline)>peak){peak=(curval-bline); peaktime=bbb; pktime=2*((double)bbb);}
      Q+=((*someWave)[bbb]);
      if(!pulsebeg) low=2*((double)bbb);
      pulsebeg=true;

    }else{

      if(length<minbin){length=0; Q=0; pulsebeg=false; low=0; hi=0; peak=0;}

        else{

          numpeaks++; length = 0; hi = 2*((double)bbb);
          int TubeId=0;
          unsigned long raw_area=0;
          unsigned short raw_amp=0;
          double calibrated_amplitude=0.;
          pulsewidth = (hi-low);

          ADCPulse pulse(TubeId,low,peaktime,bline,hi,raw_area,raw_amp,calibrated_amplitude,numpeaks);
          somepulses.push_back(pulse);
          pulsebeg = false;
          peak=0; Q=0; low=0; hi=0;

        }

    }

  }

  return somepulses;

}
