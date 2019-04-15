#include "LAPPDnnls.h"
#include "nnlsUVevent.h"

LAPPDnnls::LAPPDnnls():Tool(){}


bool LAPPDnnls::Initialise(std::string configfile, DataModel &data){

  /////////////////// Usefull header ///////////////////////
  if(configfile!="")  m_variables.Initialise(configfile); //loading config file
  //m_variables.Print();

  m_data= &data; //assigning transient data pointer
  /////////////////////////////////////////////////////////////////

  return true;
}


bool LAPPDnnls::Execute(){


	// get raw lappd data
	std::map<int,vector<Waveform<double>>> rawlappddata;
	m_data->Stores["ANNIEEvent"]->Get("RawLAPPDData",rawlappddata);

	//------------------Create a root file--------------------------------------------------------//
	TFile *f = new TFile("/docker_toolchain/data/testroot.root", "RECREATE");				//open a file
	TTree lappd("nnlsout","nnlsout");				//create a tree
	f->mkdir("wav");
	f->mkdir("pulses");

	//assumes that all channels and events have the same
	//total number of samples
	const int nsamples = rawlappddata.begin()->second.at(0).GetSamples()->size();
	int channelno;

	map <int, vector<Waveform<double>>> :: iterator itr;
  	for (itr = rawlappddata.begin(); itr != rawlappddata.end(); ++itr)
  	{
	    channelno = itr->first;
	    std::cout << "ok but here" << std::endl;
	    //this waveform vector has only one waveform
	    //(when generated from LAPPDSim. Does this change
	    //for real data input?). Hence the .at(0)
	    Waveform<double> Vwav = itr->second.at(0);
	    float arraywfm[nsamples];
	    for(int i = 0; i < nsamples; i++)
	    {
	    	arraywfm[i] = Vwav.GetSample(i);
	    }
	    std::cout << "at least here?" << std::endl;
	    //initialize the nnlsWaveform object
	    nnlsWaveform* nnlswav = new nnlsWaveform();
	    std::cout << "initialized nnlswaveform" << std::endl;
	    nnlswav->Setchno(channelno);
	    nnlswav->SetDimSize(nsamples);
	    //weird thing where undergrad indexes the first memory element
	    nnlswav->SetWave(&arraywfm[0]); 
	    std::cout << "here" << std::endl;
	    //do the peak finding algorithm
	    nnlswav->Analyze();
	    f->cd("wav");
	    std::cout<< "writing histos" << std::endl;
		nnlswav->hwav->Write();
		nnlswav->hwav_raw->Write();
		nnlswav->xvector->Write();
		nnlswav->nnlsoutput->Write();

		//if you get a peak, write the waveform
		//into the pulses folder
		if((nnlswav->npeaks)>0)
		{ 
			f->cd("pulses");        //go to folder "/pulses"
			nnlswav->hwav->Write();      //write histograms for waveforms
			std::cout << 'pulses found' << std::endl;
		}

	    
    }
    f->Close();
	return true;
}


bool LAPPDnnls::Finalise(){

  return true;
}
