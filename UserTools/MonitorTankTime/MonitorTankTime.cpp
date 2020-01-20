#include "MonitorTankTime.h"

MonitorTankTime::MonitorTankTime():Tool(){}


bool MonitorTankTime::Initialise(std::string configfile, DataModel &data){

  if(configfile!="")  m_variables.Initialise(configfile); //loading config file
  //m_variables.Print();
  m_data= &data; //assigning transient data pointer

  Log("Tool MonitorTankTime: Initialising....",v_message,verbosity);

  //-------------------------------------------------------
  //---------------Initialise config file------------------
  //-------------------------------------------------------

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
  m_variables.Get("UpdateFrequency",update_frequency);
  m_variables.Get("PlotConfiguration",plot_configuration);
  m_variables.Get("PathMonitoring",path_monitoring);
  m_variables.Get("ImageFormat",img_extension);
  m_variables.Get("ForceUpdate",force_update);
  m_variables.Get("DrawMarker",draw_marker);
  m_variables.Get("DrawSingle",draw_single);
  m_variables.Get("verbose",verbosity);

  if (verbosity > 2) std::cout <<"MonitorTankTime: Outpath (temporary): "<<outpath_temp<<std::endl;
  if (outpath_temp == "fromStore") m_data->CStore.Get("OutPath",outpath);
  else outpath = outpath_temp;
  if (verbosity > 2) std::cout <<"MonitorTankTime: Output path for plots is "<<outpath<<std::endl;
  if (update_frequency < 0.1) {
  if (verbosity > 0) std::cout <<"MonitorTankTime: Update Frequency of every "<<update_frequency<<" mins is too high. Setting default value of 5 mins."<<std::endl;
    update_frequency = 5.;
  }
  //Don't enable the drawing of single channel histograms/graphs by default --> too many plots!
  if (draw_single != 0 && draw_single !=1) {
    draw_single = 0;
  }
  //default should be no forced update of the monitoring plots every execute step
  if (force_update !=0 && force_update !=1) {
    force_update = 0;
  }
  //check if the image format is jpg or png
  if (!(img_extension == "png" || img_extension == "jpg" || img_extension == "jpeg")){
    img_extension = "jpg";
  }

  //-------------------------------------------------------
  //-----------------Get active slots----------------------
  //-------------------------------------------------------

  num_active_slots=0;
  num_active_slots_cr1=0;
  num_active_slots_cr2=0;
  num_active_slots_cr3=0;

  ifstream file(active_slots.c_str());
  unsigned int temp_crate, temp_slot;
  while (!file.eof()){

    file>>temp_crate>>temp_slot;
    Log("MonitorTankTime: Reading in active Slots: Crate "+std::to_string(temp_crate)+", Card "+std::to_string(temp_slot),v_message,verbosity);
    if (file.eof()) {
      if (verbosity >= 1) std::cout << std::endl;
      break;
    }
    if (temp_slot < 2 || temp_slot > num_slots_tank){
      Log("ERROR (MonitorTankTime): Specified slot "+std::to_string(temp_slot)+" out of range for VME crates [2...21]. Continue with next entry.",v_error,verbosity);
      continue;
    }
    if (!(std::find(crate_numbers.begin(),crate_numbers.end(),temp_crate)!=crate_numbers.end())) crate_numbers.push_back(temp_crate);
    std::vector<int>::iterator it = std::find(crate_numbers.begin(),crate_numbers.end(),temp_crate);
    int index = std::distance(crate_numbers.begin(), it);
    Log("MonitorTankTime: index = "+std::to_string(index),v_message,verbosity);
    num_active_slots++;
    switch (index) {
      case 0: {
        active_channel_cr1[temp_slot-1]=1;        //slot numbering starts at 1
        num_active_slots_cr1++;
        active_slots_cr1.push_back(temp_slot);
        break;
      }
      case 1: {
        active_channel_cr2[temp_slot-1]=1;
        num_active_slots_cr2++;
        active_slots_cr2.push_back(temp_slot);
        break;
      }
      case 2: {
        active_channel_cr3[temp_slot-1]=1;
        num_active_slots_cr3++;
        active_slots_cr3.push_back(temp_slot);
        break;
      }
    }
    std::vector<unsigned int> crateslot{temp_crate,temp_slot};
    map_slot_to_crateslot.emplace(num_active_slots,crateslot);
    map_crateslot_to_slot.emplace(crateslot,num_active_slots);
    for (int i_channel = 0; i_channel < num_channels_tank; i_channel++){
      std::vector<unsigned int> crateslotch{temp_crate,temp_slot,i_channel+1};
      map_ch_to_crateslotch.emplace(num_active_slots*num_channels_tank+i_channel,crateslotch);
      map_crateslotch_to_ch.emplace(crateslotch,num_active_slots*num_channels_tank+i_channel);
    }

  }
  file.close();
  num_active_slots = num_active_slots_cr1+num_active_slots_cr2+num_active_slots_cr3;

  Log("MonitorTankTime: Number of active Slots (Crate 1/2/3): "+std::to_string(num_active_slots_cr1)+" / "+std::to_string(num_active_slots_cr2)+" / "+std::to_string(num_active_slots_cr3),v_message,verbosity);
  Log("MonitorTankTime: Vector crate_numbers has size: "+std::to_string(crate_numbers.size()),v_debug,verbosity);

  //-------------------------------------------------------
  //-------------------Get geometry------------------------
  //-------------------------------------------------------

  m_data->Stores["ANNIEEvent"]->Header->Get("AnnieGeometry",geom);
  m_data->CStore.Get("PMTCrateSpaceToChannelNumMap",PMTCrateSpaceToChannelNumMap);
  Position center_position = geom->GetTankCentre();
  tank_center_x = center_position.X();
  tank_center_y = center_position.Y();
  tank_center_z = center_position.Z();

  //-------------------------------------------------------
  //----------Initialize configuration/hists---------------
  //-------------------------------------------------------

  ReadInConfiguration();
  InitializeHists();

  //-------------------------------------------------------
  //------Setup time variables for periodic updates--------
  //-------------------------------------------------------

  Epoch = new boost::posix_time::ptime(boost::gregorian::from_string(StartTime));
  period_update = boost::posix_time::time_duration(0,update_frequency,0,0);
  last = boost::posix_time::ptime(boost::posix_time::second_clock::local_time());

  //omit warning messages from ROOT: 1001 - info messages, 2001 - warnings, 3001 - errors
  gROOT->ProcessLine("gErrorIgnoreLevel = 3001;");
  
  return true;
}


bool MonitorTankTime::Execute(){

  Log("Tool MonitorTankTime: Executing ....",v_message,verbosity);

  current = (boost::posix_time::second_clock::local_time());
  duration = boost::posix_time::time_duration(current - last); 

  Log("MonitorTankTime: "+std::to_string(duration.total_milliseconds()/1000./60.)+" mins since last time plot",v_message,verbosity);

  //for testing purposes only: execute in every step / set State to DataFile
  m_data->CStore.Set("State","DataFile");

  std::string State;
  m_data->CStore.Get("State",State);

  if (State == "Wait"){

    //-------------------------------------------------------
    //--------------No tool is executed----------------------
    //-------------------------------------------------------

    Log("MonitorTankTime: State is "+State,v_debug,verbosity);

  } else if (State == "DataFile"){

    Log("MonitorTankTime: New data file available.",v_message,verbosity);

    //-------------------------------------------------------
    //--------------MonitorTankTime executed-----------------
    //-------------------------------------------------------

    //get FinishedPMTWaves from DataDecoder tools
    m_data->CStore.Get("FinishedPMTWaves",FinishedPMTWaves);
    LoopThroughDecodedEvents(FinishedPMTWaves);

    //Write the event information to a file
    //TODO: change this to a database later on!
    //Check if data has already been written included in WriteToFile function
    WriteToFile();

    //draw last file plots
    DrawLastFilePlots();

    //Draw customly defined plots
    UpdateMonitorPlots(config_timeframes, config_endtime_long, config_label, config_plottypes);

  } else {
   	Log("MonitorTankTime: State not recognized: ",v_debug,verbosity);
  }

  //-------------------------------------------------------
  //-----------Has enough time passed for update?----------
  //-------------------------------------------------------

  if(duration >= period_update){
    Log("MonitorTankTime: "+std::to_string(update_frequency)+" mins passed... Updating file history plot.",v_message,verbosity);

    last=current;
    DrawFileHistory(current_stamp,24.,"current");     //show 24h history of MRD files

  }
  
  //only for debugging memory leaks, otherwise comment out
  //std::cout <<"List of Objects (after execute step): "<<std::endl;
  //gObjectTable->Print();

  return true;

}


bool MonitorTankTime::Finalise(){

  Log("Tool MonitorTankTime: Finalising ....",v_message,verbosity);

  //delete all histograms/canvases/other objects that were created

  //help objects
  delete label_ped;
  delete label_sigma;
  delete label_rate;
  delete label_sigmadiff;
  delete label_peddiff;
  delete label_ratediff;
  delete line1;
  delete line2;
  delete text_crate1;
  delete text_crate2;
  delete text_crate3;
  delete leg_ped;
  delete leg_sigma;
  delete leg_rate;
  delete leg_freq;
  delete leg_temp;

  for (int i_box = 0; i_box < vector_box_inactive.size(); i_box++){
    delete vector_box_inactive.at(i_box);
  }

  //graphs
  for (int i_channel=0; i_channel < gr_ped.size(); i_channel++){
    delete gr_ped.at(i_channel);
    delete gr_sigma.at(i_channel);
    delete gr_rate.at(i_channel);
  }

  //multigraphs
  delete multi_ch_ped;
  delete multi_ch_sigma;
  delete multi_ch_rate;

  //histograms
  delete h2D_ped;
  delete h2D_sigma;
  delete h2D_rate; 
  delete h2D_peddiff;
  delete h2D_sigmadiff;
  delete h2D_ratediff;
  delete log_files;

  for (int i_channel = 0; i_channel < hChannels_temp.size(); i_channel++){
    delete hChannels_temp.at(i_channel);
    delete hChannels_freq.at(i_channel);
  }

  //canvases
  delete canvas_ped;
  delete canvas_sigma;
  delete canvas_rate;
  delete canvas_peddiff;
  delete canvas_sigmadiff;
  delete canvas_ratediff;
  delete canvas_logfile;
  delete canvas_ch_ped;
  delete canvas_ch_sigma;
  delete canvas_ch_rate;
  delete canvas_ch_single;

  for (int i_channel = 0; i_channel < canvas_Channels_temp.size(); i_channel++){
    delete canvas_Channels_temp.at(i_channel);
    delete canvas_Channels_freq.at(i_channel);
  }

  return true;

}

void MonitorTankTime::ReadInConfiguration(){

  Log("MonitorTankTime: ReadInConfiguration.",v_message,verbosity);

  //-------------------------------------------------------
  //----------------ReadInConfiguration -------------------
  //-------------------------------------------------------

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
        Log("ERROR (MonitorTankTime): ReadInConfiguration: Need at least 4 arguments in one line: TimeFrame - TimeEnd - FileLabel - PlotType1. Please look over the configuration file and adjust it accordingly.",v_error,verbosity);
        continue;
      }
      double double_value = std::stod(values.at(0));
      config_timeframes.push_back(double_value);
      config_endtime.push_back(values.at(1));
      config_label.push_back(values.at(2));
      std::vector<std::string> plottypes;
      for (int i=3; i < values.size(); i++){
        plottypes.push_back(values.at(i));
      }
      config_plottypes.push_back(plottypes);
    }
  } else {
    Log("ERROR (MonitorTankTime): ReadInConfiguration: Could not open file "+plot_configuration+"! Check if path is valid.",v_error,verbosity);
  }
  file.close();


  if (verbosity > 2){
    std::cout <<"---------------------------------------------------------------------"<<std::endl;
    std::cout << "MonitorTankTime: ReadInConfiguration: Read in the following data into configuration variables: "<<std::endl;
    for (int i_t=0; i_t < config_timeframes.size(); i_t++){
      std::cout <<config_timeframes.at(i_t)<<", "<<config_endtime.at(i_t)<<", "<<config_label.at(i_t)<<", ";
      for (int i_plot = 0; i_plot < config_plottypes.at(i_t).size(); i_plot++){
        std::cout <<config_plottypes.at(i_t).at(i_plot)<<", ";
      }
      std::cout<<std::endl;
    }
    std::cout <<"-----------------------------------------------------------------------"<<std::endl;
  }

  Log("MonitorTankTime: ReadInConfiguration: Parsing dates: ",v_message,verbosity);
  for (int i_date = 0; i_date < config_endtime.size(); i_date++){
    if (config_endtime.at(i_date) == "TEND_LASTFILE") {
      Log("MonitorTankTime: TEND_LASTFILE: Starting from end of last read-in file",v_message,verbosity);
      ULong64_t zero = 0; 
      config_endtime_long.push_back(zero);
    } else if (config_endtime.at(i_date).size()==15){
        boost::posix_time::ptime spec_endtime(boost::posix_time::from_iso_string(config_endtime.at(i_date)));
        boost::posix_time::time_duration spec_endtime_duration = boost::posix_time::time_duration(spec_endtime - *Epoch);
        ULong64_t spec_endtime_long = spec_endtime_duration.total_milliseconds();
        config_endtime_long.push_back(spec_endtime_long);
    } else {
      Log("MonitorTankTime: Specified end date "+config_endtime.at(i_date)+" does not have the desired format yyyymmddThhmmss. Please change the format in the config file in order to use this tool. Starting from end of last file",v_message,verbosity);
      ULong64_t zero = 0;
      config_endtime_long.push_back(zero);
    }
  }


}

