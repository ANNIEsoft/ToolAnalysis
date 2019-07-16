#include "MonitorMRDTime.h"

MonitorMRDTime::MonitorMRDTime():Tool(){}



bool MonitorMRDTime::Initialise(std::string configfile, DataModel &data){

  if (verbosity > 1) std::cout <<"Tool MonitorMRDTime: Initialising...."<<std::endl;

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
  m_variables.Get("OffsetDate",offset_date);          //temporary time offset to work with the test mrd output
  m_variables.Get("Mode",mode);
  m_variables.Get("DrawMarker",draw_marker);
  m_variables.Get("ScatterPlots",draw_scatter);
  m_variables.Get("AveragePlots",draw_average);
  m_variables.Get("HitMapPlots",draw_hitmap);
  m_variables.Get("EvolutionHour",draw_hour);
  m_variables.Get("EvolutionSixHour",draw_sixhour);
  m_variables.Get("EvolutionDay",draw_day);
  m_variables.Get("OnlyVitalPlots",draw_vital);

  if (mode != "Continuous" && mode != "FileList") mode = "Continuous";

  Epoch = new boost::posix_time::ptime(boost::gregorian::from_string(StartTime));
  if (verbosity > 2) std::cout <<"MRDMonitorTime: Outpath (temporary): "<<outpath_temp<<std::endl;
  if (outpath_temp == "fromStore") m_data->CStore.Get("OutPath",outpath);
  else outpath = outpath_temp;
  if (verbosity > 2) std::cout <<"MRDMonitorTime: Output path for plots is "<<outpath<<std::endl;

  num_active_slots=0;
  num_active_slots_cr1=0;
  num_active_slots_cr2=0;
  max_files=0;

  //-------------------------------------------------------
  //-----------------Get active channels-------------------
  //-------------------------------------------------------

  ifstream file(active_slots.c_str());
  int temp_crate, temp_slot;
  int loopnum=0;
  while (!file.eof()){
    file>>temp_crate>>temp_slot;
    loopnum++;
    //std::cout<<loopnum<<" , "<<temp_crate<<" , "<<temp_slot<<std::endl;
    if (file.eof()) break;
    //if (loopnum!=13){
      if (temp_crate-min_crate<0 || temp_crate-min_crate>=num_crates) {
        std::cout <<"ERROR (MonitorMRDTime): Specified crate out of range [7...8]. Continue with next entry."<<std::endl;
        continue;
      }
      if (temp_slot<1 || temp_slot>num_slots){
        std::cout <<"ERROR (MonitorMRDTime): Specified slot out of range [1...24]. Continue with next entry."<<std::endl;
        continue;
      }
      active_channel[temp_crate-min_crate][temp_slot-1]=1;		//slot start with number 1 instead of 0, crates with 7
      nr_slot.push_back((temp_crate-min_crate)*100+(temp_slot)); 
      num_active_slots++;
      if (temp_crate-min_crate==0) {num_active_slots_cr1++;active_slots_cr1.push_back(temp_slot);}
      if (temp_crate-min_crate==1) {num_active_slots_cr2++;active_slots_cr2.push_back(temp_slot);}
    //}
  }
  file.close();

  //-------------------------------------------------------
  //----------Initialize storing containers----------------
  //-------------------------------------------------------

  InitializeVectors();

  //-------------------------------------------------------
  //---------------Set initial conditions------------------
  //-------------------------------------------------------

  data_available = false;
  if (mode == "FileList"){      //plot more quickly for FileList, don't wait every 5 mins
    j_fivemin = 11;
    initial = false;
  } else if (mode == "Continuous"){
    j_fivemin = 0;
    initial = true;
  }
  //update_mins = true;
  enum_slots = 0;
  i_file_fivemin = 0;

  t_file_start_previous = 0;
  t_file_end_previous = 0;

  //-------------------------------------------------------
  //------Setup time variables for periodic updates--------
  //-------------------------------------------------------

  period_update = boost::posix_time::time_duration(0,5,0,0);
  last = boost::posix_time::ptime(boost::posix_time::second_clock::local_time());

  //counting number of iterations
  i_loop = 0;
  //omit warning messages from ROOT: 1001 - info messages, 2001 - warnings, 3001 - errors
  gROOT->ProcessLine("gErrorIgnoreLevel = 3001;");
  
  return true;
}


bool MonitorMRDTime::Execute(){

  if (verbosity > 3) std::cout <<"Tool MonitorMRDTime: Executing ...."<<std::endl;

  if (mode == "FileList"){
    current = last + boost::posix_time::time_duration(0,5,0,0);
    duration = boost::posix_time::time_duration(0,5,0,0);	
  } else if (mode == "Continuous"){
    current = (boost::posix_time::second_clock::local_time());
    duration = boost::posix_time::time_duration(current - last); 
  }

  if (verbosity > 2) std::cout <<"MRDMonitorTime: "<<duration.total_milliseconds()/1000./60.<<" mins since last time plot"<<std::endl;

  std::string State;
  m_data->CStore.Get("State",State);

   if (State == "MRDSingle" || State == "Wait"){

    //-------------------------------------------------------
    //--------------MRDMonitorLive executed------------------
    //-------------------------------------------------------

    if (verbosity > 3) std::cout <<"MRDMonitorTime: State is "<<State<<std::endl;

   }else if (State == "DataFile"){

    //-------------------------------------------------------
    //--------------MRDMonitorTime executed------------------
    //-------------------------------------------------------

   	if (verbosity > 1) std::cout<<"MRDMonitorTime: New data file available."<<std::endl;
   	m_data->Stores["CCData"]->Get("FileData",MRDdata);
    MRDdata->Print(false);
   	data_available = true;
    omit_entries=false;

    FillEvents();
    t_file_start_previous=t_file_start;
    t_file_end_previous=t_file_end;
    i_file_fivemin++;


   }else {

   	if (verbosity > 1) std::cout <<"MRDMonitorTime: State not recognized: "<<State<<std::endl;

   }

  //-------------------------------------------------------
  //-----------Has enough time passed for update?----------
  //-------------------------------------------------------

  if(duration>=period_update){
    if (verbosity > 1) std::cout <<"MRDMonitorTime: 5 mins passed... Updating plots!"<<std::endl;
    update_mins=true;
  	j_fivemin++;
    last=current;
    MonitorMRDTime::UpdateMonitorSources();
    data_available = false;
    if (j_fivemin%12 == 0) j_fivemin = 0;
    max_files=0;
    initial=false;
    i_file_fivemin=0;
    if (verbosity > 1) std::cout <<"-----------------------------------------------------------------------------------"<<std::endl; //marking end of one plotting period
	}
  
  i_loop++;

    //only for debugging memory leaks, otherwise comment out
  //std::cout <<"List of Objects (after execute step): "<<std::endl;
  //gObjectTable->Print();

  return true;

}


bool MonitorMRDTime::Finalise(){

  if (verbosity > 1) std::cout <<"Tool MonitorMRDTime: Finalising ...."<<std::endl;

  MRDdata->Delete();

  //delete all the pointer to objects that are still active
  delete Epoch;
  delete hist_hitmap_fivemin_cr1;
  delete hist_hitmap_fivemin_cr2;
  delete hist_hitmap_hour_cr1;
  delete hist_hitmap_hour_cr2;
  delete hist_hitmap_sixhour_cr1;
  delete hist_hitmap_sixhour_cr2;
  delete hist_hitmap_day_cr1;
  delete hist_hitmap_day_cr2;

  for (int i_gr=0; i_gr < gr_times_hour.size(); i_gr++){
    delete gr_times_hour.at(i_gr);
    delete gr_rms_hour.at(i_gr);
    delete gr_frequency_hour.at(i_gr);
    delete gr_times_sixhour.at(i_gr);
    delete gr_rms_sixhour.at(i_gr);
    delete gr_frequency_sixhour.at(i_gr);
    delete gr_times_day.at(i_gr);
    delete gr_rms_day.at(i_gr);
    delete gr_frequency_day.at(i_gr);
    delete hist_times_hour.at(i_gr);
    delete hist_times_sixhour.at(i_gr);
    delete hist_times_day.at(i_gr);
  }
  for (int i_gr_slot=0; i_gr_slot < gr_slot_times_hour.size(); i_gr_slot++){
    delete gr_slot_times_hour.at(i_gr_slot);
    delete gr_slot_rms_hour.at(i_gr_slot);
    delete gr_slot_frequency_hour.at(i_gr_slot);  
    delete gr_slot_times_sixhour.at(i_gr_slot);
    delete gr_slot_rms_sixhour.at(i_gr_slot);
    delete gr_slot_frequency_sixhour.at(i_gr_slot);
    delete gr_slot_times_day.at(i_gr_slot);
    delete gr_slot_rms_day.at(i_gr_slot);
    delete gr_slot_frequency_day.at(i_gr_slot); 
  }
  for (int i_gr_crate = 0; i_gr_crate < gr_crate_times_hour.size(); i_gr_crate++){
    delete gr_crate_times_hour.at(i_gr_crate);
    delete gr_crate_rms_hour.at(i_gr_crate);
    delete gr_crate_frequency_hour.at(i_gr_crate);
    delete gr_crate_times_sixhour.at(i_gr_crate);
    delete gr_crate_rms_sixhour.at(i_gr_crate);
    delete gr_crate_frequency_sixhour.at(i_gr_crate);
    delete gr_crate_times_day.at(i_gr_crate);
    delete gr_crate_rms_day.at(i_gr_crate);
    delete gr_crate_frequency_day.at(i_gr_crate); 

  }
  for (int i_hist=0; i_hist < hist_hitmap_fivemin_Channel.size(); i_hist++){
    delete hist_hitmap_fivemin_Channel.at(i_hist);
    delete hist_hitmap_hour_Channel.at(i_hist);
    delete hist_hitmap_sixhour_Channel.at(i_hist);
    delete hist_hitmap_day_Channel.at(i_hist);
  }

  return true;
}

void MonitorMRDTime::InitializeVectors(){

  //-------------------------------------------------------
  //-----------------InitializeVectors---------------------
  //-------------------------------------------------------

  if (verbosity > 2) std::cout <<"MonitorMRDTime: Initialize Vectors..."<<std::endl;

  hist_hitmap_fivemin_cr1 = new TH1F("hist_hitmap_fivemin_cr1","Hitmap last 5 mins Rack 7",num_channels*num_active_slots,0,num_channels*num_active_slots);
  hist_hitmap_fivemin_cr2 = new TH1F("hist_hitmap_fivemin_cr2","Hitmap last 5 mins Rack 8",num_channels*num_active_slots,0,num_channels*num_active_slots);
  hist_hitmap_hour_cr1 = new TH1F("hist_hitmap_hour_cr1","Hitmap last hour Rack 7",num_channels*num_active_slots,0,num_channels*num_active_slots);
  hist_hitmap_hour_cr2 = new TH1F("hist_hitmap_hour_cr2","Hitmap last hour Rack 8",num_channels*num_active_slots,0,num_channels*num_active_slots);
  hist_hitmap_sixhour_cr1 = new TH1F("hist_hitmap_sixhour_cr1","Hitmap last 6 hours Rack 7",num_channels*num_active_slots,0,num_channels*num_active_slots);
  hist_hitmap_sixhour_cr2 = new TH1F("hist_hitmap_sixhour_cr2","Hitmap last 6 hours Rack 7",num_channels*num_active_slots,0,num_channels*num_active_slots);
  hist_hitmap_day_cr1 = new TH1F("hist_hitmap_day_cr1","Hitmap last day Rack 7",num_channels*num_active_slots,0,num_channels*num_active_slots);
  hist_hitmap_day_cr2 = new TH1F("hist_hitmap_day_cr2","Hitmap last day Rack 8",num_channels*num_active_slots,0,num_channels*num_active_slots);
  hist_hitmap_fivemin_cr1->SetLineColor(8);
  hist_hitmap_fivemin_cr2->SetLineColor(9);
  hist_hitmap_hour_cr1->SetLineColor(8);
  hist_hitmap_hour_cr2->SetLineColor(9);
  hist_hitmap_sixhour_cr1->SetLineColor(8);
  hist_hitmap_sixhour_cr2->SetLineColor(9);
  hist_hitmap_day_cr1->SetLineColor(8);
  hist_hitmap_day_cr2->SetLineColor(9);
  hist_hitmap_fivemin_cr1->SetFillColor(8);
  hist_hitmap_fivemin_cr2->SetFillColor(9);
  hist_hitmap_hour_cr1->SetFillColor(8);
  hist_hitmap_hour_cr2->SetFillColor(9);
  hist_hitmap_sixhour_cr1->SetFillColor(8);
  hist_hitmap_sixhour_cr2->SetFillColor(9);
  hist_hitmap_day_cr1->SetFillColor(8);
  hist_hitmap_day_cr2->SetFillColor(9);  
  hist_hitmap_fivemin_cr1->SetStats(0);
  hist_hitmap_fivemin_cr2->SetStats(0);
  hist_hitmap_hour_cr1->SetStats(0);
  hist_hitmap_hour_cr2->SetStats(0);
  hist_hitmap_sixhour_cr1->SetStats(0);
  hist_hitmap_sixhour_cr2->SetStats(0);
  hist_hitmap_day_cr1->SetStats(0);
  hist_hitmap_day_cr2->SetStats(0);
  std::vector<double> empty_vec;
  int sum_channels=0;

  //-------------------------------------------------------
  //--------Initialise all storing containers--------------
  //-------------------------------------------------------

  for (int i_crate = 0; i_crate < num_crates; i_crate++){

    std::array<double, num_fivemin> tdc_fivemin = {0};
    std::array<double, num_halfhour> tdc_halfhour = {0};
    std::array<double, num_hour> tdc_hour = {0};
    std::array<long, num_fivemin> n_fivemin = {0};
    std::array<long, num_halfhour> n_halfhour = {0};
    std::array<long, num_hour> n_hour = {0};
    std::array<long, num_overall_fivemin> n_overall_day = {0};
    std::vector<long> empty_long_vec;
    std::stringstream ss_crate;
    ss_crate << i_crate+min_crate;

    for (int i_slot = 0; i_slot < num_slots; i_slot++){
      std::stringstream ss_slot;
      ss_slot << i_slot+1;

      if (active_channel[i_crate][i_slot]==0) continue;     //don't plot values for slots that are not active

      for (int i_channel = 0; i_channel < num_channels; i_channel++){
        std::stringstream ss_ch;
        ss_ch << i_channel;
        times_channel_hour.push_back(tdc_fivemin);
        times_channel_sixhour.push_back(tdc_halfhour);
        times_channel_day.push_back(tdc_hour);
        rms_channel_hour.push_back(tdc_fivemin);
        rms_channel_sixhour.push_back(tdc_halfhour);
        rms_channel_day.push_back(tdc_hour);
        frequency_channel_hour.push_back(tdc_fivemin);
        frequency_channel_sixhour.push_back(tdc_halfhour);
        frequency_channel_day.push_back(tdc_hour);
        n_channel_hour.push_back(n_fivemin);
        n_channel_sixhour.push_back(n_halfhour);
        n_channel_day.push_back(n_hour);
        n_channel_overall_day.push_back(n_overall_day);
        earliest_channel_hour.push_back(n_fivemin);
        earliest_channel_sixhour.push_back(n_halfhour);
        earliest_channel_day.push_back(n_hour);
        latest_channel_hour.push_back(n_fivemin);
        latest_channel_sixhour.push_back(n_halfhour);
        latest_channel_day.push_back(n_hour);
        mapping_vector_ch.push_back(i_crate*num_slots+i_slot*num_channels+i_channel);     //correspondence entries to channel numbers
        live_mins.push_back(empty_vec);
        timestamp_mins.push_back(empty_vec);
        live_halfhour.push_back(empty_vec);
        timestamp_halfhour.push_back(empty_vec);
        live_hour.push_back(empty_vec);
        timestamp_hour.push_back(empty_vec);
        live_file.push_back(empty_vec);
        n_live_file.push_back(empty_vec);
        timestamp_file.push_back(empty_vec);
        timediff_file.push_back(empty_vec);
        times_channel_overall_hour.push_back(empty_long_vec);
        stamp_channel_overall_hour.push_back(empty_long_vec);
        times_channel_overall_sixhour.push_back(empty_long_vec);
        stamp_channel_overall_sixhour.push_back(empty_long_vec);
        times_channel_overall_day.push_back(empty_long_vec);
        stamp_channel_overall_day.push_back(empty_long_vec);
        std::string name_graph = "cr"+ss_crate.str()+"_sl"+ss_slot.str()+"_ch"+ss_ch.str();
        std::string name_hist = name_graph+"_hist";
        std::string name_hist_sixhour = name_hist+"_sixhour";
        std::string name_hist_day = name_hist+"_day";
        std::string name_graph_rms = name_graph+"_rms";
        std::string name_graph_freq = name_graph+"_freq";
        std::string name_graph_sixhour = name_graph+"_6h";
        std::string name_graph_sixhour_rms = name_graph+"_6h_rms";
        std::string name_graph_sixhour_freq = name_graph+"_6h_freq";
        std::string name_graph_day = name_graph+"_day";
        std::string name_graph_day_rms = name_graph+"_day_rms";
        std::string name_graph_day_freq = name_graph+"_day_freq";
        std::string title_graph = "Crate "+ss_crate.str()+", Slot "+ss_slot.str()+", Channel "+ss_ch.str();
        std::string title_graph_rms = title_graph+" (RMS)";
        std::string title_graph_freq = title_graph+" (Freq)";
        std::string title_hist = title_graph+" (scatter)";
        std::string title_hist_sixhour = title_hist+" [6 hours]";
        std::string title_hist_day = title_hist+" [day]";
        TGraph *graph_ch = new TGraph();
        TGraph *graph_ch_rms = new TGraph();
        TGraph *graph_ch_freq = new TGraph();
        TGraph *graph_ch_sixhour = new TGraph();
        TGraph *graph_ch_sixhour_rms = new TGraph();
        TGraph *graph_ch_sixhour_freq = new TGraph();
        TGraph *graph_ch_day = new TGraph();
        TGraph *graph_ch_day_rms = new TGraph();
        TGraph *graph_ch_day_freq = new TGraph();
        TH2F *hist_ch_tdc = new TH2F(name_hist.c_str(),title_hist.c_str(),200,0,3600,500,0,500);          //one point every 30 secs
        TH2F *hist_ch_tdc_sixhour = new TH2F(name_hist_sixhour.c_str(),title_hist.c_str(),360,0,21600,500,0,500); //one point every 1 mins
        TH2F *hist_ch_tdc_day = new TH2F(name_hist_day.c_str(),title_hist.c_str(),288,0,86400,500,0,50);  //one point every 5 mins
        graph_ch->SetName(name_graph.c_str());
        graph_ch->SetTitle(title_graph.c_str());
        hist_ch_tdc->SetName(name_hist.c_str());
        hist_ch_tdc->SetTitle(title_hist.c_str());
        hist_ch_tdc_sixhour->SetName(name_hist_sixhour.c_str());
        hist_ch_tdc_sixhour->SetTitle(title_hist_sixhour.c_str());
        hist_ch_tdc_day->SetName(name_hist_day.c_str());
        hist_ch_tdc_day->SetTitle(title_hist_day.c_str());
        //int line_color_ch = (i_channel%(num_channels/2)<9)? i_channel%(num_channels/2)+1 : (i_channel%(num_channels/2)-9)+40;       //not really happy with this, colors > 40 very hard to distinguish
        //int line_color_ch = (i_channel%4)+1;  //on each canvas 4 channels
        int line_color_ch = color_scheme[i_channel%(num_channels/2)];
        if (draw_marker) {
          graph_ch->SetMarkerStyle(20);
          graph_ch->SetMarkerColor(line_color_ch);
        }
        graph_ch->SetLineColor(line_color_ch);
        graph_ch->SetLineWidth(2);
        graph_ch->SetFillColor(0);
        hist_ch_tdc->SetMarkerColor(line_color_ch);
        hist_ch_tdc->SetFillColor(0);
        hist_ch_tdc->GetYaxis()->SetTitle("TDC");
        hist_ch_tdc->SetStats(0);
        hist_ch_tdc->SetMarkerStyle(20);
        hist_ch_tdc->SetMarkerSize(0.3);
        hist_ch_tdc_sixhour->SetMarkerColor(line_color_ch);
        hist_ch_tdc_sixhour->SetFillColor(0);
        hist_ch_tdc_sixhour->GetYaxis()->SetTitle("TDC");
        hist_ch_tdc_sixhour->SetStats(0);
        hist_ch_tdc_sixhour->SetMarkerStyle(20);
        hist_ch_tdc_sixhour->SetMarkerSize(0.3);
        hist_ch_tdc_day->SetMarkerColor(line_color_ch);
        hist_ch_tdc_day->SetFillColor(0);
        hist_ch_tdc_day->GetYaxis()->SetTitle("TDC");
        hist_ch_tdc_day->SetStats(0);
        hist_ch_tdc_day->SetMarkerStyle(20);
        hist_ch_tdc_day->SetMarkerSize(0.3);
        graph_ch_rms->SetName(name_graph_rms.c_str());
        graph_ch_rms->SetTitle(title_graph_rms.c_str());
        if (draw_marker) {
          graph_ch_rms->SetMarkerStyle(20);
          graph_ch_rms->SetMarkerColor(line_color_ch);
        }
        graph_ch_rms->SetLineColor(line_color_ch);
        graph_ch_rms->SetLineWidth(2);
        graph_ch_rms->SetFillColor(0);
        graph_ch_freq->SetName(name_graph_freq.c_str());
        graph_ch_freq->SetTitle(title_graph_freq.c_str());
        if (draw_marker) {
          graph_ch_freq->SetMarkerStyle(20);
          graph_ch_freq->SetMarkerColor(line_color_ch);
        }
        graph_ch_freq->SetLineColor(line_color_ch);
        graph_ch_freq->SetLineWidth(2);
        graph_ch_freq->SetFillColor(0);
        graph_ch_sixhour->SetName(name_graph_sixhour.c_str());
        graph_ch_sixhour->SetTitle(title_graph.c_str());
        if (draw_marker) {
          graph_ch_sixhour->SetMarkerStyle(20);
          graph_ch_sixhour->SetMarkerColor(line_color_ch);
        }
        graph_ch_sixhour->SetLineColor(line_color_ch);
        graph_ch_sixhour->SetLineWidth(2);
        graph_ch_sixhour->SetFillColor(0);
        graph_ch_sixhour_rms->SetName(name_graph_sixhour_rms.c_str());
        graph_ch_sixhour_rms->SetTitle(title_graph_rms.c_str());
        if (draw_marker) {
          graph_ch_sixhour_rms->SetMarkerStyle(20);
          graph_ch_sixhour_rms->SetMarkerColor(line_color_ch);
        }
        graph_ch_sixhour_rms->SetLineColor(line_color_ch);
        graph_ch_sixhour_rms->SetLineWidth(2);
        graph_ch_sixhour_rms->SetFillColor(0);
        graph_ch_sixhour_freq->SetName(name_graph_sixhour_freq.c_str());
        graph_ch_sixhour_freq->SetTitle(title_graph_freq.c_str());
        if (draw_marker) {
          graph_ch_sixhour_freq->SetMarkerStyle(20);
          graph_ch_sixhour_freq->SetMarkerColor(line_color_ch);
        }
        graph_ch_sixhour_freq->SetLineColor(line_color_ch);
        graph_ch_sixhour_freq->SetLineWidth(2);
        graph_ch_sixhour_freq->SetFillColor(0);
        graph_ch_day->SetName(name_graph_day.c_str());
        graph_ch_day->SetTitle(title_graph.c_str());
        graph_ch_day->SetLineColor(line_color_ch);
        if (draw_marker) {
          graph_ch_day->SetMarkerStyle(20);
          graph_ch_day->SetMarkerColor(line_color_ch);
        }
        graph_ch_day->SetLineWidth(2);
        graph_ch_day->SetFillColor(0);
        graph_ch_day_rms->SetName(name_graph_day_rms.c_str());
        graph_ch_day_rms->SetTitle(title_graph_rms.c_str());
        if (draw_marker) {
          graph_ch_day_rms->SetMarkerStyle(20);
          graph_ch_day_rms->SetMarkerColor(line_color_ch);
        }
        graph_ch_day_rms->SetLineColor(line_color_ch);
        graph_ch_day_rms->SetLineWidth(2);
        graph_ch_day_rms->SetFillColor(0);
        graph_ch_day_freq->SetName(name_graph_day_freq.c_str());
        graph_ch_day_freq->SetTitle(title_graph_freq.c_str());
        if (draw_marker) {
          graph_ch_day_freq->SetMarkerStyle(20);
          graph_ch_day_freq->SetMarkerColor(line_color_ch);
        }
        graph_ch_day_freq->SetLineColor(line_color_ch);
        graph_ch_day_freq->SetLineWidth(2);
        graph_ch_day_freq->SetFillColor(0);
        gr_times_hour.push_back(graph_ch);
        gr_rms_hour.push_back(graph_ch_rms);
        gr_frequency_hour.push_back(graph_ch_freq);
        gr_times_sixhour.push_back(graph_ch_sixhour);
        gr_rms_sixhour.push_back(graph_ch_sixhour_rms);
        gr_frequency_sixhour.push_back(graph_ch_sixhour_freq);
        gr_times_day.push_back(graph_ch_day);
        gr_rms_day.push_back(graph_ch_day_rms);
        gr_frequency_day.push_back(graph_ch_day_freq);
        hist_times_hour.push_back(hist_ch_tdc);
        hist_times_sixhour.push_back(hist_ch_tdc_sixhour);
        hist_times_day.push_back(hist_ch_tdc_day);
        }

        times_slot_hour.push_back(tdc_fivemin);
        times_slot_sixhour.push_back(tdc_halfhour);
        times_slot_day.push_back(tdc_hour);
        rms_slot_hour.push_back(tdc_fivemin);
        rms_slot_sixhour.push_back(tdc_halfhour);
        rms_slot_day.push_back(tdc_hour);
        frequency_slot_hour.push_back(tdc_fivemin);
        frequency_slot_sixhour.push_back(tdc_halfhour);
        frequency_slot_day.push_back(tdc_hour);
        n_slot_hour.push_back(n_fivemin);
        n_slot_sixhour.push_back(n_halfhour);
        n_slot_day.push_back(n_hour);
        std::string name_graph = "cr"+ss_crate.str()+"_sl"+ss_slot.str();
        std::string name_graph_rms = name_graph+"_rms";
        std::string name_graph_freq = name_graph+"_freq";
        std::string name_graph_sixhour = name_graph+"_6h";
        std::string name_graph_sixhour_rms = name_graph+"_6h_rms";
        std::string name_graph_sixhour_freq = name_graph+"_6h_freq";
        std::string name_graph_day = name_graph+"_day";
        std::string name_graph_day_rms = name_graph+"_day_rms";
        std::string name_graph_day_freq = name_graph+"_day_freq";
        std::string title_graph = "slot "+ss_slot.str();
        std::string title_graph_rms = title_graph+" (RMS)";
        std::string title_graph_freq = title_graph+" (Freq)";
        TGraph *graph_slot = new TGraph();
        TGraph *graph_slot_rms = new TGraph();
        TGraph *graph_slot_freq = new TGraph();
        TGraph *graph_slot_sixhour = new TGraph();
        TGraph *graph_slot_sixhour_rms = new TGraph();
        TGraph *graph_slot_sixhour_freq = new TGraph();
        TGraph *graph_slot_day = new TGraph();
        TGraph *graph_slot_day_rms = new TGraph();
        TGraph *graph_slot_day_freq = new TGraph();
        graph_slot->SetName(name_graph.c_str());
        graph_slot->SetTitle(title_graph.c_str());
        if (draw_marker) {
          graph_slot->SetMarkerStyle(20);
          graph_slot->SetMarkerColor(enum_slots%num_active_slots_cr1+1);
        }
        graph_slot->SetLineColor(enum_slots%num_active_slots_cr1+1);
        graph_slot->SetLineWidth(2);
        graph_slot->SetFillColor(0);
        graph_slot_rms->SetName(name_graph_rms.c_str());
        graph_slot_rms->SetTitle(title_graph.c_str());
        if (draw_marker) {
          graph_slot_rms->SetMarkerStyle(20);
          graph_slot_rms->SetMarkerColor(enum_slots%num_active_slots_cr1+1);
        }
        graph_slot_rms->SetLineColor(enum_slots%num_active_slots_cr1+1);
        graph_slot_rms->SetLineWidth(2);
        graph_slot_rms->SetFillColor(0);
        graph_slot_freq->SetName(name_graph_freq.c_str());
        graph_slot_freq->SetTitle(title_graph.c_str());
        if (draw_marker) {
          graph_slot_freq->SetMarkerStyle(20);
          graph_slot_freq->SetMarkerColor(enum_slots%num_active_slots_cr1+1);
        }
        graph_slot_freq->SetLineColor(enum_slots%num_active_slots_cr1+1);
        graph_slot_freq->SetLineWidth(2);
        graph_slot_freq->SetFillColor(0);
        graph_slot_sixhour->SetName(name_graph_sixhour.c_str());
        graph_slot_sixhour->SetTitle(title_graph.c_str());
        if (draw_marker) {
          graph_slot_sixhour->SetMarkerStyle(20);
          graph_slot_sixhour->SetMarkerColor(enum_slots%num_active_slots_cr1+1);
        }
        graph_slot_sixhour->SetLineColor(enum_slots%num_active_slots_cr1+1);
        graph_slot_sixhour->SetLineWidth(2);
        graph_slot_sixhour->SetFillColor(0);
        graph_slot_sixhour_rms->SetName(name_graph_sixhour_rms.c_str());
        graph_slot_sixhour_rms->SetTitle(title_graph.c_str());
        if (draw_marker) {
          graph_slot_sixhour_rms->SetMarkerStyle(20);
          graph_slot_sixhour_rms->SetMarkerColor(enum_slots%num_active_slots_cr1+1);
        }
        graph_slot_sixhour_rms->SetLineColor(enum_slots%num_active_slots_cr1+1);
        graph_slot_sixhour_rms->SetLineWidth(2);
        graph_slot_sixhour_rms->SetFillColor(0);
        graph_slot_sixhour_freq->SetName(name_graph_sixhour_freq.c_str());
        graph_slot_sixhour_freq->SetTitle(title_graph.c_str());
        if (draw_marker) {
          graph_slot_sixhour_freq->SetMarkerStyle(20);
          graph_slot_sixhour_freq->SetMarkerColor(enum_slots%num_active_slots_cr1+1);
        }
        graph_slot_sixhour_freq->SetLineColor(enum_slots%num_active_slots_cr1+1);
        graph_slot_sixhour_freq->SetLineWidth(2);
        graph_slot_sixhour_freq->SetFillColor(0);
        graph_slot_day->SetName(name_graph_day.c_str());
        graph_slot_day->SetTitle(title_graph.c_str());
        if (draw_marker) {
          graph_slot_day->SetMarkerStyle(20);
          graph_slot_day->SetMarkerColor(enum_slots%num_active_slots_cr1+1);
        }
        graph_slot_day->SetLineColor(enum_slots%num_active_slots_cr1+1);
        graph_slot_day->SetLineWidth(2);
        graph_slot_day->SetFillColor(0);
        graph_slot_day_rms->SetName(name_graph_day_rms.c_str());
        graph_slot_day_rms->SetTitle(title_graph.c_str());
        if (draw_marker) {
          graph_slot_day_rms->SetMarkerStyle(20);
          graph_slot_day_rms->SetMarkerColor(enum_slots%num_active_slots_cr1+1);
        }
        graph_slot_day_rms->SetLineColor(enum_slots%num_active_slots_cr1+1);
        graph_slot_day_rms->SetLineWidth(2);
        graph_slot_day_rms->SetFillColor(0);
        graph_slot_day_freq->SetName(name_graph_day_freq.c_str());
        graph_slot_day_freq->SetTitle(title_graph.c_str());
        if (draw_marker) {
          graph_slot_day_freq->SetMarkerStyle(20);
          graph_slot_day_freq->SetMarkerColor(enum_slots%num_active_slots_cr1+1);
        }
        graph_slot_day_freq->SetLineColor(enum_slots%num_active_slots_cr1+1);
        graph_slot_day_freq->SetLineWidth(2);
        graph_slot_day_freq->SetFillColor(0);
        gr_slot_times_hour.push_back(graph_slot);
        gr_slot_rms_hour.push_back(graph_slot_rms);
        gr_slot_frequency_hour.push_back(graph_slot_freq);  
        gr_slot_times_sixhour.push_back(graph_slot_sixhour);
        gr_slot_rms_sixhour.push_back(graph_slot_sixhour_rms);
        gr_slot_frequency_sixhour.push_back(graph_slot_sixhour_freq);
        gr_slot_times_day.push_back(graph_slot_day);
        gr_slot_rms_day.push_back(graph_slot_day_rms);
        gr_slot_frequency_day.push_back(graph_slot_day_freq); 
        std::string str_day = " (last day)";
        std::string str_sixhour = " (last 6 hours)";
        std::string str_hour = " (last hour)";
        std::string str_fivemin = " (last 5 mins)";
        std::string name_fivemin = "_fivemin";
        std::string name_hour = "_hour";
        std::string name_sixhour = "_sixhour";
        std::string name_day = "_day";
        std::string title_hist_fivemin = "Crate "+ss_crate.str()+" slot "+ss_slot.str()+str_fivemin;
        std::string title_hist_hour = "Crate "+ss_crate.str()+" slot "+ss_slot.str()+str_hour;
        std::string title_hist_sixhour = "Crate "+ss_crate.str()+" slot "+ss_slot.str()+str_sixhour;
        std::string title_hist_day = "Crate "+ss_crate.str()+" slot "+ss_slot.str()+str_day;
        std::string name_hist_fivemin = "crate"+ss_crate.str()+"_slot"+ss_slot.str()+name_fivemin;
        std::string name_hist_hour = "crate"+ss_crate.str()+"_slot"+ss_slot.str()+name_hour;
        std::string name_hist_sixhour = "crate"+ss_crate.str()+"_slot"+ss_slot.str()+name_sixhour;
        std::string name_hist_day = "crate"+ss_crate.str()+"_slot"+ss_slot.str()+name_day;
        TH1I *hist_Slot_fivemin = new TH1I(name_hist_fivemin.c_str(),title_hist_fivemin.c_str(),num_channels,0,num_channels);
        TH1I *hist_Slot_hour = new TH1I(name_hist_hour.c_str(),title_hist_hour.c_str(),num_channels,0,num_channels);
        TH1I *hist_Slot_sixhour = new TH1I(name_hist_sixhour.c_str(),title_hist_sixhour.c_str(),num_channels,0,num_channels);
        TH1I *hist_Slot_day = new TH1I(name_hist_day.c_str(),title_hist_day.c_str(),num_channels,0,num_channels);
        hist_Slot_fivemin->GetXaxis()->SetTitle("Channel #");
        hist_Slot_fivemin->GetYaxis()->SetTitle("# of hits");
        hist_Slot_fivemin->SetLineWidth(2);
        if (i_crate == 0){hist_Slot_fivemin->SetFillColor(8);hist_Slot_fivemin->SetLineColor(8);}
        else {hist_Slot_fivemin->SetFillColor(9);hist_Slot_fivemin->SetLineColor(9);}
        hist_Slot_hour->GetXaxis()->SetTitle("Channel #");
        hist_Slot_hour->GetYaxis()->SetTitle("# of hits");
        hist_Slot_hour->SetLineWidth(2);
        if (i_crate == 0){hist_Slot_hour->SetFillColor(8);hist_Slot_hour->SetLineColor(8);}
        else {hist_Slot_hour->SetFillColor(9);hist_Slot_hour->SetLineColor(9);}
        hist_Slot_sixhour->GetXaxis()->SetTitle("Channel #");
        hist_Slot_sixhour->GetYaxis()->SetTitle("# of hits");
        hist_Slot_sixhour->SetLineWidth(2);
        if (i_crate == 0) {hist_Slot_sixhour->SetFillColor(8); hist_Slot_sixhour->SetLineColor(8);}
        else {hist_Slot_sixhour->SetFillColor(9); hist_Slot_sixhour->SetLineColor(9);}
        hist_Slot_day->GetXaxis()->SetTitle("Channel #");
        hist_Slot_day->GetYaxis()->SetTitle("# of hits");
        hist_Slot_day->SetLineWidth(2);
        if (i_crate == 0){hist_Slot_day->SetFillColor(8);hist_Slot_day->SetLineColor(8);}
        else {hist_Slot_day->SetFillColor(9); hist_Slot_day->SetLineColor(9);}
        hist_hitmap_fivemin_Channel.push_back(hist_Slot_fivemin);
        hist_hitmap_hour_Channel.push_back(hist_Slot_hour);
        hist_hitmap_sixhour_Channel.push_back(hist_Slot_sixhour);
        hist_hitmap_day_Channel.push_back(hist_Slot_day);
        enum_slots++;
      }

      times_crate_hour.push_back(tdc_fivemin);
      times_crate_sixhour.push_back(tdc_halfhour);
      times_crate_day.push_back(tdc_hour);
      rms_crate_hour.push_back(tdc_fivemin);
      rms_crate_sixhour.push_back(tdc_halfhour);
      rms_crate_day.push_back(tdc_hour);
      frequency_crate_hour.push_back(tdc_fivemin);
      frequency_crate_sixhour.push_back(tdc_halfhour);
      frequency_crate_day.push_back(tdc_hour);
      n_crate_hour.push_back(n_fivemin);
      n_crate_sixhour.push_back(n_halfhour);
      n_crate_day.push_back(n_hour);
      std::string name_graph = "cr"+ss_crate.str();
      std::string name_graph_rms = name_graph+"_rms";
      std::string name_graph_freq = name_graph+"_freq";
      std::string name_graph_sixhour = name_graph+"_6h";
      std::string name_graph_sixhour_rms = name_graph+"_6h_rms";
      std::string name_graph_sixhour_freq = name_graph+"_6h_freq";
      std::string name_graph_day = name_graph+"_day";
      std::string name_graph_day_rms = name_graph+"_day_rms";
      std::string name_graph_day_freq = name_graph+"_day_freq";
      std::string title_graph = "Crate "+ss_crate.str();
      std::string title_graph_rms = title_graph+" (RMS)";
      std::string title_graph_freq = title_graph+" (Freq)";
      TGraph *graph_crate = new TGraph();
      TGraph *graph_crate_rms = new TGraph();
      TGraph *graph_crate_freq = new TGraph();
      TGraph *graph_crate_sixhour = new TGraph();
      TGraph *graph_crate_sixhour_rms = new TGraph();
      TGraph *graph_crate_sixhour_freq = new TGraph();
      TGraph *graph_crate_day = new TGraph();
      TGraph *graph_crate_day_rms = new TGraph();
      TGraph *graph_crate_day_freq = new TGraph();
      graph_crate->SetName(name_graph.c_str());
      graph_crate->SetTitle(title_graph.c_str());
      if (draw_marker) {
        graph_crate->SetMarkerStyle(20);
        graph_crate->SetMarkerColor(i_crate+1);
      }
      graph_crate->SetLineColor(i_crate+1);
      graph_crate->SetLineWidth(2);
      graph_crate->SetFillColor(0);
      graph_crate_rms->SetName(name_graph_rms.c_str());
      graph_crate_rms->SetTitle(title_graph.c_str());
      if (draw_marker) {
        graph_crate_rms->SetMarkerStyle(20);
        graph_crate_rms->SetMarkerColor(i_crate+1);
      }
      graph_crate_rms->SetLineColor(i_crate+1);
      graph_crate_rms->SetLineWidth(2);
      graph_crate_rms->SetFillColor(0);
      graph_crate_freq->SetName(name_graph_freq.c_str());
      graph_crate_freq->SetTitle(title_graph.c_str());
      if (draw_marker) {
        graph_crate_freq->SetMarkerStyle(20);
        graph_crate_freq->SetMarkerColor(i_crate+1);
      }
      graph_crate_freq->SetLineColor(i_crate+1);
      graph_crate_freq->SetLineWidth(2);
      graph_crate_freq->SetFillColor(0);
      graph_crate_sixhour->SetName(name_graph_sixhour.c_str());
      graph_crate_sixhour->SetTitle(title_graph.c_str());
      if (draw_marker) {
        graph_crate_sixhour->SetMarkerStyle(20);
        graph_crate_sixhour->SetMarkerColor(i_crate+1);
      }
      graph_crate_sixhour->SetLineColor(i_crate+1);
      graph_crate_sixhour->SetLineWidth(2);
      graph_crate_sixhour->SetFillColor(0);
      graph_crate_sixhour_rms->SetName(name_graph_sixhour_rms.c_str());
      graph_crate_sixhour_rms->SetTitle(title_graph.c_str());
      if (draw_marker) {
        graph_crate_sixhour_rms->SetMarkerStyle(20);
        graph_crate_sixhour_rms->SetMarkerColor(i_crate+1);
      }
      graph_crate_sixhour_rms->SetLineColor(i_crate+1);
      graph_crate_sixhour_rms->SetLineWidth(2);
      graph_crate_sixhour_rms->SetFillColor(0);
      graph_crate_sixhour_freq->SetName(name_graph_sixhour_freq.c_str());
      graph_crate_sixhour_freq->SetTitle(title_graph.c_str());
      if (draw_marker) {
        graph_crate_sixhour_freq->SetMarkerStyle(20);
        graph_crate_sixhour_freq->SetMarkerColor(i_crate+1);
      }
      graph_crate_sixhour_freq->SetLineColor(i_crate+1);
      graph_crate_sixhour_freq->SetLineWidth(2);
      graph_crate_sixhour_freq->SetFillColor(0);
      graph_crate_day->SetName(name_graph_day.c_str());
      graph_crate_day->SetTitle(title_graph.c_str());
      if (draw_marker) {
        graph_crate_day->SetMarkerStyle(20);
        graph_crate_day->SetMarkerColor(i_crate+1);
      }
      graph_crate_day->SetLineColor(i_crate+1);
      graph_crate_day->SetLineWidth(2);
      graph_crate_day->SetFillColor(0);
      graph_crate_day_rms->SetName(name_graph_day_rms.c_str());
      graph_crate_day_rms->SetTitle(title_graph.c_str());
      if (draw_marker) {
        graph_crate_day_rms->SetMarkerStyle(20);
        graph_crate_day_rms->SetMarkerColor(i_crate+1);
      }
      graph_crate_day_rms->SetLineColor(i_crate+1);
      graph_crate_day_rms->SetLineWidth(2);
      graph_crate_day_rms->SetFillColor(0);
      graph_crate_day_freq->SetName(name_graph_day_freq.c_str());
      graph_crate_day_freq->SetTitle(title_graph.c_str());
      if (draw_marker) {
        graph_crate_day_freq->SetMarkerStyle(20);
        graph_crate_day_freq->SetMarkerColor(i_crate+1);
      }
      graph_crate_day_freq->SetLineColor(i_crate+1);
      graph_crate_day_freq->SetLineWidth(2);
      graph_crate_day_freq->SetFillColor(0);
      gr_crate_times_hour.push_back(graph_crate);
      gr_crate_rms_hour.push_back(graph_crate_rms);
      gr_crate_frequency_hour.push_back(graph_crate_freq);
      gr_crate_times_sixhour.push_back(graph_crate_sixhour);
      gr_crate_rms_sixhour.push_back(graph_crate_sixhour_rms);
      gr_crate_frequency_sixhour.push_back(graph_crate_sixhour_freq);
      gr_crate_times_day.push_back(graph_crate_day);
      gr_crate_rms_day.push_back(graph_crate_day_rms);
      gr_crate_frequency_day.push_back(graph_crate_day_freq); 
  }

}

