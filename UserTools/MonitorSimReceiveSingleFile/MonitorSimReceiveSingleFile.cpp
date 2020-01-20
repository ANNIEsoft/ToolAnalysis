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
    m_variables.Get("verbose",verbosity);
    //m_variables.Print();

    if (verbosity > 2) {
        std::cout <<"MRDDataPath: "<<MRDDataPath<<std::endl;
        std::cout <<"Mode: "<<mode<<std::endl;
    }

    if (mode != "Single" && mode != "File") mode = "File";


    if (verbosity > 2) std::cout <<"Define CCData BoostStore"<<std::endl;
    srand(time(NULL));
    m_data->Stores["CCData"]=new BoostStore(false,2);  
    
    BoostStore* indata=new BoostStore(false,0); //this leaks but its jsut for testing
    indata->Initialise(MRDDataPath);
    
    MRDData = new BoostStore(false,2);
    MRDData2 = new BoostStore(false,2);
    
    indata->Get("CCData",*MRDData);
    indata->Get("CCData",*MRDData2);
 
    return true;
}


bool MonitorSimReceiveSingleFile::Execute(){

    if (mode == "File"){
            std::string State="DataFile";
            m_data->CStore.Set("State",State);
            MRDData2->Save("tmp");
            m_data->Stores["CCData"]->Set("FileData",MRDData2,false);
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

    MRDData=0;
    m_data->CStore.Remove("State");
    m_data->Stores["CCData"]->Remove("FileData");
    m_data->Stores.clear();

    delete MRDData;
    delete MRDData2;

    return true;
}