void MonitorTankTime::InitializeHists(){

  Log("MonitorTankTime: Initialize Hists",v_message,verbosity);

  //-------------------------------------------------------
  //----------------InitializeHists -----------------------
  //-------------------------------------------------------

  std::vector<double> empty_vec;
  readfromfile_tend = 0;
  readfromfile_timeframe = 0.;

  str_ped = " Pedestal Mean (VME)";
  str_sigma = " Pedestal Sigma (VME)";
  str_rate = " Rate (VME)";
  str_peddiff = " Pedestal Difference";
  str_sigmadiff = " Sigma Difference";
  str_ratediff = " Rate Difference";
  crate_str = "cr";
  slot_str = "_slot";
  ch_str = "_ch";

  ss_title_ped << title_time.str() << str_ped;
  ss_title_sigma << title_time.str() << str_sigma;
  ss_title_rate << title_time.str() << str_rate;
  ss_title_peddiff << title_time.str() << str_peddiff;
  ss_title_sigmadiff << title_time.str() << str_sigmadiff;
  ss_title_ratediff << title_time.str() << str_ratediff;

  h2D_ped = new TH2F("h2D_ped",ss_title_ped.str().c_str(),num_slots_tank,0,num_slots_tank,num_crates_tank*num_channels_tank,0,num_crates_tank*num_channels_tank);                     //Fitted gauss ADC distribution mean in 2D representation of channels, slots
  h2D_sigma = new TH2F("h2D_sigma",ss_title_sigma.str().c_str(),num_slots_tank,0,num_slots_tank,num_crates_tank*num_channels_tank,0,num_crates_tank*num_channels_tank);               //Fitted gauss ADC distribution sigma in 2D representation of channels, slots
  h2D_rate = new TH2F("h2D_rate",ss_title_rate.str().c_str(),num_slots_tank,0,num_slots_tank,num_crates_tank*num_channels_tank,0,num_crates_tank*num_channels_tank);                  //Rate in 2D representation of channels, slots
  h2D_peddiff = new TH2F("h2D_peddiff",ss_title_peddiff.str().c_str(),num_slots_tank,0,num_slots_tank,num_crates_tank*num_channels_tank,0,num_crates_tank*num_channels_tank);         //Time difference of fitted pedestal mean values of all PMT channels (in percent)
  h2D_sigmadiff = new TH2F("h2D_sigmadiff",ss_title_sigmadiff.str().c_str(),num_slots_tank,0,num_slots_tank,num_crates_tank*num_channels_tank,0,num_crates_tank*num_channels_tank);   //Time difference of fitted sigma values of all PMT channels (in percent)
  h2D_ratediff = new TH2F("h2D_ratediff",ss_title_ratediff.str().c_str(),num_slots_tank,0,num_slots_tank,num_crates_tank*num_channels_tank,0,num_crates_tank*num_channels_tank);      //Time difference of rate values of all PMT channels (in percent)

  canvas_ped = new TCanvas("canvas_ped","Pedestal Mean (VME)",900,600);
  canvas_sigma = new TCanvas("canvas_sigma","Pedestal Sigma (VME)",900,600);
  canvas_rate = new TCanvas("canvas_rate","Signal Counts (VME)",900,600);
  canvas_peddiff = new TCanvas("canvas_peddiff","Pedestal Time Difference (VME)",900,600);
  canvas_sigmadiff = new TCanvas("canvas_sigmadiff","Sigma Time Difference (VME)",900,600);
  canvas_ratediff = new TCanvas("canvas_ratediff","Rate Time Difference (VME)",900,600);
  canvas_ch_ped = new TCanvas("canvas_ch_ped","Channel Ped Canvas",900,600);
  canvas_ch_sigma = new TCanvas("canvas_ch_sigma","Channel Sigma Canvas",900,600);
  canvas_ch_rate = new TCanvas("canvas_ch_rate","Channel Rate Canvas",900,600);
  canvas_ch_single = new TCanvas("canvas_ch_single","Channel Canvas Single",900,600);
 
  for (int i_active = 0; i_active<num_active_slots; i_active++){
    
    std::vector<unsigned int> crateslot = map_slot_to_crateslot[i_active];
    unsigned int crate_num = crateslot.at(0);
    unsigned int slot_num = crateslot.at(1);

    std::stringstream ss_name_hist;
    std::stringstream ss_name_hist_temp;
    std::stringstream ss_title_hist;
    std::stringstream ss_name_canvas;
    std::stringstream ss_name_canvas_temp;
    std::stringstream ss_title_canvas;
    std::string crate_str="cr";
    std::string slot_str = "_slot";
    std::string ch_str = "_ch";
    std::string canvas_str = "canvas_";

    ss_name_canvas <<canvas_str<<crate_str<<crate_num<<slot_str<<slot_num;
    ss_name_canvas_temp << ss_name_canvas.str() << "_temp";
    ss_title_canvas << "Crate" <<crate_str<<"_Slot"<<slot_str;
    TCanvas *canvas_Channel_temp = new TCanvas(ss_name_canvas_temp.str().c_str(),ss_title_canvas.str().c_str(),900,600);
    canvas_Channels_temp.push_back(canvas_Channel_temp);
    TCanvas *canvas_Channel = new TCanvas(ss_name_canvas.str().c_str(),ss_title_canvas.str().c_str(),900,600);
    canvas_Channels_freq.push_back(canvas_Channel);

    //create frequency + buffer histograms
    for (int i_channel = 0; i_channel < num_channels_tank; i_channel++){

      ss_name_hist.str("");
      ss_title_hist.str("");
      ss_name_hist<<crate_str<<crate_num<<slot_str<<slot_num<<ch_str<<i_channel;
      ss_title_hist << title_time.str() << " ADC Freq (VME Crate " << crate_num << " Slot " << slot_num <<")";
      
      TH1I *hChannel_freq = new TH1I(ss_name_hist.str().c_str(),ss_title_hist.str().c_str(),maximum_adc-minimum_adc,minimum_adc,maximum_adc);   //ADC pulse shapes for current event, 1 canvas per slot
      hChannel_freq->GetXaxis()->SetTitle("ADC");
      hChannel_freq->GetYaxis()->SetTitle("Counts");
      hChannel_freq->SetLineWidth(2);
      hChannel_freq->SetLineColor(i_channel+1);
      hChannel_freq->SetStats(0);
      hChannel_freq->GetYaxis()->SetTitleOffset(1.35);
      hChannels_freq.push_back(hChannel_freq);

      ss_name_hist_temp.str("");
      ss_title_hist_temp.str("");
      ss_name_hist_temp<<crate_str<<crate_num<<slot_str<<slot_num<<ch_str<<i_channel<<"_temp";
      ss_title_hist_temp << title_time.str() << " Buffer Temp (VME Crate " << crate_num << " Slot " << slot_num <<")";
      TH1F* hChannel_temp = new TH1F(ss_name_hist_temp.str().c_str(),ss_title_hist_temp.str().c_str(),4000,0,4000);   //default buffer size for initializing, will be readjusted according to read-out size
      hChannel_temp->GetXaxis()->SetTitle("Buffer Position");
      hChannel_temp->GetYaxis()->SetTitle("Volts");
      hChannel_temp->SetLineWidth(2);
      hChannel_temp->SetLineColor(i_channel+1);
      hChannel_temp->GetYaxis()->SetTitleOffset(1.35);
      hChannel_temp->SetStats(0);
      hChannels_temp.push_back(hChannel_temp);

      channels_mean.push_back(0);                             //starting mean/Sigma values for all channels set to 0, will be filled with the fit values from Gaussian fit
      channels_sigma.push_back(0);

    }
  }

  canvas_Channels_freq.at(num_active_slots-1)->Clear();       //this canvas gets otherwise drawn with an additional frequency histogram (last channel)

  //initialize file history histogram and canvas
  num_files_history = 10;
  log_files = new TH1F("log_files","MRD Files History",num_files_history,0,num_files_history);
  log_files->GetXaxis()->SetTimeDisplay(1);
  log_files->GetXaxis()->SetLabelSize(0.03);
  log_files->GetXaxis()->SetLabelOffset(0.03);
  log_files->GetXaxis()->SetTimeFormat("#splitline{%m/%d}{%H:%M}");
  log_files->GetYaxis()->SetTickLength(0.);
  log_files->GetYaxis()->SetLabelOffset(999);
  log_files->SetStats(0);

  //initialize vector inactive boxes
  for (int i_slot = 0; i_slot < num_slots_tank*num_crates_tank; i_slot++){
    int slot_in_crate;
    int crate_nr;
    if (i_slot < num_slots_tank) {slot_in_crate = i_slot; crate_nr=0;}
    else if (i_slot < 2*num_slots_tank) {slot_in_crate = i_slot -num_slots_tank; crate_nr=1;}
    else {slot_in_crate = i_slot - 2*num_slots_tank; crate_nr=2;}

    if (active_channel_cr1[slot_in_crate] == 0 && crate_nr==0) {
      TBox *box_inactive = new TBox(slot_in_crate,2*num_channels_tank,slot_in_crate+1,3*num_channels_tank);
      box_inactive->SetFillStyle(3004);
      box_inactive->SetFillColor(1);
      vector_box_inactive.push_back(box_inactive);
    }
    if (active_channel_cr2[slot_in_crate] == 0 && crate_nr==1) {
      TBox *box_inactive = new TBox(slot_in_crate,num_channels_tank,slot_in_crate+1,2*num_channels_tank);
      box_inactive->SetFillStyle(3004);
      box_inactive->SetFillColor(1);
      vector_box_inactive.push_back(box_inactive);
    }
    if (active_channel_cr3[slot_in_crate] == 0 && crate_nr==2) {
      TBox *box_inactive = new TBox(slot_in_crate,0,slot_in_crate+1,num_channels_tank);
      box_inactive->SetFillStyle(3004);
      box_inactive->SetFillColor(1);
      vector_box_inactive.push_back(box_inactive);
    }
  }

  //initialize labels/text boxes/etc
  label_rate = new TLatex(0.905,0.92,"Rate [Hz]");
  label_rate->SetNDC(1);
  label_rate->SetTextSize(0.030);
  label_ped = new TLatex(0.905,0.92,"#mu_{Ped}");
  label_ped->SetNDC(1);
  label_ped->SetTextSize(0.030);
  label_sigma = new TLatex(0.905,0.92,"#sigma_{Ped}");
  label_sigma->SetNDC(1);
  label_sigma->SetTextSize(0.030);
  label_peddiff = new TLatex(0.905,0.92,"#Delta #mu_{Ped}");
  label_peddiff->SetNDC(1);
  label_peddiff->SetTextSize(0.030);
  label_sigmadiff = new TLatex(0.905,0.92,"#Delta #sigma_{Ped}");
  label_sigmadiff->SetNDC(1);
  label_sigmadiff->SetTextSize(0.030);
  label_ratediff = new TLatex(0.905,0.92,"#Delta R [Hz]");
  label_ratediff->SetNDC(1);
  label_ratediff->SetTextSize(0.030);
  std::stringstream ss_crate1, ss_crate2, ss_crate3;
  ss_crate1 << "ANNIEVME0"<<crate_numbers.at(0);
  ss_crate2 << "ANNIEVME0"<<crate_numbers.at(1);
  ss_crate3 << "ANNIEVME0"<<crate_numbers.at(2);
  text_crate1 = new TText(0.04,0.68,ss_crate1.str().c_str());
  text_crate1->SetNDC(1);
  text_crate1->SetTextSize(0.030);
  text_crate1->SetTextAngle(90.);
  text_crate2 = new TText(0.04,0.41,ss_crate2.str().c_str());
  text_crate2->SetNDC(1);
  text_crate2->SetTextSize(0.030);
  text_crate2->SetTextAngle(90.);
  text_crate3 = new TText(0.04,0.14,ss_crate3.str().c_str());
  text_crate3->SetNDC(1);
  text_crate3->SetTextSize(0.030);
  text_crate3->SetTextAngle(90.);
  line1 = new TLine(-1,4,num_slots_tank,4.);
  line1->SetLineWidth(2);
  line2 = new TLine(-1,8.,num_slots_tank,8.);
  line2->SetLineWidth(2);
  leg_freq = new TLegend(0.75,0.7,0.9,0.9);
  leg_temp = new TLegend(0.75,0.7,0.9,0.9);


  // Initialize time evolution graphs

  for (int i_slot = 0; i_slot < num_active_slots; i_slot++){

    std::stringstream ss_crate, ss_slot;

    unsigned int crate_num = map_slot_to_crateslot[i_slot].at(0);
    unsigned int slot_num = map_slot_to_crateslot[i_slot].at(1);
    ss_crate << crate_num;
    ss_slot << slot_num;

    for (int i_channel = 0; i_channel < num_channels_tank; i_channel++){

      std::stringstream ss_ch;
      ss_ch << i_channel+1;
      int line_color_ch = i_channel+1;

      ped_file.push_back(empty_vec);

      //channel-wise graphs
      std::stringstream name_graph, name_graph_ped, name_graph_sigma, name_graph_rate;
      std::stringstream title_graph, title_graph_ped, title_graph_sigma, title_graph_rate;

      name_graph << "cr"<<ss_crate.str()<<"_sl"<<ss_slot.str()<<"_ch"<<ss_ch.str();
      name_graph_ped << name_graph.str()<<"_ped";
      name_graph_sigma << name_graph.str()<<"_sigma";
      name_graph_rate << name_graph.str()<<"_rate";
      title_graph << "Crate "<<ss_crate.str()<<", Slot "<<ss_slot.str()<<", Channel "<<ss_ch.str();
      title_graph_ped << title_graph.str() << " (Pedestal)";
      title_graph_sigma << title_graph.str() <<" (Ped Sigma)";
      title_graph_rate << title_graph.str() <<" (Rate)";

      TGraph *graph_ch_ped = new TGraph();
      TGraph *graph_ch_sigma = new TGraph();
      TGraph *graph_ch_rate = new TGraph();

      graph_ch_ped->SetName(name_graph_ped.str().c_str());
      graph_ch_ped->SetTitle(title_graph_ped.str().c_str());
      graph_ch_sigma->SetName(name_graph_sigma.str().c_str());
      graph_ch_sigma->SetTitle(title_graph_sigma.str().c_str());
      graph_ch_rate->SetName(name_graph_rate.str().c_str());
      graph_ch_rate->SetTitle(title_graph_rate.str().c_str());
      
      if (draw_marker) {
        graph_ch_ped->SetMarkerStyle(20);
        graph_ch_sigma->SetMarkerStyle(20);
        graph_ch_rate->SetMarkerStyle(20);
      }

      graph_ch_ped->SetMarkerColor(line_color_ch);
      graph_ch_sigma->SetMarkerColor(line_color_ch);
      graph_ch_rate->SetMarkerColor(line_color_ch);
      graph_ch_sigma->SetMarkerColor(line_color_ch);
      graph_ch_rate->SetMarkerColor(line_color_ch);
      graph_ch_ped->SetLineColor(line_color_ch);
      graph_ch_ped->SetLineWidth(2);
      graph_ch_ped->SetFillColor(0);
      graph_ch_ped->GetYaxis()->SetTitle("Pedestal");
      graph_ch_ped->GetXaxis()->SetTimeDisplay(1);
      graph_ch_ped->GetXaxis()->SetLabelSize(0.03);
      graph_ch_ped->GetXaxis()->SetLabelOffset(0.03);
      graph_ch_ped->GetXaxis()->SetTimeFormat("#splitline{%m/%d}{%H:%M}");
      graph_ch_sigma->SetLineColor(line_color_ch);
      graph_ch_sigma->SetLineWidth(2);
      graph_ch_sigma->SetFillColor(0);
      graph_ch_sigma->GetYaxis()->SetTitle("Sigma (Ped)");
      graph_ch_sigma->GetXaxis()->SetTimeDisplay(1);
      graph_ch_sigma->GetXaxis()->SetLabelSize(0.03);
      graph_ch_sigma->GetXaxis()->SetLabelOffset(0.03);
      graph_ch_sigma->GetXaxis()->SetTimeFormat("#splitline{%m/%d}{%H:%M}");
      graph_ch_rate->SetLineColor(line_color_ch);
      graph_ch_rate->SetLineWidth(2);
      graph_ch_rate->SetFillColor(0);
      graph_ch_rate->GetYaxis()->SetTitle("Rate [Hz]");
      graph_ch_rate->GetXaxis()->SetTimeDisplay(1);
      graph_ch_rate->GetXaxis()->SetLabelSize(0.03);
      graph_ch_rate->GetXaxis()->SetLabelOffset(0.03);
      graph_ch_rate->GetXaxis()->SetTimeFormat("#splitline{%m/%d}{%H:%M}");

      gr_ped.push_back(graph_ch_ped);
      gr_sigma.push_back(graph_ch_sigma);
      gr_rate.push_back(graph_ch_rate);
    }
  }

  multi_ch_ped = new TMultiGraph();
  multi_ch_sigma = new TMultiGraph();
  multi_ch_rate = new TMultiGraph();
  leg_ped = new TLegend(0.7,0.7,0.88,0.88);
  leg_sigma = new TLegend(0.7,0.7,0.88,0.88);
  leg_rate = new TLegend(0.7,0.7,0.88,0.88);
  leg_ped->SetLineColor(0);
  leg_sigma->SetLineColor(0);
  leg_rate->SetLineColor(0);

}

