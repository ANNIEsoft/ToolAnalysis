#include "MonitorLAPPDDataSingle.h"

MonitorLAPPDDataSingle::MonitorLAPPDDataSingle():Tool(){}


bool MonitorLAPPDDataSingle::Initialise(std::string configfile, DataModel &data){

  /////////////////// Useful header ///////////////////////
  if(configfile!="") m_variables.Initialise(configfile); // loading config file
  //m_variables.Print();

  m_data= &data; //assigning transient data pointer
  /////////////////////////////////////////////////////////////////

  //only for debugging memory leaks, otherwise comment out
  std::cout <<"MonitorLAPPDDataSingle: List of Objects (beginning of Initialise): "<<std::endl;
  gObjectTable->Print();

  m_variables.Get("OutputPath",outpath_temp);
  m_variables.Get("verbose",verbosity);
  m_variables.Get("ACDCBoardConfiguration",acdc_configuration);
  m_variables.Get("ImageFormat",img_extension);
  m_variables.Get("StartTime",StartTime);

  Log("Tool MonitorLAPPDDataSingle: Initializing",v_message,verbosity);
  
  if (outpath_temp == "fromStore") m_data->CStore.Get("OutPath",outpath);
  if (verbosity > 1) std::cout <<"MonitorLAPPDDataSingle: Output path for plots is "<<outpath<<std::endl;

  if (!(img_extension == "png" || img_extension == "jpg" || img_extension == "jpeg")){
    img_extension = "jpg";
  }

  Epoch = new boost::posix_time::ptime(boost::gregorian::from_string(StartTime));

  //-------------------------------------------------------
  //----------Read in ACDC board numbers-------------------
  //-------------------------------------------------------
  
  this->LoadACDCBoardConfig(acdc_configuration);

  //-------------------------------------------------------
  //----------Initialize histograms/canvases---------------
  //-------------------------------------------------------

  this->InitializeHistsLAPPDLive();

  //-------------------------------------------------------
  //------Setup time variables for periodic updates--------
  //-------------------------------------------------------
  
  period_update = boost::posix_time::time_duration(0,0,30,0); //set update frequency for this tool to be higher than MonitorLAPPDData (5mins->30sec)
  last = boost::posix_time::ptime(boost::posix_time::second_clock::local_time());
  Epoch = new boost::posix_time::ptime(boost::gregorian::from_string("1970/1/1"));

  //omit warning messages from ROOT: info messages - 1001, warning messages - 2001, error messages - 3001
  gROOT->ProcessLine("gErrorIgnoreLevel = 3001;");

  return true;
}


bool MonitorLAPPDDataSingle::Execute(){

  if (verbosity > 2) std::cout <<"Tool MonitorLAPPDDataSingle: Executing...."<<std::endl;

  //-------------------------------------------------------
  //---------------How much time passed?-------------------
  //-------------------------------------------------------
  current = (boost::posix_time::second_clock::local_time());
  duration = boost::posix_time::time_duration(current - last);

  //-------------------------------------------------------
  //---------------Get live event info---------------------
  //-------------------------------------------------------

  bool has_lappd;
  m_data->CStore.Get("HasLAPPDData",has_lappd);

  std::string State;
  m_data->CStore.Get("State",State);

  if (has_lappd){

    if (State == "LAPPDSingle"){

      //Process single LAPPD data entry
      this->ProcessLAPPDDataLive();

      //Draw live plots
      this->DrawLivePlots();


    } else if (State == "DataFile" || State == "Wait"){

      if (verbosity > 5) std::cout <<"Status file (DataFile or Wait): MonitorLAPPDDataSingle not executed..."<<std::endl;

    } else {

      if (verbosity > 1) std::cout <<"State not recognized: "<<State<<std::endl;
    }

  }

  if (duration >= period_update){

    if (verbosity > 1) std::cout <<"MonitorLAPPDDataSingle: Update period has passed --- Updating live plots"<<std::endl;
    last =current;

    //TODO: DrawLiveTimestampEvolution
    //Show the last recorded timestamps for each board (in the last two hours or so)
    this->DrawLiveTimestampEvolution();

  }

  //Check for ROOT-related memory leaks (just for debugging)
  //std::cout <<"MonitorLAPPDDataSingle: End of Execute"<<std::endl;
  //gObjectTable->Print();


  return true;
}


