#include "MonitorMRDTime.h"

MonitorMRDTime::MonitorMRDTime():Tool(){}



bool MonitorMRDTime::Initialise(std::string configfile, DataModel &data){

  std::cout <<"Tool MonitorMRDTime: Initialising...."<<std::endl;

  /////////////////// Usefull header ///////////////////////
  if(configfile!="")  m_variables.Initialise(configfile); //loading config file
  //m_variables.Print();

  m_data= &data; //assigning transient data pointer
  /////////////////////////////////////////////////////////////////

  m_variables.Get("verbose",verbosity);
  m_variables.Get("OutputPath",outpath_temp);
  m_variables.Get("ActiveSlots",active_slots);
  m_variables.Get("StartTime", StartTime);
  m_variables.Get("OffsetDate",offset_date);          //temporary time offset to work with the test mrd output
  m_variables.Get("DrawMarker",draw_marker);

  Epoch = new boost::posix_time::ptime(boost::gregorian::from_string(StartTime));

  if (verbosity > 4) std::cout <<"outpath_temp: "<<outpath_temp<<std::endl;
  if (outpath_temp == "fromStore") m_data->CStore.Get("OutPath",outpath);
  else outpath = outpath_temp;
  if (verbosity > 4) std::cout <<"Output path for plots is "<<outpath<<std::endl;

  num_active_slots=0;
  num_active_slots_cr1=0;
  num_active_slots_cr2=0;

  max_files=0;

  ifstream file(active_slots.c_str());
  int temp_crate, temp_slot;
  while (!file.eof()){

    file>>temp_crate>>temp_slot;
    if (temp_crate-min_crate<0 || temp_crate-min_crate>=num_crates) {
        std::cout <<"Specified crate out of range [7...8]. Continue with next entry."<<std::endl;
        continue;
    }
    if (temp_slot<1 || temp_slot>num_slots){
        std::cout <<"Specified slot out of range [1...24]. Continue with next entry."<<std::endl;
        continue;
    }
    active_channel[temp_crate-min_crate][temp_slot-1]=1;		//slot start with number 1 instead of 0, crates with 7
    nr_slot.push_back((temp_crate-min_crate)*100+(temp_slot)); 
    num_active_slots++;
    if (temp_crate-min_crate==0) num_active_slots_cr1++;
    if (temp_crate-min_crate==1) num_active_slots_cr2++;
    //std::cout <<"active channel: slot "<<temp_slot<<", crate "<<temp_crate<<std::endl;
  }
  file.close();

  InitializeVectors();

  data_available = false;
  j_fivemin=0;
  enum_slots = 0;
  initial = true;

  //setup time variables
  period_update = boost::posix_time::time_duration(0,5,0,0);
  last = boost::posix_time::ptime(boost::posix_time::second_clock::local_time());


  gROOT->ProcessLine("gErrorIgnoreLevel = 1001;");        //disable printing of warnings
  
  return true;
}


bool MonitorMRDTime::Execute(){

   if (verbosity > 3) std::cout <<"Tool MonitorMRDTime: Executing ...."<<std::endl;

  	boost::posix_time::ptime current(boost::posix_time::second_clock::local_time());
  	duration = boost::posix_time::time_duration(current - last);
  	if (verbosity > 2) std::cout <<"duration: "<<duration.total_milliseconds()/1000./60.<<" mins since last time plot"<<std::endl;

   std::string State;

   m_data->CStore.Get("State",State);


   if (State == "MRDSingle" || State == "Wait"){			//just check the time window that has passed

    if (verbosity > 3) std::cout <<"State is "<<State<<std::endl;

   }else if (State == "DataFile"){				//fill the data of file into vectors

   	std::cout<<"MRD: New data file available."<<std::endl;
   	m_data->Stores["CCData"]->Get("FileData",MRDdata);
   	data_available = true;

    FillEvents();

   }else {

   	std::cout <<"State not recognized: "<<State<<std::endl;

   }

  //check if time has passed without something happening / whether it's time to update the plot again

  if(duration>period_update){
    std::cout <<"5mins passed... Updating plots!"<<std::endl;
  	j_fivemin++;
    last=current;
    MonitorMRDTime::UpdateMonitorSources();
    data_available = false;
    if (j_fivemin%12 == 0) j_fivemin = 0;
    max_files=0;
    initial=false;
	}

  return true;

}


bool MonitorMRDTime::Finalise(){

  std::cout <<"Tool MonitorMRDTime: Finalising ...."<<std::endl;

  MRDdata->Delete();

  return true;
}