void MonitorTankTime::LoopThroughDecodedEvents(std::map<uint64_t, std::map<std::vector<int>, std::vector<uint16_t>>> finishedPMTWaves){

  Log("MonitorTankTime: LoopThroughDecodedEvents",v_message,verbosity);

  //-------------------------------------------------------
  //-------------LoopThroughDecodedEvents -----------------
  //-------------------------------------------------------

  int i_timestamp = 0;
  for (std::map<uint64_t, std::map<std::vector<int>, std::vector<uint16_t>>>::iterator it = finishedPMTWaves.begin(); it != finishedPMTWaves.end(); it++){

    std::cout <<"i_timestamp = "<<i_timestamp<<std::endl;
    uint64_t timestamp = it->first;
    timestamp_file.push_back(timestamp);
    std::vector<double> ped_file_temp, sigma_file_temp, rate_file_temp;
    ped_file_temp.assign(num_active_slots*num_channels_tank,0.);
    sigma_file_temp.assign(num_active_slots*num_channels_tank,0.);
    rate_file_temp.assign(num_active_slots*num_channels_tank,0.);

    std::map<std::vector<int>, std::vector<uint16_t>> afinishedPMTWaves = finishedPMTWaves.at(timestamp);
    for(std::pair<std::vector<int>, std::vector<uint16_t>> apair : afinishedPMTWaves){

      int CardID = apair.first.at(0);
      int ChannelID = apair.first.at(1);
      std::vector<uint16_t> awaveform = apair.second;
      int CrateNum, SlotNum;
      this->CardIDToElectronicsSpace(CardID, CrateNum, SlotNum);
      std::vector<unsigned int> CrateSlot{CrateNum,SlotNum};
      //check if read out crate/slot configuration is active according to configuration file...
      if (map_crateslot_to_slot.find(CrateSlot) == map_crateslot_to_slot.end()){
        Log("MonitorTankTime ERROR: Slot read out from data (Cr"+std::to_string(CrateNum)+"/Sl"+std::to_string(SlotNum)+") should not be active according to config file. Check config file...",v_error,verbosity);
      }
      int i_slot = map_crateslot_to_slot[CrateSlot];
      int i_channel = i_slot*num_channels_tank+ChannelID-1;
      hChannels_freq.at(i_channel)->Reset();            //only show the most recent plot for each PMT
      hChannels_temp.at(i_channel)->Reset();            //only show the most recent plot for each PMT
      hChannels_temp.at(i_channel)->SetBins(awaveform.size(),0,awaveform.size());
      
      //fill frequency plots
      long sum = 0;
      for (int i_buffer = 0; i_buffer < awaveform.size(); i_buffer++){
        hChannels_freq.at(i_channel)->Fill(awaveform.at(i_buffer));
        if (awaveform.at(i_buffer) > channels_mean[i_channel]+5*channels_sigma[i_channel]) sum+= awaveform.at(i_buffer);
      }

      Log("MonitorTankTime: Number of hits for channel # "+std::to_string(i_channel)+": "+std::to_string(sum),v_message,verbosity);
      channels_rate.at(i_channel) = sum;	//actually this is just the number of signal counts, convert to a rate later on

      //fit pedestal values with Gaussian
      TFitResultPtr gaussFitResult = hChannels_freq.at(i_channel)->Fit("gaus","Q");
      Int_t gaussFitResultInt = gaussFitResult;
      if (gaussFitResultInt == 0){            //status variable 0 means the fit was ok
        TF1 *gaus = (TF1*) hChannels_freq.at(i_channel)->GetFunction("gaus");
        std::stringstream ss_gaus;
        ss_gaus<<"gaus_"<<i_timestamp<<"_"<<i_channel;
        gaus->SetName(ss_gaus.str().c_str());
        channels_mean.at(i_channel) = gaus->GetParameter(1);
        channels_sigma.at(i_channel) = gaus->GetParameter(2);
        vector_gaus.push_back(gaus);          //keep track of TF1s so they can be deleted later
      }

      //fill buffer plots
      for (int i_buffer = 0; i_buffer < awaveform.size(); i_buffer++){
        hChannels_temp.at(i_channel)->SetBinContent(i_buffer,(awaveform.at(i_buffer)-channels_mean.at(i_channel))*conversion_ADC_Volt);
      }

      ped_file_temp.at(i_channel) = channels_mean.at(i_channel);
      sigma_file_temp.at(i_channel) = channels_sigma.at(i_channel);
      rate_file_temp.at(i_channel) = channels_rate.at(i_channel);
     
    }

    ped_file.push_back(ped_file_temp);
    sigma_file.push_back(sigma_file_temp);
    rate_file.push_back(rate_file_temp);
    i_timestamp++;   
 
  }

  //fill latest mean/sigma/rate value of each PMT into a storing vector

}