bool MonitorLAPPDDataSingle::Finalise(){

  if (verbosity > 2) std::cout <<"Tool MonitorLAPPDDataSingle: Finalising..."<<std::endl;

   std::cout <<"Delete TCanvas"<<std::endl;
   //TCanvas
   delete canvas_live_status;
   delete canvas_live_adc_channel;
   delete canvas_live_buffer_channel;
   delete canvas_live_timeevolution;
   delete canvas_live_occupancy;

   std::cout <<"Delete TH1"<<std::endl;
   //TH1
   for (int i_board = 0; i_board < (int) board_configuration.size(); i_board++){
     int board_nr = board_configuration.at(i_board);
     delete hist_live_adc.at(board_nr);
     delete hist_live_buffer.at(board_nr);
     delete hist_live_time.at(board_nr);
     delete hist_live_occupancy.at(board_nr);
  }

  std::cout <<"Delete TText"<<std::endl;
  //TText
  delete text_live_status;
  for (int i_board = 0; i_board < (int) board_configuration.size(); i_board++){
    delete text_live_timestamp.at(board_configuration.at(i_board));
  }

  return true;
}

void MonitorLAPPDDataSingle::LoadACDCBoardConfig(std::string acdc_config){

  Log("MonitorLAPPDDataSingle: LoadACDCBoardConfig",v_message,verbosity);

  //Load the active ACDC board numbers specified in the configuration file
   ifstream acdc_file(acdc_config);
  int board_number, board_ch;
  while (!acdc_file.eof()){
    acdc_file >> board_number >> board_ch;
    if (acdc_file.eof()) break; 
    Log("MonitorLAPPDData: Setting Board number >>>"+std::to_string(board_number)+"<<< as an active LAPPD ACDC board number",v_message,verbosity);
    board_configuration.push_back(board_number);
    board_channel.push_back(board_ch);

    current_buffer_size.emplace(board_number,0.);
    current_beam_timestamp.emplace(board_number,0);
    current_timestamp.emplace(board_number,0);
    current_numchannels.emplace(board_number,0);
    current_hasdata.emplace(board_number,false);
  }
  acdc_file.close();

  for (int i_board=0; i_board < (int) board_configuration.size(); i_board++){
    std::vector<ULong64_t> temp_vector;
    vec_timestamps_10.emplace(board_configuration.at(i_board),temp_vector);
    vec_timestamps_100.emplace(board_configuration.at(i_board),temp_vector);
    vec_timestamps_1000.emplace(board_configuration.at(i_board),temp_vector);
    vec_timestamps_10000.emplace(board_configuration.at(i_board),temp_vector);
  }

}