void MonitorMRDTime::InitializeVectors(){

  std::vector<double> empty_vec;

 // std::cout <<"Initialising vectors..."<<std::endl;
  int sum_channels=0;

  //initialize all vectors
  for (int i_crate = 0; i_crate < num_crates; i_crate++){

    std::array<double, num_fivemin> tdc_fivemin = {0};
    std::array<double, num_halfhour> tdc_halfhour = {0};
    std::array<double, num_hour> tdc_hour = {0};
    std::array<long, num_fivemin> n_fivemin = {0};
    std::array<long, num_halfhour> n_halfhour = {0};
    std::array<long, num_hour> n_hour = {0};

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

        std::string name_graph = "cr"+ss_crate.str()+"_sl"+ss_slot.str()+"_ch"+ss_ch.str();
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

        TGraph *graph_ch = new TGraph();
        TGraph *graph_ch_rms = new TGraph();
        TGraph *graph_ch_freq = new TGraph();
        TGraph *graph_ch_sixhour = new TGraph();
        TGraph *graph_ch_sixhour_rms = new TGraph();
        TGraph *graph_ch_sixhour_freq = new TGraph();
        TGraph *graph_ch_day = new TGraph();
        TGraph *graph_ch_day_rms = new TGraph();
        TGraph *graph_ch_day_freq = new TGraph();
        graph_ch->SetName(name_graph.c_str());
        graph_ch->SetTitle(title_graph.c_str());
        int line_color_ch = (i_channel%(num_channels/2)<9)? i_channel%(num_channels/2)+1 : (i_channel%(num_channels/2)-9)+40;
        //int line_color_ch = (i_channel%4)+1;  //on each canvas 4 channels
        if (draw_marker) {
          graph_ch->SetMarkerStyle(20);
          graph_ch->SetMarkerColor(line_color_ch);
        }
        graph_ch->SetLineColor(line_color_ch);
        graph_ch->SetLineWidth(2);
        graph_ch->SetFillColor(0);
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

  //std::cout <<"number of channels: "<<sum_channels<<std::endl;
}

void MonitorMRDTime::MRDTimePlots(){


//fill the TGraphs first before plotting them

  for (int i_channel=0; i_channel<num_active_slots*num_channels;i_channel++){

  	for (int i_mins=0;i_mins<num_fivemin;i_mins++){

      if (verbosity > 3){

        std::cout <<"STORED DATA (HOUR): point "<<i_mins<<", time: "<<label_fivemin[i_mins].GetTime()<<", tdc: "<<times_channel_hour.at(i_channel)[i_mins]<<std::endl;

      }

  	gr_times_hour.at(i_channel)->SetPoint(i_mins,label_fivemin[i_mins].Convert(),times_channel_hour.at(i_channel)[i_mins]);
		gr_rms_hour.at(i_channel)->SetPoint(i_mins,label_fivemin[i_mins].Convert(),rms_channel_hour.at(i_channel)[i_mins]);
		gr_frequency_hour.at(i_channel)->SetPoint(i_mins,label_fivemin[i_mins].Convert(),frequency_channel_hour.at(i_channel)[i_mins]);

  	}

    if (update_halfhour){
  	for (int i_halfhour=0;i_halfhour<num_halfhour;i_halfhour++){

      if (verbosity > 3){

        std::cout <<"STORED DATA (6 HOURs): point "<<i_halfhour<<", time: "<<label_halfhour[i_halfhour].GetTime()<<", tdc: "<<times_channel_sixhour.at(i_channel)[i_halfhour]<<std::endl;

      }
    

		gr_times_sixhour.at(i_channel)->SetPoint(i_halfhour,label_halfhour[i_halfhour].Convert(),times_channel_sixhour.at(i_channel)[i_halfhour]);
		gr_rms_sixhour.at(i_channel)->SetPoint(i_halfhour,label_halfhour[i_halfhour].Convert(),rms_channel_sixhour.at(i_channel)[i_halfhour]);
		gr_frequency_sixhour.at(i_channel)->SetPoint(i_halfhour,label_halfhour[i_halfhour].Convert(),frequency_channel_sixhour.at(i_channel)[i_halfhour]);

  	}
  }

    if (update_hour){
  	for (int i_hour=0;i_hour<num_hour;i_hour++){

      if (verbosity > 3){

        std::cout <<"STORED DATA (DAY): point "<<i_hour<<", time: "<<label_hour[i_hour].GetTime()<<", tdc: "<<times_channel_day.at(i_channel)[i_hour]<<std::endl;

      }
		
		gr_times_day.at(i_channel)->SetPoint(i_hour,label_hour[i_hour].Convert(),times_channel_day.at(i_channel)[i_hour]);
		gr_rms_day.at(i_channel)->SetPoint(i_hour,label_hour[i_hour].Convert(),rms_channel_day.at(i_channel)[i_hour]);
		gr_frequency_day.at(i_channel)->SetPoint(i_hour,label_hour[i_hour].Convert(),frequency_channel_day.at(i_channel)[i_hour]);

  	}
  }

  }

  for (int i_slot = 0; i_slot<num_active_slots;i_slot++){

  	for (int i_mins = 0;i_mins<num_fivemin;i_mins++){

		gr_slot_times_hour.at(i_slot)->SetPoint(i_mins,label_fivemin[i_mins].Convert(),times_slot_hour.at(i_slot)[i_mins]);
		gr_slot_rms_hour.at(i_slot)->SetPoint(i_mins,label_fivemin[i_mins].Convert(),rms_slot_hour.at(i_slot)[i_mins]);
		gr_slot_frequency_hour.at(i_slot)->SetPoint(i_mins,label_fivemin[i_mins].Convert(),frequency_slot_hour.at(i_slot)[i_mins]);

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

 for (int i_crate = 0; i_crate<num_crates;i_crate++){

  	for (int i_mins = 0;i_mins<num_fivemin;i_mins++){

		gr_crate_times_hour.at(i_crate)->SetPoint(i_mins,label_fivemin[i_mins].Convert(),times_crate_hour.at(i_crate)[i_mins]);
		gr_crate_rms_hour.at(i_crate)->SetPoint(i_mins,label_fivemin[i_mins].Convert(),rms_crate_hour.at(i_crate)[i_mins]);
		gr_crate_frequency_hour.at(i_crate)->SetPoint(i_mins,label_fivemin[i_mins].Convert(),frequency_crate_hour.at(i_crate)[i_mins]);

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

//plot all the distributions

	TCanvas *canvas_ch = new TCanvas("canvas_ch","Channel Canvas",900,600);
    TCanvas *canvas_ch_rms = new TCanvas("canvas_ch_rms","Channel RMS Canvas",900,600);
    TCanvas *canvas_ch_freq = new TCanvas("canvas_ch_freq","Channel Freq Canvas",900,600);
  	TCanvas *canvas_slot = new TCanvas("canvas_slot","Slot Canvas",900,600);
    TCanvas *canvas_slot_rms = new TCanvas("canvas_slot_rms","Slot RMS Canvas",900,600);
    TCanvas *canvas_slot_freq = new TCanvas("canvas_slot_freq","Slot Freq Canvas",900,600);
  	TCanvas *canvas_crate = new TCanvas("canvas_crate","Crate Canvas",900,600);
    TCanvas *canvas_crate_rms = new TCanvas("canvas_crate_rms","Crate RMS Canvas",900,600);
    TCanvas *canvas_crate_freq = new TCanvas("canvas_crate_freq","Crate Freq Canvas",900,600);
   // TCanvas *canvas_test = new TCanvas("canvas_test","Test Canvas",900,600);

    std::stringstream ss_leg_ch, ss_leg_slot, ss_leg_crate;

  if (update_mins){

    TLegend *leg;
    TLegend *leg_rms;
    TLegend *leg_freq;
    
    max_canvas = 0;
    min_canvas = 9999999.;
    max_canvas_rms = 0;
    min_canvas_rms = 99999999.;
    max_canvas_freq = 0;
    min_canvas_freq = 999999999.;
    TMultiGraph *multi_ch;
    TMultiGraph *multi_ch_rms;
    TMultiGraph *multi_ch_freq;
    int num_ch = 16;   //channels per canvas
    int enum_ch_canvas=0;


  for (int i_channel =0;i_channel<times_channel_hour.size();i_channel++){

    std::stringstream ss_ch, ss_ch_rms, ss_ch_freq;
    
  	if (i_channel%num_ch == 0) {
      if (i_channel != 0){
 
        int num_crate = (i_channel>num_active_slots_cr1*num_channels)? min_crate+1 : min_crate;
        int num_slot = (mapping_vector_ch.at(i_channel-1)-(i_channel-1)%num_channels-(num_crate-min_crate)*num_slots)/num_channels+1;

        ss_ch.str("");
        ss_ch_rms.str("");
        ss_ch_freq.str("");
        ss_ch<<"Crate "<<num_crate<<" Slot "<<num_slot<<" (1 hour) ["<<enum_ch_canvas+1<<"]";
        ss_ch_rms<<"Crate "<<num_crate<<" Slot "<<num_slot<<" (1 hour) ["<<enum_ch_canvas+1<<"]";
        ss_ch_freq<<"Crate "<<num_crate<<" Slot "<<num_slot<<" (1 hour) ["<<enum_ch_canvas+1<<"]";
        

        canvas_ch->cd();
        multi_ch->Draw("apl");
        multi_ch->SetTitle(ss_ch.str().c_str());
        multi_ch->GetYaxis()->SetTitle("TDC");
        multi_ch->GetXaxis()->SetTimeDisplay(1);
        multi_ch->GetXaxis()->SetLabelSize(0.03);
        multi_ch->GetXaxis()->SetLabelOffset(0.03);
        multi_ch->GetXaxis()->SetTimeFormat("#splitline{%m\/%d}{%H:%M}");
        //multi_ch->GetXaxis()->SetTimeOffset(0,"gmt");
        leg->Draw();

        canvas_ch_rms->cd();
        multi_ch_rms->Draw("apl");
        multi_ch_rms->SetTitle(ss_ch_rms.str().c_str());
        multi_ch_rms->GetYaxis()->SetTitle("RMS (TDC)");
        multi_ch_rms->GetYaxis()->SetTitleSize(0.035);
        multi_ch_rms->GetYaxis()->SetTitleOffset(1.3);
        multi_ch_rms->GetXaxis()->SetTimeDisplay(1);
        multi_ch_rms->GetXaxis()->SetLabelSize(0.03);
        multi_ch_rms->GetXaxis()->SetLabelOffset(0.03);
        multi_ch_rms->GetXaxis()->SetTimeFormat("#splitline{%m\/%d}{%H:%M}");
        //multi_ch_rms->GetXaxis()->SetTimeOffset(0,"gmt");
        leg_rms->Draw();

        canvas_ch_freq->cd();
        multi_ch_freq->Draw("apl");
        multi_ch_freq->SetTitle(ss_ch_freq.str().c_str());
        multi_ch_freq->GetYaxis()->SetTitle("Freq [1/min]");
        multi_ch_freq->GetYaxis()->SetTitleSize(0.035);
        multi_ch_freq->GetYaxis()->SetTitleOffset(1.3);
        multi_ch_freq->GetXaxis()->SetTimeDisplay(1);
        multi_ch_freq->GetXaxis()->SetLabelSize(0.03);
        multi_ch_freq->GetXaxis()->SetLabelOffset(0.03);
        multi_ch_freq->GetXaxis()->SetTimeFormat("#splitline{%m\/%d}{%H:%M}");
        //multi_ch_freq->GetXaxis()->SetTimeOffset(0,"gmt");
        leg_freq->Draw();

        ss_ch.str("");
        ss_ch_rms.str("");
        ss_ch_freq.str("");
        ss_ch<<outpath<<"Crate"<<num_crate<<"_Slot"<<num_slot<<"_"<<enum_ch_canvas<<"_HourTrend_TDC.png";
        ss_ch_rms<<outpath<<"Crate"<<num_crate<<"_Slot"<<num_slot<<"_"<<enum_ch_canvas<<"_HourTrend_RMS.png";
        ss_ch_freq<<outpath<<"Crate"<<num_crate<<"_Slot"<<num_slot<<"_"<<enum_ch_canvas<<"_HourTrend_Freq.png";
        //std::stringstream ss_test;
        //ss_test<<outpath<<"canvas_test.png";

        canvas_ch->SaveAs(ss_ch.str().c_str());
        canvas_ch_rms->SaveAs(ss_ch_rms.str().c_str());
        canvas_ch_freq->SaveAs(ss_ch_freq.str().c_str()); 
        //canvas_test->SaveAs(ss_test.str().c_str()); 

        enum_ch_canvas++;  
        enum_ch_canvas=enum_ch_canvas%2;

      }
  		
      canvas_ch->Clear();
      canvas_ch_rms->Clear();
      canvas_ch_freq->Clear();
      //canvas_test->Clear();

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

    //canvas_test->cd();
    //gr_times_hour.at(i_channel)->Draw();

      //canvas_ch->cd();
      //gr_times_hour.at(i_channel)->Draw();

      ss_leg_ch.str("");
      ss_leg_ch<<"ch "<<i_channel%num_channels;
      multi_ch->Add(gr_times_hour.at(i_channel));
      if (gr_times_hour.at(i_channel)->GetMaximum()>max_canvas) max_canvas = gr_times_hour.at(i_channel)->GetMaximum();
      if (gr_times_hour.at(i_channel)->GetMinimum()<min_canvas) min_canvas = gr_times_hour.at(i_channel)->GetMinimum();
      leg->AddEntry(gr_times_hour.at(i_channel),ss_leg_ch.str().c_str(),"l");
      //canvas_ch_rms->cd();
      //gr_rms_hour.at(i_channel)->Draw();
      multi_ch_rms->Add(gr_rms_hour.at(i_channel));
      leg_rms->AddEntry(gr_rms_hour.at(i_channel),ss_leg_ch.str().c_str(),"l");
      if (gr_rms_hour.at(i_channel)->GetMaximum()>max_canvas_rms) max_canvas_rms = gr_rms_hour.at(i_channel)->GetMaximum();
      if (gr_rms_hour.at(i_channel)->GetMinimum()<min_canvas_rms) min_canvas_rms = gr_rms_hour.at(i_channel)->GetMinimum();
      //canvas_ch_freq->cd();
      //gr_frequency_hour.at(i_channel)->Draw();
      multi_ch_freq->Add(gr_frequency_hour.at(i_channel));
      leg_freq->AddEntry(gr_frequency_hour.at(i_channel),ss_leg_ch.str().c_str(),"l");
      if (gr_frequency_hour.at(i_channel)->GetMaximum()>max_canvas_freq) max_canvas_freq = gr_frequency_hour.at(i_channel)->GetMaximum();
      if (gr_frequency_hour.at(i_channel)->GetMinimum()<min_canvas_freq) min_canvas_freq = gr_frequency_hour.at(i_channel)->GetMinimum();

  	} 

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

  		
  		//canvas_slot->cd();
  		//gr_slot_times_hour.at(i_slot)->Draw();
      multi_slot->Add(gr_slot_times_hour.at(i_slot));
      leg_slot->AddEntry(gr_slot_times_hour.at(i_slot),"","l");
      if (gr_slot_times_hour.at(i_slot)->GetMaximum()>max_canvas) max_canvas = gr_slot_times_hour.at(i_slot)->GetMaximum();
      if (gr_slot_times_hour.at(i_slot)->GetMinimum()<min_canvas) min_canvas = gr_slot_times_hour.at(i_slot)->GetMinimum();
  		//canvas_slot_rms->cd();
  		//gr_slot_rms_hour.at(i_slot)->Draw();
      multi_slot_rms->Add(gr_slot_rms_hour.at(i_slot));
      leg_slot_rms->AddEntry(gr_slot_rms_hour.at(i_slot),"","l");
      if (gr_slot_rms_hour.at(i_slot)->GetMaximum()>max_canvas_rms) max_canvas_rms = gr_slot_rms_hour.at(i_slot)->GetMaximum();
      if (gr_slot_rms_hour.at(i_slot)->GetMinimum()<min_canvas_rms) min_canvas_rms = gr_slot_rms_hour.at(i_slot)->GetMinimum();
  		//canvas_slot_freq->cd();
  		//gr_slot_frequency_hour.at(i_slot)->Draw();
      multi_slot_freq->Add(gr_slot_frequency_hour.at(i_slot));
      leg_slot_freq->AddEntry(gr_slot_frequency_hour.at(i_slot),"","l");
      if (gr_slot_frequency_hour.at(i_slot)->GetMaximum()>max_canvas_freq) max_canvas_freq = gr_slot_frequency_hour.at(i_slot)->GetMaximum();
      if (gr_slot_frequency_hour.at(i_slot)->GetMinimum()<min_canvas_freq) min_canvas_freq = gr_slot_frequency_hour.at(i_slot)->GetMinimum();

  	}

    canvas_slot->cd();
    multi_slot->Draw("apl");
    multi_slot->SetTitle("Crate 7 (1 hour)");
    multi_slot->GetYaxis()->SetTitle("TDC");
    multi_slot->GetXaxis()->SetTimeDisplay(1);
    multi_slot->GetXaxis()->SetLabelSize(0.03);
    multi_slot->GetXaxis()->SetLabelOffset(0.03);
    multi_slot->GetXaxis()->SetTimeFormat("#splitline{%m\/%d}{%H:%M}");
    //multi_slot->GetXaxis()->SetTimeOffset(0,"gmt");
    leg_slot->Draw();
    canvas_slot_rms->cd();
    multi_slot_rms->Draw("apl");
    multi_slot_rms->SetTitle("Crate 7 (1 hour)");
    multi_slot_rms->GetYaxis()->SetTitle("RMS (TDC)");
    multi_slot_rms->GetYaxis()->SetTitleSize(0.035);
    multi_slot_rms->GetYaxis()->SetTitleOffset(1.3);
    multi_slot_rms->GetXaxis()->SetTimeDisplay(1);
    multi_slot_rms->GetXaxis()->SetLabelSize(0.03);
    multi_slot_rms->GetXaxis()->SetLabelOffset(0.03);
    multi_slot_rms->GetXaxis()->SetTimeFormat("#splitline{%m\/%d}{%H:%M}");
    //multi_slot_rms->GetXaxis()->SetTimeOffset(0,"gmt");   
    leg_slot_rms->Draw();
    canvas_slot_freq->cd();
    multi_slot_freq->Draw("apl");
    multi_slot_freq->SetTitle("Crate 7 (1 hour)");
    multi_slot_freq->GetYaxis()->SetTitle("Freq [1/min]");
    multi_slot_freq->GetYaxis()->SetTitleSize(0.035);
    multi_slot_freq->GetYaxis()->SetTitleOffset(1.3);
    multi_slot_freq->GetXaxis()->SetTimeDisplay(1);
    multi_slot_freq->GetXaxis()->SetLabelSize(0.03);
    multi_slot_freq->GetXaxis()->SetLabelOffset(0.03);
    multi_slot_freq->GetXaxis()->SetTimeFormat("#splitline{%m\/%d}{%H:%M}");
    //multi_slot_freq->GetXaxis()->SetTimeOffset(0,"gmt");
    leg_slot_freq->Draw();

    std::stringstream ss_slot, ss_slot_rms, ss_slot_freq;
  	ss_slot<<outpath<<"Crate7_Slots_HourTrend_TDC.png";
  	ss_slot_rms<<outpath<<"Crate7_Slots_HourTrend_RMS.png";
  	ss_slot_freq<<outpath<<"Crate7_Slots_HourTrend_Freq.png";
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

		  //canvas_slot->cd();
  		//gr_slot_times_hour.at(i_slot+num_active_slots_cr1)->Draw();
      multi_slot2->Add(gr_slot_times_hour.at(i_slot+num_active_slots_cr1));
      leg_slot2->AddEntry(gr_slot_times_hour.at(i_slot+num_active_slots_cr1),"","l");
      if (gr_slot_times_hour.at(i_slot)->GetMaximum()>max_canvas) max_canvas = gr_slot_times_hour.at(i_slot)->GetMaximum();
      if (gr_slot_times_hour.at(i_slot)->GetMinimum()<min_canvas) min_canvas = gr_slot_times_hour.at(i_slot)->GetMinimum();
  		//canvas_slot_rms->cd();
  		//gr_slot_rms_hour.at(i_slot+num_active_slots_cr1)->Draw();
      multi_slot2_rms->Add(gr_slot_rms_hour.at(i_slot+num_active_slots_cr1));
      leg_slot2_rms->AddEntry(gr_slot_rms_hour.at(i_slot+num_active_slots_cr1),"","l");
      if (gr_slot_rms_hour.at(i_slot)->GetMaximum()>max_canvas_rms) max_canvas_rms = gr_slot_rms_hour.at(i_slot)->GetMaximum();
      if (gr_slot_rms_hour.at(i_slot)->GetMinimum()<min_canvas_rms) min_canvas_rms = gr_slot_rms_hour.at(i_slot)->GetMinimum();
  		//canvas_slot_freq->cd();
  		//gr_slot_frequency_hour.at(i_slot+num_active_slots_cr1)->Draw();
      multi_slot2_freq->Add(gr_slot_frequency_hour.at(i_slot+num_active_slots_cr1));
      leg_slot2_freq->AddEntry(gr_slot_frequency_hour.at(i_slot+num_active_slots_cr1),"","l");
      if (gr_slot_frequency_hour.at(i_slot)->GetMaximum()>max_canvas_freq) max_canvas_freq = gr_slot_frequency_hour.at(i_slot)->GetMaximum();
      if (gr_slot_frequency_hour.at(i_slot)->GetMinimum()<min_canvas_freq) min_canvas_freq = gr_slot_frequency_hour.at(i_slot)->GetMinimum();

  	}

    canvas_slot->cd();
    multi_slot2->Draw("apl");
    multi_slot2->SetTitle("Crate 8 (1 hour)");
    multi_slot2->GetYaxis()->SetTitle("TDC");
    multi_slot2->GetXaxis()->SetTimeDisplay(1);
    multi_slot2->GetXaxis()->SetLabelSize(0.03);
    multi_slot2->GetXaxis()->SetLabelOffset(0.03);
    multi_slot2->GetXaxis()->SetTimeFormat("#splitline{%m\/%d}{%H:%M}");
    //multi_slot2->GetXaxis()->SetTimeOffset(0,"gmt");
    leg_slot2->Draw();
    canvas_slot_rms->cd();
    multi_slot2_rms->Draw("apl");
    multi_slot2_rms->SetTitle("Crate 8 (1 hour)");
    multi_slot2_rms->GetYaxis()->SetTitle("RMS (TDC)");
    multi_slot2_rms->GetYaxis()->SetTitleSize(0.035);
    multi_slot2_rms->GetYaxis()->SetTitleOffset(1.3);
    multi_slot2_rms->GetXaxis()->SetTimeDisplay(1);
    multi_slot2_rms->GetXaxis()->SetLabelOffset(0.03);
    multi_slot2_rms->GetXaxis()->SetLabelSize(0.03);
    multi_slot2_rms->GetXaxis()->SetTimeFormat("#splitline{%m\/%d}{%H:%M}");
    //multi_slot2_rms->GetXaxis()->SetTimeOffset(0,"gmt");
    leg_slot2_rms->Draw();
    canvas_slot_freq->cd();
    multi_slot2_freq->Draw("apl");
    multi_slot2_freq->SetTitle("Crate 8 (1 hour)");
    multi_slot2_freq->GetYaxis()->SetTitle("Freq [1/min]");
    multi_slot2_freq->GetYaxis()->SetTitleSize(0.035);
    multi_slot2_freq->GetYaxis()->SetTitleOffset(1.3);
    multi_slot2_freq->GetXaxis()->SetTimeDisplay(1);
    multi_slot2_freq->GetXaxis()->SetLabelSize(0.03);
    multi_slot2_freq->GetXaxis()->SetLabelOffset(0.03);
    multi_slot2_freq->GetXaxis()->SetTimeFormat("#splitline{%m\/%d}{%H:%M}");
    //multi_slot2_freq->GetXaxis()->SetTimeOffset(0,"gmt");
    leg_slot2_freq->Draw();
/*
    gr_slot_times_hour.at(num_active_slots_cr1)->GetYaxis()->SetRangeUser(min_canvas,max_canvas);
    gr_slot_rms_hour.at(num_active_slots_cr1)->GetYaxis()->SetRangeUser(min_canvas_rms,max_canvas_rms);
    gr_slot_frequency_hour.at(num_active_slots_cr1)->GetYaxis()->SetRangeUser(min_canvas_freq,max_canvas_freq);
*/
  	ss_slot.str("");
  	ss_slot_rms.str("");
  	ss_slot_freq.str("");
  	ss_slot<<outpath<<"Crate8_Slots_HourTrend_TDC.png";
  	ss_slot_rms<<outpath<<"Crate8_Slots_HourTrend_RMS.png";
  	ss_slot_freq<<outpath<<"Crate8_Slots_HourTrend_Freq.png";
  	canvas_slot->SaveAs(ss_slot.str().c_str());
  	canvas_slot_rms->SaveAs(ss_slot_rms.str().c_str());
  	canvas_slot_freq->SaveAs(ss_slot_freq.str().c_str());

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
  		//canvas_crate->cd();
  		//gr_crate_times_hour.at(i_crate)->Draw();
      multi_crate->Add(gr_crate_times_hour.at(i_crate));
      leg_crate->AddEntry(gr_crate_times_hour.at(i_crate),"","l");
      if (gr_crate_times_hour.at(i_crate)->GetMaximum()>max_canvas) max_canvas = gr_crate_times_hour.at(i_crate)->GetMaximum();
      if (gr_crate_times_hour.at(i_crate)->GetMinimum()<min_canvas) min_canvas = gr_crate_times_hour.at(i_crate)->GetMinimum();
      //canvas_crate_rms->cd();
      //gr_crate_rms_hour.at(i_crate)->Draw();
      multi_crate_rms->Add(gr_crate_rms_hour.at(i_crate));
      leg_crate_rms->AddEntry(gr_crate_rms_hour.at(i_crate),"","l");
      if (gr_crate_rms_hour.at(i_crate)->GetMaximum()>max_canvas_rms) max_canvas_rms = gr_crate_rms_hour.at(i_crate)->GetMaximum();
      if (gr_crate_rms_hour.at(i_crate)->GetMinimum()<min_canvas_rms) min_canvas_rms = gr_crate_rms_hour.at(i_crate)->GetMinimum();
      //canvas_crate_freq->cd();
      //gr_crate_frequency_hour.at(i_crate)->Draw();
      multi_crate_freq->Add(gr_crate_frequency_hour.at(i_crate));
      leg_crate_freq->AddEntry(gr_crate_frequency_hour.at(i_crate),"","l");
      if (gr_crate_frequency_hour.at(i_crate)->GetMaximum()>max_canvas_freq) max_canvas_freq = gr_crate_frequency_hour.at(i_crate)->GetMaximum();
      if (gr_crate_frequency_hour.at(i_crate)->GetMinimum()<min_canvas_freq) min_canvas_freq = gr_crate_frequency_hour.at(i_crate)->GetMinimum();


  	}

    canvas_crate->cd();
    multi_crate->Draw("apl");
    multi_crate->SetTitle("Crates (1 hour)");
    multi_crate->GetYaxis()->SetTitle("TDC");
    multi_crate->GetXaxis()->SetTimeDisplay(1);
    multi_crate->GetXaxis()->SetLabelSize(0.03);
    multi_crate->GetXaxis()->SetLabelOffset(0.03);
    multi_crate->GetXaxis()->SetTimeFormat("#splitline{%m\/%d}{%H:%M}");
    //multi_crate->GetXaxis()->SetTimeOffset(0,"gmt");
    leg_crate->Draw();
    canvas_crate_rms->cd();
    multi_crate_rms->Draw("apl");
    multi_crate_rms->SetTitle("Crates (1 hour)");
    multi_crate_rms->GetYaxis()->SetTitle("RMS (TDC)");
    multi_crate_rms->GetYaxis()->SetTitleSize(0.035);
    multi_crate_rms->GetYaxis()->SetTitleOffset(1.3);
    multi_crate_rms->GetXaxis()->SetTimeDisplay(1);
    multi_crate_rms->GetXaxis()->SetLabelSize(0.03);
    multi_crate_rms->GetXaxis()->SetLabelOffset(0.03);
    multi_crate_rms->GetXaxis()->SetTimeFormat("#splitline{%m\/%d}{%H:%M}");
    //multi_crate_rms->GetXaxis()->SetTimeOffset(0,"gmt");
    leg_crate_rms->Draw();
    canvas_crate_freq->cd();
    multi_crate_freq->Draw("apl");
    multi_crate_freq->SetTitle("Crates (1 hour)");
    multi_crate_freq->GetYaxis()->SetTitle("Freq [1/min]");
    multi_crate_freq->GetYaxis()->SetTitleSize(0.035);
    multi_crate_freq->GetYaxis()->SetTitleOffset(1.3);
    multi_crate_freq->GetXaxis()->SetTimeDisplay(1);
    multi_crate_freq->GetXaxis()->SetLabelSize(0.03);
    multi_crate_freq->GetXaxis()->SetLabelOffset(0.03);
    multi_crate_freq->GetXaxis()->SetTimeFormat("#splitline{%m\/%d}{%H:%M}");
    //multi_crate_freq->GetXaxis()->SetTimeOffset(0,"gmt");
    leg_crate_freq->Draw();

    std::stringstream ss_crate, ss_crate_rms, ss_crate_freq;
    ss_crate<<outpath<<"Crates_HourTrend_TDC.png";
    ss_crate_rms<<outpath<<"Crates_HourTrend_RMS.png";
    ss_crate_freq<<outpath<<"Crates_HourTrend_Freq.png";
    canvas_crate->SaveAs(ss_crate.str().c_str());
    canvas_crate_rms->SaveAs(ss_crate_rms.str().c_str());
    canvas_crate_freq->SaveAs(ss_crate_freq.str().c_str());

  }

  if (update_halfhour){

    TLegend *leg;
    TLegend *leg_rms;
    TLegend *leg_freq;
    

    max_canvas = 0;
    min_canvas = 9999999.;
    max_canvas_rms = 0;
    min_canvas_rms = 99999999.;
    max_canvas_freq = 0;
    min_canvas_freq = 999999999.;

    TMultiGraph *multi_ch;
    TMultiGraph *multi_ch_rms;
    TMultiGraph *multi_ch_freq;

    int enum_ch_canvas = 0;
    int num_ch = 16;


    for (int i_channel =0;i_channel<times_channel_sixhour.size();i_channel++){

    std::stringstream ss_ch, ss_ch_rms, ss_ch_freq;
    
    if (i_channel%num_ch == 0) {
      if (i_channel != 0){


        int num_crate = (i_channel>num_active_slots_cr1*num_channels)? min_crate : min_crate+1;
        int num_slot = (mapping_vector_ch.at(i_channel-1)-(i_channel-1)%num_channels-(num_crate-min_crate)*num_slots)/num_channels;
        //int num_slot = (i_channel>num_active_slots_cr1*num_channels)? (i_channel-num_active_slots_cr1*num_channels)/num_channels+1 : i_channel/num_channels+1;


        ss_ch.str("");
        ss_ch_rms.str("");
        ss_ch_freq.str("");
        ss_ch<<"Crate "<<num_crate<<" Slot "<<num_slot<<" (6 hours) ["<<enum_ch_canvas+1<<"]";
        ss_ch_rms<<"Crate "<<num_crate<<" Slot "<<num_slot<<" (6 hours) ["<<enum_ch_canvas+1<<"]";
        ss_ch_freq<<"Crate "<<num_crate<<" Slot "<<num_slot<<" (6 hours) ["<<enum_ch_canvas+1<<"]";

        canvas_ch->cd();
        multi_ch->Draw("apl");
        multi_ch->SetTitle(ss_ch.str().c_str());
        multi_ch->GetYaxis()->SetTitle("TDC");
        multi_ch->GetXaxis()->SetTimeDisplay(1);
        multi_ch->GetXaxis()->SetLabelSize(0.03);
        multi_ch->GetXaxis()->SetLabelOffset(0.03);
        multi_ch->GetXaxis()->SetTimeFormat("#splitline{%m\/%d}{%H:%M}");
       // multi_ch->GetXaxis()->SetTimeOffset(0,"gmt");
        leg->Draw();

        canvas_ch_rms->cd();
        multi_ch_rms->Draw("apl");
        multi_ch_rms->SetTitle(ss_ch_rms.str().c_str());
        multi_ch_rms->GetYaxis()->SetTitle("RMS (TDC)");
        multi_ch_rms->GetYaxis()->SetTitleSize(0.035);
        multi_ch_rms->GetYaxis()->SetTitleOffset(1.3);
        multi_ch_rms->GetXaxis()->SetTimeDisplay(1);
        multi_ch_rms->GetXaxis()->SetLabelSize(0.03);
        multi_ch_rms->GetXaxis()->SetLabelOffset(0.03);
        multi_ch_rms->GetXaxis()->SetTimeFormat("#splitline{%m\/%d}{%H:%M}");
        //multi_ch_rms->GetXaxis()->SetTimeOffset(0,"gmt");
        leg_rms->Draw();

        canvas_ch_freq->cd();
        multi_ch_freq->Draw("apl");
        multi_ch_freq->SetTitle(ss_ch_freq.str().c_str());
        multi_ch_freq->GetYaxis()->SetTitle("Freq [1/min]");
        multi_ch_freq->GetYaxis()->SetTitleSize(0.035);
        multi_ch_freq->GetYaxis()->SetTitleOffset(1.3);
        multi_ch_freq->GetXaxis()->SetTimeDisplay(1);
        multi_ch_freq->GetXaxis()->SetLabelSize(0.03);
        multi_ch_freq->GetXaxis()->SetLabelOffset(0.03);
        multi_ch_freq->GetXaxis()->SetTimeFormat("#splitline{%m\/%d}{%H:%M}");
       // multi_ch_freq->GetXaxis()->SetTimeOffset(0,"gmt");
        leg_freq->Draw();

        ss_ch.str("");
        ss_ch_rms.str("");
        ss_ch_freq.str("");
        ss_ch<<outpath<<"Crate"<<num_crate<<"_Slot"<<num_slot<<"_"<<enum_ch_canvas<<"_6HourTrend_TDC.png";
        ss_ch_rms<<outpath<<"Crate"<<num_crate<<"_Slot"<<num_slot<<"_"<<enum_ch_canvas<<"_6HourTrend_RMS.png";
        ss_ch_freq<<outpath<<"Crate"<<num_crate<<"_Slot"<<num_slot<<"_"<<enum_ch_canvas<<"_6HourTrend_Freq.png";
        canvas_ch->SaveAs(ss_ch.str().c_str());
        canvas_ch_rms->SaveAs(ss_ch_rms.str().c_str());
        canvas_ch_freq->SaveAs(ss_ch_freq.str().c_str());    

        enum_ch_canvas++;
        enum_ch_canvas = enum_ch_canvas%2;

      }
      
      canvas_ch->Clear();
      canvas_ch_rms->Clear();
      canvas_ch_freq->Clear();

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

      ss_leg_ch.str("");
      ss_leg_ch<<"ch "<<i_channel%num_channels;
      multi_ch->Add(gr_times_sixhour.at(i_channel));
      leg->AddEntry(gr_times_sixhour.at(i_channel),ss_leg_ch.str().c_str(),"l");
      if (gr_times_sixhour.at(i_channel)->GetMaximum()>max_canvas) max_canvas = gr_times_sixhour.at(i_channel)->GetMaximum();
      if (gr_times_sixhour.at(i_channel)->GetMinimum()<min_canvas) min_canvas = gr_times_sixhour.at(i_channel)->GetMinimum();
      //canvas_ch_rms->cd();
      //gr_rms_sixhour.at(i_channel)->Draw();
      multi_ch_rms->Add(gr_rms_sixhour.at(i_channel));
      leg_rms->AddEntry(gr_rms_sixhour.at(i_channel),ss_leg_ch.str().c_str(),"l");
      if (gr_rms_sixhour.at(i_channel)->GetMaximum()>max_canvas_rms) max_canvas_rms = gr_rms_sixhour.at(i_channel)->GetMaximum();
      if (gr_rms_sixhour.at(i_channel)->GetMinimum()<min_canvas_rms) min_canvas_rms = gr_rms_sixhour.at(i_channel)->GetMinimum();
      //canvas_ch_freq->cd();
      //gr_frequency_sixhour.at(i_channel)->Draw();
      multi_ch_freq->Add(gr_frequency_sixhour.at(i_channel));
      leg_freq->AddEntry(gr_frequency_sixhour.at(i_channel),ss_leg_ch.str().c_str(),"l");
      if (gr_frequency_sixhour.at(i_channel)->GetMaximum()>max_canvas_freq) max_canvas_freq = gr_frequency_sixhour.at(i_channel)->GetMaximum();
      if (gr_frequency_sixhour.at(i_channel)->GetMinimum()<min_canvas_freq) min_canvas_freq = gr_frequency_sixhour.at(i_channel)->GetMinimum();

    } 

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
      
      //canvas_slot->cd();
      //gr_slot_times_sixhour.at(i_slot)->Draw();
      multi_slot->Add(gr_slot_times_sixhour.at(i_slot));
      leg_slot->AddEntry(gr_slot_times_sixhour.at(i_slot),"","l");
      if (gr_slot_times_sixhour.at(i_slot)->GetMaximum()>max_canvas) max_canvas = gr_slot_times_sixhour.at(i_slot)->GetMaximum();
      if (gr_slot_times_sixhour.at(i_slot)->GetMinimum()<min_canvas) min_canvas = gr_slot_times_sixhour.at(i_slot)->GetMinimum();
      //canvas_slot_rms->cd();
      //gr_slot_rms_sixhour.at(i_slot)->Draw();
      multi_slot_rms->Add(gr_slot_rms_sixhour.at(i_slot));
      leg_slot_rms->AddEntry(gr_slot_rms_sixhour.at(i_slot),"","l");
      if (gr_slot_rms_sixhour.at(i_slot)->GetMaximum()>max_canvas_rms) max_canvas_rms = gr_slot_rms_sixhour.at(i_slot)->GetMaximum();
      if (gr_slot_rms_sixhour.at(i_slot)->GetMinimum()<min_canvas_rms) min_canvas_rms = gr_slot_rms_sixhour.at(i_slot)->GetMinimum();
      //canvas_slot_freq->cd();
      //gr_slot_frequency_sixhour.at(i_slot)->Draw();
      multi_slot_freq->Add(gr_slot_frequency_sixhour.at(i_slot));
      leg_slot_freq->AddEntry(gr_slot_frequency_sixhour.at(i_slot),"","l");
      if (gr_slot_frequency_sixhour.at(i_slot)->GetMaximum()>max_canvas_freq) max_canvas_freq = gr_slot_frequency_sixhour.at(i_slot)->GetMaximum();
      if (gr_slot_frequency_sixhour.at(i_slot)->GetMinimum()<min_canvas_freq) min_canvas_freq = gr_slot_frequency_sixhour.at(i_slot)->GetMinimum();


    }

    canvas_slot->cd();
    multi_slot->Draw("apl");
    multi_slot->SetTitle("Crate 7 (6 hours)");
    multi_slot->GetYaxis()->SetTitle("TDC");
    multi_slot->GetXaxis()->SetTimeDisplay(1);
    multi_slot->GetXaxis()->SetLabelSize(0.03);
    multi_slot->GetXaxis()->SetLabelOffset(0.03);
    multi_slot->GetXaxis()->SetTimeFormat("#splitline{%m\/%d}{%H:%M}");
    //multi_slot->GetXaxis()->SetTimeOffset(0,"gmt");
    leg_slot->Draw();
    canvas_slot_rms->cd();
    multi_slot_rms->Draw("apl");
    multi_slot_rms->SetTitle("Crate 7 (6 hours)");
    multi_slot_rms->GetYaxis()->SetTitle("RMS (TDC)");
    multi_slot_rms->GetYaxis()->SetTitleSize(0.035);
    multi_slot_rms->GetYaxis()->SetTitleOffset(1.3);
    multi_slot_rms->GetXaxis()->SetTimeDisplay(1);
    multi_slot_rms->GetXaxis()->SetLabelSize(0.03);
    multi_slot_rms->GetXaxis()->SetLabelOffset(0.03);
    multi_slot_rms->GetXaxis()->SetTimeFormat("#splitline{%m\/%d}{%H:%M}");
    //multi_slot_rms->GetXaxis()->SetTimeOffset(0,"gmt");
    leg_slot_rms->Draw();
    canvas_slot_freq->cd();
    multi_slot_freq->Draw("apl");
    multi_slot_freq->SetTitle("Crate 7 (6 hours)");
    multi_slot_freq->GetYaxis()->SetTitle("Freq [1/min]");
    multi_slot_freq->GetYaxis()->SetTitleSize(0.035);
    multi_slot_freq->GetYaxis()->SetTitleOffset(1.3);
    multi_slot_freq->GetXaxis()->SetTimeDisplay(1);
    multi_slot_freq->GetXaxis()->SetLabelSize(0.03);
    multi_slot_freq->GetXaxis()->SetLabelOffset(0.03);
    multi_slot_freq->GetXaxis()->SetTimeFormat("#splitline{%m\/%d}{%H:%M}");
    //multi_slot_freq->GetXaxis()->SetTimeOffset(0,"gmt");
    leg_slot_freq->Draw();

    std::stringstream ss_slot, ss_slot_rms, ss_slot_freq;
    ss_slot<<outpath<<"Crate7_Slots_6HourTrend_TDC.png";
    ss_slot_rms<<outpath<<"Crate7_Slots_6HourTrend_RMS.png";
    ss_slot_freq<<outpath<<"Crate7_Slots_6HourTrend_Freq.png";
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

      //canvas_slot->cd();
      //gr_slot_times_sixhour.at(i_slot+num_active_slots_cr1)->Draw();
      multi_slot2->Add(gr_slot_times_sixhour.at(i_slot+num_active_slots_cr1));
      leg_slot2->AddEntry(gr_slot_times_sixhour.at(i_slot+num_active_slots_cr1),"","l");
      if (gr_slot_times_sixhour.at(i_slot)->GetMaximum()>max_canvas) max_canvas = gr_slot_times_sixhour.at(i_slot)->GetMaximum();
      if (gr_slot_times_sixhour.at(i_slot)->GetMinimum()<min_canvas) min_canvas = gr_slot_times_sixhour.at(i_slot)->GetMinimum();
      //canvas_slot_rms->cd();
      //gr_slot_rms_sixhour.at(i_slot+num_active_slots_cr1)->Draw();
      multi_slot2_rms->Add(gr_slot_rms_sixhour.at(i_slot+num_active_slots_cr1));
      leg_slot2_rms->AddEntry(gr_slot_rms_sixhour.at(i_slot+num_active_slots_cr1),"","l");
      if (gr_slot_rms_sixhour.at(i_slot)->GetMaximum()>max_canvas_rms) max_canvas_rms = gr_slot_rms_sixhour.at(i_slot)->GetMaximum();
      if (gr_slot_rms_sixhour.at(i_slot)->GetMinimum()<min_canvas_rms) min_canvas_rms = gr_slot_rms_sixhour.at(i_slot)->GetMinimum();
      //canvas_slot_freq->cd();
      //gr_slot_frequency_sixhour.at(i_slot+num_active_slots_cr1)->Draw();
      multi_slot2_freq->Add(gr_slot_frequency_sixhour.at(i_slot+num_active_slots_cr1));
      leg_slot2_freq->AddEntry(gr_slot_frequency_sixhour.at(i_slot+num_active_slots_cr1),"","l");
      if (gr_slot_frequency_sixhour.at(i_slot)->GetMaximum()>max_canvas_freq) max_canvas_freq = gr_slot_frequency_sixhour.at(i_slot)->GetMaximum();
      if (gr_slot_frequency_sixhour.at(i_slot)->GetMinimum()<min_canvas_freq) min_canvas_freq = gr_slot_frequency_sixhour.at(i_slot)->GetMinimum();


    }

    canvas_slot->cd();
    multi_slot2->Draw("apl");
    multi_slot2->SetTitle("Crate 8 (6 hours)");
    multi_slot2->GetYaxis()->SetTitle("TDC");
    multi_slot2->GetXaxis()->SetTimeDisplay(1);
    multi_slot2->GetXaxis()->SetLabelSize(0.03);
    multi_slot2->GetXaxis()->SetLabelOffset(0.03);
    multi_slot2->GetXaxis()->SetTimeFormat("#splitline{%m\/%d}{%H:%M}");
    //multi_slot2->GetXaxis()->SetTimeOffset(0,"gmt");
    leg_slot2->Draw();
    canvas_slot_rms->cd();
    multi_slot2_rms->Draw("apl");
    multi_slot2_rms->SetTitle("Crate 8 (6 hours)");
    multi_slot2_rms->GetYaxis()->SetTitle("RMS (TDC)");
    multi_slot2_rms->GetYaxis()->SetTitleSize(0.035);
    multi_slot2_rms->GetYaxis()->SetTitleOffset(1.3);
    multi_slot2_rms->GetXaxis()->SetTimeDisplay(1);
    multi_slot2_rms->GetXaxis()->SetLabelOffset(0.03);
    multi_slot2_rms->GetXaxis()->SetLabelSize(0.03);
    multi_slot2_rms->GetXaxis()->SetTimeFormat("#splitline{%m\/%d}{%H:%M}");
    //multi_slot2_rms->GetXaxis()->SetTimeOffset(0,"gmt");
    leg_slot2_rms->Draw();
    canvas_slot_freq->cd();
    multi_slot2_freq->Draw("apl");
    multi_slot2_freq->SetTitle("Crate 8 (6 hours)");
    multi_slot2_freq->GetYaxis()->SetTitle("Freq [1/min]");
    multi_slot2_freq->GetYaxis()->SetTitleSize(0.035);
    multi_slot2_freq->GetYaxis()->SetTitleOffset(1.3);
    multi_slot2_freq->GetXaxis()->SetTimeDisplay(1);
    multi_slot2_freq->GetXaxis()->SetLabelOffset(0.03);
    multi_slot2_freq->GetXaxis()->SetLabelSize(0.03);
    multi_slot2_freq->GetXaxis()->SetTimeFormat("#splitline{%m\/%d}{%H:%M}");
    //multi_slot2_freq->GetXaxis()->SetTimeOffset(0,"gmt");
    leg_slot2_freq->Draw();
/*
    gr_slot_times_sixhour.at(num_active_slots_cr1)->GetYaxis()->SetRangeUser(min_canvas,max_canvas);
    gr_slot_rms_sixhour.at(num_active_slots_cr1)->GetYaxis()->SetRangeUser(min_canvas_rms,max_canvas_rms);
    gr_slot_frequency_sixhour.at(num_active_slots_cr1)->GetYaxis()->SetRangeUser(min_canvas_freq,max_canvas_freq);
*/
    ss_slot.str("");
    ss_slot_rms.str("");
    ss_slot_freq.str("");
    ss_slot<<outpath<<"Crate8_Slots_6HourTrend_TDC.png";
    ss_slot_rms<<outpath<<"Crate8_Slots_6HourTrend_RMS.png";
    ss_slot_freq<<outpath<<"Crate8_Slots_6HourTrend_Freq.png";
    canvas_slot->SaveAs(ss_slot.str().c_str());
    canvas_slot_rms->SaveAs(ss_slot_rms.str().c_str());
    canvas_slot_freq->SaveAs(ss_slot_freq.str().c_str());

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
      //canvas_crate->cd();
      //gr_crate_times_sixhour.at(i_crate)->Draw();
      multi_crate->Add(gr_crate_times_sixhour.at(i_crate));
      leg_crate->AddEntry(gr_crate_times_sixhour.at(i_crate),"","l");
      if (gr_crate_times_sixhour.at(i_crate)->GetMaximum()>max_canvas) max_canvas = gr_crate_times_sixhour.at(i_crate)->GetMaximum();
      if (gr_crate_times_sixhour.at(i_crate)->GetMinimum()<min_canvas) min_canvas = gr_crate_times_sixhour.at(i_crate)->GetMinimum();
      //canvas_crate_rms->cd();
      //gr_crate_rms_sixhour.at(i_crate)->Draw();
      multi_crate_rms->Add(gr_crate_rms_sixhour.at(i_crate));
      leg_crate_rms->AddEntry(gr_crate_rms_sixhour.at(i_crate),"","l");
      if (gr_crate_rms_sixhour.at(i_crate)->GetMaximum()>max_canvas_rms) max_canvas_rms = gr_crate_rms_sixhour.at(i_crate)->GetMaximum();
      if (gr_crate_rms_sixhour.at(i_crate)->GetMinimum()<min_canvas_rms) min_canvas_rms = gr_crate_rms_sixhour.at(i_crate)->GetMinimum();
      //canvas_crate_freq->cd();
      //gr_crate_frequency_sixhour.at(i_crate)->Draw();
      multi_crate_freq->Add(gr_crate_frequency_sixhour.at(i_crate));
      leg_crate_freq->AddEntry(gr_crate_frequency_sixhour.at(i_crate),"","l");
      if (gr_crate_frequency_sixhour.at(i_crate)->GetMaximum()>max_canvas_freq) max_canvas_freq = gr_crate_frequency_sixhour.at(i_crate)->GetMaximum();
      if (gr_crate_frequency_sixhour.at(i_crate)->GetMinimum()<min_canvas_freq) min_canvas_freq = gr_crate_frequency_sixhour.at(i_crate)->GetMinimum();


    }

    canvas_crate->cd();
    multi_crate->Draw("apl");
    multi_crate->SetTitle("Crates (6 hours)");
    multi_crate->GetYaxis()->SetTitle("TDC");
    multi_crate->GetXaxis()->SetTimeDisplay(1);
    multi_crate->GetXaxis()->SetLabelOffset(0.03);
    multi_crate->GetXaxis()->SetLabelSize(0.03);
    multi_crate->GetXaxis()->SetTimeFormat("#splitline{%m\/%d}{%H:%M}");
    //multi_crate->GetXaxis()->SetTimeOffset(0,"gmt");
    leg_crate->Draw();
    canvas_crate_rms->cd();
    multi_crate_rms->Draw("apl");
    multi_crate_rms->SetTitle("Crates (6 hours)");
    multi_crate_rms->GetYaxis()->SetTitle("RMS (TDC)");
    multi_crate_rms->GetYaxis()->SetTitleSize(0.035);
    multi_crate_rms->GetYaxis()->SetTitleOffset(1.3);
    multi_crate_rms->GetXaxis()->SetTimeDisplay(1);
    multi_crate_rms->GetXaxis()->SetLabelOffset(0.03);
    multi_crate_rms->GetXaxis()->SetLabelSize(0.03);
    multi_crate_rms->GetXaxis()->SetTimeFormat("#splitline{%m\/%d}{%H:%M}");
   // multi_crate_rms->GetXaxis()->SetTimeOffset(0,"gmt");
    leg_crate_rms->Draw();
    canvas_crate_freq->cd();
    multi_crate_freq->Draw("apl");
    multi_crate_freq->SetTitle("Crates (6 hours)");
    multi_crate_freq->GetYaxis()->SetTitle("Freq [1/min]");
    multi_crate_freq->GetYaxis()->SetTitleSize(0.035);
    multi_crate_freq->GetYaxis()->SetTitleOffset(1.3);
    multi_crate_freq->GetXaxis()->SetTimeDisplay(1);
    multi_crate_freq->GetXaxis()->SetLabelSize(0.03);
    multi_crate_freq->GetXaxis()->SetLabelOffset(0.03);
    multi_crate_freq->GetXaxis()->SetTimeFormat("#splitline{%m\/%d}{%H:%M}");
    //multi_crate_freq->GetXaxis()->SetTimeOffset(0,"gmt");
    leg_crate_freq->Draw();

    std::stringstream ss_crate, ss_crate_rms, ss_crate_freq;
    ss_crate<<outpath<<"Crates_6HourTrend_TDC.png";
    ss_crate_rms<<outpath<<"Crates_6HourTrend_RMS.png";
    ss_crate_freq<<outpath<<"Crates_6HourTrend_Freq.png";
    canvas_crate->SaveAs(ss_crate.str().c_str());
    canvas_crate_rms->SaveAs(ss_crate_rms.str().c_str());
    canvas_crate_freq->SaveAs(ss_crate_freq.str().c_str());



  }


  if (update_hour){

    TLegend *leg;
    TLegend *leg_rms;
    TLegend *leg_freq;

    max_canvas = 0;
    min_canvas = 9999999.;
    max_canvas_rms = 0;
    min_canvas_rms = 99999999.;
    max_canvas_freq = 0;
    min_canvas_freq = 999999999.;

    TMultiGraph *multi_ch;
    TMultiGraph *multi_ch_rms;
    TMultiGraph *multi_ch_freq;

    int num_ch = 16;
    int enum_ch_canvas = 0;

 for (int i_channel =0;i_channel<times_channel_day.size();i_channel++){

    std::stringstream ss_ch, ss_ch_rms, ss_ch_freq;
    
    if (i_channel%num_ch == 0) {
      if (i_channel != 0){

        int num_crate = (i_channel>num_active_slots_cr1*num_channels)? min_crate+1 : min_crate;
//        int num_slot = (i_channel>num_active_slots_cr1*num_channels)? (i_channel-num_active_slots_cr1*num_channels)/num_channels+1 : i_channel/num_channels+1;
        int num_slot = (mapping_vector_ch.at(i_channel-1)-(i_channel-1)%num_channels-(num_crate-min_crate)*num_slots)/num_channels+1;

        ss_ch.str("");
        ss_ch_rms.str("");
        ss_ch_freq.str("");
        ss_ch<<"Crate "<<num_crate<<", Slot "<<num_slot<<" (day) ["<<enum_ch_canvas+1<<"]";
        ss_ch_rms<<"Crate "<<num_crate<<", Slot "<<num_slot<<" (day) ["<<enum_ch_canvas+1<<"]";
        ss_ch_freq<<"Crate "<<num_crate<<", Slot "<<num_slot<<" (day) ["<<enum_ch_canvas+1<<"]";


        canvas_ch->cd();
        multi_ch->Draw("apl");
        multi_ch->SetTitle(ss_ch.str().c_str());
        multi_ch->GetYaxis()->SetTitle("TDC");
        multi_ch->GetXaxis()->SetTimeDisplay(1);
        multi_ch->GetXaxis()->SetLabelOffset(0.03);
        multi_ch->GetXaxis()->SetLabelSize(0.03);
        multi_ch->GetXaxis()->SetTimeFormat("#splitline{%m\/%d}{%H:%M}");
        //multi_ch->GetXaxis()->SetTimeOffset(0,"gmt");
        leg->Draw();

        canvas_ch_rms->cd();
        multi_ch_rms->Draw("apl");
        multi_ch_rms->SetTitle(ss_ch_rms.str().c_str());
        multi_ch_rms->GetYaxis()->SetTitle("RMS (TDC)");
        multi_ch_rms->GetYaxis()->SetTitleSize(0.035);
        multi_ch_rms->GetYaxis()->SetTitleOffset(1.3);
        multi_ch_rms->GetXaxis()->SetTimeDisplay(1);
        multi_ch_rms->GetXaxis()->SetLabelSize(0.03);
        multi_ch_rms->GetXaxis()->SetLabelOffset(0.03);
        multi_ch_rms->GetXaxis()->SetTimeFormat("#splitline{%m\/%d}{%H:%M}");
        //multi_ch_rms->GetXaxis()->SetTimeOffset(0,"gmt");
        leg_rms->Draw();

        canvas_ch_freq->cd();
        multi_ch_freq->Draw("apl");
        multi_ch_freq->SetTitle(ss_ch_freq.str().c_str());
        multi_ch_freq->GetYaxis()->SetTitle("Freq [1/min]");
        multi_ch_freq->GetYaxis()->SetTitleSize(0.035);
        multi_ch_freq->GetYaxis()->SetTitleOffset(1.3);
        multi_ch_freq->GetXaxis()->SetTimeDisplay(1);
        multi_ch_freq->GetXaxis()->SetLabelSize(0.03);
        multi_ch_freq->GetXaxis()->SetLabelOffset(0.03);
        multi_ch_freq->GetXaxis()->SetTimeFormat("#splitline{%m\/%d}{%H:%M}");
        //multi_ch_freq->GetXaxis()->SetTimeOffset(0,"gmt");
        leg_freq->Draw();
/*
        gr_times_day.at(i_channel-1)->GetYaxis()->SetRangeUser(min_canvas, max_canvas);
        gr_rms_day.at(i_channel-1)->GetYaxis()->SetRangeUser(min_canvas_rms, max_canvas_rms);
        gr_frequency_day.at(i_channel-1)->GetYaxis()->SetRangeUser(min_canvas_freq, max_canvas_freq);
*/
        ss_ch.str("");
        ss_ch_rms.str("");
        ss_ch_freq.str("");
        ss_ch<<outpath<<"Crate"<<num_crate<<"_Slot"<<num_slot<<"_"<<enum_ch_canvas<<"_DayTrend_TDC.png";
        ss_ch_rms<<outpath<<"Crate"<<num_crate<<"_Slot"<<num_slot<<"_"<<enum_ch_canvas<<"_DayTrend_RMS.png";
        ss_ch_freq<<outpath<<"Crate"<<num_crate<<"_Slot"<<num_slot<<"_"<<enum_ch_canvas<<"_DayTrend_Freq.png";
        canvas_ch->SaveAs(ss_ch.str().c_str());
        canvas_ch_rms->SaveAs(ss_ch_rms.str().c_str());
        canvas_ch_freq->SaveAs(ss_ch_freq.str().c_str());   

        enum_ch_canvas++;
        enum_ch_canvas = enum_ch_canvas%2; 

      }
      
      canvas_ch->Clear();
      canvas_ch_rms->Clear();
      canvas_ch_freq->Clear();

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

      //canvas_ch->cd();
      //gr_times_day.at(i_channel)->Draw();
      ss_leg_ch.str("");
      ss_leg_ch<<"ch "<<i_channel%num_channels;
      multi_ch->Add(gr_times_day.at(i_channel));
      leg->AddEntry(gr_times_day.at(i_channel),ss_leg_ch.str().c_str(),"l");
      if (gr_times_day.at(i_channel)->GetMaximum()>max_canvas) max_canvas = gr_times_day.at(i_channel)->GetMaximum();
      if (gr_times_day.at(i_channel)->GetMinimum()<min_canvas) min_canvas = gr_times_day.at(i_channel)->GetMinimum();
      //canvas_ch_rms->cd();
      //gr_rms_day.at(i_channel)->Draw();
      multi_ch_rms->Add(gr_rms_day.at(i_channel));
      leg_rms->AddEntry(gr_rms_day.at(i_channel),ss_leg_ch.str().c_str(),"l");
      if (gr_rms_day.at(i_channel)->GetMaximum()>max_canvas_rms) max_canvas_rms = gr_rms_day.at(i_channel)->GetMaximum();
      if (gr_rms_day.at(i_channel)->GetMinimum()<min_canvas_rms) min_canvas_rms = gr_rms_day.at(i_channel)->GetMinimum();
      //canvas_ch_freq->cd();
      //gr_frequency_day.at(i_channel)->Draw();
      multi_ch_freq->Add(gr_frequency_day.at(i_channel));
      leg_freq->AddEntry(gr_frequency_day.at(i_channel),ss_leg_ch.str().c_str(),"l");
      if (gr_frequency_day.at(i_channel)->GetMaximum()>max_canvas_freq) max_canvas_freq = gr_frequency_day.at(i_channel)->GetMaximum();
      if (gr_frequency_day.at(i_channel)->GetMinimum()<min_canvas_freq) min_canvas_freq = gr_frequency_day.at(i_channel)->GetMinimum();

    } 

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
      
      //canvas_slot->cd();
      //gr_slot_times_day.at(i_slot)->Draw();
      multi_slot->Add(gr_slot_times_day.at(i_slot));
      leg_slot->AddEntry(gr_slot_times_day.at(i_slot),"","l");
      if (gr_slot_times_day.at(i_slot)->GetMaximum()>max_canvas) max_canvas = gr_slot_times_day.at(i_slot)->GetMaximum();
      if (gr_slot_times_day.at(i_slot)->GetMinimum()<min_canvas) min_canvas = gr_slot_times_day.at(i_slot)->GetMinimum();
      //canvas_slot_rms->cd();
      //gr_slot_rms_day.at(i_slot)->Draw();
      multi_slot_rms->Add(gr_slot_rms_day.at(i_slot));
      leg_slot_rms->AddEntry(gr_slot_rms_day.at(i_slot),"","l");
      if (gr_slot_rms_day.at(i_slot)->GetMaximum()>max_canvas_rms) max_canvas_rms = gr_slot_rms_day.at(i_slot)->GetMaximum();
      if (gr_slot_rms_day.at(i_slot)->GetMinimum()<min_canvas_rms) min_canvas_rms = gr_slot_rms_day.at(i_slot)->GetMinimum();
      //canvas_slot_freq->cd();
      //gr_slot_frequency_day.at(i_slot)->Draw();
      multi_slot_freq->Add(gr_slot_frequency_day.at(i_slot));
      leg_slot_freq->AddEntry(gr_slot_frequency_day.at(i_slot),"","l");
      if (gr_slot_frequency_day.at(i_slot)->GetMaximum()>max_canvas_freq) max_canvas_freq = gr_slot_frequency_day.at(i_slot)->GetMaximum();
      if (gr_slot_frequency_day.at(i_slot)->GetMinimum()<min_canvas_freq) min_canvas_freq = gr_slot_frequency_day.at(i_slot)->GetMinimum();


    }

    canvas_slot->cd();
    multi_slot->Draw("apl");
    multi_slot->SetTitle("Crate 7 (day)");
    multi_slot->GetYaxis()->SetTitle("TDC");
    multi_slot->GetXaxis()->SetTimeDisplay(1);
    multi_slot->GetXaxis()->SetLabelOffset(0.03);
    multi_slot->GetXaxis()->SetLabelSize(0.03);
    multi_slot->GetXaxis()->SetTimeFormat("#splitline{%m\/%d}{%H:%M}");
    //multi_slot->GetXaxis()->SetTimeOffset(0,"gmt");
    leg_slot->Draw();
    canvas_slot_rms->cd();
    multi_slot_rms->Draw("apl");
    multi_slot_rms->SetTitle("Crate 7 (day)");
    multi_slot_rms->GetYaxis()->SetTitleSize(0.035);
    multi_slot_rms->GetYaxis()->SetTitleOffset(1.3);
    multi_slot_rms->GetYaxis()->SetTitle("RMS (TDC)");
    multi_slot_rms->GetXaxis()->SetTimeDisplay(1);
    multi_slot_rms->GetXaxis()->SetLabelOffset(0.03);
    multi_slot_rms->GetXaxis()->SetLabelSize(0.03);
    multi_slot_rms->GetXaxis()->SetTimeFormat("#splitline{%m\/%d}{%H:%M}");
    //multi_slot_rms->GetXaxis()->SetTimeOffset(0,"gmt");
    leg_slot_rms->Draw();
    canvas_slot_freq->cd();
    multi_slot_freq->Draw("apl");
    multi_slot_freq->SetTitle("Crate 7 (day)");
    multi_slot_freq->GetYaxis()->SetTitle("Freq [1/min]");
    multi_slot_freq->GetYaxis()->SetTitleSize(0.035);
    multi_slot_freq->GetYaxis()->SetTitleOffset(1.3);
    multi_slot_freq->GetXaxis()->SetTimeDisplay(1);
    multi_slot_freq->GetXaxis()->SetLabelOffset(0.03);
    multi_slot_freq->GetXaxis()->SetLabelSize(0.03);
    multi_slot_freq->GetXaxis()->SetTimeFormat("#splitline{%m\/%d}{%H:%M}");
    //multi_slot_freq->GetXaxis()->SetTimeOffset(0,"gmt");
    leg_slot_freq->Draw();

    std::stringstream ss_slot, ss_slot_rms, ss_slot_freq;
    ss_slot<<outpath<<"Crate7_Slots_DayTrend_TDC.png";
    ss_slot_rms<<outpath<<"Crate7_Slots_DayTrend_RMS.png";
    ss_slot_freq<<outpath<<"Crate7_Slots_DayTrend_Freq.png";
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

      //canvas_slot->cd();
      //gr_slot_times_day.at(i_slot+num_active_slots_cr1)->Draw();
      multi_slot2->Add(gr_slot_times_day.at(i_slot+num_active_slots_cr1));
      leg_slot2->AddEntry(gr_slot_times_day.at(i_slot+num_active_slots_cr1),"","l");
      if (gr_slot_times_day.at(i_slot)->GetMaximum()>max_canvas) max_canvas = gr_slot_times_day.at(i_slot)->GetMaximum();
      if (gr_slot_times_day.at(i_slot)->GetMinimum()<min_canvas) min_canvas = gr_slot_times_day.at(i_slot)->GetMinimum();
      //canvas_slot_rms->cd();
      //gr_slot_rms_day.at(i_slot+num_active_slots_cr1)->Draw();
      multi_slot2_rms->Add(gr_slot_rms_day.at(i_slot+num_active_slots_cr1));
      leg_slot2_rms->AddEntry(gr_slot_rms_day.at(i_slot+num_active_slots_cr1),"","l");
      if (gr_slot_rms_day.at(i_slot)->GetMaximum()>max_canvas_rms) max_canvas_rms = gr_slot_rms_day.at(i_slot)->GetMaximum();
      if (gr_slot_rms_day.at(i_slot)->GetMinimum()<min_canvas_rms) min_canvas_rms = gr_slot_rms_day.at(i_slot)->GetMinimum();
      //canvas_slot_freq->cd();
      //gr_slot_frequency_day.at(i_slot+num_active_slots_cr1)->Draw();
      multi_slot2_freq->Add(gr_slot_frequency_day.at(i_slot+num_active_slots_cr1));
      leg_slot2_freq->AddEntry(gr_slot_frequency_day.at(i_slot+num_active_slots_cr1),"","l");
      if (gr_slot_frequency_day.at(i_slot)->GetMaximum()>max_canvas_freq) max_canvas_freq = gr_slot_frequency_day.at(i_slot)->GetMaximum();
      if (gr_slot_frequency_day.at(i_slot)->GetMinimum()<min_canvas_freq) min_canvas_freq = gr_slot_frequency_day.at(i_slot)->GetMinimum();

    }

    canvas_slot->cd();
    multi_slot2->Draw("apl");
    multi_slot2->SetTitle("Crate 8 (day)");
    multi_slot2->GetYaxis()->SetTitle("TDC");
    multi_slot2->GetXaxis()->SetTimeDisplay(1);
    multi_slot2->GetXaxis()->SetLabelSize(0.03);
    multi_slot2->GetXaxis()->SetLabelOffset(0.03);
    multi_slot2->GetXaxis()->SetTimeFormat("#splitline{%m\/%d}{%H:%M}");
    //multi_slot2->GetXaxis()->SetTimeOffset(0,"gmt");
    leg_slot2->Draw();
    canvas_slot_rms->cd();
    multi_slot2_rms->Draw("apl");
    multi_slot2_rms->SetTitle("Crate 8 (day)");
    multi_slot2_rms->GetYaxis()->SetTitleSize(0.035);
    multi_slot2_rms->GetYaxis()->SetTitleOffset(1.3);
    multi_slot2_rms->GetYaxis()->SetTitle("RMS (TDC)");
    multi_slot2_rms->GetXaxis()->SetTimeDisplay(1);
    multi_slot2_rms->GetXaxis()->SetLabelSize(0.03);
    multi_slot2_rms->GetXaxis()->SetLabelOffset(0.03);
    multi_slot2_rms->GetXaxis()->SetTimeFormat("#splitline{%m\/%d}{%H:%M}");
    //multi_slot2_rms->GetXaxis()->SetTimeOffset(0,"gmt");
    leg_slot2_rms->Draw();
    canvas_slot_freq->cd();
    multi_slot2_freq->Draw("apl");
    multi_slot2_freq->SetTitle("Crate 8 (day)");
    multi_slot2_freq->GetYaxis()->SetTitle("Freq [1/min]");
    multi_slot2_freq->GetYaxis()->SetTitleSize(0.035);
    multi_slot2_freq->GetYaxis()->SetTitleOffset(1.3);
    multi_slot2_freq->GetXaxis()->SetTimeDisplay(1);
    multi_slot2_freq->GetXaxis()->SetLabelSize(0.03);
    multi_slot2_freq->GetXaxis()->SetLabelOffset(0.03);
    multi_slot2_freq->GetXaxis()->SetTimeFormat("#splitline{%m\/%d}{%H:%M}");
    //multi_slot2_freq->GetXaxis()->SetTimeOffset(0,"gmt");
    leg_slot2_freq->Draw();

    ss_slot.str("");
    ss_slot_rms.str("");
    ss_slot_freq.str("");
    ss_slot<<outpath<<"Crate8_Slots_DayTrend_TDC.png";
    ss_slot_rms<<outpath<<"Crate8_Slots_DayTrend_RMS.png";
    ss_slot_freq<<outpath<<"Crate8_Slots_DayTrend_Freq.png";
    canvas_slot->SaveAs(ss_slot.str().c_str());
    canvas_slot_rms->SaveAs(ss_slot_rms.str().c_str());
    canvas_slot_freq->SaveAs(ss_slot_freq.str().c_str());

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

      //canvas_crate->cd();
      //gr_crate_times_day.at(i_crate)->Draw();
      multi_crate->Add(gr_crate_times_day.at(i_crate));
      leg_crate->AddEntry(gr_crate_times_day.at(i_crate),"","l");
      if (gr_crate_times_day.at(i_crate)->GetMaximum()>max_canvas) max_canvas = gr_crate_times_day.at(i_crate)->GetMaximum();
      if (gr_crate_times_day.at(i_crate)->GetMinimum()<min_canvas) min_canvas = gr_crate_times_day.at(i_crate)->GetMinimum();
      //canvas_crate_rms->cd();
      //gr_crate_rms_day.at(i_crate)->Draw();
      multi_crate_rms->Add(gr_crate_rms_day.at(i_crate));
      leg_crate_rms->AddEntry(gr_crate_rms_day.at(i_crate),"","l");
      if (gr_crate_rms_day.at(i_crate)->GetMaximum()>max_canvas_rms) max_canvas_rms = gr_crate_rms_day.at(i_crate)->GetMaximum();
      if (gr_crate_rms_day.at(i_crate)->GetMinimum()<min_canvas_rms) min_canvas_rms = gr_crate_rms_day.at(i_crate)->GetMinimum();
      //canvas_crate_freq->cd();
      //gr_crate_frequency_day.at(i_crate)->Draw();
      multi_crate_freq->Add(gr_crate_frequency_day.at(i_crate));
      leg_crate_freq->AddEntry(gr_crate_frequency_day.at(i_crate),"","l");
      if (gr_crate_frequency_day.at(i_crate)->GetMaximum()>max_canvas_freq) max_canvas_freq = gr_crate_frequency_day.at(i_crate)->GetMaximum();
      if (gr_crate_frequency_day.at(i_crate)->GetMinimum()<min_canvas_freq) min_canvas_freq = gr_crate_frequency_day.at(i_crate)->GetMinimum();

    }

    canvas_crate->cd();
    multi_crate->Draw("apl");
    multi_crate->SetTitle("Crates (day)");
    multi_crate->GetYaxis()->SetTitle("TDC");
    multi_crate->GetXaxis()->SetTimeDisplay(1);
    multi_crate->GetXaxis()->SetLabelOffset(0.03);
    multi_crate->GetXaxis()->SetLabelSize(0.03);
    multi_crate->GetXaxis()->SetTimeFormat("#splitline{%m\/%d}{%H:%M}");
    //multi_crate->GetXaxis()->SetTimeOffset(0,"gmt");
    leg_crate->Draw();
    canvas_crate_rms->cd();
    multi_crate_rms->Draw("apl");
    multi_crate_rms->SetTitle("Crates (day)");
    multi_crate_rms->GetYaxis()->SetTitleSize(0.035);
    multi_crate_rms->GetYaxis()->SetTitleOffset(1.3);
    multi_crate_rms->GetYaxis()->SetTitle("RMS (TDC)");
    multi_crate_rms->GetXaxis()->SetTimeDisplay(1);
    multi_crate_rms->GetXaxis()->SetLabelOffset(0.03);
    multi_crate_rms->GetXaxis()->SetLabelSize(0.03);
    multi_crate_rms->GetXaxis()->SetTimeFormat("#splitline{%m\/%d}{%H:%M}");
    //multi_crate_rms->GetXaxis()->SetTimeOffset(0,"gmt");
    leg_crate_rms->Draw();
    canvas_crate_freq->cd();
    multi_crate_freq->Draw("apl");
    multi_crate_freq->SetTitle("Crates (day)");
    multi_crate_freq->GetYaxis()->SetTitle("RMS (TDC)");
    multi_crate_freq->GetYaxis()->SetTitleSize(0.035);
    multi_crate_freq->GetYaxis()->SetTitleOffset(1.3);
    multi_crate_freq->GetXaxis()->SetTimeDisplay(1);
    multi_crate_freq->GetXaxis()->SetLabelOffset(0.03);
    multi_crate_freq->GetXaxis()->SetLabelSize(0.03);
    multi_crate_freq->GetXaxis()->SetTimeFormat("#splitline{%m\/%d}{%H:%M}");
    multi_crate_freq->GetYaxis()->SetTitle("Freq [1/min]");
    leg_crate_freq->Draw();

    std::stringstream ss_crate, ss_crate_rms, ss_crate_freq;
    ss_crate<<outpath<<"Crates_DayTrend_TDC.png";
    ss_crate_rms<<outpath<<"Crates_DayTrend_RMS.png";
    ss_crate_freq<<outpath<<"Crates_DayTrend_Freq.png";
    canvas_crate->SaveAs(ss_crate.str().c_str());
    canvas_crate_rms->SaveAs(ss_crate_rms.str().c_str());
    canvas_crate_freq->SaveAs(ss_crate_freq.str().c_str());

  }

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

}

