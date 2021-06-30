#include "MonitorLAPPDData.h"

MonitorLAPPDData::MonitorLAPPDData():Tool(){}


bool MonitorLAPPDData::Initialise(std::string configfile, DataModel &data){

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

  std::string acdc_configuration;

  m_variables.Get("OutputPath",outpath_temp);
  m_variables.Get("StartTime",StartTime);
  m_variables.Get("UpdateFrequency",update_frequency);
  m_variables.Get("PathMonitoring",path_monitoring);
  m_variables.Get("PlotConfiguration",plot_configuration);
  m_variables.Get("ACDCBoardConfiguration",acdc_configuration);
  m_variables.Get("ImageFormat",img_extension);
  m_variables.Get("ForceUpdate",force_update);
  m_variables.Get("DrawMarker",draw_marker);
  m_variables.Get("verbose",verbosity);

  if (verbosity > 1) std::cout <<"Tool MonitorLAPPDData: Initialising...."<<std::endl;
  // Update frequency specifies the frequency at which the File Log Histogram is updated
  // All other monitor plots are updated as soon as a new file is available for readout
  if (update_frequency < 0.1) {
    if (verbosity > 0) std::cout <<"MonitorLAPPDData: Update Frequency of "<<update_frequency<<" mins is too low. Setting default value of 5 min."<<std::endl;
    update_frequency = 5.;
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
  if (verbosity > 2) std::cout <<"MonitorLAPPDData: PathMonitoring: "<<path_monitoring<<std::endl;

  //Set up Epoch
  Epoch = new boost::posix_time::ptime(boost::gregorian::from_string(StartTime));

  //Evaluating output path for monitoring plots
  if (outpath_temp == "fromStore") m_data->CStore.Get("OutPath",outpath);
  else outpath = outpath_temp;
  if (verbosity > 2) std::cout <<"MonitorLAPPDData: Output path for plots is "<<outpath<<std::endl;

  //-------------------------------------------------------
  //----------Initialize histograms/canvases---------------
  //-------------------------------------------------------

  this->InitializeHistsLAPPD();
  std::cout <<"hist_adc_channel: "<<hist_adc_channel<<std::endl;
  //-------------------------------------------------------
  //----------Read in configuration option for plots-------
  //-------------------------------------------------------

  ReadInConfiguration();

  //-------------------------------------------------------
  //----------Read in ACDC board numbers-------------------
  //-------------------------------------------------------
  
  LoadACDCBoardConfig(acdc_configuration);

  //-------------------------------------------------------
  //------Setup time variables for periodic updates--------
  //-------------------------------------------------------
  
  period_update = boost::posix_time::time_duration(0,int(update_frequency),0,0);
  last = boost::posix_time::ptime(boost::posix_time::second_clock::local_time());

  
  // Omit warning messages from ROOT: 1001 - info messages, 2001 - warnings, 3001 - errors
  gROOT->ProcessLine("gErrorIgnoreLevel = 3001;");


  return true;
}


bool MonitorLAPPDData::Execute(){

  if (verbosity > 10) std::cout <<"MonitorLAPPDData: Executing ...."<<std::endl;

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
 
  bool has_lappd;
  m_data->CStore.Get("HasLAPPDData",has_lappd);
 
  std::string State;
  m_data->CStore.Get("State",State);

  if (has_lappd){
    if (State == "Wait" || State == "LAPPDSC"){
      if (verbosity > 2) std::cout <<"MonitorLAPPDData: State is "<<State<<std::endl;
    }
    else if (State == "DataFile"){
      if (verbosity > 1) std::cout<<"MonitorLAPPDData: New PSEC data available."<<std::endl; 


      //TODO
      //TODO: Need to confirm data format of LAPPDData, is it PsecData, or a vec of PsecData, or map<timestamp,PsecData>?
      //TODO

      m_data->Stores["LAPPDData"]->Get("LAPPDData",LAPPDData);

      //Process data
      //TODO: Adapt when data type is understood
      std::cout <<"hist_adc_channel: "<<hist_adc_channel<<std::endl;
      this->ProcessLAPPDData();

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
    else {
      if (verbosity > 1) std::cout <<"MonitorLAPPDData: State not recognized: "<<State<<std::endl;
    }
  }

  // if force_update is specified, the plots will be updated no matter whether there has been a new file or not
  if (force_update) UpdateMonitorPlots(config_timeframes, config_endtime_long, config_label, config_plottypes);

  //-------------------------------------------------------
  //-----------Has enough time passed for update?----------
  //-------------------------------------------------------

  if (duration >= period_update){

    Log("MonitorLAPPDData: "+std::to_string(update_frequency)+" mins passed... Updating file history plot.",v_message,verbosity);

    last=current;
    //TODO: Maybe implement the file history plots
    //DrawFileHistoryLAPPD(current_stamp,24.,"current_24h",1);     //show 24h history of LAPPD files
    //PrintFileTimeStampLAPPD(current_stamp,24.,"current_24h");
    //DrawFileHistoryLAPPD(current_stamp,2.,"current_2h",3);

  }

  //gObjectTable only for debugging memory leaks, otherwise comment out
  //std::cout <<"List of Objects (after execute step): "<<std::endl;
  //gObjectTable->Print();

  return true;
}


bool MonitorLAPPDData::Finalise(){

  if (verbosity > 1) std::cout <<"Tool MonitorLAPPDData: Finalising ...."<<std::endl;

  //timing pointers
  delete Epoch;

  //canvas
  delete canvas_status_data;
  delete canvas_pps_rate;
  delete canvas_frame_rate;
  delete canvas_buffer_size;
  delete canvas_int_charge;
  delete canvas_align_1file;
  delete canvas_align_5file;
  delete canvas_align_10files;
  delete canvas_align_20files;
  delete canvas_align_100files;
  delete canvas_adc_channel;
  delete canvas_buffer_channel;
  delete canvas_buffer;

  //histograms
  for (int i_board = 0; i_board < (int) board_configuration.size(); i_board++){
 
    int board_nr = board_configuration.at(i_board);
    delete hist_align_1file.at(board_nr);
    delete hist_align_5file.at(board_nr);
    delete hist_align_10files.at(board_nr);
    delete hist_align_20files.at(board_nr);
    delete hist_align_100files.at(board_nr);
    delete hist_adc_channel.at(board_nr);
    delete hist_buffer_channel.at(board_nr);
    delete hist_buffer.at(board_nr);

  }

  //graphs
  for (int i_board = 0; i_board < (int) board_configuration.size(); i_board++){
  
    int board_nr = board_configuration.at(i_board); 
    delete graph_pps_rate.at(board_nr);
    delete graph_frame_rate.at(board_nr);
    delete graph_buffer_size.at(board_nr);
    delete graph_int_charge.at(board_nr);
  }

  //multi-graphs

  //legends

  //text
  delete text_data_title;
  delete text_pps_rate;
  delete text_frame_rate;
  delete text_int_charge;

  return true;
}

void MonitorLAPPDData::InitializeHistsLAPPD(){

  if (verbosity > 2) std::cout <<"MonitorLAPPDData: InitializeHists"<<std::endl;

  //Canvas
  canvas_status_data = new TCanvas("canvas_status_data","Status of PSEC data",900,600);
  canvas_pps_rate = new TCanvas("canvas_pps_rate","PPS Rate Time evolution",900,600);
  canvas_frame_rate = new TCanvas("canvas_frame_rate","Frame Rate Time evolution",900,600);
  canvas_buffer_size = new TCanvas("canvas_buffer_size","Buffer Size Time evolution",900,600);
  canvas_int_charge = new TCanvas("canvas_int_charge","Integrated charge Time evolution",900,600);
  canvas_align_1file = new TCanvas("canvas_align_1file","Time alignment 1 file",900,600);
  canvas_align_10files = new TCanvas("canvas_align_10files","Time alignment 10 files",900,600);
  canvas_align_100files = new TCanvas("canvas_align_100files","Time alignment 100 files",900,600);
  canvas_adc_channel = new TCanvas("canvas_adc_channel","ADC values vs. channels",900,600);
  canvas_waveform = new TCanvas("canvas_waveform","Waveform canvas",900,600);
  canvas_buffer_channel = new TCanvas("canvas_buffer_channel","LAPPD Buffer size vs. channels",900,600);
  canvas_buffer = new TCanvas("canvas_buffer","LAPPD buffer size",900,600);

  //Histograms
  //Create separate histograms for all board numbers
  for (int i_board = 0; i_board < (int) board_configuration.size(); i_board++){

    int board_nr = board_configuration.at(i_board);
    std::cout <<"Setting up histograms for board number: "<<board_nr<<std::endl;
    std::stringstream ss_align_1file, ss_align_5files, ss_align_10files, ss_align_20files, ss_align_100files;
    std::stringstream ss_adc_channel, ss_waveform_channel, ss_buffer_channel, ss_buffer;
    ss_align_1file << "hist_align_1file_board"<<board_nr;
    ss_align_5files << "hist_align_5files_board"<<board_nr;
    ss_align_10files << "hist_align_10files_board"<<board_nr;
    ss_align_20files << "hist_align_20files_board"<<board_nr;
    ss_align_100files << "hist_align_100files_board"<<board_nr;
    ss_adc_channel << "hist_adc_channel_board"<<board_nr;
    ss_waveform_channel << "hist_waveform_channel_board"<<board_nr;
    ss_buffer_channel << "hist_buffer_channel_board"<<board_nr;
    ss_buffer << "hist_buffer_board"<<board_nr;
    TH1F *hist_align_1file_single = new TH1F(ss_align_1file.str().c_str(),ss_align_1file.str().c_str(),100,-4000,4000);
    TH1F *hist_align_5files_single = new TH1F(ss_align_5files.str().c_str(),ss_align_5files.str().c_str(),100,-4000,4000);
    TH1F *hist_align_10files_single = new TH1F(ss_align_10files.str().c_str(),ss_align_10files.str().c_str(),100,-4000,4000);
    TH1F *hist_align_20files_single = new TH1F(ss_align_20files.str().c_str(),ss_align_20files.str().c_str(),100,-4000,4000);
    TH1F *hist_align_100files_single = new TH1F(ss_align_100files.str().c_str(),ss_align_100files.str().c_str(),100,-4000,4000);
    TH2F *hist_adc_channel_single = new TH2F(ss_adc_channel.str().c_str(),ss_adc_channel.str().c_str(),200,0,4096,30,0,30);
    TH2F *hist_waveform_channel_single = new TH2F(ss_waveform_channel.str().c_str(),ss_waveform_channel.str().c_str(),256,0,256,30,0,30);
    TH2F *hist_buffer_channel_single = new TH2F(ss_buffer_channel.str().c_str(),ss_buffer_channel.str().c_str(),50,0,2000,30,0,30);
    TH1F *hist_buffer_single = new TH1F(ss_buffer.str().c_str(),ss_buffer.str().c_str(),50,0,2000);

    hist_adc_channel_single->GetXaxis()->SetTitle("ADC value");
    hist_adc_channel_single->GetYaxis()->SetTitle("Channelkey");
    hist_adc_channel_single->SetStats(0);

    hist_waveform_channel_single->GetXaxis()->SetTitle("Buffer position");
    hist_waveform_channel_single->GetYaxis()->SetTitle("Channelkey");
    hist_waveform_channel_single->SetStats(0);

    hist_buffer_channel_single->GetXaxis()->SetTitle("Buffer size");
    hist_buffer_channel_single->GetYaxis()->SetTitle("Channelkey");
    hist_buffer_channel_single->SetStats(0);

    hist_buffer_single->GetXaxis()->SetTitle("Buffer size");
    hist_buffer_single->GetYaxis()->SetTitle("#");
    hist_buffer_single->SetStats(0);

    hist_align_1file.emplace(board_nr,hist_align_1file_single);
    hist_align_5files.emplace(board_nr,hist_align_5files_single);
    hist_align_10files.emplace(board_nr,hist_align_10files_single);
    hist_align_20files.emplace(board_nr,hist_align_20files_single);
    hist_align_100files.emplace(board_nr,hist_align_100files_single);
    hist_adc_channel.emplace(board_nr,hist_adc_channel_single);
    hist_waveform_channel.emplace(board_nr,hist_waveform_channel_single);
    hist_buffer_channel.emplace(board_nr,hist_buffer_channel_single);
    hist_buffer.emplace(board_nr,hist_buffer_single);
  }


  //Graphs
  //Make one of each graph type for each board
  for (int i_board = 0; i_board < (int) board_configuration.size(); i_board++){

    int board_nr = board_configuration.at(i_board);

    graph_pps_rate_single = new TGraph();
    graph_frame_rate_single = new TGraph();
    graph_buffer_size_single = new TGraph();
    graph_int_charge_single = new TGraph();

    std::stringstream ss_pps_rate, ss_frame_rate, ss_buffer_size, ss_int_charge;
    ss_pps_rate << "graph_pps_rate_board"<<board_nr;
    ss_frame_rate << "graph_frame_rate_board"<<board_nr;
    ss_buffer_size << "graph_buffer_size_board"<<board_nr;
    ss_int_charge << "graph_int_charge_board"<<board_nr;

    graph_pps_rate_single->SetName(ss_pps_rate.str().c_str());
    graph_frame_rate_single->SetName(ss_frame_rate.str().c_str());
    graph_buffer_size_single->SetName(ss_buffer_size.str().c_str());
    graph_int_charge_single->SetName(ss_int_charge.str().c_str());
    
    std::stringstream title_pps_rate, title_frame_rate, title_buffer_size, title_int_charge;
    title_pps_rate << "LAPPD PPS Rate time evolution - Board "<<board_nr;
    title_frame_rate << "LAPPD Frame Rate time evolution - Board "<<board_nr;
    title_buffer_size << "LAPPD Buffer Size time evolution - Board "<<board_nr;
    title_int_charge << "LAPPD Integrated Charge time evolution - Board "<<board_nr;

    graph_pps_rate_single->SetTitle(title_pps_rate.str().c_str());
    graph_frame_rate_single->SetTitle(title_frame_rate.str().c_str());
    graph_buffer_size_single->SetTitle(title_buffer_size.str().c_str());
    graph_int_charge_single->SetTitle(title_int_charge.str().c_str());

    if (draw_marker){
      graph_pps_rate_single->SetMarkerStyle(20);
      graph_frame_rate_single->SetMarkerStyle(20);
      graph_buffer_size_single->SetMarkerStyle(20);
      graph_int_charge_single->SetMarkerStyle(20);
    }

    graph_pps_rate_single->SetMarkerColor(kBlack);
    graph_frame_rate_single->SetMarkerColor(kBlack);
    graph_buffer_size_single->SetMarkerColor(kBlack);
    graph_int_charge_single->SetMarkerColor(kBlack);
  
    graph_pps_rate_single->SetLineColor(kBlack);
    graph_frame_rate_single->SetLineColor(kBlack);
    graph_buffer_size_single->SetLineColor(kBlack);
    graph_int_charge_single->SetLineColor(kBlack);

    graph_pps_rate_single->SetLineWidth(2);
    graph_frame_rate_single->SetLineWidth(2);
    graph_buffer_size_single->SetLineWidth(2);
    graph_int_charge_single->SetLineWidth(2);

    graph_pps_rate_single->SetFillColor(0);
    graph_frame_rate_single->SetFillColor(0);
    graph_buffer_size_single->SetFillColor(0);
    graph_int_charge_single->SetFillColor(0);

    graph_pps_rate_single->GetYaxis()->SetTitle("PPS Rate [Hz]");
    graph_frame_rate_single->GetYaxis()->SetTitle("Frame Rate [Hz]");
    graph_buffer_size_single->GetYaxis()->SetTitle("Buffer size");
    graph_int_charge_single->GetYaxis()->SetTitle("Integrated charge");

    graph_pps_rate_single->GetXaxis()->SetTimeDisplay(1);
    graph_frame_rate_single->GetXaxis()->SetTimeDisplay(1);
    graph_buffer_size_single->GetXaxis()->SetTimeDisplay(1);
    graph_int_charge_single->GetXaxis()->SetTimeDisplay(1);
 
    graph_pps_rate_single->GetXaxis()->SetLabelSize(0.03);
    graph_frame_rate_single->GetXaxis()->SetLabelSize(0.03);
    graph_buffer_size_single->GetXaxis()->SetLabelSize(0.03);
    graph_int_charge_single->GetXaxis()->SetLabelSize(0.03);
 
    graph_pps_rate_single->GetXaxis()->SetLabelOffset(0.03);
    graph_frame_rate_single->GetXaxis()->SetLabelOffset(0.03);
    graph_buffer_size_single->GetXaxis()->SetLabelOffset(0.03);
    graph_int_charge_single->GetXaxis()->SetLabelOffset(0.03);

    graph_pps_rate_single->GetXaxis()->SetTimeFormat("#splitline(%m/%d}{%H:%M}");
    graph_frame_rate_single->GetXaxis()->SetTimeFormat("#splitline(%m/%d}{%H:%M}");
    graph_buffer_size_single->GetXaxis()->SetTimeFormat("#splitline(%m/%d}{%H:%M}");
    graph_int_charge_single->GetXaxis()->SetTimeFormat("#splitline(%m/%d}{%H:%M}");

    graph_pps_rate.emplace(board_nr,graph_pps_rate_single);
    graph_frame_rate.emplace(board_nr,graph_frame_rate_single);
    graph_buffer_size.emplace(board_nr,graph_buffer_size_single);
    graph_int_charge.emplace(board_nr,graph_int_charge_single);

  }
  //Multi-Graphs

  //Legends

  //Text
  text_data_title = new TText();
  text_pps_rate = new TText();
  text_frame_rate = new TText();
  text_int_charge = new TText();

  text_data_title->SetNDC(1);
  text_pps_rate->SetNDC(1);
  text_frame_rate->SetNDC(1);
  text_int_charge->SetNDC(1);

}

void MonitorLAPPDData::ReadInConfiguration(){

  //-------------------------------------------------------
  //----------------ReadInConfiguration -------------------
  //-------------------------------------------------------

  Log("MonitorLAPPDData::ReadInConfiguration",v_message,verbosity);

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
        if (verbosity > 0) std::cout <<"ERROR (MonitorLAPPDData): ReadInConfiguration: Need at least 4 arguments in one line: TimeFrame - TimeEnd - FileLabel - PlotType1. Please look over the configuration file and adjust it accordingly."<<std::endl;
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
    if (verbosity > 0) std::cout <<"ERROR (MonitorLAPPDData): ReadInConfiguration: Could not open file "<<plot_configuration<<"! Check if path is valid..."<<std::endl;
  }
  file.close();


  if (verbosity > 2){
    std::cout <<"---------------------------------------------------------------------"<<std::endl;
    std::cout <<"MonitorLAPPDData: ReadInConfiguration: Read in the following data into configuration variables: "<<std::endl;
    for (unsigned int i_t=0; i_t < config_timeframes.size(); i_t++){
      std::cout <<config_timeframes.at(i_t)<<", "<<config_endtime.at(i_t)<<", "<<config_label.at(i_t)<<", ";
      for (unsigned int i_plot = 0; i_plot < config_plottypes.at(i_t).size(); i_plot++){
        std::cout <<config_plottypes.at(i_t).at(i_plot)<<", ";
      }
      std::cout<<std::endl;
    }
    std::cout <<"-----------------------------------------------------------------------"<<std::endl;
  }

  if (verbosity > 2) std::cout <<"MonitorLAPPDData: ReadInConfiguration: Parsing dates: "<<std::endl;
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

void MonitorLAPPDData::LoadACDCBoardConfig(std::string acdc_config){

  //Load the active ACDC board numbers specified in the configuration file
  ifstream acdc_file(acdc_config);
  int board_number;
  while (!acdc_file.eof()){
    acdc_file >> board_number;
    board_configuration.push_back(board_number);
    if (acdc_file.eof()) break; 
  }


}


std::string MonitorLAPPDData::convertTimeStamp_to_Date(ULong64_t timestamp){

    //format of date is YYYY_MM-DD
    
    boost::posix_time::ptime convertedtime = *Epoch + boost::posix_time::time_duration(int(timestamp/MSEC_to_SEC/SEC_to_MIN/MIN_to_HOUR),int(timestamp/MSEC_to_SEC/SEC_to_MIN)%60,int(timestamp/MSEC_to_SEC/1000.)%60,timestamp%1000);
    struct tm converted_tm = boost::posix_time::to_tm(convertedtime);
    std::stringstream ss_date;
    ss_date << converted_tm.tm_year+1900 << "_" << converted_tm.tm_mon+1 << "-" <<converted_tm.tm_mday;
    return ss_date.str();

}

bool MonitorLAPPDData::does_file_exist(std::string filename){

  std::ifstream infile(filename.c_str());
  bool file_good = infile.good();
  infile.close();
  return file_good;

}

void MonitorLAPPDData::WriteToFile(){

  Log("MonitorLAPPDData: WriteToFile",v_message,verbosity);

  //-------------------------------------------------------
  //------------------WriteToFile -------------------------
  //-------------------------------------------------------

  std::string file_start_date = convertTimeStamp_to_Date(t_current);
  std::stringstream root_filename;
  root_filename << path_monitoring << "LAPPDPData_" << file_start_date << ".root";

  Log("MonitorLAPPDData: ROOT filename: "+root_filename.str(),v_message,verbosity);

  std::string root_option = "RECREATE";
  if (does_file_exist(root_filename.str())) root_option = "UPDATE";
  TFile *f = new TFile(root_filename.str().c_str(),root_option.c_str());

  ULong64_t t_time;
  double t_pps_rate;
  double t_frame_rate;
  double t_int_charge;
  int t_buffer_size;
 
  TTree *t;
  if (f->GetListOfKeys()->Contains("lappddatamonitor_tree")) {
    Log("MonitorLAPPDData: WriteToFile: Tree already exists",v_message,verbosity);
    t = (TTree*) f->Get("lappddatamonitor_tree");
    t->SetBranchAddress("t_current",&t_time);
    t->SetBranchAddress("pps_rate",&t_pps_rate);
    t->SetBranchAddress("frame_rate",&t_frame_rate);
    t->SetBranchAddress("int_charge",&t_int_charge);
    t->SetBranchAddress("buffer_size",&t_buffer_size);
  } else {
    t = new TTree("lappddatamonitor_tree","LAPPD Data Monitoring tree");
    Log("MonitorLAPPDData: WriteToFile: Tree is created from scratch",v_message,verbosity);
    t->Branch("t_current",&t_time);
    t->Branch("pps_rate",&t_pps_rate);
    t->Branch("frame_rate",&t_frame_rate);
    t->Branch("int_charge",&t_int_charge);
    t->Branch("buffer_size",&t_buffer_size);
  }

  int n_entries = t->GetEntries();
  bool omit_entries = false;
  for (int i_entry = 0; i_entry < n_entries; i_entry++){
    t->GetEntry(i_entry);
    if (t_time == t_current) {
      Log("WARNING (MonitorLAPPDData): WriteToFile: Wanted to write data from file that is already written to DB. Omit entries",v_warning,verbosity);
      omit_entries = true;
    }
  }

  //if data is already written to DB/File, do not write it again
  if (omit_entries) {
    //don't write file again, but still delete TFile and TTree object!!!
    f->Close();
    //delete t_vec_errors;
    delete f;

    gROOT->cd();

    return;
  }

  //If we have vectors, they need to be cleared
  //t_vec_errors->clear();

  t_time = t_current; //XXX TODO: set t_current somewhere in the code

   boost::posix_time::ptime starttime = *Epoch + boost::posix_time::time_duration(int(t_time/MSEC_to_SEC/SEC_to_MIN/MIN_to_HOUR),int(t_time/MSEC_to_SEC/SEC_to_MIN)%60,int(t_time/MSEC_to_SEC/1000.)%60,t_time%1000);
  struct tm starttime_tm = boost::posix_time::to_tm(starttime);
  Log("MonitorLAPPDData: WriteToFile: Writing data to file: "+std::to_string(starttime_tm.tm_year+1900)+"/"+std::to_string(starttime_tm.tm_mon+1)+"/"+std::to_string(starttime_tm.tm_mday)+"-"+std::to_string(starttime_tm.tm_hour)+":"+std::to_string(starttime_tm.tm_min)+":"+std::to_string(starttime_tm.tm_sec),v_message,verbosity);

  //Get data that was processed
  t_pps_rate = current_pps_rate;
  t_frame_rate = current_frame_rate;
  t_int_charge = current_int_charge;
  t_buffer_size = current_buffer_size;

  t->Fill();
  t->Write("",TObject::kOverwrite);     //prevent ROOT from making endless keys for the same tree when updating the tree
  f->Close();

  //Delete potential vectors
  //delete t_vec_errors;
  delete f;

  gROOT->cd();

}

void MonitorLAPPDData::ReadFromFile(ULong64_t timestamp, double time_frame){

  Log("MonitorLAPPDData: ReadFromFile",v_message,verbosity);

  //-------------------------------------------------------
  //------------------ReadFromFile ------------------------
  //-------------------------------------------------------

  data_times_plot.clear();
  pps_rate_plot.clear();
  frame_rate_plot.clear();
  int_charge_plot.clear();
  buffer_size_plot.clear();
  labels_timeaxis.clear();

  //take the end time and calculate the start time with the given time_frame
  ULong64_t timestamp_start = timestamp - time_frame*MIN_to_HOUR*SEC_to_MIN*MSEC_to_SEC;
    boost::posix_time::ptime starttime = *Epoch + boost::posix_time::time_duration(int(timestamp_start/MSEC_to_SEC/SEC_to_MIN/MIN_to_HOUR),int(timestamp_start/MSEC_to_SEC/SEC_to_MIN)%60,int(timestamp_start/MSEC_to_SEC/1000.)%60,timestamp_start%1000);
  struct tm starttime_tm = boost::posix_time::to_tm(starttime);
  boost::posix_time::ptime endtime = *Epoch + boost::posix_time::time_duration(int(timestamp/MSEC_to_SEC/SEC_to_MIN/MIN_to_HOUR),int(timestamp/MSEC_to_SEC/SEC_to_MIN)%60,int(timestamp/MSEC_to_SEC/1000.)%60,timestamp%1000);
  struct tm endtime_tm = boost::posix_time::to_tm(endtime);

  Log("MonitorLAPPDData: ReadFromFile: Reading in data for time frame "+std::to_string(starttime_tm.tm_year+1900)+"/"+std::to_string(starttime_tm.tm_mon+1)+"/"+std::to_string(starttime_tm.tm_mday)+"-"+std::to_string(starttime_tm.tm_hour)+":"+std::to_string(starttime_tm.tm_min)+":"+std::to_string(starttime_tm.tm_sec)
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
    root_filename_i << path_monitoring << "LAPPDPData_" << string_date_i <<".root";
    bool tree_exists = true;

    if (does_file_exist(root_filename_i.str())) {
      TFile *f = new TFile(root_filename_i.str().c_str(),"READ");
      TTree *t;
      if (f->GetListOfKeys()->Contains("lappddatamonitor_tree")) t = (TTree*) f->Get("lappddatamonitor_tree");
      else { 
        Log("WARNING (MonitorLAPPDData): File "+root_filename_i.str()+" does not contain lappddatamonitor_tree. Omit file.",v_warning,verbosity);
        tree_exists = false;
      }

      if (tree_exists){

        Log("MonitorLAPPDData: Tree exists, start reading in data",v_message,verbosity);

        ULong64_t t_time;
        double t_pps_rate;
        double t_frame_rate;
        double t_int_charge;
        int t_buffer_size;

        int nentries_tree;

        t->SetBranchAddress("t_current",&t_time);
        t->SetBranchAddress("pps_rate",&t_pps_rate);
        t->SetBranchAddress("frame_rate",&t_frame_rate);
        t->SetBranchAddress("int_charge",&t_int_charge);
        t->SetBranchAddress("buffer_size",&t_buffer_size);

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
            data_times_plot.push_back(t_time);
            pps_rate_plot.push_back(t_pps_rate);
            frame_rate_plot.push_back(t_frame_rate);
            int_charge_plot.push_back(t_int_charge);
            buffer_size_plot.push_back(t_buffer_size);

            boost::posix_time::ptime boost_tend = *Epoch+boost::posix_time::time_duration(int(t_time/MSEC_to_SEC/SEC_to_MIN/MIN_to_HOUR),int(t_time/MSEC_to_SEC/SEC_to_MIN)%60,int(t_time/MSEC_to_SEC/1000.)%60,t_time%1000);
            struct tm label_timestamp = boost::posix_time::to_tm(boost_tend);
            
            TDatime datime_timestamp(1900+label_timestamp.tm_year,label_timestamp.tm_mon+1,label_timestamp.tm_mday,label_timestamp.tm_hour,label_timestamp.tm_min,label_timestamp.tm_sec);
            labels_timeaxis.push_back(datime_timestamp);
          }

        }
	//Delete vectors, if we have any
        //delete t_vec_errors;

      }

      f->Close();
      delete f;
      gROOT->cd();

    } else {
      Log("MonitorLAPPDData: ReadFromFile: File "+root_filename_i.str()+" does not exist. Omit file.",v_warning,verbosity);
    }

  }

  //Set the readfromfile time variables to make sure data is not read twice for the same time window
  readfromfile_tend = timestamp;
  readfromfile_timeframe = time_frame;

}