void MonitorMRDTime::MRDTimePlots(){

  //-------------------------------------------------------
  //------------------MRDTimePlots-------------------------
  //-------------------------------------------------------

  //fill the TGraphs first before plotting them

  //variables for scaling hitmaps
  max_sum_fivemin=0;
  max_sum_hour=0;
  max_sum_sixhour=0;
  max_sum_day=0;
  max_sum_day_channel.assign(num_active_slots,0);

  for (int i_channel=0; i_channel<num_active_slots*num_channels;i_channel++){

    //-------------------------------------------------------
    //--------------Updating scatter plots-------------------
    //-------------------------------------------------------

    if (draw_scatter){
      if (hist_times_hour.at(i_channel)->GetEntries()>0) hist_times_hour.at(i_channel)->Reset();
      if (hist_times_sixhour.at(i_channel)->GetEntries()>0) hist_times_sixhour.at(i_channel)->Reset();
      if (hist_times_day.at(i_channel)->GetEntries()>0) hist_times_day.at(i_channel)->Reset();

      for (int i_entry=0; i_entry<times_channel_overall_hour.at(i_channel).size(); i_entry++){
        double hist_time = (3600.-(current_stamp-stamp_channel_overall_hour.at(i_channel).at(i_entry))/(1000.));
        if (verbosity > 5) std::cout <<"1 hour: time diff: "<< (current_stamp-stamp_channel_overall_hour.at(i_channel).at(i_entry)) <<", hist time bin: "<<hist_time<<std::endl;
        if (hist_time>0. && hist_time<3600.) hist_times_hour.at(i_channel)->Fill(hist_time,times_channel_overall_hour.at(i_channel).at(i_entry));
      }
      for (int i_entry=0; i_entry<times_channel_overall_sixhour.at(i_channel).size();i_entry++){
        double hist_time_sixhour = (21600.-(current_stamp-stamp_channel_overall_sixhour.at(i_channel).at(i_entry))/(1000.));
        if (verbosity > 5) std::cout <<"6 hours: time diff: "<< (current_stamp-stamp_channel_overall_sixhour.at(i_channel).at(i_entry)) <<", hist time bin: "<<hist_time_sixhour<<std::endl;
        if (hist_time_sixhour>0. && hist_time_sixhour < 21600.) hist_times_sixhour.at(i_channel)->Fill(hist_time_sixhour,times_channel_overall_sixhour.at(i_channel).at(i_entry));
      }
      for (int i_entry=0;i_entry<times_channel_overall_day.at(i_channel).size();i_entry++){
        double hist_time_day = (86400.-(current_stamp-stamp_channel_overall_day.at(i_channel).at(i_entry))/(1000.));
        if (verbosity > 5) std::cout <<"DAY: time diff: "<< (current_stamp-stamp_channel_overall_day.at(i_channel).at(i_entry)) <<", hist time bin: "<<hist_time_day<<std::endl;
        if (hist_time_day>0. && hist_time_day < 86400.) hist_times_day.at(i_channel)->Fill(hist_time_day,times_channel_overall_day.at(i_channel).at(i_entry));
      }
    }

    //-------------------------------------------------------
    //--------------Updating other graphs--------------------
    //-------------------------------------------------------

    if (update_mins){
      if (draw_average){
        for (int i_mins=0;i_mins<num_fivemin;i_mins++){
          if (verbosity > 3) std::cout <<"MonitorMRDTime: Stored data (1 hour): Channel #: "<<i_channel<<", bin "<<i_mins<<", time: "<<label_fivemin[i_mins].GetTime()<<", tdc: "<<times_channel_hour.at(i_channel)[i_mins]<<std::endl;
          gr_times_hour.at(i_channel)->SetPoint(i_mins,label_fivemin[i_mins].Convert(),times_channel_hour.at(i_channel)[i_mins]);
          gr_rms_hour.at(i_channel)->SetPoint(i_mins,label_fivemin[i_mins].Convert(),rms_channel_hour.at(i_channel)[i_mins]);
          gr_frequency_hour.at(i_channel)->SetPoint(i_mins,label_fivemin[i_mins].Convert(),frequency_channel_hour.at(i_channel)[i_mins]);
        }
      }//close draw average

      //-------------------------------------------------------
      //--------------Updating hitmaps-------------------------
      //-------------------------------------------------------
      if (draw_hitmap){

        long sum_fivemin=0;
        long sum_hour=0;
        long sum_sixhour=0;
        long sum_day=0;
        for (int i_overall_mins=0;i_overall_mins<num_overall_fivemin;i_overall_mins++){
          sum_day+=n_channel_overall_day.at(i_channel)[i_overall_mins];
          if (i_overall_mins>=0.75*num_overall_fivemin){
            sum_sixhour+=n_channel_overall_day.at(i_channel)[i_overall_mins];
            if (i_overall_mins>=int(23./24*num_overall_fivemin)){               //only the last part starting from 23/24 (last hour)
              sum_hour+=n_channel_overall_day.at(i_channel)[i_overall_mins];
              if (i_overall_mins>=int(num_overall_fivemin-1)){                  //only the last 5 mins
                sum_fivemin+=n_channel_overall_day.at(i_channel)[i_overall_mins];
              }
            }
          }
        }

        int num_crate = (i_channel < num_active_slots_cr1*num_channels)? min_crate : min_crate+1; 
        int num_slot = (mapping_vector_ch.at(i_channel)-(i_channel)%num_channels-(num_crate-min_crate)*num_slots)/num_channels+1;
        //if (verbosity > 4) std::cout <<"Channel number: "<<i_channel<<", slot number: "<<num_slot<<std::endl;


        if (i_channel<num_active_slots_cr1*num_channels){
          hist_hitmap_fivemin_cr2->SetBinContent(i_channel+1,0.001);
          hist_hitmap_hour_cr2->SetBinContent(i_channel+1,0.001);
          hist_hitmap_sixhour_cr2->SetBinContent(i_channel+1,0.001);
          hist_hitmap_day_cr2->SetBinContent(i_channel+1,0.001);
          std::vector<int>::iterator it = std::find(active_slots_cr1.begin(),active_slots_cr1.end(),num_slot);
          int index = std::distance(active_slots_cr1.begin(),it);
          if (sum_fivemin!=0) {
            if (max_sum_fivemin < sum_fivemin) max_sum_fivemin = sum_fivemin;
            hist_hitmap_fivemin_cr1->SetBinContent(i_channel+1,sum_fivemin);
            hist_hitmap_fivemin_Channel.at(index)->SetBinContent(i_channel%num_channels+1,sum_fivemin);
          } else {
            hist_hitmap_fivemin_cr1->SetBinContent(i_channel+1,0.001);
            hist_hitmap_fivemin_Channel.at(index)->SetBinContent(i_channel%num_channels+1,0.001);
          }
          if (sum_hour!=0) {
            if (max_sum_hour < sum_hour) max_sum_hour = sum_hour;
            hist_hitmap_hour_cr1->SetBinContent(i_channel+1,sum_hour);
            hist_hitmap_hour_Channel.at(index)->SetBinContent(i_channel%num_channels+1,sum_hour);
          } else {
            hist_hitmap_hour_cr1->SetBinContent(i_channel+1,0.001);
            hist_hitmap_hour_Channel.at(index)->SetBinContent(i_channel%num_channels+1,0.001);
          }
          if (sum_sixhour!=0) {
            if (max_sum_sixhour < sum_sixhour) max_sum_sixhour = sum_sixhour;
            hist_hitmap_sixhour_cr1->SetBinContent(i_channel+1,sum_sixhour);
            hist_hitmap_sixhour_Channel.at(index)->SetBinContent(i_channel%num_channels+1,sum_sixhour);
          } else {
            hist_hitmap_sixhour_cr1->SetBinContent(i_channel+1,0.001);
            hist_hitmap_sixhour_Channel.at(index)->SetBinContent(i_channel%num_channels+1,0.001);
          }
          if (sum_day!=0) {
            if (max_sum_day < sum_day) max_sum_day = sum_day;
            if (max_sum_day_channel.at(i_channel/num_channels) < sum_day) max_sum_day_channel.at(i_channel/num_channels) = sum_day;
            hist_hitmap_day_cr1->SetBinContent(i_channel+1,sum_day);
            hist_hitmap_day_Channel.at(index)->SetBinContent(i_channel%num_channels+1,sum_day);
          } else {
            hist_hitmap_day_cr1->SetBinContent(i_channel+1,0.001);
            hist_hitmap_day_Channel.at(index)->SetBinContent(i_channel%num_channels+1,0.001);
          } 
        } else {
          hist_hitmap_fivemin_cr1->SetBinContent(i_channel+1,0.001);
          hist_hitmap_hour_cr1->SetBinContent(i_channel+1,0.001);
          hist_hitmap_sixhour_cr1->SetBinContent(i_channel+1,0.001);
          hist_hitmap_day_cr1->SetBinContent(i_channel+1,0.001);
          std::vector<int>::iterator it = std::find(active_slots_cr2.begin(),active_slots_cr2.end(),num_slot);
          int index = std::distance(active_slots_cr2.begin(),it);
          if (sum_fivemin!=0) {
            if (max_sum_fivemin < sum_fivemin) max_sum_fivemin = sum_fivemin;
            hist_hitmap_fivemin_cr2->SetBinContent(i_channel+1,sum_fivemin);
            hist_hitmap_fivemin_Channel.at(num_active_slots_cr1+index)->SetBinContent(i_channel%num_channels+1,sum_fivemin);
          } else {
            hist_hitmap_fivemin_cr2->SetBinContent(i_channel+1,0.001);
            hist_hitmap_fivemin_Channel.at(num_active_slots_cr1+index)->SetBinContent(i_channel%num_channels+1,0.001);
          }
          if (sum_hour!=0) {
            if (max_sum_hour < sum_hour) max_sum_hour = sum_hour;
            hist_hitmap_hour_cr2->SetBinContent(i_channel+1,sum_hour);
            hist_hitmap_hour_Channel.at(num_active_slots_cr1+index)->SetBinContent(i_channel%num_channels+1,sum_hour);
          } else {
            hist_hitmap_hour_cr2->SetBinContent(i_channel+1,0.001);
            hist_hitmap_hour_Channel.at(num_active_slots_cr1+index)->SetBinContent(i_channel%num_channels+1,0.001);
          }
          if (sum_sixhour!=0) {
            if (max_sum_sixhour < sum_sixhour) max_sum_sixhour = sum_sixhour;         
            hist_hitmap_sixhour_cr2->SetBinContent(i_channel+1,sum_sixhour);
            hist_hitmap_sixhour_Channel.at(num_active_slots_cr1+index)->SetBinContent(i_channel%num_channels+1,sum_sixhour);
          } else {
            hist_hitmap_sixhour_cr2->SetBinContent(i_channel+1,0.001);
            hist_hitmap_sixhour_Channel.at(num_active_slots_cr1+index)->SetBinContent(i_channel%num_channels+1,0.001);
          }
          if (sum_day!=0) {
            if (max_sum_day < sum_day) max_sum_day = sum_day;
            if (max_sum_day_channel.at(i_channel/num_channels) < sum_day) max_sum_day_channel.at(i_channel/num_channels) = sum_day;
            hist_hitmap_day_cr2->SetBinContent(i_channel+1,sum_day);
            hist_hitmap_day_Channel.at(num_active_slots_cr1+index)->SetBinContent(i_channel%num_channels+1,sum_day);
          } else {
            hist_hitmap_day_cr2->SetBinContent(i_channel+1,0.001);
            hist_hitmap_day_Channel.at(num_active_slots_cr1+index)->SetBinContent(i_channel%num_channels+1,0.001);
          }       
        }
      }
    }//close draw hitmap

    if (draw_average){
      if (update_halfhour){
    	 for (int i_halfhour=0;i_halfhour<num_halfhour;i_halfhour++){
          if (verbosity > 3) std::cout <<"MonitorMRDTime: Stored data (6 hours): bin "<<i_halfhour<<", time: "<<label_halfhour[i_halfhour].GetTime()<<", TDC: "<<times_channel_sixhour.at(i_channel)[i_halfhour]<<std::endl;
  		    gr_times_sixhour.at(i_channel)->SetPoint(i_halfhour,label_halfhour[i_halfhour].Convert(),times_channel_sixhour.at(i_channel)[i_halfhour]);
  		    gr_rms_sixhour.at(i_channel)->SetPoint(i_halfhour,label_halfhour[i_halfhour].Convert(),rms_channel_sixhour.at(i_channel)[i_halfhour]);
  		    gr_frequency_sixhour.at(i_channel)->SetPoint(i_halfhour,label_halfhour[i_halfhour].Convert(),frequency_channel_sixhour.at(i_channel)[i_halfhour]);
        }
      }
      if (update_hour){
        for (int i_hour=0;i_hour<num_hour;i_hour++){
          if (verbosity > 3) std::cout <<"MonitorMRDTime: Stored data (day): bin "<<i_hour<<", time: "<<label_hour[i_hour].GetTime()<<", TDC: "<<times_channel_day.at(i_channel)[i_hour]<<std::endl;
          gr_times_day.at(i_channel)->SetPoint(i_hour,label_hour[i_hour].Convert(),times_channel_day.at(i_channel)[i_hour]);
          gr_rms_day.at(i_channel)->SetPoint(i_hour,label_hour[i_hour].Convert(),rms_channel_day.at(i_channel)[i_hour]);
          gr_frequency_day.at(i_channel)->SetPoint(i_hour,label_hour[i_hour].Convert(),frequency_channel_day.at(i_channel)[i_hour]);
        }
      }
    }//close draw average
  }

  //-------------------------------------------------------
  //--------------Updating slot graphs---------------------
  //-------------------------------------------------------
  if (verbosity > 2) std::cout <<"Updating slot graphs..."<<std::endl;

  if (draw_average){
    for (int i_slot = 0; i_slot<num_active_slots;i_slot++){
      if (update_mins){
        for (int i_mins = 0;i_mins<num_fivemin;i_mins++){
          gr_slot_times_hour.at(i_slot)->SetPoint(i_mins,label_fivemin[i_mins].Convert(),times_slot_hour.at(i_slot)[i_mins]);
          gr_slot_rms_hour.at(i_slot)->SetPoint(i_mins,label_fivemin[i_mins].Convert(),rms_slot_hour.at(i_slot)[i_mins]);
          gr_slot_frequency_hour.at(i_slot)->SetPoint(i_mins,label_fivemin[i_mins].Convert(),frequency_slot_hour.at(i_slot)[i_mins]);
        }
      }
      if (update_halfhour){
        for (int i_halfhour = 0; i_halfhour<num_halfhour;i_halfhour++){	
          gr_slot_times_sixhour.at(i_slot)->SetPoint(i_halfhour,label_halfhour[i_halfhour].Convert(),times_slot_sixhour.at(i_slot)[i_halfhour]);
          gr_slot_rms_sixhour.at(i_slot)->SetPoint(i_halfhour,label_halfhour[i_halfhour].Convert(),rms_slot_sixhour.at(i_slot)[i_halfhour]);
          gr_slot_frequency_sixhour.at(i_slot)->SetPoint(i_halfhour,label_halfhour[i_halfhour].Convert(),frequency_slot_sixhour.at(i_slot)[i_halfhour]);
        }
      }
      if (update_hour){
  	   for (int i_hour = 0; i_hour<num_hour;i_hour++){
  		    gr_slot_times_day.at(i_slot)->SetPoint(i_hour,label_hour[i_hour].Convert(),times_slot_day.at(i_slot)[i_hour]);
  		    gr_slot_rms_day.at(i_slot)->SetPoint(i_hour,label_hour[i_hour].Convert(),rms_slot_day.at(i_slot)[i_hour]);
  		    gr_slot_frequency_day.at(i_slot)->SetPoint(i_hour,label_hour[i_hour].Convert(),frequency_slot_day.at(i_slot)[i_hour]);
        }
      }
    }

    //-------------------------------------------------------
    //--------------Updating crate graphs--------------------
    //-------------------------------------------------------


    if (verbosity > 2) std::cout <<"Updating crate graphs..."<<std::endl;


    for (int i_crate = 0; i_crate<num_crates;i_crate++){
      if (update_mins){
      	for (int i_mins = 0;i_mins<num_fivemin;i_mins++){
    		  gr_crate_times_hour.at(i_crate)->SetPoint(i_mins,label_fivemin[i_mins].Convert(),times_crate_hour.at(i_crate)[i_mins]);
    		  gr_crate_rms_hour.at(i_crate)->SetPoint(i_mins,label_fivemin[i_mins].Convert(),rms_crate_hour.at(i_crate)[i_mins]);
    		  gr_crate_frequency_hour.at(i_crate)->SetPoint(i_mins,label_fivemin[i_mins].Convert(),frequency_crate_hour.at(i_crate)[i_mins]);
        }
      }
      if (update_halfhour){
        for (int i_halfhour = 0; i_halfhour<num_halfhour;i_halfhour++){
      	 gr_crate_times_sixhour.at(i_crate)->SetPoint(i_halfhour,label_halfhour[i_halfhour].Convert(),times_crate_sixhour.at(i_crate)[i_halfhour]);
         gr_crate_rms_sixhour.at(i_crate)->SetPoint(i_halfhour,label_halfhour[i_halfhour].Convert(),rms_crate_sixhour.at(i_crate)[i_halfhour]);
         gr_crate_frequency_sixhour.at(i_crate)->SetPoint(i_halfhour,label_halfhour[i_halfhour].Convert(),frequency_crate_sixhour.at(i_crate)[i_halfhour]);
        }  
      }
      if (update_hour){
        for (int i_hour = 0; i_hour<num_hour;i_hour++){
    		  gr_crate_times_day.at(i_crate)->SetPoint(i_hour,label_hour[i_hour].Convert(),times_crate_day.at(i_crate)[i_hour]);
    		  gr_crate_rms_day.at(i_crate)->SetPoint(i_hour,label_hour[i_hour].Convert(),rms_crate_day.at(i_crate)[i_hour]);
    	   	gr_crate_frequency_day.at(i_crate)->SetPoint(i_hour,label_hour[i_hour].Convert(),frequency_crate_day.at(i_crate)[i_hour]);
        }
      }
    }
  }//close draw average

  //defining all canvases
  if (verbosity > 2) std::cout <<"MRDMonitorTime: Defining Canvas"<<std::endl;
  TCanvas *canvas_ch = new TCanvas("canvas_ch","Channel Canvas",900,600);
  TCanvas *canvas_ch_rms = new TCanvas("canvas_ch_rms","Channel RMS Canvas",900,600);
  TCanvas *canvas_ch_freq = new TCanvas("canvas_ch_freq","Channel Freq Canvas",900,600);
	TCanvas *canvas_slot = new TCanvas("canvas_slot","Slot Canvas",900,600);
  TCanvas *canvas_slot_rms = new TCanvas("canvas_slot_rms","Slot RMS Canvas",900,600);
  TCanvas *canvas_slot_freq = new TCanvas("canvas_slot_freq","Slot Freq Canvas",900,600);
	TCanvas *canvas_crate = new TCanvas("canvas_crate","Crate Canvas",900,600);
  TCanvas *canvas_crate_rms = new TCanvas("canvas_crate_rms","Crate RMS Canvas",900,600);
  TCanvas *canvas_crate_freq = new TCanvas("canvas_crate_freq","Crate Freq Canvas",900,600);
  TCanvas *canvas_hitmap_fivemin = new TCanvas("canvas_hitmap_fivemin","Hitmap (5 mins)",900,600);
  TCanvas *canvas_hitmap_hour = new TCanvas("canvas_hitmap_hour","Hitmap (1 hour)",900,600);
  TCanvas *canvas_hitmap_sixhour = new TCanvas("canvas_hitmap_sixhour","Hitmap (6 hours)",900,600);
  TCanvas *canvas_hitmap_day = new TCanvas("canvas_hitmap_day","Hitmap (day)",900,600);
  TCanvas *canvas_ch_hist = new TCanvas("canvas_ch_hist","Channel Scatter Canvas",900,600);
  TCanvas *canvas_ch_hist_sixhour = new TCanvas("canvas_ch_hist_sixhour","Channel Scatter Canvas",900,600);
  TCanvas *canvas_ch_hist_day = new TCanvas("canvas_ch_hist_day","Channel Scatter Canvas",900,600);

  TCanvas *canvas_ch_temp[num_channels];
  for (int i_channel=0;i_channel<num_channels;i_channel++){
    std::stringstream ss_ch_temp;
    ss_ch_temp<<i_channel;
    canvas_ch_temp[i_channel] = new TCanvas(ss_ch_temp.str().c_str(),"Canvas",900,600);
  }

  //-------------------------------------------------------
  //--------------Getting current time --------------------
  //-------------------------------------------------------

  const char *labels_crate[2]={"Rack 7","Rack 8"};
  double min_cr1, min_cr2, max_cr1, max_cr2, min_scale, max_scale;
  t = time(0);
  struct tm *now = localtime( & t );
  title_time.str("");
  title_time<<(now->tm_year + 1900) << '-' << (now->tm_mon + 1) << '-' <<  now->tm_mday<<','<<now->tm_hour<<':'<<now->tm_min<<':'<<now->tm_sec;
  std::stringstream ss_hist_title;
  std::stringstream ss_leg_ch, ss_leg_slot, ss_leg_crate;

  //-------------------------------------------------------
  //--------------Update 1 Hour Plots ---------------------
  //-------------------------------------------------------

  if (verbosity > 2) {
    std::cout <<"Updating: ";
    if (update_mins) std::cout <<" 1 hour plots";
    if (update_halfhour) std::cout <<" + 6 hour plots";
    if (update_hour) std::cout <<" + 24 hour plots";
    std::cout<<" ..."<<std::endl;
  }

  //std::cout <<"MRDMonitorTime: Beginning drawing canvas..."<<std::endl;
  if (update_mins){

    //1 HOUR - CHANNELS
    TLegend *leg = new TLegend(0.7,0.7,0.88,0.88);
    TLegend *leg_rms = new TLegend(0.7,0.7,0.88,0.88);
    TLegend *leg_freq = new TLegend(0.7,0.7,0.88,0.88);
    max_canvas = 0;
    min_canvas = 9999999.;
    max_canvas_rms = 0;
    min_canvas_rms = 99999999.;
    max_canvas_freq = 0;
    min_canvas_freq = 999999999.;
    TMultiGraph *multi_ch = new TMultiGraph();
    TMultiGraph *multi_ch_rms = new TMultiGraph();
    TMultiGraph *multi_ch_freq = new TMultiGraph();
    int num_ch = 16;   //channels per canvas
    int enum_ch_canvas=0;

    for (int i_channel =0;i_channel<times_channel_hour.size();i_channel++){
      std::stringstream ss_ch, ss_ch_rms, ss_ch_freq;//ss_ch_hist, ss_ch_hist_sixhour, ss_ch_hist_day;
    
  	   if (i_channel%num_ch == 0 || i_channel == times_channel_hour.size()-1) {
        if (i_channel != 0){
          int num_crate = (i_channel>num_active_slots_cr1*num_channels)? min_crate+1 : min_crate;
          int num_slot = (mapping_vector_ch.at(i_channel-1)-(i_channel-1)%num_channels-(num_crate-min_crate)*num_slots)/num_channels+1;
          if (i_channel==num_ch && verbosity > 1) std::cout<<"Looping over channels: 0 ...";//std::cout <<"i_channel: "<<i_channel<<", num crate: "<<num_crate<<", num_slot: "<<num_slot<<", i_channel%num_ch: "<<i_channel%num_ch<<std::endl;
          if (i_channel==times_channel_hour.size()-1 && verbosity > 1) std::cout<<i_channel+1<<std::endl;
          ss_ch.str("");
          ss_ch_rms.str("");
          ss_ch_freq.str("");
          ss_ch<<"Crate "<<num_crate<<" Slot "<<num_slot<<" (1 hour) ["<<enum_ch_canvas+1<<"]";
          ss_ch_rms<<"Crate "<<num_crate<<" Slot "<<num_slot<<" (1 hour) ["<<enum_ch_canvas+1<<"]";
          ss_ch_freq<<"Crate "<<num_crate<<" Slot "<<num_slot<<" (1 hour) ["<<enum_ch_canvas+1<<"]";
          if (draw_average){
            if ( i_channel == times_channel_hour.size() - 1){
              ss_leg_ch.str("");
              ss_leg_ch<<"ch "<<i_channel%num_channels;
              multi_ch->Add(gr_times_hour.at(i_channel));
              leg->AddEntry(gr_times_hour.at(i_channel),ss_leg_ch.str().c_str(),"l");
              multi_ch_rms->Add(gr_rms_hour.at(i_channel));
              leg_rms->AddEntry(gr_rms_hour.at(i_channel),ss_leg_ch.str().c_str(),"l");
              multi_ch_freq->Add(gr_frequency_hour.at(i_channel));
              leg_freq->AddEntry(gr_frequency_hour.at(i_channel),ss_leg_ch.str().c_str(),"l");
            }
            canvas_ch->cd();
            multi_ch->Draw("apl");
            //multi_ch->GetYaxis()->SetRangeUser(0,1.2*max_canvas);
            multi_ch->SetTitle(ss_ch.str().c_str());
            multi_ch->GetYaxis()->SetTitle("TDC");
            multi_ch->GetXaxis()->SetTimeDisplay(1);
            multi_ch->GetXaxis()->SetLabelSize(0.03);
            multi_ch->GetXaxis()->SetLabelOffset(0.03);
            multi_ch->GetXaxis()->SetTimeFormat("#splitline{%m/%d}{%H:%M}");
            leg->Draw();
            canvas_ch_rms->cd();
            multi_ch_rms->Draw("apl");
            //multi_ch_rms->GetYaxis()->SetRangeUser(0.,1.2*max_canvas_rms);
            multi_ch_rms->SetTitle(ss_ch_rms.str().c_str());
            multi_ch_rms->GetYaxis()->SetTitle("RMS (TDC)");
            multi_ch_rms->GetYaxis()->SetTitleSize(0.035);
            multi_ch_rms->GetYaxis()->SetTitleOffset(1.3);
            multi_ch_rms->GetXaxis()->SetTimeDisplay(1);
            multi_ch_rms->GetXaxis()->SetLabelSize(0.03);
            multi_ch_rms->GetXaxis()->SetLabelOffset(0.03);
            multi_ch_rms->GetXaxis()->SetTimeFormat("#splitline{%m/%d}{%H:%M}");
            leg_rms->Draw();
            canvas_ch_freq->cd();
            multi_ch_freq->Draw("apl");
            //multi_ch_freq->GetYaxis()->SetRangeUser(0.,1.2*max_canvas_freq);
            multi_ch_freq->SetTitle(ss_ch_freq.str().c_str());
            multi_ch_freq->GetYaxis()->SetTitle("Freq [1/min]");
            multi_ch_freq->GetYaxis()->SetTitleSize(0.035);
            multi_ch_freq->GetYaxis()->SetTitleOffset(1.3);
            multi_ch_freq->GetXaxis()->SetTimeDisplay(1);
            multi_ch_freq->GetXaxis()->SetLabelSize(0.03);
            multi_ch_freq->GetXaxis()->SetLabelOffset(0.03);
            multi_ch_freq->GetXaxis()->SetTimeFormat("#splitline{%m/%d}{%H:%M}");
            leg_freq->Draw();
            ss_ch.str("");
            ss_ch_rms.str("");
            ss_ch_freq.str("");
            ss_ch<<outpath<<"Crate"<<num_crate<<"_Slot"<<num_slot<<"_"<<enum_ch_canvas<<"_HourTrend_TDC.jpg";
            ss_ch_rms<<outpath<<"Crate"<<num_crate<<"_Slot"<<num_slot<<"_"<<enum_ch_canvas<<"_HourTrend_RMS.jpg";
            ss_ch_freq<<outpath<<"Crate"<<num_crate<<"_Slot"<<num_slot<<"_"<<enum_ch_canvas<<"_HourTrend_Freq.jpg";
            canvas_ch->SaveAs(ss_ch.str().c_str());
            canvas_ch_rms->SaveAs(ss_ch_rms.str().c_str());
            canvas_ch_freq->SaveAs(ss_ch_freq.str().c_str()); 
          }//close draw average

          enum_ch_canvas++;  
          enum_ch_canvas=enum_ch_canvas%2;

          for (int i_gr=0; i_gr < num_ch; i_gr++){
            int i_balance = (i_channel == times_channel_hour.size()-1)? 1 : 0;
            multi_ch->RecursiveRemove(gr_times_hour.at(i_channel-num_ch+i_gr+i_balance));
            multi_ch_rms->RecursiveRemove(gr_rms_hour.at(i_channel-num_ch+i_gr+i_balance));
            multi_ch_freq->RecursiveRemove(gr_frequency_hour.at(i_channel-num_ch+i_gr+i_balance));
          }

        } //close i_channel!=0
  		
        canvas_ch->Clear();
        canvas_ch_rms->Clear();
        canvas_ch_freq->Clear();

        delete multi_ch;
        delete multi_ch_rms;
        delete multi_ch_freq;
        delete leg;
        delete leg_rms;
        delete leg_freq;

        if (i_channel!=times_channel_hour.size()-1){
          multi_ch = new TMultiGraph();
          multi_ch_rms = new TMultiGraph();
          multi_ch_freq = new TMultiGraph();
          leg = new TLegend(0.7,0.7,0.88,0.88);
          leg->SetNColumns(4);
          leg_rms = new TLegend(0.7,0.7,0.88,0.88);
          leg_rms->SetNColumns(4);
          leg_freq = new TLegend(0.7,0.7,0.88,0.88);
          leg_freq->SetNColumns(4);
          leg->SetLineColor(0);
          leg_rms->SetLineColor(0);
          leg_freq->SetLineColor(0);
        }
  	   } //close i_channel%num_channel=0 

      if (draw_average && (i_channel != times_channel_hour.size()-1)){
        ss_leg_ch.str("");
        ss_leg_ch<<"ch "<<i_channel%num_channels;
        multi_ch->Add(gr_times_hour.at(i_channel));
        if (gr_times_hour.at(i_channel)->GetMaximum()>max_canvas) max_canvas = gr_times_hour.at(i_channel)->GetMaximum();
        if (gr_times_hour.at(i_channel)->GetMinimum()<min_canvas) min_canvas = gr_times_hour.at(i_channel)->GetMinimum();
        leg->AddEntry(gr_times_hour.at(i_channel),ss_leg_ch.str().c_str(),"l");
        multi_ch_rms->Add(gr_rms_hour.at(i_channel));
        leg_rms->AddEntry(gr_rms_hour.at(i_channel),ss_leg_ch.str().c_str(),"l");
        if (gr_rms_hour.at(i_channel)->GetMaximum()>max_canvas_rms) max_canvas_rms = gr_rms_hour.at(i_channel)->GetMaximum();
        if (gr_rms_hour.at(i_channel)->GetMinimum()<min_canvas_rms) min_canvas_rms = gr_rms_hour.at(i_channel)->GetMinimum();
        multi_ch_freq->Add(gr_frequency_hour.at(i_channel));
        leg_freq->AddEntry(gr_frequency_hour.at(i_channel),ss_leg_ch.str().c_str(),"l");
        if (gr_frequency_hour.at(i_channel)->GetMaximum()>max_canvas_freq) max_canvas_freq = gr_frequency_hour.at(i_channel)->GetMaximum();
        if (gr_frequency_hour.at(i_channel)->GetMinimum()<min_canvas_freq) min_canvas_freq = gr_frequency_hour.at(i_channel)->GetMinimum();
      }//close draw average

      //single channel plots - omit, too many plots!
      /*
      int num_crate = (i_channel>=num_active_slots_cr1*num_channels)? min_crate+1 : min_crate;
      int num_slot = (mapping_vector_ch.at(i_channel)-(i_channel)%num_channels-(num_crate-min_crate)*num_slots)/num_channels+1;
      canvas_ch_temp[i_channel%(num_channels)]->Clear();
      canvas_ch_temp[i_channel%(num_channels)]->cd();
      hist_times_hour.at(i_channel)->Draw();
      std::stringstream ss_ch_hist_temp;
      ss_ch_hist_temp<<outpath<<"Crate"<<num_crate<<"_Slot"<<num_slot<<"_Channel"<<(i_channel%(num_channels))<<"_HourTrend_Scatter.jpg";
      canvas_ch_temp[i_channel%(num_channels)]->SaveAs(ss_ch_hist_temp.str().c_str());
    */
  	} //close i_channel loop

    if (draw_average){
    //1 HOUR - SLOTS (1)
    max_canvas = 0;
    min_canvas = 9999999.;
    max_canvas_rms = 0;
    min_canvas_rms = 99999999.;
    max_canvas_freq = 0;
    min_canvas_freq = 999999999.;
    TLegend *leg_slot = new TLegend(0.8,0.7,0.88,0.88);
    TLegend *leg_slot_rms = new TLegend(0.8,0.7,0.88,0.88);
    TLegend *leg_slot_freq = new TLegend(0.8,0.7,0.88,0.88);
    leg_slot->SetLineColor(0);
    leg_slot_rms->SetLineColor(0);
    leg_slot_freq->SetLineColor(0);
    TMultiGraph *multi_slot = new TMultiGraph();
    TMultiGraph *multi_slot_rms = new TMultiGraph();
    TMultiGraph *multi_slot_freq = new TMultiGraph();

  	for (int i_slot = 0;i_slot<num_active_slots_cr1;i_slot++){
      multi_slot->Add(gr_slot_times_hour.at(i_slot));
      leg_slot->AddEntry(gr_slot_times_hour.at(i_slot),"","l");
      if (gr_slot_times_hour.at(i_slot)->GetMaximum()>max_canvas) max_canvas = gr_slot_times_hour.at(i_slot)->GetMaximum();
      if (gr_slot_times_hour.at(i_slot)->GetMinimum()<min_canvas) min_canvas = gr_slot_times_hour.at(i_slot)->GetMinimum();
      multi_slot_rms->Add(gr_slot_rms_hour.at(i_slot));
      leg_slot_rms->AddEntry(gr_slot_rms_hour.at(i_slot),"","l");
      if (gr_slot_rms_hour.at(i_slot)->GetMaximum()>max_canvas_rms) max_canvas_rms = gr_slot_rms_hour.at(i_slot)->GetMaximum();
      if (gr_slot_rms_hour.at(i_slot)->GetMinimum()<min_canvas_rms) min_canvas_rms = gr_slot_rms_hour.at(i_slot)->GetMinimum();
      multi_slot_freq->Add(gr_slot_frequency_hour.at(i_slot));
      leg_slot_freq->AddEntry(gr_slot_frequency_hour.at(i_slot),"","l");
      if (gr_slot_frequency_hour.at(i_slot)->GetMaximum()>max_canvas_freq) max_canvas_freq = gr_slot_frequency_hour.at(i_slot)->GetMaximum();
      if (gr_slot_frequency_hour.at(i_slot)->GetMinimum()<min_canvas_freq) min_canvas_freq = gr_slot_frequency_hour.at(i_slot)->GetMinimum();
  	}

    canvas_slot->cd();
    multi_slot->Draw("apl");
    //ulti_slot->GetYaxis()->SetRangeUser(0.,1.2*max_canvas);
    multi_slot->SetTitle("Crate 7 (1 hour)");
    multi_slot->GetYaxis()->SetTitle("TDC");
    multi_slot->GetXaxis()->SetTimeDisplay(1);
    multi_slot->GetXaxis()->SetLabelSize(0.03);
    multi_slot->GetXaxis()->SetLabelOffset(0.03);
    multi_slot->GetXaxis()->SetTimeFormat("#splitline{%m/%d}{%H:%M}");
    leg_slot->Draw();
    canvas_slot_rms->cd();
    multi_slot_rms->Draw("apl");
    //multi_slot_rms->GetYaxis()->SetRangeUser(0.,1.2*max_canvas_rms);
    multi_slot_rms->SetTitle("Crate 7 (1 hour)");
    multi_slot_rms->GetYaxis()->SetTitle("RMS (TDC)");
    multi_slot_rms->GetYaxis()->SetTitleSize(0.035);
    multi_slot_rms->GetYaxis()->SetTitleOffset(1.3);
    multi_slot_rms->GetXaxis()->SetTimeDisplay(1);
    multi_slot_rms->GetXaxis()->SetLabelSize(0.03);
    multi_slot_rms->GetXaxis()->SetLabelOffset(0.03);
    multi_slot_rms->GetXaxis()->SetTimeFormat("#splitline{%m/%d}{%H:%M}");
    leg_slot_rms->Draw();
    canvas_slot_freq->cd();
    multi_slot_freq->Draw("apl");
   // multi_slot_freq->GetYaxis()->SetRangeUser(0.,1.2*max_canvas_freq);
    multi_slot_freq->SetTitle("Crate 7 (1 hour)");
    multi_slot_freq->GetYaxis()->SetTitle("Freq [1/min]");
    multi_slot_freq->GetYaxis()->SetTitleSize(0.035);
    multi_slot_freq->GetYaxis()->SetTitleOffset(1.3);
    multi_slot_freq->GetXaxis()->SetTimeDisplay(1);
    multi_slot_freq->GetXaxis()->SetLabelSize(0.03);
    multi_slot_freq->GetXaxis()->SetLabelOffset(0.03);
    multi_slot_freq->GetXaxis()->SetTimeFormat("#splitline{%m/%d}{%H:%M}");
    leg_slot_freq->Draw();
    std::stringstream ss_slot, ss_slot_rms, ss_slot_freq;
  	ss_slot<<outpath<<"Crate7_Slots_HourTrend_TDC.jpg";
  	ss_slot_rms<<outpath<<"Crate7_Slots_HourTrend_RMS.jpg";
  	ss_slot_freq<<outpath<<"Crate7_Slots_HourTrend_Freq.jpg";
  	canvas_slot->SaveAs(ss_slot.str().c_str());
  	canvas_slot_rms->SaveAs(ss_slot_rms.str().c_str());
  	canvas_slot_freq->SaveAs(ss_slot_freq.str().c_str());

    //1 HOUR - SLOTS (2) 
  	canvas_slot->Clear();
  	canvas_slot_rms->Clear();
  	canvas_slot_freq->Clear();
    max_canvas = 0;
    min_canvas = 9999999.;
    max_canvas_rms = 0;
    min_canvas_rms = 99999999.;
    max_canvas_freq = 0;
    min_canvas_freq = 999999999.;
    TLegend *leg_slot2 = new TLegend(0.8,0.7,0.88,0.88);
    TLegend *leg_slot2_rms = new TLegend(0.8,0.7,0.88,0.88);
    TLegend *leg_slot2_freq = new TLegend(0.8,0.7,0.88,0.88);
    leg_slot2->SetLineColor(0);
    leg_slot2_rms->SetLineColor(0);
    leg_slot2_freq->SetLineColor(0);
    TMultiGraph *multi_slot2 = new TMultiGraph();
    TMultiGraph *multi_slot2_rms = new TMultiGraph();
    TMultiGraph *multi_slot2_freq = new TMultiGraph();

  	for (int i_slot = 0;i_slot<num_active_slots_cr2;i_slot++){
      multi_slot2->Add(gr_slot_times_hour.at(i_slot+num_active_slots_cr1));
      leg_slot2->AddEntry(gr_slot_times_hour.at(i_slot+num_active_slots_cr1),"","l");
      if (gr_slot_times_hour.at(i_slot)->GetMaximum()>max_canvas) max_canvas = gr_slot_times_hour.at(i_slot)->GetMaximum();
      if (gr_slot_times_hour.at(i_slot)->GetMinimum()<min_canvas) min_canvas = gr_slot_times_hour.at(i_slot)->GetMinimum();
      multi_slot2_rms->Add(gr_slot_rms_hour.at(i_slot+num_active_slots_cr1));
      leg_slot2_rms->AddEntry(gr_slot_rms_hour.at(i_slot+num_active_slots_cr1),"","l");
      if (gr_slot_rms_hour.at(i_slot)->GetMaximum()>max_canvas_rms) max_canvas_rms = gr_slot_rms_hour.at(i_slot)->GetMaximum();
      if (gr_slot_rms_hour.at(i_slot)->GetMinimum()<min_canvas_rms) min_canvas_rms = gr_slot_rms_hour.at(i_slot)->GetMinimum();
      multi_slot2_freq->Add(gr_slot_frequency_hour.at(i_slot+num_active_slots_cr1));
      leg_slot2_freq->AddEntry(gr_slot_frequency_hour.at(i_slot+num_active_slots_cr1),"","l");
      if (gr_slot_frequency_hour.at(i_slot)->GetMaximum()>max_canvas_freq) max_canvas_freq = gr_slot_frequency_hour.at(i_slot)->GetMaximum();
      if (gr_slot_frequency_hour.at(i_slot)->GetMinimum()<min_canvas_freq) min_canvas_freq = gr_slot_frequency_hour.at(i_slot)->GetMinimum();
  	}

    canvas_slot->cd();
    multi_slot2->Draw("apl");
    //multi_slot2->GetYaxis()->SetRangeUser(0.,1.2*max_canvas);
    multi_slot2->SetTitle("Crate 8 (1 hour)");
    multi_slot2->GetYaxis()->SetTitle("TDC");
    multi_slot2->GetXaxis()->SetTimeDisplay(1);
    multi_slot2->GetXaxis()->SetLabelSize(0.03);
    multi_slot2->GetXaxis()->SetLabelOffset(0.03);
    multi_slot2->GetXaxis()->SetTimeFormat("#splitline{%m/%d}{%H:%M}");
    leg_slot2->Draw();
    canvas_slot_rms->cd();
    multi_slot2_rms->Draw("apl");
    //multi_slot2_rms->GetYaxis()->SetRangeUser(0.,1.2*max_canvas_rms);
    multi_slot2_rms->SetTitle("Crate 8 (1 hour)");
    multi_slot2_rms->GetYaxis()->SetTitle("RMS (TDC)");
    multi_slot2_rms->GetYaxis()->SetTitleSize(0.035);
    multi_slot2_rms->GetYaxis()->SetTitleOffset(1.3);
    multi_slot2_rms->GetXaxis()->SetTimeDisplay(1);
    multi_slot2_rms->GetXaxis()->SetLabelOffset(0.03);
    multi_slot2_rms->GetXaxis()->SetLabelSize(0.03);
    multi_slot2_rms->GetXaxis()->SetTimeFormat("#splitline{%m/%d}{%H:%M}");
    leg_slot2_rms->Draw();
    canvas_slot_freq->cd();
    multi_slot2_freq->Draw("apl");
    //multi_slot2_freq->GetYaxis()->SetRangeUser(0.,1.2*max_canvas_freq);
    multi_slot2_freq->SetTitle("Crate 8 (1 hour)");
    multi_slot2_freq->GetYaxis()->SetTitle("Freq [1/min]");
    multi_slot2_freq->GetYaxis()->SetTitleSize(0.035);
    multi_slot2_freq->GetYaxis()->SetTitleOffset(1.3);
    multi_slot2_freq->GetXaxis()->SetTimeDisplay(1);
    multi_slot2_freq->GetXaxis()->SetLabelSize(0.03);
    multi_slot2_freq->GetXaxis()->SetLabelOffset(0.03);
    multi_slot2_freq->GetXaxis()->SetTimeFormat("#splitline{%m/%d}{%H:%M}");
    leg_slot2_freq->Draw();
  	ss_slot.str("");
  	ss_slot_rms.str("");
  	ss_slot_freq.str("");
  	ss_slot<<outpath<<"Crate8_Slots_HourTrend_TDC.jpg";
  	ss_slot_rms<<outpath<<"Crate8_Slots_HourTrend_RMS.jpg";
  	ss_slot_freq<<outpath<<"Crate8_Slots_HourTrend_Freq.jpg";
  	canvas_slot->SaveAs(ss_slot.str().c_str());
  	canvas_slot_rms->SaveAs(ss_slot_rms.str().c_str());
  	canvas_slot_freq->SaveAs(ss_slot_freq.str().c_str());

    //1 HOUR - CRATE
    canvas_crate->Clear();
    canvas_crate_rms->Clear();
    canvas_crate_freq->Clear();
    max_canvas = 0;
    min_canvas = 9999999.;
    max_canvas_rms = 0;
    min_canvas_rms = 99999999.;
    max_canvas_freq = 0;
    min_canvas_freq = 999999999.;
    TLegend *leg_crate = new TLegend(0.7,0.7,0.88,0.88);
    TLegend *leg_crate_rms = new TLegend(0.7,0.7,0.88,0.88);
    TLegend *leg_crate_freq = new TLegend(0.7,0.7,0.88,0.88);
    leg_crate->SetLineColor(0);
    leg_crate_rms->SetLineColor(0);
    leg_crate_freq->SetLineColor(0);
    TMultiGraph *multi_crate = new TMultiGraph();
    TMultiGraph *multi_crate_rms = new TMultiGraph();
    TMultiGraph *multi_crate_freq = new TMultiGraph();

  	for (int i_crate=0;i_crate<num_crates;i_crate++){
      multi_crate->Add(gr_crate_times_hour.at(i_crate));
      leg_crate->AddEntry(gr_crate_times_hour.at(i_crate),"","l");
      if (gr_crate_times_hour.at(i_crate)->GetMaximum()>max_canvas) max_canvas = gr_crate_times_hour.at(i_crate)->GetMaximum();
      if (gr_crate_times_hour.at(i_crate)->GetMinimum()<min_canvas) min_canvas = gr_crate_times_hour.at(i_crate)->GetMinimum();
      multi_crate_rms->Add(gr_crate_rms_hour.at(i_crate));
      leg_crate_rms->AddEntry(gr_crate_rms_hour.at(i_crate),"","l");
      if (gr_crate_rms_hour.at(i_crate)->GetMaximum()>max_canvas_rms) max_canvas_rms = gr_crate_rms_hour.at(i_crate)->GetMaximum();
      if (gr_crate_rms_hour.at(i_crate)->GetMinimum()<min_canvas_rms) min_canvas_rms = gr_crate_rms_hour.at(i_crate)->GetMinimum();
      multi_crate_freq->Add(gr_crate_frequency_hour.at(i_crate));
      leg_crate_freq->AddEntry(gr_crate_frequency_hour.at(i_crate),"","l");
      if (gr_crate_frequency_hour.at(i_crate)->GetMaximum()>max_canvas_freq) max_canvas_freq = gr_crate_frequency_hour.at(i_crate)->GetMaximum();
      if (gr_crate_frequency_hour.at(i_crate)->GetMinimum()<min_canvas_freq) min_canvas_freq = gr_crate_frequency_hour.at(i_crate)->GetMinimum();
  	}

    canvas_crate->cd();
    multi_crate->Draw("apl");
    multi_crate->SetTitle("Crates (1 hour)");
    //multi_crate->GetYaxis()->SetRangeUser(0.,1.2*max_canvas);
    multi_crate->GetYaxis()->SetTitle("TDC");
    multi_crate->GetXaxis()->SetTimeDisplay(1);
    multi_crate->GetXaxis()->SetLabelSize(0.03);
    multi_crate->GetXaxis()->SetLabelOffset(0.03);
    multi_crate->GetXaxis()->SetTimeFormat("#splitline{%m/%d}{%H:%M}");
    leg_crate->Draw();
    canvas_crate_rms->cd();
    multi_crate_rms->Draw("apl");
    //multi_crate->GetYaxis()->SetRangeUser(0.,1.2*max_canvas_rms);
    multi_crate_rms->SetTitle("Crates (1 hour)");
    multi_crate_rms->GetYaxis()->SetTitle("RMS (TDC)");
    multi_crate_rms->GetYaxis()->SetTitleSize(0.035);
    multi_crate_rms->GetYaxis()->SetTitleOffset(1.3);
    multi_crate_rms->GetXaxis()->SetTimeDisplay(1);
    multi_crate_rms->GetXaxis()->SetLabelSize(0.03);
    multi_crate_rms->GetXaxis()->SetLabelOffset(0.03);
    multi_crate_rms->GetXaxis()->SetTimeFormat("#splitline{%m/%d}{%H:%M}");
    leg_crate_rms->Draw();
    canvas_crate_freq->cd();
    multi_crate_freq->Draw("apl");
    //multi_crate_freq->GetYaxis()->SetRangeUser(0.,1.2*max_canvas_freq);
    multi_crate_freq->SetTitle("Crates (1 hour)");
    multi_crate_freq->GetYaxis()->SetTitle("Freq [1/min]");
    multi_crate_freq->GetYaxis()->SetTitleSize(0.035);
    multi_crate_freq->GetYaxis()->SetTitleOffset(1.3);
    multi_crate_freq->GetXaxis()->SetTimeDisplay(1);
    multi_crate_freq->GetXaxis()->SetLabelSize(0.03);
    multi_crate_freq->GetXaxis()->SetLabelOffset(0.03);
    multi_crate_freq->GetXaxis()->SetTimeFormat("#splitline{%m/%d}{%H:%M}");
    leg_crate_freq->Draw();
    std::stringstream ss_crate, ss_crate_rms, ss_crate_freq;
    ss_crate<<outpath<<"Crates_HourTrend_TDC.jpg";
    ss_crate_rms<<outpath<<"Crates_HourTrend_RMS.jpg";
    ss_crate_freq<<outpath<<"Crates_HourTrend_Freq.jpg";
    canvas_crate->SaveAs(ss_crate.str().c_str());
    canvas_crate_rms->SaveAs(ss_crate_rms.str().c_str());
    canvas_crate_freq->SaveAs(ss_crate_freq.str().c_str());

    //delete all remaining pointers to objects

    for (int i_slot = 0;i_slot<num_active_slots_cr2;i_slot++){
      multi_slot->RecursiveRemove(gr_slot_times_hour.at(i_slot));
      multi_slot_rms->RecursiveRemove(gr_slot_rms_hour.at(i_slot));
      multi_slot_freq->RecursiveRemove(gr_slot_frequency_hour.at(i_slot));
      multi_slot2->RecursiveRemove(gr_slot_times_hour.at(i_slot+num_active_slots_cr1));
      multi_slot2_rms->RecursiveRemove(gr_slot_rms_hour.at(i_slot+num_active_slots_cr1));
      multi_slot2_freq->RecursiveRemove(gr_slot_frequency_hour.at(i_slot+num_active_slots_cr1));
    }
    
    for (int i_crate = 0; i_crate < num_crates; i_crate++){
      multi_crate->RecursiveRemove(gr_crate_times_hour.at(i_crate));
      multi_crate_rms->RecursiveRemove(gr_crate_rms_hour.at(i_crate));
      multi_crate_freq->RecursiveRemove(gr_crate_frequency_hour.at(i_crate));
    }

    delete multi_slot;
    delete multi_slot_rms;
    delete multi_slot_freq;
    delete multi_slot2;
    delete multi_slot2_rms;
    delete multi_slot2_freq;
    delete multi_crate;
    delete multi_crate_rms;
    delete multi_crate_freq;

    delete leg_slot;
    delete leg_slot_rms;
    delete leg_slot_freq;
    delete leg_slot2;
    delete leg_slot2_rms;
    delete leg_slot2_freq;
    delete leg_crate;
    delete leg_crate_rms;
    delete leg_crate_freq;

    } // close draw average

    if (draw_hitmap){

    //HITMAP - 5 MINS
    canvas_hitmap_fivemin->cd();
    hist_hitmap_fivemin_cr1->GetYaxis()->SetTitle("# of entries");
    hist_hitmap_fivemin_cr1->GetXaxis()->SetNdivisions(int(num_active_slots));
    hist_hitmap_fivemin_cr2->GetXaxis()->SetNdivisions(int(num_active_slots));
    for (int i_label=0;i_label<int(num_active_slots);i_label++){
      if (i_label<num_active_slots_cr1){
        std::stringstream ss_slot;
        ss_slot<<(active_slots_cr1.at(i_label));
        std::string str_slot = "slot "+ss_slot.str();
        hist_hitmap_fivemin_cr1->GetXaxis()->SetBinLabel(num_channels*(i_label+0.5),str_slot.c_str());
      }
      else {
        std::stringstream ss_slot;
        ss_slot<<(active_slots_cr2.at(i_label-num_active_slots_cr1));
        std::string str_slot = "slot "+ss_slot.str();
        hist_hitmap_fivemin_cr1->GetXaxis()->SetBinLabel(num_channels*(i_label+0.5),str_slot.c_str());          
      }
    }
    hist_hitmap_fivemin_cr1->LabelsOption("v");
    hist_hitmap_fivemin_cr1->SetTickLength(0,"X");   //workaround to only have labels for every slot
    hist_hitmap_fivemin_cr2->SetTickLength(0,"X");
    hist_hitmap_fivemin_cr1->GetYaxis()->SetRangeUser(0.8,max_sum_fivemin+10);
    hist_hitmap_fivemin_cr2->GetYaxis()->SetRangeUser(0.8,max_sum_fivemin+10);
    canvas_hitmap_fivemin->SetGridy();
    canvas_hitmap_fivemin->SetLogy();
    hist_hitmap_fivemin_cr1->SetLineWidth(2);
    hist_hitmap_fivemin_cr2->SetLineWidth(2);
    std::string Hitmap = "Hitmap ";
    std::string string_lastfivemin = " (last 5 mins)";
    ss_hist_title<<Hitmap<<title_time.str()<<string_lastfivemin;
    hist_hitmap_fivemin_cr1->SetTitle(ss_hist_title.str().c_str());
    hist_hitmap_fivemin_cr1->SetTitleSize(0.3,"t");
    hist_hitmap_fivemin_cr1->Draw();
    hist_hitmap_fivemin_cr2->Draw("same");
    TLine *separate_crates = new TLine(num_channels*num_active_slots_cr1,0.8,num_channels*num_active_slots_cr1,max_sum_fivemin+10);
    separate_crates->SetLineStyle(2);
    separate_crates->SetLineWidth(2);
    separate_crates->Draw("same");
    TPaveText *label_cr1 = new TPaveText(0.02,0.93,0.17,0.98,"NDC");
    label_cr1->SetFillColor(0);
    label_cr1->SetTextColor(8);
    label_cr1->AddText("Rack 7");
    label_cr1->Draw();
    TPaveText *label_cr2 = new TPaveText(0.83,0.93,0.98,0.98,"NDC");
    label_cr2->SetFillColor(0);
    label_cr2->SetTextColor(9);
    label_cr2->AddText("Rack 8");
    label_cr2->Draw();
    canvas_hitmap_fivemin->Update();
    TF1 *f1 = new TF1("f1","x",0,num_active_slots);       //workaround to only have labels for every slot
    TGaxis *labels_grid = new TGaxis(0,canvas_hitmap_fivemin->GetUymin(),num_active_slots*num_channels,canvas_hitmap_fivemin->GetUymin(),"f1",num_active_slots,"w");
    labels_grid->SetLabelSize(0);
    labels_grid->Draw("w");
    std::string str_hitmaps_fivemin = "Hitmaps_lastfivemins.jpg";
    std::string save_path_fivemin = outpath+str_hitmaps_fivemin;
    canvas_hitmap_fivemin->SaveAs(save_path_fivemin.c_str());

    delete separate_crates;
    delete label_cr1;
    delete label_cr2;
    delete labels_grid;

    //HITMAP - 1 HOUR
    canvas_hitmap_hour->cd();
    /*min_cr1=hist_hitmap_hour_cr1->GetMinimum();
    min_cr2 = hist_hitmap_hour_cr2->GetMinimum();
    min_scale = (min_cr1<min_cr2)? min_cr1 : min_cr2;
    max_cr1=hist_hitmap_hour_cr1->GetMaximum();
    max_cr2 = hist_hitmap_hour_cr2->GetMaximum();
    max_scale = (max_cr1>max_cr2)? max_cr1 : max_cr2;*/
    hist_hitmap_hour_cr1->GetYaxis()->SetTitle("# of entries");
    hist_hitmap_hour_cr1->GetXaxis()->SetNdivisions(int(num_active_slots));
    hist_hitmap_hour_cr2->GetXaxis()->SetNdivisions(int(num_active_slots));
    for (int i_label=0;i_label<int(num_active_slots);i_label++){
      if (i_label<num_active_slots_cr1){
        std::stringstream ss_slot;
        ss_slot<<(active_slots_cr1.at(i_label));
        std::string str_slot = "slot "+ss_slot.str();
        hist_hitmap_hour_cr1->GetXaxis()->SetBinLabel(num_channels*(i_label+0.5),str_slot.c_str());
      }
      else {
        std::stringstream ss_slot;
        ss_slot<<(active_slots_cr2.at(i_label-num_active_slots_cr1));
        std::string str_slot = "slot "+ss_slot.str();
        hist_hitmap_hour_cr1->GetXaxis()->SetBinLabel(num_channels*(i_label+0.5),str_slot.c_str());          
      }
    }
    hist_hitmap_hour_cr1->LabelsOption("v");
    hist_hitmap_hour_cr1->SetTickLength(0,"X");   //workaround to only have labels for every slot
    hist_hitmap_hour_cr2->SetTickLength(0,"X");
    hist_hitmap_hour_cr1->GetYaxis()->SetRangeUser(0.8,max_sum_hour+10);
    hist_hitmap_hour_cr2->GetYaxis()->SetRangeUser(0.8,max_sum_hour+10);
    canvas_hitmap_hour->SetGridy();
    canvas_hitmap_hour->SetLogy();
    hist_hitmap_hour_cr1->SetLineWidth(2);
    hist_hitmap_hour_cr2->SetLineWidth(2);
    std::string string_lasthour = " (last hour)";
    ss_hist_title.str("");
    ss_hist_title<<Hitmap<<title_time.str()<<string_lasthour;
    hist_hitmap_hour_cr1->SetTitle(ss_hist_title.str().c_str());
    hist_hitmap_hour_cr1->SetTitleSize(0.3,"t");
    hist_hitmap_hour_cr1->Draw();
    hist_hitmap_hour_cr2->Draw("same");
    TLine *separate_crates_hour = new TLine(num_channels*num_active_slots_cr1,0.8,num_channels*num_active_slots_cr1,max_sum_hour+10);
    separate_crates_hour->SetLineStyle(2);
    separate_crates_hour->SetLineWidth(2);
    separate_crates_hour->Draw("same");
    TPaveText *label_cr1_hour = new TPaveText(0.02,0.93,0.17,0.98,"NDC");
    label_cr1_hour->SetFillColor(0);
    label_cr1_hour->SetTextColor(8);
    label_cr1_hour->AddText("Rack 7");
    label_cr1_hour->Draw();
    TPaveText *label_cr2_hour = new TPaveText(0.83,0.93,0.98,0.98,"NDC");
    label_cr2_hour->SetFillColor(0);
    label_cr2_hour->SetTextColor(9);
    label_cr2_hour->AddText("Rack 8");
    label_cr2_hour->Draw();
    canvas_hitmap_hour->Update();
    TGaxis *labels_grid_hour = new TGaxis(0,canvas_hitmap_hour->GetUymin(),num_active_slots*num_channels,canvas_hitmap_hour->GetUymin(),"f1",num_active_slots,"w");
    labels_grid_hour->SetLabelSize(0);
    labels_grid_hour->Draw("w");
    std::string str_hitmaps_hour = "Hitmaps_lasthour.jpg";
    std::string save_path_hour = outpath+str_hitmaps_hour;
    canvas_hitmap_hour->SaveAs(save_path_hour.c_str());

    delete separate_crates_hour;
    delete label_cr1_hour;
    delete label_cr2_hour;
    delete labels_grid_hour;

    //HITMAP - 6 HOURS
    canvas_hitmap_sixhour->cd();
    /*min_cr1=hist_hitmap_sixhour_cr1->GetMinimum();
    min_cr2 = hist_hitmap_sixhour_cr2->GetMinimum();
    min_scale = (min_cr1<min_cr2)? min_cr1 : min_cr2;
    max_cr1=hist_hitmap_sixhour_cr1->GetMaximum();
    max_cr2 = hist_hitmap_sixhour_cr2->GetMaximum();
    max_scale = (max_cr1>max_cr2)? max_cr1 : max_cr2;*/
    hist_hitmap_sixhour_cr1->GetYaxis()->SetTitle("# of entries");
    hist_hitmap_sixhour_cr1->GetXaxis()->SetNdivisions(int(num_active_slots));
    hist_hitmap_sixhour_cr2->GetXaxis()->SetNdivisions(int(num_active_slots));
    for (int i_label=0;i_label<int(num_active_slots);i_label++){
      if (i_label<num_active_slots_cr1){
        std::stringstream ss_slot;
        ss_slot<<(active_slots_cr1.at(i_label));
        std::string str_slot = "slot "+ss_slot.str();
        hist_hitmap_sixhour_cr1->GetXaxis()->SetBinLabel(num_channels*(i_label+0.5),str_slot.c_str());
      }
      else {
        std::stringstream ss_slot;
        ss_slot<<(active_slots_cr2.at(i_label-num_active_slots_cr1));
        std::string str_slot = "slot "+ss_slot.str();
        hist_hitmap_sixhour_cr1->GetXaxis()->SetBinLabel(num_channels*(i_label+0.5),str_slot.c_str());          
      }
    }
    hist_hitmap_sixhour_cr1->LabelsOption("v");
    hist_hitmap_sixhour_cr1->SetTickLength(0,"X");   //workaround to only have labels for every slot
    hist_hitmap_sixhour_cr2->SetTickLength(0,"X");
    //hist_hitmap_sixhour_cr1->GetYaxis()->SetRangeUser(0.8,max_scale+10);
    //hist_hitmap_sixhour_cr2->GetYaxis()->SetRangeUser(0.8,max_scale+10);
    canvas_hitmap_sixhour->SetGridy();
    canvas_hitmap_sixhour->SetLogy();
    hist_hitmap_sixhour_cr1->SetLineWidth(2);
    hist_hitmap_sixhour_cr2->SetLineWidth(2);
    ss_hist_title.str("");
    std::string string_lastsixhour = " (last six hours)";
    ss_hist_title<<Hitmap<<title_time.str()<<string_lastsixhour;
    hist_hitmap_sixhour_cr1->SetTitle(ss_hist_title.str().c_str());
    hist_hitmap_sixhour_cr1->SetTitleSize(0.3,"t");
    hist_hitmap_sixhour_cr1->Draw();
    hist_hitmap_sixhour_cr2->Draw("same");
    /*min_cr1=hist_hitmap_sixhour_cr1->GetMinimum();
    min_cr2 = hist_hitmap_sixhour_cr2->GetMinimum();
    min_scale = (min_cr1<min_cr2)? min_cr1 : min_cr2;
    max_cr1=hist_hitmap_sixhour_cr1->GetMaximum();
    max_cr2 = hist_hitmap_sixhour_cr2->GetMaximum();
    max_scale = (max_cr1>max_cr2)? max_cr1 : max_cr2;*/
    hist_hitmap_sixhour_cr1->GetYaxis()->SetRangeUser(0.8,max_sum_sixhour+10);
    hist_hitmap_sixhour_cr2->GetYaxis()->SetRangeUser(0.8,max_sum_sixhour+10);
    TLine *separate_crates_sixhour = new TLine(num_channels*num_active_slots_cr1,0.8,num_channels*num_active_slots_cr1,max_sum_sixhour+10);
    separate_crates_sixhour->SetLineStyle(2);
    separate_crates_sixhour->SetLineWidth(2);
    separate_crates_sixhour->Draw("same");
    TPaveText *label_cr1_sixhour = new TPaveText(0.02,0.93,0.17,0.98,"NDC");
    label_cr1_sixhour->SetFillColor(0);
    label_cr1_sixhour->SetTextColor(8);
    label_cr1_sixhour->AddText("Rack 7");
    label_cr1_sixhour->Draw();
    TPaveText *label_cr2_sixhour = new TPaveText(0.83,0.93,0.98,0.98,"NDC");
    label_cr2_sixhour->SetFillColor(0);
    label_cr2_sixhour->SetTextColor(9);
    label_cr2_sixhour->AddText("Rack 8");
    label_cr2_sixhour->Draw();
    canvas_hitmap_sixhour->Update();
    TGaxis *labels_grid_sixhour = new TGaxis(0,canvas_hitmap_sixhour->GetUymin(),num_active_slots*num_channels,canvas_hitmap_sixhour->GetUymin(),"f1",num_active_slots,"w");
    labels_grid_sixhour->SetLabelSize(0);
    labels_grid_sixhour->Draw("w");
    std::string str_hitmaps_sixhours = "Hitmaps_lastsixhours.jpg";
    std::string save_path_sixhours = outpath+str_hitmaps_sixhours;
    canvas_hitmap_sixhour->SaveAs(save_path_sixhours.c_str());

    delete separate_crates_sixhour;
    delete label_cr1_sixhour;
    delete label_cr2_sixhour;
    delete labels_grid_sixhour;

    //HITMAP - DAY
    canvas_hitmap_day->cd();
    /* min_cr1=hist_hitmap_day_cr1->GetMinimum();
    min_cr2 = hist_hitmap_day_cr2->GetMinimum();
    min_scale = (min_cr1<min_cr2)? min_cr1 : min_cr2;
    max_cr1 = hist_hitmap_day_cr1->GetMaximum();
    max_cr2 = hist_hitmap_day_cr2->GetMaximum();
    max_scale = (max_cr1>max_cr2)? max_cr1 : max_cr2;
    std::cout << "hitmap DAY: max1: "<<max_cr1<<", max cr2: "<<max_cr2<<", max gesamt: "<<max_scale<<std::endl;*/
    hist_hitmap_day_cr1->GetYaxis()->SetTitle("# of entries");
    hist_hitmap_day_cr1->GetXaxis()->SetNdivisions(int(num_active_slots));
    hist_hitmap_day_cr2->GetXaxis()->SetNdivisions(int(num_active_slots));
    for (int i_label=0;i_label<int(num_active_slots);i_label++){
      if (i_label<num_active_slots_cr1){
        std::stringstream ss_slot;
        ss_slot<<(active_slots_cr1.at(i_label));
        std::string str_slot = "slot "+ss_slot.str();
        hist_hitmap_day_cr1->GetXaxis()->SetBinLabel(num_channels*(i_label+0.5),str_slot.c_str());
      }
      else {
        std::stringstream ss_slot;
        ss_slot<<(active_slots_cr2.at(i_label-num_active_slots_cr1));
        std::string str_slot = "slot "+ss_slot.str();
        hist_hitmap_day_cr1->GetXaxis()->SetBinLabel(num_channels*(i_label+0.5),str_slot.c_str());          
      }
    }
    hist_hitmap_day_cr1->LabelsOption("v");
    hist_hitmap_day_cr1->SetTickLength(0,"X");   //workaround to only have labels for every slot
    hist_hitmap_day_cr2->SetTickLength(0,"X");
    //hist_hitmap_day_cr1->GetYaxis()->SetRangeUser(0.8,max_scale+10);
    //hist_hitmap_day_cr2->GetYaxis()->SetRangeUser(0.8,max_scale+10);
    canvas_hitmap_day->SetGridy();
    canvas_hitmap_day->SetLogy();
    hist_hitmap_day_cr1->SetLineWidth(2);
    hist_hitmap_day_cr2->SetLineWidth(2);
    ss_hist_title.str("");
    std::string string_lastday = " (last day)";
    ss_hist_title<<Hitmap<<title_time.str()<<string_lastday;
    hist_hitmap_day_cr1->SetTitle(ss_hist_title.str().c_str());
    hist_hitmap_day_cr1->SetTitleSize(0.3,"t");
    hist_hitmap_day_cr1->Draw();
    hist_hitmap_day_cr2->Draw("same");
    /*min_cr1=hist_hitmap_day_cr1->GetMinimum();
    min_cr2 = hist_hitmap_day_cr2->GetMinimum();
    min_scale = (min_cr1<min_cr2)? min_cr1 : min_cr2;
    max_cr1 = hist_hitmap_day_cr1->GetMaximum();
    max_cr2 = hist_hitmap_day_cr2->GetMaximum();
    max_scale = (max_cr1>max_cr2)? max_cr1 : max_cr2;*/
    //std::cout << "hitmap DAY: max1: "<<max_cr1<<", max cr2: "<<max_cr2<<", max gesamt: "<<max_scale<<std::endl;
    //std::cout << "hitmap DAY: max sum: "<<max_sum_day<<std::endl;
    hist_hitmap_day_cr1->GetYaxis()->SetRangeUser(0.8,max_sum_day+10);
    hist_hitmap_day_cr2->GetYaxis()->SetRangeUser(0.8,max_sum_day+10);
    TLine *separate_crates_day = new TLine(num_channels*num_active_slots_cr1,0.8,num_channels*num_active_slots_cr1,max_sum_day+10);
    separate_crates_day->SetLineStyle(2);
    separate_crates_day->SetLineWidth(2);
    separate_crates_day->Draw("same");
    TPaveText *label_cr1_day = new TPaveText(0.02,0.93,0.17,0.98,"NDC");
    label_cr1_day->SetFillColor(0);
    label_cr1_day->SetTextColor(8);
    label_cr1_day->AddText("Rack 7");
    label_cr1_day->Draw();
    TPaveText *label_cr2_day = new TPaveText(0.83,0.93,0.98,0.98,"NDC");
    label_cr2_day->SetFillColor(0);
    label_cr2_day->SetTextColor(9);
    label_cr2_day->AddText("Rack 8");
    label_cr2_day->Draw();
    canvas_hitmap_day->Update();
    TGaxis *labels_grid_day = new TGaxis(0,canvas_hitmap_day->GetUymin(),num_active_slots*num_channels,canvas_hitmap_day->GetUymin(),"f1",num_active_slots,"w");
    labels_grid_day->SetLabelSize(0);
    labels_grid_day->Draw("w");
    std::string str_hitmaps_day = "Hitmaps_lastday.jpg";
    std::string save_path_day = outpath+str_hitmaps_day;
    canvas_hitmap_day->SaveAs(save_path_day.c_str());

    delete separate_crates_day;
    delete label_cr1_day;
    delete label_cr2_day;
    delete labels_grid_day;
    delete f1;

    //HITMAPS - SINGLE CHANNEL DISTRIBUTIONS
    TCanvas *canvas_Hitmap_Slots_fivemin, *canvas_Hitmap_Slots_hour, *canvas_Hitmap_Slots_sixhour, *canvas_Hitmap_Slots_day;

      for (int i_slot = 0; i_slot < num_active_slots; i_slot++){
        std::stringstream ss_title_fivemin, ss_title_hour, ss_title_sixhour, ss_title_day;
        std::string prefix_fivemin = "Hitmaps_lastfivemin_Slot";
        std::string prefix_hour = "Hitmaps_lasthour_Slot";
        std::string prefix_sixhour = "Hitmaps_lastsixhours_Slot";
        std::string prefix_day = "Hitmaps_lastday_Slot";
        ss_title_fivemin << prefix_fivemin << (i_slot)+1;
        ss_title_hour << prefix_hour << (i_slot)+1;
        ss_title_sixhour << prefix_sixhour << (i_slot)+1;
        ss_title_day << prefix_day << (i_slot)+1;
        canvas_Hitmap_Slots_fivemin = new TCanvas(ss_title_fivemin.str().c_str(),"CanvasFiveMin",900,600);
        canvas_Hitmap_Slots_hour = new TCanvas(ss_title_hour.str().c_str(),"CanvasHour",900,600);
        canvas_Hitmap_Slots_sixhour = new TCanvas(ss_title_sixhour.str().c_str(),"CanvasSixHour",900,600);
        canvas_Hitmap_Slots_day = new TCanvas(ss_title_day.str().c_str(),"CanvasDay",900,600);
        canvas_Hitmap_Slots_fivemin->cd();
        hist_hitmap_fivemin_Channel.at(i_slot)->SetStats(0);
        hist_hitmap_fivemin_Channel.at(i_slot)->Draw();
        if (hist_hitmap_fivemin_Channel.at(i_slot)->GetMaximum()>500) {
          canvas_Hitmap_Slots_fivemin->SetLogy();
          hist_hitmap_fivemin_Channel.at(i_slot)->GetYaxis()->SetRangeUser(0.8,hist_hitmap_fivemin_Channel.at(i_slot)->GetMaximum()+10);
        }
        std::stringstream ss_savepath_fivemin;
        ss_savepath_fivemin << outpath << prefix_fivemin << (i_slot+1) <<".jpg";
        canvas_Hitmap_Slots_fivemin->SetGridx();
        canvas_Hitmap_Slots_fivemin->SaveAs(ss_savepath_fivemin.str().c_str());
        canvas_Hitmap_Slots_fivemin->Clear();
        canvas_Hitmap_Slots_hour->cd();
        hist_hitmap_hour_Channel.at(i_slot)->SetStats(0);
        hist_hitmap_hour_Channel.at(i_slot)->Draw();
        if (hist_hitmap_hour_Channel.at(i_slot)->GetMaximum()>500) {
          canvas_Hitmap_Slots_hour->SetLogy();
          hist_hitmap_hour_Channel.at(i_slot)->GetYaxis()->SetRangeUser(0.8,hist_hitmap_hour_Channel.at(i_slot)->GetMaximum()+10);
        }
        std::stringstream ss_savepath_hour;
        ss_savepath_hour << outpath << prefix_hour << (i_slot+1) <<".jpg";
        canvas_Hitmap_Slots_hour->SetGridx();
        canvas_Hitmap_Slots_hour->SaveAs(ss_savepath_hour.str().c_str());
        canvas_Hitmap_Slots_hour->Clear();
        canvas_Hitmap_Slots_sixhour->cd();
        hist_hitmap_sixhour_Channel.at(i_slot)->SetStats(0);
        hist_hitmap_sixhour_Channel.at(i_slot)->Draw();
        if (hist_hitmap_sixhour_Channel.at(i_slot)->GetMaximum()>500) {
          canvas_Hitmap_Slots_sixhour->SetLogy();
          hist_hitmap_sixhour_Channel.at(i_slot)->GetYaxis()->SetRangeUser(0.8,hist_hitmap_sixhour_Channel.at(i_slot)->GetMaximum()+10);
        }
        std::stringstream ss_savepath_sixhour;
        ss_savepath_sixhour << outpath << prefix_sixhour << (i_slot+1) <<".jpg";
        canvas_Hitmap_Slots_sixhour->SetGridx();
        canvas_Hitmap_Slots_sixhour->SaveAs(ss_savepath_sixhour.str().c_str());
        canvas_Hitmap_Slots_sixhour->Clear();
        canvas_Hitmap_Slots_day->cd();
        hist_hitmap_day_Channel.at(i_slot)->SetStats(0);
        hist_hitmap_day_Channel.at(i_slot)->Draw();
        if (hist_hitmap_day_Channel.at(i_slot)->GetMaximum()>500) {
          canvas_Hitmap_Slots_day->SetLogy();
          hist_hitmap_day_Channel.at(i_slot)->GetYaxis()->SetRangeUser(0.8,max_sum_day_channel.at(i_slot)+10);
          //hist_hitmap_day_Channel.at(i_slot)->GetYaxis()->SetRangeUser(0.8,hist_hitmap_day_Channel.at(i_slot)->GetMaximum()+10);
        }
        std::stringstream ss_savepath_day;
        ss_savepath_day << outpath << prefix_day << (i_slot+1) <<".jpg";
        canvas_Hitmap_Slots_day->SetGridx();
        canvas_Hitmap_Slots_day->SaveAs(ss_savepath_day.str().c_str());
        canvas_Hitmap_Slots_day->Clear();
        delete canvas_Hitmap_Slots_fivemin;
        delete canvas_Hitmap_Slots_hour;
        delete canvas_Hitmap_Slots_sixhour;
        delete canvas_Hitmap_Slots_day;
      }
      max_sum_day_channel.clear();
      }//close draw hitmap
  }  
  if (draw_scatter){   //update only the scatter plots if 5 mins have not passed
    //1 HOUR - SCATTER
    //std::cout <<"MRDMonitorTime: Updating Scatter Plots: "<<std::endl;
    TLegend *leg_hist = new TLegend(0.7,0.7,0.88,0.88);
    TLegend *leg_hist_sixhour = new TLegend(0.7,0.7,0.88,0.88);
    TLegend *leg_hist_day = new TLegend(0.7,0.7,0.88,0.88);
    max_canvas_hist = 0.;
    min_canvas_hist = 999999999.;
    int num_ch = 16;   //channels per canvas
    int enum_ch_canvas=0;

    for (int i_channel =0;i_channel<times_channel_hour.size();i_channel++){

      //std::cout <<"SCATTER: i_channel: "<<i_channel<<std::endl;

      std::stringstream ss_ch_hist, ss_ch_hist_sixhour, ss_ch_hist_day;
    
      if (i_channel%num_ch == 0 || i_channel == times_channel_hour.size()-1) {
        if (i_channel != 0){
          int num_crate = (i_channel>num_active_slots_cr1*num_channels)? min_crate+1 : min_crate;
          int num_slot = (mapping_vector_ch.at(i_channel-1)-(i_channel-1)%num_channels-(num_crate-min_crate)*num_slots)/num_channels+1;
          //std::cout <<"i_channel: "<<i_channel<<", num crate: "<<num_crate<<", num_slot: "<<num_slot<<", i_channel%num_ch: "<<i_channel%num_ch<<std::endl;
          if (i_channel==num_ch && verbosity > 1) std::cout<<"Scattering Plots: Looping over channels: 0 ...";//std::cout <<"i_channel: "<<i_channel<<", num crate: "<<num_crate<<", num_slot: "<<num_slot<<", i_channel%num_ch: "<<i_channel%num_ch<<std::endl;
          if (i_channel==times_channel_hour.size()-1 && verbosity > 1) std::cout<<i_channel+1<<std::endl;
          ss_ch_hist<<"Crate "<<num_crate<<" Slot "<<num_slot<<" (1 hour) ["<<enum_ch_canvas+1<<"]";
          ss_ch_hist_sixhour<<"Crate "<<num_crate<<" Slot "<<num_slot<<" (6 hours) ["<<enum_ch_canvas+1<<"]";
          ss_ch_hist_day<<"Crate "<<num_crate<<" Slot "<<num_slot<<" (1 day) ["<<enum_ch_canvas+1<<"]";
          if ( i_channel == times_channel_hour.size() - 1){
            canvas_ch_hist->cd();
            hist_times_hour.at(i_channel)->Draw("same");
            canvas_ch_hist_sixhour->cd();
            hist_times_sixhour.at(i_channel)->Draw("same");
            canvas_ch_hist_day->cd();
            hist_times_day.at(i_channel)->Draw("same");
            ss_leg_ch.str("");
            ss_leg_ch<<"ch "<<i_channel%num_channels;
            leg_hist->AddEntry(hist_times_hour.at(i_channel),ss_leg_ch.str().c_str(),"p");
            leg_hist_sixhour->AddEntry(hist_times_sixhour.at(i_channel),ss_leg_ch.str().c_str(),"p");
            leg_hist_day->AddEntry(hist_times_day.at(i_channel),ss_leg_ch.str().c_str(),"p");
            canvas_ch_hist->cd();
            hist_times_hour.at(i_channel-(num_channels/2)+1)->SetTitle(ss_ch_hist.str().c_str());
            leg_hist->Draw();
            canvas_ch_hist_sixhour->cd();
            hist_times_sixhour.at(i_channel-(num_channels/2)+1)->SetTitle(ss_ch_hist_sixhour.str().c_str());
            leg_hist_sixhour->Draw();
            canvas_ch_hist_day->cd();
            hist_times_day.at(i_channel-(num_channels/2)+1)->SetTitle(ss_ch_hist_day.str().c_str());
            leg_hist_day->Draw();
          }else {
          canvas_ch_hist->cd();
          hist_times_hour.at(i_channel-(num_channels/2))->SetTitle(ss_ch_hist.str().c_str());
          leg_hist->Draw();
          canvas_ch_hist_sixhour->cd();
          hist_times_sixhour.at(i_channel-(num_channels/2))->SetTitle(ss_ch_hist_sixhour.str().c_str());
          leg_hist_sixhour->Draw();
          canvas_ch_hist_day->cd();
          hist_times_day.at(i_channel-(num_channels/2))->SetTitle(ss_ch_hist_day.str().c_str());
          leg_hist_day->Draw();
          }
          ss_ch_hist.str("");
          ss_ch_hist_sixhour.str("");
          ss_ch_hist_day.str("");
          ss_ch_hist<<outpath<<"Crate"<<num_crate<<"_Slot"<<num_slot<<"_"<<enum_ch_canvas<<"_HourTrend_Scatter.jpg";
          ss_ch_hist_sixhour<<outpath<<"Crate"<<num_crate<<"_Slot"<<num_slot<<"_"<<enum_ch_canvas<<"_6HourTrend_Scatter.jpg";
          ss_ch_hist_day<<outpath<<"Crate"<<num_crate<<"_Slot"<<num_slot<<"_"<<enum_ch_canvas<<"_DayTrend_Scatter.jpg";
          canvas_ch_hist->SaveAs(ss_ch_hist.str().c_str());
          canvas_ch_hist_sixhour->SaveAs(ss_ch_hist_sixhour.str().c_str());
          canvas_ch_hist_day->SaveAs(ss_ch_hist_day.str().c_str());
          enum_ch_canvas++;  
          enum_ch_canvas=enum_ch_canvas%2;
        }

        canvas_ch_hist->Clear();
        canvas_ch_hist_sixhour->Clear();
        canvas_ch_hist_day->Clear();

        delete leg_hist;
        delete leg_hist_sixhour;
        delete leg_hist_day;

        if (i_channel != times_channel_hour.size()-1){
          leg_hist = new TLegend(0.7,0.7,0.88,0.88);
          leg_hist->SetNColumns(4);
          leg_hist_sixhour = new TLegend(0.7,0.7,0.88,0.88);
          leg_hist_sixhour->SetNColumns(4);
          leg_hist_day = new TLegend(0.7,0.7,0.88,0.88);
          leg_hist_day->SetNColumns(4);
          leg_hist->SetLineColor(0);
          leg_hist_sixhour->SetLineColor(0);
          leg_hist_day->SetLineColor(0);
        }
      }

      canvas_ch_hist->cd();
      if (i_channel%(num_channels/2)==0) {
        hist_times_hour.at(i_channel)->GetXaxis()->SetLabelSize(0.03);
        hist_times_hour.at(i_channel)->GetXaxis()->SetLabelOffset(0.03);
        hist_times_hour.at(i_channel)->GetXaxis()->SetTimeDisplay(1);
        hist_times_hour.at(i_channel)->GetXaxis()->SetTimeOffset(timeoffset_hour.Convert());
        hist_times_hour.at(i_channel)->GetXaxis()->SetTimeFormat("#splitline{%m/%d}{%H:%M}");
        hist_times_hour.at(i_channel)->Draw();
        canvas_ch_hist_sixhour->cd();
        hist_times_sixhour.at(i_channel)->GetXaxis()->SetLabelSize(0.03);
        hist_times_sixhour.at(i_channel)->GetXaxis()->SetLabelOffset(0.03);
        hist_times_sixhour.at(i_channel)->GetXaxis()->SetTimeDisplay(1);
        hist_times_sixhour.at(i_channel)->GetXaxis()->SetTimeOffset(timeoffset_sixhour.Convert());
        hist_times_sixhour.at(i_channel)->GetXaxis()->SetTimeFormat("#splitline{%m/%d}{%H:%M}");
        hist_times_sixhour.at(i_channel)->Draw();
        canvas_ch_hist_day->cd();
        hist_times_day.at(i_channel)->GetXaxis()->SetLabelSize(0.03);
        hist_times_day.at(i_channel)->GetXaxis()->SetLabelOffset(0.03);
        hist_times_day.at(i_channel)->GetXaxis()->SetTimeDisplay(1);
        hist_times_day.at(i_channel)->GetXaxis()->SetTimeOffset(timeoffset_day.Convert());
        hist_times_day.at(i_channel)->GetXaxis()->SetTimeFormat("#splitline{%m/%d}{%H:%M}");
        hist_times_day.at(i_channel)->Draw();
      } else {
        hist_times_hour.at(i_channel)->Draw("same");
        canvas_ch_hist_sixhour->cd();
        hist_times_sixhour.at(i_channel)->Draw("same");
        canvas_ch_hist_day->cd();
        hist_times_day.at(i_channel)->Draw("same");
      }

      ss_leg_ch.str("");
      ss_leg_ch<<"ch "<<i_channel%num_channels;
      if (hist_times_hour.at(i_channel)->GetMaximum()>max_canvas_hist) max_canvas_hist = hist_times_hour.at(i_channel)->GetMaximum();
      if (hist_times_hour.at(i_channel)->GetMinimum()<min_canvas_hist) min_canvas_hist = hist_times_hour.at(i_channel)->GetMinimum();
      leg_hist->AddEntry(hist_times_hour.at(i_channel),ss_leg_ch.str().c_str(),"p");
      leg_hist_sixhour->AddEntry(hist_times_sixhour.at(i_channel),ss_leg_ch.str().c_str(),"p");
      leg_hist_day->AddEntry(hist_times_day.at(i_channel),ss_leg_ch.str().c_str(),"p");

      int num_crate = (i_channel>=num_active_slots_cr1*num_channels)? min_crate+1 : min_crate;
      int num_slot = (mapping_vector_ch.at(i_channel)-(i_channel)%num_channels-(num_crate-min_crate)*num_slots)/num_channels+1;
      canvas_ch_temp[i_channel%(num_channels)]->Clear();
      canvas_ch_temp[i_channel%(num_channels)]->cd();
      hist_times_day.at(i_channel)->Draw();
      std::stringstream ss_ch_hist_temp;
      ss_ch_hist_temp<<outpath<<"Crate"<<num_crate<<"_Slot"<<num_slot<<"_Channel"<<(i_channel%(num_channels))<<"_DayTrend_Scatter.jpg";
      canvas_ch_temp[i_channel%(num_channels)]->SaveAs(ss_ch_hist_temp.str().c_str());
    } 
  } //close draw scatter

  //std::cout <<"Finished plotting scattering plots..."<<std::endl;

  //-------------------------------------------------------
  //--------------Update 6 Hour Plots ---------------------
  //-------------------------------------------------------

  if (draw_average){
  if (update_halfhour){

    //6 HOURS - CHANNELS
    TLegend *leg = new TLegend(0.7,0.7,0.88,0.88);
    TLegend *leg_rms = new TLegend(0.7,0.7,0.88,0.88);
    TLegend *leg_freq = new TLegend(0.7,0.7,0.88,0.88);
    max_canvas = 0;
    min_canvas = 9999999.;
    max_canvas_rms = 0;
    min_canvas_rms = 99999999.;
    max_canvas_freq = 0;
    min_canvas_freq = 999999999.;
    TMultiGraph *multi_ch = new TMultiGraph();
    TMultiGraph *multi_ch_rms = new TMultiGraph();
    TMultiGraph *multi_ch_freq = new TMultiGraph();
    int enum_ch_canvas = 0;
    int num_ch = 16;

    for (int i_channel =0;i_channel<times_channel_sixhour.size();i_channel++){

      std::stringstream ss_ch, ss_ch_rms, ss_ch_freq;
    
      if (i_channel%num_ch == 0 || i_channel == times_channel_sixhour.size()-1) {
        if (i_channel != 0){
          int num_crate = (i_channel>num_active_slots_cr1*num_channels)? min_crate+1 : min_crate;
          int num_slot = (mapping_vector_ch.at(i_channel-1)-(i_channel-1)%num_channels-(num_crate-min_crate)*num_slots)/num_channels+1;
          ss_ch.str("");
          ss_ch_rms.str("");
          ss_ch_freq.str("");
          ss_ch<<"Crate "<<num_crate<<" Slot "<<num_slot<<" (6 hours) ["<<enum_ch_canvas+1<<"]";
          ss_ch_rms<<"Crate "<<num_crate<<" Slot "<<num_slot<<" (6 hours) ["<<enum_ch_canvas+1<<"]";
          ss_ch_freq<<"Crate "<<num_crate<<" Slot "<<num_slot<<" (6 hours) ["<<enum_ch_canvas+1<<"]";
          if ( i_channel == times_channel_sixhour.size() - 1){
            ss_leg_ch.str("");
            ss_leg_ch<<"ch "<<i_channel%num_channels;
            multi_ch->Add(gr_times_sixhour.at(i_channel));
            leg->AddEntry(gr_times_sixhour.at(i_channel),ss_leg_ch.str().c_str(),"l");
            multi_ch_rms->Add(gr_rms_sixhour.at(i_channel));
            leg_rms->AddEntry(gr_rms_sixhour.at(i_channel),ss_leg_ch.str().c_str(),"l");
            multi_ch_freq->Add(gr_frequency_sixhour.at(i_channel));
            leg_freq->AddEntry(gr_frequency_sixhour.at(i_channel),ss_leg_ch.str().c_str(),"l");
          }
          canvas_ch->cd();
          multi_ch->Draw("apl");
          //multi_ch->GetYaxis()->SetRangeUser(0.,1.2*max_canvas);
          multi_ch->SetTitle(ss_ch.str().c_str());
          multi_ch->GetYaxis()->SetTitle("TDC");
          multi_ch->GetXaxis()->SetTimeDisplay(1);
          multi_ch->GetXaxis()->SetLabelSize(0.03);
          multi_ch->GetXaxis()->SetLabelOffset(0.03);
          multi_ch->GetXaxis()->SetTimeFormat("#splitline{%m/%d}{%H:%M}");
          leg->Draw();
          canvas_ch_rms->cd();
          multi_ch_rms->Draw("apl");
          //multi_ch_rms->GetYaxis()->SetRangeUser(0.,1.2*max_canvas_rms);
          multi_ch_rms->SetTitle(ss_ch_rms.str().c_str());
          multi_ch_rms->GetYaxis()->SetTitle("RMS (TDC)");
          multi_ch_rms->GetYaxis()->SetTitleSize(0.035);
          multi_ch_rms->GetYaxis()->SetTitleOffset(1.3);
          multi_ch_rms->GetXaxis()->SetTimeDisplay(1);
          multi_ch_rms->GetXaxis()->SetLabelSize(0.03);
          multi_ch_rms->GetXaxis()->SetLabelOffset(0.03);
          multi_ch_rms->GetXaxis()->SetTimeFormat("#splitline{%m/%d}{%H:%M}");
          leg_rms->Draw();
          canvas_ch_freq->cd();
          multi_ch_freq->Draw("apl");
          //multi_ch_freq->GetYaxis()->SetRangeUser(0.,1.2*max_canvas_freq);
          multi_ch_freq->SetTitle(ss_ch_freq.str().c_str());
          multi_ch_freq->GetYaxis()->SetTitle("Freq [1/min]");
          multi_ch_freq->GetYaxis()->SetTitleSize(0.035);
          multi_ch_freq->GetYaxis()->SetTitleOffset(1.3);
          multi_ch_freq->GetXaxis()->SetTimeDisplay(1);
          multi_ch_freq->GetXaxis()->SetLabelSize(0.03);
          multi_ch_freq->GetXaxis()->SetLabelOffset(0.03);
          multi_ch_freq->GetXaxis()->SetTimeFormat("#splitline{%m/%d}{%H:%M}");
          leg_freq->Draw();
          ss_ch.str("");
          ss_ch_rms.str("");
          ss_ch_freq.str("");
          ss_ch<<outpath<<"Crate"<<num_crate<<"_Slot"<<num_slot<<"_"<<enum_ch_canvas<<"_6HourTrend_TDC.jpg";
          ss_ch_rms<<outpath<<"Crate"<<num_crate<<"_Slot"<<num_slot<<"_"<<enum_ch_canvas<<"_6HourTrend_RMS.jpg";
          ss_ch_freq<<outpath<<"Crate"<<num_crate<<"_Slot"<<num_slot<<"_"<<enum_ch_canvas<<"_6HourTrend_Freq.jpg";
          canvas_ch->SaveAs(ss_ch.str().c_str());
          canvas_ch_rms->SaveAs(ss_ch_rms.str().c_str());
          canvas_ch_freq->SaveAs(ss_ch_freq.str().c_str());    
          enum_ch_canvas++;
          enum_ch_canvas = enum_ch_canvas%2;

          for (int i_gr=0; i_gr < num_ch; i_gr++){
            int i_balance = (i_channel == times_channel_sixhour.size()-1)? 1 : 0;
            multi_ch->RecursiveRemove(gr_times_sixhour.at(i_channel-num_ch+i_gr+i_balance));
            multi_ch_rms->RecursiveRemove(gr_rms_sixhour.at(i_channel-num_ch+i_gr+i_balance));
            multi_ch_freq->RecursiveRemove(gr_frequency_sixhour.at(i_channel-num_ch+i_gr+i_balance));
          }
        }
      
        canvas_ch->Clear();
        canvas_ch_rms->Clear();
        canvas_ch_freq->Clear();



        delete leg;
        delete leg_rms;
        delete leg_freq;
        delete multi_ch;
        delete multi_ch_rms;
        delete multi_ch_freq;

        if (i_channel != times_channel_sixhour.size() - 1){
          multi_ch = new TMultiGraph();
          multi_ch_rms = new TMultiGraph();
          multi_ch_freq = new TMultiGraph();
          leg = new TLegend(0.7,0.7,0.88,0.88);
          leg->SetNColumns(4);
          leg_rms = new TLegend(0.7,0.7,0.88,0.88);
          leg_rms->SetNColumns(4);
          leg_freq = new TLegend(0.7,0.7,0.88,0.88);
          leg_freq->SetNColumns(4);
          leg->SetLineColor(0);
          leg_rms->SetLineColor(0);
          leg_freq->SetLineColor(0);
        }
      }

      if (i_channel != times_channel_hour.size()-1){
        ss_leg_ch.str("");
        ss_leg_ch<<"ch "<<i_channel%num_channels;
        multi_ch->Add(gr_times_sixhour.at(i_channel));
        leg->AddEntry(gr_times_sixhour.at(i_channel),ss_leg_ch.str().c_str(),"l");
        if (gr_times_sixhour.at(i_channel)->GetMaximum()>max_canvas) max_canvas = gr_times_sixhour.at(i_channel)->GetMaximum();
        if (gr_times_sixhour.at(i_channel)->GetMinimum()<min_canvas) min_canvas = gr_times_sixhour.at(i_channel)->GetMinimum();
        multi_ch_rms->Add(gr_rms_sixhour.at(i_channel));
        leg_rms->AddEntry(gr_rms_sixhour.at(i_channel),ss_leg_ch.str().c_str(),"l");
        if (gr_rms_sixhour.at(i_channel)->GetMaximum()>max_canvas_rms) max_canvas_rms = gr_rms_sixhour.at(i_channel)->GetMaximum();
        if (gr_rms_sixhour.at(i_channel)->GetMinimum()<min_canvas_rms) min_canvas_rms = gr_rms_sixhour.at(i_channel)->GetMinimum();
        multi_ch_freq->Add(gr_frequency_sixhour.at(i_channel));
        leg_freq->AddEntry(gr_frequency_sixhour.at(i_channel),ss_leg_ch.str().c_str(),"l");
        if (gr_frequency_sixhour.at(i_channel)->GetMaximum()>max_canvas_freq) max_canvas_freq = gr_frequency_sixhour.at(i_channel)->GetMaximum();
        if (gr_frequency_sixhour.at(i_channel)->GetMinimum()<min_canvas_freq) min_canvas_freq = gr_frequency_sixhour.at(i_channel)->GetMinimum();
      }
    } 

    //6 HOURS - SLOTS (1)
    TLegend *leg_slot = new TLegend(0.8,0.7,0.88,0.88);
    TLegend *leg_slot_rms = new TLegend(0.8,0.7,0.88,0.88);
    TLegend *leg_slot_freq = new TLegend(0.8,0.7,0.88,0.88);
    leg_slot->SetLineColor(0);
    leg_slot_rms->SetLineColor(0);
    leg_slot_freq->SetLineColor(0);
    max_canvas = 0;
    min_canvas = 9999999.;
    max_canvas_rms = 0;
    min_canvas_rms = 99999999.;
    max_canvas_freq = 0;
    min_canvas_freq = 999999999.;
    TMultiGraph *multi_slot = new TMultiGraph();
    TMultiGraph *multi_slot_rms = new TMultiGraph();
    TMultiGraph *multi_slot_freq = new TMultiGraph();

    for (int i_slot = 0;i_slot<num_active_slots_cr1;i_slot++){
      multi_slot->Add(gr_slot_times_sixhour.at(i_slot));
      leg_slot->AddEntry(gr_slot_times_sixhour.at(i_slot),"","l");
      if (gr_slot_times_sixhour.at(i_slot)->GetMaximum()>max_canvas) max_canvas = gr_slot_times_sixhour.at(i_slot)->GetMaximum();
      if (gr_slot_times_sixhour.at(i_slot)->GetMinimum()<min_canvas) min_canvas = gr_slot_times_sixhour.at(i_slot)->GetMinimum();
      multi_slot_rms->Add(gr_slot_rms_sixhour.at(i_slot));
      leg_slot_rms->AddEntry(gr_slot_rms_sixhour.at(i_slot),"","l");
      if (gr_slot_rms_sixhour.at(i_slot)->GetMaximum()>max_canvas_rms) max_canvas_rms = gr_slot_rms_sixhour.at(i_slot)->GetMaximum();
      if (gr_slot_rms_sixhour.at(i_slot)->GetMinimum()<min_canvas_rms) min_canvas_rms = gr_slot_rms_sixhour.at(i_slot)->GetMinimum();
      multi_slot_freq->Add(gr_slot_frequency_sixhour.at(i_slot));
      leg_slot_freq->AddEntry(gr_slot_frequency_sixhour.at(i_slot),"","l");
      if (gr_slot_frequency_sixhour.at(i_slot)->GetMaximum()>max_canvas_freq) max_canvas_freq = gr_slot_frequency_sixhour.at(i_slot)->GetMaximum();
      if (gr_slot_frequency_sixhour.at(i_slot)->GetMinimum()<min_canvas_freq) min_canvas_freq = gr_slot_frequency_sixhour.at(i_slot)->GetMinimum();
    }

    canvas_slot->cd();
    multi_slot->Draw("apl");
    //multi_slot->GetYaxis()->SetRangeUser(0.,1.2*max_canvas);
    multi_slot->SetTitle("Crate 7 (6 hours)");
    multi_slot->GetYaxis()->SetTitle("TDC");
    multi_slot->GetXaxis()->SetTimeDisplay(1);
    multi_slot->GetXaxis()->SetLabelSize(0.03);
    multi_slot->GetXaxis()->SetLabelOffset(0.03);
    multi_slot->GetXaxis()->SetTimeFormat("#splitline{%m/%d}{%H:%M}");
    leg_slot->Draw();
    canvas_slot_rms->cd();
    multi_slot_rms->Draw("apl");
    //multi_slot_rms->GetYaxis()->SetRangeUser(0.,1.2*max_canvas_rms);
    multi_slot_rms->SetTitle("Crate 7 (6 hours)");
    multi_slot_rms->GetYaxis()->SetTitle("RMS (TDC)");
    multi_slot_rms->GetYaxis()->SetTitleSize(0.035);
    multi_slot_rms->GetYaxis()->SetTitleOffset(1.3);
    multi_slot_rms->GetXaxis()->SetTimeDisplay(1);
    multi_slot_rms->GetXaxis()->SetLabelSize(0.03);
    multi_slot_rms->GetXaxis()->SetLabelOffset(0.03);
    multi_slot_rms->GetXaxis()->SetTimeFormat("#splitline{%m/%d}{%H:%M}");
    leg_slot_rms->Draw();
    canvas_slot_freq->cd();
    multi_slot_freq->Draw("apl");
    //multi_slot_freq->GetYaxis()->SetRangeUser(0.,1.2*max_canvas_freq);
    multi_slot_freq->SetTitle("Crate 7 (6 hours)");
    multi_slot_freq->GetYaxis()->SetTitle("Freq [1/min]");
    multi_slot_freq->GetYaxis()->SetTitleSize(0.035);
    multi_slot_freq->GetYaxis()->SetTitleOffset(1.3);
    multi_slot_freq->GetXaxis()->SetTimeDisplay(1);
    multi_slot_freq->GetXaxis()->SetLabelSize(0.03);
    multi_slot_freq->GetXaxis()->SetLabelOffset(0.03);
    multi_slot_freq->GetXaxis()->SetTimeFormat("#splitline{%m/%d}{%H:%M}");
    leg_slot_freq->Draw();
    std::stringstream ss_slot, ss_slot_rms, ss_slot_freq;
    ss_slot<<outpath<<"Crate7_Slots_6HourTrend_TDC.jpg";
    ss_slot_rms<<outpath<<"Crate7_Slots_6HourTrend_RMS.jpg";
    ss_slot_freq<<outpath<<"Crate7_Slots_6HourTrend_Freq.jpg";
    canvas_slot->SaveAs(ss_slot.str().c_str());
    canvas_slot_rms->SaveAs(ss_slot_rms.str().c_str());
    canvas_slot_freq->SaveAs(ss_slot_freq.str().c_str());

    //6 HOURS - SLOTS (2)
    canvas_slot->Clear();
    canvas_slot_rms->Clear();
    canvas_slot_freq->Clear();
    max_canvas = 0;
    min_canvas = 9999999.;
    max_canvas_rms = 0;
    min_canvas_rms = 99999999.;
    max_canvas_freq = 0;
    min_canvas_freq = 999999999.;
    TMultiGraph *multi_slot2 = new TMultiGraph();
    TMultiGraph *multi_slot2_rms = new TMultiGraph();
    TMultiGraph *multi_slot2_freq = new TMultiGraph();
    TLegend *leg_slot2 = new TLegend(0.8,0.7,0.88,0.88);
    TLegend *leg_slot2_rms = new TLegend(0.8,0.7,0.88,0.88);
    TLegend *leg_slot2_freq = new TLegend(0.8,0.7,0.88,0.88);
    leg_slot2->SetLineColor(0);
    leg_slot2_rms->SetLineColor(0);
    leg_slot2_freq->SetLineColor(0);

    for (int i_slot = 0;i_slot<num_active_slots_cr2;i_slot++){
      multi_slot2->Add(gr_slot_times_sixhour.at(i_slot+num_active_slots_cr1));
      leg_slot2->AddEntry(gr_slot_times_sixhour.at(i_slot+num_active_slots_cr1),"","l");
      if (gr_slot_times_sixhour.at(i_slot)->GetMaximum()>max_canvas) max_canvas = gr_slot_times_sixhour.at(i_slot)->GetMaximum();
      if (gr_slot_times_sixhour.at(i_slot)->GetMinimum()<min_canvas) min_canvas = gr_slot_times_sixhour.at(i_slot)->GetMinimum();
      multi_slot2_rms->Add(gr_slot_rms_sixhour.at(i_slot+num_active_slots_cr1));
      leg_slot2_rms->AddEntry(gr_slot_rms_sixhour.at(i_slot+num_active_slots_cr1),"","l");
      if (gr_slot_rms_sixhour.at(i_slot)->GetMaximum()>max_canvas_rms) max_canvas_rms = gr_slot_rms_sixhour.at(i_slot)->GetMaximum();
      if (gr_slot_rms_sixhour.at(i_slot)->GetMinimum()<min_canvas_rms) min_canvas_rms = gr_slot_rms_sixhour.at(i_slot)->GetMinimum();
      multi_slot2_freq->Add(gr_slot_frequency_sixhour.at(i_slot+num_active_slots_cr1));
      leg_slot2_freq->AddEntry(gr_slot_frequency_sixhour.at(i_slot+num_active_slots_cr1),"","l");
      if (gr_slot_frequency_sixhour.at(i_slot)->GetMaximum()>max_canvas_freq) max_canvas_freq = gr_slot_frequency_sixhour.at(i_slot)->GetMaximum();
      if (gr_slot_frequency_sixhour.at(i_slot)->GetMinimum()<min_canvas_freq) min_canvas_freq = gr_slot_frequency_sixhour.at(i_slot)->GetMinimum();
    }

    canvas_slot->cd();
    multi_slot2->Draw("apl");
    //multi_slot2->GetYaxis()->SetRangeUser(0.,1.2*max_canvas);
    multi_slot2->SetTitle("Crate 8 (6 hours)");
    multi_slot2->GetYaxis()->SetTitle("TDC");
    multi_slot2->GetXaxis()->SetTimeDisplay(1);
    multi_slot2->GetXaxis()->SetLabelSize(0.03);
    multi_slot2->GetXaxis()->SetLabelOffset(0.03);
    multi_slot2->GetXaxis()->SetTimeFormat("#splitline{%m/%d}{%H:%M}");
    leg_slot2->Draw();
    canvas_slot_rms->cd();
    multi_slot2_rms->Draw("apl");
    //multi_slot2_rms->GetYaxis()->SetRangeUser(0.,1.2*max_canvas_rms);
    multi_slot2_rms->SetTitle("Crate 8 (6 hours)");
    multi_slot2_rms->GetYaxis()->SetTitle("RMS (TDC)");
    multi_slot2_rms->GetYaxis()->SetTitleSize(0.035);
    multi_slot2_rms->GetYaxis()->SetTitleOffset(1.3);
    multi_slot2_rms->GetXaxis()->SetTimeDisplay(1);
    multi_slot2_rms->GetXaxis()->SetLabelOffset(0.03);
    multi_slot2_rms->GetXaxis()->SetLabelSize(0.03);
    multi_slot2_rms->GetXaxis()->SetTimeFormat("#splitline{%m/%d}{%H:%M}");
    leg_slot2_rms->Draw();
    canvas_slot_freq->cd();
    multi_slot2_freq->Draw("apl");
    //multi_slot2_freq->GetYaxis()->SetRangeUser(0.,1.2*max_canvas_freq);
    multi_slot2_freq->SetTitle("Crate 8 (6 hours)");
    multi_slot2_freq->GetYaxis()->SetTitle("Freq [1/min]");
    multi_slot2_freq->GetYaxis()->SetTitleSize(0.035);
    multi_slot2_freq->GetYaxis()->SetTitleOffset(1.3);
    multi_slot2_freq->GetXaxis()->SetTimeDisplay(1);
    multi_slot2_freq->GetXaxis()->SetLabelOffset(0.03);
    multi_slot2_freq->GetXaxis()->SetLabelSize(0.03);
    multi_slot2_freq->GetXaxis()->SetTimeFormat("#splitline{%m/%d}{%H:%M}");
    leg_slot2_freq->Draw();
    ss_slot.str("");
    ss_slot_rms.str("");
    ss_slot_freq.str("");
    ss_slot<<outpath<<"Crate8_Slots_6HourTrend_TDC.jpg";
    ss_slot_rms<<outpath<<"Crate8_Slots_6HourTrend_RMS.jpg";
    ss_slot_freq<<outpath<<"Crate8_Slots_6HourTrend_Freq.jpg";
    canvas_slot->SaveAs(ss_slot.str().c_str());
    canvas_slot_rms->SaveAs(ss_slot_rms.str().c_str());
    canvas_slot_freq->SaveAs(ss_slot_freq.str().c_str());

    //6 HOURS - CRATE 
    canvas_crate->Clear();
    canvas_crate_rms->Clear();
    canvas_crate_freq->Clear();
    max_canvas = 0;
    min_canvas = 9999999.;
    max_canvas_rms = 0;
    min_canvas_rms = 99999999.;
    max_canvas_freq = 0;
    min_canvas_freq = 999999999.;
    TLegend *leg_crate = new TLegend(0.7,0.7,0.88,0.88);
    TLegend *leg_crate_rms = new TLegend(0.7,0.7,0.88,0.88);
    TLegend *leg_crate_freq = new TLegend(0.7,0.7,0.88,0.88);
    leg_crate->SetLineColor(0);
    leg_crate_rms->SetLineColor(0);
    leg_crate_freq->SetLineColor(0);
    TMultiGraph *multi_crate = new TMultiGraph();
    TMultiGraph *multi_crate_rms = new TMultiGraph();
    TMultiGraph *multi_crate_freq = new TMultiGraph();

    for (int i_crate=0;i_crate<num_crates;i_crate++){
      multi_crate->Add(gr_crate_times_sixhour.at(i_crate));
      leg_crate->AddEntry(gr_crate_times_sixhour.at(i_crate),"","l");
      if (gr_crate_times_sixhour.at(i_crate)->GetMaximum()>max_canvas) max_canvas = gr_crate_times_sixhour.at(i_crate)->GetMaximum();
      if (gr_crate_times_sixhour.at(i_crate)->GetMinimum()<min_canvas) min_canvas = gr_crate_times_sixhour.at(i_crate)->GetMinimum();
      multi_crate_rms->Add(gr_crate_rms_sixhour.at(i_crate));
      leg_crate_rms->AddEntry(gr_crate_rms_sixhour.at(i_crate),"","l");
      if (gr_crate_rms_sixhour.at(i_crate)->GetMaximum()>max_canvas_rms) max_canvas_rms = gr_crate_rms_sixhour.at(i_crate)->GetMaximum();
      if (gr_crate_rms_sixhour.at(i_crate)->GetMinimum()<min_canvas_rms) min_canvas_rms = gr_crate_rms_sixhour.at(i_crate)->GetMinimum();
      multi_crate_freq->Add(gr_crate_frequency_sixhour.at(i_crate));
      leg_crate_freq->AddEntry(gr_crate_frequency_sixhour.at(i_crate),"","l");
      if (gr_crate_frequency_sixhour.at(i_crate)->GetMaximum()>max_canvas_freq) max_canvas_freq = gr_crate_frequency_sixhour.at(i_crate)->GetMaximum();
      if (gr_crate_frequency_sixhour.at(i_crate)->GetMinimum()<min_canvas_freq) min_canvas_freq = gr_crate_frequency_sixhour.at(i_crate)->GetMinimum();
    }

    canvas_crate->cd();
    multi_crate->Draw("apl");
    //multi_crate->GetYaxis()->SetRangeUser(0.,1.2*max_canvas);
    multi_crate->SetTitle("Crates (6 hours)");
    multi_crate->GetYaxis()->SetTitle("TDC");
    multi_crate->GetXaxis()->SetTimeDisplay(1);
    multi_crate->GetXaxis()->SetLabelOffset(0.03);
    multi_crate->GetXaxis()->SetLabelSize(0.03);
    multi_crate->GetXaxis()->SetTimeFormat("#splitline{%m/%d}{%H:%M}");
    leg_crate->Draw();
    canvas_crate_rms->cd();
    multi_crate_rms->Draw("apl");
    //multi_crate_rms->GetYaxis()->SetRangeUser(0.,1.2*max_canvas_rms);
    multi_crate_rms->SetTitle("Crates (6 hours)");
    multi_crate_rms->GetYaxis()->SetTitle("RMS (TDC)");
    multi_crate_rms->GetYaxis()->SetTitleSize(0.035);
    multi_crate_rms->GetYaxis()->SetTitleOffset(1.3);
    multi_crate_rms->GetXaxis()->SetTimeDisplay(1);
    multi_crate_rms->GetXaxis()->SetLabelOffset(0.03);
    multi_crate_rms->GetXaxis()->SetLabelSize(0.03);
    multi_crate_rms->GetXaxis()->SetTimeFormat("#splitline{%m/%d}{%H:%M}");
    leg_crate_rms->Draw();
    canvas_crate_freq->cd();
    multi_crate_freq->Draw("apl");
    //multi_crate_freq->GetYaxis()->SetRangeUser(0.,1.2*max_canvas_freq);
    multi_crate_freq->SetTitle("Crates (6 hours)");
    multi_crate_freq->GetYaxis()->SetTitle("Freq [1/min]");
    multi_crate_freq->GetYaxis()->SetTitleSize(0.035);
    multi_crate_freq->GetYaxis()->SetTitleOffset(1.3);
    multi_crate_freq->GetXaxis()->SetTimeDisplay(1);
    multi_crate_freq->GetXaxis()->SetLabelSize(0.03);
    multi_crate_freq->GetXaxis()->SetLabelOffset(0.03);
    multi_crate_freq->GetXaxis()->SetTimeFormat("#splitline{%m/%d}{%H:%M}");
    leg_crate_freq->Draw();
    std::stringstream ss_crate, ss_crate_rms, ss_crate_freq;
    ss_crate<<outpath<<"Crates_6HourTrend_TDC.jpg";
    ss_crate_rms<<outpath<<"Crates_6HourTrend_RMS.jpg";
    ss_crate_freq<<outpath<<"Crates_6HourTrend_Freq.jpg";
    canvas_crate->SaveAs(ss_crate.str().c_str());
    canvas_crate_rms->SaveAs(ss_crate_rms.str().c_str());
    canvas_crate_freq->SaveAs(ss_crate_freq.str().c_str());

    for (int i_slot = 0;i_slot<num_active_slots_cr2;i_slot++){
      multi_slot->RecursiveRemove(gr_slot_times_sixhour.at(i_slot));
      multi_slot_rms->RecursiveRemove(gr_slot_rms_sixhour.at(i_slot));
      multi_slot_freq->RecursiveRemove(gr_slot_frequency_sixhour.at(i_slot));
      multi_slot2->RecursiveRemove(gr_slot_times_sixhour.at(i_slot+num_active_slots_cr1));
      multi_slot2_rms->RecursiveRemove(gr_slot_rms_sixhour.at(i_slot+num_active_slots_cr1));
      multi_slot2_freq->RecursiveRemove(gr_slot_frequency_sixhour.at(i_slot+num_active_slots_cr1));
    }
    
    for (int i_crate = 0; i_crate < num_crates; i_crate++){
      multi_crate->RecursiveRemove(gr_crate_times_sixhour.at(i_crate));
      multi_crate_rms->RecursiveRemove(gr_crate_rms_sixhour.at(i_crate));
      multi_crate_freq->RecursiveRemove(gr_crate_frequency_sixhour.at(i_crate));
    }

    delete multi_slot;
    delete multi_slot_rms;
    delete multi_slot_freq;
    delete multi_slot2;
    delete multi_slot2_rms;
    delete multi_slot2_freq;
    delete multi_crate;
    delete multi_crate_rms;
    delete multi_crate_freq;

    delete leg_slot;
    delete leg_slot_rms;
    delete leg_slot_freq;
    delete leg_slot2;
    delete leg_slot2_rms;
    delete leg_slot2_freq;
    delete leg_crate;
    delete leg_crate_rms;
    delete leg_crate_freq;

  }

  //-------------------------------------------------------
  //----------------Update Day Plots ----------------------
  //-------------------------------------------------------

  if (update_hour){

    //DAY - CHANNELS
    TLegend *leg = new TLegend(0.7,0.7,0.88,0.88);
    TLegend *leg_rms = new TLegend(0.7,0.7,0.88,0.88);
    TLegend *leg_freq = new TLegend(0.7,0.7,0.88,0.88);
    max_canvas = 0;
    min_canvas = 9999999.;
    max_canvas_rms = 0;
    min_canvas_rms = 99999999.;
    max_canvas_freq = 0;
    min_canvas_freq = 999999999.;
    TMultiGraph *multi_ch = new TMultiGraph();
    TMultiGraph *multi_ch_rms = new TMultiGraph();
    TMultiGraph *multi_ch_freq = new TMultiGraph();
    int num_ch = 16;
    int enum_ch_canvas = 0;

    for (int i_channel =0;i_channel<times_channel_day.size();i_channel++){

      std::stringstream ss_ch, ss_ch_rms, ss_ch_freq;

      if (i_channel%num_ch == 0 || i_channel == times_channel_day.size()-1) {
        if (i_channel != 0){
          int num_crate = (i_channel>num_active_slots_cr1*num_channels)? min_crate+1 : min_crate;
          int num_slot = (mapping_vector_ch.at(i_channel-1)-(i_channel-1)%num_channels-(num_crate-min_crate)*num_slots)/num_channels+1;
          ss_ch.str("");
          ss_ch_rms.str("");
          ss_ch_freq.str("");
          ss_ch<<"Crate "<<num_crate<<", Slot "<<num_slot<<" (day) ["<<enum_ch_canvas+1<<"]";
          ss_ch_rms<<"Crate "<<num_crate<<", Slot "<<num_slot<<" (day) ["<<enum_ch_canvas+1<<"]";
          ss_ch_freq<<"Crate "<<num_crate<<", Slot "<<num_slot<<" (day) ["<<enum_ch_canvas+1<<"]";
          if ( i_channel == times_channel_hour.size() - 1){
            ss_leg_ch.str("");
            ss_leg_ch<<"ch "<<i_channel%num_channels;
            multi_ch->Add(gr_times_day.at(i_channel));
            leg->AddEntry(gr_times_day.at(i_channel),ss_leg_ch.str().c_str(),"l");
            multi_ch_rms->Add(gr_rms_day.at(i_channel));
            leg_rms->AddEntry(gr_rms_day.at(i_channel),ss_leg_ch.str().c_str(),"l");
            multi_ch_freq->Add(gr_frequency_day.at(i_channel));
            leg_freq->AddEntry(gr_frequency_day.at(i_channel),ss_leg_ch.str().c_str(),"l");
          }
          canvas_ch->cd();
          multi_ch->Draw("apl");
          //multi_ch->GetYaxis()->SetRangeUser(0.,1.2*max_canvas);
          multi_ch->SetTitle(ss_ch.str().c_str());
          multi_ch->GetYaxis()->SetTitle("TDC");
          multi_ch->GetXaxis()->SetTimeDisplay(1);
          multi_ch->GetXaxis()->SetLabelOffset(0.03);
          multi_ch->GetXaxis()->SetLabelSize(0.03);
          multi_ch->GetXaxis()->SetTimeFormat("#splitline{%m/%d}{%H:%M}");
          leg->Draw();
          canvas_ch_rms->cd();
          multi_ch_rms->Draw("apl");
          //multi_ch_rms->GetYaxis()->SetRangeUser(0.,1.2*max_canvas_rms);
          multi_ch_rms->SetTitle(ss_ch_rms.str().c_str());
          multi_ch_rms->GetYaxis()->SetTitle("RMS (TDC)");
          multi_ch_rms->GetYaxis()->SetTitleSize(0.035);
          multi_ch_rms->GetYaxis()->SetTitleOffset(1.3);
          multi_ch_rms->GetXaxis()->SetTimeDisplay(1);
          multi_ch_rms->GetXaxis()->SetLabelSize(0.03);
          multi_ch_rms->GetXaxis()->SetLabelOffset(0.03);
          multi_ch_rms->GetXaxis()->SetTimeFormat("#splitline{%m/%d}{%H:%M}");
          leg_rms->Draw();
          canvas_ch_freq->cd();
          multi_ch_freq->Draw("apl");
          //multi_ch_freq->GetYaxis()->SetRangeUser(0.,1.2*max_canvas_freq);
          multi_ch_freq->SetTitle(ss_ch_freq.str().c_str());
          multi_ch_freq->GetYaxis()->SetTitle("Freq [1/min]");
          multi_ch_freq->GetYaxis()->SetTitleSize(0.035);
          multi_ch_freq->GetYaxis()->SetTitleOffset(1.3);
          multi_ch_freq->GetXaxis()->SetTimeDisplay(1);
          multi_ch_freq->GetXaxis()->SetLabelSize(0.03);
          multi_ch_freq->GetXaxis()->SetLabelOffset(0.03);
          multi_ch_freq->GetXaxis()->SetTimeFormat("#splitline{%m/%d}{%H:%M}");
          leg_freq->Draw();
          ss_ch.str("");
          ss_ch_rms.str("");
          ss_ch_freq.str("");
          ss_ch<<outpath<<"Crate"<<num_crate<<"_Slot"<<num_slot<<"_"<<enum_ch_canvas<<"_DayTrend_TDC.jpg";
          ss_ch_rms<<outpath<<"Crate"<<num_crate<<"_Slot"<<num_slot<<"_"<<enum_ch_canvas<<"_DayTrend_RMS.jpg";
          ss_ch_freq<<outpath<<"Crate"<<num_crate<<"_Slot"<<num_slot<<"_"<<enum_ch_canvas<<"_DayTrend_Freq.jpg";
          canvas_ch->SaveAs(ss_ch.str().c_str());
          canvas_ch_rms->SaveAs(ss_ch_rms.str().c_str());
          canvas_ch_freq->SaveAs(ss_ch_freq.str().c_str());   
          enum_ch_canvas++;
          enum_ch_canvas = enum_ch_canvas%2; 

          for (int i_gr=0; i_gr < num_ch; i_gr++){
            int i_balance = (i_channel == times_channel_day.size()-1)? 1 : 0;
            multi_ch->RecursiveRemove(gr_times_day.at(i_channel-num_ch+i_gr+i_balance));
            multi_ch_rms->RecursiveRemove(gr_rms_day.at(i_channel-num_ch+i_gr+i_balance));
            multi_ch_freq->RecursiveRemove(gr_frequency_day.at(i_channel-num_ch+i_gr+i_balance));
          }
        }

        canvas_ch->Clear();
        canvas_ch_rms->Clear();
        canvas_ch_freq->Clear();

        delete multi_ch;
        delete multi_ch_rms;
        delete multi_ch_freq;
        delete leg;
        delete leg_rms;
        delete leg_freq;

        if (i_channel != times_channel_hour.size() - 1){
          multi_ch = new TMultiGraph();
          multi_ch_rms = new TMultiGraph();
          multi_ch_freq = new TMultiGraph();
          leg = new TLegend(0.7,0.7,0.88,0.88);
          leg->SetNColumns(4);
          leg_rms = new TLegend(0.7,0.7,0.88,0.88);
          leg_rms->SetNColumns(4);
          leg_freq = new TLegend(0.7,0.7,0.88,0.88);
          leg_freq->SetNColumns(4);
          leg->SetLineColor(0);
          leg_rms->SetLineColor(0);
          leg_freq->SetLineColor(0);
        }
      }

      if (i_channel != times_channel_hour.size() -1){
        ss_leg_ch.str("");
        ss_leg_ch<<"ch "<<i_channel%num_channels;
        multi_ch->Add(gr_times_day.at(i_channel));
        leg->AddEntry(gr_times_day.at(i_channel),ss_leg_ch.str().c_str(),"l");
        if (gr_times_day.at(i_channel)->GetMaximum()>max_canvas) max_canvas = gr_times_day.at(i_channel)->GetMaximum();
        if (gr_times_day.at(i_channel)->GetMinimum()<min_canvas) min_canvas = gr_times_day.at(i_channel)->GetMinimum();
        multi_ch_rms->Add(gr_rms_day.at(i_channel));
        leg_rms->AddEntry(gr_rms_day.at(i_channel),ss_leg_ch.str().c_str(),"l");
        if (gr_rms_day.at(i_channel)->GetMaximum()>max_canvas_rms) max_canvas_rms = gr_rms_day.at(i_channel)->GetMaximum();
        if (gr_rms_day.at(i_channel)->GetMinimum()<min_canvas_rms) min_canvas_rms = gr_rms_day.at(i_channel)->GetMinimum();
        multi_ch_freq->Add(gr_frequency_day.at(i_channel));
        leg_freq->AddEntry(gr_frequency_day.at(i_channel),ss_leg_ch.str().c_str(),"l");
        if (gr_frequency_day.at(i_channel)->GetMaximum()>max_canvas_freq) max_canvas_freq = gr_frequency_day.at(i_channel)->GetMaximum();
        if (gr_frequency_day.at(i_channel)->GetMinimum()<min_canvas_freq) min_canvas_freq = gr_frequency_day.at(i_channel)->GetMinimum();
      }
    } 

    //DAY - SLOTS (1)
    TLegend *leg_slot = new TLegend(0.8,0.7,0.88,0.88);
    TLegend *leg_slot_rms = new TLegend(0.8,0.7,0.88,0.88);
    TLegend *leg_slot_freq = new TLegend(0.8,0.7,0.88,0.88);
    leg_slot->SetLineColor(0);
    leg_slot_rms->SetLineColor(0);
    leg_slot_freq->SetLineColor(0);
    max_canvas = 0;
    min_canvas = 9999999.;
    max_canvas_rms = 0;
    min_canvas_rms = 99999999.;
    max_canvas_freq = 0;
    min_canvas_freq = 999999999.;
    TMultiGraph *multi_slot = new TMultiGraph();
    TMultiGraph *multi_slot_rms = new TMultiGraph();
    TMultiGraph *multi_slot_freq = new TMultiGraph();

    for (int i_slot = 0;i_slot<num_active_slots_cr1;i_slot++){
      multi_slot->Add(gr_slot_times_day.at(i_slot));
      leg_slot->AddEntry(gr_slot_times_day.at(i_slot),"","l");
      if (gr_slot_times_day.at(i_slot)->GetMaximum()>max_canvas) max_canvas = gr_slot_times_day.at(i_slot)->GetMaximum();
      if (gr_slot_times_day.at(i_slot)->GetMinimum()<min_canvas) min_canvas = gr_slot_times_day.at(i_slot)->GetMinimum();
      multi_slot_rms->Add(gr_slot_rms_day.at(i_slot));
      leg_slot_rms->AddEntry(gr_slot_rms_day.at(i_slot),"","l");
      if (gr_slot_rms_day.at(i_slot)->GetMaximum()>max_canvas_rms) max_canvas_rms = gr_slot_rms_day.at(i_slot)->GetMaximum();
      if (gr_slot_rms_day.at(i_slot)->GetMinimum()<min_canvas_rms) min_canvas_rms = gr_slot_rms_day.at(i_slot)->GetMinimum();
      multi_slot_freq->Add(gr_slot_frequency_day.at(i_slot));
      leg_slot_freq->AddEntry(gr_slot_frequency_day.at(i_slot),"","l");
      if (gr_slot_frequency_day.at(i_slot)->GetMaximum()>max_canvas_freq) max_canvas_freq = gr_slot_frequency_day.at(i_slot)->GetMaximum();
      if (gr_slot_frequency_day.at(i_slot)->GetMinimum()<min_canvas_freq) min_canvas_freq = gr_slot_frequency_day.at(i_slot)->GetMinimum();
    }

    canvas_slot->cd();
    multi_slot->Draw("apl");
    //multi_slot->GetYaxis()->SetRangeUser(0.,1.2*max_canvas);
    multi_slot->SetTitle("Crate 7 (day)");
    multi_slot->GetYaxis()->SetTitle("TDC");
    multi_slot->GetXaxis()->SetTimeDisplay(1);
    multi_slot->GetXaxis()->SetLabelOffset(0.03);
    multi_slot->GetXaxis()->SetLabelSize(0.03);
    multi_slot->GetXaxis()->SetTimeFormat("#splitline{%m/%d}{%H:%M}");
    leg_slot->Draw();
    canvas_slot_rms->cd();
    multi_slot_rms->Draw("apl");
   // multi_slot_rms->GetYaxis()->SetRangeUser(0.,1.2*max_canvas_rms);
    multi_slot_rms->SetTitle("Crate 7 (day)");
    multi_slot_rms->GetYaxis()->SetTitleSize(0.035);
    multi_slot_rms->GetYaxis()->SetTitleOffset(1.3);
    multi_slot_rms->GetYaxis()->SetTitle("RMS (TDC)");
    multi_slot_rms->GetXaxis()->SetTimeDisplay(1);
    multi_slot_rms->GetXaxis()->SetLabelOffset(0.03);
    multi_slot_rms->GetXaxis()->SetLabelSize(0.03);
    multi_slot_rms->GetXaxis()->SetTimeFormat("#splitline{%m/%d}{%H:%M}");
    leg_slot_rms->Draw();
    canvas_slot_freq->cd();
    multi_slot_freq->Draw("apl");
    //multi_slot_freq->GetYaxis()->SetRangeUser(0.,1.2*max_canvas_freq);
    multi_slot_freq->SetTitle("Crate 7 (day)");
    multi_slot_freq->GetYaxis()->SetTitle("Freq [1/min]");
    multi_slot_freq->GetYaxis()->SetTitleSize(0.035);
    multi_slot_freq->GetYaxis()->SetTitleOffset(1.3);
    multi_slot_freq->GetXaxis()->SetTimeDisplay(1);
    multi_slot_freq->GetXaxis()->SetLabelOffset(0.03);
    multi_slot_freq->GetXaxis()->SetLabelSize(0.03);
    multi_slot_freq->GetXaxis()->SetTimeFormat("#splitline{%m/%d}{%H:%M}");
    leg_slot_freq->Draw();

    //DAY - SLOTS (2)
    std::stringstream ss_slot, ss_slot_rms, ss_slot_freq;
    ss_slot<<outpath<<"Crate7_Slots_DayTrend_TDC.jpg";
    ss_slot_rms<<outpath<<"Crate7_Slots_DayTrend_RMS.jpg";
    ss_slot_freq<<outpath<<"Crate7_Slots_DayTrend_Freq.jpg";
    canvas_slot->SaveAs(ss_slot.str().c_str());
    canvas_slot_rms->SaveAs(ss_slot_rms.str().c_str());
    canvas_slot_freq->SaveAs(ss_slot_freq.str().c_str());
    canvas_slot->Clear();
    canvas_slot_rms->Clear();
    canvas_slot_freq->Clear();
    max_canvas = 0;
    min_canvas = 9999999.;
    max_canvas_rms = 0;
    min_canvas_rms = 99999999.;
    max_canvas_freq = 0;
    min_canvas_freq = 999999999.;
    TMultiGraph *multi_slot2 = new TMultiGraph();
    TMultiGraph *multi_slot2_rms = new TMultiGraph();
    TMultiGraph *multi_slot2_freq = new TMultiGraph();
    TLegend *leg_slot2 = new TLegend(0.8,0.7,0.88,0.88);
    TLegend *leg_slot2_rms = new TLegend(0.8,0.7,0.88,0.88);
    TLegend *leg_slot2_freq = new TLegend(0.8,0.7,0.88,0.88);
    leg_slot2->SetLineColor(0);
    leg_slot2_rms->SetLineColor(0);
    leg_slot2_freq->SetLineColor(0);

    for (int i_slot = 0;i_slot<num_active_slots_cr2;i_slot++){
      multi_slot2->Add(gr_slot_times_day.at(i_slot+num_active_slots_cr1));
      leg_slot2->AddEntry(gr_slot_times_day.at(i_slot+num_active_slots_cr1),"","l");
      if (gr_slot_times_day.at(i_slot)->GetMaximum()>max_canvas) max_canvas = gr_slot_times_day.at(i_slot)->GetMaximum();
      if (gr_slot_times_day.at(i_slot)->GetMinimum()<min_canvas) min_canvas = gr_slot_times_day.at(i_slot)->GetMinimum();
      multi_slot2_rms->Add(gr_slot_rms_day.at(i_slot+num_active_slots_cr1));
      leg_slot2_rms->AddEntry(gr_slot_rms_day.at(i_slot+num_active_slots_cr1),"","l");
      if (gr_slot_rms_day.at(i_slot)->GetMaximum()>max_canvas_rms) max_canvas_rms = gr_slot_rms_day.at(i_slot)->GetMaximum();
      if (gr_slot_rms_day.at(i_slot)->GetMinimum()<min_canvas_rms) min_canvas_rms = gr_slot_rms_day.at(i_slot)->GetMinimum();
      multi_slot2_freq->Add(gr_slot_frequency_day.at(i_slot+num_active_slots_cr1));
      leg_slot2_freq->AddEntry(gr_slot_frequency_day.at(i_slot+num_active_slots_cr1),"","l");
      if (gr_slot_frequency_day.at(i_slot)->GetMaximum()>max_canvas_freq) max_canvas_freq = gr_slot_frequency_day.at(i_slot)->GetMaximum();
      if (gr_slot_frequency_day.at(i_slot)->GetMinimum()<min_canvas_freq) min_canvas_freq = gr_slot_frequency_day.at(i_slot)->GetMinimum();
    }

    canvas_slot->cd();
    multi_slot2->Draw("apl");
    //multi_slot2->GetYaxis()->SetRangeUser(0.,1.2*max_canvas);
    multi_slot2->SetTitle("Crate 8 (day)");
    multi_slot2->GetYaxis()->SetTitle("TDC");
    multi_slot2->GetXaxis()->SetTimeDisplay(1);
    multi_slot2->GetXaxis()->SetLabelSize(0.03);
    multi_slot2->GetXaxis()->SetLabelOffset(0.03);
    multi_slot2->GetXaxis()->SetTimeFormat("#splitline{%m/%d}{%H:%M}");
    leg_slot2->Draw();
    canvas_slot_rms->cd();
    multi_slot2_rms->Draw("apl");
    //multi_slot2_rms->GetYaxis()->SetRangeUser(0.,1.2*max_canvas_rms);
    multi_slot2_rms->SetTitle("Crate 8 (day)");
    multi_slot2_rms->GetYaxis()->SetTitleSize(0.035);
    multi_slot2_rms->GetYaxis()->SetTitleOffset(1.3);
    multi_slot2_rms->GetYaxis()->SetTitle("RMS (TDC)");
    multi_slot2_rms->GetXaxis()->SetTimeDisplay(1);
    multi_slot2_rms->GetXaxis()->SetLabelSize(0.03);
    multi_slot2_rms->GetXaxis()->SetLabelOffset(0.03);
    multi_slot2_rms->GetXaxis()->SetTimeFormat("#splitline{%m/%d}{%H:%M}");
    leg_slot2_rms->Draw();
    canvas_slot_freq->cd();
    multi_slot2_freq->Draw("apl");
    multi_slot2_freq->SetTitle("Crate 8 (day)");
    //multi_slot2_freq->GetYaxis()->SetRangeUser(0.,1.2*max_canvas_freq);
    multi_slot2_freq->GetYaxis()->SetTitle("Freq [1/min]");
    multi_slot2_freq->GetYaxis()->SetTitleSize(0.035);
    multi_slot2_freq->GetYaxis()->SetTitleOffset(1.3);
    multi_slot2_freq->GetXaxis()->SetTimeDisplay(1);
    multi_slot2_freq->GetXaxis()->SetLabelSize(0.03);
    multi_slot2_freq->GetXaxis()->SetLabelOffset(0.03);
    multi_slot2_freq->GetXaxis()->SetTimeFormat("#splitline{%m/%d}{%H:%M}");
    leg_slot2_freq->Draw();
    ss_slot.str("");
    ss_slot_rms.str("");
    ss_slot_freq.str("");
    ss_slot<<outpath<<"Crate8_Slots_DayTrend_TDC.jpg";
    ss_slot_rms<<outpath<<"Crate8_Slots_DayTrend_RMS.jpg";
    ss_slot_freq<<outpath<<"Crate8_Slots_DayTrend_Freq.jpg";
    canvas_slot->SaveAs(ss_slot.str().c_str());
    canvas_slot_rms->SaveAs(ss_slot_rms.str().c_str());
    canvas_slot_freq->SaveAs(ss_slot_freq.str().c_str());

    //DAY - CRATE
    canvas_crate->Clear();
    canvas_crate_rms->Clear();
    canvas_crate_freq->Clear();
    TLegend *leg_crate = new TLegend(0.7,0.7,0.88,0.88);
    TLegend *leg_crate_rms = new TLegend(0.7,0.7,0.88,0.88);
    TLegend *leg_crate_freq = new TLegend(0.7,0.7,0.88,0.88);
    leg_crate->SetLineColor(0);
    leg_crate_rms->SetLineColor(0);
    leg_crate_freq->SetLineColor(0);
    max_canvas = 0;
    min_canvas = 9999999.;
    max_canvas_rms = 0;
    min_canvas_rms = 99999999.;
    max_canvas_freq = 0;
    min_canvas_freq = 999999999.;
    TMultiGraph *multi_crate = new TMultiGraph();
    TMultiGraph *multi_crate_rms = new TMultiGraph();
    TMultiGraph *multi_crate_freq = new TMultiGraph();

    for (int i_crate=0;i_crate<num_crates;i_crate++){
      multi_crate->Add(gr_crate_times_day.at(i_crate));
      leg_crate->AddEntry(gr_crate_times_day.at(i_crate),"","l");
      if (gr_crate_times_day.at(i_crate)->GetMaximum()>max_canvas) max_canvas = gr_crate_times_day.at(i_crate)->GetMaximum();
      if (gr_crate_times_day.at(i_crate)->GetMinimum()<min_canvas) min_canvas = gr_crate_times_day.at(i_crate)->GetMinimum();
      multi_crate_rms->Add(gr_crate_rms_day.at(i_crate));
      leg_crate_rms->AddEntry(gr_crate_rms_day.at(i_crate),"","l");
      if (gr_crate_rms_day.at(i_crate)->GetMaximum()>max_canvas_rms) max_canvas_rms = gr_crate_rms_day.at(i_crate)->GetMaximum();
      if (gr_crate_rms_day.at(i_crate)->GetMinimum()<min_canvas_rms) min_canvas_rms = gr_crate_rms_day.at(i_crate)->GetMinimum();
      multi_crate_freq->Add(gr_crate_frequency_day.at(i_crate));
      leg_crate_freq->AddEntry(gr_crate_frequency_day.at(i_crate),"","l");
      if (gr_crate_frequency_day.at(i_crate)->GetMaximum()>max_canvas_freq) max_canvas_freq = gr_crate_frequency_day.at(i_crate)->GetMaximum();
      if (gr_crate_frequency_day.at(i_crate)->GetMinimum()<min_canvas_freq) min_canvas_freq = gr_crate_frequency_day.at(i_crate)->GetMinimum();
    }

    canvas_crate->cd();
    multi_crate->Draw("apl");
    //multi_crate->GetYaxis()->SetRangeUser(0.,1.2*max_canvas);
    multi_crate->SetTitle("Crates (day)");
    multi_crate->GetYaxis()->SetTitle("TDC");
    multi_crate->GetXaxis()->SetTimeDisplay(1);
    multi_crate->GetXaxis()->SetLabelOffset(0.03);
    multi_crate->GetXaxis()->SetLabelSize(0.03);
    multi_crate->GetXaxis()->SetTimeFormat("#splitline{%m/%d}{%H:%M}");
    leg_crate->Draw();
    canvas_crate_rms->cd();
    multi_crate_rms->Draw("apl");
    //multi_crate_rms->GetYaxis()->SetRangeUser(0.,1.2*max_canvas_rms);
    multi_crate_rms->SetTitle("Crates (day)");
    multi_crate_rms->GetYaxis()->SetTitleSize(0.035);
    multi_crate_rms->GetYaxis()->SetTitleOffset(1.3);
    multi_crate_rms->GetYaxis()->SetTitle("RMS (TDC)");
    multi_crate_rms->GetXaxis()->SetTimeDisplay(1);
    multi_crate_rms->GetXaxis()->SetLabelOffset(0.03);
    multi_crate_rms->GetXaxis()->SetLabelSize(0.03);
    multi_crate_rms->GetXaxis()->SetTimeFormat("#splitline{%m/%d}{%H:%M}");
    leg_crate_rms->Draw();
    canvas_crate_freq->cd();
    multi_crate_freq->Draw("apl");
    //multi_crate_freq->GetYaxis()->SetRangeUser(0.,1.2*max_canvas_freq);
    multi_crate_freq->SetTitle("Crates (day)");
    multi_crate_freq->GetYaxis()->SetTitle("RMS (TDC)");
    multi_crate_freq->GetYaxis()->SetTitleSize(0.035);
    multi_crate_freq->GetYaxis()->SetTitleOffset(1.3);
    multi_crate_freq->GetXaxis()->SetTimeDisplay(1);
    multi_crate_freq->GetXaxis()->SetLabelOffset(0.03);
    multi_crate_freq->GetXaxis()->SetLabelSize(0.03);
    multi_crate_freq->GetXaxis()->SetTimeFormat("#splitline{%m/%d}{%H:%M}");
    multi_crate_freq->GetYaxis()->SetTitle("Freq [1/min]");
    leg_crate_freq->Draw();
    std::stringstream ss_crate, ss_crate_rms, ss_crate_freq;
    ss_crate<<outpath<<"Crates_DayTrend_TDC.jpg";
    ss_crate_rms<<outpath<<"Crates_DayTrend_RMS.jpg";
    ss_crate_freq<<outpath<<"Crates_DayTrend_Freq.jpg";
    canvas_crate->SaveAs(ss_crate.str().c_str());
    canvas_crate_rms->SaveAs(ss_crate_rms.str().c_str());
    canvas_crate_freq->SaveAs(ss_crate_freq.str().c_str());

    for (int i_slot = 0;i_slot<num_active_slots_cr2;i_slot++){
      multi_slot->RecursiveRemove(gr_slot_times_day.at(i_slot));
      multi_slot_rms->RecursiveRemove(gr_slot_rms_day.at(i_slot));
      multi_slot_freq->RecursiveRemove(gr_slot_frequency_day.at(i_slot));
      multi_slot2->RecursiveRemove(gr_slot_times_day.at(i_slot+num_active_slots_cr1));
      multi_slot2_rms->RecursiveRemove(gr_slot_rms_day.at(i_slot+num_active_slots_cr1));
      multi_slot2_freq->RecursiveRemove(gr_slot_frequency_day.at(i_slot+num_active_slots_cr1));
    }

    for (int i_crate = 0; i_crate < num_crates; i_crate++){
      multi_crate->RecursiveRemove(gr_crate_times_day.at(i_crate));
      multi_crate_rms->RecursiveRemove(gr_crate_rms_day.at(i_crate));
      multi_crate_freq->RecursiveRemove(gr_crate_frequency_day.at(i_crate));
    }

    delete multi_slot;
    delete multi_slot_rms;
    delete multi_slot_freq;
    delete multi_slot2;
    delete multi_slot2_rms;
    delete multi_slot2_freq;
    delete multi_crate;
    delete multi_crate_rms;
    delete multi_crate_freq;

    delete leg_slot;
    delete leg_slot_rms;
    delete leg_slot_freq;
    delete leg_slot2;
    delete leg_slot2_rms;
    delete leg_slot2_freq;
    delete leg_crate;
    delete leg_crate_rms;
    delete leg_crate_freq;


  }
  }//close draw average

  //std::cout <<"deleting canvases..."<<std::endl;

  update_mins = false;
  update_halfhour = false;
  update_hour = false;

  delete canvas_ch;
  delete canvas_ch_rms;
  delete canvas_ch_freq;
  delete canvas_slot;
  delete canvas_slot_rms;
  delete canvas_slot_freq;
  delete canvas_crate;
  delete canvas_crate_rms;
  delete canvas_crate_freq;
  delete canvas_ch_hist;
  delete canvas_ch_hist_sixhour;
  delete canvas_ch_hist_day;
  delete canvas_hitmap_fivemin;
  delete canvas_hitmap_hour;
  delete canvas_hitmap_sixhour;
  delete canvas_hitmap_day;

  for (int i_channel = 0; i_channel < num_channels; i_channel++){
    delete canvas_ch_temp[i_channel];
  }

  //std::cout <<"deleted all canvases..."<<std::endl;

}