void MonitorMRDTime::UpdateMonitorSources(){

	//shift all entries by 1 to the left & insert newest event (live) as the last entry of all arrays in the vectors

	update_mins = true;

  if (verbosity > 2) std::cout <<"Update MonitorSources...."<<std::endl;
  //std::cout <<"UpdateMonitorSources: Data available: "<<data_available<<std::endl;
	
		for (int i_channel = 0; i_channel<num_channels*num_active_slots; i_channel++){


			for (int i_mins = 0;i_mins<num_fivemin-1;i_mins++){
				times_channel_hour.at(i_channel)[i_mins] = times_channel_hour.at(i_channel)[i_mins+1];
				rms_channel_hour.at(i_channel)[i_mins] = rms_channel_hour.at(i_channel)[i_mins+1];
				frequency_channel_hour.at(i_channel)[i_mins] = frequency_channel_hour.at(i_channel)[i_mins+1];
        		n_channel_hour.at(i_channel)[i_mins] = n_channel_hour.at(i_channel)[i_mins+1];
			}

			if (!data_available || live_mins.at(i_channel).empty()){
			times_channel_hour.at(i_channel)[num_fivemin-1] = 0;
			rms_channel_hour.at(i_channel)[num_fivemin-1] = 0;
			frequency_channel_hour.at(i_channel)[num_fivemin-1] = 0;
      		n_channel_hour.at(i_channel)[num_fivemin-1] = 0;
			} else {
				times_channel_hour.at(i_channel)[num_fivemin-1] = std::accumulate(live_mins.at(i_channel).begin(),live_mins.at(i_channel).end(),0.0)/live_mins.at(i_channel).size();
				rms_channel_hour.at(i_channel)[num_fivemin-1] = compute_variance(times_channel_hour.at(i_channel)[num_fivemin-1],live_mins.at(i_channel));
				frequency_channel_hour.at(i_channel)[num_fivemin-1] = live_mins.at(i_channel).size()/duration_fivemin;
        		n_channel_hour.at(i_channel)[num_fivemin-1] = live_mins.at(i_channel).size();
        		//std::cout <<"Filling tdc "<<std::accumulate(live_mins.at(i_channel).begin(),live_mins.at(i_channel).end(),0.0)/live_mins.at(i_channel).size()<<", rms: "<<compute_variance(times_channel_hour.at(i_channel)[num_fivemin-1],live_mins.at(i_channel))<<", freq: "<<live_mins.at(i_channel).size()/duration_fivemin<<", size: "<<live_mins.at(i_channel).size()<<std::endl;
				live_mins.at(i_channel).clear();
			}

			if (j_fivemin%6 ==0 && !initial){
        if (verbosity > 3) std::cout <<"Updating 6h plots!"<<std::endl;
				update_halfhour=true;
				for (int i_halfhour = 0; i_halfhour < num_halfhour-1; i_halfhour++){
					times_channel_sixhour.at(i_channel)[i_halfhour] = times_channel_sixhour.at(i_channel)[i_halfhour+1];
					rms_channel_sixhour.at(i_channel)[i_halfhour] = rms_channel_sixhour.at(i_channel)[i_halfhour+1];
					frequency_channel_sixhour.at(i_channel)[i_halfhour] = frequency_channel_sixhour.at(i_channel)[i_halfhour+1];
          			n_channel_sixhour.at(i_channel)[i_halfhour] = n_channel_sixhour.at(i_channel)[i_halfhour+1];
				}
				if (live_halfhour.at(i_channel).empty()){
          if (verbosity > 3) std::cout <<"live vector empty for channel "<<i_channel<<std::endl;
				times_channel_sixhour.at(i_channel)[num_halfhour-1] = 0;
				rms_channel_sixhour.at(i_channel)[num_halfhour-1] = 0;
				frequency_channel_sixhour.at(i_channel)[num_halfhour-1] = 0;
        		n_channel_sixhour.at(i_channel)[num_halfhour-1] = 0;
				} else {
				times_channel_sixhour.at(i_channel)[num_halfhour-1] = std::accumulate(live_halfhour.at(i_channel).begin(),live_halfhour.at(i_channel).end(),0.0)/live_halfhour.at(i_channel).size();
				rms_channel_sixhour.at(i_channel)[num_halfhour-1] = compute_variance(times_channel_sixhour.at(i_channel)[num_halfhour-1],live_halfhour.at(i_channel));
				frequency_channel_sixhour.at(i_channel)[num_halfhour-1] = live_halfhour.at(i_channel).size()/duration_halfhour;
        		n_channel_sixhour.at(i_channel)[num_halfhour-1] = live_halfhour.at(i_channel).size();
        //std::cout << " Filling tdc "<< std::accumulate(live_halfhour.at(i_channel).begin(),live_halfhour.at(i_channel).end(),0.0)/live_halfhour.at(i_channel).size()<<", rms: "<<compute_variance(times_channel_sixhour.at(i_channel)[num_halfhour-1],live_halfhour.at(i_channel))<<", freq: "<<live_halfhour.at(i_channel).size()/duration_fivemin<<", size: "<<live_halfhour.at(i_channel).size()<<std::endl;
				live_halfhour.at(i_channel).clear();

				}
			}

			if (j_fivemin%12 == 0 && !initial){
        		if (verbosity > 3) std::cout <<"Updating 24h plots!"<<std::endl;
				update_hour=true;
				for (int i_hour = 0;i_hour<num_hour-1;i_hour++){
					times_channel_day.at(i_channel)[i_hour] = times_channel_day.at(i_channel)[i_hour+1];
					rms_channel_day.at(i_channel)[i_hour] = rms_channel_day.at(i_channel)[i_hour+1];
					frequency_channel_day.at(i_channel)[i_hour] = frequency_channel_day.at(i_channel)[i_hour+1];
          			n_channel_day.at(i_channel)[i_hour] = n_channel_day.at(i_channel)[i_hour+1];
				}
				if (live_hour.at(i_channel).empty()){
          			if (verbosity > 3) std::cout <<"live vector empty for channel "<<i_channel<<std::endl;
				times_channel_day.at(i_channel)[num_hour-1] = 0;
				rms_channel_day.at(i_channel)[num_hour-1] = 0;
				frequency_channel_day.at(i_channel)[num_hour-1] = 0;
        		n_channel_day.at(i_channel)[num_hour-1] = 0;
				}else {
				times_channel_day.at(i_channel)[num_hour-1] = std::accumulate(live_hour.at(i_channel).begin(),live_hour.at(i_channel).end(),0.0)/live_hour.at(i_channel).size();
				rms_channel_day.at(i_channel)[num_hour-1] = compute_variance(times_channel_day.at(i_channel)[num_hour-1],live_hour.at(i_channel));
				frequency_channel_day.at(i_channel)[num_hour-1] = live_hour.at(i_channel).size()/duration_hour;
        		n_channel_day.at(i_channel)[num_hour-1] = live_hour.at(i_channel).size();
				live_hour.at(i_channel).clear();
				}
			}
		}

    //checking the live file vector for any other entries that would need to be added
    for (int i_channel = 0; i_channel < num_channels*num_active_slots;i_channel++){
      for (int i_live = 0; i_live < live_file.at(i_channel).size(); i_live++){
        if (timediff_file.at(i_channel).at(i_live) > 5*60*1000.){           //timediff bigger than 5 mins?  --> fill in 1h graph
          int bin = timediff_file.at(i_channel).at(i_live)/int((5*60*1000));
          if (i_live == 0 && verbosity > 3) std::cout <<"time diff: "<<timediff_file.at(i_channel).at(i_live)<<", h: "<<int(timediff_file.at(i_channel).at(i_live)/(3600*10000))%24<<", min: "<<int(timediff_file.at(i_channel).at(i_live)/(60*10000))%60<<", bin: "<<bin<<", index: "<<num_fivemin-1-bin<<std::endl;
          if (bin < 12){
            //std::cout <<">5mins, bin nr :"<<bin;
            //std::cout<<", times channel ["<<num_fivemin-1-bin<<"]: "<< (times_channel_hour.at(i_channel)[num_fivemin-1-bin]*n_channel_hour.at(i_channel)[num_fivemin-1-bin]+live_file.at(i_channel).at(i_live))/(n_channel_hour.at(i_channel)[num_fivemin-1-bin]+1)<<std::endl;
            times_channel_hour.at(i_channel)[num_fivemin-1-bin] = (times_channel_hour.at(i_channel)[num_fivemin-1-bin]*n_channel_hour.at(i_channel)[num_fivemin-1-bin]+live_file.at(i_channel).at(i_live))/(n_channel_hour.at(i_channel)[num_fivemin-1-bin]+1);
            rms_channel_hour.at(i_channel)[num_fivemin-1-bin] = (rms_channel_hour.at(i_channel)[num_fivemin-1-bin]*(n_channel_hour.at(i_channel)[num_fivemin-1-bin]-1)+pow(live_file.at(i_channel).at(i_live)-times_channel_hour.at(i_channel)[num_fivemin-1-bin],2))/(n_channel_hour.at(i_channel)[num_fivemin-1-bin]+1);
            frequency_channel_hour.at(i_channel)[num_fivemin-1-bin] = (frequency_channel_hour.at(i_channel)[num_fivemin-1-bin]+1./duration_fivemin);
            n_channel_hour.at(i_channel)[num_fivemin-1-bin]+=1;
          }


          if (timediff_file.at(i_channel).at(i_live) > ((j_fivemin-1)%6+1)*5*60*1000.){         //timediff bigger than 30 mins? --> also fill in 6h graph
          	  
              int bin = (timediff_file.at(i_channel).at(i_live)-((j_fivemin-1)%6+1)*5*60*1000.)/int(30*60*1000);
              if (j_fivemin%6==0) bin++;
              if (bin < 12){
                  //std::cout <<">30mins, bin nr: "<<bin;
              	  //std::cout <<"SIX HOUR array: Multiple of 6: "<<j_fivemin%6;
                  //std::cout<<", times channel ["<<num_halfhour-1-bin<<"]: "<< (times_channel_sixhour.at(i_channel)[num_halfhour-1-bin]*n_channel_sixhour.at(i_channel)[num_halfhour-1-bin]+live_file.at(i_channel).at(i_live))/(n_channel_sixhour.at(i_channel)[num_halfhour-1-bin]+1)<<std::endl;
                  times_channel_sixhour.at(i_channel)[num_halfhour-1-bin] = (times_channel_sixhour.at(i_channel)[num_halfhour-1-bin]*n_channel_sixhour.at(i_channel)[num_halfhour-1-bin]+live_file.at(i_channel).at(i_live))/(n_channel_sixhour.at(i_channel)[num_halfhour-1-bin]+1);
                  rms_channel_sixhour.at(i_channel)[num_halfhour-1-bin] = (rms_channel_sixhour.at(i_channel)[num_halfhour-1-bin]*(n_channel_sixhour.at(i_channel)[num_halfhour-1-bin]-1)+pow(live_file.at(i_channel).at(i_live)-times_channel_sixhour.at(i_channel)[num_halfhour-1-bin],2))/(n_channel_sixhour.at(i_channel)[num_halfhour-1-bin]+1);
                  frequency_channel_sixhour.at(i_channel)[num_halfhour-1-bin] = (frequency_channel_sixhour.at(i_channel)[num_halfhour-1-bin]+1./duration_halfhour);
                  n_channel_sixhour.at(i_channel)[num_halfhour-1-bin]+=1;
              }


            if (timediff_file.at(i_channel).at(i_live) > (j_fivemin)*5*60*1000.){        //timediff bigger than 60 mins? --> also fill in 24h graph

              int bin = (timediff_file.at(i_channel).at(i_live)-(j_fivemin)*5*60*1000.)/int(60*60*1000.);
              if (j_fivemin%12==0) bin++;
              if (bin<24){
                  //std::cout <<">60mins, bin nr: "<<bin;
              	  //std::cout <<"DAY array: Multiple of 12: "<<j_fivemin%12;
                  //std::cout<<", times channel ["<<num_hour-1-bin<<"]: "<< (times_channel_day.at(i_channel)[num_hour-1-bin]*n_channel_day.at(i_channel)[num_hour-1-bin]+live_file.at(i_channel).at(i_live))/(n_channel_day.at(i_channel)[num_hour-1-bin]+1)<<std::endl;
                  times_channel_day.at(i_channel)[num_hour-1-bin] = (times_channel_day.at(i_channel)[num_hour-1-bin]*n_channel_day.at(i_channel)[num_hour-1-bin]+live_file.at(i_channel).at(i_live))/(n_channel_day.at(i_channel)[num_hour-1-bin]+1);
                  rms_channel_day.at(i_channel)[num_hour-1-bin] = (rms_channel_day.at(i_channel)[num_hour-1-bin]*(n_channel_day.at(i_channel)[num_hour-1-bin]-1)+pow(live_file.at(i_channel).at(i_live)-times_channel_day.at(i_channel)[num_hour-1-bin],2))/(n_channel_day.at(i_channel)[num_hour-1-bin]+1);
                  frequency_channel_day.at(i_channel)[num_hour-1-bin] = (frequency_channel_day.at(i_channel)[num_hour-1-bin]+1./duration_hour);
                  n_channel_day.at(i_channel)[num_hour-1-bin]+=1;
              }

            }

          }

        }

      }

    }


    // std::cout <<"Update slot vectors..."<<std::endl;

    //fill slot vectors

    for (int i_slot = 0; i_slot< num_active_slots; i_slot++){
      //std::cout <<"slot: "<<i_slot;
      for (int i_mins = 0;i_mins<num_fivemin;i_mins++){
        //std::cout <<", i_mins: "<<i_mins;
        times_slot_hour.at(i_slot)[i_mins] = accumulate_array12(times_channel_hour,i_slot*num_channels,(i_slot+1)*num_channels-1,i_mins);
        //std::cout <<", filling tdc: "<<accumulate_array12(times_channel_hour,i_slot*num_channels,(i_slot+1)*num_channels-1,i_mins)<<std::endl;
        rms_slot_hour.at(i_slot)[i_mins] = accumulate_array12(rms_channel_hour,i_slot*num_channels,(i_slot+1)*num_channels-1,i_mins);
        frequency_slot_hour.at(i_slot)[i_mins] = accumulate_array12(frequency_channel_hour,i_slot*num_channels,(i_slot+1)*num_channels-1,i_mins);
        n_slot_hour.at(i_slot)[i_mins] = accumulate_longarray12(n_channel_hour,i_slot*num_channels,(i_slot+1)*num_channels-1,i_mins);
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

    //std::cout <<"Update crate vectors..."<<std::endl;

    //fill crate vectors

    for (int i_crate = 0; i_crate< num_crates; i_crate++){
      int slot1 = (i_crate == 0)? num_active_slots_cr1 : num_active_slots;
      for (int i_mins = 0;i_mins<num_fivemin;i_mins++){
        times_crate_hour.at(i_crate)[i_mins] = accumulate_array12(times_channel_hour,i_crate*num_active_slots_cr1*num_channels,slot1*num_channels-1,i_mins);
        rms_crate_hour.at(i_crate)[i_mins] = accumulate_array12(rms_channel_hour,i_crate*num_active_slots_cr1*num_channels,slot1*num_channels-1,i_mins);
        frequency_crate_hour.at(i_crate)[i_mins] = accumulate_array12(frequency_channel_hour,i_crate*num_active_slots_cr1*num_channels,slot1*num_channels-1,i_mins);
        n_crate_hour.at(i_crate)[i_mins] = accumulate_longarray12(n_channel_hour,i_crate*num_active_slots_cr1*num_channels,slot1*num_channels-1,i_mins);
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

  for (int i_channel = 0; i_channel < num_channels*(num_active_slots); i_channel++){
    //clearing live file vector
    live_file.at(i_channel).clear();
    timestamp_file.at(i_channel).clear();
    timediff_file.at(i_channel).clear();
    n_live_file.at(i_channel).clear();
  }

  //update the axis labels

    boost::posix_time::ptime current(boost::posix_time::second_clock::local_time());

    for (int i_label = 0; i_label < 12; i_label++){

      boost::posix_time::time_duration offset_label(0,i_label*5,0,0);   //multiples of 5 mins
      boost::posix_time::ptime labeltime = current - offset_label;
      struct tm label_tm = boost::posix_time::to_tm(labeltime);
      //std::cout <<1900+label_tm.tm_year<<", "<<label_tm.tm_mon +1<<", "<<label_tm.tm_mday<<", "<<label_tm.tm_hour<<", "<<label_tm.tm_min<<", "<<label_tm.tm_sec<<std::endl;
      TDatime label_date(1900+label_tm.tm_year,label_tm.tm_mon +1,label_tm.tm_mday,label_tm.tm_hour,label_tm.tm_min,label_tm.tm_sec);
      if (verbosity > 2) std::cout <<"i_label: "<<i_label<<", label_date: "<<boost::posix_time::to_simple_string(labeltime)<<std::endl;
      //std::string title_string = to_simple_string(labeltime); 
      //std::cout <<"calculated time: "<<labeltime<<std::endl;
      //TDatime label_date(2005,1,4,8,32,15);
      //label_fivemin[11-i_label] = title_string;       //labels that are to be displayed on the x-axis (times)
      label_fivemin[11-i_label] = label_date;
    }
 
  if (update_halfhour){
  
    for (int i_label = 0; i_label < 12; i_label++){

      boost::posix_time::time_duration offset_label(0,i_label*30,0,0);   //multiples of 30 mins
      boost::posix_time::ptime labeltime = current - offset_label;
      struct tm label_tm = boost::posix_time::to_tm(labeltime);
      TDatime label_date(1900+label_tm.tm_year,label_tm.tm_mon +1,label_tm.tm_mday,label_tm.tm_hour,label_tm.tm_min,label_tm.tm_sec);
      //std::string title_string = to_simple_string(labeltime); 
      //std::cout <<"i_label: "<<i_label<<", string title_string: "<<title_string<<std::endl;
      label_halfhour[11-i_label] = label_date;       //labels that are to be displayed on the x-axis (times)
    }

  }

  if (update_hour){

    for (int i_label = 0; i_label < 24; i_label++){

      boost::posix_time::time_duration offset_label(i_label,0,0,0);   //multiples of 1 hours
      boost::posix_time::ptime labeltime = current - offset_label;
      struct tm label_tm = boost::posix_time::to_tm(labeltime);
      TDatime label_date(1900+label_tm.tm_year,label_tm.tm_mon +1,label_tm.tm_mday,label_tm.tm_hour,label_tm.tm_min,label_tm.tm_sec);
      //std::string title_string = to_simple_string(labeltime); 
      //std::cout <<"i_label: "<<i_label<<", string title_string: "<<title_string<<std::endl;
      label_hour[23-i_label] = label_date;       //labels that are to be displayed on the x-axis (times)
    }

  }
	
	//plot the newly created vectors
	MonitorMRDTime::MRDTimePlots();

}

void MonitorMRDTime::FillEvents(){


  int total_number_entries;

	//MRDdata->Header->Get("TotalEntries",total_number_entries);
  
  MRDdata->Header->Get("TotalEntries",total_number_entries);

  //print store information!!!

  //TH1F *channels = new TH1F("channels","Channel distribution",1000,1,0);

  //std::cout <<"MRDdata store information: "<<std::endl;
  //RDdata->Print();

  //TEST: setting number of events to 1000 (should be working according to MonitorSimReceive)
  //total_number_entries = 1000;

  //std::cout <<"MRDdata number of events: "<<total_number_entries<<std::endl;
  int count=0;
	for (int i_event = 0; i_event <  total_number_entries; i_event++){

	MRDdata->GetEntry(i_event);

    MRDdata->Get("Data",MRDout);

    boost::posix_time::ptime current(boost::posix_time::second_clock::local_time());
    boost::posix_time::ptime eventtime;
    ULong64_t timestamp = MRDout.TimeStamp;
    timestamp+=offset_date;

    eventtime = *Epoch + boost::posix_time::time_duration(int(timestamp/1000./60./60.),int(timestamp/1000./60.)%60,int(timestamp/1000.)%60,timestamp%1000);   //default last entry in miliseconds, need special compilation option for nanosec hh:mm:ss:msms
    if (verbosity > 2 && count == 0) std::cout <<"Starting from /1/08/1970, this results in the date: "<<eventtime.date()<<", with the time: "<<eventtime.time_of_day()<<std::endl;

    boost::posix_time::time_duration dt = eventtime - current;
    double dt_ms = -(dt.total_milliseconds());

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


      int ch = active_slot_nr*num_channels+MRDout.Channel.at(i_entry);
       
      live_file.at(ch).push_back(MRDout.Value.at(i_entry));
      timestamp_file.at(ch).push_back(MRDout.TimeStamp);
      timediff_file.at(ch).push_back(dt_ms);         //in msecs,

      //if (dt_ms<(j_fivemin%12+1)*5*60*1000){
       if (dt_ms<(j_fivemin*5*60*1000+duration.total_milliseconds())){
        if (verbosity > 3) std::cout <<"filling hour vector with "<<MRDout.Value.at(i_entry)<<"..."<<std::endl;
        live_hour.at(ch).push_back(MRDout.Value.at(i_entry));   //save newest entries for all live vectors
        timestamp_hour.at(ch).push_back(timestamp);         //save corresponding timestamps to the events
        //if (dt_ms<((j_fivemin%6+1)*5*60*1000)){
          if (dt_ms<((j_fivemin%6)*5*60*10000+duration.total_milliseconds())){
          if (verbosity > 3) std::cout <<"filling half hour vector with "<<MRDout.Value.at(i_entry)<<"..."<<std::endl;
          live_halfhour.at(ch).push_back(MRDout.Value.at(i_entry));
          timestamp_halfhour.at(ch).push_back(timestamp);
          //if (dt_ms<5*60*1000){
          if (dt_ms<duration.total_milliseconds()){
            if (verbosity > 3) std::cout <<"filling 5mins vector with "<<MRDout.Value.at(i_entry)<<"..."<<std::endl;
            live_mins.at(ch).push_back(MRDout.Value.at(i_entry));
            timestamp_mins.at(ch).push_back(timestamp);
          }
        }
      }
      
    }

	MRDout.Value.clear();
    MRDout.Slot.clear();
    MRDout.Channel.clear();
    MRDout.Crate.clear();
    MRDout.Type.clear();

	}

}