void MonitorLAPPDData::DrawLastFilePlots(){

  Log("MonitorLAPPDData: DrawLastFilePlots",v_message,verbosity);

  //-------------------------------------------------------
  //------------------DrawLastFilePlots -------------------
  //-------------------------------------------------------
  
  //Draw status of PSecData in last data file
  DrawStatus_PsecData();

  //Draw some histograms about the last file
  DrawLastFileHists();

  //Draw time alignment plots
  DrawTimeAlignment();

}

void MonitorLAPPDData::UpdateMonitorPlots(std::vector<double> timeFrames, std::vector<ULong64_t> endTimes, std::vector<std::string> fileLabels, std::vector<std::vector<std::string>> plotTypes){

  Log("MonitorLAPPDData: UpdateMonitorPlots",v_message,verbosity);

  //-------------------------------------------------------
  //------------------UpdateMonitorPlots ------------------
  //-------------------------------------------------------
 
  //Draw the monitoring plots according to the specifications in the configfiles

  for (unsigned int i_time = 0; i_time < timeFrames.size(); i_time++){

    ULong64_t zero = 0;
    if (endTimes.at(i_time) == zero) endTimes.at(i_time) = t_current;        //set 0 for t_file_end since we did not know what that was at the beginning of initialise
    
    for (unsigned int i_plot = 0; i_plot < plotTypes.at(i_time).size(); i_plot++){
      if (plotTypes.at(i_time).at(i_plot) == "TimeEvolution") DrawTimeEvolutionLAPPDData(endTimes.at(i_time),timeFrames.at(i_time),fileLabels.at(i_time));
      else {
        if (verbosity > 0) std::cout <<"ERROR (MonitorLAPPDData): UpdateMonitorPlots: Specified plot type -"<<plotTypes.at(i_time).at(i_plot)<<"- does not exist! Omit entry."<<std::endl;
      }
    }
  }


}

