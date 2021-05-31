#include "MonitorLAPPDSC.h"

MonitorLAPPDSC::MonitorLAPPDSC():Tool(){}


bool MonitorLAPPDSC::Initialise(std::string configfile, DataModel &data){

  /////////////////// Useful header ///////////////////////
  if(configfile!="") m_variables.Initialise(configfile); // loading config file
  //m_variables.Print();

  m_data= &data; //assigning transient data pointer
  /////////////////////////////////////////////////////////////////

  //gObjectTable only for debugging memory leaks, otherwise comment out
  //std::cout <<"MonitorMRDTime: List of Objects (beginning of Initialise): "<<std::endl;
  //gObjectTable->Print();

  //-------------------------------------------------------
  //-----------------Get Configuration---------------------
  //-------------------------------------------------------

  update_frequency = 0.;

  m_variables.Get("OutputPath",outpath_temp);
  m_variables.Get("StartTime",StartTime);
  m_variables.Get("UpdateFrequency",update_frequency);
  m_variables.Get("PathMonitoring",path_monitoring);
  m_variables.Get("PlotConfiguration",plot_configuration);
  m_variables.Get("ImageFormat",img_extension);
  m_variables.Get("ForceUpdate",force_update);
  m_variables.Get("DrawMarker",draw_marker);
  m_variables.Get("verbose",verbosity);

  if (verbosity > 1) std::cout <<"Tool MonitorLAPPDSC: Initialising...."<<std::endl;
  // Update frequency specifies the frequency at which the File Log Histogram is updated
  // All other monitor plots are updated as soon as a new file is available for readout
  if (update_frequency < 0.1) {
    if (verbosity > 0) std::cout <<"MonitorLAPPDSC: Update Frequency of "<<update_frequency<<" mins is too low. Setting default value of 1 min."<<std::endl;
    update_frequency = 1.;
  }

  //default should be no forced update of the monitoring plots every execute step
  if (force_update !=0 && force_update !=1) {
    force_update = 0;
  }

  //check if the image format is jpg or png
  if (!(img_extension == "png" || img_extension == "jpg" || img_extension == "jpeg")){
    img_extension = "jpg";
  }

  //Print out path to monitoring files
  std::cout <<"PathMonitoring: "<<path_monitoring<<std::endl;

  //Set up Epoch
  Epoch = new boost::posix_time::ptime(boost::gregorian::from_string(StartTime));

  //Evaluating output path for monitoring plots
  if (outpath_temp == "fromStore") m_data->CStore.Get("OutPath",outpath);
  else outpath = outpath_temp;
  if (verbosity > 2) std::cout <<"MonitorLAPPDSC: Output path for plots is "<<outpath<<std::endl;

  //-------------------------------------------------------
  //----------Initialize histograms/canvases---------------
  //-------------------------------------------------------

  InitializeHists();

  //-------------------------------------------------------
  //----------Read in configuration option for plots-------
  //-------------------------------------------------------

  ReadInConfiguration();

  //-------------------------------------------------------
  //------Setup time variables for periodic updates--------
  //-------------------------------------------------------
  
  period_update = boost::posix_time::time_duration(0,int(update_frequency),0,0);
  last = boost::posix_time::ptime(boost::posix_time::second_clock::local_time());

  
  // Omit warning messages from ROOT: 1001 - info messages, 2001 - warnings, 3001 - errors
  gROOT->ProcessLine("gErrorIgnoreLevel = 3001;");


  return true;
}


bool MonitorLAPPDSC::Execute(){

  if (verbosity > 10) std::cout <<"MonitorLAPPDSC: Executing ...."<<std::endl;

  current = (boost::posix_time::second_clock::local_time());
  duration = boost::posix_time::time_duration(current - last);
  current_stamp_duration = boost::posix_time::time_duration(current - *Epoch);
  current_stamp = current_stamp_duration.total_milliseconds();
  utc = (boost::posix_time::second_clock::universal_time());
  current_utc_duration = boost::posix_time::time_duration(utc-current);
  current_utc = current_utc_duration.total_milliseconds();
  utc_to_t = (ULong64_t) current_utc;
  t_current = (ULong64_t) current_stamp;

  //------------------------------------------------------------
  //---------Checking the state of LAPPD SC data stream---------
  //------------------------------------------------------------
  
  std::string State;
  m_data->CStore.Get("State",State);

  if (State == "Wait"){
    if (verbosity > 2) std::cout <<"MonitorLAPPDSC: State is "<<State<<std::endl;
  }
  else if (State == "LAPPDSC"){
    if (verbosity > 1) std::cout<<"MonitorLAPPDSC: New slow-control data available."<<std::endl; 

    if (duration > period_update) {

      m_data->Stores["LAPPDData"]->Get("LAPPDSC",lappd_sc);

      //Write the event information to a file
      //TODO: change this to a database later on!
      //Check if data has already been written included in WriteToFile function
      WriteToFile();

      //Plot plots only associated to current file
      DrawLastFilePlots();

      //Draw customly defined plots   
      UpdateMonitorPlots(config_timeframes, config_endtime_long, config_label, config_plottypes);

      last = current;

    }
  }
  else {
    if (verbosity > 1) std::cout <<"MonitorLAPPDSC: State not recognized: "<<State<<std::endl;
  }

  // if force_update is specified, the plots will be updated no matter whether there has been a new file or not
  if (force_update) UpdateMonitorPlots(config_timeframes, config_endtime_long, config_label, config_plottypes);

  //gObjectTable only for debugging memory leaks, otherwise comment out
  //std::cout <<"List of Objects (after execute step): "<<std::endl;
  //gObjectTable->Print();

  return true;
}


bool MonitorLAPPDSC::Finalise(){

  if (verbosity > 1) std::cout <<"Tool MonitorLAPPDSC: Finalising ...."<<std::endl;

  //timing pointers
  delete Epoch;

  //canvas
  delete canvas_temp;
  delete canvas_humidity;
  delete canvas_light;
  delete canvas_hv;
  delete canvas_lv;
  delete canvas_status_temphum;
  delete canvas_status_lvhv;
  delete canvas_status_relay;
  delete canvas_status_trigger;
  delete canvas_status_error;

  //graphs
  delete graph_temp;
  delete graph_humidity;
  delete graph_light;
  delete graph_hv_volt;
  delete graph_lv_volt1;
  delete graph_lv_volt2;
  delete graph_lv_volt3;

  //multi-graphs
  delete multi_lv;

  //legends
  delete leg_lv;

  //text
  delete text_temphum_title;
  delete text_temp;
  delete text_hum;
  delete text_light;
  delete text_flag_temp;
  delete text_flag_hum;

  return true;
}

