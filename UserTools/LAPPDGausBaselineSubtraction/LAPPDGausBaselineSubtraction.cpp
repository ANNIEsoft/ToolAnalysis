#include "LAPPDGausBaselineSubtraction.h"

LAPPDGausBaselineSubtraction::LAPPDGausBaselineSubtraction():Tool(){}


bool LAPPDGausBaselineSubtraction::Initialise(std::string configfile, DataModel &data){

  /////////////////// Useful header ///////////////////////
  if(configfile!="") m_variables.Initialise(configfile); // loading config file
  //m_variables.Print();

  m_data= &data; //assigning transient data pointer
  /////////////////////////////////////////////////////////////////

    m_variables.Get("Nsamples", DimSize);
    m_variables.Get("SampleSize",Deltat);
    m_variables.Get("TrigChannel",TrigChannel);
    m_variables.Get("BLSInputWavLabel", BLSInputWavLabel);
    m_variables.Get("BLSOutputWavLabel", BLSOutputWavLabel);
    m_variables.Get("BaselineSubstractVerbosityLevel",BLSVerbosityLevel);
        m_variables.Get("LAPPDchannelOffset",LAPPDchannelOffset);
  return true;
}


bool LAPPDGausBaselineSubtraction::Execute(){

    bool isBLsub=true;
    m_data->Stores["ANNIEEvent"]->Set("isBLsubtracted",isBLsub);
    if(BLSVerbosityLevel>2) cout<<"Made it to here "<<BLSInputWavLabel<<" is read in."<<endl;
    Waveform<double> bwav;

    std::map<unsigned long,vector<Waveform<double>>> lappddata;
    m_data->Stores["ANNIEEvent"]->Get(BLSInputWavLabel,lappddata);
    if(BLSVerbosityLevel>2) cout<<"And data is loaded"<<endl;
    // the filtered Waveform
    std::map<unsigned long,vector<Waveform<double>>> blsublappddata;

    map <unsigned long, vector<Waveform<double>>> :: iterator itr;

    // loop over all channels
    for (itr = lappddata.begin(); itr != lappddata.end(); ++itr)
    {
        int channelno = itr->first;
        vector<Waveform<double>> Vwavs = itr->second;
        vector<Waveform<double>> Vfwavs;
        int bi = (int)(channelno-LAPPDchannelOffset)/30;

        //loop over all Waveforms
        for(int i=0; i<Vwavs.size(); i++){
		Waveform<double> bwav = Vwavs.at(i);

		// histogram range
		double minvalue = *std::min_element(bwav.GetSamples()->begin(), bwav.GetSamples()->end());
		double maxvalue = *std::max_element(bwav.GetSamples()->begin(), bwav.GetSamples()->end());

		TString hname = "waveformHist";
		hname+=channelno;
		hname+="_wav";
		hname+=i;
		TH1D* BLHist = new TH1D(hname,hname,20,minvalue,maxvalue);
		
		int numofsamples = bwav.GetSamples()->size();

		// loop over each element in the waveform, adding it to BLHist
		for(int j=0; j<numofsamples; j++)
		{
		    double value = bwav.GetSamples()->at(j);
		    BLHist->Fill(value);
		}

		BLHist->Fit("gaus","Q");
		TF1* fit = BLHist->GetFunction("gaus");
		double AvgBL = fit->GetParameter(1);
		double chi2 = fit->GetChisquare();

		Waveform<double> blswav;
		for(int k=0; k<bwav.GetSamples()->size(); k++)
		{
		    blswav.PushSample((bwav.GetSamples()->at(k))-AvgBL);
		}

		Vfwavs.push_back(blswav);
		delete BLHist;
    }

    blsublappddata.insert(pair <int,vector<Waveform<double>>> (channelno,Vfwavs));
    }

    if(BLSVerbosityLevel>2) cout<<"Baseline substraction is done in " <<BLSOutputWavLabel<<endl;
    m_data->Stores["ANNIEEvent"]->Set(BLSOutputWavLabel,blsublappddata);

  return true;
}


bool LAPPDGausBaselineSubtraction::Finalise(){

  return true;
}