void MonitorMRDTime::UpdateMonitorSources(){

  //-------------------------------------------------------
  //----------------UpdateMonitorSources ------------------
  //-------------------------------------------------------

	//shift all entries by 1 to the left & insert newest event (live) as the last entry of all arrays in the vectors

  if (verbosity > 2) std::cout <<"Update MonitorSources...."<<std::endl;
		for (int i_channel = 0; i_channel<num_channels*num_active_slots; i_channel++){
      if (update_mins){
        if (verbosity > 4) std::cout <<"MRDMonitorTime: Updating channel "<<i_channel+1<<std::endl;

        //---------------------------------------------------
        //----------------Hitmap vectors --------------------
        //---------------------------------------------------
        if (draw_hitmap){
        for (int i_overall_mins = 0;i_overall_mins<num_overall_fivemin-1;i_overall_mins++){
          n_channel_overall_day.at(i_channel)[i_overall_mins] = n_channel_overall_day.at(i_channel)[i_overall_mins+1];
        }
        if (!data_available || live_mins.at(i_channel).empty()) n_channel_overall_day.at(i_channel)[num_overall_fivemin-1] = 0;
        else n_channel_overall_day.at(i_channel)[num_overall_fivemin-1] = live_mins.at(i_channel).size();
        } //close draw hitmap

        //---------------------------------------------------
        //----------------Update 1 hour plots----------------
        //---------------------------------------------------

        //if (draw_average){
  			for (int i_mins = 0;i_mins<num_fivemin-1;i_mins++){
          /*std::cout <<"i_mins: "<<i_mins<<std::endl;
          std::cout<<"times channel: "<<times_channel_hour.at(i_channel)[i_mins+1]<<std::endl;
          std::cout<<"rms: "<<rms_channel_hour.at(i_channel)[i_mins+1]<<std::endl;
          std::cout<<"freq: "<<frequency_channel_hour.at(i_channel)[i_mins+1]<<std::endl;
          std::cout<<"n: "<<n_channel_hour.at(i_channel)[i_mins+1]<<std::endl;
          std::cout<<"earliest: "<<earliest_channel_hour.at(i_channel)[i_mins]<<std::endl;
          std::cout<<"latest: "<<latest_channel_hour.at(i_channel)[i_mins+1]<<std::endl;*/
  				times_channel_hour.at(i_channel)[i_mins] = times_channel_hour.at(i_channel)[i_mins+1];
  				rms_channel_hour.at(i_channel)[i_mins] = rms_channel_hour.at(i_channel)[i_mins+1];
  				frequency_channel_hour.at(i_channel)[i_mins] = frequency_channel_hour.at(i_channel)[i_mins+1];
          n_channel_hour.at(i_channel)[i_mins] = n_channel_hour.at(i_channel)[i_mins+1];
          earliest_channel_hour.at(i_channel)[i_mins] = earliest_channel_hour.at(i_channel)[i_mins+1];
          latest_channel_hour.at(i_channel)[i_mins] = latest_channel_hour.at(i_channel)[i_mins+1];
  			}
  			if (!data_available || live_mins.at(i_channel).empty()){
      		times_channel_hour.at(i_channel)[num_fivemin-1] = 0;
      		rms_channel_hour.at(i_channel)[num_fivemin-1] = 0;
      		frequency_channel_hour.at(i_channel)[num_fivemin-1] = 0;
          n_channel_hour.at(i_channel)[num_fivemin-1] = 0;
          //n_channel_overall_day.at(i_channel)[num_overall_fivemin-1] = 0;
          earliest_channel_hour.at(i_channel)[num_fivemin-1] = 0;
          latest_channel_hour.at(i_channel)[num_fivemin-1] = 0;
  			} else {
          earliest_channel_hour.at(i_channel)[num_fivemin-1] = *(std::min_element(timestamp_mins.at(i_channel).begin(), timestamp_mins.at(i_channel).end()));
          latest_channel_hour.at(i_channel)[num_fivemin-1] = *(std::max_element(timestamp_mins.at(i_channel).begin(), timestamp_mins.at(i_channel).end()));
  				times_channel_hour.at(i_channel)[num_fivemin-1] = std::accumulate(live_mins.at(i_channel).begin(),live_mins.at(i_channel).end(),0.0)/live_mins.at(i_channel).size();
          //std::cout <<"times channel hour: "<<times_channel_hour.at(i_channel)[num_fivemin-1]<<", live mins size: "<<live_mins.at(i_channel).size()<<std::endl;
  				rms_channel_hour.at(i_channel)[num_fivemin-1] = compute_variance(times_channel_hour.at(i_channel)[num_fivemin-1],live_mins.at(i_channel));
  				//frequency_channel_hour.at(i_channel)[num_fivemin-1] = live_mins.at(i_channel).size()/duration_fivemin;
          if (t_file_end > current_stamp-5*60*1000.) denominator_duration = 5.-(current_stamp - t_file_end)/(60*1000.);
          else denominator_duration = duration_fivemin;
          frequency_channel_hour.at(i_channel)[num_fivemin-1] = live_mins.at(i_channel).size()/denominator_duration;
          n_channel_hour.at(i_channel)[num_fivemin-1] = live_mins.at(i_channel).size();
          //n_channel_overall_day.at(i_channel)[num_overall_fivemin-1] = live_mins.at(i_channel).size();
  				live_mins.at(i_channel).clear();
          timestamp_mins.at(i_channel).clear();
  			}
      //}//close draw average
      }

      //---------------------------------------------------
      //----------------Update 6 hour plots----------------
      //---------------------------------------------------

      //if (draw_average){
			if (j_fivemin%6 ==0 && !initial){
        if (verbosity > 3 && i_channel == 0) std::cout <<"MRDMonitorTime: Updating 6h plots!"<<std::endl;
				update_halfhour=true;
				for (int i_halfhour = 0; i_halfhour < num_halfhour-1; i_halfhour++){
					times_channel_sixhour.at(i_channel)[i_halfhour] = times_channel_sixhour.at(i_channel)[i_halfhour+1];
					rms_channel_sixhour.at(i_channel)[i_halfhour] = rms_channel_sixhour.at(i_channel)[i_halfhour+1];
					frequency_channel_sixhour.at(i_channel)[i_halfhour] = frequency_channel_sixhour.at(i_channel)[i_halfhour+1];
          n_channel_sixhour.at(i_channel)[i_halfhour] = n_channel_sixhour.at(i_channel)[i_halfhour+1];
          earliest_channel_sixhour.at(i_channel)[i_halfhour] = earliest_channel_sixhour.at(i_channel)[i_halfhour+1];
          latest_channel_sixhour.at(i_channel)[i_halfhour] = latest_channel_sixhour.at(i_channel)[i_halfhour+1];
				}
				if (live_halfhour.at(i_channel).empty()){
				times_channel_sixhour.at(i_channel)[num_halfhour-1] = 0;
				rms_channel_sixhour.at(i_channel)[num_halfhour-1] = 0;
				frequency_channel_sixhour.at(i_channel)[num_halfhour-1] = 0;
        n_channel_sixhour.at(i_channel)[num_halfhour-1] = 0;
        earliest_channel_sixhour.at(i_channel)[num_halfhour-1] = 0;
        latest_channel_sixhour.at(i_channel)[num_halfhour-1] = 0;
				} else {
        earliest_channel_sixhour.at(i_channel)[num_halfhour-1] = *(std::min_element(timestamp_halfhour.at(i_channel).begin(), timestamp_halfhour.at(i_channel).end()));
        latest_channel_sixhour.at(i_channel)[num_halfhour-1] = *(std::max_element(timestamp_halfhour.at(i_channel).begin(), timestamp_halfhour.at(i_channel).end()));
				times_channel_sixhour.at(i_channel)[num_halfhour-1] = std::accumulate(live_halfhour.at(i_channel).begin(),live_halfhour.at(i_channel).end(),0.0)/live_halfhour.at(i_channel).size();
				rms_channel_sixhour.at(i_channel)[num_halfhour-1] = compute_variance(times_channel_sixhour.at(i_channel)[num_halfhour-1],live_halfhour.at(i_channel));
				//frequency_channel_sixhour.at(i_channel)[num_halfhour-1] = live_halfhour.at(i_channel).size()/duration_halfhour;
        denominator_duration = 30.-(current_stamp - t_file_end)/(60*1000.);
        frequency_channel_sixhour.at(i_channel)[num_halfhour-1] = live_halfhour.at(i_channel).size()/denominator_duration;
        n_channel_sixhour.at(i_channel)[num_halfhour-1] = live_halfhour.at(i_channel).size();
        live_halfhour.at(i_channel).clear();
        timestamp_halfhour.at(i_channel).clear();
				}
			}

      //----------------------------------------------------
      //----------------Update 24 hour plots----------------
      //----------------------------------------------------

			if (j_fivemin%12 == 0 && !initial){
        	if (verbosity > 5 && i_channel == 0) std::cout <<"MRDMonitorTime: Updating 24h plots!"<<std::endl;
				  update_hour=true;
				  for (int i_hour = 0;i_hour<num_hour-1;i_hour++){
					times_channel_day.at(i_channel)[i_hour] = times_channel_day.at(i_channel)[i_hour+1];
					rms_channel_day.at(i_channel)[i_hour] = rms_channel_day.at(i_channel)[i_hour+1];
					frequency_channel_day.at(i_channel)[i_hour] = frequency_channel_day.at(i_channel)[i_hour+1];
          n_channel_day.at(i_channel)[i_hour] = n_channel_day.at(i_channel)[i_hour+1];
          earliest_channel_day.at(i_channel)[i_hour] = earliest_channel_day.at(i_channel)[i_hour+1];
          latest_channel_day.at(i_channel)[i_hour] = latest_channel_day.at(i_channel)[i_hour+1];
				}
				if (live_hour.at(i_channel).empty()){
				    times_channel_day.at(i_channel)[num_hour-1] = 0;
				    rms_channel_day.at(i_channel)[num_hour-1] = 0;
				    frequency_channel_day.at(i_channel)[num_hour-1] = 0;
            n_channel_day.at(i_channel)[num_hour-1] = 0;
            earliest_channel_day.at(i_channel)[num_hour-1] = 0;
            latest_channel_day.at(i_channel)[num_hour-1] = 0;
				}else {
				earliest_channel_day.at(i_channel)[num_hour-1] = *(std::min_element(timestamp_hour.at(i_channel).begin(), timestamp_hour.at(i_channel).end()));
        latest_channel_day.at(i_channel)[num_hour-1] = *(std::max_element(timestamp_hour.at(i_channel).begin(), timestamp_hour.at(i_channel).end()));
        times_channel_day.at(i_channel)[num_hour-1] = std::accumulate(live_hour.at(i_channel).begin(),live_hour.at(i_channel).end(),0.0)/live_hour.at(i_channel).size();
				rms_channel_day.at(i_channel)[num_hour-1] = compute_variance(times_channel_day.at(i_channel)[num_hour-1],live_hour.at(i_channel));
        denominator_duration = 60.-(current_stamp - t_file_end)/(60*1000.);
        frequency_channel_day.at(i_channel)[num_hour-1] = live_hour.at(i_channel).size()/denominator_duration;
        n_channel_day.at(i_channel)[num_hour-1] = live_hour.at(i_channel).size();
				live_hour.at(i_channel).clear();
        timestamp_hour.at(i_channel).clear();
				}
			}
      //}//close draw average
		}

    //----------------------------------------------------------------------
    //----------Initialise counting vectors for live vectors----------------
    //----------------------------------------------------------------------

    std::array<long, num_fivemin> n_fivemin = {0};
    std::array<long, num_halfhour> n_halfhour = {0};
    std::array<long, num_hour> n_hour = {0};
    for (int i_channel=0; i_channel < num_channels*num_active_slots;i_channel++){
      n_channel_hour_file.push_back(n_fivemin);
      n_channel_sixhour_file.push_back(n_halfhour);
      n_channel_day_file.push_back(n_hour);
    }

    //---------------------------------------------------
    //----------Removing old live entries----------------
    //---------------------------------------------------
    if (draw_scatter){
    if (verbosity > 3) std::cout <<"MRDMonitorTime: Cleanup old live vectors..."<<std::endl;
    for (int i_channel=0; i_channel < num_channels*num_active_slots; i_channel++){
      for (int i_entry=0; i_entry < times_channel_overall_hour.at(i_channel).size(); i_entry++){
        if (current_stamp - stamp_channel_overall_hour.at(i_channel).at(i_entry) > 60*60*1000.){                //delete entries that are older than 60 minutes from vector
          if (verbosity > 3) std::cout <<"MRDMonitorTime: Cleaning up: Time diff: "<<current_stamp - stamp_channel_overall_hour.at(i_channel).at(i_entry)<<" > 1 HOUR ... "<<std::endl;
          times_channel_overall_hour.at(i_channel).erase(times_channel_overall_hour.at(i_channel).begin()+i_entry);
          stamp_channel_overall_hour.at(i_channel).erase(stamp_channel_overall_hour.at(i_channel).begin()+i_entry);
        }
      }
    }
    for (int i_channel=0; i_channel < num_channels*num_active_slots; i_channel++){
      for (int i_entry=0; i_entry < times_channel_overall_sixhour.at(i_channel).size(); i_entry++){
        if (current_stamp - stamp_channel_overall_sixhour.at(i_channel).at(i_entry) > 6*60*60*1000.){                //delete entries that are older than 60 minutes from vector
          if (verbosity > 3) std::cout <<"MRDMonitorTime: Cleaning up: Time diff: "<<current_stamp - stamp_channel_overall_sixhour.at(i_channel).at(i_entry)<<" > 6 HOURS ... "<<std::endl;
          times_channel_overall_sixhour.at(i_channel).erase(times_channel_overall_sixhour.at(i_channel).begin()+i_entry);
          stamp_channel_overall_sixhour.at(i_channel).erase(stamp_channel_overall_sixhour.at(i_channel).begin()+i_entry);
        }
      }
    }
    for (int i_channel=0; i_channel < num_channels*num_active_slots; i_channel++){
      for (int i_entry=0; i_entry < times_channel_overall_day.at(i_channel).size(); i_entry++){
        if (current_stamp - stamp_channel_overall_day.at(i_channel).at(i_entry) > 24*60*60*1000.){                //delete entries that are older than 60 minutes from vector
          if (verbosity > 3) std::cout <<"MRDMonitorTime: Cleaning up: Time diff: "<<current_stamp - stamp_channel_overall_day.at(i_channel).at(i_entry)<<" > 1 DAY .. "<<std::endl;
          times_channel_overall_day.at(i_channel).erase(times_channel_overall_day.at(i_channel).begin()+i_entry);
          stamp_channel_overall_day.at(i_channel).erase(stamp_channel_overall_day.at(i_channel).begin()+i_entry);
        }
      }
    }
    }//close draw scatter

    //---------------------------------------------------
    //--------------Update live vectors------------------
    //---------------------------------------------------

    if (verbosity > 3) std::cout <<"MRDMonitorTime: Updating live vectors..."<<std::endl;
    //checking the live file vector for any other entries that would need to be added
    for (int i_channel = 0; i_channel < num_channels*num_active_slots;i_channel++){
      bool has_entries[num_fivemin];
      bool has_entries_sixhour[num_halfhour];
      bool has_entries_day[num_hour];

      for (int i_mins=0;i_mins<num_hour;i_mins++) {
        if (i_mins< num_fivemin){
          has_entries[i_mins] = true;
          has_entries_sixhour[i_mins] = true;
        }
        has_entries_day[i_mins] = true;
      }

      for (int i_live = 0; i_live < live_file.at(i_channel).size(); i_live++){
        if (draw_scatter){
        if (timediff_file.at(i_channel).at(i_live) < 24*60*60*1000.){
          times_channel_overall_day.at(i_channel).push_back(live_file.at(i_channel).at(i_live));
          stamp_channel_overall_day.at(i_channel).push_back(timestamp_file.at(i_channel).at(i_live));
          if (timediff_file.at(i_channel).at(i_live) < 6*60*60*1000.){
            times_channel_overall_sixhour.at(i_channel).push_back(live_file.at(i_channel).at(i_live));
            stamp_channel_overall_sixhour.at(i_channel).push_back(timestamp_file.at(i_channel).at(i_live));
            if (timediff_file.at(i_channel).at(i_live) < 60*60*1000.){
              times_channel_overall_hour.at(i_channel).push_back(live_file.at(i_channel).at(i_live));
              stamp_channel_overall_hour.at(i_channel).push_back(timestamp_file.at(i_channel).at(i_live));
            }
          } 
        }
        }//close draw scatter

        //if (draw_average){
        if (update_mins){
        //std::cout <<"timediff: "<<timediff_file.at(i_channel).at(i_live)<<" > 5 min? : "<<(timediff_file.at(i_channel).at(i_live) > 5*60*1000.)<<std::endl;
        if (timediff_file.at(i_channel).at(i_live) > 5*60*1000.){           //timediff bigger than 5 mins?  --> fill in 1h graph
          int bin = timediff_file.at(i_channel).at(i_live)/int((5*60*1000));
          if (i_live == 0 && verbosity > 3) std::cout <<"MonitorMRDTime: 1 hour: Time diff: "<<timediff_file.at(i_channel).at(i_live)<<", h: "<<int(timediff_file.at(i_channel).at(i_live)/(3600*10000))%24<<", min: "<<int(timediff_file.at(i_channel).at(i_live)/(60*10000))%60<<", bin: "<<bin<<", index: "<<num_fivemin-1-bin<<std::endl;
          if (bin < 288) n_channel_overall_day.at(i_channel)[num_overall_fivemin-1-bin]++;
          if (bin < 12){
            //1 hour - has_entries
            if (n_channel_hour_file.at(i_channel)[num_fivemin-1-bin]==0 && n_channel_hour.at(i_channel)[num_fivemin-1-bin]==0) has_entries[num_fivemin-1-bin] = false;
            if (earliest_channel_hour.at(i_channel)[num_fivemin-1-bin] == 0 || (timestamp_file.at(i_channel).at(i_live) < earliest_channel_hour.at(i_channel)[num_fivemin-1-bin])) earliest_channel_hour.at(i_channel)[num_fivemin-1-bin] = timestamp_file.at(i_channel).at(i_live);
            if (latest_channel_hour.at(i_channel)[num_fivemin-1-bin] == 0 || (timestamp_file.at(i_channel).at(i_live) > latest_channel_hour.at(i_channel)[num_fivemin-1-bin])) latest_channel_hour.at(i_channel)[num_fivemin-1-bin] = timestamp_file.at(i_channel).at(i_live);
            //1 hour - times_channel
            times_channel_hour.at(i_channel)[num_fivemin-1-bin] = (times_channel_hour.at(i_channel)[num_fivemin-1-bin]*n_channel_hour.at(i_channel)[num_fivemin-1-bin]+live_file.at(i_channel).at(i_live))/(n_channel_hour.at(i_channel)[num_fivemin-1-bin]+1);
            //1 hour - frequency_channel
            if (t_file_end < (current_stamp - (bin)*5*60*1000)) denominator_duration = (5. - (current_stamp - (bin)*5*60*1000 - t_file_end)/(60*1000.));
            else denominator_duration = duration_fivemin; 
            if (t_file_start > (current_stamp - (bin+1)*5*60*1000)) denominator_start = (t_file_start - (current_stamp - (bin+1)*5*60*1000))/(60*1000.);
            else denominator_start = 0.;
            if (has_entries[num_fivemin-1-bin]==false) denominator_duration-=denominator_start;     //if no previous file in that bin, only take part of the data
            if (denominator_duration < 0) {
              boost::posix_time::ptime temp_start = *Epoch + boost::posix_time::time_duration(int((t_file_start+offset_date)/1000./60./60.),int((t_file_start+offset_date)/1000./60.)%60,int((t_file_start+offset_date)/1000.)%60,(t_file_start+offset_date)%1000);
              boost::posix_time::ptime temp_end = *Epoch + boost::posix_time::time_duration(int((t_file_end+offset_date)/1000./60./60.),int((t_file_end+offset_date)/1000./60.)%60,int((t_file_end+offset_date)/1000.)%60,(t_file_end+offset_date)%1000);
              long bin_start = current_stamp - (bin+1)*5*60*1000;
              long bin_end = current_stamp - bin*5*60*1000;
              long timestamp_temp = timestamp_file.at(i_channel).at(i_live);
              boost::posix_time::ptime temp_bin_start = *Epoch + boost::posix_time::time_duration(int((bin_start+offset_date)/1000./60./60.),int((bin_start+offset_date)/1000./60.)%60,int((bin_start+offset_date)/1000.)%60,(bin_start+offset_date)%1000);
              boost::posix_time::ptime temp_bin_end = *Epoch + boost::posix_time::time_duration(int((bin_end+offset_date)/1000./60./60.),int((bin_end+offset_date)/1000./60.)%60,int((bin_end+offset_date)/1000.)%60,(bin_end+offset_date)%1000);  
              boost::posix_time::ptime temp_current = *Epoch + boost::posix_time::time_duration(int((timestamp_temp+offset_date)/1000./60./60.),int((timestamp_temp+offset_date)/1000./60.)%60,int((timestamp_temp+offset_date)/1000.)%60,(timestamp_temp+offset_date)%1000);  
              if (verbosity > 1) std::cout <<"WARNING (1 HOUR): Denominator duration < 0! File "<<temp_start.time_of_day()<<"-"<<temp_end.time_of_day()<<", Bin: "<<temp_bin_start.time_of_day()<<"-"<<temp_bin_end.time_of_day()<<", current: "<<temp_current.time_of_day()<<std::endl;
              //std::cout <<"t_file_start: "<<t_file_start<<", t_file_end: "<<t_file_end<<", bin start: "<<current_stamp-(bin+1)*5*60*1000<<", bin end: "<<current_stamp-(bin)*5*60*1000<<std::endl;
              //std::cout <<"HOUR: denominator duration: "<<denominator_duration<<", denominator start: "<<denominator_start<<std::endl;
            }
            if (n_channel_hour_file.at(i_channel)[num_fivemin-1-bin] == 0) frequency_channel_hour.at(i_channel)[num_fivemin-1-bin] = frequency_channel_hour.at(i_channel)[num_fivemin-1-bin]*(denominator_start/(denominator_duration))+1./denominator_duration;
            else frequency_channel_hour.at(i_channel)[num_fivemin-1-bin] = frequency_channel_hour.at(i_channel)[num_fivemin-1-bin]+1./denominator_duration;
            //1 hour - n_channel
            n_channel_hour.at(i_channel)[num_fivemin-1-bin]+=1;
            n_channel_hour_file.at(i_channel)[num_fivemin-1-bin]++;
          //  std::cout <<"HOUR: frequency: "<<frequency_channel_hour.at(i_channel)[num_fivemin-1-bin]<<std::endl;
          }
        }
          if (timediff_file.at(i_channel).at(i_live) > ((j_fivemin-1)%6+1)*5*60*1000.){         //timediff bigger than 30 mins? --> also fill in 6h graph 	  
              int bin = (timediff_file.at(i_channel).at(i_live)-((j_fivemin-1)%6+1)*5*60*1000.)/int(30*60*1000);
              if (j_fivemin%6==0) bin++;
              if (bin < 12){
                  //6 hours - has_entries
                  if (n_channel_sixhour_file.at(i_channel)[num_halfhour-1-bin]==0 && n_channel_sixhour.at(i_channel)[num_halfhour-1-bin]==0) has_entries_sixhour[num_halfhour-1-bin] = false;
                  if (earliest_channel_sixhour.at(i_channel)[num_halfhour-1-bin] == 0 || (timestamp_file.at(i_channel).at(i_live) < earliest_channel_sixhour.at(i_channel)[num_halfhour-1-bin])) earliest_channel_sixhour.at(i_channel)[num_halfhour-1-bin] = timestamp_file.at(i_channel).at(i_live);
                  if (latest_channel_sixhour.at(i_channel)[num_halfhour-1-bin] == 0 || (timestamp_file.at(i_channel).at(i_live) > latest_channel_sixhour.at(i_channel)[num_halfhour-1-bin])) latest_channel_sixhour.at(i_channel)[num_halfhour-1-bin] = timestamp_file.at(i_channel).at(i_live);
                  //6 hours - times_channel
                  times_channel_sixhour.at(i_channel)[num_halfhour-1-bin] = (times_channel_sixhour.at(i_channel)[num_halfhour-1-bin]*n_channel_sixhour.at(i_channel)[num_halfhour-1-bin]+live_file.at(i_channel).at(i_live))/(n_channel_sixhour.at(i_channel)[num_halfhour-1-bin]+1);
                  //6 hours - frequency_channel
                  int bin_time = (j_fivemin%6==0)? bin-1 : bin;
                  if (t_file_end < (current_stamp - ((j_fivemin-1)%6+1)*5*60*1000. - (bin_time)*30*60*1000)) denominator_duration = (30. -(current_stamp - ((j_fivemin-1)%6+1)*5*60*1000. - (bin_time)*30*60*1000 - t_file_end)/(60*1000.));
                  else denominator_duration = duration_halfhour; 
                  if (t_file_start > (current_stamp - ((j_fivemin-1)%6+1)*5*60*1000. - (bin_time+1)*30*60*1000)) denominator_start = (t_file_start - (current_stamp - ((j_fivemin-1)%6+1)*5*60*1000. - (bin_time+1)*30*60*1000))/(60*1000.);
                  else denominator_start = 0.;
                  if (has_entries_sixhour[num_halfhour-1-bin]==false) denominator_duration-=denominator_start;
                  if (denominator_duration < 0) {
                    //std::cout <<"t_file_start: "<<t_file_start<<", t_file_end: "<<t_file_end<<", bin start: "<<current_stamp-(bin+1)*30*60*1000<<", bin end: "<<current_stamp-(bin)*30*60*1000<<std::endl;
                    //std::cout <<"6 HOURS: denominator duration: "<<denominator_duration<<", denominator start: "<<denominator_start<<std::endl;
                    boost::posix_time::ptime temp_start = *Epoch + boost::posix_time::time_duration(int((t_file_start+offset_date)/1000./60./60.),int((t_file_start+offset_date)/1000./60.)%60,int((t_file_start+offset_date)/1000.)%60,(t_file_start+offset_date)%1000);
                    boost::posix_time::ptime temp_end = *Epoch + boost::posix_time::time_duration(int((t_file_end+offset_date)/1000./60./60.),int((t_file_end+offset_date)/1000./60.)%60,int((t_file_end+offset_date)/1000.)%60,(t_file_end+offset_date)%1000);
                    long bin_start = current_stamp - ((j_fivemin-1)%6+1)*5*6*1000. - (bin_time+1)*30*60*1000;
                    long bin_end = current_stamp - ((j_fivemin-1)%6+1)*5*6*1000. -  bin_time*30*60*1000;
                    long timestamp_temp = timestamp_file.at(i_channel).at(i_live);
                    boost::posix_time::ptime temp_bin_start = *Epoch + boost::posix_time::time_duration(int((bin_start+offset_date)/1000./60./60.),int((bin_start+offset_date)/1000./60.)%60,int((bin_start+offset_date)/1000.)%60,(bin_start+offset_date)%1000);
                    boost::posix_time::ptime temp_bin_end = *Epoch + boost::posix_time::time_duration(int((bin_end+offset_date)/1000./60./60.),int((bin_end+offset_date)/1000./60.)%60,int((bin_end+offset_date)/1000.)%60,(bin_end+offset_date)%1000);  
                    boost::posix_time::ptime temp_current = *Epoch + boost::posix_time::time_duration(int((timestamp_temp+offset_date)/1000./60./60.),int((timestamp_temp+offset_date)/1000./60.)%60,int((timestamp_temp+offset_date)/1000.)%60,(timestamp_temp+offset_date)%1000);  
                    if (verbosity > 1) std::cout <<"WARNING (6 HOURS): Denominator duration < 0! File "<<temp_start.time_of_day()<<"-"<<temp_end.time_of_day()<<", Bin: "<<temp_bin_start.time_of_day()<<"-"<<temp_bin_end.time_of_day()<<", current: "<<temp_current.time_of_day()<<std::endl;
                  }
                  //6 hours - n_channel
                  if (n_channel_sixhour_file.at(i_channel)[num_halfhour-1-bin] == 0) frequency_channel_sixhour.at(i_channel)[num_halfhour-1-bin] = frequency_channel_sixhour.at(i_channel)[num_halfhour-1-bin]*(denominator_start/(denominator_duration))+1./denominator_duration;
                  else frequency_channel_sixhour.at(i_channel)[num_halfhour-1-bin] = frequency_channel_sixhour.at(i_channel)[num_halfhour-1-bin]+1./denominator_duration;
                  n_channel_sixhour.at(i_channel)[num_halfhour-1-bin]+=1;
                  n_channel_sixhour_file.at(i_channel)[num_halfhour-1-bin]++;
                 // std::cout <<"6 HOURs: frequency: "<<frequency_channel_sixhour.at(i_channel)[num_halfhour-1-bin]<<std::endl;
              }
            }
            if (timediff_file.at(i_channel).at(i_live) > (j_fivemin)*5*60*1000.){        //timediff bigger than 60 mins? --> also fill in 24h graph
              int bin = (timediff_file.at(i_channel).at(i_live)-(j_fivemin)*5*60*1000.)/int(60*60*1000.);
              if (j_fivemin%12==0) bin++;
              if (bin<24){
                  //1 day - has_entries
                  if (n_channel_day_file.at(i_channel)[num_hour-1-bin]==0 && n_channel_day.at(i_channel)[num_hour-1-bin]==0) has_entries_day[num_hour-1-bin] = false;
                  if ((earliest_channel_day.at(i_channel)[num_hour-1-bin] == 0) || (timestamp_file.at(i_channel).at(i_live) < earliest_channel_day.at(i_channel)[num_hour-1-bin])) earliest_channel_day.at(i_channel)[num_hour-1-bin] = timestamp_file.at(i_channel).at(i_live);
                  if ((latest_channel_day.at(i_channel)[num_hour-1-bin] == 0) || (timestamp_file.at(i_channel).at(i_live) > latest_channel_day.at(i_channel)[num_hour-1-bin])) latest_channel_day.at(i_channel)[num_hour-1-bin] = timestamp_file.at(i_channel).at(i_live);
                  //std::cout <<"Timestamp file: "<<timestamp_file.at(i_channel).at(i_live)<<", Timediff: "<<(timediff_file.at(i_channel).at(i_live)-(j_fivemin)*5*60*1000.);
                  //std::cout <<", > 60mins, bin nr: "<<bin<<std::endl;
                  //1 day - times_channel
                  times_channel_day.at(i_channel)[num_hour-1-bin] = (times_channel_day.at(i_channel)[num_hour-1-bin]*n_channel_day.at(i_channel)[num_hour-1-bin]+live_file.at(i_channel).at(i_live))/(n_channel_day.at(i_channel)[num_hour-1-bin]+1);
                  //1 day - frequency_channel
                  int bin_time = (j_fivemin%12==0)? bin-1 : bin;
                  if (t_file_end < (current_stamp - (j_fivemin)*5*60*1000.-(bin_time)*60*60*1000)) denominator_duration = (60. -(current_stamp - j_fivemin*5*60*1000. - (bin_time)*60*60*1000 - t_file_end)/(60*1000.));
                  else denominator_duration = duration_hour; 
                  if (t_file_start > (current_stamp - j_fivemin*5*60*1000 - (bin_time+1)*60*60*1000)) denominator_start = (t_file_start - (current_stamp - j_fivemin*5*60*1000. - (bin_time+1)*60*60*1000))/(60*1000.);
                  else denominator_start = 0.;
                  if (has_entries_day[num_hour-1-bin]==false) denominator_duration-=denominator_start;
                  if (denominator_duration < 0) {
                    //std::cout <<"t_file_start: "<<t_file_start/double(1000000)<<", t_file_end: "<<t_file_end/double(1000000)<<", bin start: "<<(current_stamp-(bin+1)*60*60*1000)/double(1000000)<<", bin end: "<<(current_stamp-(bin)*60*60*1000)/1000000.<<std::endl;
                    //std::cout <<" DAY: denominator duration: "<<denominator_duration<<", denominator start: "<<denominator_start<<std::endl;
                    boost::posix_time::ptime temp_start = *Epoch + boost::posix_time::time_duration(int((t_file_start+offset_date)/1000./60./60.),int((t_file_start+offset_date)/1000./60.)%60,int((t_file_start+offset_date)/1000.)%60,(t_file_start+offset_date)%1000);
                    boost::posix_time::ptime temp_end = *Epoch + boost::posix_time::time_duration(int((t_file_end+offset_date)/1000./60./60.),int((t_file_end+offset_date)/1000./60.)%60,int((t_file_end+offset_date)/1000.)%60,(t_file_end+offset_date)%1000);
                    long bin_start = current_stamp - j_fivemin*5*60*1000 - (bin_time+1)*60*60*1000;
                    long bin_end = current_stamp - j_fivemin*5*60*1000 - bin_time*60*60*1000;
                    long timestamp_temp = timestamp_file.at(i_channel).at(i_live);
                    boost::posix_time::ptime temp_bin_start = *Epoch + boost::posix_time::time_duration(int((bin_start+offset_date)/1000./60./60.),int((bin_start+offset_date)/1000./60.)%60,int((bin_start+offset_date)/1000.)%60,(bin_start+offset_date)%1000);
                    boost::posix_time::ptime temp_bin_end = *Epoch + boost::posix_time::time_duration(int((bin_end+offset_date)/1000./60./60.),int((bin_end+offset_date)/1000./60.)%60,int((bin_end+offset_date)/1000.)%60,(bin_end+offset_date)%1000);  
                    boost::posix_time::ptime temp_current = *Epoch + boost::posix_time::time_duration(int((timestamp_temp+offset_date)/1000./60./60.),int((timestamp_temp+offset_date)/1000./60.)%60,int((timestamp_temp+offset_date)/1000.)%60,(timestamp_temp+offset_date)%1000);  
                    if (verbosity > 1) std::cout <<"WARNING (DAY): Denominator duration < 0! File "<<temp_start.time_of_day()<<"-"<<temp_end.time_of_day()<<", Bin: "<<temp_bin_start.time_of_day()<<"-"<<temp_bin_end.time_of_day()<<", current: "<<temp_current.time_of_day()<<std::endl;
                  }
                  if (n_channel_day_file.at(i_channel)[num_hour-1-bin] == 0) frequency_channel_day.at(i_channel)[num_hour-1-bin] = frequency_channel_day.at(i_channel)[num_hour-1-bin]*(denominator_start/(denominator_duration))+1./denominator_duration;
                  else frequency_channel_day.at(i_channel)[num_hour-1-bin] = frequency_channel_day.at(i_channel)[num_hour-1-bin]+1./denominator_duration;
                  //1 day - n_channel
                  n_channel_day.at(i_channel)[num_hour-1-bin]+=1;
                  n_channel_day_file.at(i_channel)[num_hour-1-bin]++;
                 // std::cout <<"DAY: frequency: "<<frequency_channel_day.at(i_channel)[num_hour-1-bin]<<std::endl;
              }
            }
          }
          //}//close draw average
      //}

      long n_temp_hour[num_fivemin]={0};
      long n_temp_sixhour[num_halfhour]={0};
      long n_temp_day[num_hour]={0};

    //---------------------------------------------------
    //--------------Update RMS values--------------------
    //---------------------------------------------------

      if (draw_average){
      for (int i_live = 0; i_live< live_file.at(i_channel).size(); i_live++){
        if (timediff_file.at(i_channel).at(i_live) > 5*60*1000.){           //timediff bigger than 5 mins?  --> fill in 1h graph
          int bin = timediff_file.at(i_channel).at(i_live)/int((5*60*1000));
          if (bin < 12){
            n_temp_hour[num_fivemin-1-bin]++;
            if (n_channel_hour.at(i_channel)[num_fivemin-1-bin] == 1){
              rms_channel_hour.at(i_channel)[num_fivemin-1-bin] = 0;
            }
            else if(n_channel_hour.at(i_channel)[num_fivemin-1-bin] == 2){
              rms_channel_hour.at(i_channel)[num_fivemin-1-bin] = sqrt(pow(live_file.at(i_channel).at(i_live)-times_channel_hour.at(i_channel)[num_fivemin-1-bin],2));
            } else {
              rms_channel_hour.at(i_channel)[num_fivemin-1-bin]=sqrt((pow(rms_channel_hour.at(i_channel)[num_fivemin-1-bin],2)*(n_channel_hour.at(i_channel)[num_fivemin-1-bin]-n_channel_hour_file.at(i_channel)[num_fivemin-1-bin]+n_temp_hour[num_fivemin-1-bin])+pow(live_file.at(i_channel).at(i_live)-times_channel_hour.at(i_channel)[num_fivemin-1-bin],2))/double(n_channel_hour.at(i_channel)[num_fivemin-1-bin]-n_channel_hour_file.at(i_channel)[num_fivemin-1-bin]+n_temp_hour[num_fivemin-1-bin]+1));
              }
            if (verbosity > 5) std::cout <<"MRDMonitorTime: Calculated RMS for 1 hour, bin: "<<bin<<": "<<rms_channel_hour.at(i_channel)[num_fivemin-1-bin]<<std::endl;
          }
        }
        if (timediff_file.at(i_channel).at(i_live) > ((j_fivemin-1)%6+1)*5*60*1000.){         //timediff bigger than 30 mins? --> also fill in 6h graph
              int bin = (timediff_file.at(i_channel).at(i_live)-((j_fivemin-1)%6+1)*5*60*1000.)/int(30*60*1000);
              if (j_fivemin%6==0) bin++;
              if (bin < 12){
                n_temp_sixhour[num_halfhour-1-bin]++;
                if (n_channel_sixhour.at(i_channel)[num_halfhour-1-bin] == 1){
                  rms_channel_sixhour.at(i_channel)[num_halfhour-1-bin] = 0;
                }
                else if (n_channel_sixhour.at(i_channel)[num_halfhour-1-bin] == 2){
                  rms_channel_sixhour.at(i_channel)[num_halfhour-1-bin] = sqrt(pow(live_file.at(i_channel).at(i_live)-times_channel_sixhour.at(i_channel)[num_halfhour-1-bin],2));
              } else {
                rms_channel_sixhour.at(i_channel)[num_halfhour-1-bin]=sqrt((pow(rms_channel_sixhour.at(i_channel)[num_halfhour-1-bin],2)*(n_channel_sixhour.at(i_channel)[num_halfhour-1-bin]-n_channel_sixhour_file.at(i_channel)[num_halfhour-1-bin]+n_temp_sixhour[num_halfhour-1-bin])+pow(live_file.at(i_channel).at(i_live)-times_channel_sixhour.at(i_channel)[num_halfhour-1-bin],2))/double(n_channel_sixhour.at(i_channel)[num_halfhour-1-bin]-n_channel_sixhour_file.at(i_channel)[num_halfhour-1-bin]+n_temp_sixhour[num_halfhour-1-bin]+1)); 
              }    
            if (verbosity > 5) std::cout <<"MRDMonitorTime: Calculated RMS for 6 hours, bin: "<<bin<<": "<<rms_channel_sixhour.at(i_channel)[num_halfhour-1-bin]<<std::endl;
          }
        }   
        if (timediff_file.at(i_channel).at(i_live) > (j_fivemin)*5*60*1000.){        //timediff bigger than 60 mins? --> also fill in 24h graph
              int bin = (timediff_file.at(i_channel).at(i_live)-(j_fivemin)*5*60*1000.)/int(60*60*1000.);
              if (j_fivemin%12==0) bin++;
              if (bin<24){
                n_temp_day[num_hour-1-bin]++;
                if (n_channel_day.at(i_channel)[num_hour-1-bin] == 1){
                  rms_channel_day.at(i_channel)[num_hour-1-bin] = 0;
                }
                else if(n_channel_day.at(i_channel)[num_hour-1-bin] == 2){
                  rms_channel_day.at(i_channel)[num_hour-1-bin] = sqrt(pow(live_file.at(i_channel).at(i_live)-times_channel_day.at(i_channel)[num_hour-1-bin],2));
                } else {
                  if (verbosity > 5) {
                    std::cout <<"MRDMonitorTime: Live TDC: "<<live_file.at(i_channel).at(i_live)<<", mean TDC: "<<times_channel_day.at(i_channel)[num_hour-1-bin]<<std::endl;
                    std::cout <<"MRDMonitorTime: RMS: Adding: "<<pow(live_file.at(i_channel).at(i_live)-times_channel_day.at(i_channel)[num_hour-1-bin],2)<<", old RMS: "<<rms_channel_day.at(i_channel)[num_hour-1-bin]<<", dividing by"<<n_channel_day.at(i_channel)[num_hour-1-bin]-n_channel_day_file.at(i_channel)[num_hour-1-bin]+n_temp_day[num_hour-1-bin]+1<<std::endl;
                  }
                rms_channel_day.at(i_channel)[num_hour-1-bin]=sqrt((pow(rms_channel_day.at(i_channel)[num_hour-1-bin],2)*(n_channel_day.at(i_channel)[num_hour-1-bin]-n_channel_day_file.at(i_channel)[num_hour-1-bin]+n_temp_day[num_hour-1-bin])+pow(live_file.at(i_channel).at(i_live)-times_channel_day.at(i_channel)[num_hour-1-bin],2))/double(n_channel_day.at(i_channel)[num_hour-1-bin]-n_channel_day_file.at(i_channel)[num_hour-1-bin]+n_temp_day[num_hour-1-bin]+1));   
              }
            if (verbosity > 5) std::cout << "MRDMonitorTime: Calculated rms for day, bin: "<<bin<<": "<<rms_channel_day.at(i_channel)[num_hour-1-bin]<<std::endl;
          }
        }
      }
      }//close draw average
    }
  }

    //---------------------------------------------------
    //--------------Fill crate vectors-------------------
    //---------------------------------------------------

    if (draw_average){
    if (verbosity > 3) std::cout <<"MRDMonitorTime: Updating slot vectors..."<<std::endl;
    for (int i_slot = 0; i_slot< num_active_slots; i_slot++){
      if (update_mins){
        for (int i_mins = 0;i_mins<num_fivemin;i_mins++){
          times_slot_hour.at(i_slot)[i_mins] = accumulate_array12(times_channel_hour,i_slot*num_channels,(i_slot+1)*num_channels-1,i_mins);
          rms_slot_hour.at(i_slot)[i_mins] = accumulate_array12(rms_channel_hour,i_slot*num_channels,(i_slot+1)*num_channels-1,i_mins);
          frequency_slot_hour.at(i_slot)[i_mins] = accumulate_array12(frequency_channel_hour,i_slot*num_channels,(i_slot+1)*num_channels-1,i_mins);
          n_slot_hour.at(i_slot)[i_mins] = accumulate_longarray12(n_channel_hour,i_slot*num_channels,(i_slot+1)*num_channels-1,i_mins);
        }
      }
      if (j_fivemin%6 == 0 && !initial){
        update_halfhour = true;
        for (int i_halfhour=0;i_halfhour<num_halfhour;i_halfhour++){
          times_slot_sixhour.at(i_slot)[i_halfhour] = accumulate_array12(times_channel_sixhour,i_slot*num_channels,(i_slot+1)*num_channels-1,i_halfhour);
          rms_slot_sixhour.at(i_slot)[i_halfhour] = accumulate_array12(rms_channel_sixhour,i_slot*num_channels,(i_slot+1)*num_channels-1,i_halfhour);
          frequency_slot_sixhour.at(i_slot)[i_halfhour] = accumulate_array12(frequency_channel_sixhour,i_slot*num_channels,(i_slot+1)*num_channels-1,i_halfhour);
          n_slot_sixhour.at(i_slot)[i_halfhour] = accumulate_longarray12(n_channel_sixhour,i_slot*num_channels,(i_slot+1)*num_channels-1,i_halfhour);
        }
      }
      if (j_fivemin%12 == 0 && !initial){
        update_hour = true;
        for (int i_hour=0;i_hour<num_hour;i_hour++){
          times_slot_day.at(i_slot)[i_hour] = accumulate_array24(times_channel_day,i_slot*num_channels,(i_slot+1)*num_channels-1,i_hour);
          rms_slot_day.at(i_slot)[i_hour] = accumulate_array24(rms_channel_day,i_slot*num_channels,(i_slot+1)*num_channels-1,i_hour);
          frequency_slot_day.at(i_slot)[i_hour] = accumulate_array24(frequency_channel_day,i_slot*num_channels,(i_slot+1)*num_channels-1,i_hour);
          n_slot_day.at(i_slot)[i_hour] = accumulate_longarray24(n_channel_day,i_slot*num_channels,(i_slot+1)*num_channels-1,i_hour);
        }
      }
    }

    //---------------------------------------------------
    //--------------Fill crate vectors-------------------
    //---------------------------------------------------

    if (verbosity > 3) std::cout <<"MRDMonitorTime: Updating crate vectors..."<<std::endl;
    for (int i_crate = 0; i_crate< num_crates; i_crate++){
      int slot1 = (i_crate == 0)? num_active_slots_cr1 : num_active_slots;
      if (update_mins){
        for (int i_mins = 0;i_mins<num_fivemin;i_mins++){
          times_crate_hour.at(i_crate)[i_mins] = accumulate_array12(times_channel_hour,i_crate*num_active_slots_cr1*num_channels,slot1*num_channels-1,i_mins);
          rms_crate_hour.at(i_crate)[i_mins] = accumulate_array12(rms_channel_hour,i_crate*num_active_slots_cr1*num_channels,slot1*num_channels-1,i_mins);
          frequency_crate_hour.at(i_crate)[i_mins] = accumulate_array12(frequency_channel_hour,i_crate*num_active_slots_cr1*num_channels,slot1*num_channels-1,i_mins);
          n_crate_hour.at(i_crate)[i_mins] = accumulate_longarray12(n_channel_hour,i_crate*num_active_slots_cr1*num_channels,slot1*num_channels-1,i_mins);
        }
      }
      if (j_fivemin%6 == 0 && !initial){
        update_halfhour=true;
        for (int i_halfhour = 0;i_halfhour<num_halfhour;i_halfhour++){
          times_crate_sixhour.at(i_crate)[i_halfhour] = accumulate_array12(times_channel_sixhour,i_crate*num_active_slots_cr1*num_channels,slot1*num_channels-1,i_halfhour);
          rms_crate_sixhour.at(i_crate)[i_halfhour] = accumulate_array12(rms_channel_sixhour,i_crate*num_active_slots_cr1*num_channels,slot1*num_channels-1,i_halfhour);
          frequency_crate_sixhour.at(i_crate)[i_halfhour] = accumulate_array12(frequency_channel_sixhour,i_crate*num_active_slots_cr1*num_channels,slot1*num_channels-1,i_halfhour);
          n_crate_sixhour.at(i_crate)[i_halfhour] = accumulate_longarray12(n_channel_sixhour,i_crate*num_active_slots_cr1*num_channels,slot1*num_channels-1,i_halfhour);
        }
      }
      if (j_fivemin%12 == 0 && !initial){
        update_hour = true;
        for (int i_hour = 0;i_hour<num_hour;i_hour++){
          times_crate_day.at(i_crate)[i_hour] = accumulate_array24(times_channel_day,i_crate*num_active_slots_cr1*num_channels,slot1*num_channels-1,i_hour);
          rms_crate_day.at(i_crate)[i_hour] = accumulate_array24(rms_channel_day,i_crate*num_active_slots_cr1*num_channels,slot1*num_channels-1,i_hour);
          frequency_crate_day.at(i_crate)[i_hour] = accumulate_array24(frequency_channel_day,i_crate*num_active_slots_cr1*num_channels,slot1*num_channels-1,i_hour);
          n_crate_day.at(i_crate)[i_hour] = accumulate_longarray24(n_channel_day,i_crate*num_active_slots_cr1*num_channels,slot1*num_channels-1,i_hour);
        }
      }
    }
    }

  //---------------------------------------------------
  //-----------Clearing live file vectors--------------
  //---------------------------------------------------

  for (int i_channel = 0; i_channel < num_channels*(num_active_slots); i_channel++){
    live_file.at(i_channel).clear();
    timestamp_file.at(i_channel).clear();
    timediff_file.at(i_channel).clear();
    n_live_file.at(i_channel).clear();
  }
  n_channel_hour_file.clear();
  n_channel_sixhour_file.clear();
  n_channel_day_file.clear();

  
  //---------------------------------------------------
  //--Updating the axis label vectors for all TGraphs--
  //---------------------------------------------------

  //boost::posix_time::ptime current(boost::posix_time::second_clock::local_time());
  struct tm label_offset_hour = boost::posix_time::to_tm(current-boost::posix_time::time_duration(1,0,0,0));
  struct tm label_offset_sixhour = boost::posix_time::to_tm(current-boost::posix_time::time_duration(6,0,0,0));
  struct tm label_offset_day = boost::posix_time::to_tm(current-boost::posix_time::time_duration(24,0,0,0));
  timeoffset_hour = TDatime(1900+label_offset_hour.tm_year,label_offset_hour.tm_mon +1,label_offset_hour.tm_mday,label_offset_hour.tm_hour,label_offset_hour.tm_min,label_offset_hour.tm_sec);
  timeoffset_sixhour = TDatime(1900+label_offset_sixhour.tm_year,label_offset_sixhour.tm_mon +1,label_offset_sixhour.tm_mday,label_offset_sixhour.tm_hour,label_offset_sixhour.tm_min,label_offset_sixhour.tm_sec);
  timeoffset_day = TDatime(1900+label_offset_day.tm_year,label_offset_day.tm_mon +1,label_offset_day.tm_mday,label_offset_day.tm_hour,label_offset_day.tm_min,label_offset_day.tm_sec);
  //std::cout <<"time offset date: "<<1900+label_offset_hour.tm_year<<"-"<<label_offset_hour.tm_mon +1<<"-"<<label_offset_hour.tm_mday<<","<<label_offset_hour.tm_hour<<":"<<label_offset_hour.tm_min<<":"<<label_offset_hour.tm_sec<<std::endl;
  if (update_mins){
    for (int i_label = 0; i_label < 12; i_label++){
      boost::posix_time::time_duration offset_label(0,i_label*5,0,0);   //multiples of 5 mins
      boost::posix_time::ptime labeltime = current - offset_label;
      struct tm label_tm = boost::posix_time::to_tm(labeltime);
      //std::cout <<1900+label_tm.tm_year<<", "<<label_tm.tm_mon +1<<", "<<label_tm.tm_mday<<", "<<label_tm.tm_hour<<", "<<label_tm.tm_min<<", "<<label_tm.tm_sec<<std::endl;
      TDatime label_date(1900+label_tm.tm_year,label_tm.tm_mon +1,label_tm.tm_mday,label_tm.tm_hour,label_tm.tm_min,label_tm.tm_sec);
      if (verbosity > 4) std::cout <<"Axis labels: Label # "<<i_label<<", label_date: "<<boost::posix_time::to_simple_string(labeltime)<<std::endl;
      label_fivemin[11-i_label] = label_date;
    }
  }
  if (update_halfhour){
    for (int i_label = 0; i_label < 12; i_label++){
      boost::posix_time::time_duration offset_label(0,i_label*30,0,0);   //multiples of 30 mins
      boost::posix_time::ptime labeltime = current - offset_label;
      struct tm label_tm = boost::posix_time::to_tm(labeltime);
      TDatime label_date(1900+label_tm.tm_year,label_tm.tm_mon +1,label_tm.tm_mday,label_tm.tm_hour,label_tm.tm_min,label_tm.tm_sec);
      label_halfhour[11-i_label] = label_date;       //labels that are to be displayed on the x-axis (times)
    }
  }
  if (update_hour){
    for (int i_label = 0; i_label < 24; i_label++){
      boost::posix_time::time_duration offset_label(i_label,0,0,0);   //multiples of 1 hours
      boost::posix_time::ptime labeltime = current - offset_label;
      struct tm label_tm = boost::posix_time::to_tm(labeltime);
      TDatime label_date(1900+label_tm.tm_year,label_tm.tm_mon +1,label_tm.tm_mday,label_tm.tm_hour,label_tm.tm_min,label_tm.tm_sec);
      label_hour[23-i_label] = label_date;       //labels that are to be displayed on the x-axis (times)
    }
  }
	
  //---------------------------------------------------
  //-----------Plot newly created vectors--------------
  //---------------------------------------------------

	MonitorMRDTime::MRDTimePlots();

}