void MonitorLAPPDData::DrawStatus_PsecData(){

  Log("MonitorLAPPDData: DrawStatus_PsecData",v_message,verbosity);

  //-------------------------------------------------------
  //-------------DrawStatus_PsecData ----------------------
  //-------------------------------------------------------
 
   boost::posix_time::ptime currenttime = *Epoch + boost::posix_time::time_duration(int(t_current/MSEC_to_SEC/SEC_to_MIN/MIN_to_HOUR),int(t_current/MSEC_to_SEC/SEC_to_MIN)%60,int(t_current/MSEC_to_SEC)%60,t_current%1000);
  struct tm currenttime_tm = boost::posix_time::to_tm(currenttime);
  std::stringstream current_time;
  current_time << currenttime_tm.tm_year+1900<<"/"<<currenttime_tm.tm_mon+1<<"/"<<currenttime_tm.tm_mday<<"-"<<currenttime_tm.tm_hour<<":"<<currenttime_tm.tm_min<<":"<<currenttime_tm.tm_sec;
  

  //TODO: Add status board for PSEC data status
  //TODO: One Status board for each ACDC board
  // 
  // ----Rough sketch-----
  // Title: LAPPD PSEC Data
  // PPS Rate:
  // Frame Rate:
  // Buffer size:
  // Integrated charge:
  // ----End of rough sketch---
  //
  // Number of rows in canvas: 5 (maximum of 10 rows, so it fits)
   
}