void MonitorLAPPDDataSingle::InitializeHistsLAPPDLive(){

  if (verbosity > 2) std::cout <<"MonitorLAPPDDataSingle: InitializeHists"<<std::endl;

  gROOT->cd();
  
  //Canvas
  canvas_live_status = new TCanvas("canvas_live_status","LAPPD Live Status",900,600);
  canvas_live_adc_channel = new TCanvas("canvas_live_adc_channel","LAPPD Live ADC vs. channel",900,600);
  canvas_live_buffer_channel = new TCanvas("canvas_live_buffer_channel","LAPPD Live Buffer size vs. channel",900,600);
  canvas_live_waveform_channel = new TCanvas("canvas_live_waveform_channel","LAPPD Live Waveform vs. channel",900,600);
  canvas_live_timeevolution = new TCanvas("canvas_live_timeevolution","Time evolution Live Data",900,600);
  canvas_live_occupancy = new TCanvas("canvas_live_occupancy","LAPPD Live channel occupancy",900,600);
  canvas_live_timealign_10 = new TCanvas("canvas_live_timealign_10","LAPPD timestamp - beamgate",900,600);
  canvas_live_timealign_100 = new TCanvas("canvas_live_timealign_100","LAPPD timestamp - beamgate",900,600);
  canvas_live_timealign_1000 = new TCanvas("canvas_live_timealign_1000","LAPPD timestamp - beamgate",900,600);
  canvas_live_timealign_10000 = new TCanvas("canvas_live_timealign_10000","LAPPD timestamp - beamgate",900,600);

  //Histograms
  for (int i_board = 0; i_board < (int) board_configuration.size(); i_board++){

    int board_nr = board_configuration.at(i_board);
    std::stringstream ss_adc_channel, title_adc_channel;
    std::stringstream ss_buffer_channel, title_buffer_channel;
    std::stringstream ss_waveform_channel, title_waveform_channel;
    std::stringstream ss_timeevolution, title_timeevolution;
    std::stringstream ss_occupancy, title_occupancy;
    std::stringstream ss_timealign_10, title_timealign_10;
    std::stringstream ss_timealign_100, title_timealign_100;
    std::stringstream ss_timealign_1000, title_timealign_1000;
    std::stringstream ss_timealign_10000, title_timealign_10000;

    ss_adc_channel << "hist_live_adc_channel_board"<<board_nr;
    ss_buffer_channel << "hist_live_buffer_channel_board"<<board_nr;
    ss_timeevolution << "hist_live_timeevolution_board"<<board_nr;
    ss_occupancy << "hist_live_occupancy_board"<<board_nr;
    ss_waveform_channel << "hist_live_waveform_channel_board"<<board_nr;
    ss_timealign_10 << "hist_live_timealign_10_board"<<board_nr;
    ss_timealign_100 << "hist_live_timealign_100_board"<<board_nr;
    ss_timealign_1000 << "hist_live_timealign_1000_board"<<board_nr;
    ss_timealign_10000 << "hist_live_timealign_10000_board"<<board_nr;

    title_adc_channel << "LAPPD Live ADC vs. channel number, Board "<<board_nr;
    title_buffer_channel << "LAPPD Live Buffer size vs. channel number, Board "<<board_nr;
    title_timeevolution << "LAPPD Live Time Evolution, Board "<<board_nr;
    title_occupancy << "LAPPD Live Channel Occupancy, Board "<<board_nr;
    title_waveform_channel << "LAPPD Live Waveform, Board "<<board_nr;
    title_timealign_10 << "LAPPD Timestamp alignment (10 events), Board "<<board_nr;
    title_timealign_100 << "LAPPD Timestamp alignment (100 events), Board "<<board_nr;
    title_timealign_1000 << "LAPPD Timestamp alignment (1000 events), Board "<<board_nr;
    title_timealign_10000 << "LAPPD Timestamp alignment (10,000 events), Board "<<board_nr;

    TH2F *hist_adc_channel = new TH2F(ss_adc_channel.str().c_str(),title_adc_channel.str().c_str(),200,0,4096,30,0,30);
    TH2F *hist_buffer_channel = new TH2F(ss_buffer_channel.str().c_str(),title_buffer_channel.str().c_str(),50,0,2000,30,0,30);
    TH1F *hist_timeevolution = new TH1F(ss_timeevolution.str().c_str(),title_timeevolution.str().c_str(),10,0,10);
    TH1F *hist_occupancy = new TH1F(ss_occupancy.str().c_str(),title_occupancy.str().c_str(),30,0,30);
    TH2F *hist_waveform_channel = new TH2F(ss_waveform_channel.str().c_str(),title_waveform_channel.str().c_str(),256,0,256,30,0,30);
    TH1F *hist_timealign_10 = new TH1F(ss_timealign_10.str().c_str(),title_timealign_10.str().c_str(),100,-4000,4000);
    TH1F *hist_timealign_100 = new TH1F(ss_timealign_100.str().c_str(),title_timealign_100.str().c_str(),100,-4000,4000);
    TH1F *hist_timealign_1000 = new TH1F(ss_timealign_1000.str().c_str(),title_timealign_1000.str().c_str(),100,-4000,4000);
    TH1F *hist_timealign_10000 = new TH1F(ss_timealign_10000.str().c_str(),title_timealign_10000.str().c_str(),100,-4000,4000);

    hist_adc_channel->GetXaxis()->SetTitle("ADC value");
    hist_adc_channel->GetYaxis()->SetTitle("Channelkey");
    hist_adc_channel->SetStats(0);

    hist_buffer_channel->GetXaxis()->SetTitle("Buffer position");
    hist_buffer_channel->GetYaxis()->SetTitle("Channelkey");
    hist_buffer_channel->SetStats(0);

    hist_timeevolution->GetXaxis()->SetTitle("Date");
    hist_timeevolution->SetStats(0);
  
    hist_occupancy->GetXaxis()->SetTitle("Channel #");
    hist_occupancy->GetYaxis()->SetTitle("Frequency");
    hist_occupancy->SetStats(0);

    hist_waveform_channel->GetXaxis()->SetTitle("Buffer position");
    hist_waveform_channel->GetYaxis()->SetTitle("ADC");
    hist_waveform_channel->SetStats(0);

    hist_timealign_10->GetXaxis()->SetTitle("Timestamp - Beamgate [ns]");
    hist_timealign_10->GetYaxis()->SetTitle("#");
    hist_timealign_10->SetStats(0);
    
    hist_timealign_100->GetXaxis()->SetTitle("Timestamp - Beamgate [ns]");
    hist_timealign_100->GetYaxis()->SetTitle("#");
    hist_timealign_100->SetStats(0);

    hist_timealign_1000->GetXaxis()->SetTitle("Timestamp - Beamgate [ns]");
    hist_timealign_1000->GetYaxis()->SetTitle("#");
    hist_timealign_1000->SetStats(0);

    hist_timealign_10000->GetXaxis()->SetTitle("Timestamp - Beamgate [ns]");
    hist_timealign_10000->GetYaxis()->SetTitle("#");
    hist_timealign_10000->SetStats(0);

    hist_live_adc.emplace(board_nr,hist_adc_channel);
    hist_live_buffer.emplace(board_nr,hist_buffer_channel);
    hist_live_time.emplace(board_nr,hist_timeevolution);
    hist_live_occupancy.emplace(board_nr,hist_occupancy);   
    hist_live_waveform.emplace(board_nr,hist_waveform_channel);
    hist_live_timealign_10.emplace(board_nr,hist_timealign_10); 
    hist_live_timealign_100.emplace(board_nr,hist_timealign_100); 
    hist_live_timealign_1000.emplace(board_nr,hist_timealign_1000); 
    hist_live_timealign_10000.emplace(board_nr,hist_timealign_10000); 

  }

  text_live_status = new TText();
  text_live_status->SetNDC(1);
  for (int i_board = 0; i_board < (int) board_configuration.size(); i_board++){
    TText *text_lasttimestamp = new TText();
    text_lasttimestamp->SetNDC(1);
    int board_nr = board_configuration.at(i_board);
    text_live_timestamp.emplace(board_nr,text_lasttimestamp);
  }
  
}

