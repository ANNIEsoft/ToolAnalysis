#include "LAPPDParseScope.h"
#include <stdlib.h>
#include <TF1.h>

LAPPDParseScope::LAPPDParseScope():Tool(){}


bool LAPPDParseScope::Initialise(std::string configfile, DataModel &data){

  /////////////////// Usefull header ///////////////////////
  if(configfile!="")  m_variables.Initialise(configfile); //loading config file
  //m_variables.Print();

  m_data= &data; //assigning transient data pointer
  /////////////////////////////////////////////////////////////////

  m_data->Stores["ANNIEEvent"]= new BoostStore(false,2);

  std::string FileInput = "test.fff";	 //default input file name
  // get the actual input file name from the config file
  m_variables.Get("FileInput", FileInput);
  // open the data file
  isin.open(FileInput, ios::in); //open input .txt files
  std::cout<<"just opened: "<<FileInput<<std::endl;

  // Get the details of the Waveform from the config file
  m_variables.Get("Nsamples", WavDimSize);
  m_variables.Get("NChannels", NChannel);
  m_variables.Get("TrigChannel", TrigChannel);

  // Since this is data, we set the isSim boolean to false
  bool isSim = false;
  m_data->Stores["ANNIEEvent"]->Header->Set("isSim",isSim);

  iter=0;

  return true;
}


bool LAPPDParseScope::Execute(){

  std::map<int,vector<Waveform<double>>> RawLAPPDData;

  if(iter%1000==0)cout<<"iteration: "<<iter<<endl;

  //loop over the text file

  double hsample;
  for(int m=0;m<NChannel;m++) {
   // create an instance of a Wavefom
    vector<Waveform<double>> thewavs;
    Waveform<double> hwav;
    for(int n=0;n<WavDimSize;n++) {
      isin>>hsample;
      // push the samples into the Waveform, flip the sign
      if(m==TrigChannel) hwav.PushSample(-hsample);
      else hwav.PushSample(-hsample);
      //std::cout<<"Sample from data: "<<hsample<<std::endl;
    }
    //std::cout<<"channel: "<<m<<" "<<"size: "<<(hwav.GetSamples())->size()<<std::endl;
    thewavs.push_back(hwav);
    RawLAPPDData.insert(pair <int,vector<Waveform<double>>> (m,thewavs));
  }

  // add the map of Waveforms to the Boost Store
  m_data->Stores["ANNIEEvent"]->Set("RawLAPPDData",RawLAPPDData);

  iter++;

  return true;
}

bool LAPPDParseScope::Finalise(){

  return true;
}