void MonitorLAPPDData::DrawLastFileHists(){

  Log("MonitorLAPPDData: DrawLastFileHists",v_message,verbosity);

  //-------------------------------------------------------
  //-------------DrawLastFileHists ------------------------
  //-------------------------------------------------------
 
   boost::posix_time::ptime currenttime = *Epoch + boost::posix_time::time_duration(int(t_current/MSEC_to_SEC/SEC_to_MIN/MIN_to_HOUR),int(t_current/MSEC_to_SEC/SEC_to_MIN)%60,int(t_current/MSEC_to_SEC)%60,t_current%1000);
  struct tm currenttime_tm = boost::posix_time::to_tm(currenttime);
  std::stringstream current_time;
  current_time << currenttime_tm.tm_year+1900<<"/"<<currenttime_tm.tm_mon+1<<"/"<<currenttime_tm.tm_mday<<"-"<<currenttime_tm.tm_hour<<":"<<currenttime_tm.tm_min<<":"<<currenttime_tm.tm_sec;

  //Draw histograms for each board
  for (int i_board = 0; i_board < (int) board_configuration.size(); i_board++){

    int board_nr = board_configuration.at(i_board);

    canvas_adc_channel->Clear();
    canvas_adc_channel->cd();
    std::stringstream ss_text_adc;
    ss_text_adc << "ADC value vs. channelkey Board "<<board_nr<<" ("<<current_time.str()<<")";
    hist_adc_channel.at(board_nr)->SetTitle(ss_text_adc.str().c_str());
    hist_adc_channel.at(board_nr)->SetStats(0);
    hist_adc_channel.at(board_nr)->Draw("colz");
    std::stringstream ss_path_adc;
    ss_path_adc << outpath << "LAPPDData_ADC_Chkey_Board"<<board_nr<<"_current."<<img_extension;
    canvas_adc_channel->SaveAs(ss_path_adc.str().c_str());

    canvas_waveform->Clear();
    canvas_waveform->cd();
    std::stringstream ss_text_waveform;
    ss_text_waveform << "Exemplary waveform Board "<<board_nr<<" ("<<current_time.str()<<")";
    hist_waveform.at(board_nr)->SetTitle(ss_text_waveform.str().c_str());
    hist_waveform.at(board_nr)->SetStats(0);
    hist_waveform.at(board_nr)->Draw();
    std::stringstream ss_path_waveform;
    ss_path_waveform << outpath << "LAPPDData_Waveform_Board"<<board_nr<<"_current."<<img_extension;
    canvas_waveform->SaveAs(ss_path_waveform.str().c_str());
 
    canvas_buffer_channel->Clear();
    canvas_buffer_channel->cd();
    std::stringstream ss_text_buffer_ch;
    ss_text_buffer_ch << "LAPPD buffer size vs. channelkey Board "<<board_nr<<" ("<<current_time.str()<<")";
    hist_buffer_channel.at(board_nr)->SetTitle(ss_text_buffer_ch.str().c_str());
    hist_buffer_channel.at(board_nr)->SetStats(0);
    hist_buffer_channel.at(board_nr)->Draw("colz");
    std::stringstream ss_path_buffer_ch;
    ss_path_buffer_ch << outpath << "LAPPDData_Buffer_Ch_Board"<<board_nr<<"_current."<<img_extension;
    canvas_buffer_channel->SaveAs(ss_path_buffer_ch.str().c_str());

    canvas_buffer->Clear();
    canvas_buffer->cd();
    std::stringstream ss_text_buffer;
    ss_text_buffer << "LAPPD buffer size Board "<<board_nr<<" ("<<current_time.str()<<")";
    hist_buffer.at(board_nr)->SetTitle(ss_text_buffer.str().c_str());
    hist_buffer.at(board_nr)->SetStats(0);
    hist_buffer.at(board_nr)->Draw();
    std::stringstream ss_path_buffer;
    ss_path_buffer << outpath << "LAPPDData_Buffer_Board"<<board_nr<<"_current."<<img_extension;
    canvas_buffer->SaveAs(ss_path_buffer.str().c_str());

  }

}

