#include "MonitorSimReceiveLAPPD.h"

MonitorSimReceiveLAPPD::MonitorSimReceiveLAPPD():Tool(){}


bool MonitorSimReceiveLAPPD::Initialise(std::string configfile, DataModel &data){

  /////////////////// Useful header ///////////////////////
  if(configfile!="") m_variables.Initialise(configfile); // loading config file
  //m_variables.Print();

  m_data= &data; //assigning transient data pointer
  /////////////////////////////////////////////////////////////////

  m_variables.Get("OutPath",outpath);
  m_variables.Get("Mode",mode);		//Mode: Wait/Single
  m_variables.Get("verbose",verbosity);

  m_data->CStore.Set("OutPath",outpath);

  if (verbosity > 2) std::cout <<"MonitorSimReceiveLAPPD: Initialising"<<std::endl;

  m_data->Stores["LAPPDData"] = new BoostStore(false,2);

  return true;
}


bool MonitorSimReceiveLAPPD::Execute(){

  if (verbosity > 2) std::cout <<"MonitorSimReceiveLAPPD: Executing"<<std::endl;

  if (mode == "Wait"){
    std::string Wait = "Wait";
    m_data->CStore.Set("State",Wait);
    return true;
  }
  else if (mode == "Single"){
    std::string State = "LAPPDSC";
    m_data->CStore.Set("State",State);
    SlowControlMonitor lappd_sc;
    TRandom3 rand(0);
    lappd_sc.humidity_mon = 40*rand.Gaus(1,0.02);
    lappd_sc.temperature_mon = 83*rand.Gaus(1,0.02);
    lappd_sc.HV_mon=1;
    lappd_sc.HV_state_set=true;
    lappd_sc.HV_volts=1000*rand.Gaus(1,0.02);
    lappd_sc.LV_mon=1;
    lappd_sc.LV_state_set=true;
    lappd_sc.v33=3.3*rand.Gaus(1,0.01);
    lappd_sc.v25=2.5*rand.Gaus(1,0.01);
    lappd_sc.v12=1.2*rand.Gaus(1,0.01);
    lappd_sc.LIMIT_temperature_low = 30;
    lappd_sc.LIMIT_temperature_high = 50;
    lappd_sc.LIMIT_humidity_low = 75;
    lappd_sc.LIMIT_humidity_high = 90;
    lappd_sc.FLAG_temperature = 1;
    lappd_sc.FLAG_humidity = 0;
    lappd_sc.relayCh1 = true;
    lappd_sc.relayCh2 = true;
    lappd_sc.relayCh3 = false;
    lappd_sc.relayCh1_mon = true;
    lappd_sc.relayCh2_mon = true;
    lappd_sc.relayCh3_mon = true;
    lappd_sc.TrigVref = 2.981*rand.Gaus(1,0.01);
    lappd_sc.Trig1_threshold = 1;
    lappd_sc.Trig1_mon = 1.*rand.Gaus(1,0.05);
    lappd_sc.Trig0_threshold = 1;
    lappd_sc.Trig0_mon = 1.*rand.Gaus(1,0.05);
    lappd_sc.light = 0.8*rand.Gaus(1,0.05);
    std::vector<unsigned int> vec_errors;
    int numberOfErrors = (int)rand.Gaus(9, 4);
    for(int i = 0; i < 11; i++){
    	vec_errors.push_back((int)rand.Gaus(30, 10));
    }
    lappd_sc.errorcodes = vec_errors;
    m_data->Stores["LAPPDData"]->Set("LAPPDSC",lappd_sc);
  }

  return true;
}


bool MonitorSimReceiveLAPPD::Finalise(){

  if (verbosity > 2) std::cout <<"MonitorSimReceiveLAPPD: Finalising"<<std::endl;

  return true;
}