void MonitorLAPPDSC::InitializeHists(){

  if (verbosity > 2) std::cout <<"MonitorLAPPDSC: InitializeHists"<<std::endl;

  //Canvas
  canvas_temp = new TCanvas("canvas_temp","LVHV Temperature",900,600);
  canvas_humidity = new TCanvas("canvas_humidity","LVHV Humidity",900,600);
  canvas_light = new TCanvas("canvas_light","LVHV Light",900,600);
  canvas_hv = new TCanvas("canvas_hv","LVHV HV",900,600);
  canvas_lv = new TCanvas("canvas_lv","LVHV LV",900,600);
  canvas_status_temphum = new TCanvas("canvas_status_temphum","Temperature / Humidity status",900,600);
  canvas_status_lvhv = new TCanvas("canvas_status_lvhv","LV / HV status",900,600);
  canvas_status_relay = new TCanvas("canvas_status_relay","Relay status",900,600);
  canvas_status_trigger = new TCanvas("canvas_status_trigger","Trigger status",900,600);
  canvas_status_error = new TCanvas("canvas_status_error","Error status",900,600);

  //Graphs
  graph_temp = new TGraph();
  graph_humidity = new TGraph();
  graph_light = new TGraph();
  graph_hv_volt = new TGraph();
  graph_lv_volt1 = new TGraph();
  graph_lv_volt2 = new TGraph();
  graph_lv_volt3 = new TGraph();

  graph_temp->SetName("graph_temp");
  graph_humidity->SetName("graph_humidity");
  graph_light->SetName("graph_light");
  graph_hv_volt->SetName("graph_hv_volt");
  graph_lv_volt1->SetName("graph_lv_volt1");
  graph_lv_volt2->SetName("graph_lv_volt2");
  graph_lv_volt3->SetName("graph_lv_volt3");

  graph_temp->SetTitle("LVHV temperature time evolution");
  graph_humidity->SetTitle("LVHV humidity time evolution");
  graph_light->SetTitle("LVHV light level time evolution");
  graph_hv_volt->SetTitle("LVHV HV time evolution");
  graph_lv_volt1->SetTitle("LVHV LV1 time evolution");
  graph_lv_volt2->SetTitle("LVHV LV2 time evolution");
  graph_lv_volt3->SetTitle("LVHV LV3 time evolution");

  if (draw_marker){
    graph_temp->SetMarkerStyle(20);
    graph_humidity->SetMarkerStyle(20);
    graph_light->SetMarkerStyle(20);
    graph_hv_volt->SetMarkerStyle(20);
    graph_lv_volt1->SetMarkerStyle(20);
    graph_lv_volt2->SetMarkerStyle(20);
    graph_lv_volt3->SetMarkerStyle(20);
  }

  graph_temp->SetMarkerColor(kBlack);
  graph_humidity->SetMarkerColor(kBlack);
  graph_light->SetMarkerColor(kBlack);
  graph_hv_volt->SetMarkerColor(kBlack);
  graph_lv_volt1->SetMarkerColor(kBlack);
  graph_lv_volt2->SetMarkerColor(kRed);
  graph_lv_volt3->SetMarkerColor(kBlue);
  
  graph_temp->SetLineColor(kBlack);
  graph_humidity->SetLineColor(kBlack);
  graph_light->SetLineColor(kBlack);
  graph_hv_volt->SetLineColor(kBlack);
  graph_lv_volt1->SetLineColor(kBlack);
  graph_lv_volt2->SetLineColor(kRed);
  graph_lv_volt3->SetLineColor(kBlue);

  graph_temp->SetLineWidth(2);
  graph_humidity->SetLineWidth(2);
  graph_light->SetLineWidth(2);
  graph_hv_volt->SetLineWidth(2);
  graph_lv_volt1->SetLineWidth(2);
  graph_lv_volt2->SetLineWidth(2);
  graph_lv_volt3->SetLineWidth(2);

  graph_temp->SetFillColor(0);
  graph_humidity->SetFillColor(0);
  graph_light->SetFillColor(0);
  graph_hv_volt->SetFillColor(0);
  graph_lv_volt1->SetFillColor(0);
  graph_lv_volt2->SetFillColor(0);
  graph_lv_volt3->SetFillColor(0);

  graph_temp->GetYaxis()->SetTitle("Temperature");
  graph_humidity->GetYaxis()->SetTitle("Humidity");
  graph_light->GetYaxis()->SetTitle("Light level");
  graph_hv_volt->GetYaxis()->SetTitle("HV mon (V)");
  graph_lv_volt1->GetYaxis()->SetTitle("LV mon (V)");
  graph_lv_volt2->GetYaxis()->SetTitle("LV mon (V)");
  graph_lv_volt3->GetYaxis()->SetTitle("LV mon (V)");

  graph_temp->GetXaxis()->SetTimeDisplay(1);
  graph_humidity->GetXaxis()->SetTimeDisplay(1);
  graph_light->GetXaxis()->SetTimeDisplay(1);
  graph_hv_volt->GetXaxis()->SetTimeDisplay(1);
  graph_lv_volt1->GetXaxis()->SetTimeDisplay(1);
  graph_lv_volt2->GetXaxis()->SetTimeDisplay(1);
  graph_lv_volt3->GetXaxis()->SetTimeDisplay(1);
 
  graph_temp->GetXaxis()->SetLabelSize(0.03);
  graph_humidity->GetXaxis()->SetLabelSize(0.03);
  graph_light->GetXaxis()->SetLabelSize(0.03);
  graph_hv_volt->GetXaxis()->SetLabelSize(0.03);
  graph_lv_volt1->GetXaxis()->SetLabelSize(0.03);
  graph_lv_volt2->GetXaxis()->SetLabelSize(0.03);
  graph_lv_volt3->GetXaxis()->SetLabelSize(0.03);
 
  graph_temp->GetXaxis()->SetLabelOffset(0.03);
  graph_humidity->GetXaxis()->SetLabelOffset(0.03);
  graph_light->GetXaxis()->SetLabelOffset(0.03);
  graph_hv_volt->GetXaxis()->SetLabelOffset(0.03);
  graph_lv_volt1->GetXaxis()->SetLabelOffset(0.03);
  graph_lv_volt2->GetXaxis()->SetLabelOffset(0.03);
  graph_lv_volt3->GetXaxis()->SetLabelOffset(0.03);

  graph_temp->GetXaxis()->SetTimeFormat("#splitline(%m/%d}{%H:%M}");
  graph_humidity->GetXaxis()->SetTimeFormat("#splitline(%m/%d}{%H:%M}");
  graph_light->GetXaxis()->SetTimeFormat("#splitline(%m/%d}{%H:%M}");
  graph_hv_volt->GetXaxis()->SetTimeFormat("#splitline(%m/%d}{%H:%M}");
  graph_lv_volt1->GetXaxis()->SetTimeFormat("#splitline(%m/%d}{%H:%M}");
  graph_lv_volt2->GetXaxis()->SetTimeFormat("#splitline(%m/%d}{%H:%M}");
  graph_lv_volt3->GetXaxis()->SetTimeFormat("#splitline(%m/%d}{%H:%M}");

  //Multi-Graphs
  multi_lv = new TMultiGraph();

  //Legends
  leg_lv = new TLegend(0.7,0.7,0.88,0.88);
  leg_lv->SetLineColor(0);

  //Text
  text_temphum_title = new TText();
  text_temp = new TText();
  text_hum = new TText();
  text_light = new TText();
  text_flag_temp = new TText();
  text_flag_hum = new TText();

  text_temphum_title->SetNDC(1);
  text_temp->SetNDC(1);
  text_hum->SetNDC(1);
  text_light->SetNDC(1);
  text_flag_temp->SetNDC(1);
  text_flag_hum->SetNDC(1);

}