void MonitorTankTime::WriteToFile(){

  Log("MonitorTankTime: WriteToFile",v_message,verbosity);

  //-------------------------------------------------------
  //------------------WriteToFile -------------------------
  //-------------------------------------------------------

  std::string file_start_date = convertTimeStamp_to_Date(t_file_start);
  std::stringstream root_filename;
  root_filename << path_monitoring << "PMT_" << file_start_date <<".root";
  
  Log("MonitorTankTime: ROOT filename: "+root_filename.str(),v_message,verbosity);

  std::string root_option = "RECREATE";
  if (does_file_exist(root_filename.str())) root_option = "UPDATE";
  TFile *f = new TFile(root_filename.str().c_str(),root_option.c_str());

  ULong64_t t_start, t_end, t_frame;
  std::vector<unsigned int> *crate = new std::vector<unsigned int>;
  std::vector<unsigned int> *slot = new std::vector<unsigned int>;
  std::vector<unsigned int> *channel = new std::vector<unsigned int>;
  std::vector<double> *ped = new std::vector<double>;
  std::vector<double> *sigma = new std::vector<double>;
  std::vector<double> *rate = new std::vector<double>;
  std::vector<int> *channelcount = new std::vector<int>;

  TTree *t;
  if (f->GetListOfKeys()->Contains("tankmonitor_tree")) {
    Log("MonitorTankTime: WriteToFile: Tree already exists",v_message,verbosity);
    t = (TTree*) f->Get("tankmonitor_tree");
    t->SetBranchAddress("t_start",&t_start);
    t->SetBranchAddress("t_end",&t_end);
    t->SetBranchAddress("crate",&crate);
    t->SetBranchAddress("slot",&slot);
    t->SetBranchAddress("channel",&channel);
    t->SetBranchAddress("ped",&ped);
    t->SetBranchAddress("sigma",&sigma);
    t->SetBranchAddress("rate",&rate);
    t->SetBranchAddress("channelcount",&channelcount);
  } else {
    t = new TTree("tankmonitor_tree","Tank Monitoring tree");
    Log("MonitorTankTime: WriteToFile: Tree is created from scratch",v_message,verbosity);
    t->Branch("t_start",&t_start);
    t->Branch("t_end",&t_end);
    t->Branch("crate",&crate);
    t->Branch("slot",&slot);
    t->Branch("channel",&channel);
    t->Branch("ped",&ped);
    t->Branch("sigma",&sigma);
    t->Branch("rate",&rate);
    t->Branch("channelcount",&channelcount);
  }

  int n_entries = t->GetEntries();
  bool omit_entries = false;
  for (int i_entry = 0; i_entry < n_entries; i_entry++){
    t->GetEntry(i_entry);
    if (t_start == t_file_start) {
      Log("WARNING (MonitorTankTime): WriteToFile: Wanted to write data from file that is already written to DB. Omit entries",v_warning,verbosity);
      omit_entries = true;
    }
  }
  //if data is already written to DB/File, do not write it again
  if (omit_entries) {

    //don't write file again, but still delete TFile and TTree object!!!
    f->Close();
    delete f;
    return;

  } 

  crate->clear();
  slot->clear();
  channel->clear();
  ped->clear();
  sigma->clear();
  rate->clear();
  channelcount->clear();

  /*t_start = t_file_start;
  t_end = t_file_end;
  t_frame = t_end - t_start;
  rate_zerohits = n_zerohits/(t_frame/MSEC_to_SEC);
  rate_normalhit = n_normalhits/(t_frame/MSEC_to_SEC);
  rate_noloopback = n_noloopback/(t_frame/MSEC_to_SEC);*/

  boost::posix_time::ptime starttime = *Epoch + boost::posix_time::time_duration(int(t_start/MSEC_to_SEC/SEC_to_MIN/MIN_to_HOUR),int(t_start/MSEC_to_SEC/SEC_to_MIN)%60,int(t_start/MSEC_to_SEC/1000.)%60,t_start%1000);
  struct tm starttime_tm = boost::posix_time::to_tm(starttime);
  boost::posix_time::ptime endtime = *Epoch + boost::posix_time::time_duration(int(t_end/MSEC_to_SEC/SEC_to_MIN/MIN_to_HOUR),int(t_end/MSEC_to_SEC/SEC_to_MIN)%60,int(t_end/MSEC_to_SEC/1000.)%60,t_end%1000);
  struct tm endtime_tm = boost::posix_time::to_tm(endtime);
  Log("MonitorTankTime: WriteToFile: Writing data to file: "+std::to_string(starttime_tm.tm_year+1900)+"/"+std::to_string(starttime_tm.tm_mon+1)+"/"+std::to_string(starttime_tm.tm_mday)+"-"+std::to_string(starttime_tm.tm_hour)+":"+std::to_string(starttime_tm.tm_min)+":"+std::to_string(starttime_tm.tm_sec)
    +"..."+std::to_string(endtime_tm.tm_year+1900)+"/"+std::to_string(endtime_tm.tm_mon+1)+"/"+std::to_string(endtime_tm.tm_mday)+"-"+std::to_string(endtime_tm.tm_hour)+":"+std::to_string(endtime_tm.tm_min)+":"+std::to_string(endtime_tm.tm_sec),v_message,verbosity);

 t_frame = 5*60*1000;	//set delta t to five minutes for testing purposes

  for (int i_channel = 0; i_channel < num_active_slots*num_channels_tank; i_channel++){
    std::vector<unsigned int> crateslotch_temp = map_ch_to_crateslotch[i_channel];
    unsigned int crate_temp = crateslotch_temp.at(0);
    unsigned int slot_temp = crateslotch_temp.at(1);
    unsigned int channel_temp = crateslotch_temp.at(2);    //channel numbering goes from 1-4
    double rate_temp = ped_file.at(i_channel).size() / (t_frame/MSEC_to_SEC);
    double mean_ped = 0.;
    double sigma_temp = 0.;
    for (int i_ped = 0; i_ped < ped_file.at(i_channel).size(); i_ped++){
      mean_ped+=ped_file.at(i_channel).at(i_ped);
    }
    if (ped_file.at(i_channel).size() > 0) {
      mean_ped/=ped_file.at(i_channel).size();
      for (int i_ped = 0.; i_ped < ped_file.at(i_channel).size(); i_ped++){
        sigma_temp += pow((ped_file.at(i_channel).at(i_ped)-mean_ped),2);
      }
      sigma_temp=sqrt(sigma_temp);
      sigma_temp/=ped_file.at(i_channel).size();
    } else {
      sigma_temp = 0.;
    }

    crate->push_back(crate_temp);
    slot->push_back(slot_temp);
    channel->push_back(channel_temp);
    ped->push_back(mean_ped);
    sigma->push_back(sigma_temp);
    rate->push_back(rate_temp);
    channelcount->push_back(ped_file.at(i_channel).size());
  }
/*
  if (fabs(t_frame) > 0.1) {
    rate_beam = n_beam/(t_frame/MSEC_to_SEC);
    rate_cosmic = n_cosmic/(t_frame/MSEC_to_SEC);
  } else {
    rate_beam = 0.;
    rate_cosmic = 0.;
  }*/

  t->Fill();
  t->Write("",TObject::kOverwrite);           //prevent ROOT from making endless keys for the same tree when updating the tree
  f->Close();

  delete crate;
  delete slot;
  delete channel;
  delete ped;
  delete sigma;
  delete rate;
  delete channelcount;
  delete f;     //tree should get deleted automatically by closing file

}

void MonitorTankTime::ReadFromFile(ULong64_t timestamp_end, double time_frame){
  
  Log("MonitorTankTime: ReadFromFile",v_message,verbosity);

  //-------------------------------------------------------
  //------------------ReadFromFile ------------------------
  //-------------------------------------------------------

  ped_plot.clear();
  sigma_plot.clear();
  rate_plot.clear();
  channelcount_plot.clear();
  tstart_plot.clear();
  tend_plot.clear();
  labels_timeaxis.clear();

  //take the end time and calculate the start time with the given time_frame
  ULong64_t timestamp_start = timestamp_end - time_frame*MIN_to_HOUR*SEC_to_MIN*MSEC_to_SEC;

  boost::posix_time::ptime starttime = *Epoch + boost::posix_time::time_duration(int(timestamp_start/MSEC_to_SEC/SEC_to_MIN/MIN_to_HOUR),int(timestamp_start/MSEC_to_SEC/SEC_to_MIN)%60,int(timestamp_start/MSEC_to_SEC/1000.)%60,timestamp_start%1000);
  struct tm starttime_tm = boost::posix_time::to_tm(starttime);
  boost::posix_time::ptime endtime = *Epoch + boost::posix_time::time_duration(int(timestamp_end/MSEC_to_SEC/SEC_to_MIN/MIN_to_HOUR),int(timestamp_end/MSEC_to_SEC/SEC_to_MIN)%60,int(timestamp_end/MSEC_to_SEC/1000.)%60,timestamp_end%1000);
  struct tm endtime_tm = boost::posix_time::to_tm(endtime);

   Log("MonitorTankTime: ReadFromFile: Reading in data for time frame "+std::to_string(starttime_tm.tm_year+1900)+"/"+std::to_string(starttime_tm.tm_mon+1)+"/"+std::to_string(starttime_tm.tm_mday)+"-"+std::to_string(starttime_tm.tm_hour)+":"+std::to_string(starttime_tm.tm_min)+":"+std::to_string(starttime_tm.tm_sec)
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
    root_filename_i << path_monitoring << "PMT_" << string_date_i <<".root";
    bool tree_exists = true;

    if (does_file_exist(root_filename_i.str())) {
      TFile *f = new TFile(root_filename_i.str().c_str(),"READ");
      TTree *t;
      if (f->GetListOfKeys()->Contains("tankmonitor_tree")) t = (TTree*) f->Get("tankmonitor_tree");
      else { 
        Log("WARNING (MonitorTankTime): File "+root_filename_i.str()+" does not contain tankmonitor_tree. Omit file.",v_warning,verbosity);
        tree_exists = false;
      }

      if (tree_exists){

        Log("MonitorTankTime: Tree exists, start reading in data",v_message,verbosity);

        ULong64_t t_start, t_end;
        std::vector<unsigned int> *crate = new std::vector<unsigned int>;
        std::vector<unsigned int> *slot = new std::vector<unsigned int>;
        std::vector<unsigned int> *channel = new std::vector<unsigned int>;
        std::vector<double> *ped = new std::vector<double>;
        std::vector<double> *sigma = new std::vector<double>;
        std::vector<double> *rate = new std::vector<double>;
        std::vector<int> *channelcount = new std::vector<int>;

        int nevents;
        int nentries_tree;

        t->SetBranchAddress("t_start",&t_start);
        t->SetBranchAddress("t_end",&t_end);
        t->SetBranchAddress("crate",&crate);
        t->SetBranchAddress("slot",&slot);
        t->SetBranchAddress("channel",&channel);
        t->SetBranchAddress("ped",&ped);
        t->SetBranchAddress("sigma",&sigma);
        t->SetBranchAddress("rate",&rate);
        t->SetBranchAddress("channelcount",&channelcount);

        nentries_tree = t->GetEntries();

        for (int i_entry = 0; i_entry < nentries_tree; i_entry++){

          t->GetEntry(i_entry);
          if (t_start >= timestamp_start && t_end <= timestamp_end){
            ped_plot.push_back(*ped);
            sigma_plot.push_back(*sigma);
            rate_plot.push_back(*rate);
            channelcount_plot.push_back(*channelcount);
            tstart_plot.push_back(t_start);
            tend_plot.push_back(t_end);
            boost::posix_time::ptime boost_tend = *Epoch+boost::posix_time::time_duration(int(t_end/MSEC_to_SEC/SEC_to_MIN/MIN_to_HOUR),int(t_end/MSEC_to_SEC/SEC_to_MIN)%60,int(t_end/MSEC_to_SEC/1000.)%60,t_end%1000);
            struct tm label_timestamp = boost::posix_time::to_tm(boost_tend);
            
            TDatime datime_timestamp(1900+label_timestamp.tm_year,label_timestamp.tm_mon+1,label_timestamp.tm_mday,label_timestamp.tm_hour,label_timestamp.tm_min,label_timestamp.tm_sec);
            labels_timeaxis.push_back(datime_timestamp);
          }

        }

        delete crate;
        delete slot;
        delete channel;
        delete ped;
        delete sigma;
        delete rate;
        delete channelcount;

      }

      f->Close();
      delete f;

    } else {
      Log("MonitorTankTime: ReadFromFile: File "+root_filename_i.str()+" does not exist. Omit file.",v_warning,verbosity);
    }

  }

  //Set the readfromfile time variables to make sure data is not read twice for the same time window
  readfromfile_tend = timestamp_end;
  readfromfile_timeframe = time_frame;

}

void MonitorTankTime::DrawLastFilePlots(){

  Log("MonitorTankTime: DrawLastFilePlots",v_message,verbosity);

  //-------------------------------------------------------
  //------------------DrawLastFilePlots -------------------
  //-------------------------------------------------------

  //draw time buffer plots
  DrawBufferPlots();

  //draw ADC frequency plots
  DrawADCFreqPlots();

  //Draw ped plots plots
  DrawPedPlotElectronics(t_file_end,(t_file_end-t_file_start)/MSEC_to_SEC/SEC_to_MIN/MIN_to_HOUR,"lastFile");
  //DrawPedPlotPhysical(t_file_end,(t_file_end-t_file_start)/MSEC_to_SEC/SEC_to_MIN/MIN_to_HOUR,"lastFile");

  //Draw ped Sigma plots
  DrawSigmaPlotElectronics(t_file_end,(t_file_end-t_file_start)/MSEC_to_SEC/SEC_to_MIN/MIN_to_HOUR,"lastFile");
  //DrawSigmaPlotPhysical(t_file_end,(t_file_end-t_file_start)/MSEC_to_SEC/SEC_to_MIN/MIN_to_HOUR,"lastFile");

  //Draw rate plots
  DrawRatePlotElectronics(t_file_end,(t_file_end-t_file_start)/MSEC_to_SEC/SEC_to_MIN/MIN_to_HOUR,"lastFile");
  //DrawRatePlotPhysical(t_file_end,(t_file_end-t_file_start)/MSEC_to_SEC/SEC_to_MIN/MIN_to_HOUR,"lastFile");

}

void MonitorTankTime::UpdateMonitorPlots(std::vector<double> timeFrames, std::vector<ULong64_t> endTimes, std::vector<std::string> fileLabels, std::vector<std::vector<std::string>> plotTypes){

  Log("MonitorTankTime: UpdateMonitorPlots",v_message,verbosity);

  //-------------------------------------------------------
  //------------------UpdateMonitorPlots ------------------
  //-------------------------------------------------------

  //Draw the monitoring plots according to the specifications in the configfiles

  for (int i_time = 0; i_time < timeFrames.size(); i_time++){

    ULong64_t zero = 0;
    if (endTimes.at(i_time) == zero) endTimes.at(i_time) = t_file_end;        //set 0 for t_file_end since we did not know what that was at the beginning of initialise
    /*std::cout << (endTimes.at(i_time) == zero) << std::endl;
    std::cout << (endTimes.at(i_time) == 0) << std::endl;
    std::cout <<t_file_end<<std::endl;*/

    for (int i_plot = 0; i_plot < plotTypes.at(i_time).size(); i_plot++){

      if (plotTypes.at(i_time).at(i_plot) == "RateElectronics") DrawRatePlotElectronics(endTimes.at(i_time),timeFrames.at(i_time),fileLabels.at(i_time));
      else if (plotTypes.at(i_time).at(i_plot) == "RatePhysical") DrawRatePlotPhysical(endTimes.at(i_time),timeFrames.at(i_time),fileLabels.at(i_time));
      else if (plotTypes.at(i_time).at(i_plot) == "PedElectronics") DrawPedPlotElectronics(endTimes.at(i_time),timeFrames.at(i_time),fileLabels.at(i_time));
      else if (plotTypes.at(i_time).at(i_plot) == "PedPhysical") DrawPedPlotPhysical(endTimes.at(i_time),timeFrames.at(i_time),fileLabels.at(i_time));
      else if (plotTypes.at(i_time).at(i_plot) == "SigmaElectronics") DrawSigmaPlotElectronics(endTimes.at(i_time),timeFrames.at(i_time),fileLabels.at(i_time));
      else if (plotTypes.at(i_time).at(i_plot) == "SigmaPhysical") DrawSigmaPlotPhysical(endTimes.at(i_time),timeFrames.at(i_time),fileLabels.at(i_time));
      else if (plotTypes.at(i_time).at(i_plot) == "TimeEvolution") DrawTimeEvolution(endTimes.at(i_time),timeFrames.at(i_time),fileLabels.at(i_time));
      else if (plotTypes.at(i_time).at(i_plot) == "TimeDifference") DrawTimeDifference(endTimes.at(i_time),timeFrames.at(i_time),fileLabels.at(i_time));
      else if (plotTypes.at(i_time).at(i_plot) == "FileHistory") DrawFileHistory(endTimes.at(i_time),timeFrames.at(i_time),fileLabels.at(i_time));
      else {
        if (verbosity > 0) std::cout <<"ERROR (MonitorTankTime): UpdateMonitorPlots: Specified plot type -"<<plotTypes.at(i_time).at(i_plot)<<"- does not exist! Omit entry."<<std::endl;
      }
    }
  }

}


