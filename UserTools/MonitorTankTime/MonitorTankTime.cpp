#include "MonitorTankTime.h"

MonitorTankTime::MonitorTankTime():Tool(){}


bool MonitorTankTime::Initialise(std::string configfile, DataModel &data){

  if(configfile!="")  m_variables.Initialise(configfile); //loading config file
  //m_variables.Print();
  m_data= &data; //assigning transient data pointer

  if (verbosity > 1) std::cout <<"Tool MonitorTankTime: Initialising...."<<std::endl;

  //-------------------------------------------------------
  //---------------Initialise config file------------------
  //-------------------------------------------------------

  if(configfile!="")  m_variables.Initialise(configfile); //loading config file
  m_data= &data; //assigning transient data pointer

  //only for debugging memory leaks, otherwise comment out
  //std::cout <<"List of Objects (beginning of initialise): "<<std::endl;
  //gObjectTable->Print();

  //-------------------------------------------------------
  //-----------------Get Configuration---------------------
  //-------------------------------------------------------

  m_variables.Get("verbose",verbosity);
  m_variables.Get("OutputPath",outpath_temp);
  m_variables.Get("ActiveSlots",active_slots);
  m_variables.Get("StartTime", StartTime);

  if (verbosity > 2) std::cout <<"MonitorTankTime: Outpath (temporary): "<<outpath_temp<<std::endl;
  if (outpath_temp == "fromStore") m_data->CStore.Get("OutPath",outpath);
  else outpath = outpath_temp;
  if (verbosity > 2) std::cout <<"MonitorTankTime: Output path for plots is "<<outpath<<std::endl;

  Epoch = new boost::posix_time::ptime(boost::gregorian::from_string(StartTime));

  num_active_slots=0;
  num_active_slots_cr1=0;
  num_active_slots_cr2=0;

  //-------------------------------------------------------
  //-----------------Get active channels-------------------
  //-------------------------------------------------------


  //-------------------------------------------------------
  //----------Initialize storing containers----------------
  //-------------------------------------------------------


  //-------------------------------------------------------
  //---------------Set initial conditions------------------
  //-------------------------------------------------------

 

  //-------------------------------------------------------
  //------Setup time variables for periodic updates--------
  //-------------------------------------------------------

  period_update = boost::posix_time::time_duration(0,5,0,0);
  last = boost::posix_time::ptime(boost::posix_time::second_clock::local_time());

  //omit warning messages from ROOT: 1001 - info messages, 2001 - warnings, 3001 - errors
  gROOT->ProcessLine("gErrorIgnoreLevel = 3001;");
  
  return true;
}


bool MonitorTankTime::Execute(){

  if (verbosity > 3) std::cout <<"Tool MonitorTankTime: Executing ...."<<std::endl;

  current = (boost::posix_time::second_clock::local_time());
  duration = boost::posix_time::time_duration(current - last); 

  if (verbosity > 2) std::cout <<"MonitorTankTime: "<<duration.total_milliseconds()/1000./60.<<" mins since last time plot"<<std::endl;

  std::string State;
  m_data->CStore.Get("State",State);

  if (State == "PMTSingle" || State == "Wait"){

    //-------------------------------------------------------
    //--------------MRDMonitorLive executed------------------
    //-------------------------------------------------------

    if (verbosity > 3) std::cout <<"MonitorTankTime: State is "<<State<<std::endl;

  }else if (State == "DataFile"){

    //-------------------------------------------------------
    //--------------MonitorTankTime executed------------------
    //-------------------------------------------------------

   	if (verbosity > 1) std::cout<<"MonitorTankTime: New data file available."<<std::endl;
   	//m_data->Stores["CCData"]->Get("FileData",PMTdata);
    //PMTdata->Print(false);

  }else {
   	if (verbosity > 1) std::cout <<"MonitorTankTime: State not recognized: "<<State<<std::endl;
  }

  //-------------------------------------------------------
  //-----------Has enough time passed for update?----------
  //-------------------------------------------------------

  if(duration>=period_update){
    if (verbosity > 1) std::cout <<"MonitorTankTime: 5 mins passed... Updating plots!"<<std::endl;

    //.....
    //.....

    //


    if (verbosity > 1) std::cout <<"-----------------------------------------------------------------------------------"<<std::endl; //marking end of one plotting period
	}
  

  //only for debugging memory leaks, otherwise comment out
  //std::cout <<"List of Objects (after execute step): "<<std::endl;
  //gObjectTable->Print();

  return true;

}


bool MonitorTankTime::Finalise(){

  if (verbosity > 1) std::cout <<"Tool MonitorTankTime: Finalising ...."<<std::endl;

  //PMTdata->Delete();

  return true;
}