void MonitorLAPPDSC::ReadInConfiguration(){

  //-------------------------------------------------------
  //----------------ReadInConfiguration -------------------
  //-------------------------------------------------------

  Log("MonitorLAPPDSC::ReadInConfiguration",v_message,verbosity);

  ifstream file(plot_configuration.c_str());

  std::string line;
  if (file.is_open()){
    while(std::getline(file,line)){
      if (line.find("#") != std::string::npos) continue;
      std::vector<std::string> values;
      std::stringstream ss;
        ss.str(line);
        std::string item;
        while (std::getline(ss, item, '\t')) {
            values.push_back(item);
        }
      if (values.size() < 4 ) {
        if (verbosity > 0) std::cout <<"ERROR (MonitorLAPPDSC): ReadInConfiguration: Need at least 4 arguments in one line: TimeFrame - TimeEnd - FileLabel - PlotType1. Please look over the configuration file and adjust it accordingly."<<std::endl;
        continue;
      }
      double double_value = std::stod(values.at(0));
      config_timeframes.push_back(double_value);
      config_endtime.push_back(values.at(1));
      config_label.push_back(values.at(2));
      std::vector<std::string> plottypes;
      for (unsigned int i=3; i < values.size(); i++){
        plottypes.push_back(values.at(i));
      }
      config_plottypes.push_back(plottypes);
    }
  } else {
    if (verbosity > 0) std::cout <<"ERROR (MonitorLAPPDSC): ReadInConfiguration: Could not open file "<<plot_configuration<<"! Check if path is valid..."<<std::endl;
  }
  file.close();


  if (verbosity > 2){
    std::cout <<"---------------------------------------------------------------------"<<std::endl;
    std::cout <<"MonitorLAPPDSC: ReadInConfiguration: Read in the following data into configuration variables: "<<std::endl;
    for (unsigned int i_t=0; i_t < config_timeframes.size(); i_t++){
      std::cout <<config_timeframes.at(i_t)<<", "<<config_endtime.at(i_t)<<", "<<config_label.at(i_t)<<", ";
      for (unsigned int i_plot = 0; i_plot < config_plottypes.at(i_t).size(); i_plot++){
        std::cout <<config_plottypes.at(i_t).at(i_plot)<<", ";
      }
      std::cout<<std::endl;
    }
    std::cout <<"-----------------------------------------------------------------------"<<std::endl;
  }

  if (verbosity > 2) std::cout <<"MonitorLAPPDSC: ReadInConfiguration: Parsing dates: "<<std::endl;
  for (unsigned int i_date = 0; i_date < config_endtime.size(); i_date++){
    if (config_endtime.at(i_date) == "TEND_LASTFILE") {
      if (verbosity > 2) std::cout <<"TEND_LASTFILE: Starting from end of last read-in file"<<std::endl;
      ULong64_t zero = 0; 
      config_endtime_long.push_back(zero);
    } else if (config_endtime.at(i_date).size()==15){
        boost::posix_time::ptime spec_endtime(boost::posix_time::from_iso_string(config_endtime.at(i_date)));
        boost::posix_time::time_duration spec_endtime_duration = boost::posix_time::time_duration(spec_endtime - *Epoch);
        ULong64_t spec_endtime_long = spec_endtime_duration.total_milliseconds();
        config_endtime_long.push_back(spec_endtime_long);
    } else {
      if (verbosity > 2) std::cout <<"Specified end date "<<config_endtime.at(i_date)<<" does not have the desired format YYYYMMDDTHHMMSS. Please change the format in the config file in order to use this tool. Starting from end of last file"<<std::endl;
      ULong64_t zero = 0;
      config_endtime_long.push_back(zero);
    }
  }
}

std::string MonitorLAPPDSC::convertTimeStamp_to_Date(ULong64_t timestamp){

    //format of date is YYYY_MM-DD
    
    boost::posix_time::ptime convertedtime = *Epoch + boost::posix_time::time_duration(int(timestamp/MSEC_to_SEC/SEC_to_MIN/MIN_to_HOUR),int(timestamp/MSEC_to_SEC/SEC_to_MIN)%60,int(timestamp/MSEC_to_SEC/1000.)%60,timestamp%1000);
    struct tm converted_tm = boost::posix_time::to_tm(convertedtime);
    std::stringstream ss_date;
    ss_date << converted_tm.tm_year+1900 << "_" << converted_tm.tm_mon+1 << "-" <<converted_tm.tm_mday;
    return ss_date.str();

}

bool MonitorLAPPDSC::does_file_exist(std::string filename){

  std::ifstream infile(filename.c_str());
  bool file_good = infile.good();
  infile.close();
  return file_good;

}

void MonitorLAPPDSC::WriteToFile(){

  Log("MonitorLAPPDSC: WriteToFile",v_message,verbosity);

  //-------------------------------------------------------
  //------------------WriteToFile -------------------------
  //-------------------------------------------------------

  std::string file_start_date = convertTimeStamp_to_Date(t_current);
  std::stringstream root_filename;
  root_filename << path_monitoring << "LAPPDSC_" << file_start_date << ".root";

  Log("MonitorLAPPDSC: ROOT filename: "+root_filename.str(),v_message,verbosity);

  std::string root_option = "RECREATE";
  if (does_file_exist(root_filename.str())) root_option = "UPDATE";
  TFile *f = new TFile(root_filename.str().c_str(),root_option.c_str());

  ULong64_t t_time;
  float t_hum;
  float t_temp;
  int t_hvmon;
  bool t_hvstateset;
  float t_hvvolt;
  int t_lvmon;
  bool t_lvstateset;
  float t_v33;
  float t_v25;
  float t_v12;
  float t_temp_low;
  float t_temp_high;
  float t_hum_low;
  float t_hum_high;
  int t_flag_temp;
  int t_flag_humidity;
  bool t_relCh1;
  bool t_relCh2;
  bool t_relCh3;
  bool t_relCh1_mon;
  bool t_relCh2_mon;
  bool t_relCh3_mon;
  float t_trig_vref;
  float t_trig1_thr;
  float t_trig1_mon;
  float t_trig0_thr;
  float t_trig0_mon;
  float t_trig_mon;
  float t_light_level;
  std::vector<unsigned int> *t_vec_errors = new std::vector<unsigned int>;
 
  TTree *t;
  if (f->GetListOfKeys()->Contains("lappdscmonitor_tree")) {
    Log("MonitorLAPPDSC: WriteToFile: Tree already exists",v_message,verbosity);
    t = (TTree*) f->Get("lappdscmonitor_tree");
    t->SetBranchAddress("t_current",&t_time);
    t->SetBranchAddress("humidity",&t_hum);
    t->SetBranchAddress("temp",&t_temp);
    t->SetBranchAddress("hv_mon",&t_hvmon);
    t->SetBranchAddress("hv_state_set",&t_hvstateset);
    t->SetBranchAddress("hv_volt",&t_hvvolt);
    t->SetBranchAddress("lv_mon",&t_lvmon);
    t->SetBranchAddress("lv_state_set",&t_lvstateset);
    t->SetBranchAddress("v33",&t_v33);
    t->SetBranchAddress("v25",&t_v25);
    t->SetBranchAddress("v12",&t_v12);
    t->SetBranchAddress("temp_low",&t_temp_low);
    t->SetBranchAddress("temp_high",&t_temp_high);
    t->SetBranchAddress("hum_low",&t_hum_low);
    t->SetBranchAddress("hum_high",&t_hum_high);
    t->SetBranchAddress("flag_temp",&t_flag_temp);
    t->SetBranchAddress("flag_hum",&t_flag_humidity);
    t->SetBranchAddress("relCh1",&t_relCh1);
    t->SetBranchAddress("relCh2",&t_relCh2);
    t->SetBranchAddress("relCh3",&t_relCh3);
    t->SetBranchAddress("relCh1_mon",&t_relCh1_mon);
    t->SetBranchAddress("relCh2_mon",&t_relCh2_mon);
    t->SetBranchAddress("relCh3_mon",&t_relCh3_mon);
    t->SetBranchAddress("trig_vref",&t_trig_vref);
    t->SetBranchAddress("trig1_thr",&t_trig1_thr);
    t->SetBranchAddress("trig1_mon",&t_trig1_mon);
    t->SetBranchAddress("trig0_thr",&t_trig0_thr);
    t->SetBranchAddress("trig0_mon",&t_trig0_mon);
    t->SetBranchAddress("light_level",&t_light_level);
    t->SetBranchAddress("vec_errors",&t_vec_errors);
  } else {
    t = new TTree("lappdscmonitor_tree","LAPPD SC Monitoring tree");
    Log("MonitorLAPPDSC: WriteToFile: Tree is created from scratch",v_message,verbosity);
    t->Branch("t_current",&t_time);
    t->Branch("humidity",&t_hum);
    t->Branch("temp",&t_temp);
    t->Branch("hv_mon",&t_hvmon);
    t->Branch("hv_state_set",&t_hvstateset);
    t->Branch("hv_volt",&t_hvvolt);
    t->Branch("lv_mon",&t_lvmon);
    t->Branch("lv_state_set",&t_lvstateset);
    t->Branch("v33",&t_v33);
    t->Branch("v25",&t_v25);
    t->Branch("v12",&t_v12);
    t->Branch("temp_low",&t_temp_low);
    t->Branch("temp_high",&t_temp_high);
    t->Branch("hum_low",&t_hum_low);
    t->Branch("hum_high",&t_hum_high);
    t->Branch("flag_temp",&t_flag_temp);
    t->Branch("flag_hum",&t_flag_humidity);
    t->Branch("relCh1",&t_relCh1);    
    t->Branch("relCh2",&t_relCh2);    
    t->Branch("relCh3",&t_relCh3);    
    t->Branch("relCh1_mon",&t_relCh1_mon);    
    t->Branch("relCh2_mon",&t_relCh2_mon);    
    t->Branch("relCh3_mon",&t_relCh3_mon);    
    t->Branch("trig_vref",&t_trig_vref);
    t->Branch("trig1_thr",&t_trig1_thr);
    t->Branch("trig1_mon",&t_trig1_mon);
    t->Branch("trig0_thr",&t_trig0_thr);
    t->Branch("trig0_mon",&t_trig0_mon);
    t->Branch("light_level",&t_light_level);
    t->Branch("vec_errors",&t_vec_errors);
  }

  int n_entries = t->GetEntries();
  bool omit_entries = false;
  for (int i_entry = 0; i_entry < n_entries; i_entry++){
    t->GetEntry(i_entry);
    if (t_time == t_current) {
      Log("WARNING (MonitorLAPPDSC): WriteToFile: Wanted to write data from file that is already written to DB. Omit entries",v_warning,verbosity);
      omit_entries = true;
    }
  }

  //if data is already written to DB/File, do not write it again
  if (omit_entries) {
    //don't write file again, but still delete TFile and TTree object!!!
    f->Close();
    delete t_vec_errors;
    delete f;

    gROOT->cd();

    return;
  }

  t_vec_errors->clear();

  t_time = t_current; //XXX TODO: set t_current somewhere in the code

   boost::posix_time::ptime starttime = *Epoch + boost::posix_time::time_duration(int(t_time/MSEC_to_SEC/SEC_to_MIN/MIN_to_HOUR),int(t_time/MSEC_to_SEC/SEC_to_MIN)%60,int(t_time/MSEC_to_SEC/1000.)%60,t_time%1000);
  struct tm starttime_tm = boost::posix_time::to_tm(starttime);
  Log("MonitorTankTime: WriteToFile: Writing data to file: "+std::to_string(starttime_tm.tm_year+1900)+"/"+std::to_string(starttime_tm.tm_mon+1)+"/"+std::to_string(starttime_tm.tm_mday)+"-"+std::to_string(starttime_tm.tm_hour)+":"+std::to_string(starttime_tm.tm_min)+":"+std::to_string(starttime_tm.tm_sec),v_message,verbosity);

  t_hum = lappd_sc.humidity_mon;
  t_temp = lappd_sc.temperature_mon;
  t_hvmon = lappd_sc.HV_mon;
  t_hvstateset = lappd_sc.HV_state_set;
  t_hvvolt = lappd_sc.HV_volts;
  t_lvmon = lappd_sc.LV_mon;
  t_lvstateset = lappd_sc.LV_state_set;
  t_v33 = lappd_sc.v33;
  t_v25 = lappd_sc.v25;
  t_v12 = lappd_sc.v12;
  t_temp_low = lappd_sc.LIMIT_temperature_low;
  t_temp_high = lappd_sc.LIMIT_temperature_high;
  t_hum_low = lappd_sc.LIMIT_humidity_low;
  t_hum_high = lappd_sc.LIMIT_humidity_high;
  t_flag_temp = lappd_sc.FLAG_temperature;
  t_flag_humidity = lappd_sc.FLAG_humidity;
  t_relCh1 = lappd_sc.relayCh1;
  t_relCh2 = lappd_sc.relayCh2;
  t_relCh3 = lappd_sc.relayCh3;
  t_relCh1_mon = lappd_sc.relayCh1_mon;
  t_relCh2_mon = lappd_sc.relayCh2_mon;
  t_relCh3_mon = lappd_sc.relayCh3_mon;
  t_trig_vref = lappd_sc.TrigVref;
  t_trig1_thr = lappd_sc.Trig1_threshold;
  t_trig1_mon = lappd_sc.Trig1_mon;
  t_trig0_thr = lappd_sc.Trig0_threshold;
  t_trig_mon = lappd_sc.Trig0_mon;
  t_light_level = lappd_sc.light;
  for (int i_error = 0; i_error < (int) lappd_sc.errorcodes.size(); i_error++){
    t_vec_errors->push_back(lappd_sc.errorcodes.at(i_error));
  } 

  t->Fill();
  t->Write("",TObject::kOverwrite);     //prevent ROOT from making endless keys for the same tree when updating the tree
  f->Close();

  delete t_vec_errors;
  delete f;

  gROOT->cd();

}