void MonitorTankTime::DrawRatePlotElectronics(ULong64_t timestamp_end, double time_frame, std::string file_ending){

  Log("MonitorTankTime: DrawRatePlotElectronics",v_message,verbosity);

  //-------------------------------------------------------
  //-------------DrawRatePlotElectronics ------------------
  //-------------------------------------------------------

  if (timestamp_end != readfromfile_tend || time_frame != readfromfile_timeframe) ReadFromFile(timestamp_end, time_frame);

  std::vector<double> overall_rates;
  ULong64_t current_timeframe;
  ULong64_t overall_timeframe = 0;
  overall_rates.assign(num_active_slots*num_channels_tank,0);

  for (int i_file = 0; i_file < rate_plot.size(); i_file++){

    current_timeframe = (tend_plot.at(i_file)-tstart_plot.at(i_file))/MSEC_to_SEC;
    overall_timeframe += current_timeframe;

    for (int i_ch = 0; i_ch < num_active_slots*num_channels_tank; i_ch++){
      overall_rates.at(i_ch) += (rate_plot.at(i_file).at(i_ch)*current_timeframe);
    }
  }

  if (overall_timeframe > 0.){
    for (int i_ch = 0.; i_ch < num_active_slots*num_channels_tank; i_ch++){
      overall_rates.at(i_ch)/=overall_timeframe;
    }
  }

  long max_rate = 0;
  long min_rate = 999999;
  for (int i_slot = 0; i_slot < num_active_slots; i_slot++){
    std::vector<unsigned int> crateslot = map_slot_to_crateslot[i_slot];
    unsigned int i_crate = crateslot.at(0);
    for (int i_channel = 0; i_channel < num_channels_tank; i_channel++){
      int i_active = i_slot*num_channels_tank+i_channel;
      int x = i_slot+1;
      int y = (3-i_crate)*num_channels_tank - i_channel;      //top most crate is displayed at the top, then crate #2 in the middle and crate #3 at the bottom
      h2D_rate->SetBinContent(x,y,overall_rates.at(i_active));
      if (overall_rates.at(i_active) > max_rate) max_rate = overall_rates.at(i_active);
      else if (overall_rates.at(i_active) < min_rate) min_rate = overall_rates.at(i_active);
    }
  }

  ss_title_rate.str("");
  ss_title_rate << title_time.str() << str_rate;
  h2D_rate->SetTitle(ss_title_rate.str().c_str());

  TPad *p_rate = (TPad*) canvas_rate->cd();
  h2D_rate->SetStats(0);
  h2D_rate->GetXaxis()->SetNdivisions(num_slots_tank);
  h2D_rate->GetYaxis()->SetNdivisions(num_crates_tank*num_channels_tank);
  p_rate->SetGrid();
  for (int i_label = 0; i_label < int(num_slots_tank); i_label++){
    std::stringstream ss_slot;
    ss_slot<<(i_label+1);
    std::string str_slot = "slot "+ss_slot.str();
    h2D_rate->GetXaxis()->SetBinLabel(i_label+1,str_slot.c_str());
  }
  for (int i_label=0; i_label < int(num_channels_tank*num_crates_tank); i_label++){
    std::stringstream ss_ch;
    if (i_label < 4) ss_ch<<((3-i_label)%4);
    else if (i_label < 8) ss_ch<<((7-i_label)%4);
    else ss_ch<<((11-i_label)%4);
    std::string str_ch = "ch "+ss_ch.str();
    h2D_rate->GetYaxis()->SetBinLabel(i_label+1,str_ch.c_str());
  }
  h2D_rate->LabelsOption("v");
  h2D_rate->Draw("colz");

  for (int i_box =0; i_box < vector_box_inactive.size(); i_box++){
    vector_box_inactive.at(i_box)->Draw("same");
  }

  line1->Draw("same");
  line2->Draw("same");
  text_crate1->Draw();
  text_crate2->Draw();
  text_crate3->Draw();
  label_rate->Draw();
  p_rate ->Update();

  if (h2D_rate->GetMaximum()>0.){
  if (abs(max_rate-min_rate)==0) h2D_sigma->GetZaxis()->SetRangeUser(min_rate-1,max_rate+1);
  else h2D_rate->GetZaxis()->SetRangeUser(min_rate-0.5,max_rate+0.5);
  TPaletteAxis *palette = 
  (TPaletteAxis*)h2D_rate->GetListOfFunctions()->FindObject("palette");
  palette->SetX1NDC(0.9);
  palette->SetX2NDC(0.92);
  palette->SetY1NDC(0.1);
  palette->SetY2NDC(0.9);
  }
  p_rate->Update();

  std::stringstream ss_rate;
  ss_rate<<outpath<<"PMT_Rate_Electronics.jpg";
  canvas_rate->SaveAs(ss_rate.str().c_str());
  Log("MonitorTankTime: Output path Rate plot (Electronics space): "+ss_rate.str(),v_message,verbosity);

  h2D_rate->Reset();
  canvas_rate->Clear();

}

void MonitorTankTime::DrawRatePlotPhysical(ULong64_t timestamp_end, double time_frame, std::string file_ending){

  Log("MonitorTankTime: DrawRatePlotPhysical",v_message,verbosity);

  //-------------------------------------------------------
  //-------------DrawRatePlotPhysical ---------------------
  //-------------------------------------------------------

  //TODO


}

void MonitorTankTime::DrawPedPlotElectronics(ULong64_t timestamp_end, double time_frame, std::string file_ending){

  Log("MonitorTankTime: DrawPedPlotElectronics",v_message,verbosity);

  //-------------------------------------------------------
  //-------------DrawPedPlotElectronics -------------------
  //-------------------------------------------------------

  if (timestamp_end != readfromfile_tend || time_frame != readfromfile_timeframe) ReadFromFile(timestamp_end, time_frame);

  std::vector<double> overall_peds;
  ULong64_t current_timeframe;
  ULong64_t overall_timeframe = 0;
  overall_peds.assign(num_active_slots*num_channels_tank,0);

  for (int i_file = 0; i_file < ped_plot.size(); i_file++){

    current_timeframe = (tend_plot.at(i_file)-tstart_plot.at(i_file))/MSEC_to_SEC;
    overall_timeframe += current_timeframe;

    for (int i_ch = 0; i_ch < num_active_slots*num_channels_tank; i_ch++){
      overall_peds.at(i_ch) += (ped_plot.at(i_file).at(i_ch)*current_timeframe);
    }
  }

  if (overall_timeframe > 0.){
    for (int i_ch = 0.; i_ch < num_active_slots*num_channels_tank; i_ch++){
      overall_peds.at(i_ch)/=overall_timeframe;
    }
  }

  double max_ped = 0;
  double min_ped = 999999;
  for (int i_slot = 0; i_slot < num_active_slots; i_slot++){
    std::vector<unsigned int> crateslot = map_slot_to_crateslot[i_slot];
    unsigned int i_crate = crateslot.at(0);
    for (int i_channel = 0; i_channel < num_channels_tank; i_channel++){
      int i_active = i_slot*num_channels_tank+i_channel;
      int x = i_slot+1;
      int y = (3-i_crate)*num_channels_tank - i_channel;      //top most crate is displayed at the top, then crate #2 in the middle and crate #3 at the bottom
      h2D_ped->SetBinContent(x,y,overall_peds.at(i_active));
      if (overall_peds.at(i_active) > max_ped) max_ped = overall_peds.at(i_active);
      else if (overall_peds.at(i_active) < min_ped) min_ped = overall_peds.at(i_active);
    }
  }

  ss_title_ped.str("");
  ss_title_ped << title_time.str() << str_ped;
  h2D_ped->SetTitle(ss_title_ped.str().c_str());

  TPad *p = (TPad*) canvas_ped->cd();
  h2D_ped->SetStats(0);
  h2D_ped->GetXaxis()->SetNdivisions(num_slots_tank);
  h2D_ped->GetYaxis()->SetNdivisions(num_crates_tank*num_channels_tank);
  p->SetGrid();
  for (int i_label = 0; i_label < int(num_slots_tank); i_label++){
    std::stringstream ss_slot;
    ss_slot<<(i_label+1);
    std::string str_slot = "slot "+ss_slot.str();
    h2D_ped->GetXaxis()->SetBinLabel(i_label+1,str_slot.c_str());
  }
  for (int i_label=0; i_label < int(num_channels_tank*num_crates_tank); i_label++){
    std::stringstream ss_ch;
    if (i_label < 4) ss_ch<<((3-i_label)%4);
    else if (i_label < 8) ss_ch<<((7-i_label)%4);
    else ss_ch<<((11-i_label)%4);
    std::string str_ch = "ch "+ss_ch.str();
    h2D_ped->GetYaxis()->SetBinLabel(i_label+1,str_ch.c_str());
  }
  h2D_ped->LabelsOption("v");
  h2D_ped->Draw("colz");

  for (int i_box =0; i_box < vector_box_inactive.size(); i_box++){
    vector_box_inactive.at(i_box)->Draw("same");
  }

  p->Update();
  if (h2D_ped->GetMaximum()>0.){
  if (abs(max_ped-min_ped)==0) h2D_ped->GetZaxis()->SetRangeUser(min_ped-1,max_ped+1);
  else h2D_ped->GetZaxis()->SetRangeUser(min_ped-0.5,max_ped+0.5);
  TPaletteAxis *palette = 
  (TPaletteAxis*)h2D_ped->GetListOfFunctions()->FindObject("palette");
  palette->SetX1NDC(0.9);
  palette->SetX2NDC(0.92);
  palette->SetY1NDC(0.1);
  palette->SetY2NDC(0.9);
  }
  p->Update();

  line1->Draw("same");
  line2->Draw("same");
  text_crate1->Draw();
  text_crate2->Draw();
  text_crate3->Draw();
  label_ped->Draw();

  std::stringstream ss_ped;
  ss_ped<<outpath<<"PMT_Ped_Electronics.jpg";
  Log("MonitorTankTime: Output path Pedestal mean plot (Electronics space): "+ss_ped.str(),v_message,verbosity);
  canvas_ped->SaveAs(ss_ped.str().c_str());

  h2D_ped->Reset();
  canvas_ped->Clear();

}

void MonitorTankTime::DrawPedPlotPhysical(ULong64_t timestamp_end, double time_frame, std::string file_ending){

  Log("MonitorTankTime: DrawPedPlotPhysical",v_message,verbosity);

  //-------------------------------------------------------
  //-------------DrawPedPlotPhysical ----------------------
  //-------------------------------------------------------

  //TODO

}