void MonitorLAPPDDataSingle::ProcessLAPPDDataLive(){

 //--------------------------------------------------------
 //-------------ProcessLAPPDDataLive ----------------------
 //--------------------------------------------------------

  std::map<unsigned long, std::vector<Waveform<double>>> RawLAPPDData;
  std::vector<unsigned short> Metadata;
  std::vector<unsigned short> AccInfoFrame;
  m_data->Stores["LAPPDData"]->Get("RawLAPPDData",RawLAPPDData);
  m_data->Stores["LAPPDData"]->Get("Metadata",Metadata);
  m_data->Stores["LAPPDData"]->Get("AccInfoFrame",AccInfoFrame);
 
  unsigned short metadata_0 = Metadata.at(0);
  unsigned short metadata_1 = Metadata.at(1);
  int offset = 0;
  int board_idx = -1;
  if (metadata_0 == 56496 /*51712*/) offset = 1;
  else if (metadata_1 == 56496 /*51712*/) {
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
 
  std::cout <<"hist_live_occupancy entries (before): "<<hist_live_occupancy.at(board_idx)->GetEntries();
  std::cout <<"count hist_live_adc: "<<hist_live_adc.count(board_idx)<<std::endl;
  if (hist_live_adc.count(board_idx)>0){
    hist_live_adc.at(board_idx)->Reset();
    hist_live_buffer.at(board_idx)->Reset();
    hist_live_waveform.at(board_idx)->Reset();
    hist_live_time.at(board_idx)->Reset();
    hist_live_occupancy.at(board_idx)->Reset();
  }
  std::cout <<"hist_live_occupancy entries: "<<hist_live_occupancy.at(board_idx)->GetEntries();

  //Build beamgate timestamp
  unsigned short beamgate_63_48 = Metadata.at(7-offset);	//Shift everything by 1 for the test file
  unsigned short beamgate_47_32 = Metadata.at(27-offset);	//Shift everything by 1 for the test file
  unsigned short beamgate_31_16 = Metadata.at(47-offset);	//Shift everything by 1 for the test file
  unsigned short beamgate_15_0 = Metadata.at(67-offset);	//Shift everything by 1 for the test file
  std::bitset<16> bits_beamgate_63_48(beamgate_63_48);
  std::bitset<16> bits_beamgate_47_32(beamgate_47_32);
  std::bitset<16> bits_beamgate_31_16(beamgate_31_16);
  std::bitset<16> bits_beamgate_15_0(beamgate_15_0);
  //std::cout <<"bits_beamgate_63_48: "<<bits_beamgate_63_48<<std::endl;
  //std::cout <<"bits_beamgate_47_32: "<<bits_beamgate_47_32<<std::endl;
  //std::cout <<"bits_beamgate_31_16: "<<bits_beamgate_31_16<<std::endl;
  //std::cout <<"bits_beamgate_15_0: "<<bits_beamgate_15_0<<std::endl;
  unsigned long beamgate_63_0 = (static_cast<unsigned long>(beamgate_63_48) << 48) + (static_cast<unsigned long>(beamgate_47_32) << 32) + (static_cast<unsigned long>(beamgate_31_16) << 16) + (static_cast<unsigned long>(beamgate_15_0));
  std::cout <<"beamgate combined: "<<beamgate_63_0<<std::endl;
  std::bitset<64> bits_beamgate_63_0(beamgate_63_0);
  //std::cout <<"bits_beamgate_63_0: "<<bits_beamgate_63_0<<std::endl;

  //Build data timestamp
  unsigned short timestamp_63_48 = Metadata.at(70-offset);	//Shift everything by 1 for the test file
  unsigned short timestamp_47_32 = Metadata.at(50-offset);	//Shift everything by 1 for the test file
  unsigned short timestamp_31_16 = Metadata.at(30-offset);	//Shift everything by 1 for the test file
  unsigned short timestamp_15_0 = Metadata.at(10-offset);	//Shift everything by 1 for the test file
  std::bitset<16> bits_timestamp_63_48(timestamp_63_48);
  std::bitset<16> bits_timestamp_47_32(timestamp_47_32);
  std::bitset<16> bits_timestamp_31_16(timestamp_31_16);
  std::bitset<16> bits_timestamp_15_0(timestamp_15_0);
  //std::cout <<"bits_timestamp_63_48: "<<bits_timestamp_63_48<<std::endl;
  //std::cout <<"bits_timestamp_47_32: "<<bits_timestamp_47_32<<std::endl;
  //std::cout <<"bits_timestamp_31_16: "<<bits_timestamp_31_16<<std::endl;
  //std::cout <<"bits_timestamp_15_0: "<<bits_timestamp_15_0<<std::endl;
  unsigned long timestamp_63_0 = (static_cast<unsigned long>(timestamp_63_48) << 48) + (static_cast<unsigned long>(timestamp_47_32) << 32) + (static_cast<unsigned long>(timestamp_31_16) << 16) + (static_cast<unsigned long>(timestamp_15_0));
  std::cout <<"timestamp combined: "<<timestamp_63_0<<std::endl;
  std::bitset<64> bits_timestamp_63_0(timestamp_63_0);
  //std::cout <<"bits_timestamp_63_0: "<<bits_timestamp_63_0<<std::endl;

  int n_channels = 0;
  double buffer_size = 0;
  for (std::map<unsigned long,std::vector<Waveform<double>>>::iterator it = RawLAPPDData.begin(); it!= RawLAPPDData.end(); it++){

    n_channels++;
    std::vector<Waveform<double>> waveforms = it->second;
    for (int i_vec = 0; i_vec < (int) waveforms.size(); i_vec++){
      std::vector<double> waveform = *(waveforms.at(i_vec).GetSamples());
      hist_live_buffer.at(board_idx)->Fill(waveform.size(),it->first);
      buffer_size += waveform.size();
      hist_live_occupancy.at(board_idx)->Fill(it->first);
      for (int i_wave=0; i_wave < (int) waveform.size(); i_wave++){ 
        hist_live_adc.at(board_idx)->Fill(waveform.at(i_wave),it->first);
        hist_live_waveform.at(board_idx)->SetBinContent(i_wave+1,it->first+1,hist_live_waveform.at(board_idx)->GetBinContent(i_wave+1,it->first+1)+waveform.at(i_wave));
      }
    }
  }
  if (n_channels !=0) buffer_size /= n_channels;

  beamgate_63_0 *= (CLOCK_to_SEC*1000);
  timestamp_63_0 *= (CLOCK_to_SEC*1000);

  current_buffer_size[board_idx] = buffer_size;
  current_beam_timestamp[board_idx] = beamgate_63_0;
  current_timestamp[board_idx] = timestamp_63_0;
  current_numchannels[board_idx] = n_channels;
  current_hasdata[board_idx] = true;
  current_boardidx = board_idx;

  vec_timestamps_10[board_idx].push_back(timestamp_63_0);
  vec_beamgates_10[board_idx].push_back(beamgate_63_0);
  vec_timestamps_100[board_idx].push_back(timestamp_63_0);
  vec_beamgates_100[board_idx].push_back(beamgate_63_0);
  vec_timestamps_1000[board_idx].push_back(timestamp_63_0);
  vec_beamgates_1000[board_idx].push_back(beamgate_63_0);
  vec_timestamps_10000[board_idx].push_back(timestamp_63_0);
  vec_beamgates_10000[board_idx].push_back(beamgate_63_0);


  while (vec_timestamps_10[board_idx].size() > 10){
    vec_timestamps_10[board_idx].erase(vec_timestamps_10[board_idx].begin());
    vec_beamgates_10[board_idx].erase(vec_beamgates_10[board_idx].begin());
  }
  while (vec_timestamps_100[board_idx].size() > 100){
    vec_timestamps_100[board_idx].erase(vec_timestamps_100[board_idx].begin());
    vec_beamgates_100[board_idx].erase(vec_beamgates_100[board_idx].begin());
  }
  while (vec_timestamps_1000[board_idx].size() > 1000){
    vec_timestamps_1000[board_idx].erase(vec_timestamps_1000[board_idx].begin());
    vec_beamgates_1000[board_idx].erase(vec_beamgates_1000[board_idx].begin());
  }
  while (vec_timestamps_10000[board_idx].size() > 10000){
    vec_timestamps_10000[board_idx].erase(vec_timestamps_10000[board_idx].begin());
    vec_beamgates_10000[board_idx].erase(vec_beamgates_10000[board_idx].begin());
  }

  hist_live_timealign_10.at(board_idx)->Reset();
  hist_live_timealign_100.at(board_idx)->Reset();
  hist_live_timealign_1000.at(board_idx)->Reset();
  hist_live_timealign_10000.at(board_idx)->Reset();

  for (int i_vec=0; i_vec < (int) vec_timestamps_10.at(board_idx).size(); i_vec++){
    hist_live_timealign_10.at(board_idx)->Fill(vec_timestamps_10[board_idx].at(i_vec)-vec_beamgates_10[board_idx].at(i_vec));
  }
  for (int i_vec=0; i_vec < (int) vec_timestamps_100.at(board_idx).size(); i_vec++){
    hist_live_timealign_100.at(board_idx)->Fill(vec_timestamps_100[board_idx].at(i_vec)-vec_beamgates_100[board_idx].at(i_vec));
  }
  for (int i_vec=0; i_vec < (int) vec_timestamps_1000.at(board_idx).size(); i_vec++){
    hist_live_timealign_1000.at(board_idx)->Fill(vec_timestamps_1000[board_idx].at(i_vec)-vec_beamgates_1000[board_idx].at(i_vec));
  }
  for (int i_vec=0; i_vec < (int) vec_timestamps_10000.at(board_idx).size(); i_vec++){
    hist_live_timealign_10000.at(board_idx)->Fill(vec_timestamps_10000[board_idx].at(i_vec)-vec_beamgates_10000[board_idx].at(i_vec));
  }

}

void MonitorLAPPDDataSingle::DrawLivePlots(){

  Log("MonitorLAPPDDataSingle: DrawLivePlots",v_message,verbosity);

  //-------------------------------------------------------
  //------------------DrawLivePlots -----------------------
  //-------------------------------------------------------
  
  //Draw live histograms
  DrawLiveHistograms();

  //Draw status board
  DrawLiveStatus();

}

void MonitorLAPPDDataSingle::DrawLiveHistograms(){

  //-------------------------------------------------------
  //------------------DrawLiveHistograms ------------------
  //-------------------------------------------------------
  
  Log("MonitorLAPPDDataSingle: DrawLiveHistograms",v_message,verbosity);
 
  int board_nr = current_boardidx;

  uint64_t t_live = current_timestamp[current_boardidx];
  boost::posix_time::ptime livetime = *Epoch + boost::posix_time::time_duration(int(t_live/MSEC_to_SEC/SEC_to_MIN/MIN_to_HOUR),int(t_live/MSEC_to_SEC/SEC_to_MIN)%60,int(t_live/MSEC_to_SEC)%60,t_live%1000);
  struct tm livetime_tm = boost::posix_time::to_tm(livetime);
  std::stringstream live_time;
  live_time << livetime_tm.tm_year+1900<<"/"<<livetime_tm.tm_mon+1<<"/"<<livetime_tm.tm_mday<<"-"<<livetime_tm.tm_hour<<":"<<livetime_tm.tm_min<<":"<<livetime_tm.tm_sec;
  
  canvas_live_status->Clear();
  canvas_live_status->cd();
  std::stringstream ss_text_status;

  for (int i_board=0; i_board < (int) board_configuration.size(); i_board++){
    int board = board_configuration.at(i_board);
    canvas_live_adc_channel->Clear();
    canvas_live_adc_channel->cd();
    std::stringstream ss_text_adc;
    ss_text_adc << "LAPPD Live ADC value Board "<<board<<" ("<<live_time.str()<<")";
    hist_live_adc.at(board)->SetTitle(ss_text_adc.str().c_str());
    hist_live_adc.at(board)->SetStats(0);
    hist_live_adc.at(board)->Draw("colz");
    std::stringstream ss_path_adc;
    ss_path_adc << outpath << "LAPPDData_ADC_Chkey_Board"<<board<<"_Live."<<img_extension;
    canvas_live_adc_channel->SaveAs(ss_path_adc.str().c_str());
  }

  for (int i_board=0; i_board < (int) board_configuration.size(); i_board++){
    int board = board_configuration.at(i_board);
    canvas_live_buffer_channel->Clear();
    canvas_live_buffer_channel->cd();
    std::stringstream ss_text_buffer;
    ss_text_buffer << "LAPPD Live Buffer size Board "<<board<<" ("<<live_time.str()<<")";
    hist_live_buffer.at(board)->SetTitle(ss_text_buffer.str().c_str());
    hist_live_buffer.at(board)->SetStats(0);
    hist_live_buffer.at(board)->Draw("colz");
    std::stringstream ss_path_buffer;
    ss_path_buffer << outpath << "LAPPDData_Buffer_Chkey_Board"<<board<<"_Live."<<img_extension;
    canvas_live_buffer_channel->SaveAs(ss_path_buffer.str().c_str());  
  }

  for (int i_board=0; i_board < (int) board_configuration.size(); i_board++){
    int board = board_configuration.at(i_board);
    canvas_live_waveform_channel->Clear();
    canvas_live_waveform_channel->cd();
    std::stringstream ss_text_waveform;
    ss_text_waveform << "LAPPD Live Waveform Board "<<board<<" ("<<live_time.str()<<")";
    hist_live_waveform.at(board)->SetTitle(ss_text_waveform.str().c_str());
    hist_live_waveform.at(board)->SetStats(0);
    hist_live_waveform.at(board)->Draw("colz");
    std::stringstream ss_path_waveform;
    ss_path_waveform << outpath << "LAPPDData_Waveform_Chkey_Board"<<board<<"_Live."<<img_extension;
    canvas_live_waveform_channel->SaveAs(ss_path_waveform.str().c_str());
  }

  for (int i_board=0; i_board < (int) board_configuration.size(); i_board++){
    int board = board_configuration.at(i_board);
    canvas_live_timeevolution->Clear();
    canvas_live_timeevolution->cd();
    std::stringstream ss_text_timeevolution;
    ss_text_timeevolution << "LAPPD Live Time evolution Board "<<board<<" ("<< live_time.str()<<")";
    hist_live_time.at(board)->SetTitle(ss_text_timeevolution.str().c_str());
    hist_live_time.at(board)->SetStats(0);
    hist_live_time.at(board)->Draw("colz");
    std::stringstream ss_path_timeev;
    ss_path_timeev << outpath << "LAPPDData_TimeEv_Board"<<board<<"_Live."<<img_extension;
    canvas_live_timeevolution->SaveAs(ss_path_timeev.str().c_str());
  }

  for (int i_board=0; i_board < (int) board_configuration.size(); i_board++){
    int board = board_configuration.at(i_board);
    canvas_live_occupancy->Clear();
    canvas_live_occupancy->cd();
    std::stringstream ss_text_occupancy;
    ss_text_occupancy << "LAPPD Live Occupancy Board "<<board<<" ("<<live_time.str()<<")";
    hist_live_occupancy.at(board)->SetTitle(ss_text_occupancy.str().c_str());
    hist_live_occupancy.at(board)->SetStats(0);
    hist_live_occupancy.at(board)->Draw();
    std::stringstream ss_path_occupancy;
    ss_path_occupancy << outpath << "LAPPDData_Occupancy_Board"<<board<<"_Live."<<img_extension;
    canvas_live_occupancy->SaveAs(ss_path_occupancy.str().c_str());
  }

  for (int i_board=0; i_board < (int) board_configuration.size(); i_board++){
    int board = board_configuration.at(i_board);
    canvas_live_timealign_10->Clear();
    canvas_live_timealign_10->cd();
    std::stringstream ss_text_timealign_10;
    ss_text_timealign_10 << "LAPPD Live Time Alignment 10 events - Board "<<board<<" ("<<live_time.str()<<")";
    hist_live_timealign_10.at(board)->SetTitle(ss_text_timealign_10.str().c_str());
    hist_live_timealign_10.at(board)->SetStats(0);
    hist_live_timealign_10.at(board)->Draw();
    std::stringstream ss_path_align_10;
    ss_path_align_10 << outpath << "LAPPDData_TimeAlign_10_Board"<<board<<"_Live."<<img_extension;
    canvas_live_timealign_10->SaveAs(ss_path_align_10.str().c_str());
  }
  
  for (int i_board=0; i_board < (int) board_configuration.size(); i_board++){
    int board = board_configuration.at(i_board);
    canvas_live_timealign_100->Clear();
    canvas_live_timealign_100->cd();
    std::stringstream ss_text_timealign_100;
    ss_text_timealign_100 << "LAPPD Live Time Alignment 100 events - Board "<<board<<" ("<<live_time.str()<<")";
    hist_live_timealign_100.at(board)->SetTitle(ss_text_timealign_100.str().c_str());
    hist_live_timealign_100.at(board)->SetStats(0);
    hist_live_timealign_100.at(board)->Draw();
    std::stringstream ss_path_align_100;
    ss_path_align_100 << outpath << "LAPPDData_TimeAlign_100_Board"<<board<<"_Live."<<img_extension;
    canvas_live_timealign_100->SaveAs(ss_path_align_100.str().c_str());
  }

  for (int i_board=0; i_board < (int) board_configuration.size(); i_board++){
    int board = board_configuration.at(i_board);
    canvas_live_timealign_1000->Clear();
    canvas_live_timealign_1000->cd();
    std::stringstream ss_text_timealign_1000;
    ss_text_timealign_1000 << "LAPPD Live Time Alignment 1000 events - Board "<<board<<" ("<<live_time.str()<<")";
    hist_live_timealign_1000.at(board)->SetTitle(ss_text_timealign_1000.str().c_str());
    hist_live_timealign_1000.at(board)->SetStats(0);
    hist_live_timealign_1000.at(board)->Draw();
    std::stringstream ss_path_align_1000;
    ss_path_align_1000 << outpath << "LAPPDData_TimeAlign_1000_Board"<<board<<"_Live."<<img_extension;
    canvas_live_timealign_1000->SaveAs(ss_path_align_1000.str().c_str());
  }

  for (int i_board=0; i_board < (int) board_configuration.size(); i_board++){
    int board = board_configuration.at(i_board);
    canvas_live_timealign_10000->Clear();
    canvas_live_timealign_10000->cd();
    std::stringstream ss_text_timealign_10000;
    ss_text_timealign_10000 << "LAPPD Live Time Alignment 10000 events - Board "<<board<<" ("<<live_time.str()<<")";
    hist_live_timealign_10000.at(board)->SetTitle(ss_text_timealign_10000.str().c_str());
    hist_live_timealign_10000.at(board)->SetStats(0);
    hist_live_timealign_10000.at(board)->Draw();
    std::stringstream ss_path_align_10000;
    ss_path_align_10000 << outpath << "LAPPDData_TimeAlign_10000_Board"<<board<<"_Live."<<img_extension;
    canvas_live_timealign_10000->SaveAs(ss_path_align_10000.str().c_str());
  }

}

void MonitorLAPPDDataSingle::DrawLiveStatus(){

  //-------------------------------------------------------
  //------------------DrawLiveStatus ----------------------
  //-------------------------------------------------------

  Log("MonitorLAPPDDataSingle: DrawLiveStatus",v_message,verbosity);

  int board_nr = current_boardidx;

  uint64_t t_live = current_timestamp[current_boardidx];
  boost::posix_time::ptime livetime = *Epoch + boost::posix_time::time_duration(int(t_live/MSEC_to_SEC/SEC_to_MIN/MIN_to_HOUR),int(t_live/MSEC_to_SEC/SEC_to_MIN)%60,int(t_live/MSEC_to_SEC)%60,t_live%1000);
  struct tm livetime_tm = boost::posix_time::to_tm(livetime);
  std::stringstream live_time;
  live_time << livetime_tm.tm_year+1900<<"/"<<livetime_tm.tm_mon+1<<"/"<<livetime_tm.tm_mday<<"-"<<livetime_tm.tm_hour<<":"<<livetime_tm.tm_min<<":"<<livetime_tm.tm_sec;

  //Design
  //Title: LAPPD Live Status
  //Last Timestamp Board 0: XXX
  //Last Timestamp Board 1: YYY
  //Last Timestamp Board 2: ZZZ

  canvas_live_status->Clear();
  canvas_live_status->cd();
  std::stringstream ss_title;
  ss_title << "LAPPD Live Data Status ("<<live_time.str()<<")";
  text_live_status->SetText(0.06,0.9,ss_title.str().c_str()); 
  text_live_status->SetTextSize(0.05);
  text_live_status->SetNDC(1);
  text_live_status->Draw(); 

  std::stringstream ss_timestamp_board;
  int board = 0;
  for (int i_board = 0; i_board < (int) board_configuration.size(); i_board++){
    
    int board_nr = board_configuration.at(i_board);
    ss_timestamp_board.str("");
    ss_timestamp_board << "Last Timestamp Board "<<board_nr<<": ";
    if (current_hasdata[board_nr]) {
      uint64_t t_board = current_timestamp[board_nr];
      boost::posix_time::ptime boardtime = *Epoch + boost::posix_time::time_duration(int(t_board/MSEC_to_SEC/SEC_to_MIN/MIN_to_HOUR),int(t_board/MSEC_to_SEC/SEC_to_MIN)%60,int(t_board/MSEC_to_SEC)%60,t_board%1000);
      struct tm boardtime_tm = boost::posix_time::to_tm(boardtime);
      std::stringstream board_time;
      board_time << boardtime_tm.tm_year+1900<<"/"<<boardtime_tm.tm_mon+1<<"/"<<boardtime_tm.tm_mday<<"-"<<boardtime_tm.tm_hour<<":"<<boardtime_tm.tm_min<<":"<<boardtime_tm.tm_sec;
      ss_timestamp_board << board_time.str();
    } else {
      ss_timestamp_board << "Never";
    }
    text_live_timestamp.at(board_nr)->SetText(0.06,0.8-board*0.1,ss_timestamp_board.str().c_str());
    text_live_timestamp.at(board_nr)->SetTextSize(0.05);
    text_live_timestamp.at(board_nr)->SetNDC(1);
    text_live_timestamp.at(board_nr)->Draw();
    board++;   

  }

  std::stringstream ss_path_livestatus;
  ss_path_livestatus << outpath << "LAPPDData_LiveTimestamps."<<img_extension;
  canvas_live_status->SaveAs(ss_path_livestatus.str().c_str());

}

void MonitorLAPPDDataSingle::DrawLiveTimestampEvolution(){

  Log("MonitorLAPPDDataSingle: DrawLiveTimeStampEvolution");

  //TODO: Implement some plotting function here


}