void MonitorLAPPDSC::ReadFromFile(ULong64_t timestamp, double time_frame){

  Log("MonitorLAPPDSC: ReadFromFile",v_message,verbosity);

  //-------------------------------------------------------
  //------------------ReadFromFile ------------------------
  //-------------------------------------------------------

  times_plot.clear();
  temp_plot.clear();
  humidity_plot.clear();
  hum_low_plot.clear();
  hv_volt_plot.clear();
  hv_mon_plot.clear();
  hv_stateset_plot.clear();
  lv_mon_plot.clear();
  lv_stateset_plot.clear();
  lv_v33_plot.clear();
  lv_v25_plot.clear();
  lv_v12_plot.clear();
  hum_high_plot.clear();
  temp_low_plot.clear();
  temp_high_plot.clear();
  flag_temp_plot.clear();
  flag_hum_plot.clear();
  relCh1_plot.clear();
  relCh2_plot.clear();
  relCh3_plot.clear();
  relCh1mon_plot.clear();
  relCh2mon_plot.clear();
  relCh3mon_plot.clear();
  trig1_plot.clear();
  trig1thr_plot.clear();
  trig0_plot.clear();
  trig0thr_plot.clear();
  trig_vref_plot.clear();
  light_plot.clear();
  num_errors_plot.clear();
  labels_timeaxis.clear();

  //take the end time and calculate the start time with the given time_frame
  ULong64_t timestamp_start = timestamp - time_frame*MIN_to_HOUR*SEC_to_MIN*MSEC_to_SEC;
    boost::posix_time::ptime starttime = *Epoch + boost::posix_time::time_duration(int(timestamp_start/MSEC_to_SEC/SEC_to_MIN/MIN_to_HOUR),int(timestamp_start/MSEC_to_SEC/SEC_to_MIN)%60,int(timestamp_start/MSEC_to_SEC/1000.)%60,timestamp_start%1000);
  struct tm starttime_tm = boost::posix_time::to_tm(starttime);
  boost::posix_time::ptime endtime = *Epoch + boost::posix_time::time_duration(int(timestamp/MSEC_to_SEC/SEC_to_MIN/MIN_to_HOUR),int(timestamp/MSEC_to_SEC/SEC_to_MIN)%60,int(timestamp/MSEC_to_SEC/1000.)%60,timestamp%1000);
  struct tm endtime_tm = boost::posix_time::to_tm(endtime);

  Log("MonitorLAPPDSC: ReadFromFile: Reading in data for time frame "+std::to_string(starttime_tm.tm_year+1900)+"/"+std::to_string(starttime_tm.tm_mon+1)+"/"+std::to_string(starttime_tm.tm_mday)+"-"+std::to_string(starttime_tm.tm_hour)+":"+std::to_string(starttime_tm.tm_min)+":"+std::to_string(starttime_tm.tm_sec)
    +" ... "+std::to_string(endtime_tm.tm_year+1900)+"/"+std::to_string(endtime_tm.tm_mon+1)+"/"+std::to_string(endtime_tm.tm_mday)+"-"+std::to_string(endtime_tm.tm_hour)+":"+std::to_string(endtime_tm.tm_min)+":"+std::to_string(endtime_tm.tm_sec),v_message,verbosity);

  std::stringstream ss_startdate, ss_enddate;
  ss_startdate << starttime_tm.tm_year+1900 << "-" << starttime_tm.tm_mon+1 <<"-"<< starttime_tm.tm_mday;
  ss_enddate << endtime_tm.tm_year+1900 << "-" << endtime_tm.tm_mon+1 <<"-"<< endtime_tm.tm_mday;

  int number_of_days;
  if (endtime_tm.tm_mon == starttime_tm.tm_mon) number_of_days = endtime_tm.tm_mday - starttime_tm.tm_mday;
  else {
    boost::gregorian::date endtime_greg(boost::gregorian::from_simple_string(ss_enddate.str()));
    boost::gregorian::date starttime_greg(boost::gregorian::from_simple_string(ss_startdate.str()));
    boost::gregorian::days days_dataframe = endtime_greg - starttime_greg;
    number_of_days = days_dataframe.days();
  }

 for (int i_day = 0; i_day <= number_of_days; i_day++){

    ULong64_t timestamp_i = timestamp_start+i_day*HOUR_to_DAY*MIN_to_HOUR*SEC_to_MIN*MSEC_to_SEC;
    std::string string_date_i = convertTimeStamp_to_Date(timestamp_i);
    std::stringstream root_filename_i;
    root_filename_i << path_monitoring << "LAPPDSC_" << string_date_i <<".root";
    bool tree_exists = true;

    if (does_file_exist(root_filename_i.str())) {
      TFile *f = new TFile(root_filename_i.str().c_str(),"READ");
      TTree *t;
      if (f->GetListOfKeys()->Contains("lappdscmonitor_tree")) t = (TTree*) f->Get("lappdscmonitor_tree");
      else { 
        Log("WARNING (MonitorLAPPDSC): File "+root_filename_i.str()+" does not contain lappdscmonitor_tree. Omit file.",v_warning,verbosity);
        tree_exists = false;
      }

      if (tree_exists){

        Log("MonitorLAPPDSC: Tree exists, start reading in data",v_message,verbosity);

        ULong64_t t_time;
        std::vector<unsigned int> *t_vec_errors = new std::vector<unsigned int>;
        float t_hum;
        float t_temp;
        int t_hvmon;
        bool t_hvstateset;
        float t_hvvolt;
        int t_lvmon;
        bool t_lvstateset;
        float t_v33;
        float t_v25;
        float t_v12;
        float t_temp_low;
        float t_temp_high;
        float t_hum_low;
        float t_hum_high;
        int t_flag_temp;
        int t_flag_humidity;
        bool t_relCh1;
        bool t_relCh2;
        bool t_relCh3;
        bool t_relCh1_mon;
        bool t_relCh2_mon;
        bool t_relCh3_mon;
        float t_trig_vref;
        float t_trig1_thr;
        float t_trig1_mon;
        float t_trig0_thr;
        float t_trig0_mon;
        float t_trig_mon;
        float t_light_level;

        int nentries_tree;

        t->SetBranchAddress("t_current",&t_time);
        t->SetBranchAddress("humidity",&t_hum);
        t->SetBranchAddress("temp",&t_temp);
        t->SetBranchAddress("hv_mon",&t_hvmon);
        t->SetBranchAddress("hv_state_set",&t_hvstateset);
        t->SetBranchAddress("hv_volt",&t_hvvolt);
        t->SetBranchAddress("lv_mon",&t_lvmon);
        t->SetBranchAddress("lv_state_set",&t_lvstateset);
        t->SetBranchAddress("v33",&t_v33);
        t->SetBranchAddress("v25",&t_v25);
        t->SetBranchAddress("v12",&t_v12);
        t->SetBranchAddress("temp_low",&t_temp_low);
        t->SetBranchAddress("temp_high",&t_temp_high);
        t->SetBranchAddress("hum_low",&t_hum_low);
        t->SetBranchAddress("hum_high",&t_hum_high);
        t->SetBranchAddress("flag_temp",&t_flag_temp);
        t->SetBranchAddress("flag_hum",&t_flag_humidity);
        t->SetBranchAddress("relCh1",&t_relCh1);
        t->SetBranchAddress("relCh2",&t_relCh2);
        t->SetBranchAddress("relCh3",&t_relCh3);
        t->SetBranchAddress("relCh1_mon",&t_relCh1_mon);
        t->SetBranchAddress("relCh2_mon",&t_relCh2_mon);
        t->SetBranchAddress("relCh3_mon",&t_relCh3_mon);
        t->SetBranchAddress("trig_vref",&t_trig_vref);
        t->SetBranchAddress("trig1_thr",&t_trig1_thr);
        t->SetBranchAddress("trig1_mon",&t_trig1_mon);
        t->SetBranchAddress("trig0_thr",&t_trig0_thr);
        t->SetBranchAddress("trig0_mon",&t_trig0_mon);
        t->SetBranchAddress("light_level",&t_light_level);
        t->SetBranchAddress("vec_errors",&t_vec_errors);

        nentries_tree = t->GetEntries();
	      
	//Sort timestamps for the case that they are not in order
	
	std::vector<ULong64_t> vector_timestamps;
        std::map<ULong64_t,int> map_timestamp_entry;
	for (int i_entry = 0; i_entry < nentries_tree; i_entry++){
	  t->GetEntry(i_entry);
	  if (t_time >= timestamp_start && t_time <= timestamp){
	    vector_timestamps.push_back(t_time);
	    map_timestamp_entry.emplace(t_time,i_entry);	    
	  }
	}

	std::sort(vector_timestamps.begin(), vector_timestamps.end());
	std::vector<int> vector_sorted_entry;

	for (int i_entry = 0; i_entry < (int) vector_timestamps.size(); i_entry++){
	  vector_sorted_entry.push_back(map_timestamp_entry.at(vector_timestamps.at(i_entry)));
	}

        for (int i_entry = 0; i_entry < (int) vector_sorted_entry.size(); i_entry++){
		
	  int next_entry = vector_sorted_entry.at(i_entry);

          t->GetEntry(next_entry);
          if (t_time >= timestamp_start && t_time <= timestamp){
            times_plot.push_back(t_time);
            temp_plot.push_back(t_temp);
            humidity_plot.push_back(t_hum);
            hv_mon_plot.push_back(t_hvmon);
            hv_volt_plot.push_back(t_hvvolt);
            hv_stateset_plot.push_back(t_hvstateset);
            lv_mon_plot.push_back(t_lvmon);
            lv_stateset_plot.push_back(t_lvstateset);
            lv_v33_plot.push_back(t_v33);
            lv_v25_plot.push_back(t_v25);
            lv_v12_plot.push_back(t_v12);
            hum_low_plot.push_back(t_hum_low);
            hum_high_plot.push_back(t_hum_high);
            temp_low_plot.push_back(t_temp_low);
            temp_high_plot.push_back(t_temp_high);
            flag_temp_plot.push_back(t_flag_temp);
            flag_hum_plot.push_back(t_flag_humidity);
            relCh1_plot.push_back(t_relCh1);
            relCh2_plot.push_back(t_relCh2);
            relCh3_plot.push_back(t_relCh3);
            relCh1mon_plot.push_back(t_relCh1_mon);
            relCh2mon_plot.push_back(t_relCh2_mon);
            relCh3mon_plot.push_back(t_relCh3_mon);
            trig1_plot.push_back(t_trig1_mon);
            trig0_plot.push_back(t_trig0_mon);
            trig1thr_plot.push_back(t_trig1_thr);
            trig0thr_plot.push_back(t_trig0_thr);
            trig_vref_plot.push_back(t_trig_vref);
            light_plot.push_back(t_light_level);
            num_errors_plot.push_back(t_vec_errors->size());

            boost::posix_time::ptime boost_tend = *Epoch+boost::posix_time::time_duration(int(t_time/MSEC_to_SEC/SEC_to_MIN/MIN_to_HOUR),int(t_time/MSEC_to_SEC/SEC_to_MIN)%60,int(t_time/MSEC_to_SEC/1000.)%60,t_time%1000);
            struct tm label_timestamp = boost::posix_time::to_tm(boost_tend);
            
            TDatime datime_timestamp(1900+label_timestamp.tm_year,label_timestamp.tm_mon+1,label_timestamp.tm_mday,label_timestamp.tm_hour,label_timestamp.tm_min,label_timestamp.tm_sec);
            labels_timeaxis.push_back(datime_timestamp);
          }

        }

        delete t_vec_errors;

      }

      f->Close();
      delete f;
      gROOT->cd();

    } else {
      Log("MonitorLAPPDSC: ReadFromFile: File "+root_filename_i.str()+" does not exist. Omit file.",v_warning,verbosity);
    }

  }

  //Set the readfromfile time variables to make sure data is not read twice for the same time window
  readfromfile_tend = timestamp;
  readfromfile_timeframe = time_frame;

}

