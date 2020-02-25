#include "LoadRawStores.h"

LoadRawStores::LoadRawStores():Tool(){}


bool LoadRawStores::Initialise(std::string configfile, DataModel &data){

  /////////////////// Useful header ///////////////////////
  if(configfile!="") m_variables.Initialise(configfile); // loading config file
  //m_variables.Print();

  m_data= &data; //assigning transient data pointer
  /////////////////////////////////////////////////////////////////

  //file list contains the paths to example raw data files
  m_variables.Get("FileList",file_list);

  ifstream file_sim(file_list);
  while(!file_sim.eof()){
      std::string string_temp;
      file_sim >> string_temp;
      if (string_temp!="") vec_filename.push_back(string_temp);
  }
  file_sim.close();

  //Define stores
  m_data->Stores["PMTData"]= new BoostStore(false,2);
  PMTData = 0;
  indata = 0;
  i_loop = 0;

  return true;
}


bool LoadRawStores::Execute(){

  std::cout <<"i_loop: "<<i_loop<<std::endl;
  if (i_loop > vec_filename.size()) {m_data->vars.Set("StopLoop",true); return true;}
  std::string datapath = vec_filename.at(i_loop);
 
  std::cout <<datapath<<std::endl; 
  if (PMTData!=0){ PMTData->Close(); PMTData->Delete(); delete PMTData; PMTData = 0;}
  if (indata!=0){indata->Close(); indata->Delete(); delete indata; indata = 0;}
  
  std::cout <<"initialise indata"<<std::endl;
  BoostStore *indata = new BoostStore(false,0);
  indata->Initialise(datapath);

  std::cout <<"define PMTData"<<std::endl;
  BoostStore *PMTData = new BoostStore(false,2);
  indata->Get("PMTData",*PMTData);
  PMTData->Save("tmp");
  m_data->Stores["PMTData"]->Set("FileData",PMTData,false);
 
  i_loop++;

  return true;
}


bool LoadRawStores::Finalise(){

  if (PMTData!=0){ PMTData->Close(); PMTData->Delete(); delete PMTData; PMTData = 0;}
  if (indata!=0){ indata->Close(); indata->Delete(); delete indata; indata = 0;}
  m_data->Stores["PMTData"]->Remove("FileData");
  m_data->Stores.clear();

  return true;
}
