#include "MonitorSimReceiveLAPPDData.h"

MonitorSimReceiveLAPPDData::MonitorSimReceiveLAPPDData():Tool(){}


bool MonitorSimReceiveLAPPDData::Initialise(std::string configfile, DataModel &data){

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

    if (verbosity > 2) std::cout <<"MonitorSimReceiveLAPPDData: Initialising"<<std::endl;

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

    if (verbosity > 2) std::cout <<"Define LAPPDData BoostStore"<<std::endl;

    srand(time(NULL));
    m_data->Stores["LAPPDData"]=new BoostStore(false,2);

    LAPPDData = 0;
    i_loop = 0;

    if (mode == "Single"){
      std::string datapath = vec_filename.at(0);
      LAPPDData = new BoostStore(false,2);
      bool has_lappd = LAPPDData->Initialise(datapath);
    }


    return true;
}


bool MonitorSimReceiveLAPPDData::Execute(){

    if (verbosity > 2) std::cout <<"MonitorSimReceiveLAPPDData: Executing"<<std::endl;

    m_data->CStore.Set("HasLAPPDData",false);

    if (mode == "Wait"){
      std::string Wait = "Wait";     
      m_data->CStore.Set("State",Wait);
      return true;
    }
    else if (mode == "Single"){
       m_data->CStore.Set("HasLAPPDData",true);
       long entries = LAPPDData->Header->Get("TotalEntries",entries);
       int event=rand() % entries;
       std::string State="LAPPDSingle";
       m_data->CStore.Set("State",State);
       std::cout <<"event: "<<event<<", entries: "<<entries<<std::endl;
       if (event < entries) LAPPDData->GetEntry(event);
       else LAPPDData->GetEntry(entries-1);
       std::map<unsigned long, std::vector<Waveform<double>>> RawLAPPDData;
       std::vector<unsigned short> Metadata;
       std::vector<unsigned short> AccInfoFrame;
       LAPPDData->Get("RawLAPPDData",RawLAPPDData);
       LAPPDData->Get("Metadata",Metadata);
       LAPPDData->Get("AccInfoFrame",AccInfoFrame);
       m_data->Stores["LAPPDData"]->Set("RawLAPPDData",RawLAPPDData);
       m_data->Stores["LAPPDData"]->Set("Metadata",Metadata);
       m_data->Stores["LAPPDData"]->Set("AccInfoFrame",AccInfoFrame);
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
      if (LAPPDData!=0) {LAPPDData->Close(); LAPPDData->Delete(); delete LAPPDData; LAPPDData = 0;}
      return true;
    }

    if (LAPPDData!=0){
      m_data->Stores["LAPPDData"]->Delete();
      /*LAPPDData->Close();
      LAPPDData->Delete();
      delete LAPPDData;
      LAPPDData=0;*/
    }

    std::cout <<"Read in new LAPPD data"<<std::endl;
    LAPPDData=new BoostStore(false,2); //this leaks but its jsut for testing
    bool has_lappd = LAPPDData->Initialise(datapath);
    
    std::cout <<"Print LAPPD data:"<<std::endl;
    LAPPDData->Print(false);

    long entries;
    LAPPDData->Header->Get("TotalEntries",entries);
    std::cout <<"# of entries: "<<entries<<std::endl;

    std::cout <<"datapath: "<<datapath<<std::endl;

    m_data->CStore.Set("HasNewFile",true);
    m_data->CStore.Set("CurrentFileName",datapath);

    
    //Check the size of the current file
    uintmax_t current_filesize = boost::filesystem::file_size(datapath.c_str());
    m_data->CStore.Set("CurrentFileSize",current_filesize);

    std::time_t current_filetime = boost::filesystem::last_write_time(datapath.c_str());
    m_data->CStore.Set("CurrentFileTime",current_filetime);

    m_data->CStore.Set("HasLAPPDData",has_lappd);
    std::string State="DataFile";
    m_data->CStore.Set("State",State);

    if (has_lappd){
      long totalentries_lappd;
      LAPPDData->Header->Get("TotalEntries",totalentries_lappd);
      m_data->Stores["LAPPDData"]->Set("LAPPDData",LAPPDData,false);
      //TODO
      //Potentially just transmit already processed data (depending on format)
    }

    i_loop++;
    }

    return true;
}


bool MonitorSimReceiveLAPPDData::Finalise(){

    if (verbosity > 2) std::cout <<"MonitorSimReceiveLAPPDData: Finalising"<<std::endl;

    if (m_data->CStore.Has("State")) m_data->CStore.Remove("State");
    if (m_data->CStore.Has("HasLAPPDData")) m_data->CStore.Remove("HasLAPPDData");
    if (m_data->Stores["LAPPDData"]->Has("LAPPDData")) m_data->Stores["LAPPDData"]->Remove("LAPPDData");
    m_data->Stores["LAPPDData"]->Close(); m_data->Stores["LAPPDData"]->Delete();
    m_data->Stores.clear();

    return true;
}