void MonitorLAPPDSC::DrawLastFilePlots(){

  Log("MonitorLAPPDSC: DrawLastFilePlots",v_message,verbosity);

  //-------------------------------------------------------
  //------------------DrawLastFilePlots -------------------
  //-------------------------------------------------------

  //draw temp / humidity status
  DrawStatus_TempHumidity();

  //draw HV / LV status
  DrawStatus_LVHV();

  //draw trigger status
  DrawStatus_Trigger();

  //draw relay status
  DrawStatus_Relay();

  //draw error status
  DrawStatus_Errors();

  //draw general status overview
  DrawStatus_Overview();

}

void MonitorLAPPDSC::UpdateMonitorPlots(std::vector<double> timeFrames, std::vector<ULong64_t> endTimes, std::vector<std::string> fileLabels, std::vector<std::vector<std::string>> plotTypes){

  Log("MonitorLAPPDSC: UpdateMonitorPlots",v_message,verbosity);

  //-------------------------------------------------------
  //------------------UpdateMonitorPlots ------------------
  //-------------------------------------------------------
 
  //Draw the monitoring plots according to the specifications in the configfiles

  for (unsigned int i_time = 0; i_time < timeFrames.size(); i_time++){

    ULong64_t zero = 0;
    if (endTimes.at(i_time) == zero) endTimes.at(i_time) = t_current;        //set 0 for t_file_end since we did not know what that was at the beginning of initialise
    
    for (unsigned int i_plot = 0; i_plot < plotTypes.at(i_time).size(); i_plot++){
      if (plotTypes.at(i_time).at(i_plot) == "TimeEvolution") DrawTimeEvolutionLAPPDSC(endTimes.at(i_time),timeFrames.at(i_time),fileLabels.at(i_time));
      else {
        if (verbosity > 0) std::cout <<"ERROR (MonitorLAPPDSC): UpdateMonitorPlots: Specified plot type -"<<plotTypes.at(i_time).at(i_plot)<<"- does not exist! Omit entry."<<std::endl;
      }
    }
  }


}

