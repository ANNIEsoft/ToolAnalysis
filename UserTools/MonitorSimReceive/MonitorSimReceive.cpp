#include "MonitorSimReceive.h"

MonitorSimReceive::MonitorSimReceive():Tool(){}


bool MonitorSimReceive::Initialise(std::string configfile, DataModel &data){

    /////////////////// Usefull header ///////////////////////
    if(configfile!="")  m_variables.Initialise(configfile); //loading config file
    //m_variables.Print();

    m_data= &data; //assigning transient data pointer
    /////////////////////////////////////////////////////////////////

    m_variables.Get("MRDDataPath", MRDDataPathSingle);
    m_variables.Get("MRDTxtFile", MRD_path_to_file);
    m_variables.Get("Mode",mode);
    m_variables.Get("verbose",verbosity);
    //m_variables.Print();

    if (verbosity > 2) std::cout <<"MonitorSimReceive: Initialising"<<std::endl;

    if (verbosity > 2) {
        std::cout <<"MRDDataPath: "<<MRDDataPathSingle<<std::endl;
        std::cout <<"MRDTxtFile: "<<MRD_path_to_file<<std::endl;
        std::cout <<"Mode: "<<mode<<std::endl;
    }

    if (mode != "Random" && mode != "FileList") mode = "Random";

    if (mode == "FileList"){

        ifstream file_sim(MRD_path_to_file);
        while(!file_sim.eof()){
            std::string string_temp;
            file_sim >> string_temp;
            if (string_temp!="") vec_filename.push_back(string_temp);
            std::cout <<string_temp<<std::endl;
        }
        file_sim.close();

        std::cout <<"Data Filelist vector size: "<<vec_filename.size()<<std::endl;
    }

    if (verbosity > 2) std::cout <<"Define CCData BoostStore"<<std::endl;

    srand(time(NULL));
    m_data->Stores["CCData"]=new BoostStore(false,2);  
    //m_data->Stores["CCData"]->Save("tmp");

    i_loop = 0;
    return true;
}


bool MonitorSimReceive::Execute(){

    if (verbosity > 2) std::cout <<"MonitorSimReceive: Executing"<<std::endl;

    if (mode == "FileList"){

        if (i_loop<vec_filename.size()){
            MRDDataPath = vec_filename.at(i_loop);
            if (verbosity > 2) std::cout <<"MRDDataPath: "<<MRDDataPath<<std::endl;
        } else {
            MRDDataPath = MRDDataPathSingle;
        }
    } else {
        MRDDataPath = MRDDataPathSingle;
        if (verbosity > 2) std::cout <<"MRDDataPath: "<<MRDDataPath<<std::endl;
    } 

    //std::cout <<"initialise indata"<<std::endl;
    BoostStore* indata=new BoostStore(false,0); //this leaks but its jsut for testing
    indata->Initialise(MRDDataPath);

    //std::cout <<"delete MRDData/2"<<std::endl;

    //the following two lines do not work to free up memory
    //if (MRDData) delete MRDData;
    //if (MRDData2) delete MRDData2;

    MRDData = new BoostStore(false,2);
    MRDData2 = new BoostStore(false,2);


    //std::cout <<"Get MRDData from CCData"<<std::endl;
    indata->Get("CCData",*MRDData);
    indata->Get("CCData",*MRDData2);

    int a=rand() % 10;
    int b=rand() % 100;

    if (mode == "Random"){
        if(!a){
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
        else if(!b){    
            std::string State="DataFile";
            m_data->CStore.Set("State",State);
            MRDData2->Save("tmp");
            m_data->Stores["CCData"]->Set("FileData",MRDData2,false);
            } else{
            std::string State="Wait";
            m_data->CStore.Set("State",State);
        }
    } else if (mode == "FileList"){
        if (i_loop<vec_filename.size()){          //plot all files in Time Plot
            std::string State="DataFile";
            m_data->CStore.Set("State",State);
            MRDData2->Save("tmp");
            m_data->Stores["CCData"]->Set("FileData",MRDData2,false);
        }
        else {                  //then test the Live tool
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
    }

    i_loop++;

    delete indata;


    return true;
}


bool MonitorSimReceive::Finalise(){

    if (verbosity > 2) std::cout <<"MonitorSimReceive: Finalising"<<std::endl;

    MRDData=0;
    m_data->CStore.Remove("State");
    m_data->Stores["CCData"]->Remove("FileData");
    m_data->Stores.clear();

    //delete MRDData;
    //delete MRDData2;

    return true;
}