void MonitorMRDTime::FillEvents(){

  //-------------------------------------------------------
  //----------------FillEvents ----------------------------
  //-------------------------------------------------------

  for (int i_channel = 0; i_channel < num_active_slots*num_channels; i_channel++){
    n_new_hour.push_back(0);
    n_new_sixhour.push_back(0);
    n_new_day.push_back(0);
    n_new_file.push_back(0);
  }


  if (verbosity > 1) std::cout <<"MonitorMRDTime: FillEvents..."<<std::endl;
  t_file_end = 0;
  int total_number_entries;  
  MRDdata->Header->Get("TotalEntries",total_number_entries);
  if (verbosity > 1) std::cout <<"MonitorMRDTime: MRDdata number of events: "<<total_number_entries<<std::endl;
  int count=0;
  if (verbosity > 1) std::cout <<"Data File: time = ";

	for (int i_event = 0; i_event <  total_number_entries; i_event++){

	  MRDdata->GetEntry(i_event);
    MRDdata->Get("Data",MRDout);

    //get current time + date
    boost::posix_time::ptime eventtime;
    ULong64_t timestamp = MRDout.TimeStamp;
    timestamp+=offset_date;
    eventtime = *Epoch + boost::posix_time::time_duration(int(timestamp/1000./60./60.),int(timestamp/1000./60.)%60,int(timestamp/1000.)%60,timestamp%1000);   //default last entry in miliseconds, need special compilation option for nanosec hh:mm:ss:msms
    //if (verbosity > 2 && count == 0) std::cout <<"Starting from /1/08/1970, this results in the date: "<<eventtime.date()<<", with the time: "<<eventtime.time_of_day()<<std::endl;
    if (i_event == 0 && verbosity > 1) std::cout <<eventtime.date()<<","<<eventtime.time_of_day();
    if (i_event == total_number_entries-1 && verbosity > 1) std::cout <<" ... "<<eventtime.date()<<","<<eventtime.time_of_day()<<std::endl;
    boost::posix_time::time_duration dt = eventtime - current;
    double dt_ms = -(dt.total_milliseconds());
    double dt_ms_to_bin = (duration_fivemin*60*1000. - duration.total_milliseconds()) + dt_ms;
    boost::posix_time::time_duration current_stamp_duration = boost::posix_time::time_duration(current - *Epoch);
    current_stamp = current_stamp_duration.total_milliseconds();
    current_stamp-=offset_date;
    //if (verbosity > 5) std::cout <<"Current time stamp: "<<current_stamp<<", data time stamp: "<<MRDout.TimeStamp<<", difference: "<<(current_stamp-MRDout.TimeStamp)<<", dt ms: "<<dt_ms<<std::endl;

    //get end and start points for processed data file
    if (i_event==0 && i_file_fivemin==0) t_file_start = MRDout.TimeStamp;
    if (timestamp > t_file_end) t_file_end = MRDout.TimeStamp;
    if (timestamp < t_file_start) t_file_start = MRDout.TimeStamp;

    //printing intormation about the event
    if (verbosity > 3){
      std::cout <<"------------------------------------------------------------------------------------------------------------------------"<<std::endl;
      std::cout <<"i_event: "<<i_event<<", TimeStamp: "<<timestamp<<std::endl;
      std::cout <<"Slot size: "<<MRDout.Slot.size()<<", Crate size: "<<MRDout.Crate.size()<<", Channel size: "<<MRDout.Channel.size()<<std::endl;
      std::cout <<"OutN: "<<MRDout.OutN<<", Trigger: "<<MRDout.Trigger<<", Type size: "<<MRDout.Type.size()<<std::endl;
    }

    //looping over all channels in event
    for (int i_entry = 0; i_entry < MRDout.Slot.size(); i_entry++){
      int active_slot_nr;
      std::vector<int>::iterator it = std::find(nr_slot.begin(), nr_slot.end(), (MRDout.Slot.at(i_entry))+(MRDout.Crate.at(i_entry)-min_crate)*100);
      if (it == nr_slot.end()){
        std::cout <<"Read-out Crate/Slot/Channel number not active according to configuration file. Check the configfile to process the data..."<<std::endl;
        std::cout <<"Crate: "<<MRDout.Crate.at(i_entry)<<", Slot: "<<MRDout.Slot.at(i_entry)<<std::endl;
        continue;
      }
      count++;
      active_slot_nr = std::distance(nr_slot.begin(),it);
      if (verbosity > 3 && !(MRDout.Crate.at(i_entry)==8 && MRDout.Slot.at(i_entry)==9)) std::cout <<"Getting data.... Crate "<<MRDout.Crate.at(i_entry)<<", Slot "<<MRDout.Slot.at(i_entry)<<", Channel "<<MRDout.Channel.at(i_entry)<<", TDC value: "<<MRDout.Value.at(i_entry)<<std::endl;

      //fill data in live vectors
      int ch = active_slot_nr*num_channels+MRDout.Channel.at(i_entry);
      live_file.at(ch).push_back(MRDout.Value.at(i_entry));
      timestamp_file.at(ch).push_back(MRDout.TimeStamp);
      timediff_file.at(ch).push_back(dt_ms_to_bin);         //in msecs
      n_new_file.at(ch)++;

      //fill data in recent data arrays
       if (dt_ms<(j_fivemin*5*60*1000+duration.total_milliseconds())){
        if (verbosity > 4) std::cout <<"MRDMonitorTime: Filling hour vector with "<<MRDout.Value.at(i_entry)<<"..."<<std::endl;
        live_hour.at(ch).push_back(MRDout.Value.at(i_entry));   //save newest entries for all live vectors
        timestamp_hour.at(ch).push_back(MRDout.TimeStamp);         //save corresponding timestamps to the events
        n_new_day.at(ch)++;
          if (dt_ms<((j_fivemin%6)*5*60*10000+duration.total_milliseconds())){
          if (verbosity > 4) std::cout <<"MRDMonitorTime: Filling half hour vector with "<<MRDout.Value.at(i_entry)<<"..."<<std::endl;
          live_halfhour.at(ch).push_back(MRDout.Value.at(i_entry));
          timestamp_halfhour.at(ch).push_back(MRDout.TimeStamp);
          n_new_sixhour.at(ch)++;
          if (dt_ms<duration.total_milliseconds()){
            if (verbosity > 4) std::cout <<"MRDMonitorTime: Filling 5mins vector with "<<MRDout.Value.at(i_entry)<<"..."<<std::endl;
            live_mins.at(ch).push_back(MRDout.Value.at(i_entry));
            timestamp_mins.at(ch).push_back(MRDout.TimeStamp);
            n_new_hour.at(ch)++;
          }
        }
      }
    }

    //clear MRDout vectors afterwards
	  MRDout.Value.clear();
    MRDout.Slot.clear();
    MRDout.Channel.clear();
    MRDout.Crate.clear();
    MRDout.Type.clear();
	}

  //check if current file has already been procesed
  if (fabs(t_file_start-t_file_start_previous)<0.001 && fabs(t_file_end-t_file_end_previous)<0.001) {
    if (verbosity > 1) std::cout <<"MRDMonitorTime: File with the same start and end times has already been processed. Omit entries..."<<std::endl;
    omit_entries = true;     //don't double-count files that are identical [identical start & end time]
  }

  //
  if (omit_entries){
    for (int i_channel=0;i_channel < num_active_slots*num_channels; i_channel++){
      live_mins.at(i_channel).erase(live_mins.at(i_channel).end()-n_new_hour.at(i_channel),live_mins.at(i_channel).end());
      timestamp_mins.at(i_channel).erase(timestamp_mins.at(i_channel).end()-n_new_hour.at(i_channel),timestamp_mins.at(i_channel).end());
      live_halfhour.at(i_channel).erase(live_halfhour.at(i_channel).end()-n_new_sixhour.at(i_channel),live_halfhour.at(i_channel).end());
      timestamp_halfhour.at(i_channel).erase(timestamp_halfhour.at(i_channel).end()-n_new_sixhour.at(i_channel),timestamp_halfhour.at(i_channel).end());
      live_hour.at(i_channel).erase(live_hour.at(i_channel).end()-n_new_day.at(i_channel),live_hour.at(i_channel).end());
      timestamp_hour.at(i_channel).erase(timestamp_hour.at(i_channel).end()-n_new_day.at(i_channel),timestamp_hour.at(i_channel).end());
      live_file.at(i_channel).erase(live_file.at(i_channel).end()-n_new_file.at(i_channel),live_file.at(i_channel).end());
      timestamp_file.at(i_channel).erase(timestamp_file.at(i_channel).end()-n_new_file.at(i_channel),timestamp_file.at(i_channel).end());
      timediff_file.at(i_channel).erase(timediff_file.at(i_channel).end()-n_new_file.at(i_channel),timediff_file.at(i_channel).end());
    }
  }

  n_new_hour.clear();
  n_new_sixhour.clear();
  n_new_day.clear();
  n_new_file.clear();

}