void MonitorLAPPDSC::DrawStatus_TempHumidity(){

  Log("MonitorLAPPDSC: DrawStatus_TempHumidity",v_message,verbosity);

  //-------------------------------------------------------
  //-------------DrawStatus_TempHumidity ------------------
  //-------------------------------------------------------
 
   boost::posix_time::ptime currenttime = *Epoch + boost::posix_time::time_duration(int(t_current/MSEC_to_SEC/SEC_to_MIN/MIN_to_HOUR),int(t_current/MSEC_to_SEC/SEC_to_MIN)%60,int(t_current/MSEC_to_SEC)%60,t_current%1000);
  struct tm currenttime_tm = boost::posix_time::to_tm(currenttime);
  std::stringstream current_time;
  current_time << currenttime_tm.tm_year+1900<<"/"<<currenttime_tm.tm_mon+1<<"/"<<currenttime_tm.tm_mday<<"-"<<currenttime_tm.tm_hour<<":"<<currenttime_tm.tm_min<<":"<<currenttime_tm.tm_sec;
  
  text_temphum_title->SetText(0.06,0.9,"LAPPD Temperature / Humidity");
  std::stringstream ss_text_temp;
  ss_text_temp << "Temperature: "<< lappd_sc.temperature_mon << " ("<<current_time.str()<<")";
  text_temp->SetText(0.06,0.8,ss_text_temp.str().c_str());
  text_temp->SetTextColor(1);
  if (lappd_sc.temperature_mon < lappd_sc.LIMIT_temperature_low || lappd_sc.temperature_mon > lappd_sc.LIMIT_temperature_high) {
    Log("MonitorLAPPDSC: SEVERE ERROR: Monitored temperature >>>"+std::to_string(lappd_sc.temperature_mon)+"<<< is outside of expected range ["+std::to_string(lappd_sc.LIMIT_temperature_low)+" , "+std::to_string(lappd_sc.LIMIT_temperature_high)+"]!!!",v_error,verbosity);
    text_temp->SetTextColor(kRed);
  }

  std::stringstream ss_text_hum;
  ss_text_hum << "Humidity: "<< lappd_sc.humidity_mon << " ("<<current_time.str()<<")";
  text_hum->SetText(0.06,0.7,ss_text_hum.str().c_str());
  text_hum->SetTextColor(1);
  if (lappd_sc.humidity_mon < lappd_sc.LIMIT_humidity_low || lappd_sc.humidity_mon > lappd_sc.LIMIT_humidity_high) {
    Log("MonitorLAPPDSC: SEVERE ERROR: Monitored humidity >>>"+std::to_string(lappd_sc.humidity_mon)+"<<< is outside of expected range ["+std::to_string(lappd_sc.LIMIT_humidity_low)+" , "+std::to_string(lappd_sc.LIMIT_humidity_high)+"]!!!",v_error,verbosity);
    text_hum->SetTextColor(kRed);
  }
  
  std::stringstream ss_text_light;
  ss_text_light << "Light level: "<< lappd_sc.light << " ("<<current_time.str()<<")";
  text_light->SetText(0.06,0.6,ss_text_light.str().c_str());
  text_light->SetTextColor(1);

  std::stringstream ss_text_flag_temp;
  std::string emergency_temp = (lappd_sc.FLAG_temperature)? "True" : "False";
  ss_text_flag_temp << "Emergency Temperature: "<< emergency_temp << " ("<<current_time.str()<<")";
  text_flag_temp->SetText(0.06,0.5,ss_text_flag_temp.str().c_str());
  text_flag_temp->SetTextColor(1);
  if (emergency_temp == "True") {
    Log("MonitorLAPPDSC: SEVERE ERROR: Temperature emergency flag is set!!!",v_error,verbosity);
    text_flag_temp->SetTextColor(kRed);
  }

  std::stringstream ss_text_flag_hum;
  std::string emergency_hum = (lappd_sc.FLAG_humidity)? "True" : "False";
  ss_text_flag_hum << "Emergency Humidity: "<< emergency_hum << " ("<<current_time.str()<<")";
  text_flag_hum->SetText(0.06,0.4,ss_text_flag_hum.str().c_str());
  text_flag_hum->SetTextColor(1);
  if (emergency_hum == "True"){
    Log("MonitorLAPPDSC: SEVERE ERROR: Humidity emergency flag is set!!!",v_error,verbosity);
    text_flag_hum->SetTextColor(kRed);
  }

  text_temphum_title->SetTextSize(0.05);
  text_temp->SetTextSize(0.05);
  text_hum->SetTextSize(0.05);
  text_light->SetTextSize(0.05);
  text_flag_temp->SetTextSize(0.05);
  text_flag_hum->SetTextSize(0.05);

  text_temphum_title->SetNDC(1);
  text_temp->SetNDC(1);
  text_hum->SetNDC(1);
  text_light->SetNDC(1);
  text_flag_temp->SetNDC(1);
  text_flag_hum->SetNDC(1);

  canvas_status_temphum->cd();
  canvas_status_temphum->Clear();
  text_temphum_title->Draw();
  text_temp->Draw();
  text_hum->Draw();
  text_light->Draw();
  text_flag_temp->Draw();
  text_flag_hum->Draw();

  std::stringstream ss_path_tempinfo;
  ss_path_tempinfo << outpath << "LAPPDSC_TempHumInfo_current."<<img_extension;
  canvas_status_temphum->SaveAs(ss_path_tempinfo.str().c_str());
  canvas_status_temphum->Clear();


   
}

