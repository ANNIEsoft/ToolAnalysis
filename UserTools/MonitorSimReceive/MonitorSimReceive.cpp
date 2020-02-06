#include "MonitorSimReceive.h"

MonitorSimReceive::MonitorSimReceive():Tool(){}


bool MonitorSimReceive::Initialise(std::string configfile, DataModel &data){

    /////////////////// Usefull header ///////////////////////
    if(configfile!="")  m_variables.Initialise(configfile); //loading config file
    //m_variables.Print();

    m_data= &data; //assigning transient data pointer
    /////////////////////////////////////////////////////////////////

    m_variables.Get("FileList",file_list);
    m_variables.Get("OutPath",outpath);
    m_variables.Get("verbose",verbosity);

    m_data->CStore.Set("OutPath",outpath);

    if (verbosity > 2) std::cout <<"MonitorSimReceive: Initialising"<<std::endl;

    if (verbosity > 2) {
        std::cout <<"FileList: "<<file_list<<std::endl;
    }

    ifstream file_sim(file_list);
    while(!file_sim.eof()){
      std::string string_temp;
      file_sim >> string_temp;
      if (string_temp!="") vec_filename.push_back(string_temp);
      std::cout <<string_temp<<std::endl;
    }
    file_sim.close();

    std::cout <<"Data Filelist vector size: "<<vec_filename.size()<<std::endl;

    if (verbosity > 2) std::cout <<"Define CCData & PMTData BoostStores"<<std::endl;

    srand(time(NULL));
    m_data->Stores["CCData"]=new BoostStore(false,2);  
    m_data->Stores["PMTData"]=new BoostStore(false,2);

    i_loop = 0;
    return true;
}


bool MonitorSimReceive::Execute(){

    if (verbosity > 2) std::cout <<"MonitorSimReceive: Executing"<<std::endl;

    std::string datapath;
    if (i_loop<vec_filename.size()){
      datapath = vec_filename.at(i_loop);
      if (verbosity > 2) std::cout <<"Current file: "<<datapath<<std::endl;
    } else {
      m_data->vars.Set("StopLoop",true);
      return true;
    }

    if (MRDData!=0){
      MRDData->Close();
      MRDData->Delete();
      delete MRDData;
      MRDData = 0;
      m_data->Stores["CCData"]->Remove("FileData");
    }
    if (PMTData!=0){
      PMTData->Close();
      PMTData->Delete();
      delete PMTData;
      PMTData = 0;
      m_data->Stores["PMTData"]->Remove("FileData");
    }
    if (indata!=0){
      indata->Close();
      indata->Delete();
      delete indata;
      indata = 0;
    }

    //BoostStore* indata=new BoostStore(false,0); //this leaks but its jsut for testing
    indata=new BoostStore(false,0); //this leaks but its jsut for testing
    indata->Initialise(datapath);

    //BoostStore *MRDData = new BoostStore(false,2);
    //BoostStore *PMTData = new BoostStore(false,2);
    MRDData = new BoostStore(false,2);
    PMTData = new BoostStore(false,2);

    std::cout <<"Print indata:"<<std::endl;
    indata->Print(false);

    indata->Get("CCData",*MRDData);
    indata->Get("PMTData",*PMTData);
    
    std::string State="DataFile";
    m_data->CStore.Set("State",State);
    MRDData->Save("tmp");
    m_data->Stores["CCData"]->Set("FileData",MRDData,false);
    PMTData->Save("tmp");
    m_data->Stores["PMTData"]->Set("FileData",PMTData,false);

    i_loop++;


    return true;
}


bool MonitorSimReceive::Finalise(){

    if (verbosity > 2) std::cout <<"MonitorSimReceive: Finalising"<<std::endl;

    MRDData=0;
    PMTData=0;
    indata = 0;

    m_data->CStore.Remove("State");
    m_data->Stores["CCData"]->Remove("FileData");
    m_data->Stores["PMTData"]->Remove("FileData");
    m_data->Stores.clear();

    return true;
}
