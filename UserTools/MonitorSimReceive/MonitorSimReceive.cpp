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
    m_variables.Get("Mode",mode);
    m_variables.Get("verbose",verbosity);

    m_data->CStore.Set("OutPath",outpath);

    if (verbosity > 2) std::cout <<"MonitorSimReceive: Initialising"<<std::endl;

    if (verbosity > 2) {
        std::cout <<"FileList: "<<file_list<<std::endl;
    }

    if (mode != "FileList" && mode != "Single" && mode != "Wait") mode = "FileList";
    std::cout <<"Mode: "<<mode<<std::endl;

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

    MRDData = 0;
    PMTData = 0;
    indata = 0;

    i_loop = 0;

    if (mode == "Single"){
      indata=new BoostStore(false,0); //this leaks but its jsut for testing
      std::string datapath = vec_filename.at(0);
      indata->Initialise(datapath);
      MRDData = new BoostStore(false,2);
      bool has_cc = indata->Has("CCData");
      if (has_cc) indata->Get("CCData",*MRDData);
    }


    return true;
}


bool MonitorSimReceive::Execute(){

    if (verbosity > 2) std::cout <<"MonitorSimReceive: Executing"<<std::endl;

    if (mode == "Wait"){
      std::string Wait = "Wait";     
      m_data->CStore.Set("State",Wait);
      return true;
    }
    else if (mode == "Single"){
       bool has_cc = indata->Has("CCData");
       m_data->CStore.Set("HasCCData",has_cc);
       if (!has_cc) {
           std::string State="Wait";
           m_data->CStore.Set("State",State); 
           return true;
       }
       int event=rand() % 1000;
       std::string State="MRDSingle";
       m_data->CStore.Set("State",State);
       MRDOut tmp;
       long entries;
       MRDData->Header->Get("TotalEntries",entries);
       std::cout <<"event: "<<event<<", entries: "<<entries<<std::endl;
       if (event < entries) MRDData->GetEntry(event);
       else MRDData->GetEntry(entries-1);
       MRDData->Get("Data", tmp);
       m_data->Stores["CCData"]->Set("Single",tmp);
       return true;
    } 
    else if (mode == "FileList"){
    std::string datapath;
    if (i_loop<vec_filename.size()){
      datapath = vec_filename.at(i_loop);
      if (verbosity > 2) std::cout <<"Current file: "<<datapath<<std::endl;
      if (i_loop == vec_filename.size()-1) m_data->vars.Set("StopLoop",true);
    } else {
      m_data->vars.Set("StopLoop",true);
      if (indata!=0){ indata->Close(); indata->Delete(); delete indata; indata = 0;}
      return true;
    }

    if (MRDData!=0){
      m_data->Stores["CCData"]->Delete();
    }
    if (PMTData!=0){
      m_data->Stores["PMTData"]->Delete();
    }
    if (indata!=0){
      std::cout <<"close indata"<<std::endl;
      indata->Close();
      indata->Delete();
      delete indata;
      indata = 0;
    }
    

    std::cout <<"create new indata"<<std::endl;
    indata=new BoostStore(false,0); //this leaks but its jsut for testing
    indata->Initialise(datapath);
    
    std::cout <<"Print indata:"<<std::endl;
    indata->Print(false);

    bool has_cc=false;
    bool has_pmt=false;

    if (indata->Has("CCData")){
	std::cout <<"RawData has CCData Store!"<<std::endl;
	has_cc = true;
        MRDData = new BoostStore(false,2);
    } 
    if (indata->Has("PMTData")){
	std::cout <<"RawData has PMTData Store!"<<std::endl;
        has_pmt = true;
        PMTData = new BoostStore(false,2);
    }

    m_data->CStore.Set("HasCCData",has_cc);
    m_data->CStore.Set("HasPMTData",has_pmt);
    std::string State="DataFile";
    m_data->CStore.Set("State",State);

    if (has_cc){
        indata->Get("CCData",*MRDData);
        MRDData->Save("tmp");
        m_data->Stores["CCData"]->Set("FileData",MRDData,false);
    }

    if (has_pmt){
        indata->Get("PMTData",*PMTData);
        PMTData->Save("tmp");
        m_data->Stores["PMTData"]->Set("FileData",PMTData,false);
    }

    i_loop++;
    }

    return true;
}


bool MonitorSimReceive::Finalise(){

    if (verbosity > 2) std::cout <<"MonitorSimReceive: Finalising"<<std::endl;

    if (indata!=0){ indata->Close(); indata->Delete(); delete indata; indata = 0;}

    if (m_data->CStore.Has("State")) m_data->CStore.Remove("State");
    if (m_data->CStore.Has("HasCCData")) m_data->CStore.Remove("HasCCData");
    if (m_data->CStore.Has("HasPMTData")) m_data->CStore.Remove("HasPMTData");
    if (m_data->Stores["CCData"]->Has("FileData")) m_data->Stores["CCData"]->Remove("FileData");
    if (m_data->Stores["PMTData"]->Has("FileData")) m_data->Stores["PMTData"]->Remove("FileData");
    m_data->Stores["CCData"]->Close(); m_data->Stores["CCData"]->Delete(); 
    m_data->Stores["PMTData"]->Close(); m_data->Stores["PMTData"]->Delete(); 
    m_data->Stores.clear();

    return true;
}
