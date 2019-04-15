#include "LAPPDRawToACDC.h"
#include <iostream>
#include <fstream>

LAPPDRawToACDC::LAPPDRawToACDC():Tool(){}


bool LAPPDRawToACDC::Initialise(std::string configfile, DataModel &data){

  /////////////////// Usefull header ///////////////////////
  if(configfile!="")  m_variables.Initialise(configfile); //loading config file
  //m_variables.Print();

  m_data= &data; //assigning transient data pointer
  /////////////////////////////////////////////////////////////////
  m_variables.Get("rawtoacdc_outputfile", outputfile);

  return true;
}


bool LAPPDRawToACDC::Execute(){


  	// get raw lappd data
  	std::map<int,vector<Waveform<double>>> rawlappddata;
	m_data->Stores["ANNIEEvent"]->Get("RawLAPPDData",rawlappddata);


	//manage output file
	char logDataFilename[300];
	sprintf(logDataFilename, "%s.acdc", outputfile);
	ofstream dataofs;
	dataofs.open(logDataFilename, ios::trunc);


	//get the number of samples in each waveform
	//assuming that all waveforms are alike (use 
	//first waveform as the example)
	Waveform<double> exwav = rawlappddata.begin()->second.at(0);
	vector<double>* exsamples = exwav.GetSamples();
	int psecSampleCells = exsamples->size();
	cout << psecSampleCells << endl;
	return true;


	//put in the header that is in ACDC data output
	char delim = ' ';
	dataofs << "Event" << delim << "Board" << delim << "Ch";
    for (int i = 0; i < psecSampleCells; i++) {
        dataofs << delim << i;
    }

	map <int, vector<Waveform<double>>> :: iterator itr;
  for (itr = rawlappddata.begin(); itr != rawlappddata.end(); ++itr){
    int ch = itr->first;
    vector<Waveform<double>> wavs = itr->second;

    //loop over all Waveforms
    for(int i=0; i<wavs.size(); i++){

        Waveform<double> wav = wavs.at(i);

      }

    }


  return true;
}


bool LAPPDRawToACDC::Finalise(){

  return true;
}