void MonitorLAPPDData::DrawTimeAlignment(){

  Log("MonitorLAPPDData: DrawTimeAlignment",v_message,verbosity);

  //-------------------------------------------------------
  //-------------DrawTimeAlignment ------------------------
  //-------------------------------------------------------

  //TODO: Implement function to fill time alignment histograms
  //Probably for different time frames: Last file, last 10 files, last 20 files, ...

  for (int i_board = 0; i_board < (int) board_configuration.size(); i_board++){

    int board_nr = board_configuration.at(i_board);

    //--------Last File-----------
    hist_align_1file.at(board_nr)->Clear();
    for (int i_align=0; i_align < (int) data_beamgate_lastfile.size(); i_align++){
      hist_align_1file.at(board_nr)->Fill(data_beamgate_lastfile.at(i_align));
    }

    //-------Last 5 Files---------
    hist_align_5files.at(board_nr)->Clear();
    for (int i_align=0; i_align < (int) data_beamgate_last5files.size(); i_align++){
      for (int i_data=0; i_data < data_beamgate_last5files.at(i_align).size(); i_data++){
        hist_align_5files.at(board_nr)->Fill(data_beamgate_last5files.at(i_align).at(i_data));
      }
    }

    //-------Last 10 Files--------
    hist_align_10files.at(board_nr)->Clear();
    for (int i_align=0; i_align < (int) data_beamgate_last10files.size(); i_align++){
      for (int i_data=0; i_data < data_beamgate_last10files.at(i_align).size(); i_data++){
        hist_align_10files.at(board_nr)->Fill(data_beamgate_last10files.at(i_align).at(i_data));
      }
    }

    //------Last 20 Files---------
    hist_align_20files.at(board_nr)->Clear();
    for (int i_align=0; i_align < (int) data_beamgate_last20files.size(); i_align++){
      for (int i_data=0; i_data < data_beamgate_last20files.at(i_align).size(); i_data++){
        hist_align_20files.at(board_nr)->Fill(data_beamgate_last20files.at(i_align).at(i_data));
      }
    }
  }
}