void MonitorTankTime::DrawSigmaPlotElectronics(ULong64_t timestamp_end, double time_frame, std::string file_ending){

  Log("MonitorTankTime: DrawSigmaPlotElectronics",v_message,verbosity);

  //-------------------------------------------------------
  //-------------DrawSigmaPlotElectronics -----------------
  //-------------------------------------------------------

  if (timestamp_end != readfromfile_tend || time_frame != readfromfile_timeframe) ReadFromFile(timestamp_end, time_frame);
    
  std::vector<double> overall_sigmas;
  ULong64_t current_timeframe;
  ULong64_t overall_timeframe = 0;
  overall_sigmas.assign(num_active_slots*num_channels_tank,0);

  for (int i_file = 0; i_file < sigma_plot.size(); i_file++){

    current_timeframe = (tend_plot.at(i_file)-tstart_plot.at(i_file))/MSEC_to_SEC;
    overall_timeframe += current_timeframe;

    for (int i_ch = 0; i_ch < num_active_slots*num_channels_tank; i_ch++){
      overall_sigmas.at(i_ch) += (sigma_plot.at(i_file).at(i_ch)*current_timeframe);
    }
  }

  if (overall_timeframe > 0.){
    for (int i_ch = 0.; i_ch < num_active_slots*num_channels_tank; i_ch++){
      overall_sigmas.at(i_ch)/=overall_timeframe;
    }
  }

  double max_sigma = 0;
  double min_sigma = 999999;
  
  for (int i_slot = 0; i_slot < num_active_slots; i_slot++){
    std::vector<unsigned int> crateslot = map_slot_to_crateslot[i_slot];
    unsigned int i_crate = crateslot.at(0);
    for (int i_channel = 0; i_channel < num_channels_tank; i_channel++){
      int i_active = i_slot*num_channels_tank+i_channel;
      int x = i_slot+1;
      int y = (3-i_crate)*num_channels_tank - i_channel;      //top most crate is displayed at the top, then crate #2 in the middle and crate #3 at the bottom
      h2D_sigma->SetBinContent(x,y,overall_sigmas.at(i_active));
      if (overall_sigmas.at(i_active) > max_sigma) max_sigma = overall_sigmas.at(i_active);
      else if (overall_sigmas.at(i_active) < min_sigma) min_sigma = overall_sigmas.at(i_active);
    }
  }

  ss_title_sigma.str("");
  ss_title_sigma << title_time.str() << str_sigma;
  h2D_sigma->SetTitle(ss_title_sigma.str().c_str());

  TPad *p_sigma = (TPad*) canvas_sigma->cd();
  h2D_sigma->SetStats(0);
  h2D_sigma->GetXaxis()->SetNdivisions(num_slots_tank);
  h2D_sigma->GetYaxis()->SetNdivisions(num_crates_tank*num_channels_tank);
  p_sigma->SetGrid();
  for (int i_label = 0; i_label < int(num_slots_tank); i_label++){
    std::stringstream ss_slot;
    ss_slot<<(i_label+1);
    std::string str_slot = "slot "+ss_slot.str();
    h2D_sigma->GetXaxis()->SetBinLabel(i_label+1,str_slot.c_str());
  }
  for (int i_label=0; i_label < int(num_channels_tank*num_crates_tank); i_label++){
    std::stringstream ss_ch;
    if (i_label < 4) ss_ch<<((3-i_label)%4);
    else if (i_label < 8) ss_ch<<((7-i_label)%4);
    else ss_ch<<((11-i_label)%4);
    std::string str_ch = "ch "+ss_ch.str();
    h2D_sigma->GetYaxis()->SetBinLabel(i_label+1,str_ch.c_str());
  }
  h2D_sigma->LabelsOption("v");
  h2D_sigma->Draw("colz");

  for (int i_box =0; i_box < vector_box_inactive.size(); i_box++){
    vector_box_inactive.at(i_box)->Draw("same");
  }

  text_crate1->Draw();
  text_crate2->Draw();
  text_crate3->Draw();
  line1->Draw("same");
  line2->Draw("same");
  label_sigma->Draw();
  p_sigma ->Update();

  if (h2D_sigma->GetMaximum()>0.){
    if (abs(max_sigma-min_sigma)==0) h2D_sigma->GetZaxis()->SetRangeUser(min_sigma-1,max_sigma+1);
    else h2D_sigma->GetZaxis()->SetRangeUser(min_sigma-0.5,max_sigma+0.5);
    TPaletteAxis *palette = 
    (TPaletteAxis*)h2D_sigma->GetListOfFunctions()->FindObject("palette");
    palette->SetX1NDC(0.9);
    palette->SetX2NDC(0.92);
    palette->SetY1NDC(0.1);
    palette->SetY2NDC(0.9);
  }
  p_sigma->Update();
  std::stringstream ss_sigma;
  ss_sigma<<outpath<<"PMT_Sigma_Electronics.jpg";
  canvas_sigma->SaveAs(ss_sigma.str().c_str());
  Log("MonitorTankTime: Output path Pedestal Sigma plot (Electronics Space): "+ss_sigma.str(),v_message,verbosity);

  h2D_sigma->Reset();
  canvas_sigma->Clear();

}

void MonitorTankTime::DrawSigmaPlotPhysical(ULong64_t timestamp_end, double time_frame, std::string file_ending){

  Log("MonitorTankTime: DrawSigmaPlotPhysical",v_message,verbosity);

  //-------------------------------------------------------
  //-------------DrawSigmaPlotPhysical ----------------------
  //-------------------------------------------------------

  //TODO

}

void MonitorTankTime::DrawHitMap(ULong64_t timestamp_end, double time_frame, std::string file_ending){

  Log("MonitorTankTime: DrawHitMap",v_message,verbosity);

  //-------------------------------------------------------
  //---------------------DrawHitMap -----------------------
  //-------------------------------------------------------

  //TODO

}

void MonitorTankTime::DrawTimeEvolution(ULong64_t timestamp_end, double time_frame, std::string file_ending){

  Log("MonitorTankTime: DrawTimeEvolution",v_message,verbosity);

  //-------------------------------------------------------
  //-------------DrawTimeEvolution ------------------------
  //-------------------------------------------------------

   if (timestamp_end != readfromfile_tend || time_frame != readfromfile_timeframe) ReadFromFile(timestamp_end, time_frame);

  //looping over all files that are in the time interval, each file will be one data point

  std::stringstream ss_timeframe;
  ss_timeframe << round(time_frame*100.)/100.;

  for (int i_channel = 0; i_channel < num_active_slots*num_channels_tank; i_channel++){
    gr_ped.at(i_channel)->Set(0);
    gr_sigma.at(i_channel)->Set(0);
    gr_rate.at(i_channel)->Set(0);
  }

  for (int i_file=0; i_file<ped_plot.size(); i_file++){

    //Updating channel graphs

    Log("MonitorTankTime: Stored data (file #"+std::to_string(i_file+1)+"): ",v_message,verbosity);
    for (int i_channel = 0; i_channel < num_active_slots*num_channels_tank; i_channel++){
      gr_ped.at(i_channel)->SetPoint(i_file,labels_timeaxis[i_file].Convert(),ped_plot.at(i_file).at(i_channel));
      gr_sigma.at(i_channel)->SetPoint(i_file,labels_timeaxis[i_file].Convert(),sigma_plot.at(i_file).at(i_channel));
      gr_rate.at(i_channel)->SetPoint(i_file,labels_timeaxis[i_file].Convert(),rate_plot.at(i_file).at(i_channel));
    }

  }

  // Drawing time evolution plots

  double max_canvas = 0;
  double min_canvas = 9999999.;
  double max_canvas_sigma = 0;
  double min_canvas_sigma = 99999999.;
  double max_canvas_rate = 0;
  double min_canvas_rate = 999999999.;

  int CH_per_CANVAS = 4;   //channels per canvas

  for (int i_channel = 0; i_channel<num_active_slots*num_channels_tank; i_channel++){

    std::vector<unsigned int> crateslotchannel = map_ch_to_crateslotch[i_channel];
    unsigned int crate = crateslotchannel.at(0);
    unsigned int slot = crateslotchannel.at(1);
    unsigned int channel = crateslotchannel.at(2);

    std::stringstream ss_ch_ped, ss_ch_sigma, ss_ch_rate, ss_leg_time;
  
     if (i_channel%CH_per_CANVAS == 0 || i_channel == num_active_slots*num_channels_tank-1) {
      if (i_channel != 0){

        ss_ch_ped.str("");
        ss_ch_sigma.str("");
        ss_ch_rate.str("");

        ss_ch_ped<<"Crate "<<crate<<" Slot "<<slot<<" ("<<ss_timeframe.str()<<"h)";
        ss_ch_sigma<<"Crate "<<crate<<" Slot "<<slot<<" ("<<ss_timeframe.str()<<"h)";
        ss_ch_rate<<"Crate "<<crate<<" Slot "<<slot<<" ("<<ss_timeframe.str()<<"h)";

        if (i_channel == num_active_slots*num_channels_tank - 1){
          ss_leg_time.str("");
          ss_leg_time<<"ch "<<channel;
          multi_ch_ped->Add(gr_ped.at(i_channel));
          leg_ped->AddEntry(gr_ped.at(i_channel),ss_leg_time.str().c_str(),"l");
          multi_ch_sigma->Add(gr_sigma.at(i_channel));
          leg_sigma->AddEntry(gr_sigma.at(i_channel),ss_leg_time.str().c_str(),"l");
          multi_ch_rate->Add(gr_rate.at(i_channel));
          leg_rate->AddEntry(gr_rate.at(i_channel),ss_leg_time.str().c_str(),"l");
        }

        canvas_ch_ped->cd();
        multi_ch_ped->Draw("apl");
        multi_ch_ped->SetTitle(ss_ch_ped.str().c_str());
        multi_ch_ped->GetYaxis()->SetTitle("Pedestal");
        multi_ch_ped->GetXaxis()->SetTimeDisplay(1);
        multi_ch_ped->GetXaxis()->SetLabelSize(0.03);
        multi_ch_ped->GetXaxis()->SetLabelOffset(0.03);
        multi_ch_ped->GetXaxis()->SetTimeFormat("#splitline{%m/%d}{%H:%M}");
        multi_ch_ped->GetXaxis()->SetTimeOffset(0.);
        leg_ped->Draw();
        canvas_ch_sigma->cd();
        multi_ch_sigma->Draw("apl");
        multi_ch_sigma->SetTitle(ss_ch_sigma.str().c_str());
        multi_ch_sigma->GetYaxis()->SetTitle("Sigma");
        multi_ch_sigma->GetYaxis()->SetTitleSize(0.035);
        multi_ch_sigma->GetYaxis()->SetTitleOffset(1.3);
        multi_ch_sigma->GetXaxis()->SetTimeDisplay(1);
        multi_ch_sigma->GetXaxis()->SetLabelSize(0.03);
        multi_ch_sigma->GetXaxis()->SetLabelOffset(0.03);
        multi_ch_sigma->GetXaxis()->SetTimeFormat("#splitline{%m/%d}{%H:%M}");
        multi_ch_sigma->GetXaxis()->SetTimeOffset(0.);
        leg_sigma->Draw();
        canvas_ch_rate->cd();
        multi_ch_rate->Draw("apl");
        multi_ch_rate->SetTitle(ss_ch_rate.str().c_str());
        multi_ch_rate->GetYaxis()->SetTitle("Rate [Hz]");
        multi_ch_rate->GetYaxis()->SetTitleSize(0.035);
        multi_ch_rate->GetYaxis()->SetTitleOffset(1.3);
        multi_ch_rate->GetXaxis()->SetTimeDisplay(1);
        multi_ch_rate->GetXaxis()->SetLabelSize(0.03);
        multi_ch_rate->GetXaxis()->SetLabelOffset(0.03);
        multi_ch_rate->GetXaxis()->SetTimeFormat("#splitline{%m/%d}{%H:%M}");
        multi_ch_rate->GetXaxis()->SetTimeOffset(0.);
        leg_rate->Draw();

        ss_ch_ped.str("");
        ss_ch_sigma.str("");
        ss_ch_rate.str("");
        ss_ch_ped<<outpath<<"PMTTimeEvolutionPed_Cr"<<crate<<"_Sl"<<slot<<"_"<<file_ending<<"."<<img_extension;
        ss_ch_sigma<<outpath<<"PMTTimeEvolutionSigma_Cr"<<crate<<"_Sl"<<slot<<"_"<<file_ending<<"."<<img_extension;
        ss_ch_rate<<outpath<<"PMTTimeEvolutionRate_Cr"<<crate<<"_Sl"<<slot<<"_"<<file_ending<<"."<<img_extension;

        canvas_ch_ped->SaveAs(ss_ch_ped.str().c_str());
        canvas_ch_sigma->SaveAs(ss_ch_sigma.str().c_str());
        canvas_ch_rate->SaveAs(ss_ch_rate.str().c_str()); 

        for (int i_gr=0; i_gr < CH_per_CANVAS; i_gr++){
          int i_balance = (i_channel == num_active_slots*num_channels_tank-1)? 1 : 0;
          multi_ch_ped->RecursiveRemove(gr_ped.at(i_channel-CH_per_CANVAS+i_gr+i_balance));
          multi_ch_sigma->RecursiveRemove(gr_sigma.at(i_channel-CH_per_CANVAS+i_gr+i_balance));
          multi_ch_rate->RecursiveRemove(gr_rate.at(i_channel-CH_per_CANVAS+i_gr+i_balance));
        }
      }

      leg_ped->Clear();
      leg_sigma->Clear();
      leg_rate->Clear();
      
      canvas_ch_ped->Clear();
      canvas_ch_sigma->Clear();
      canvas_ch_rate->Clear();

     } 

    if (i_channel != num_active_slots*num_channels_tank-1){

      ss_leg_time.str("");
      ss_leg_time<<"ch "<<i_channel%(num_channels_tank) + 1;
      multi_ch_ped->Add(gr_ped.at(i_channel));
      if (gr_ped.at(i_channel)->GetMaximum()>max_canvas) max_canvas = gr_ped.at(i_channel)->GetMaximum();
      if (gr_ped.at(i_channel)->GetMinimum()<min_canvas) min_canvas = gr_ped.at(i_channel)->GetMinimum();
      leg_ped->AddEntry(gr_ped.at(i_channel),ss_leg_time.str().c_str(),"l");
      multi_ch_sigma->Add(gr_sigma.at(i_channel));
      leg_sigma->AddEntry(gr_sigma.at(i_channel),ss_leg_time.str().c_str(),"l");
      if (gr_sigma.at(i_channel)->GetMaximum()>max_canvas_sigma) max_canvas_sigma = gr_sigma.at(i_channel)->GetMaximum();
      if (gr_sigma.at(i_channel)->GetMinimum()<min_canvas_sigma) min_canvas_sigma = gr_sigma.at(i_channel)->GetMinimum();
      multi_ch_rate->Add(gr_rate.at(i_channel));
      leg_rate->AddEntry(gr_rate.at(i_channel),ss_leg_time.str().c_str(),"l");
      if (gr_rate.at(i_channel)->GetMaximum()>max_canvas_rate) max_canvas_rate = gr_rate.at(i_channel)->GetMaximum();
      if (gr_rate.at(i_channel)->GetMinimum()<min_canvas_rate) min_canvas_rate = gr_rate.at(i_channel)->GetMinimum();

    }

    //single channel time evolution plots

    if (draw_single){

      canvas_ch_single->cd();
      canvas_ch_single->Clear();
      gr_ped.at(i_channel)->GetYaxis()->SetTitle("Pedestal");
      gr_ped.at(i_channel)->GetXaxis()->SetTimeDisplay(1);
      gr_ped.at(i_channel)->GetXaxis()->SetLabelSize(0.03);
      gr_ped.at(i_channel)->GetXaxis()->SetLabelOffset(0.03);
      gr_ped.at(i_channel)->GetXaxis()->SetTimeFormat("#splitline{%m/%d}{%H:%M}");
      gr_ped.at(i_channel)->GetXaxis()->SetTimeOffset(0.);
      gr_ped.at(i_channel)->Draw("apl");
      std::stringstream ss_ch_single;
      ss_ch_single<<outpath<<"PMTTimeEvolutionPed_Cr"<<crate<<"_Sl"<<slot<<"_Ch"<<channel<<"_"<<file_ending<<"."<<img_extension;
      canvas_ch_single->SaveAs(ss_ch_single.str().c_str());

      canvas_ch_single->Clear();
      gr_sigma.at(i_channel)->GetYaxis()->SetTitle("Sigma");
      gr_sigma.at(i_channel)->GetXaxis()->SetTimeDisplay(1);
      gr_sigma.at(i_channel)->GetXaxis()->SetLabelSize(0.03);
      gr_sigma.at(i_channel)->GetXaxis()->SetLabelOffset(0.03);
      gr_sigma.at(i_channel)->GetXaxis()->SetTimeFormat("#splitline{%m/%d}{%H:%M}");
      gr_sigma.at(i_channel)->GetXaxis()->SetTimeOffset(0.);
      gr_sigma.at(i_channel)->Draw("apl");
      ss_ch_single.str("");
      ss_ch_single<<outpath<<"PMTTimeEvolutionSigma_Cr"<<crate<<"_Sl"<<slot<<"_Ch"<<channel<<"_"<<file_ending<<"."<<img_extension;
      canvas_ch_single->SaveAs(ss_ch_single.str().c_str());

      canvas_ch_single->Clear();
      gr_rate.at(i_channel)->GetYaxis()->SetTitle("Rate [Hz]");
      gr_rate.at(i_channel)->GetXaxis()->SetTimeDisplay(1);
      gr_rate.at(i_channel)->GetXaxis()->SetLabelSize(0.03);
      gr_rate.at(i_channel)->GetXaxis()->SetLabelOffset(0.03);
      gr_rate.at(i_channel)->GetXaxis()->SetTimeFormat("#splitline{%m/%d}{%H:%M}");
      gr_rate.at(i_channel)->GetXaxis()->SetTimeOffset(0.);
      gr_rate.at(i_channel)->Draw("apl");
      ss_ch_single.str("");
      ss_ch_single<<outpath<<"PMTTimeEvolutionRate_Cr"<<crate<<"_Sl"<<slot<<"_Ch"<<channel<<"_"<<file_ending<<"."<<img_extension;
      canvas_ch_single->SaveAs(ss_ch_single.str().c_str());

    }

  }


}

