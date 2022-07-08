#include "LAPPDBaselineSubtract.h"

LAPPDBaselineSubtract::LAPPDBaselineSubtract():Tool(){}


bool LAPPDBaselineSubtract::Initialise(std::string configfile, DataModel &data){

  /////////////////// Usefull header ///////////////////////
  if(configfile!="")  m_variables.Initialise(configfile); //loading config file
  //m_variables.Print();

  m_data= &data; //assigning transient data pointer
  /////////////////////////////////////////////////////////////////

  //bool isBLsub = true;
  //m_data->Stores["ANNIEEvent"]->Header->Set("isBLsubtracted",isBLsub);

  m_data->Stores["ANNIEEvent"]->Get("SampleSize",Deltat);
  m_variables.Get("TrigChannel1",TrigChannel1);
  m_variables.Get("TrigChannel2",TrigChannel2);

  m_variables.Get("LowBLfitrange", LowBLfitrange);
  m_variables.Get("HiBLfitrange",HiBLfitrange);
  m_variables.Get("TrigLowBLfitrange", TrigLowBLfitrange);
  m_variables.Get("TrigHiBLfitrange",TrigHiBLfitrange);
  m_variables.Get("oldLaserTrigAmpRange",oldLaserTrigAmpRange);

  TString BLSIWL;
  m_variables.Get("BLSInputWavLabel", BLSIWL);
  BLSInputWavLabel= BLSIWL;

  TString BLSOWL;
  m_variables.Get("BLSOutputWavLabel", BLSOWL);
  BLSOutputWavLabel=BLSOWL;

  return true;
}


bool LAPPDBaselineSubtract::Execute(){
    bool isBLsub=true;
    m_data->Stores["ANNIEEvent"]->Set("isBLsubtracted",isBLsub);
    //cout<<"made it to here "<<BLSInputWavLabel<<endl;
    Waveform<double> bwav;

    oldLaser = 0;
    m_data->Stores["ANNIEEvent"]->Get("oldLaser", oldLaser);
    //cout<<"BLSub oldLaser: "<<oldLaser<<endl;

  // get raw lappd data
    std::map<unsigned long,vector<Waveform<double>>> lappddata;
    m_data->Stores["ANNIEEvent"]->Get(BLSInputWavLabel,lappddata);
    //cout<< "did it get to this"<<endl;
    // the filtered Waveform
    std::map<unsigned long,vector<Waveform<double>>> blsublappddata;

  map <unsigned long, vector<Waveform<double>>> :: iterator itr;
  for (itr = lappddata.begin(); itr != lappddata.end(); ++itr){
    int channelno = itr->first;
    vector<Waveform<double>> Vwavs = itr->second;
    vector<Waveform<double>> Vfwavs;

    //loop over all Waveforms
    for(int i=0; i<Vwavs.size(); i++){

        Waveform<double> bwav = Vwavs.at(i);
        DimSize = bwav.GetSamples()->size();
        // This is from back when we had a sinusoidal pedestal
        //Waveform<double> blswav = SubtractSine(bwav);

        // Loop over first N samples and get average value
        if(LowBLfitrange>DimSize || HiBLfitrange>DimSize){
            cout<<"BASELINE FITRANGE IS WRONG!!! "<<LowBLfitrange<<" "<<HiBLfitrange<<endl;
            LowBLfitrange=0;
            HiBLfitrange=1;
        }

        if(TrigLowBLfitrange>DimSize || TrigHiBLfitrange>DimSize){
            cout<<"BASELINE FITRANGE IS WRONG!!! "<<TrigLowBLfitrange<<" "<<TrigHiBLfitrange<<endl;
            TrigLowBLfitrange=0;
            TrigHiBLfitrange=1;
        }

        double BLval=0;
        double AvgBL=0;
        if(channelno==TrigChannel1 || channelno==TrigChannel2){
          for(int j=TrigLowBLfitrange; j<TrigHiBLfitrange; j++){
              BLval+=bwav.GetSamples()->at(j);
          }
          AvgBL = BLval/((double)(HiBLfitrange-LowBLfitrange));
          if(oldLaser == 1) {
            BLval = 0;
            int range = 0;
            double min =  *std::min_element(bwav.GetSamples()->begin(), bwav.GetSamples()->end()) ;
            for(int k = 0; k<bwav.GetSamples()->size();k++){
              if(bwav.GetSamples()->at(k) < min + oldLaserTrigAmpRange){
                BLval += bwav.GetSamples()->at(k);
                range += 1;
              }
            }
            AvgBL = BLval/((double)range);

          };

        } else{
            for(int j=LowBLfitrange; j<HiBLfitrange; j++){
                BLval+=bwav.GetSamples()->at(j);
            }
            AvgBL = BLval/((double)(HiBLfitrange-LowBLfitrange));
        }


        Waveform<double> blswav;
        for(int k=0; k<bwav.GetSamples()->size(); k++){
            blswav.PushSample((bwav.GetSamples()->at(k))-AvgBL);
        }


        Vfwavs.push_back(blswav);
      }

      blsublappddata.insert(pair <int,vector<Waveform<double>>> (channelno,Vfwavs));
    }

  //cout<<BLSOutputWavLabel<<endl;
  m_data->Stores["ANNIEEvent"]->Set(BLSOutputWavLabel,blsublappddata);

  return true;
}


Waveform<double> LAPPDBaselineSubtract::SubtractSine(Waveform<double> iwav) {

  Waveform<double> subWav;

  int nbins = iwav.GetSamples()->size();
  double starttime=0.;
  double endtime = starttime + ((double)nbins)*100.;
  TH1D* hwav_raw = new TH1D("hwav_raw","hwav_raw",nbins,starttime,endtime);

  for(int i=0; i<nbins; i++){
    hwav_raw->SetBinContent(i+1,iwav.GetSample(i));
    hwav_raw->SetBinError(i+1,0.1);

  }

  TF1* sinit = new TF1("sinit","([0]*sin([2]*x+[1]))",0,DimSize*Deltat);
  sinit->SetParameter(0,0.4);
  sinit->SetParameter(1,0.0);
  sinit->SetParameter(2,0.0);
  sinit->SetParameter(2,0.00055);
  sinit->SetParLimits(2,0.0003,0.0008);
  sinit->SetParLimits(0,0.,1.0);

  hwav_raw->Fit("sinit","QNO","",LowBLfitrange,HiBLfitrange);
  //cout<<"Parameters: "<< sinit->GetParameter(3)<<" "<<LowBLfitrange<<" "<<HiBLfitrange<<endl;

  for(int j=0; j<nbins; j++){

    subWav.PushSample((hwav_raw->GetBinContent(j+1))-(sinit->Eval(hwav_raw->GetBinCenter(j+1))));
  }

  delete hwav_raw;
  delete sinit;
  return subWav;
}


bool LAPPDBaselineSubtract::Finalise(){

  return true;
}