void MonitorLAPPDData::DrawTimeEvolutionLAPPDData(ULong64_t timestamp_end, double time_frame, std::string file_ending){

  Log("MonitorLAPPDData: DrawTimeEvolutionLAPPDData",v_message,verbosity);

  //--------------------------------------------------------
  //-------------DrawTimeEvolutionLAPPDData ----------------
  // -------------------------------------------------------

  boost::posix_time::ptime endtime = *Epoch + boost::posix_time::time_duration(int(timestamp_end/MSEC_to_SEC/SEC_to_MIN/MIN_to_HOUR),int(timestamp_end/MSEC_to_SEC/SEC_to_MIN)%60,int(timestamp_end/MSEC_to_SEC/1000.)%60,timestamp_end%1000);
  struct tm endtime_tm = boost::posix_time::to_tm(endtime);
  std::stringstream end_time;
  end_time << endtime_tm.tm_year+1900<<"/"<<endtime_tm.tm_mon+1<<"/"<<endtime_tm.tm_mday<<"-"<<endtime_tm.tm_hour<<":"<<endtime_tm.tm_min<<":"<<endtime_tm.tm_sec;

  if (timestamp_end != readfromfile_tend || time_frame != readfromfile_timeframe) ReadFromFile(timestamp_end, time_frame);

  //looping over all files that are in the time interval, each file will be one data point
  
  std::stringstream ss_timeframe;
  ss_timeframe << round(time_frame*100.)/100.;

  graph_pps_rate->Set(0);
  graph_frame_rate->Set(0);
  graph_buffer_size->Set(0);
  graph_int_charge->Set(0);

  for (unsigned int i_file = 0; i_file < pps_rate_plot.size(); i_file++){

    Log("MonitorLAPPDData: Stored data (file #"+std::to_string(i_file+1)+"): ",v_message,verbosity);
    graph_pps_rate->SetPoint(i_file,labels_timeaxis[i_file].Convert(),pps_rate_plot.at(i_file));
    graph_frame_rate->SetPoint(i_file,labels_timeaxis[i_file].Convert(),frame_rate_plot.at(i_file));
    graph_buffer_size->SetPoint(i_file,labels_timeaxis[i_file].Convert(),buffer_size_plot.at(i_file));
    graph_int_charge->SetPoint(i_file,labels_timeaxis[i_file].Convert(),int_charge_plot.at(i_file));

  }

  // Drawing time evolution plots

  double max_canvas = 0;
  double min_canvas = 9999999.;
  
  std::stringstream ss_pps;
  ss_pps << "PPS rate time evolution (last "<<ss_timeframe.str()<<"h) "<<end_time.str();
  canvas_pps_rate->cd();
  canvas_pps_rate->Clear();
  graph_pps_rate->SetTitle(ss_pps.str().c_str());
  graph_pps_rate->GetYaxis()->SetTitle("PPS rate [Hz]");
  graph_pps_rate->GetXaxis()->SetTimeDisplay(1);
  graph_pps_rate->GetXaxis()->SetLabelSize(0.03);
  graph_pps_rate->GetXaxis()->SetLabelOffset(0.03);
  graph_pps_rate->GetXaxis()->SetTimeFormat("#splitline{%m/%d}{%H:%M}");
  graph_pps_rate->GetXaxis()->SetTimeOffset(0.);
  graph_pps_rate->Draw("apl");
  double max_pps = TMath::MaxElement(pps_rate_plot.size(),graph_pps_rate->GetY());
  graph_pps_rate->GetYaxis()->SetRangeUser(0.001,1.1*max_pps);
  std::stringstream ss_pps_path;
  ss_pps_path << outpath << "LAPPDData_TimeEvolution_PPSRate_"<<file_ending<<"."<<img_extension;
  canvas_pps_rate->SaveAs(ss_pps_path.str().c_str());

  std::stringstream ss_frame;
  ss_frame << "Frame rate time evolution (last "<<ss_timeframe.str()<<"h) "<<end_time.str();
  canvas_frame_rate->cd();
  canvas_frame_rate->Clear();
  graph_frame_rate->SetTitle(ss_frame.str().c_str());
  graph_frame_rate->GetYaxis()->SetTitle("Frame rate [Hz]");
  graph_frame_rate->GetXaxis()->SetTimeDisplay(1);
  graph_frame_rate->GetXaxis()->SetLabelSize(0.03);
  graph_frame_rate->GetXaxis()->SetLabelOffset(0.03);
  graph_frame_rate->GetXaxis()->SetTimeFormat("#splitline{%m/%d}{%H:%M}");
  graph_frame_rate->GetXaxis()->SetTimeOffset(0.);
  graph_frame_rate->Draw("apl");
  double max_frame = TMath::MaxElement(frame_rate_plot.size(),graph_frame_rate->GetY());
  graph_frame_rate->GetYaxis()->SetRangeUser(0.001,1.1*max_frame);
  std::stringstream ss_frame_path;
  ss_frame_path << outpath << "LAPPDData_TimeEvolution_FrameRate_"<<file_ending<<"."<<img_extension;
  canvas_frame_rate->SaveAs(ss_frame_path.str().c_str());

  std::stringstream ss_charge;
  ss_charge << "Integrated charge time evolution (last "<<ss_timeframe.str()<<"h) "<<end_time.str();
  canvas_int_charge->cd();
  canvas_int_charge->Clear();
  graph_int_charge->SetTitle(ss_charge.str().c_str());
  graph_int_charge->GetYaxis()->SetTitle("Integrated charge");
  graph_int_charge->GetXaxis()->SetTimeDisplay(1);
  graph_int_charge->GetXaxis()->SetLabelSize(0.03);
  graph_int_charge->GetXaxis()->SetLabelOffset(0.03);
  graph_int_charge->GetXaxis()->SetTimeFormat("#splitline{%m/%d}{%H:%M}");
  graph_int_charge->GetXaxis()->SetTimeOffset(0.);
  graph_int_charge->Draw("apl");
  double max_charge = TMath::MaxElement(int_charge_plot.size(),graph_int_charge->GetY());
  graph_int_charge->GetYaxis()->SetRangeUser(0.001,1.1*max_charge);
  std::stringstream ss_charge_path;
  ss_charge_path << outpath << "LAPPDData_TimeEvolution_IntCharge_"<<file_ending<<"."<<img_extension;
  canvas_int_charge->SaveAs(ss_charge_path.str().c_str());

  std::stringstream ss_buffer;
  ss_buffer << "Buffer size time evolution (last "<<ss_timeframe.str()<<"h) "<<end_time.str();
  canvas_buffer_size->cd();
  canvas_buffer_size->Clear();
  graph_buffer_size->SetTitle(ss_buffer.str().c_str());
  graph_buffer_size->GetYaxis()->SetTitle("HV [V]");
  graph_buffer_size->GetXaxis()->SetTimeDisplay(1);
  graph_buffer_size->GetXaxis()->SetLabelSize(0.03);
  graph_buffer_size->GetXaxis()->SetLabelOffset(0.03);
  graph_buffer_size->GetXaxis()->SetTimeFormat("#splitline{%m/%d}{%H:%M}");
  graph_buffer_size->GetXaxis()->SetTimeOffset(0.);
  graph_buffer_size->Draw("apl");
  double max_buffer = TMath::MaxElement(buffer_size_plot.size(),graph_buffer_size->GetY());
  graph_buffer_size->GetYaxis()->SetRangeUser(0.001,1.1*max_buffer);
  std::stringstream ss_buffer_path;
  ss_buffer_path << outpath << "LAPPDData_TimeEvolution_BufferSize_"<<file_ending<<"."<<img_extension;
  canvas_buffer_size->SaveAs(ss_buffer_path.str().c_str());

}