void MonitorTankTime::DrawTimeDifference(ULong64_t timestamp_end, double time_frame, std::string file_ending){

  Log("MonitorTankTime: DrawTimeDifference",v_message,verbosity);

  //-------------------------------------------------------
  //----------------DrawTimeDifference --------------------
  //-------------------------------------------------------

  if (timestamp_end != readfromfile_tend || time_frame != readfromfile_timeframe) ReadFromFile(timestamp_end, time_frame);
    
  double max_peddiff = 0.;
  double min_peddiff = 999999.;
  double max_sigmadiff = 0.;
  double min_sigmadiff = 9999999.;
  double max_ratediff = 0.;
  double min_ratediff = 9999999.; 

  //pedestal time difference plot

  std::vector<double> overall_peddiffs;
  overall_peddiffs.assign(num_active_slots*num_channels_tank,0.);

  if (ped_plot.size() >= 1){
    for (int i_ch = 0; i_ch < num_active_slots*num_channels_tank; i_ch++){
      overall_peddiffs.at(i_ch) = ped_plot.at(ped_plot.size()-1).at(i_ch) - ped_plot.at(0).at(i_ch);
    }
  }

  for (int i_slot = 0; i_slot < num_active_slots; i_slot++){
    std::vector<unsigned int> crateslot = map_slot_to_crateslot[i_slot];
    unsigned int i_crate = crateslot.at(0);
    for (int i_channel = 0; i_channel < num_channels_tank; i_channel++){
      int i_active = i_slot*num_channels_tank+i_channel;
      int x = i_slot+1;
      int y = (3-i_crate)*num_channels_tank - i_channel;      //top most crate is displayed at the top, then crate #2 in the middle and crate #3 at the bottom
      h2D_peddiff->SetBinContent(x,y,overall_peddiffs.at(i_active));
      if (overall_peddiffs.at(i_active) > max_peddiff) max_peddiff = overall_peddiffs.at(i_active);
      else if (overall_peddiffs.at(i_active) < min_peddiff) min_peddiff = overall_peddiffs.at(i_active);
    }
  }

  ss_title_peddiff.str("");
  ss_title_peddiff << title_time.str() << str_peddiff;
  h2D_peddiff->SetTitle(ss_title_peddiff.str().c_str());

  TPad *p_peddiff = (TPad*) canvas_peddiff->cd();
  h2D_peddiff->SetStats(0);
  h2D_peddiff->GetXaxis()->SetNdivisions(num_slots_tank);
  h2D_peddiff->GetYaxis()->SetNdivisions(num_crates_tank*num_channels_tank);
  p_peddiff->SetGrid();
  for (int i_label = 0; i_label < int(num_slots_tank); i_label++){
    std::stringstream ss_slot;
    ss_slot<<(i_label+1);
    std::string str_slot = "slot "+ss_slot.str();
    h2D_peddiff->GetXaxis()->SetBinLabel(i_label+1,str_slot.c_str());
  }
  for (int i_label=0; i_label < int(num_channels_tank*num_crates_tank); i_label++){
    std::stringstream ss_ch;
    if (i_label < 4) ss_ch<<((3-i_label)%4);
    else if (i_label < 8) ss_ch<<((7-i_label)%4);
    else ss_ch<<((11-i_label)%4);
    std::string str_ch = "ch "+ss_ch.str();
    h2D_peddiff->GetYaxis()->SetBinLabel(i_label+1,str_ch.c_str());
  }
  h2D_peddiff->LabelsOption("v");
  h2D_peddiff->Draw("colz");

  for (int i_box =0; i_box < vector_box_inactive.size(); i_box++){
    vector_box_inactive.at(i_box)->Draw("same");
  }

  line1->Draw("same");
  line2->Draw("same");
  text_crate1->Draw();
  text_crate2->Draw();
  text_crate3->Draw();
  label_peddiff->Draw();
  p_peddiff->Update();

  if (h2D_peddiff->GetMaximum()>0.){
    if (abs(max_peddiff-min_peddiff)==0) h2D_peddiff->GetZaxis()->SetRangeUser(min_peddiff-1,max_peddiff+1);
    else h2D_peddiff->GetZaxis()->SetRangeUser(min_peddiff-0.5,max_peddiff+0.5);
    TPaletteAxis *palette = 
    (TPaletteAxis*)h2D_peddiff->GetListOfFunctions()->FindObject("palette");
    palette->SetX1NDC(0.9);
    palette->SetX2NDC(0.92);
    palette->SetY1NDC(0.1);
    palette->SetY2NDC(0.9);
  }
  p_peddiff->Update();

  std::stringstream ss_peddiff;
  ss_peddiff<<outpath<<"PMT_2D_PedDifference.jpg";
  canvas_peddiff->SaveAs(ss_peddiff.str().c_str());
  Log("MonitorTankTime: Output path Pedestal time difference plot: "+ss_peddiff.str(),v_message,verbosity);

  //sigma time difference plot

  std::vector<double> overall_sigmadiffs;
  overall_sigmadiffs.assign(num_active_slots*num_channels_tank,0.);

  if (sigma_plot.size() >= 1){
    for (int i_ch = 0; i_ch < num_active_slots*num_channels_tank; i_ch++){
      overall_sigmadiffs.at(i_ch) = sigma_plot.at(sigma_plot.size()-1).at(i_ch) - sigma_plot.at(0).at(i_ch);
    }
  }

  for (int i_slot = 0; i_slot < num_active_slots; i_slot++){
    std::vector<unsigned int> crateslot = map_slot_to_crateslot[i_slot];
    unsigned int i_crate = crateslot.at(0);
    for (int i_channel = 0; i_channel < num_channels_tank; i_channel++){
      int i_active = i_slot*num_channels_tank+i_channel;
      int x = i_slot+1;
      int y = (3-i_crate)*num_channels_tank - i_channel;      //top most crate is displayed at the top, then crate #2 in the middle and crate #3 at the bottom
      h2D_sigmadiff->SetBinContent(x,y,overall_sigmadiffs.at(i_active));
      if (overall_sigmadiffs.at(i_active) > max_sigmadiff) max_sigmadiff = overall_sigmadiffs.at(i_active);
      else if (overall_sigmadiffs.at(i_active) < min_sigmadiff) min_sigmadiff = overall_sigmadiffs.at(i_active);
    }
  }

  ss_title_sigmadiff.str("");
  ss_title_sigmadiff << title_time.str() << str_sigmadiff;
  h2D_sigmadiff->SetTitle(ss_title_sigmadiff.str().c_str());

  TPad *p_sigmadiff = (TPad*) canvas_sigmadiff->cd();
  h2D_sigmadiff->SetStats(0);
  h2D_sigmadiff->GetXaxis()->SetNdivisions(num_slots_tank);
  h2D_sigmadiff->GetYaxis()->SetNdivisions(num_crates_tank*num_channels_tank);
  p_sigmadiff->SetGrid();
  for (int i_label = 0; i_label < int(num_slots_tank); i_label++){
    std::stringstream ss_slot;
    ss_slot<<(i_label+1);
    std::string str_slot = "slot "+ss_slot.str();
    h2D_sigmadiff->GetXaxis()->SetBinLabel(i_label+1,str_slot.c_str());
  }
  for (int i_label=0; i_label < int(num_channels_tank*num_crates_tank); i_label++){
    std::stringstream ss_ch;
    if (i_label < 4) ss_ch<<((3-i_label)%4);
    else if (i_label < 8) ss_ch<<((7-i_label)%4);
    else ss_ch<<((11-i_label)%4);
    std::string str_ch = "ch "+ss_ch.str();
    h2D_sigmadiff->GetYaxis()->SetBinLabel(i_label+1,str_ch.c_str());
  }
  h2D_sigmadiff->LabelsOption("v");
  h2D_sigmadiff->Draw("colz");

  for (int i_box =0; i_box < vector_box_inactive.size(); i_box++){
    vector_box_inactive.at(i_box)->Draw("same");
  }

  line1->Draw("same");
  line2->Draw("same");
  text_crate1->Draw();
  text_crate2->Draw();
  text_crate3->Draw();
  label_sigmadiff->Draw();

  p_sigmadiff->Update();

  if (h2D_sigmadiff->GetMaximum()>0.){
    if (abs(max_sigmadiff-min_sigmadiff)==0) h2D_sigmadiff->GetZaxis()->SetRangeUser(min_sigmadiff-1,max_sigmadiff+1);
    else h2D_sigmadiff->GetZaxis()->SetRangeUser(min_sigmadiff-0.5,max_sigmadiff+0.5);
    TPaletteAxis *palette = 
    (TPaletteAxis*)h2D_sigmadiff->GetListOfFunctions()->FindObject("palette");
    palette->SetX1NDC(0.9);
    palette->SetX2NDC(0.92);
    palette->SetY1NDC(0.1);
    palette->SetY2NDC(0.9);
  }

  std::stringstream ss_sigmadiff;
  ss_sigmadiff<<outpath<<"PMT_2D_SigmaDifference.jpg";
  canvas_sigmadiff->SaveAs(ss_sigmadiff.str().c_str());
  Log("MonitorTankTime: Output path Pedestal Sigma Time Difference plot: "+ss_sigmadiff.str(),v_message,verbosity);

  //rate time difference plot

  std::vector<double> overall_ratediffs;
  overall_ratediffs.assign(num_active_slots*num_channels_tank,0.);

  if (rate_plot.size() >= 1){
    for (int i_ch = 0; i_ch < num_active_slots*num_channels_tank; i_ch++){
      overall_ratediffs.at(i_ch) = rate_plot.at(rate_plot.size()-1).at(i_ch) - rate_plot.at(0).at(i_ch);
    }
  }

  for (int i_slot = 0; i_slot < num_active_slots; i_slot++){
    std::vector<unsigned int> crateslot = map_slot_to_crateslot[i_slot];
    unsigned int i_crate = crateslot.at(0);
    for (int i_channel = 0; i_channel < num_channels_tank; i_channel++){
      int i_active = i_slot*num_channels_tank+i_channel;
      int x = i_slot+1;
      int y = (3-i_crate)*num_channels_tank - i_channel;      //top most crate is displayed at the top, then crate #2 in the middle and crate #3 at the bottom
      h2D_ratediff->SetBinContent(x,y,overall_ratediffs.at(i_active));
      if (overall_ratediffs.at(i_active) > max_ratediff) max_ratediff = overall_ratediffs.at(i_active);
      else if (overall_ratediffs.at(i_active) < min_ratediff) min_ratediff = overall_ratediffs.at(i_active);
    }
  }

  ss_title_ratediff.str("");
  ss_title_ratediff << title_time.str() << str_ratediff;
  h2D_ratediff->SetTitle(ss_title_ratediff.str().c_str());

  TPad *p_ratediff = (TPad*) canvas_ratediff->cd();
  h2D_ratediff->SetStats(0);
  h2D_ratediff->GetXaxis()->SetNdivisions(num_slots_tank);
  h2D_ratediff->GetYaxis()->SetNdivisions(num_crates_tank*num_channels_tank);
  p_ratediff->SetGrid();
  for (int i_label = 0; i_label < int(num_slots_tank); i_label++){
    std::stringstream ss_slot;
    ss_slot<<(i_label+1);
    std::string str_slot = "slot "+ss_slot.str();
    h2D_ratediff->GetXaxis()->SetBinLabel(i_label+1,str_slot.c_str());
  }
  for (int i_label=0; i_label < int(num_channels_tank*num_crates_tank); i_label++){
    std::stringstream ss_ch;
    if (i_label < 4) ss_ch<<((3-i_label)%4);
    else if (i_label < 8) ss_ch<<((7-i_label)%4);
    else ss_ch<<((11-i_label)%4);
    std::string str_ch = "ch "+ss_ch.str();
    h2D_ratediff->GetYaxis()->SetBinLabel(i_label+1,str_ch.c_str());
  }
  h2D_ratediff->LabelsOption("v");
  h2D_ratediff->Draw("colz");

  for (int i_box =0; i_box < vector_box_inactive.size(); i_box++){
    vector_box_inactive.at(i_box)->Draw("same");
  }

  line1->Draw("same");
  line2->Draw("same");
  text_crate1->Draw();
  text_crate2->Draw();
  text_crate3->Draw();
  label_ratediff->Draw();

  p_ratediff->Update();

  if (h2D_ratediff->GetMaximum()>0.){
    if (abs(max_ratediff-min_ratediff)==0) h2D_ratediff->GetZaxis()->SetRangeUser(min_ratediff-1,max_ratediff+1);
    else h2D_ratediff->GetZaxis()->SetRangeUser(min_ratediff-0.5,max_ratediff+0.5);
    TPaletteAxis *palette = 
    (TPaletteAxis*)h2D_ratediff->GetListOfFunctions()->FindObject("palette");
    palette->SetX1NDC(0.9);
    palette->SetX2NDC(0.92);
    palette->SetY1NDC(0.1);
    palette->SetY2NDC(0.9);
  }

  std::stringstream ss_ratediff;
  ss_ratediff<<outpath<<"PMT_2D_RateDifference.jpg";
  canvas_ratediff->SaveAs(ss_ratediff.str().c_str());
  Log("MonitorTankTime: Output path Rate Time Difference plot: "+ss_ratediff.str(),v_message,verbosity);

}