void MonitorLAPPDSC::DrawStatus_LVHV(){

  Log("MonitorLAPPDSC: DrawStatus_LVHV",v_message,verbosity);

  //-------------------------------------------------------
  //-------------DrawStatus_LVHV --------------------------
  //-------------------------------------------------------

  //TODO: Add status board for LV and HV values
  // 
  // ----Rough sketch-----
  // Title: LAPPD HV/LV
  // HV State Set: ON/OFF
  // HV Mon: 
  // HV Volt: 
  // LV Stae Set: ON/OFF
  // LV Mon: 
  // V33: 
  // V25:
  // V12:
  // ----End of rough sketch---
  //
  // Number of rows in canvas: 9 (maximum of 10 rows, so it fits)

  //Variable names:
/*
  t_hum = lappd_sc.humidity_mon;
  std::cout <<"t_hum: "<<t_hum<<std::endl;
  t_temp = lappd_sc.temperature_mon;
  t_hvmon = lappd_sc.HV_mon;
  t_hvstateset = lappd_sc.HV_state_set;
  t_hvvolt = lappd_sc.HV_volts;
  t_lvmon = lappd_sc.LV_mon;
  t_lvstateset = lappd_sc.LV_state_set;
  t_v33 = lappd_sc.v33;
  t_v25 = lappd_sc.v25;
  t_v12 = lappd_sc.v12;
  t_temp_low = lappd_sc.LIMIT_temperature_low;
  t_temp_high = lappd_sc.LIMIT_temperature_high;
  t_hum_low = lappd_sc.LIMIT_humidity_low;
  t_hum_high = lappd_sc.LIMIT_humidity_high;
  t_flag_temp = lappd_sc.FLAG_temperature;
  t_flag_humidity = lappd_sc.FLAG_humidity;
  t_relCh1 = lappd_sc.relayCh1;
  t_relCh2 = lappd_sc.relayCh2;
  t_relCh3 = lappd_sc.relayCh3;
  t_relCh1_mon = lappd_sc.relayCh1_mon;
  t_relCh2_mon = lappd_sc.relayCh2_mon;
  t_relCh3_mon = lappd_sc.relayCh3_mon;
  t_trig_vref = lappd_sc.TrigVref;
  t_trig1_thr = lappd_sc.Trig1_threshold;
  t_trig1_mon = lappd_sc.Trig1_mon;
  t_trig0_thr = lappd_sc.Trig0_threshold;
  t_trig_mon = lappd_sc.Trig0_mon;
  t_light_level = lappd_sc.light;
*/
}

void MonitorLAPPDSC::DrawStatus_Trigger(){

  Log("MonitorLAPPDSC: DrawStatus_Trigger",v_message,verbosity);

  //-------------------------------------------------------
  //-------------DrawStatus_Trigger -----------------------
  //-------------------------------------------------------
  
  //TODO: Add status board for trigger values
  // 
  // ----Rough sketch-----
  // Title: LAPPD Triggers
  // Trigger Vref: 
  // Trig0 Thr: 
  // Trig0 Mon: 
  // Trig1 Thr:
  // Trig1 Mon:
  // ----End of rough sketch---
  //
  // Number of rows in canvas: 6 (maximum of 10 rows, so it fits)
  
  //Important variables
  /*
    t_trig_vref = lappd_sc.TrigVref;
    t_trig1_thr = lappd_sc.Trig1_threshold;
    t_trig1_mon = lappd_sc.Trig1_mon;
    t_trig0_thr = lappd_sc.Trig0_threshold;
    t_trig_mon = lappd_sc.Trig0_mon;
  */

}

void MonitorLAPPDSC::DrawStatus_Relay(){

  Log("MonitorLAPPDSC: DrawStatus_Relay",v_message,verbosity);

  //-------------------------------------------------------
  //-------------DrawStatus_Relay ------------------------
  //-------------------------------------------------------
  
  //TODO: Add status board for relay values
  // 
  // ----Rough sketch-----
  // Title: LAPPD Relays
  // Relay1 Set: ON/OFF 
  // Relay2 Set: ON/OFF
  // Relay3 Set: ON/OFF
  // Relay1 Mon: ON/OFF
  // Relay2 Mon: ON/OFF
  // Relay3 Mon: ON/OFF
  // ----End of rough sketch---
  //
  // Number of rows in canvas: 7 (maximum of 10 rows, so it fits)
  
  //Important variables
  /*
   t_relCh1 = lappd_sc.relayCh1;
   t_relCh2 = lappd_sc.relayCh2;
   t_relCh3 = lappd_sc.relayCh3;
   t_relCh1_mon = lappd_sc.relayCh1_mon;
   t_relCh2_mon = lappd_sc.relayCh2_mon;
   t_relCh3_mon = lappd_sc.relayCh3_mon;
   */
}

void MonitorLAPPDSC::DrawStatus_Errors(){

  Log("MonitorLAPPDSC: DrawStatus_Errors",v_message,verbosity);

  //--------------------------------------------------------
  //-------------DrawStatus_Errors -------------------------
  //--------------------------------------------------------
  
  //TODO: Add status board for error values
  // 
  // ----Rough sketch-----
  // Title: LAPPD Errors
  // Either each line has an error code with timestamp (only 9 most recent ones)
  // Or there will be a histogram of error codes and their frequencies
  // potentially both
  // ----End of rough sketch---
  //
  // Number of rows in canvas: X (maximum of 10 rows)
}

void MonitorLAPPDSC::DrawStatus_Overview(){

  Log("MonitorLAPPDSC: DrawStatus_Overview",v_message,verbosity);
  
  //--------------------------------------------------------
  //-------------DrawStatus_Overview -----------------------
  // -------------------------------------------------------
  
  // Potential TODO (low priority): One status board with all status variables of the Slow Control
  // Could use green circles to indicate good behavior, red circles for bad behavior
  // Can be done in case everything else is finished (optional)

}