void MonitorLAPPDData::ProcessLAPPDData(){

 //--------------------------------------------------------
 //-------------ProcessLAPPDData --------------------------
 //--------------------------------------------------------

  //TODO: Process real data once data structure is understood
  //For now just set some default values

  hist_adc_channel->Clear();
  hist_waveform->Clear();
  hist_buffer_channel->Clear();
  hist_buffer->Clear();

  current_pps_rate = 0.0;
  current_frame_rate = 0.0;
  current_int_charge = 0.0;
  current_buffer_size = 0.0;

  beamgate_timestamp.clear();
  data_timestamp.clear();
  num_channels.clear();
  average_buffer.clear();
  data_beamgate_lastfile.clear();

  uint64_t first_beamgate_timestamp = 0;
  uint64_t last_beamgate_timestamp = 0;
  uint64_t first_timestamp = 0;
  uint64_t last_timestamp = 0;
  //map_channel_waveform.clear();

  long entries;
  LAPPDData->Header->Get("TotalEntries",entries);
  for (int i_entry = 0; i_entry < (int) entries; i_entry++){
    LAPPDData->GetEntry(i_entry);
    std::map<unsigned long,std::vector<Waveform<double>>> RawLAPPDData;
    std::vector<unsigned short> Metadata;
    std::vector<unsigned short> AccInfoFrame;
    LAPPDData->Get("RawLAPPDData",RawLAPPDData);
    LAPPDData->Get("Metadata",Metadata);
    LAPPDData->Get("AccInfoFrame",AccInfoFrame);
    std::cout <<"******************************"<<std::endl;
    std::cout <<"Entry: "<<i_entry<<", loop through LAPPD Raw data"<<std::endl;
    
    //Check if we have a Board Index entry
    //The entry after the Board Index will always be the startword of chip 0 (0xCA00, or 51712)
    //Create "offset" variable based on position of the Board Index
    unsigned short metadata_0 = Metadata.at(0);
    unsigned short metadata_1 = Metadata.at(1);
    int offset = 0;
    int board_idx = -1;
    if (metadata_0 == 51712) offset = 1;
    else if (metadata_1 == 51712) {
      offset = 0;
      board_idx = metadata_0;
    }

    if (board_idx != -1){
      std::cout <<"Metadata Board #: "<<board_idx<<std::endl;
      std::bitset<16> bits_metadata(board_idx);
      std::cout <<"Metadata Bits: "<<bits_metadata<<std::endl;
    } else {
      Log("MonitorLAPPDData: ERROR!!! Found no board index in the data!",v_error,verbosity);
    }

    //Build beamgate timestamp
    unsigned short beamgate_63_48 = Metadata.at(7-offset);	//Shift everything by 1 for the test file
    unsigned short beamgate_47_32 = Metadata.at(27-offset);	//Shift everything by 1 for the test file
    unsigned short beamgate_31_16 = Metadata.at(47-offset);	//Shift everything by 1 for the test file
    unsigned short beamgate_15_0 = Metadata.at(67-offset);	//Shift everything by 1 for the test file
    std::bitset<16> bits_beamgate_63_48(beamgate_63_48);
    std::bitset<16> bits_beamgate_47_32(beamgate_47_32);
    std::bitset<16> bits_beamgate_31_16(beamgate_31_16);
    std::bitset<16> bits_beamgate_15_0(beamgate_15_0);
    std::cout <<"bits_beamgate_63_48: "<<bits_beamgate_63_48<<std::endl;
    std::cout <<"bits_beamgate_47_32: "<<bits_beamgate_47_32<<std::endl;
    std::cout <<"bits_beamgate_31_16: "<<bits_beamgate_31_16<<std::endl;
    std::cout <<"bits_beamgate_15_0: "<<bits_beamgate_15_0<<std::endl;
    unsigned long beamgate_63_0 = (static_cast<unsigned long>(beamgate_63_48) << 48) + (static_cast<unsigned long>(beamgate_47_32) << 32) + (static_cast<unsigned long>(beamgate_31_16) << 16) + (static_cast<unsigned long>(beamgate_15_0));
    std::cout <<"beamgate combined: "<<beamgate_63_0<<std::endl;
    std::bitset<64> bits_beamgate_63_0(beamgate_63_0);
    std::cout <<"bits_beamgate_63_0: "<<bits_beamgate_63_0<<std::endl;

    //Build data timestamp
    unsigned short timestamp_63_48 = Metadata.at(70-offset);	//Shift everything by 1 for the test file
    unsigned short timestamp_47_32 = Metadata.at(50-offset);	//Shift everything by 1 for the test file
    unsigned short timestamp_31_16 = Metadata.at(30-offset);	//Shift everything by 1 for the test file
    unsigned short timestamp_15_0 = Metadata.at(10-offset);	//Shift everything by 1 for the test file
    std::bitset<16> bits_timestamp_63_48(timestamp_63_48);
    std::bitset<16> bits_timestamp_47_32(timestamp_47_32);
    std::bitset<16> bits_timestamp_31_16(timestamp_31_16);
    std::bitset<16> bits_timestamp_15_0(timestamp_15_0);
    std::cout <<"bits_timestamp_63_48: "<<bits_timestamp_63_48<<std::endl;
    std::cout <<"bits_timestamp_47_32: "<<bits_timestamp_47_32<<std::endl;
    std::cout <<"bits_timestamp_31_16: "<<bits_timestamp_31_16<<std::endl;
    std::cout <<"bits_timestamp_15_0: "<<bits_timestamp_15_0<<std::endl;
    unsigned long timestamp_63_0 = (static_cast<unsigned long>(timestamp_63_48) << 48) + (static_cast<unsigned long>(timestamp_47_32) << 32) + (static_cast<unsigned long>(timestamp_31_16) << 16) + (static_cast<unsigned long>(timestamp_15_0));
    std::cout <<"timestamp combined: "<<timestamp_63_0<<std::endl;
    std::bitset<64> bits_timestamp_63_0(timestamp_63_0);
    std::cout <<"bits_timestamp_63_0: "<<bits_timestamp_63_0<<std::endl;
    beamgate_timestamp.push_back(beamgate_63_0);
    data_timestamp.push_back(timestamp_63_0);
    data_beamgate_lastfile.push_back(timestamp_63_0-beamgate_63_0);

    if (i_entry==0){
      first_beamgate_timestamp = beamgate_63_0;
      first_timestamp = timestamp_63_0;
    } else if (i_entry == entries -1 ){
      last_beamgate_timestamp = beamgate_63_0;
      last_timestamp = timestamp_63_0;
    }

    //Loop through LAPPD data
    int n_channels = 0;
    int n_buffer = 0;
    double buffer_size = 0;
    for (std::map<unsigned long,std::vector<Waveform<double>>>::iterator it = RawLAPPDData.begin(); it!= RawLAPPDData.end(); it++){
      //std::cout <<"Got channel "<<it->first<<std::endl;
      n_channels++;
      std::vector<Waveform<double>> waveforms = it->second;
      for (int i_vec = 0; i_vec < (int) waveforms.size(); i_vec++){
        //std::cout <<"Waveform # "<<i_vec<<" contains ADC values: "<<std::endl;
        //waveforms.at(i_vec).Print();       
        std::vector<double> waveform = *(waveforms.at(i_vec).GetSamples());
        //std::cout <<"waveform size: "<<waveform.size()<<std::endl;
        hist_buffer_channel.at(board_idx)->Fill(waveform.size(),it->first);
        hist_buffer.at(board_idx)->Fill(waveform.size());
        current_buffer_size += waveform.size();
        buffer_size += waveform.size();
        n_buffer++;
        for (int i_wave=0; i_wave < waveform.size(); i_wave++){
          //std::cout <<waveform.at(i_wave)<<", ";
          hist_adc_channel.at(board_idx)->Fill(waveform.at(i_wave),it->first);
          hist_waveform_channel.at(board_idx)->SetBinContent(i_wave+1,it->first+1,hist_waveform_channel.at(board_idx)->GetBinContent(i_wave+1)+waveform.at(i_wave));
        }
        //std::cout << std::endl;
      }
    }
    std::cout <<"*******************************"<<std::endl;
    num_channels.push_back(n_channels);
    if (n_channels !=0) buffer_size /= n_channels;
    average_buffer.push_back(buffer_size);
  }

  double diff_timestamps = last_timestamp -first_timestamp;
  if (diff_timestamps > 0.) current_frame_rate = n_buffer / diff_timestamps; // Need to convert difference of timestamps into seconds or something similar
  if (n_buffer != 0) current_buffer_size /= n_buffer;
  for (int i_board=0; i_board < (int) board_configuration.size(); i_board++){
    int board_nr = board_configuration.at(i_board);
    if (entries > 0) hist_waveform_channel.at(board_nr) -> Scale(1./entries);
  }
}