void MonitorTankTime::DrawBufferPlots(){

  Log("MonitorTankTime: DrawBufferPlots",v_message,verbosity);

  //-------------------------------------------------------
  //------------------DrawBufferPlots ---------------------
  //-------------------------------------------------------

  for (int i_slot = 0; i_slot<num_active_slots; i_slot++){

    int max_freq = 0;
    double max_temp = -999999.;
    double min_temp = 999999.;
    leg_temp->Clear();

    std::vector<unsigned int> crateslot = map_slot_to_crateslot[i_slot];
    int crate_num = crateslot.at(0);
    int slot_num = crateslot.at(1);

    canvas_Channels_temp.at(i_slot)->cd();
    canvas_Channels_temp.at(i_slot)->Clear();
    ss_title_hist_temp.str("");
    ss_title_hist_temp << title_time.str() << " Temp (VME Crate " << crate_num << " Slot " << slot_num <<")";
    hChannels_temp.at(i_slot*num_channels_tank)->SetTitle(ss_title_hist_temp.str().c_str());

    for (int i_channel = 0; i_channel < num_channels_tank; i_channel++){
      std::stringstream ss_channel;
      ss_channel << "Cr "<<crate_num<<"/Sl"<<slot_num<<"/Ch"<<i_channel;
      hChannels_temp.at(i_slot*num_channels_tank+i_channel)->Draw("same");
      if (hChannels_temp.at(i_slot*num_channels_tank+i_channel)->GetMaximum() > max_temp) max_temp = hChannels_temp.at(i_slot*num_channels_tank+i_channel)->GetMaximum();
      if (hChannels_temp.at(i_slot*num_channels_tank+i_channel)->GetMinimum() < min_temp) min_temp = hChannels_temp.at(i_slot*num_channels_tank+i_channel)->GetMinimum();
      leg_temp->AddEntry(hChannels_temp.at(i_slot*num_channels_tank+i_channel),ss_channel.str().c_str(),"l");
    }
    hChannels_temp.at(i_slot*num_channels_tank)->GetYaxis()->SetRangeUser(min_temp,max_temp);
    leg_temp->Draw();
    std::stringstream ss_canvas_temp;
    ss_canvas_temp << outpath << "PMT_Temp_Cr"<<crate_num<<"_Sl"<<slot_num<<".jpg";
    canvas_Channels_temp.at(i_slot)->SaveAs(ss_canvas_temp.str().c_str());

  }

  //Resetting & Clearing Channel histograms, canvasses
  for (int i_slot = 0; i_slot < num_active_slots; i_slot++){
    for (int i_channel = 0; i_channel < num_channels_tank; i_channel++){
      hChannels_temp.at(i_slot*num_channels_tank+i_channel)->Reset();
    }
  }

}

void MonitorTankTime::DrawADCFreqPlots(){

  Log("MonitorTankTime: DrawADCFreqPlots",v_message,verbosity);

  //-------------------------------------------------------
  //------------------DrawADCFreqPlots --------------------
  //-------------------------------------------------------

  std::stringstream ss_title_hist_freq;

  for (int i_slot = 0; i_slot<num_active_slots; i_slot++){

    int max_freq = 0;
    double max_temp = -999999.;
    double min_temp = 999999.;
    leg_freq->Clear();

    std::vector<unsigned int> crateslot = map_slot_to_crateslot[i_slot];
    int crate_num = crateslot.at(0);
    int slot_num = crateslot.at(1);

    canvas_Channels_freq.at(i_slot)->cd();
    canvas_Channels_freq.at(i_slot)->Clear();
    ss_title_hist_freq.str("");
    ss_title_hist_freq << title_time.str() << " Freq (VME Crate " << crate_num << " Slot " << slot_num <<")";
    hChannels_freq.at(i_slot*num_channels_tank)->SetTitle(ss_title_hist_freq.str().c_str());

    for (int i_channel = 0; i_channel < num_channels_tank; i_channel++){
      std::stringstream ss_channel;
      ss_channel << "Cr "<<crate_num<<"/Sl"<<slot_num<<"/Ch"<<i_channel;
      hChannels_freq.at(i_slot*num_channels_tank+i_channel)->Draw("same");
      if (hChannels_freq.at(i_slot*num_channels_tank+i_channel)->GetMaximum() > max_freq) max_freq = hChannels_freq.at(i_slot*num_channels_tank+i_channel)->GetMaximum();
      leg_freq->AddEntry(hChannels_freq.at(i_slot*num_channels_tank+i_channel),ss_channel.str().c_str());
    }
    hChannels_freq.at(i_slot*num_channels_tank)->GetYaxis()->SetRangeUser(0.,max_freq);
    leg_freq->Draw();
    std::stringstream ss_canvas_freq;
    ss_canvas_freq << outpath << "PMT_Freq_Cr"<<crate_num<<"_Sl"<<slot_num<<".jpg";
    canvas_Channels_freq.at(i_slot)->SaveAs(ss_canvas_freq.str().c_str());
  }

  //Delete gaussian fit functions after saving the histograms (with the fit functions drawn on top)
  for (int i_gaus=0; i_gaus < vector_gaus.size(); i_gaus++){
    delete vector_gaus.at(i_gaus);
  }
  vector_gaus.clear();

  for (int i_slot = 0; i_slot < num_active_slots; i_slot++){
    for (int i_channel = 0; i_channel < num_channels_tank; i_channel++){
      hChannels_freq.at(i_slot*num_channels_tank+i_channel)->Reset();
    }
  }


}

void MonitorTankTime::DrawFileHistory(ULong64_t timestamp_end, double time_frame, std::string file_ending){

  Log("MonitorTankTime: DrawFileHistory",v_message,verbosity);

  //-------------------------------------------------------
  //------------------DrawFileHistory ---------------------
  //-------------------------------------------------------

  //Creates a plot showing the time stamps for all the files within the last time_frame mins
  //The plot is updated with the update_frequency specified in the configuration file (default: 5 mins)

  if (timestamp_end != readfromfile_tend || time_frame != readfromfile_timeframe) ReadFromFile(timestamp_end, time_frame);

  ULong64_t timestamp_start = timestamp_end - time_frame*MSEC_to_SEC*SEC_to_MIN*MIN_to_HOUR;

  canvas_logfile->cd();
  log_files->SetBins(num_files_history,timestamp_start/MSEC_to_SEC,timestamp_end/MSEC_to_SEC);
  log_files->GetXaxis()->SetTimeOffset(0.);
  log_files->Draw();

  std::vector<TLine*> file_markers;
  for (int i_file = 0; i_file < tend_plot.size(); i_file++){
    TLine *line_file = new TLine(tend_plot.at(i_file)/MSEC_to_SEC,0.,tend_plot.at(i_file)/MSEC_to_SEC,1.);
    line_file->SetLineColor(1);
    line_file->SetLineStyle(1);
    line_file->SetLineWidth(1);
    line_file->Draw("same"); 
    file_markers.push_back(line_file);
  }

  std::stringstream ss_logfiles;
  ss_logfiles << outpath << "Tank_FileHistory_" << file_ending << "." << img_extension;
  canvas_logfile->SaveAs(ss_logfiles.str().c_str());

  for (int i_line = 0; i_line < file_markers.size(); i_line++){
    delete file_markers.at(i_line);
  }

  log_files->Reset();
  canvas_logfile->Clear();


}

void MonitorTankTime::CardIDToElectronicsSpace(int CardID, 
        int &CrateNum, int &SlotNum)
{
  //CardID = CrateNum * 1000 + SlotNum.  This logic works if we have less than
  // 10 crates and less than 100 Slots (which we do).
  SlotNum = CardID % 100;
  CrateNum = CardID / 1000;
  return;
}

std::string MonitorTankTime::convertTimeStamp_to_Date(ULong64_t timestamp){

  //format of date is YYYY_MM-DD

  boost::posix_time::ptime convertedtime = *Epoch + boost::posix_time::time_duration(int(timestamp/MSEC_to_SEC/SEC_to_MIN/MIN_to_HOUR),int(timestamp/MSEC_to_SEC/SEC_to_MIN)%60,int(timestamp/MSEC_to_SEC/1000.)%60,timestamp%1000);
  struct tm converted_tm = boost::posix_time::to_tm(convertedtime);
  std::stringstream ss_date;
  ss_date << converted_tm.tm_year+1900 << "_" << converted_tm.tm_mon+1 << "-" <<converted_tm.tm_mday;
  return ss_date.str();

}


bool MonitorTankTime::does_file_exist(std::string filename){

  std::ifstream infile(filename.c_str());
  bool file_good = infile.good();
  infile.close();
  return file_good;

}


