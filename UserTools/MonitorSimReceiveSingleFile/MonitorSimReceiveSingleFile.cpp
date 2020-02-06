#include "MonitorSimReceiveSingleFile.h"

MonitorSimReceiveSingleFile::MonitorSimReceiveSingleFile():Tool(){}


bool MonitorSimReceiveSingleFile::Initialise(std::string configfile, DataModel &data){

    /////////////////// Usefull header ///////////////////////
    if(configfile!="")  m_variables.Initialise(configfile); //loading config file
    //m_variables.Print();

    m_data= &data; //assigning transient data pointer
    /////////////////////////////////////////////////////////////////

    m_variables.Get("MRDDataPath", MRDDataPath);
    m_variables.Get("Mode",mode);
    m_variables.Get("OutPath",outpath);
    m_variables.Get("verbose",verbosity);
    //m_variables.Print();

    m_data->CStore.Set("OutPath",outpath);

    if (verbosity > 2) {
        std::cout <<"MRDDataPath: "<<MRDDataPath<<std::endl;
        std::cout <<"Mode: "<<mode<<std::endl;
    }

    if (mode != "Single" && mode != "File") mode = "File";

    if (verbosity > 2) std::cout <<"Define CCData &PMTData BoostStores"<<std::endl;
    srand(time(NULL));
    m_data->Stores["CCData"]=new BoostStore(false,2);  
    m_data->Stores["PMTData"]=new BoostStore(false,2);    

    return true;
}


bool MonitorSimReceiveSingleFile::Execute(){
    
    BoostStore* indata=new BoostStore(false,0); //this leaks but its jsut for testing
    BoostStore* MRDData = new BoostStore(false,2);
    BoostStore* PMTData = new BoostStore(false,2);   

    std::cout <<"initialise indata"<<std::endl;
    indata->Initialise(MRDDataPath);
    indata->Print(false); 
    indata->Get("CCData",*MRDData);
    indata->Get("PMTData",*PMTData);
    
    if (mode == "File"){
            std::string State="DataFile";
            std::cout <<"Set state"<<std::endl;
            m_data->CStore.Set("State",State);
            MRDData->Save("tmp");
            std::cout <<"Set MRDData"<<std::endl;
            m_data->Stores["CCData"]->Set("FileData",MRDData,false);
            PMTData->Save("tmp");
            std::cout <<"Set PMTData"<<std::endl;
            m_data->Stores["PMTData"]->Set("FileData",PMTData,false);
    }
    if (mode == "Single"){
            int event=rand() % 1000;
	    std::string State="MRDSingle";
            m_data->CStore.Set("State",State);
            MRDOut tmp;
            long entries;
            MRDData->Header->Get("TotalEntries",entries);
            MRDData->GetEntry(event);
            MRDData->Get("Data", tmp);
            m_data->Stores["CCData"]->Set("Single",tmp);
    }

    return true;
}


bool MonitorSimReceiveSingleFile::Finalise(){

    m_data->CStore.Remove("State");
    m_data->Stores["CCData"]->Remove("FileData");
    m_data->Stores["PMTData"]->Remove("FileData");
 //   m_data->Stores.clear();

    return true;
}