void MonitorLAPPDSC::DrawTimeEvolutionLAPPDSC(ULong64_t timestamp_end, double time_frame, std::string file_ending){

  Log("MonitorLAPPDSC: DrawTimeEvolutionLAPPDSC",v_message,verbosity);

  //--------------------------------------------------------
  //-------------DrawTimeEvolutionLAPPDSC ------------------
  // -------------------------------------------------------

  boost::posix_time::ptime endtime = *Epoch + boost::posix_time::time_duration(int(timestamp_end/MSEC_to_SEC/SEC_to_MIN/MIN_to_HOUR),int(timestamp_end/MSEC_to_SEC/SEC_to_MIN)%60,int(timestamp_end/MSEC_to_SEC/1000.)%60,timestamp_end%1000);
  struct tm endtime_tm = boost::posix_time::to_tm(endtime);
  std::stringstream end_time;
  end_time << endtime_tm.tm_year+1900<<"/"<<endtime_tm.tm_mon+1<<"/"<<endtime_tm.tm_mday<<"-"<<endtime_tm.tm_hour<<":"<<endtime_tm.tm_min<<":"<<endtime_tm.tm_sec;

  if (timestamp_end != readfromfile_tend || time_frame != readfromfile_timeframe) ReadFromFile(timestamp_end, time_frame);

  //looping over all files that are in the time interval, each file will be one data point
  
  std::stringstream ss_timeframe;
  ss_timeframe << round(time_frame*100.)/100.;

  graph_temp->Set(0);
  graph_humidity->Set(0);
  graph_light->Set(0);
  graph_hv_volt->Set(0);
  graph_lv_volt1->Set(0);
  graph_lv_volt2->Set(0);
  graph_lv_volt3->Set(0);

  for (unsigned int i_file = 0; i_file < temp_plot.size(); i_file++){

    Log("MonitorLAPPDSC: Stored data (file #"+std::to_string(i_file+1)+"): ",v_message,verbosity);
    graph_temp->SetPoint(i_file,labels_timeaxis[i_file].Convert(),temp_plot.at(i_file));
    graph_humidity->SetPoint(i_file,labels_timeaxis[i_file].Convert(),humidity_plot.at(i_file));
    graph_light->SetPoint(i_file,labels_timeaxis[i_file].Convert(),light_plot.at(i_file));
    graph_hv_volt->SetPoint(i_file,labels_timeaxis[i_file].Convert(),hv_volt_plot.at(i_file));
    graph_lv_volt1->SetPoint(i_file,labels_timeaxis[i_file].Convert(),lv_v33_plot.at(i_file));
    graph_lv_volt2->SetPoint(i_file,labels_timeaxis[i_file].Convert(),lv_v25_plot.at(i_file));
    graph_lv_volt3->SetPoint(i_file,labels_timeaxis[i_file].Convert(),lv_v12_plot.at(i_file));

  }

  // Drawing time evolution plots

  double max_canvas = 0;
  double min_canvas = 9999999.;
  double max_canvas_sigma = 0;
  double min_canvas_sigma = 99999999.;
  double max_canvas_rate = 0;
  double min_canvas_rate = 999999999.;
  
  std::stringstream ss_temp;
  ss_temp << "Temperature time evolution (last "<<ss_timeframe.str()<<"h) "<<end_time.str();
  canvas_temp->cd();
  canvas_temp->Clear();
  graph_temp->SetTitle(ss_temp.str().c_str());
  graph_temp->GetYaxis()->SetTitle("Temperature");
  graph_temp->GetXaxis()->SetTimeDisplay(1);
  graph_temp->GetXaxis()->SetLabelSize(0.03);
  graph_temp->GetXaxis()->SetLabelOffset(0.03);
  graph_temp->GetXaxis()->SetTimeFormat("#splitline{%m/%d}{%H:%M}");
  graph_temp->GetXaxis()->SetTimeOffset(0.);
  graph_temp->Draw("apl");
  double max_temp = TMath::MaxElement(temp_plot.size(),graph_temp->GetY());
  graph_temp->GetYaxis()->SetRangeUser(0.001,1.1*max_temp);
  std::stringstream ss_temp_path;
  ss_temp_path << outpath << "LAPPDSC_TimeEvolution_Temp_"<<file_ending<<"."<<img_extension;
  canvas_temp->SaveAs(ss_temp_path.str().c_str());

  std::stringstream ss_hum;
  ss_hum << "Humidity time evolution (last "<<ss_timeframe.str()<<"h) "<<end_time.str();
  canvas_humidity->cd();
  canvas_humidity->Clear();
  graph_humidity->SetTitle(ss_hum.str().c_str());
  graph_humidity->GetYaxis()->SetTitle("Humidity");
  graph_humidity->GetXaxis()->SetTimeDisplay(1);
  graph_humidity->GetXaxis()->SetLabelSize(0.03);
  graph_humidity->GetXaxis()->SetLabelOffset(0.03);
  graph_humidity->GetXaxis()->SetTimeFormat("#splitline{%m/%d}{%H:%M}");
  graph_humidity->GetXaxis()->SetTimeOffset(0.);
  graph_humidity->Draw("apl");
  double max_humidity = TMath::MaxElement(humidity_plot.size(),graph_humidity->GetY());
  graph_humidity->GetYaxis()->SetRangeUser(0.001,1.1*max_humidity);
  std::stringstream ss_hum_path;
  ss_hum_path << outpath << "LAPPDSC_TimeEvolution_Humidity_"<<file_ending<<"."<<img_extension;
  canvas_humidity->SaveAs(ss_hum_path.str().c_str());

  std::stringstream ss_light;
  ss_light << "Light level time evolution (last "<<ss_timeframe.str()<<"h) "<<end_time.str();
  canvas_light->cd();
  canvas_light->Clear();
  graph_light->SetTitle(ss_light.str().c_str());
  graph_light->GetYaxis()->SetTitle("Light level");
  graph_light->GetXaxis()->SetTimeDisplay(1);
  graph_light->GetXaxis()->SetLabelSize(0.03);
  graph_light->GetXaxis()->SetLabelOffset(0.03);
  graph_light->GetXaxis()->SetTimeFormat("#splitline{%m/%d}{%H:%M}");
  graph_light->GetXaxis()->SetTimeOffset(0.);
  graph_light->Draw("apl");
  double max_light = TMath::MaxElement(light_plot.size(),graph_light->GetY());
  graph_light->GetYaxis()->SetRangeUser(0.001,1.1*max_light);
  std::stringstream ss_light_path;
  ss_light_path << outpath << "LAPPDSC_TimeEvolution_Light_"<<file_ending<<"."<<img_extension;
  canvas_light->SaveAs(ss_light_path.str().c_str());

  std::stringstream ss_hv;
  ss_hv << "HV time evolution (last "<<ss_timeframe.str()<<"h) "<<end_time.str();
  canvas_hv->cd();
  canvas_hv->Clear();
  graph_hv_volt->SetTitle(ss_hv.str().c_str());
  graph_hv_volt->GetYaxis()->SetTitle("HV [V]");
  graph_hv_volt->GetXaxis()->SetTimeDisplay(1);
  graph_hv_volt->GetXaxis()->SetLabelSize(0.03);
  graph_hv_volt->GetXaxis()->SetLabelOffset(0.03);
  graph_hv_volt->GetXaxis()->SetTimeFormat("#splitline{%m/%d}{%H:%M}");
  graph_hv_volt->GetXaxis()->SetTimeOffset(0.);
  graph_hv_volt->Draw("apl");
  double max_hv_volt = TMath::MaxElement(hv_volt_plot.size(),graph_hv_volt->GetY());
  graph_hv_volt->GetYaxis()->SetRangeUser(0.001,1.1*max_hv_volt);
  std::stringstream ss_hv_path;
  ss_hv_path << outpath << "LAPPDSC_TimeEvolution_HV_"<<file_ending<<"."<<img_extension;
  canvas_hv->SaveAs(ss_hv_path.str().c_str());

  std::stringstream ss_lv;
  ss_lv << "LV time evolution (last "<<ss_timeframe.str()<<"h) "<<end_time.str();
  multi_lv->Add(graph_lv_volt1);
  leg_lv->AddEntry(graph_lv_volt1,"V35","l");
  multi_lv->Add(graph_lv_volt2);
  leg_lv->AddEntry(graph_lv_volt2,"V21","l");
  multi_lv->Add(graph_lv_volt3);
  leg_lv->AddEntry(graph_lv_volt3,"V12","l");
  canvas_lv->cd();
  canvas_lv->Clear();
  multi_lv->Draw("apl");
  multi_lv->SetTitle(ss_lv.str().c_str());
  multi_lv->GetYaxis()->SetTitle("LV [V]");
  multi_lv->GetXaxis()->SetTimeDisplay(1);
  multi_lv->GetXaxis()->SetLabelSize(0.03);
  multi_lv->GetXaxis()->SetLabelOffset(0.03);
  multi_lv->GetXaxis()->SetTimeFormat("#splitline{%m/%d}{%H:%M}");
  multi_lv->GetXaxis()->SetTimeOffset(0.);
  leg_lv->Draw();
  std::stringstream ss_lv_path;
  ss_lv_path << outpath << "LAPPDSC_TimeEvolution_LV_"<<file_ending<<"."<<img_extension;
  canvas_lv->SaveAs(ss_lv_path.str().c_str());

  multi_lv->RecursiveRemove(graph_lv_volt1);
  multi_lv->RecursiveRemove(graph_lv_volt2);
  multi_lv->RecursiveRemove(graph_lv_volt3);

  leg_lv->Clear();
  canvas_lv->Clear();

}
