#include "MonitorLAPPDSC.h"

MonitorLAPPDSC::MonitorLAPPDSC() :
		Tool() {
}

bool MonitorLAPPDSC::Initialise(std::string configfile, DataModel &data) {

	/////////////////// Useful header ///////////////////////
	if (configfile != "")
		m_variables.Initialise(configfile); // loading config file
	//m_variables.Print();

	m_data = &data; //assigning transient data pointer
	/////////////////////////////////////////////////////////////////

	//gObjectTable only for debugging memory leaks, otherwise comment out
	//std::cout <<"MonitorLAPPDSC: List of Objects (beginning of Initialise): "<<std::endl;
	//gObjectTable->Print();

	//-------------------------------------------------------
	//-----------------Get Configuration---------------------
	//-------------------------------------------------------

	update_frequency = 0.;
	limit_salt_low = 500000;	// no idea what the units are, probably non-sense default limits?
	limit_salt_high = 400000;	//no idea what the units are, probably non-sense default limits?
  	limit_temperature_low = 50.;
	limit_temperature_high = 58.;
	limit_humidity_low = 15.;
	limit_humidity_high = 20.;
	limit_thermistor_temperature_low = 7000.;
	limit_thermistor_temperature_high = 5800.;

	m_variables.Get("OutputPath", outpath_temp);
	m_variables.Get("StartTime", StartTime);
	m_variables.Get("UpdateFrequency", update_frequency);
	m_variables.Get("PathMonitoring", path_monitoring);
	m_variables.Get("PlotConfiguration", plot_configuration);
	m_variables.Get("ImageFormat", img_extension);
	m_variables.Get("ForceUpdate", force_update);
	m_variables.Get("DrawMarker", draw_marker);
	m_variables.Get("verbose", verbosity);
	m_variables.Get("VoltageMin33", v33_min);
	m_variables.Get("VoltageMax33", v33_max);
	m_variables.Get("VoltageMin25", v25_min);
	m_variables.Get("VoltageMax25", v25_max);
	m_variables.Get("VoltageMin12", v12_min);
	m_variables.Get("VoltageMax12", v12_max);
	m_variables.Get("LimitSaltLow", limit_salt_low);
	m_variables.Get("LimitSaltHigh", limit_salt_high);
	m_variables.Get("LimitTempLow", limit_temperature_low);
	m_variables.Get("LimitTempHigh", limit_temperature_high);
	m_variables.Get("LimitHumLow", limit_humidity_low);
	m_variables.Get("LimitHumHigh", limit_humidity_high);
	m_variables.Get("LimitThermistorLow", limit_thermistor_temperature_low);
	m_variables.Get("LimitThermistorHigh", limit_thermistor_temperature_high);
	m_variables.Get("LAPPDIDFile", lappd_id_file);

	if (verbosity > 2) {
		std::cout << "v33_min " << v33_min << std::endl;
		std::cout << "v33_max " << v33_max << std::endl;
		std::cout << "v25_min " << v25_min << std::endl;
		std::cout << "v25_max " << v25_max << std::endl;
		std::cout << "v12_min " << v12_min << std::endl;
		std::cout << "v12_max " << v12_max << std::endl;
		std::cout << "limit_salt_low: " << limit_salt_low << std::endl;
		std::cout << "limit_salt_high: " << limit_salt_high << std::endl;
		std::cout << "limit_temperature_low: " << limit_temperature_low << std::endl;
		std::cout << "limit_temperature_high: " << limit_temperature_high << std::endl;
		std::cout << "limit_humidity_low: "<< limit_humidity_low << std::endl;
		std::cout << "limit_humidity_high: "<< limit_humidity_high << std::endl;
		std::cout << "limit_thermistor_temperature_low: "<< limit_thermistor_temperature_low << std::endl;
		std::cout << "limit_thermistor_temperature_high: "<< limit_thermistor_temperature_high << std::endl;
	}

	if (verbosity > 1)
		std::cout << "Tool MonitorLAPPDSC: Initialising...." << std::endl;
	// Update frequency specifies the frequency at which the File Log Histogram is updated
	// All other monitor plots are updated as soon as a new file is available for readout
/*	if (update_frequency < 0.1) {
		if (verbosity > 0)
			std::cout << "MonitorLAPPDSC: Update Frequency of " << update_frequency << " mins is too low. Setting default value of 1 min." << std::endl;
		update_frequency = 1.;
	}*/

	//default should be no forced update of the monitoring plots every execute step
	if (force_update != 0 && force_update != 1) {
		force_update = 0;
	}

	//check if the image format is jpg or png
	if (!(img_extension == "png" || img_extension == "jpg" || img_extension == "jpeg")) {
		img_extension = "jpg";
	}

	//Print out path to monitoring files
	std::cout << "PathMonitoring: " << path_monitoring << std::endl;

	//Read in the expected LAPPD IDs
	ifstream file_lappdid(lappd_id_file.c_str());
	int temp_id;
	while (!file_lappdid.eof()){
		file_lappdid >> temp_id;
		if (std::find(vector_lappd_id.begin(),vector_lappd_id.end(),temp_id) == vector_lappd_id.end())	vector_lappd_id.push_back(temp_id);
		std::cout <<"MonitorLAPPDSC: Register LAPPD ID: "<<temp_id<<std::endl;
		if (file_lappdid.eof()) break;
	}
	file_lappdid.close();
	
	m_data->CStore.Set("VectorLAPPDID",vector_lappd_id);

	//Set up Epoch
	Epoch = new boost::posix_time::ptime(boost::gregorian::from_string(StartTime));

	//Set default value for last update time
	t_last_update = 0;

	//Status variable for slack messages in MonitorDAQ
         m_data->CStore.Set("LAPPDSlowControlWarning",false);
         m_data->CStore.Set("LAPPDSCWarningTemp",false);
         m_data->CStore.Set("LAPPDSCWarningHum",false);
         m_data->CStore.Set("LAPPDSCWarningHV",false);
         m_data->CStore.Set("LAPPDSCWarningLV1",false);
         m_data->CStore.Set("LAPPDSCWarningLV2",false);
         m_data->CStore.Set("LAPPDSCWarningLV3",false);
         m_data->CStore.Set("LAPPDSCWarningSalt",false);
         m_data->CStore.Set("LAPPDSCWarningThermistor",false);
         m_data->CStore.Set("LAPPDSCWarningLight",false);
         m_data->CStore.Set("LAPPDSCWarningRelay",false);
         m_data->CStore.Set("LAPPDSCWarningErrors",false);

	//Evaluating output path for monitoring plots
	if (outpath_temp == "fromStore")
		m_data->CStore.Get("OutPath", outpath);
	else
		outpath = outpath_temp;
	if (verbosity > 2)
		std::cout << "MonitorLAPPDSC: Output path for plots is " << outpath << std::endl;

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

	period_update = boost::posix_time::time_duration(0, int(update_frequency), 0, 0);
	period_warning = boost::posix_time::time_duration(0, 10, 0, 0);
	last = boost::posix_time::ptime(boost::posix_time::second_clock::local_time());
        current = (boost::posix_time::second_clock::local_time());

	duration = boost::posix_time::time_duration(current-last);

	for (int i_id=0; i_id < (int) vector_lappd_id.size(); i_id++){
		map_last.emplace(vector_lappd_id.at(i_id),last);
		map_duration.emplace(vector_lappd_id.at(i_id),duration);
	}

	// Omit warning messages from ROOT: 1001 - info messages, 2001 - warnings, 3001 - errors
	gROOT->ProcessLine("gErrorIgnoreLevel = 3001;");

	return true;
}

bool MonitorLAPPDSC::Execute() {

	if (verbosity > 10)
		std::cout << "MonitorLAPPDSC: Executing ...." << std::endl;

	current = (boost::posix_time::second_clock::local_time());
	for (int i_id=0; i_id < (int) vector_lappd_id.size(); i_id++){
		int lappdid=vector_lappd_id.at(i_id);
		duration = boost::posix_time::time_duration(current - map_last[lappdid]);
		map_duration[lappdid]=duration;
	}
	current_stamp_duration = boost::posix_time::time_duration(current - *Epoch);
	current_stamp = current_stamp_duration.total_milliseconds();
	utc = (boost::posix_time::second_clock::universal_time());
	current_utc_duration = boost::posix_time::time_duration(utc - current);
	current_utc = current_utc_duration.total_milliseconds();
	utc_to_t = (ULong64_t) current_utc;
	t_current = (ULong64_t) current_stamp;

	//------------------------------------------------------------
	//---------Checking the state of LAPPD SC data stream---------
	//------------------------------------------------------------

	std::string State;
	m_data->CStore.Get("State", State);

	bool has_mon_data;
	m_data->CStore.Get("HasLAPPDMonData",has_mon_data);

	if (State == "Wait") {
		if (verbosity > 2)
			std::cout << "MonitorLAPPDSC: State is " << State << std::endl;
	} else if (State == "LAPPDMon" && has_mon_data) {
		if (verbosity > 1)
			std::cout << "MonitorLAPPDSC: New slow-control data available." << std::endl;

		m_data->CStore.Set("LAPPDSlowControlWarning",false);

		m_data->Stores["LAPPDData"]->Get("LAPPDSC", lappd_SC);
		t_last_update = t_current;
		if ((lappd_SC.errorcodes.size() > 1) || (lappd_SC.errorcodes.at(0) != 0)){
			std::cout <<"///////// Encountered error in slow control data //////////"<<std::endl;
			std::cout << lappd_SC.Print() << std::endl;
		}
		
		if (map_duration[lappd_SC.LAPPD_ID] > period_update) {

			//m_data->Stores["LAPPDData"]->Get("LAPPDSC", lappd_SC);

			//std::cout <<"Salt-bridge (received): "<<lappd_SC.saltbridge<<std::endl;
			if (verbosity > -1) {
				std::cout <<"////////////////Got slow control data: //////////////"<<std::endl;
				std::cout << lappd_SC.Print()<<std::endl;
			}
			//Write the event information to a file
			//TODO: change this to a database later on!
			//Check if data has already been written included in WriteToFile function
			WriteToFile();

			//Plot plots only associated to current file
			DrawLastFilePlots();

			//Draw customly defined plots
			UpdateMonitorPlots(config_timeframes, config_endtime_long, config_label, config_plottypes);

			map_last[lappd_SC.LAPPD_ID] = current;

		}
	} else {
		if (verbosity > 1)
			std::cout << "MonitorLAPPDSC: State not recognized: " << State << std::endl;
	}

	// if force_update is specified, the plots will be updated no matter whether there has been a new file or not
	if (force_update)
		UpdateMonitorPlots(config_timeframes, config_endtime_long, config_label, config_plottypes);

	for (int i_id=0; i_id < (int) vector_lappd_id.size(); i_id++){
		int lappdid = vector_lappd_id.at(i_id);
		if (map_duration[lappdid] > period_warning) {
			//Send warning in case the time since the last LAPPD Slow Control update is > 10 minutes
			//Log("Tool MonitorLAPPDSC: SEVERE ERROR: Time since last slow control update is > 10 minutes! (LAPPD ID "+std::to_string(lappdid)+")",v_error,verbosity);
			m_data->CStore.Set("LAPPDSlowControlWarning",true);
			m_data->CStore.Set("LAPPDID",lappdid);
		}
	}

	//gObjectTable only for debugging memory leaks, otherwise comment out
	//std::cout <<"List of Objects (after execute step): "<<std::endl;
	//gObjectTable->Print();

	return true;
}

bool MonitorLAPPDSC::Finalise() {

	if (verbosity > 1)
		std::cout << "Tool MonitorLAPPDSC: Finalising ...." << std::endl;

	//timing pointers
	delete Epoch;

	//canvas
	delete canvas_temp;
	delete canvas_humidity;
	delete canvas_thermistor;
	delete canvas_salt;
	delete canvas_light;
	delete canvas_hv;
	delete canvas_lv;
	delete canvas_status_temphum;
	delete canvas_status_lvhv;
	delete canvas_status_relay;
	delete canvas_status_trigger;
	delete canvas_status_error;
	delete canvas_status_overview;

	//graphs
	for (int i_id=0; i_id < (int) vector_lappd_id.size(); i_id++){
		int lappdid = vector_lappd_id.at(i_id);
		delete map_graph_temp[lappdid];
		delete map_graph_humidity[lappdid];
		delete map_graph_light[lappdid];
		delete map_graph_thermistor[lappdid];
		delete map_graph_salt[lappdid];
		delete map_graph_hv_volt[lappdid];
		delete map_graph_hv_volt_mon[lappdid];
		delete map_graph_lv_volt1[lappdid];
		delete map_graph_lv_volt2[lappdid];
		delete map_graph_lv_volt3[lappdid];
	}

	//multi-graphs
	delete multi_lv;
	delete multi_temp;
	delete multi_humidity;
	delete multi_light;
	delete multi_thermistor;
	delete multi_salt;
	delete multi_hv_volt;
	delete multi_hv_volt_mon;
	delete multi_lv_volt1;
	delete multi_lv_volt2;
	delete multi_lv_volt3;

	//legends
	delete leg_lv;
	delete leg_temp;
	delete leg_humidity;
	delete leg_light;
	delete leg_thermistor;
	delete leg_salt;
	delete leg_hv_volt;
	delete leg_lv_volt1;
	delete leg_lv_volt2;
	delete leg_lv_volt3;

	//text
	//TempHumidity texts
	delete text_temphum_title;
	delete text_temp;
	delete text_hum;
	delete text_thermistor;
	delete text_salt;
	delete text_light;
	delete text_flag_temp;
	delete text_flag_hum;
	delete text_flag_thermistor;
	delete text_flag_salt;

	//LVHV texts
	delete text_lvhv_title;
	delete text_hv_state;
	delete text_hv_mon;
	delete text_hv_volt;
	delete text_hv_monvolt;
	delete text_lv_state;
	delete text_lv_mon;
	delete text_v33;
	delete text_v25;
	delete text_v12;

	//Trigger texts
	delete text_trigger_title;
	delete text_trigger_vref;
	delete text_trigger_trig0_thr;
	delete text_trigger_trig0_mon;
	delete text_trigger_trig1_thr;
	delete text_trigger_trig1_mon;

	//Relay texts
	delete text_relay_title;
	delete text_relay_set1;
	delete text_relay_set2;
	delete text_relay_set3;
	delete text_relay_mon1;
	delete text_relay_mon2;
	delete text_relay_mon3;

	//Error texts
	delete text_error_title;
	delete text_error_number1;
	delete text_error_number2;
	delete text_error_number3;
	delete text_error_number4;
	delete text_error_number5;
	delete text_error_number6;
	delete text_error_number7;
	delete text_error_number8;
	delete text_error_number9;

	delete text_overview_title;
	delete text_overview_temp;
	delete text_overview_lvhv;
	delete text_overview_trigger;
	delete text_overview_relay;
	delete text_overview_error;
	delete text_current_time;
	delete text_sc_time;


	return true;
}

void MonitorLAPPDSC::InitializeHists() {

	if (verbosity > 2)
		std::cout << "MonitorLAPPDSC: InitializeHists" << std::endl;

	//Canvas
	canvas_temp = new TCanvas("canvas_temp", "LVHV Temperature", 900, 600);
	canvas_humidity = new TCanvas("canvas_humidity", "LVHV Humidity", 900, 600);
	canvas_thermistor = new TCanvas("canvas_thermistor", "LVHV Thermistor", 900, 600);
	canvas_salt = new TCanvas("canvas_salt", "LVHV Salt bridge", 900, 600);
	canvas_light = new TCanvas("canvas_light", "LVHV Light", 900, 600);
	canvas_hv = new TCanvas("canvas_hv", "LVHV HV", 900, 600);
	canvas_lv = new TCanvas("canvas_lv", "LVHV LV", 900, 600);
	canvas_status_temphum = new TCanvas("canvas_status_temphum", "Temperature / Humidity status", 900, 600);
	canvas_status_lvhv = new TCanvas("canvas_status_lvhv", "LV / HV status", 900, 600);
	canvas_status_relay = new TCanvas("canvas_status_relay", "Relay status", 900, 600);
	canvas_status_trigger = new TCanvas("canvas_status_trigger", "Trigger status", 900, 600);
	canvas_status_error = new TCanvas("canvas_status_error", "Error status", 900, 600);
	canvas_status_overview = new TCanvas("canvas_status_overview", "Overview status", 900, 600);

	std::vector<int> linecolors{1,2,4,8,9};


	//Graphs
	for (int i_id=0; i_id < (int) vector_lappd_id.size(); i_id++){
		TGraph *graph_temp = new TGraph();
		TGraph *graph_humidity = new TGraph();
		TGraph *graph_thermistor = new TGraph();
		TGraph *graph_salt = new TGraph();
		TGraph *graph_light = new TGraph();
		TGraph *graph_hv_volt = new TGraph();
		TGraph *graph_hv_volt_mon = new TGraph();
		TGraph *graph_lv_volt1 = new TGraph();
		TGraph *graph_lv_volt2 = new TGraph();
		TGraph *graph_lv_volt3 = new TGraph();

		std::stringstream ss_gr_temp, ss_gr_humidity, ss_gr_thermistor, ss_gr_salt, ss_gr_light;
		std::stringstream ss_gr_volt, ss_gr_volt_mon, ss_gr_lv1, ss_gr_lv2, ss_gr_lv3;

		ss_gr_temp << "graph_temp_"<<vector_lappd_id.at(i_id);
		ss_gr_humidity << "graph_humidity_"<<vector_lappd_id.at(i_id);
		ss_gr_thermistor << "graph_thermistor_"<<vector_lappd_id.at(i_id);
		ss_gr_salt << "graph_salt_"<<vector_lappd_id.at(i_id);
		ss_gr_light << "graph_light_"<<vector_lappd_id.at(i_id);
		ss_gr_volt << "graph_hv_volt_"<<vector_lappd_id.at(i_id);
		ss_gr_volt_mon << "graph_hv_volt_mon_"<<vector_lappd_id.at(i_id);
		ss_gr_lv1 << "graph_lv_volt1_"<<vector_lappd_id.at(i_id);
		ss_gr_lv2 << "graph_lv_volt2_"<<vector_lappd_id.at(i_id);
		ss_gr_lv3 << "graph_lv_volt3_"<<vector_lappd_id.at(i_id);

		graph_temp->SetName(ss_gr_temp.str().c_str());
		graph_humidity->SetName(ss_gr_humidity.str().c_str());
		graph_thermistor->SetName(ss_gr_thermistor.str().c_str());
		graph_salt->SetName(ss_gr_salt.str().c_str());
		graph_light->SetName(ss_gr_light.str().c_str());
		graph_hv_volt->SetName(ss_gr_volt.str().c_str());
		graph_hv_volt_mon->SetName(ss_gr_volt_mon.str().c_str());
		graph_lv_volt1->SetName(ss_gr_lv1.str().c_str());
		graph_lv_volt2->SetName(ss_gr_lv2.str().c_str());
		graph_lv_volt3->SetName(ss_gr_lv3.str().c_str());

		graph_temp->SetTitle("LVHV temperature time evolution");
		graph_humidity->SetTitle("LVHV humidity time evolution");
		graph_thermistor->SetTitle("LVHV thermistor time evolution");
		graph_salt->SetTitle("LVHV salt bridge time evolution");
		graph_light->SetTitle("LVHV light level time evolution");
		graph_hv_volt->SetTitle("LVHV HV time evolution");
		graph_hv_volt_mon->SetTitle("LVHV HV MOn time evolution");
		graph_lv_volt1->SetTitle("LVHV LV1 time evolution");
		graph_lv_volt2->SetTitle("LVHV LV2 time evolution");
		graph_lv_volt3->SetTitle("LVHV LV3 time evolution");

		if (draw_marker) {
			graph_temp->SetMarkerStyle(20);
			graph_humidity->SetMarkerStyle(20);
			graph_thermistor->SetMarkerStyle(20);
			graph_salt->SetMarkerStyle(20);
			graph_light->SetMarkerStyle(20);
			graph_hv_volt->SetMarkerStyle(20);
			graph_hv_volt_mon->SetMarkerStyle(20);
			graph_lv_volt1->SetMarkerStyle(20);
			graph_lv_volt2->SetMarkerStyle(20);
			graph_lv_volt3->SetMarkerStyle(20);
		}

		int linecolor=i_id+1;
		if (i_id < 5) linecolor = linecolors.at(i_id);

		graph_temp->SetMarkerColor(linecolor);
		graph_humidity->SetMarkerColor(linecolor);
		graph_thermistor->SetMarkerColor(linecolor);
		graph_salt->SetMarkerColor(linecolor);
		graph_light->SetMarkerColor(linecolor);
		graph_hv_volt->SetMarkerColor(linecolor);
		graph_hv_volt_mon->SetMarkerColor(linecolor);
		graph_lv_volt1->SetMarkerColor(linecolor);
		graph_lv_volt2->SetMarkerColor(linecolor);
		graph_lv_volt3->SetMarkerColor(linecolor);

		graph_temp->SetLineColor(linecolor);
		graph_humidity->SetLineColor(linecolor);
		graph_thermistor->SetLineColor(linecolor);
		graph_salt->SetLineColor(linecolor);
		graph_light->SetLineColor(linecolor);
		graph_hv_volt->SetLineColor(linecolor);
		graph_hv_volt_mon->SetLineColor(linecolor);
		graph_lv_volt1->SetLineColor(linecolor);
		graph_lv_volt2->SetLineColor(linecolor);
		graph_lv_volt3->SetLineColor(linecolor);

		graph_temp->SetLineWidth(2);
		graph_humidity->SetLineWidth(2);
		graph_thermistor->SetLineWidth(2);
		graph_salt->SetLineWidth(2);
		graph_light->SetLineWidth(2);
		graph_hv_volt->SetLineWidth(2);
		graph_hv_volt_mon->SetLineWidth(2);
		graph_lv_volt1->SetLineWidth(2);
		graph_lv_volt2->SetLineWidth(2);
		graph_lv_volt3->SetLineWidth(2);

		graph_temp->SetFillColor(0);
		graph_humidity->SetFillColor(0);
		graph_thermistor->SetFillColor(0);
		graph_salt->SetFillColor(0);
		graph_light->SetFillColor(0);
		graph_hv_volt->SetFillColor(0);
		graph_hv_volt_mon->SetFillColor(0);
		graph_lv_volt1->SetFillColor(0);
		graph_lv_volt2->SetFillColor(0);
		graph_lv_volt3->SetFillColor(0);

		graph_temp->GetYaxis()->SetTitle("Temperature");
		graph_humidity->GetYaxis()->SetTitle("Humidity");
		graph_thermistor->GetYaxis()->SetTitle("Thermistor");
		graph_salt->GetYaxis()->SetTitle("Salt-Bridge");
		graph_light->GetYaxis()->SetTitle("Light level");
		graph_hv_volt->GetYaxis()->SetTitle("HV set (V)");
		graph_hv_volt_mon->GetYaxis()->SetTitle("HV mon (V)");
		graph_lv_volt1->GetYaxis()->SetTitle("LV mon (V)");
		graph_lv_volt2->GetYaxis()->SetTitle("LV mon (V)");
		graph_lv_volt3->GetYaxis()->SetTitle("LV mon (V)");

		graph_temp->GetXaxis()->SetTimeDisplay(1);
		graph_humidity->GetXaxis()->SetTimeDisplay(1);
		graph_thermistor->GetXaxis()->SetTimeDisplay(1);
		graph_salt->GetXaxis()->SetTimeDisplay(1);
		graph_light->GetXaxis()->SetTimeDisplay(1);
		graph_hv_volt->GetXaxis()->SetTimeDisplay(1);
		graph_hv_volt_mon->GetXaxis()->SetTimeDisplay(1);
		graph_lv_volt1->GetXaxis()->SetTimeDisplay(1);
		graph_lv_volt2->GetXaxis()->SetTimeDisplay(1);
		graph_lv_volt3->GetXaxis()->SetTimeDisplay(1);

		graph_temp->GetXaxis()->SetLabelSize(0.03);
		graph_humidity->GetXaxis()->SetLabelSize(0.03);
		graph_thermistor->GetXaxis()->SetLabelSize(0.03);
		graph_salt->GetXaxis()->SetLabelSize(0.03);
		graph_light->GetXaxis()->SetLabelSize(0.03);
		graph_hv_volt->GetXaxis()->SetLabelSize(0.03);
		graph_hv_volt_mon->GetXaxis()->SetLabelSize(0.03);
		graph_lv_volt1->GetXaxis()->SetLabelSize(0.03);
		graph_lv_volt2->GetXaxis()->SetLabelSize(0.03);
		graph_lv_volt3->GetXaxis()->SetLabelSize(0.03);
	
		graph_temp->GetXaxis()->SetLabelOffset(0.03);
		graph_humidity->GetXaxis()->SetLabelOffset(0.03);
		graph_thermistor->GetXaxis()->SetLabelOffset(0.03);
		graph_salt->GetXaxis()->SetLabelOffset(0.03);
		graph_light->GetXaxis()->SetLabelOffset(0.03);
		graph_hv_volt->GetXaxis()->SetLabelOffset(0.03);
		graph_hv_volt_mon->GetXaxis()->SetLabelOffset(0.03);
		graph_lv_volt1->GetXaxis()->SetLabelOffset(0.03);
		graph_lv_volt2->GetXaxis()->SetLabelOffset(0.03);
		graph_lv_volt3->GetXaxis()->SetLabelOffset(0.03);

		graph_temp->GetXaxis()->SetTimeFormat("#splitline(%m/%d}{%H:%M}");
		graph_humidity->GetXaxis()->SetTimeFormat("#splitline(%m/%d}{%H:%M}");
		graph_thermistor->GetXaxis()->SetTimeFormat("#splitline(%m/%d}{%H:%M}");
		graph_salt->GetXaxis()->SetTimeFormat("#splitline(%m/%d}{%H:%M}");
		graph_light->GetXaxis()->SetTimeFormat("#splitline(%m/%d}{%H:%M}");
		graph_hv_volt->GetXaxis()->SetTimeFormat("#splitline(%m/%d}{%H:%M}");
		graph_hv_volt_mon->GetXaxis()->SetTimeFormat("#splitline(%m/%d}{%H:%M}");
		graph_lv_volt1->GetXaxis()->SetTimeFormat("#splitline(%m/%d}{%H:%M}");
		graph_lv_volt2->GetXaxis()->SetTimeFormat("#splitline(%m/%d}{%H:%M}");
		graph_lv_volt3->GetXaxis()->SetTimeFormat("#splitline(%m/%d}{%H:%M}");

		map_graph_temp.emplace(vector_lappd_id.at(i_id),graph_temp);
		map_graph_humidity.emplace(vector_lappd_id.at(i_id),graph_humidity);
		map_graph_thermistor.emplace(vector_lappd_id.at(i_id),graph_thermistor);
		map_graph_salt.emplace(vector_lappd_id.at(i_id),graph_salt);
		map_graph_light.emplace(vector_lappd_id.at(i_id),graph_light);
		map_graph_hv_volt.emplace(vector_lappd_id.at(i_id),graph_hv_volt);
		map_graph_hv_volt_mon.emplace(vector_lappd_id.at(i_id),graph_hv_volt_mon);
		map_graph_lv_volt1.emplace(vector_lappd_id.at(i_id),graph_lv_volt1);
		map_graph_lv_volt2.emplace(vector_lappd_id.at(i_id),graph_lv_volt2);
		map_graph_lv_volt3.emplace(vector_lappd_id.at(i_id),graph_lv_volt3);
	}

	//Multi-Graphs
	multi_lv = new TMultiGraph();
	multi_temp = new TMultiGraph();
  	multi_humidity = new TMultiGraph();
  	multi_thermistor = new TMultiGraph();
  	multi_salt = new TMultiGraph();
  	multi_light = new TMultiGraph();
  	multi_hv_volt = new TMultiGraph();
  	multi_hv_volt_mon = new TMultiGraph();
  	multi_lv_volt1 = new TMultiGraph();
  	multi_lv_volt2 = new TMultiGraph();
  	multi_lv_volt3 = new TMultiGraph();

	//Legends
	leg_lv = new TLegend(0.7, 0.7, 0.88, 0.88);
	leg_lv->SetLineColor(0);
	leg_temp = new TLegend(0.7, 0.7, 0.88, 0.88);
	leg_humidity = new TLegend(0.7, 0.7, 0.88, 0.88);
	leg_thermistor = new TLegend(0.7, 0.7, 0.88, 0.88);
	leg_salt = new TLegend(0.7, 0.7, 0.88, 0.88);
	leg_light = new TLegend(0.7, 0.7, 0.88, 0.88);
	leg_hv_volt = new TLegend(0.7, 0.7, 0.88, 0.88);
	leg_hv_volt_mon = new TLegend(0.7, 0.7, 0.88, 0.88);
	leg_lv_volt1 = new TLegend(0.7, 0.7, 0.88, 0.88);
	leg_lv_volt2 = new TLegend(0.7, 0.7, 0.88, 0.88);
	leg_lv_volt3 = new TLegend(0.7, 0.7, 0.88, 0.88);
	leg_temp->SetLineColor(0);
	leg_humidity->SetLineColor(0);
	leg_thermistor->SetLineColor(0);
	leg_salt->SetLineColor(0);
	leg_light->SetLineColor(0);
	leg_hv_volt->SetLineColor(0);
	leg_hv_volt_mon->SetLineColor(0);
	leg_lv_volt1->SetLineColor(0);
	leg_lv_volt2->SetLineColor(0);
	leg_lv_volt3->SetLineColor(0);

	//Text
	//TempHumidity texts
	text_temphum_title = new TText();
	text_temp = new TText();
	text_hum = new TText();
	text_thermistor = new TText();
	text_salt = new TText();
	text_light = new TText();
	text_flag_temp = new TText();
	text_flag_hum = new TText();
	text_flag_thermistor = new TText();
	text_flag_salt = new TText();

	text_temphum_title->SetNDC(1);
	text_temp->SetNDC(1);
	text_hum->SetNDC(1);
	text_thermistor->SetNDC(1);
	text_salt->SetNDC(1);
	text_light->SetNDC(1);
	text_flag_temp->SetNDC(1);
	text_flag_hum->SetNDC(1);
	text_flag_thermistor->SetNDC(1);
	text_flag_salt->SetNDC(1);

	//LVHV texts
	text_lvhv_title = new TText();
	text_hv_state = new TText();
	text_hv_mon = new TText();
	text_hv_volt = new TText();
	text_hv_monvolt = new TText();
	text_lv_state = new TText();
	text_lv_mon = new TText();
	text_v33 = new TText();
	text_v25 = new TText();
	text_v12 = new TText();

	//LVHV texts
	text_lvhv_title->SetNDC(1);
	text_hv_state->SetNDC(1);
	text_hv_mon->SetNDC(1);
	text_hv_volt->SetNDC(1);
	text_hv_monvolt->SetNDC(1);
	text_lv_state->SetNDC(1);
	text_lv_mon->SetNDC(1);
	text_v33->SetNDC(1);
	text_v25->SetNDC(1);
	text_v12->SetNDC(1);

	//Trigger texts
	text_trigger_title = new TText();
	text_trigger_vref = new TText();
	text_trigger_trig0_thr = new TText();
	text_trigger_trig0_mon = new TText();
	text_trigger_trig1_thr = new TText();
	text_trigger_trig1_mon = new TText();

	text_trigger_title->SetNDC(1);
	text_trigger_vref->SetNDC(1);
	text_trigger_trig0_thr->SetNDC(1);
	text_trigger_trig0_mon->SetNDC(1);
	text_trigger_trig1_thr->SetNDC(1);
	text_trigger_trig1_mon->SetNDC(1);

	//Relay texts
	text_relay_title = new TText();
	text_relay_set1 = new TText();
	text_relay_set2 = new TText();
	text_relay_set3 = new TText();
	text_relay_mon1 = new TText();
	text_relay_mon2 = new TText();
	text_relay_mon3 = new TText();

	text_relay_title->SetNDC(1);
	text_relay_set1->SetNDC(1);
	text_relay_set2->SetNDC(1);
	text_relay_set3->SetNDC(1);
	text_relay_mon1->SetNDC(1);
	text_relay_mon2->SetNDC(1);
	text_relay_mon3->SetNDC(1);

	//Error text
	text_error_title = new TText();
	text_error_number1 = new TText();
	text_error_number2 = new TText();
	text_error_number3 = new TText();
	text_error_number4 = new TText();
	text_error_number5 = new TText();
	text_error_number6 = new TText();
	text_error_number7 = new TText();
	text_error_number8 = new TText();
	text_error_number9 = new TText();

	text_error_title->SetNDC(1);
	text_error_number1->SetNDC(1);
	text_error_number2->SetNDC(1);
	text_error_number3->SetNDC(1);
	text_error_number4->SetNDC(1);
	text_error_number5->SetNDC(1);
	text_error_number6->SetNDC(1);
	text_error_number7->SetNDC(1);
	text_error_number8->SetNDC(1);
	text_error_number9->SetNDC(1);

	text_error_vector.push_back(text_error_number1);
	text_error_vector.push_back(text_error_number2);
	text_error_vector.push_back(text_error_number3);
	text_error_vector.push_back(text_error_number4);
	text_error_vector.push_back(text_error_number5);
	text_error_vector.push_back(text_error_number6);
	text_error_vector.push_back(text_error_number7);
	text_error_vector.push_back(text_error_number8);
	text_error_vector.push_back(text_error_number9);

	//Overview texts
	text_overview_title = new TText();
	text_overview_temp = new TText();
	text_overview_lvhv = new TText();
	text_overview_trigger = new TText();
	text_overview_relay = new TText();
	text_overview_error = new TText();
	text_current_time = new TText();
	text_sc_time = new TText();

	text_overview_title->SetNDC(1);
	text_overview_temp->SetNDC(1);
	text_overview_lvhv->SetNDC(1);
	text_overview_trigger->SetNDC(1);
	text_overview_relay->SetNDC(1);
	text_overview_error->SetNDC(1);
	text_current_time->SetNDC(1);
	text_sc_time->SetNDC(1);

}

void MonitorLAPPDSC::ReadInConfiguration() {

	//-------------------------------------------------------
	//----------------ReadInConfiguration -------------------
	//-------------------------------------------------------

	Log("MonitorLAPPDSC::ReadInConfiguration", v_message, verbosity);

	ifstream file(plot_configuration.c_str());

	std::string line;
	if (file.is_open()) {
		while (std::getline(file, line)) {
			if (line.find("#") != std::string::npos)
				continue;
			std::vector<std::string> values;
			std::stringstream ss;
			ss.str(line);
			std::string item;
			while (std::getline(ss, item, '\t')) {
				values.push_back(item);
			}
			if (values.size() < 4) {
				if (verbosity > 0)
					std::cout << "ERROR (MonitorLAPPDSC): ReadInConfiguration: Need at least 4 arguments in one line: TimeFrame - TimeEnd - FileLabel - PlotType1. Please look over the configuration file and adjust it accordingly." << std::endl;
				continue;
			}
			double double_value = std::stod(values.at(0));
			config_timeframes.push_back(double_value);
			config_endtime.push_back(values.at(1));
			config_label.push_back(values.at(2));
			std::vector<std::string> plottypes;
			for (unsigned int i = 3; i < values.size(); i++) {
				plottypes.push_back(values.at(i));
			}
			config_plottypes.push_back(plottypes);
		}
	} else {
		if (verbosity > 0)
			std::cout << "ERROR (MonitorLAPPDSC): ReadInConfiguration: Could not open file " << plot_configuration << "! Check if path is valid..." << std::endl;
	}
	file.close();

	if (verbosity > 2) {
		std::cout << "---------------------------------------------------------------------" << std::endl;
		std::cout << "MonitorLAPPDSC: ReadInConfiguration: Read in the following data into configuration variables: " << std::endl;
		for (unsigned int i_t = 0; i_t < config_timeframes.size(); i_t++) {
			std::cout << config_timeframes.at(i_t) << ", " << config_endtime.at(i_t) << ", " << config_label.at(i_t) << ", ";
			for (unsigned int i_plot = 0; i_plot < config_plottypes.at(i_t).size(); i_plot++) {
				std::cout << config_plottypes.at(i_t).at(i_plot) << ", ";
			}
			std::cout << std::endl;
		}
		std::cout << "-----------------------------------------------------------------------" << std::endl;
	}

	if (verbosity > 2)
		std::cout << "MonitorLAPPDSC: ReadInConfiguration: Parsing dates: " << std::endl;
	for (unsigned int i_date = 0; i_date < config_endtime.size(); i_date++) {
		if (config_endtime.at(i_date) == "TEND_LASTFILE") {
			if (verbosity > 2)
				std::cout << "TEND_LASTFILE: Starting from end of last read-in file" << std::endl;
			ULong64_t zero = 0;
			config_endtime_long.push_back(zero);
		} else if (config_endtime.at(i_date).size() == 15) {
			boost::posix_time::ptime spec_endtime(boost::posix_time::from_iso_string(config_endtime.at(i_date)));
			boost::posix_time::time_duration spec_endtime_duration = boost::posix_time::time_duration(spec_endtime - *Epoch);
			ULong64_t spec_endtime_long = spec_endtime_duration.total_milliseconds();
			config_endtime_long.push_back(spec_endtime_long);
		} else {
			if (verbosity > 2)
				std::cout << "Specified end date " << config_endtime.at(i_date) << " does not have the desired format YYYYMMDDTHHMMSS. Please change the format in the config file in order to use this tool. Starting from end of last file" << std::endl;
			ULong64_t zero = 0;
			config_endtime_long.push_back(zero);
		}
	}
}

std::string MonitorLAPPDSC::convertTimeStamp_to_Date(ULong64_t timestamp) {

	//format of date is YYYY_MM-DD

	boost::posix_time::ptime convertedtime = *Epoch + boost::posix_time::time_duration(int(timestamp / MSEC_to_SEC / SEC_to_MIN / MIN_to_HOUR), int(timestamp / MSEC_to_SEC / SEC_to_MIN) % 60, int(timestamp / MSEC_to_SEC / 1000.) % 60, timestamp % 1000);
	struct tm converted_tm = boost::posix_time::to_tm(convertedtime);
	std::stringstream ss_date;
	ss_date << converted_tm.tm_year + 1900 << "_" << converted_tm.tm_mon + 1 << "-" << converted_tm.tm_mday;
	return ss_date.str();

}

bool MonitorLAPPDSC::does_file_exist(std::string filename) {

	std::ifstream infile(filename.c_str());
	bool file_good = infile.good();
	infile.close();
	return file_good;

}

void MonitorLAPPDSC::WriteToFile() {

	Log("MonitorLAPPDSC: WriteToFile", v_message, verbosity);

	//-------------------------------------------------------
	//------------------WriteToFile -------------------------
	//-------------------------------------------------------

	std::string file_start_date = convertTimeStamp_to_Date(t_current);
	std::stringstream root_filename;
	root_filename << path_monitoring << "LAPPDSC_" << file_start_date << ".root";

	Log("MonitorLAPPDSC: ROOT filename: " + root_filename.str(), v_message, verbosity);

	std::string root_option = "RECREATE";
	if (does_file_exist(root_filename.str()))
		root_option = "UPDATE";
	TFile *f = new TFile(root_filename.str().c_str(), root_option.c_str());

	ULong64_t t_time;
	float t_hum;
	float t_temp;
	float t_thermistor;
	float t_salt;
	int t_hvmon;
	bool t_hvstateset;
	float t_hvvolt;
	float t_hvreturnmon;
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
	int t_flag_thermistor;
	int t_flag_salt;
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
	int t_lappdid;

	TTree *t;
	if (f->GetListOfKeys()->Contains("lappdscmonitor_tree")) {
		Log("MonitorLAPPDSC: WriteToFile: Tree already exists", v_message, verbosity);
		t = (TTree*) f->Get("lappdscmonitor_tree");
		t->SetBranchAddress("t_current", &t_time);
		t->SetBranchAddress("humidity", &t_hum);
		t->SetBranchAddress("temp", &t_temp);
		t->SetBranchAddress("thermistor", &t_thermistor);
		t->SetBranchAddress("salt", &t_salt);
		t->SetBranchAddress("hv_mon", &t_hvmon);
		t->SetBranchAddress("hv_state_set", &t_hvstateset);
		t->SetBranchAddress("hv_volt", &t_hvvolt);
		t->SetBranchAddress("hv_returnmon", &t_hvreturnmon);
		t->SetBranchAddress("lv_mon", &t_lvmon);
		t->SetBranchAddress("lv_state_set", &t_lvstateset);
		t->SetBranchAddress("v33", &t_v33);
		t->SetBranchAddress("v25", &t_v25);
		t->SetBranchAddress("v12", &t_v12);
		t->SetBranchAddress("temp_low", &t_temp_low);
		t->SetBranchAddress("temp_high", &t_temp_high);
		t->SetBranchAddress("hum_low", &t_hum_low);
		t->SetBranchAddress("hum_high", &t_hum_high);
		t->SetBranchAddress("flag_temp", &t_flag_temp);
		t->SetBranchAddress("flag_hum", &t_flag_humidity);
		t->SetBranchAddress("flag_thermistor", &t_flag_thermistor);
		t->SetBranchAddress("flag_salt", &t_flag_salt);
		t->SetBranchAddress("relCh1", &t_relCh1);
		t->SetBranchAddress("relCh2", &t_relCh2);
		t->SetBranchAddress("relCh3", &t_relCh3);
		t->SetBranchAddress("relCh1_mon", &t_relCh1_mon);
		t->SetBranchAddress("relCh2_mon", &t_relCh2_mon);
		t->SetBranchAddress("relCh3_mon", &t_relCh3_mon);
		t->SetBranchAddress("trig_vref", &t_trig_vref);
		t->SetBranchAddress("trig1_thr", &t_trig1_thr);
		t->SetBranchAddress("trig1_mon", &t_trig1_mon);
		t->SetBranchAddress("trig0_thr", &t_trig0_thr);
		t->SetBranchAddress("trig0_mon", &t_trig0_mon);
		t->SetBranchAddress("light_level", &t_light_level);
		t->SetBranchAddress("vec_errors", &t_vec_errors);
		t->SetBranchAddress("lappdid", &t_lappdid);
	} else {
		t = new TTree("lappdscmonitor_tree", "LAPPD SC Monitoring tree");
		Log("MonitorLAPPDSC: WriteToFile: Tree is created from scratch", v_message, verbosity);
		t->Branch("t_current", &t_time);
		t->Branch("humidity", &t_hum);
		t->Branch("temp", &t_temp);
		t->Branch("thermistor", &t_thermistor);
		t->Branch("salt", &t_salt);
		t->Branch("hv_mon", &t_hvmon);
		t->Branch("hv_state_set", &t_hvstateset);
		t->Branch("hv_volt", &t_hvvolt);
                t->Branch("hv_returnmon", &t_hvreturnmon);
		t->Branch("lv_mon", &t_lvmon);
		t->Branch("lv_state_set", &t_lvstateset);
		t->Branch("v33", &t_v33);
		t->Branch("v25", &t_v25);
		t->Branch("v12", &t_v12);
		t->Branch("temp_low", &t_temp_low);
		t->Branch("temp_high", &t_temp_high);
		t->Branch("hum_low", &t_hum_low);
		t->Branch("hum_high", &t_hum_high);
		t->Branch("flag_temp", &t_flag_temp);
		t->Branch("flag_hum", &t_flag_humidity);
		t->Branch("flag_thermistor", &t_flag_thermistor);
		t->Branch("flag_salt", &t_flag_salt);
		t->Branch("relCh1", &t_relCh1);
		t->Branch("relCh2", &t_relCh2);
		t->Branch("relCh3", &t_relCh3);
		t->Branch("relCh1_mon", &t_relCh1_mon);
		t->Branch("relCh2_mon", &t_relCh2_mon);
		t->Branch("relCh3_mon", &t_relCh3_mon);
		t->Branch("trig_vref", &t_trig_vref);
		t->Branch("trig1_thr", &t_trig1_thr);
		t->Branch("trig1_mon", &t_trig1_mon);
		t->Branch("trig0_thr", &t_trig0_thr);
		t->Branch("trig0_mon", &t_trig0_mon);
		t->Branch("light_level", &t_light_level);
		t->Branch("vec_errors", &t_vec_errors);
		t->Branch("lappdid", &t_lappdid);
	}

	int n_entries = t->GetEntries();
	bool omit_entries = false;
	for (int i_entry = 0; i_entry < n_entries; i_entry++) {
		t->GetEntry(i_entry);
		if (t_time == t_current) {
			Log("WARNING (MonitorLAPPDSC): WriteToFile: Wanted to write data from file that is already written to DB. Omit entries", v_warning, verbosity);
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

	boost::posix_time::ptime starttime = *Epoch + boost::posix_time::time_duration(int(t_time / MSEC_to_SEC / SEC_to_MIN / MIN_to_HOUR), int(t_time / MSEC_to_SEC / SEC_to_MIN) % 60, int(t_time / MSEC_to_SEC / 1000.) % 60, t_time % 1000);
	struct tm starttime_tm = boost::posix_time::to_tm(starttime);
	Log(
			"MonitorTankTime: WriteToFile: Writing data to file: " + std::to_string(starttime_tm.tm_year + 1900) + "/" + std::to_string(starttime_tm.tm_mon + 1) + "/" + std::to_string(starttime_tm.tm_mday) + "-" + std::to_string(starttime_tm.tm_hour) + ":" + std::to_string(starttime_tm.tm_min) + ":"
					+ std::to_string(starttime_tm.tm_sec), v_message, verbosity);

	t_hum = lappd_SC.humidity_mon;
	t_temp = lappd_SC.temperature_mon;
	t_thermistor = 0.;
	if (lappd_SC.temperature_thermistor > 0) t_thermistor = TMath::Log(lappd_SC.temperature_thermistor/1000./29.4)/(-0.0437);
	//t_thermistor = lappd_SC.temperature_thermistor;
	t_salt = lappd_SC.saltbridge;
	t_hvmon = lappd_SC.HV_mon;
	t_hvstateset = lappd_SC.HV_state_set;
	t_hvvolt = lappd_SC.HV_volts;
        t_hvreturnmon = lappd_SC.HV_return_mon;
	t_lvmon = lappd_SC.LV_mon;
	t_lvstateset = lappd_SC.LV_state_set;
	t_v33 = lappd_SC.v33;
	t_v25 = lappd_SC.v25;
	t_v12 = lappd_SC.v12;
	t_temp_low = lappd_SC.LIMIT_temperature_low;
	t_temp_high = lappd_SC.LIMIT_temperature_high;
	t_hum_low = lappd_SC.LIMIT_humidity_low;
	t_hum_high = lappd_SC.LIMIT_humidity_high;
	t_flag_temp = lappd_SC.FLAG_temperature;
	t_flag_humidity = lappd_SC.FLAG_humidity;
	t_flag_thermistor = lappd_SC.FLAG_temperature_Thermistor;
	t_flag_salt = lappd_SC.FLAG_saltbridge;
	t_relCh1 = lappd_SC.relayCh1;
	t_relCh2 = lappd_SC.relayCh2;
	t_relCh3 = lappd_SC.relayCh3;
	t_relCh1_mon = lappd_SC.relayCh1_mon;
	t_relCh2_mon = lappd_SC.relayCh2_mon;
	t_relCh3_mon = lappd_SC.relayCh3_mon;
	t_trig_vref = lappd_SC.TrigVref;
	t_trig1_thr = lappd_SC.Trig1_threshold;
	t_trig1_mon = lappd_SC.Trig1_mon;
	t_trig0_thr = lappd_SC.Trig0_threshold;
	t_trig_mon = lappd_SC.Trig0_mon;
	t_light_level = lappd_SC.light;
	for (int i_error = 0; i_error < (int) lappd_SC.errorcodes.size(); i_error++) {
		t_vec_errors->push_back(lappd_SC.errorcodes.at(i_error));
	}
	t_lappdid = lappd_SC.LAPPD_ID;

	t->Fill();
	t->Write("", TObject::kOverwrite);     //prevent ROOT from making endless keys for the same tree when updating the tree
	f->Close();

	delete t_vec_errors;
	delete f;

	gROOT->cd();

}

void MonitorLAPPDSC::ReadFromFile(ULong64_t timestamp, double time_frame) {

	Log("MonitorLAPPDSC: ReadFromFile", v_message, verbosity);

	//-------------------------------------------------------
	//------------------ReadFromFile ------------------------
	//-------------------------------------------------------

	times_plot.clear();
	temp_plot.clear();
	humidity_plot.clear();
	thermistor_plot.clear();
	salt_plot.clear();
	hum_low_plot.clear();
	hv_volt_plot.clear();
	hv_mon_plot.clear();
	hv_stateset_plot.clear();
	hv_returnmon_plot.clear();
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
	flag_thermistor_plot.clear();
	flag_salt_plot.clear();
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
	lappdid_plot.clear();

	//take the end time and calculate the start time with the given time_frame
	ULong64_t timestamp_start = timestamp - time_frame * MIN_to_HOUR * SEC_to_MIN * MSEC_to_SEC;
	boost::posix_time::ptime starttime = *Epoch + boost::posix_time::time_duration(int(timestamp_start / MSEC_to_SEC / SEC_to_MIN / MIN_to_HOUR), int(timestamp_start / MSEC_to_SEC / SEC_to_MIN) % 60, int(timestamp_start / MSEC_to_SEC / 1000.) % 60, timestamp_start % 1000);
	struct tm starttime_tm = boost::posix_time::to_tm(starttime);
	boost::posix_time::ptime endtime = *Epoch + boost::posix_time::time_duration(int(timestamp / MSEC_to_SEC / SEC_to_MIN / MIN_to_HOUR), int(timestamp / MSEC_to_SEC / SEC_to_MIN) % 60, int(timestamp / MSEC_to_SEC / 1000.) % 60, timestamp % 1000);
	struct tm endtime_tm = boost::posix_time::to_tm(endtime);

	Log(
			"MonitorLAPPDSC: ReadFromFile: Reading in data for time frame " + std::to_string(starttime_tm.tm_year + 1900) + "/" + std::to_string(starttime_tm.tm_mon + 1) + "/" + std::to_string(starttime_tm.tm_mday) + "-" + std::to_string(starttime_tm.tm_hour) + ":"
					+ std::to_string(starttime_tm.tm_min) + ":" + std::to_string(starttime_tm.tm_sec) + " ... " + std::to_string(endtime_tm.tm_year + 1900) + "/" + std::to_string(endtime_tm.tm_mon + 1) + "/" + std::to_string(endtime_tm.tm_mday) + "-" + std::to_string(endtime_tm.tm_hour) + ":"
					+ std::to_string(endtime_tm.tm_min) + ":" + std::to_string(endtime_tm.tm_sec), v_message, verbosity);

	std::stringstream ss_startdate, ss_enddate;
	ss_startdate << starttime_tm.tm_year + 1900 << "-" << starttime_tm.tm_mon + 1 << "-" << starttime_tm.tm_mday;
	ss_enddate << endtime_tm.tm_year + 1900 << "-" << endtime_tm.tm_mon + 1 << "-" << endtime_tm.tm_mday;

	int number_of_days;
	if (endtime_tm.tm_mon == starttime_tm.tm_mon)
		number_of_days = endtime_tm.tm_mday - starttime_tm.tm_mday;
	else {
		boost::gregorian::date endtime_greg(boost::gregorian::from_simple_string(ss_enddate.str()));
		boost::gregorian::date starttime_greg(boost::gregorian::from_simple_string(ss_startdate.str()));
		boost::gregorian::days days_dataframe = endtime_greg - starttime_greg;
		number_of_days = days_dataframe.days();
	}

	for (int i_day = 0; i_day <= number_of_days; i_day++) {

		ULong64_t timestamp_i = timestamp_start + i_day * HOUR_to_DAY * MIN_to_HOUR * SEC_to_MIN * MSEC_to_SEC;
		std::string string_date_i = convertTimeStamp_to_Date(timestamp_i);
		std::stringstream root_filename_i;
		root_filename_i << path_monitoring << "LAPPDSC_" << string_date_i << ".root";
		bool tree_exists = true;

		if (does_file_exist(root_filename_i.str())) {
			TFile *f = new TFile(root_filename_i.str().c_str(), "READ");
			TTree *t;
			if (f->GetListOfKeys()->Contains("lappdscmonitor_tree"))
				t = (TTree*) f->Get("lappdscmonitor_tree");
			else {
				Log("WARNING (MonitorLAPPDSC): File " + root_filename_i.str() + " does not contain lappdscmonitor_tree. Omit file.", v_warning, verbosity);
				tree_exists = false;
			}

			if (tree_exists) {

				Log("MonitorLAPPDSC: Tree exists, start reading in data", v_message, verbosity);

				ULong64_t t_time;
				std::vector<unsigned int> *t_vec_errors = new std::vector<unsigned int>;
				float t_hum;
				float t_temp;
				float t_thermistor;
				float t_salt;
				int t_hvmon;
				bool t_hvstateset;
				float t_hvvolt;
				float t_hvreturnmon;
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
				int t_flag_thermistor;
				int t_flag_salt;
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
				int t_lappdid;

				int nentries_tree;

				t->SetBranchAddress("t_current", &t_time);
				t->SetBranchAddress("humidity", &t_hum);
				t->SetBranchAddress("temp", &t_temp);
				t->SetBranchAddress("thermistor", &t_thermistor);
				t->SetBranchAddress("salt", &t_salt);
				t->SetBranchAddress("hv_mon", &t_hvmon);
				t->SetBranchAddress("hv_state_set", &t_hvstateset);
				t->SetBranchAddress("hv_volt", &t_hvvolt);
				t->SetBranchAddress("hv_returnmon", &t_hvreturnmon);
				t->SetBranchAddress("lv_mon", &t_lvmon);
				t->SetBranchAddress("lv_state_set", &t_lvstateset);
				t->SetBranchAddress("v33", &t_v33);
				t->SetBranchAddress("v25", &t_v25);
				t->SetBranchAddress("v12", &t_v12);
				t->SetBranchAddress("temp_low", &t_temp_low);
				t->SetBranchAddress("temp_high", &t_temp_high);
				t->SetBranchAddress("hum_low", &t_hum_low);
				t->SetBranchAddress("hum_high", &t_hum_high);
				t->SetBranchAddress("flag_temp", &t_flag_temp);
				t->SetBranchAddress("flag_hum", &t_flag_humidity);
				t->SetBranchAddress("flag_thermistor", &t_flag_thermistor);
				t->SetBranchAddress("flag_salt", &t_flag_salt);
				t->SetBranchAddress("relCh1", &t_relCh1);
				t->SetBranchAddress("relCh2", &t_relCh2);
				t->SetBranchAddress("relCh3", &t_relCh3);
				t->SetBranchAddress("relCh1_mon", &t_relCh1_mon);
				t->SetBranchAddress("relCh2_mon", &t_relCh2_mon);
				t->SetBranchAddress("relCh3_mon", &t_relCh3_mon);
				t->SetBranchAddress("trig_vref", &t_trig_vref);
				t->SetBranchAddress("trig1_thr", &t_trig1_thr);
				t->SetBranchAddress("trig1_mon", &t_trig1_mon);
				t->SetBranchAddress("trig0_thr", &t_trig0_thr);
				t->SetBranchAddress("trig0_mon", &t_trig0_mon);
				t->SetBranchAddress("light_level", &t_light_level);
				t->SetBranchAddress("vec_errors", &t_vec_errors);
				t->SetBranchAddress("lappdid", &t_lappdid);

				nentries_tree = t->GetEntries();

				//Sort timestamps for the case that they are not in order

				std::vector<ULong64_t> vector_timestamps;
				std::map<ULong64_t, int> map_timestamp_entry;
				for (int i_entry = 0; i_entry < nentries_tree; i_entry++) {
					t->GetEntry(i_entry);
					if (t_time >= timestamp_start && t_time <= timestamp) {
						vector_timestamps.push_back(t_time);
						map_timestamp_entry.emplace(t_time, i_entry);
					}
				}

				std::sort(vector_timestamps.begin(), vector_timestamps.end());
				std::vector<int> vector_sorted_entry;

				for (int i_entry = 0; i_entry < (int) vector_timestamps.size(); i_entry++) {
					vector_sorted_entry.push_back(map_timestamp_entry.at(vector_timestamps.at(i_entry)));
				}

				for (int i_entry = 0; i_entry < (int) vector_sorted_entry.size(); i_entry++) {

					int next_entry = vector_sorted_entry.at(i_entry);

					t->GetEntry(next_entry);
					if (t_time >= timestamp_start && t_time <= timestamp) {
						times_plot.push_back(t_time);
						temp_plot.push_back(t_temp);
						humidity_plot.push_back(t_hum);
						thermistor_plot.push_back(t_thermistor);
						salt_plot.push_back(t_salt);
						hv_mon_plot.push_back(t_hvmon);
						hv_volt_plot.push_back(t_hvvolt);
						hv_stateset_plot.push_back(t_hvstateset);
						hv_returnmon_plot.push_back(t_hvreturnmon);
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
						flag_thermistor_plot.push_back(t_flag_thermistor);
						flag_salt_plot.push_back(t_flag_salt);
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
						lappdid_plot.push_back(t_lappdid);

						boost::posix_time::ptime boost_tend = *Epoch + boost::posix_time::time_duration(int(t_time / MSEC_to_SEC / SEC_to_MIN / MIN_to_HOUR), int(t_time / MSEC_to_SEC / SEC_to_MIN) % 60, int(t_time / MSEC_to_SEC / 1000.) % 60, t_time % 1000);
						struct tm label_timestamp = boost::posix_time::to_tm(boost_tend);

						TDatime datime_timestamp(1900 + label_timestamp.tm_year, label_timestamp.tm_mon + 1, label_timestamp.tm_mday, label_timestamp.tm_hour, label_timestamp.tm_min, label_timestamp.tm_sec);
						labels_timeaxis.push_back(datime_timestamp);
					}

				}

				delete t_vec_errors;

			}

			f->Close();
			delete f;
			gROOT->cd();

		} else {
			Log("MonitorLAPPDSC: ReadFromFile: File " + root_filename_i.str() + " does not exist. Omit file.", v_warning, verbosity);
		}

	}

	//Set the readfromfile time variables to make sure data is not read twice for the same time window
	readfromfile_tend = timestamp;
	readfromfile_timeframe = time_frame;

}

void MonitorLAPPDSC::DrawLastFilePlots() {

	Log("MonitorLAPPDSC: DrawLastFilePlots", v_message, verbosity);

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

void MonitorLAPPDSC::UpdateMonitorPlots(std::vector<double> timeFrames, std::vector<ULong64_t> endTimes, std::vector<std::string> fileLabels, std::vector<std::vector<std::string>> plotTypes) {

	Log("MonitorLAPPDSC: UpdateMonitorPlots", v_message, verbosity);

	//-------------------------------------------------------
	//------------------UpdateMonitorPlots ------------------
	//-------------------------------------------------------

	//Draw the monitoring plots according to the specifications in the configfiles

	for (unsigned int i_time = 0; i_time < timeFrames.size(); i_time++) {

		ULong64_t zero = 0;
		if (endTimes.at(i_time) == zero)
			endTimes.at(i_time) = t_current;        //set 0 for t_file_end since we did not know what that was at the beginning of initialise

		for (unsigned int i_plot = 0; i_plot < plotTypes.at(i_time).size(); i_plot++) {
			if (plotTypes.at(i_time).at(i_plot) == "TimeEvolution")
				DrawTimeEvolutionLAPPDSC(endTimes.at(i_time), timeFrames.at(i_time), fileLabels.at(i_time));
			else {
				if (verbosity > 0)
					std::cout << "ERROR (MonitorLAPPDSC): UpdateMonitorPlots: Specified plot type -" << plotTypes.at(i_time).at(i_plot) << "- does not exist! Omit entry." << std::endl;
			}
		}
	}

}

void MonitorLAPPDSC::DrawStatus_TempHumidity() {

	Log("MonitorLAPPDSC: DrawStatus_TempHumidity", v_message, verbosity);

	//-------------------------------------------------------
	//-------------DrawStatus_TempHumidity ------------------
	//-------------------------------------------------------

	temp_humid_check = true;
	temp_humid_warning = false;

	int lappdid = lappd_SC.LAPPD_ID;

	boost::posix_time::ptime currenttime = *Epoch + boost::posix_time::time_duration(int(t_current / MSEC_to_SEC / SEC_to_MIN / MIN_to_HOUR), int(t_current / MSEC_to_SEC / SEC_to_MIN) % 60, int(t_current / MSEC_to_SEC) % 60, t_current % 1000);
	struct tm currenttime_tm = boost::posix_time::to_tm(currenttime);
	std::stringstream current_time;
	current_time << currenttime_tm.tm_year + 1900 << "/" << currenttime_tm.tm_mon + 1 << "/" << currenttime_tm.tm_mday << "-" << currenttime_tm.tm_hour << ":" << currenttime_tm.tm_min << ":" << currenttime_tm.tm_sec;

	std::stringstream title_temphum;
	title_temphum << "LAPPD Temperature / Humidity (ID "<<lappdid<<")";
	text_temphum_title->SetText(0.06, 0.9, title_temphum.str().c_str());
	std::stringstream ss_text_temp;
	ss_text_temp << "Temperature: " << lappd_SC.temperature_mon << " (" << current_time.str() << ")";
	text_temp->SetText(0.06, 0.8, ss_text_temp.str().c_str());
	text_temp->SetTextColor(1);
	if (lappd_SC.temperature_mon > limit_temperature_low) {
		Log("MonitorLAPPDSC: ERROR: Monitored temperature >>>" + std::to_string(lappd_SC.temperature_mon) + "<<< is over first alert level [" + std::to_string(limit_temperature_low) + "]!!! (LAPPD ID "+std::to_string(lappd_SC.LAPPD_ID)+")", v_error, verbosity);
		text_temp->SetTextColor(kOrange);
		temp_humid_warning = true;
	}
	if (lappd_SC.temperature_mon > limit_temperature_high) {
		Log("MonitorLAPPDSC: SEVERE ERROR: Monitored temperature >>>" + std::to_string(lappd_SC.temperature_mon) + "<<< is over second alert level [" + std::to_string(limit_temperature_high) + "]!!! (LAPPD ID "+std::to_string(lappd_SC.LAPPD_ID)+")", v_error, verbosity);
		text_temp->SetTextColor(kRed);
		temp_humid_check = false;
                m_data->CStore.Set("LAPPDSCWarningTemp",true);
                m_data->CStore.Set("LAPPDID",lappd_SC.LAPPD_ID);
		m_data->CStore.Set("LAPPDTemp",lappd_SC.temperature_mon);
	}

	std::stringstream ss_text_hum;
	ss_text_hum << "Humidity: " << lappd_SC.humidity_mon << " (" << current_time.str() << ")";
	text_hum->SetText(0.06, 0.7, ss_text_hum.str().c_str());
	text_hum->SetTextColor(1);
	if (lappd_SC.humidity_mon > limit_humidity_low) {
		Log("MonitorLAPPDSC: ERROR: Monitored humidity >>>" + std::to_string(lappd_SC.humidity_mon) + "<<< is over first alert level [" + std::to_string(limit_humidity_low) + "]!!! (LAPPD ID "+std::to_string(lappd_SC.LAPPD_ID)+")", v_error, verbosity);
		text_hum->SetTextColor(kOrange);
		temp_humid_warning = true;
	}
	if (lappd_SC.humidity_mon > limit_humidity_high) {
		Log("MonitorLAPPDSC: SEVERE ERROR: Monitored humidity >>>" + std::to_string(lappd_SC.humidity_mon) + "<<< is over second alert level [" + std::to_string(limit_humidity_high) + "]!!! (LAPPD ID "+std::to_string(lappd_SC.LAPPD_ID)+")", v_error, verbosity);
		text_hum->SetTextColor(kRed);
		temp_humid_check = false;
                m_data->CStore.Set("LAPPDSCWarningHum",true);
                m_data->CStore.Set("LAPPDID",lappd_SC.LAPPD_ID);
                m_data->CStore.Set("LAPPDHum",lappd_SC.humidity_mon);
	}

	std::stringstream ss_text_thermistor;
	double thermistor_temp = 0.;
	if (lappd_SC.temperature_thermistor>0) thermistor_temp = TMath::Log(lappd_SC.temperature_thermistor/1000./29.4)/(-0.0437);
	ss_text_thermistor << "Thermistor: " << thermistor_temp << " deg C (" << current_time.str() << ")";
	text_thermistor->SetText(0.06, 0.6, ss_text_thermistor.str().c_str());
	text_thermistor->SetTextColor(1);
	if (lappd_SC.temperature_thermistor < limit_thermistor_temperature_low) {
		Log("MonitorLAPPDSC: ERROR: Monitored thermistor resistance >>>" + std::to_string(lappd_SC.temperature_thermistor) + "<<< is below first alert level [" + std::to_string(limit_thermistor_temperature_low) + "]!!! (LAPPD ID "+std::to_string(lappd_SC.LAPPD_ID)+")", v_error, verbosity);
		text_thermistor->SetTextColor(kOrange);
		temp_humid_warning = true;
	}
	if (lappd_SC.temperature_thermistor < limit_thermistor_temperature_high) {
		Log("MonitorLAPPDSC: SEVERE ERROR: Monitored thermistor resistance >>>" + std::to_string(lappd_SC.temperature_thermistor) + "<<< is below second alert level [" + std::to_string(limit_thermistor_temperature_high) + "]!!! (LAPPD ID "+std::to_string(lappd_SC.LAPPD_ID)+")", v_error, verbosity);
		text_thermistor->SetTextColor(kRed);
		temp_humid_check = false;
                m_data->CStore.Set("LAPPDSCWarningThermistor",true);
                m_data->CStore.Set("LAPPDID",lappd_SC.LAPPD_ID);
                m_data->CStore.Set("LAPPDThermistor",lappd_SC.temperature_thermistor);
	}

	std::stringstream ss_text_salt;
	ss_text_salt << "Salt bridge: " << lappd_SC.saltbridge << " (" << current_time.str() << ")";
	text_salt->SetText(0.06, 0.5, ss_text_salt.str().c_str());
	text_salt->SetTextColor(1);
	if (lappd_SC.saltbridge < limit_salt_low) {
		Log("MonitorLAPPDSC: ERROR: Monitored salt-bridge value >>>" + std::to_string(lappd_SC.saltbridge) + "<<< is below first alert level [" + std::to_string(limit_salt_low) + "]!!! (LAPPD ID "+std::to_string(lappd_SC.LAPPD_ID)+")", v_error, verbosity);
		text_salt->SetTextColor(kOrange);
		temp_humid_warning = true;
	}
	if (lappd_SC.saltbridge < limit_salt_high) {
		Log("MonitorLAPPDSC: SEVERE ERROR: Monitored salt-bridge value >>>" + std::to_string(lappd_SC.saltbridge) + "<<< is below second alert level [" + std::to_string(limit_salt_high) + "]!!! (LAPPD ID "+std::to_string(lappd_SC.LAPPD_ID)+")", v_error, verbosity);
		text_salt->SetTextColor(kRed);
		temp_humid_check = false;
                m_data->CStore.Set("LAPPDSCWarningSalt",true);
                m_data->CStore.Set("LAPPDID",lappd_SC.LAPPD_ID);
                m_data->CStore.Set("LAPPDSalt",lappd_SC.saltbridge);
	}
	if (lappd_SC.saltbridge > 630000.) {
		Log("MonitorLAPPDSC: SEVERE ERROR: Monitored salt-bridge value >>>" + std::to_string(lappd_SC.saltbridge) + "<<< is above first alert level [630,000]!!! (LAPPD ID "+std::to_string(lappd_SC.LAPPD_ID)+")", v_error, verbosity);
		text_salt->SetTextColor(kOrange);
		temp_humid_warning = true;
	}

	std::stringstream ss_text_light;
	ss_text_light << "Light level: " << lappd_SC.light << " (" << current_time.str() << ")";
	text_light->SetText(0.06, 0.4, ss_text_light.str().c_str());
	text_light->SetTextColor(1);

	std::stringstream ss_text_flag_temp;
	std::string emergency_temp = "False";
	if (lappd_SC.FLAG_temperature == 1) emergency_temp = "WATCH (1)";
	else if (lappd_SC.FLAG_temperature == 2) emergency_temp = "SHUT OFF (2)";
	else if (lappd_SC.FLAG_temperature == 3) emergency_temp = "PANIC (3)";
	//std::string emergency_temp = (lappd_SC.FLAG_temperature) ? "True" : "False";
	ss_text_flag_temp << "Emergency Temperature: " << emergency_temp << " (" << current_time.str() << ")";
	text_flag_temp->SetText(0.06, 0.3, ss_text_flag_temp.str().c_str());
	text_flag_temp->SetTextColor(1);
	if (lappd_SC.FLAG_temperature > 0) {
		Log("MonitorLAPPDSC: SEVERE ERROR: Temperature emergency flag is set to >>> "+std::to_string(lappd_SC.FLAG_temperature)+"<<<< ("+emergency_temp+") !!! (LAPPD ID "+std::to_string(lappd_SC.LAPPD_ID)+")", v_error, verbosity);
		if (lappd_SC.FLAG_temperature == 1) text_flag_temp->SetTextColor(kOrange);
		else text_flag_temp->SetTextColor(kRed);
		if (lappd_SC.FLAG_temperature == 1) temp_humid_warning = true;
		else temp_humid_check = false;
	}

	std::stringstream ss_text_flag_hum;
	std::string emergency_hum = "False";
	if (lappd_SC.FLAG_humidity == 1) emergency_hum = "WATCH (1)";
	else if (lappd_SC.FLAG_humidity == 2) emergency_hum = "SHUT OFF (2)";
	else if (lappd_SC.FLAG_humidity == 3) emergency_hum = "PANIC (3)";
	//std::string emergency_hum = (lappd_SC.FLAG_humidity) ? "True" : "False";
	ss_text_flag_hum << "Emergency Humidity: " << emergency_hum << " (" << current_time.str() << ")";
	text_flag_hum->SetText(0.06, 0.2, ss_text_flag_hum.str().c_str());
	text_flag_hum->SetTextColor(1);
	if (lappd_SC.FLAG_humidity > 0) {
		Log("MonitorLAPPDSC: SEVERE ERROR: Humidity emergency flag is set to >>> " +std::to_string(lappd_SC.FLAG_humidity) + " <<<< (" + emergency_hum + ") !!! (LAPPD ID "+std::to_string(lappd_SC.LAPPD_ID)+")", v_error, verbosity);
		if (lappd_SC.FLAG_humidity == 1) text_flag_hum->SetTextColor(kOrange);
		else text_flag_hum->SetTextColor(kRed);
		if (lappd_SC.FLAG_humidity == 1) temp_humid_warning = true;
		else temp_humid_check = false;
	}

	std::stringstream ss_text_flag_thermistor;
	std::string emergency_thermistor = "False";
	if (lappd_SC.FLAG_temperature_Thermistor == 1) emergency_thermistor = "WATCH (1)";
	else if (lappd_SC.FLAG_temperature_Thermistor == 2) emergency_thermistor = "SHUT OFF (2)";
	else if (lappd_SC.FLAG_temperature_Thermistor == 3) emergency_thermistor = "PANIC (3)";
	//std::string emergency_thermistor = (lappd_SC.FLAG_temperature_Thermistor) ? "True" : "False";
	ss_text_flag_thermistor << "Emergency Thermistor: " << emergency_thermistor << " (" << current_time.str() << ")";
	text_flag_thermistor->SetText(0.06, 0.1, ss_text_flag_thermistor.str().c_str());
	text_flag_thermistor->SetTextColor(1);
	if (lappd_SC.FLAG_temperature_Thermistor) {
		Log("MonitorLAPPDSC: SEVERE ERROR: Thermistor emergency flag is set to >>> " +std::to_string(lappd_SC.FLAG_temperature_Thermistor) +  "<<<< (" + emergency_thermistor + ") !!! (LAPPD ID "+std::to_string(lappd_SC.LAPPD_ID)+")", v_error, verbosity);
		if (lappd_SC.FLAG_temperature_Thermistor == 1) text_flag_thermistor->SetTextColor(kOrange);
		else text_flag_thermistor->SetTextColor(kRed);
		if (lappd_SC.FLAG_temperature_Thermistor == 1) temp_humid_warning = true;
		else temp_humid_check = false;
	}

	std::stringstream ss_text_flag_salt;
	//std::string emergency_salt = (lappd_SC.FLAG_saltbridge) ? "True" : "False";
	std::string emergency_salt = "False";
	if (lappd_SC.FLAG_saltbridge == 1) emergency_salt = "WATCH (1)";
	else if (lappd_SC.FLAG_saltbridge == 2) emergency_salt = "SHUT OFF (2)";
	else if (lappd_SC.FLAG_saltbridge == 3) emergency_salt = "PANIC (3)";
	ss_text_flag_salt << "Emergency Salt-Bridge: " << emergency_salt << " (" << current_time.str() << ")";
	text_flag_salt->SetText(0.06, 0.0, ss_text_flag_salt.str().c_str());
	text_flag_salt->SetTextColor(1);
	if (lappd_SC.FLAG_saltbridge > 0) {
		Log("MonitorLAPPDSC: SEVERE ERROR: Salt-bridge emergency flag is set to >>> " +std::to_string(lappd_SC.FLAG_saltbridge) + "<<<< (" + emergency_salt + ") !!! (LAPPD ID "+std::to_string(lappd_SC.LAPPD_ID)+")", v_error, verbosity);
		if (lappd_SC.FLAG_saltbridge == 1) text_flag_salt->SetTextColor(kOrange);
		else text_flag_salt->SetTextColor(kRed);
		if (lappd_SC.FLAG_saltbridge == 1) temp_humid_warning = true;
		else temp_humid_check = false;
	}

	text_temphum_title->SetTextSize(0.05);
	text_temp->SetTextSize(0.05);
	text_hum->SetTextSize(0.05);
	text_thermistor->SetTextSize(0.05);
	text_salt->SetTextSize(0.05);
	text_light->SetTextSize(0.05);
	text_flag_temp->SetTextSize(0.05);
	text_flag_hum->SetTextSize(0.05);
	text_flag_thermistor->SetTextSize(0.05);
	text_flag_salt->SetTextSize(0.05);

	text_temphum_title->SetNDC(1);
	text_temp->SetNDC(1);
	text_hum->SetNDC(1);
	text_thermistor->SetNDC(1);
	text_salt->SetNDC(1);
	text_light->SetNDC(1);
	text_flag_temp->SetNDC(1);
	text_flag_hum->SetNDC(1);
	text_flag_thermistor->SetNDC(1);
	text_flag_salt->SetNDC(1);

	canvas_status_temphum->cd();
	canvas_status_temphum->Clear();
	text_temphum_title->Draw();
	text_temp->Draw();
	text_hum->Draw();
	text_thermistor->Draw();
	text_salt->Draw();
	text_light->Draw();
	text_flag_temp->Draw();
	text_flag_hum->Draw();
	text_flag_thermistor->Draw();
	text_flag_salt->Draw();

	std::stringstream ss_path_tempinfo;
	ss_path_tempinfo << outpath << "LAPPDSC_TempHumInfo_ID"<<lappdid<<"_current." << img_extension;
	canvas_status_temphum->SaveAs(ss_path_tempinfo.str().c_str());
	canvas_status_temphum->Clear();

}

void MonitorLAPPDSC::DrawStatus_LVHV() {

	Log("MonitorLAPPDSC: DrawStatus_LVHV", v_message, verbosity);

	//-------------------------------------------------------
	//-------------DrawStatus_LVHV --------------------------
	//-------------------------------------------------------

	lvhv_check = true;
	lvhv_warning = false;

	int lappdid = lappd_SC.LAPPD_ID;

	//I guess this part will be the same for each of the status drawings
	boost::posix_time::ptime currenttime = *Epoch + boost::posix_time::time_duration(int(t_current / MSEC_to_SEC / SEC_to_MIN / MIN_to_HOUR), int(t_current / MSEC_to_SEC / SEC_to_MIN) % 60, int(t_current / MSEC_to_SEC) % 60, t_current % 1000);
	struct tm currenttime_tm = boost::posix_time::to_tm(currenttime);
	std::stringstream current_time;
	current_time << currenttime_tm.tm_year + 1900 << "/" << currenttime_tm.tm_mon + 1 << "/" << currenttime_tm.tm_mday << "-" << currenttime_tm.tm_hour << ":" << currenttime_tm.tm_min << ":" << currenttime_tm.tm_sec;

	//Title
	std::stringstream title_lvhv;
	title_lvhv << "LAPPD HV/LV - (ID "<<lappdid<<")";
        text_lvhv_title->SetText(0.06, 0.9, title_lvhv.str().c_str());

	//HV State
	/*std::stringstream ss_text_hv_state;
	std::string temp_hv_state = (lappd_SC.HV_state_set) ? "ON" : "OFF";
	ss_text_hv_state << "HV State Set: " << temp_hv_state << " (" << current_time.str() << ")";
	text_hv_state->SetText(0.06, 0.8, ss_text_hv_state.str().c_str());
	text_hv_state->SetTextColor(1);
	if (temp_hv_state == "OFF") {
		text_hv_state->SetTextColor(kRed);
		lvhv_check = false;
	}*/

	//ToDo: These Logs only if Set State is ON and Mon State is OFF
	//Log("MonitorLAPPDSC: SEVERE ERROR: HV state is set to OFF!!!",v_error,verbosity);

	//ToDo: Which values are acceptable? Any messages to display?
	//HV Mon
	std::stringstream ss_hv_mon_temp;
	ss_hv_mon_temp << "HV State Mon: " << lappd_SC.HV_mon << " (" << current_time.str() << ")";
	text_hv_mon->SetText(0.06, 0.8, ss_hv_mon_temp.str().c_str());
	text_hv_mon->SetTextColor(1);
	/*if (lappd_SC.HV_mon == 0) {
		text_hv_mon->SetTextColor(kRed);
		lvhv_check = false;
	}*/

	//HV Set and Mon mismatch warning
	if (lappd_SC.HV_state_set && lappd_SC.HV_mon == 0) {
		Log("MonitorLAPPDSC: SEVERE ERROR: HV state is set to ON, but HV mon is OFF!!! (LAPPD ID "+std::to_string(lappd_SC.LAPPD_ID)+")", v_error, verbosity);
		text_hv_mon->SetTextColor(kRed);
		lvhv_check = false;
                m_data->CStore.Set("LAPPDSCWarningHV",true);
                m_data->CStore.Set("LAPPDID",lappd_SC.LAPPD_ID);
	}

	if (!lappd_SC.HV_state_set && lappd_SC.HV_mon > 0) {
		Log("MonitorLAPPDSC: SEVERE ERROR: HV state is set to OFF, but HV mon is ON!!! (LAPPD ID "+std::to_string(lappd_SC.LAPPD_ID)+")", v_error, verbosity);
		text_hv_mon->SetTextColor(kRed);
		lvhv_check = false;
                m_data->CStore.Set("LAPPDSCWarningHV",true);
                m_data->CStore.Set("LAPPDID",lappd_SC.LAPPD_ID);
	}

	//HV Volt
	std::stringstream ss_hv_volt_temp;
	ss_hv_volt_temp << "HV Set Volt: " << lappd_SC.HV_volts << " V (" << current_time.str() << ")";
	text_hv_volt->SetText(0.06, 0.7, ss_hv_volt_temp.str().c_str());
	text_hv_volt->SetTextColor(1);

	//HV Mon Volt
	std::stringstream ss_hv_monvolt_temp;
	ss_hv_monvolt_temp << "HV Mon Volt: "<< lappd_SC.HV_return_mon << " V (" << current_time.str() << ")";
	text_hv_monvolt->SetText(0.06, 0.6, ss_hv_monvolt_temp.str().c_str());
	text_hv_monvolt->SetTextColor(1);
	if (lappd_SC.HV_return_mon > 2400.) text_hv_monvolt->SetTextColor(kRed);


	//LV State Set
	/*std::stringstream ss_text_lv_state;
	std::string temp_lv_state = (lappd_SC.LV_state_set) ? "ON" : "OFF";
	ss_text_lv_state << "LV State Set: " << temp_lv_state << " (" << current_time.str() << ")";
	text_lv_state->SetText(0.06, 0.5, ss_text_lv_state.str().c_str());
	text_lv_state->SetTextColor(1);
	if (temp_lv_state == "OFF") {
		text_lv_state->SetTextColor(kRed);
		lvhv_check = false;
	}*/

	//LV Mon
	std::stringstream ss_lv_mon_temp;
	ss_lv_mon_temp << "LV State Mon: " << lappd_SC.LV_mon << " (" << current_time.str() << ")";
	text_lv_mon->SetText(0.06, 0.5, ss_lv_mon_temp.str().c_str());
	text_lv_mon->SetTextColor(1);
	/*if (lappd_SC.LV_mon == 0) {
		text_lv_mon->SetTextColor(kRed);
		lvhv_check = false;
	}*/

	//LV Set and Mon mismatch warning
	if (lappd_SC.LV_state_set && lappd_SC.LV_mon == 0) {
		Log("MonitorLAPPDSC: SEVERE ERROR: LV state is set to ON, but LV mon is OFF!!! (LAPPD ID "+std::to_string(lappd_SC.LAPPD_ID)+")", v_error, verbosity);
		text_lv_mon->SetTextColor(kRed);
		lvhv_check = false;
                m_data->CStore.Set("LAPPDSCWarningHV",true);
                m_data->CStore.Set("LAPPDID",lappd_SC.LAPPD_ID);
	}

	if (!lappd_SC.LV_state_set && lappd_SC.LV_mon > 0) {
		Log("MonitorLAPPDSC: SEVERE ERROR: LV state is set to OFF, but LV mon is ON!!! (LAPPD ID "+std::to_string(lappd_SC.LAPPD_ID)+")", v_error, verbosity);
		text_lv_mon->SetTextColor(kRed);
		lvhv_check = false;
                m_data->CStore.Set("LAPPDSCWarningHV",true);
                m_data->CStore.Set("LAPPDID",lappd_SC.LAPPD_ID);
	}

	//V33
	std::stringstream ss_v33_temp;
	ss_v33_temp << "V33: " << lappd_SC.v33 << " V (" << current_time.str() << ")";
	text_v33->SetText(0.06, 0.4, ss_v33_temp.str().c_str());
	text_v33->SetTextColor(1);
	if (lappd_SC.LV_state_set){
	if (lappd_SC.v33 < v33_min || lappd_SC.v33 > v33_max) {
		Log("MonitorLAPPDSC: SEVERE ERROR: Monitored v33 >>>" + std::to_string(lappd_SC.v33) + "<<< is outside of expected range [" + std::to_string(v33_min) + " , " + std::to_string(v33_max) + "]!!! (LAPPD ID "+std::to_string(lappd_SC.LAPPD_ID)+")", v_error, verbosity);
		text_v33->SetTextColor(kRed);
		lvhv_check = false;
                m_data->CStore.Set("LAPPDSCWarningLV3",true);
                m_data->CStore.Set("LAPPDID",lappd_SC.LAPPD_ID);
	}
	}

	//V25
	std::stringstream ss_v25_temp;
	ss_v25_temp << "V31: " << lappd_SC.v25 << " V (" << current_time.str() << ")";
	text_v25->SetText(0.06, 0.3, ss_v25_temp.str().c_str());
	text_v25->SetTextColor(1);
	if (lappd_SC.LV_state_set){
	if (lappd_SC.v25 < v25_min || lappd_SC.v25 > v25_max) {
		Log("MonitorLAPPDSC: SEVERE ERROR: Monitored v25 >>>" + std::to_string(lappd_SC.v25) + "<<< is outside of expected range [" + std::to_string(v25_min) + " , " + std::to_string(v25_max) + "]!!! (LAPPD ID "+std::to_string(lappd_SC.LAPPD_ID)+")", v_error, verbosity);
		text_v25->SetTextColor(kRed);
		lvhv_check = false;
                m_data->CStore.Set("LAPPDSCWarningLV2",true);
                m_data->CStore.Set("LAPPDID",lappd_SC.LAPPD_ID);
	}
	}

	//V12
	std::stringstream ss_v12_temp;
	ss_v12_temp << "V18: " << lappd_SC.v12 << " V (" << current_time.str() << ")";
	text_v12->SetText(0.06, 0.2, ss_v12_temp.str().c_str());
	text_v12->SetTextColor(1);
	if (lappd_SC.LV_state_set){
	if (lappd_SC.v12 < v12_min || lappd_SC.v12 > v12_max) {
		Log("MonitorLAPPDSC: SEVERE ERROR: Monitored v12 >>>" + std::to_string(lappd_SC.v12) + "<<< is outside of expected range [" + std::to_string(v12_min) + " , " + std::to_string(v12_max) + "]!!! (LAPPD ID "+std::to_string(lappd_SC.LAPPD_ID)+")", v_error, verbosity);
		text_v12->SetTextColor(kRed);
		lvhv_check = false;
                m_data->CStore.Set("LAPPDSCWarningLV1",true);
                m_data->CStore.Set("LAPPDID",lappd_SC.LAPPD_ID);
	}
	}
	if (lappd_SC.LV_mon == 0){
		if (lappd_SC.v12 > 0.1){
			Log("MonitorLAPPDSC: SEVERE ERROR: Monitored v12 >>>" + std::to_string(lappd_SC.v12) + "<<< is above 0.1V although LV state (mon) is OFF!!! (LAPPD ID "+std::to_string(lappd_SC.LAPPD_ID)+")", v_error, verbosity);
			text_v12->SetTextColor(kRed);
			lvhv_check = false;
                	m_data->CStore.Set("LAPPDSCWarningLV",true);
                	m_data->CStore.Set("LAPPDID",lappd_SC.LAPPD_ID);
		}
		if (lappd_SC.v25 > 0.1){
			Log("MonitorLAPPDSC: SEVERE ERROR: Monitored v25 >>>" + std::to_string(lappd_SC.v25) + "<<< is above 0.1V although LV state (mon) is OFF!!! (LAPPD ID "+std::to_string(lappd_SC.LAPPD_ID)+")", v_error, verbosity);
			text_v25->SetTextColor(kRed);
			lvhv_check = false;
                        m_data->CStore.Set("LAPPDSCWarningLV",true);
                	m_data->CStore.Set("LAPPDID",lappd_SC.LAPPD_ID);
		}
		if (lappd_SC.v33 > 0.1){
			Log("MonitorLAPPDSC: SEVERE ERROR: Monitored v33 >>>" + std::to_string(lappd_SC.v33) + "<<< is above 0.1V although LV state (mon) is OFF!!! (LAPPD ID "+std::to_string(lappd_SC.LAPPD_ID)+")", v_error, verbosity);
			text_v33->SetTextColor(kRed);
			lvhv_check = false;
                        m_data->CStore.Set("LAPPDSCWarningLV",true);
                	m_data->CStore.Set("LAPPDID",lappd_SC.LAPPD_ID);
		}
	}

	//Actual drawing
	text_lvhv_title->SetTextSize(0.05);
	text_hv_state->SetTextSize(0.05);
	text_hv_mon->SetTextSize(0.05);
	text_hv_volt->SetTextSize(0.05);
	text_lv_state->SetTextSize(0.05);
	text_lv_mon->SetTextSize(0.05);
	text_v33->SetTextSize(0.05);
	text_v25->SetTextSize(0.05);
	text_v12->SetTextSize(0.05);

	text_lvhv_title->SetNDC(1);
	text_hv_state->SetNDC(1);
	text_hv_mon->SetNDC(1);
	text_hv_volt->SetNDC(1);
	text_lv_state->SetNDC(1);
	text_lv_mon->SetNDC(1);
	text_v33->SetNDC(1);
	text_v25->SetNDC(1);
	text_v12->SetNDC(1);

	canvas_status_lvhv->cd();
	canvas_status_lvhv->Clear();
	text_lvhv_title->Draw();
	text_hv_state->Draw();
	text_hv_mon->Draw();
	text_hv_volt->Draw();
	text_hv_monvolt->Draw();
	text_lv_state->Draw();
	text_lv_mon->Draw();
	text_v33->Draw();
	text_v25->Draw();
	text_v12->Draw();

	std::stringstream ss_path_lvhvinfo;
	ss_path_lvhvinfo << outpath << "LAPPDSC_LVHVinfo_ID"<<lappdid<<"_current." << img_extension;
	canvas_status_lvhv->SaveAs(ss_path_lvhvinfo.str().c_str());
	canvas_status_lvhv->Clear();

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
	 t_hum = lappd_SC.humidity_mon;
	 t_temp = lappd_SC.temperature_mon;
	 t_hvmon = lappd_SC.HV_mon;
	 t_hvstateset = lappd_SC.HV_state_set;
	 t_hvvolt = lappd_SC.HV_volts;
	 t_lvmon = lappd_SC.LV_mon;
	 t_lvstateset = lappd_SC.LV_state_set;
	 t_v33 = lappd_SC.v33;
	 t_v25 = lappd_SC.v25;
	 t_v12 = lappd_SC.v12;
	 t_temp_low = lappd_SC.LIMIT_temperature_low;
	 t_temp_high = lappd_SC.LIMIT_temperature_high;
	 t_hum_low = lappd_SC.LIMIT_humidity_low;
	 t_hum_high = lappd_SC.LIMIT_humidity_high;
	 t_flag_temp = lappd_SC.FLAG_temperature;
	 t_flag_humidity = lappd_SC.FLAG_humidity;
	 t_relCh1 = lappd_SC.relayCh1;
	 t_relCh2 = lappd_SC.relayCh2;
	 t_relCh3 = lappd_SC.relayCh3;
	 t_relCh1_mon = lappd_SC.relayCh1_mon;
	 t_relCh2_mon = lappd_SC.relayCh2_mon;
	 t_relCh3_mon = lappd_SC.relayCh3_mon;
	 t_trig_vref = lappd_SC.TrigVref;
	 t_trig1_thr = lappd_SC.Trig1_threshold;
	 t_trig1_mon = lappd_SC.Trig1_mon;
	 t_trig0_thr = lappd_SC.Trig0_threshold;
	 t_trig_mon = lappd_SC.Trig0_mon;
	 t_light_level = lappd_SC.light;
	 */

}

void MonitorLAPPDSC::DrawStatus_Trigger() {

	Log("MonitorLAPPDSC: DrawStatus_Trigger", v_message, verbosity);
	
	trigger_check = true;
	trigger_warning = false;

	int lappdid = lappd_SC.LAPPD_ID;

	//-------------------------------------------------------
	//-------------DrawStatus_Trigger -----------------------
	//-------------------------------------------------------

	boost::posix_time::ptime currenttime = *Epoch + boost::posix_time::time_duration(int(t_current / MSEC_to_SEC / SEC_to_MIN / MIN_to_HOUR), int(t_current / MSEC_to_SEC / SEC_to_MIN) % 60, int(t_current / MSEC_to_SEC) % 60, t_current % 1000);
	struct tm currenttime_tm = boost::posix_time::to_tm(currenttime);
	std::stringstream current_time;
	current_time << currenttime_tm.tm_year + 1900 << "/" << currenttime_tm.tm_mon + 1 << "/" << currenttime_tm.tm_mday << "-" << currenttime_tm.tm_hour << ":" << currenttime_tm.tm_min << ":" << currenttime_tm.tm_sec;

	//Title
	std::stringstream title_trigger;
	title_trigger << "LAPPD Trigger (ID "<<lappdid<<")";
        text_trigger_title->SetText(0.06, 0.9, title_trigger.str().c_str());

	//Trigger Vref
	std::stringstream ss_trigger_vref_temp;
	ss_trigger_vref_temp << "Trigger Vref: " << lappd_SC.TrigVref << " V (" << current_time.str() << ")";
	text_trigger_vref->SetText(0.06, 0.8, ss_trigger_vref_temp.str().c_str());
	text_trigger_vref->SetTextColor(1);

	//Trig0 Thr
	std::stringstream ss_trigger_trig0_thr_temp;
	ss_trigger_trig0_thr_temp << "Trig0 Thr: " << lappd_SC.Trig0_threshold << " V (" << current_time.str() << ")";
	text_trigger_trig0_thr->SetText(0.06, 0.7, ss_trigger_trig0_thr_temp.str().c_str());
	text_trigger_trig0_thr->SetTextColor(1);

	//Trig0 Mon
	std::stringstream ss_trigger_trig0_mon_temp;
	ss_trigger_trig0_mon_temp << "Trig0 Mon: " << lappd_SC.Trig0_mon << " V (" << current_time.str() << ")";
	text_trigger_trig0_mon->SetText(0.06, 0.6, ss_trigger_trig0_mon_temp.str().c_str());
	text_trigger_trig0_mon->SetTextColor(1);

	//Trig1 Thr
	std::stringstream ss_trigger_trig1_thr_temp;
	ss_trigger_trig1_thr_temp << "Trig1 Thr: " << lappd_SC.Trig1_threshold << " V (" << current_time.str() << ")";
	text_trigger_trig1_thr->SetText(0.06, 0.5, ss_trigger_trig1_thr_temp.str().c_str());
	text_trigger_trig1_thr->SetTextColor(1);

	//Trig1 Mon
	std::stringstream ss_trigger_trig1_mon_temp;
	ss_trigger_trig1_mon_temp << "Trig1 Mon: " << lappd_SC.Trig1_mon << " V (" << current_time.str() << ")";
	text_trigger_trig1_mon->SetText(0.06, 0.4, ss_trigger_trig1_mon_temp.str().c_str());
	text_trigger_trig1_mon->SetTextColor(1);

	//Actual drawing
	text_trigger_title->SetTextSize(0.05);
	text_trigger_vref->SetTextSize(0.05);
	text_trigger_trig0_thr->SetTextSize(0.05);
	text_trigger_trig0_mon->SetTextSize(0.05);
	text_trigger_trig1_thr->SetTextSize(0.05);
	text_trigger_trig1_mon->SetTextSize(0.05);

	text_trigger_title->SetNDC(1);
	text_trigger_vref->SetNDC(1);
	text_trigger_trig0_thr->SetNDC(1);
	text_trigger_trig0_mon->SetNDC(1);
	text_trigger_trig1_thr->SetNDC(1);
	text_trigger_trig1_mon->SetNDC(1);

	canvas_status_trigger->cd();
	canvas_status_trigger->Clear();
	text_trigger_title->Draw();
	text_trigger_vref->Draw();
	text_trigger_trig0_thr->Draw();
	text_trigger_trig0_mon->Draw();
	text_trigger_trig1_thr->Draw();
	text_trigger_trig1_mon->Draw();

	std::stringstream ss_path_triggerinfo;
	ss_path_triggerinfo << outpath << "LAPPDSC_triggerinfo_ID"<<lappdid<<"_current." << img_extension;
	canvas_status_trigger->SaveAs(ss_path_triggerinfo.str().c_str());
	canvas_status_trigger->Clear();

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
	 t_trig_vref = lappd_SC.TrigVref;
	 t_trig1_thr = lappd_SC.Trig1_threshold;
	 t_trig1_mon = lappd_SC.Trig1_mon;
	 t_trig0_thr = lappd_SC.Trig0_threshold;
	 t_trig_mon = lappd_SC.Trig0_mon;
	 */

}

void MonitorLAPPDSC::DrawStatus_Relay() {

	//ToDo: Adjust to write True and False

	Log("MonitorLAPPDSC: DrawStatus_Relay", v_message, verbosity);

	//-------------------------------------------------------
	//-------------DrawStatus_Relay ------------------------
	//-------------------------------------------------------

	relay_check = true;
	relay_warning = false;

	int lappdid = lappd_SC.LAPPD_ID;

	boost::posix_time::ptime currenttime = *Epoch + boost::posix_time::time_duration(int(t_current / MSEC_to_SEC / SEC_to_MIN / MIN_to_HOUR), int(t_current / MSEC_to_SEC / SEC_to_MIN) % 60, int(t_current / MSEC_to_SEC) % 60, t_current % 1000);
	struct tm currenttime_tm = boost::posix_time::to_tm(currenttime);
	std::stringstream current_time;
	current_time << currenttime_tm.tm_year + 1900 << "/" << currenttime_tm.tm_mon + 1 << "/" << currenttime_tm.tm_mday << "-" << currenttime_tm.tm_hour << ":" << currenttime_tm.tm_min << ":" << currenttime_tm.tm_sec;

	//Title
	std::stringstream title_relay;
	title_relay << "LAPPD Relays (ID "<<lappdid<<")";
        text_relay_title->SetText(0.06, 0.9, title_relay.str().c_str());

	//Relay 1
	std::stringstream ss_relay_ch1_temp;
	std::string ch1_temp = (lappd_SC.relayCh1) ? "True" : "False";
	ss_relay_ch1_temp << "Relay 1 Set: " << ch1_temp << " (" << current_time.str() << ")";
	text_relay_set1->SetText(0.06, 0.8, ss_relay_ch1_temp.str().c_str());
	text_relay_set1->SetTextColor(1);
	/*if (!lappd_SC.relayCh1) {
		text_relay_set1->SetTextColor(kRed);
		relay_check = false;
	}*/

	//Relay 2
	std::stringstream ss_relay_ch2_temp;
	std::string ch2_temp = (lappd_SC.relayCh2) ? "True" : "False";
	ss_relay_ch2_temp << "Relay 2 Set: " << ch2_temp << " (" << current_time.str() << ")";
	text_relay_set2->SetText(0.06, 0.7, ss_relay_ch2_temp.str().c_str());
	text_relay_set2->SetTextColor(1);
	/*if (!lappd_SC.relayCh2) {
		text_relay_set2->SetTextColor(kRed);
		relay_check = false;
	}*/

	//Relay 3
	std::stringstream ss_relay_ch3_temp;
	std::string ch3_temp = (lappd_SC.relayCh3) ? "True" : "False";
	ss_relay_ch3_temp << "Relay 3 Set: " << ch3_temp << " (" << current_time.str() << ")";
	text_relay_set3->SetText(0.06, 0.6, ss_relay_ch3_temp.str().c_str());
	text_relay_set3->SetTextColor(1);
	/*if (!lappd_SC.relayCh3) {
		text_relay_set3->SetTextColor(kRed);
		relay_check = false;
	}*/

	//Relay Mon 1
	std::stringstream ss_relay_mon1_temp;
	std::string ch1_mon_temp = (lappd_SC.relayCh1_mon) ? "True" : "False";
	ss_relay_mon1_temp << "Relay 1 Mon: " << ch1_mon_temp << " (" << current_time.str() << ")";
	text_relay_mon1->SetText(0.06, 0.5, ss_relay_mon1_temp.str().c_str());
	text_relay_mon1->SetTextColor(1);
	if (lappd_SC.relayCh1_mon != lappd_SC.relayCh1) {
		text_relay_mon1->SetTextColor(kRed);
		relay_check = false;
                m_data->CStore.Set("LAPPDSCWarningRelay",true);
                m_data->CStore.Set("LAPPDID",lappd_SC.LAPPD_ID);
	}

	//Relay Mon 2
	std::stringstream ss_relay_mon2_temp;
	std::string ch2_mon_temp = (lappd_SC.relayCh2_mon) ? "True" : "False";
	ss_relay_mon2_temp << "Relay 2 Mon: " << ch2_mon_temp << " (" << current_time.str() << ")";
	text_relay_mon2->SetText(0.06, 0.4, ss_relay_mon2_temp.str().c_str());
	text_relay_mon2->SetTextColor(1);
	if (lappd_SC.relayCh2_mon != lappd_SC.relayCh2) {
		text_relay_mon2->SetTextColor(kRed);
		relay_check = false;
                m_data->CStore.Set("LAPPDSCWarningRelay",true);
                m_data->CStore.Set("LAPPDID",lappd_SC.LAPPD_ID);
	}

	//Relay Mon 3
	std::stringstream ss_relay_mon3_temp;
	std::string ch3_mon_temp = (lappd_SC.relayCh3_mon) ? "True" : "False";
	ss_relay_mon3_temp << "Relay 3 Mon: " << ch3_mon_temp << " (" << current_time.str() << ")";
	text_relay_mon3->SetText(0.06, 0.3, ss_relay_mon3_temp.str().c_str());
	text_relay_mon3->SetTextColor(1);
	if (lappd_SC.relayCh3_mon != lappd_SC.relayCh3) {
		text_relay_mon3->SetTextColor(kRed);
		relay_check = false;
                m_data->CStore.Set("LAPPDSCWarningRelay",true);
                m_data->CStore.Set("LAPPDID",lappd_SC.LAPPD_ID);
	}

	//Warnings
	if (lappd_SC.relayCh1 && !lappd_SC.relayCh1_mon) {
		Log("MonitorLAPPDSC: SEVERE ERROR: Relay 1 state is set to ON, but relay mon 1 is OFF!!! (LAPPD ID "+std::to_string(lappd_SC.LAPPD_ID)+")", v_error, verbosity);
	}

	if (!lappd_SC.relayCh1 && lappd_SC.relayCh1_mon) {
		Log("MonitorLAPPDSC: SEVERE ERROR: Relay 1 state is set to OFF, but relay mon 1 is ON!!! (LAPPD ID "+std::to_string(lappd_SC.LAPPD_ID)+")", v_error, verbosity);
	}

	//Warnings
	if (lappd_SC.relayCh2 && !lappd_SC.relayCh2_mon) {
		Log("MonitorLAPPDSC: SEVERE ERROR: Relay 2 state is set to ON, but relay mon 2 is OFF!!! (LAPPD ID "+std::to_string(lappd_SC.LAPPD_ID)+")", v_error, verbosity);
	}

	if (!lappd_SC.relayCh2 && lappd_SC.relayCh2_mon) {
		Log("MonitorLAPPDSC: SEVERE ERROR: Relay 2 state is set to OFF, but relay mon 2 is ON!!! (LAPPD ID "+std::to_string(lappd_SC.LAPPD_ID)+")", v_error, verbosity);
	}

	//Warnings
	if (lappd_SC.relayCh3 && !lappd_SC.relayCh3_mon) {
		Log("MonitorLAPPDSC: SEVERE ERROR: Relay 3 state is set to ON, but relay mon 3 is OFF!!! (LAPPD ID "+std::to_string(lappd_SC.LAPPD_ID)+")", v_error, verbosity);
	}

	if (!lappd_SC.relayCh3 && lappd_SC.relayCh3_mon) {
		Log("MonitorLAPPDSC: SEVERE ERROR: Relay 3 state is set to OFF, but relay mon 3 is ON!!! (LAPPD ID "+std::to_string(lappd_SC.LAPPD_ID)+")", v_error, verbosity);
	}

	//Actual drawing
	text_relay_title->SetTextSize(0.05);
	text_relay_set1->SetTextSize(0.05);
	text_relay_set2->SetTextSize(0.05);
	text_relay_set3->SetTextSize(0.05);
	text_relay_mon1->SetTextSize(0.05);
	text_relay_mon2->SetTextSize(0.05);
	text_relay_mon3->SetTextSize(0.05);

	text_relay_title->SetNDC(1);
	text_relay_set1->SetNDC(1);
	text_relay_set2->SetNDC(1);
	text_relay_set3->SetNDC(1);
	text_relay_mon1->SetNDC(1);
	text_relay_mon2->SetNDC(1);
	text_relay_mon3->SetNDC(1);

	canvas_status_relay->cd();
	canvas_status_relay->Clear();
	text_relay_title->Draw();
	text_relay_set1->Draw();
	text_relay_set2->Draw();
	text_relay_set3->Draw();
	text_relay_mon1->Draw();
	text_relay_mon2->Draw();
	text_relay_mon3->Draw();

	std::stringstream ss_path_relayinfo;
	ss_path_relayinfo << outpath << "LAPPDSC_relayinfo_ID"<<lappdid<<"_current." << img_extension;
	canvas_status_relay->SaveAs(ss_path_relayinfo.str().c_str());
	canvas_status_relay->Clear();

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
	 t_relCh1 = lappd_SC.relayCh1;
	 t_relCh2 = lappd_SC.relayCh2;
	 t_relCh3 = lappd_SC.relayCh3;
	 t_relCh1_mon = lappd_SC.relayCh1_mon;
	 t_relCh2_mon = lappd_SC.relayCh2_mon;
	 t_relCh3_mon = lappd_SC.relayCh3_mon;
	 */
}

void MonitorLAPPDSC::DrawStatus_Errors() {

	Log("MonitorLAPPDSC: DrawStatus_Errors", v_message, verbosity);

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

	//Title

	std::vector<unsigned int> errorCodes = lappd_SC.errorcodes;

	error_check = true;
	error_warning = false;

	int lappdid = lappd_SC.LAPPD_ID;
	std::stringstream title_error;
        title_error << "LAPPD Errors (ID "<<lappdid<<")";
        text_error_title->SetText(0.06, 0.9, title_error.str().c_str());

	//Always write all the errors to the log (unless there's only one error = 0x0000), then all good
	if (!(errorCodes.size()==1 && errorCodes.at(0) == 0)){
	for (size_t i_error = 0 ; i_error < errorCodes.size(); i_error++) {
		boost::posix_time::ptime currenttime = *Epoch + boost::posix_time::time_duration(int(t_current / MSEC_to_SEC / SEC_to_MIN / MIN_to_HOUR), int(t_current / MSEC_to_SEC / SEC_to_MIN) % 60, int(t_current / MSEC_to_SEC) % 60, t_current % 1000);
		struct tm currenttime_tm = boost::posix_time::to_tm(currenttime);
		std::stringstream current_time;
		current_time << currenttime_tm.tm_year + 1900 << "/" << currenttime_tm.tm_mon + 1 << "/" << currenttime_tm.tm_mday << "-" << currenttime_tm.tm_hour << ":" << currenttime_tm.tm_min << ":" << currenttime_tm.tm_sec;

		std::stringstream ss_errorcode_hex;
		ss_errorcode_hex << "0x" << std::hex << errorCodes.at(errorCodes.size()-1-i_error);
		std::string errorMessage = "MonitorLAPPDSC: SEVERE ERROR: LAPPD ID "+std::to_string(lappd_SC.LAPPD_ID)+", Error Code: " + ss_errorcode_hex.str() + " timestamp " + current_time.str();
		Log(errorMessage, v_error, verbosity);
	}
	}

	//If we have too many errors to show them all, just display one warning.
	if (errorCodes.size() > 7) {
		boost::posix_time::ptime currenttime = *Epoch + boost::posix_time::time_duration(int(t_current / MSEC_to_SEC / SEC_to_MIN / MIN_to_HOUR), int(t_current / MSEC_to_SEC / SEC_to_MIN) % 60, int(t_current / MSEC_to_SEC) % 60, t_current % 1000);
		struct tm currenttime_tm = boost::posix_time::to_tm(currenttime);
		std::stringstream current_time;
		current_time << currenttime_tm.tm_year + 1900 << "/" << currenttime_tm.tm_mon + 1 << "/" << currenttime_tm.tm_mday << "-" << currenttime_tm.tm_hour << ":" << currenttime_tm.tm_min << ":" << currenttime_tm.tm_sec;

		canvas_status_error->cd();
			canvas_status_error->Clear();
			text_error_title->SetTextSize(0.05);
			text_error_title->SetNDC(1);
			text_error_title->Draw();
			for (size_t i_error = 0 ; i_error < 7; i_error++) {
				std::stringstream ss_error_temp;
				std::stringstream ss_errorcode_hex;
                		ss_errorcode_hex << "0x" << std::hex << errorCodes.at(errorCodes.size()-1-i_error);
				ss_error_temp << "Error with Code " << ss_errorcode_hex.str() << " (" << current_time.str() << ")";
				text_error_vector.at(i_error)->SetText(0.06, 0.8 - (i_error*0.1), ss_error_temp.str().c_str());
				text_error_vector.at(i_error)->SetTextColor(kRed);
				text_error_vector.at(i_error)->SetTextSize(0.05);
				text_error_vector.at(i_error)->SetNDC(1);
				text_error_vector.at(i_error)->Draw();
			}






		std::stringstream ss_error_end_temp;
		int numberOfHiddenErrors = errorCodes.size() - 7;
		ss_error_end_temp << "Too many errors to display! " << std::to_string(numberOfHiddenErrors) << " Errors are not shown.";
		text_error_number9->SetText(0.06, 0.1, ss_error_end_temp.str().c_str());
		text_error_number9->SetTextColor(kRed);

		text_error_number9->SetTextSize(0.05);
		text_error_number9->SetNDC(1);
		text_error_number9->Draw();

		error_check = false;

	} else {
		boost::posix_time::ptime currenttime = *Epoch + boost::posix_time::time_duration(int(t_current / MSEC_to_SEC / SEC_to_MIN / MIN_to_HOUR), int(t_current / MSEC_to_SEC / SEC_to_MIN) % 60, int(t_current / MSEC_to_SEC) % 60, t_current % 1000);
		struct tm currenttime_tm = boost::posix_time::to_tm(currenttime);
		std::stringstream current_time;
		current_time << currenttime_tm.tm_year + 1900 << "/" << currenttime_tm.tm_mon + 1 << "/" << currenttime_tm.tm_mday << "-" << currenttime_tm.tm_hour << ":" << currenttime_tm.tm_min << ":" << currenttime_tm.tm_sec;

		if(errorCodes.size() == 0 || (errorCodes.size() == 1 && errorCodes.at(0) == 0)){
			std::stringstream ss_error_1_temp;
			ss_error_1_temp << "No errors! (" << current_time.str() << ")";
			text_error_number1->SetText(0.06, 0.8, ss_error_1_temp.str().c_str());
			text_error_number1->SetTextColor(kGreen);

			text_error_title->SetTextSize(0.05);
			text_error_number1->SetTextSize(0.05);
			text_error_title->SetNDC(1);
			text_error_number1->SetNDC(1);
			canvas_status_error->cd();
			canvas_status_error->Clear();
			text_error_title->Draw();
			text_error_number1->Draw();
			error_check = true;
		}
		else{
			canvas_status_error->cd();
			canvas_status_error->Clear();
			text_error_title->SetTextSize(0.05);
			text_error_title->SetNDC(1);
			text_error_title->Draw();
			for (size_t i_error = 0 ; i_error < errorCodes.size(); i_error++) {
				std::stringstream ss_error_temp;
				std::stringstream ss_errorcode_hex;
                		ss_errorcode_hex << "0x" << std::hex << errorCodes.size()-1-i_error;
				ss_error_temp << "Error with Code " << ss_errorcode_hex.str() << " (" << current_time.str() << ")";
				text_error_vector.at(i_error)->SetText(0.06, 0.8 - (i_error*0.1), ss_error_temp.str().c_str());
				text_error_vector.at(i_error)->SetTextColor(kRed);
				text_error_vector.at(i_error)->SetTextSize(0.05);
				text_error_vector.at(i_error)->SetNDC(1);
				text_error_vector.at(i_error)->Draw();
			}
			error_check = false;

		}
	}

	std::stringstream ss_path_errorinfo;
	ss_path_errorinfo << outpath << "LAPPDSC_errorinfo_ID"<<lappdid<<"_current." << img_extension;
	canvas_status_error->SaveAs(ss_path_errorinfo.str().c_str());
	canvas_status_error->Clear();

}

void MonitorLAPPDSC::DrawStatus_Overview() {

	Log("MonitorLAPPDSC: DrawStatus_Overview", v_message, verbosity);

	//--------------------------------------------------------
	//-------------DrawStatus_Overview -----------------------
	// -------------------------------------------------------

	// Potential TODO (low priority): One status board with all status variables of the Slow Control
	// Could use green circles to indicate good behavior, red circles for bad behavior
	// Can be done in case everything else is finished (optional)

	boost::posix_time::ptime currenttime = *Epoch + boost::posix_time::time_duration(int(t_current / MSEC_to_SEC / SEC_to_MIN / MIN_to_HOUR), int(t_current / MSEC_to_SEC / SEC_to_MIN) % 60, int(t_current / MSEC_to_SEC) % 60, t_current % 1000);
	struct tm currenttime_tm = boost::posix_time::to_tm(currenttime);
	std::stringstream current_time;
	current_time << currenttime_tm.tm_year + 1900 << "/" << currenttime_tm.tm_mon + 1 << "/" << currenttime_tm.tm_mday << "-" << currenttime_tm.tm_hour << ":" << currenttime_tm.tm_min << ":" << currenttime_tm.tm_sec;

	int lappdid = lappd_SC.LAPPD_ID;

	//Title
	std::stringstream title_overview;
	title_overview << "LAPPD Overview (ID "<<lappdid<<")";
        text_overview_title->SetText(0.06, 0.9, title_overview.str().c_str());

	std::stringstream ss_temphum_temp;
	std::string temphum = (temp_humid_check) ? "Everything is awesome!" : "Something is wrong!";
	ss_temphum_temp << "Temperature/Humidity Status: " << temphum << " (" << current_time.str() << ")";
	text_overview_temp->SetText(0.06, 0.8, ss_temphum_temp.str().c_str());
	text_overview_temp->SetTextColor(1);
	if (!temp_humid_check) {
		text_overview_temp->SetTextColor(kRed);
	}
	else if (temp_humid_warning) {
		text_overview_temp->SetTextColor(kOrange);
	}
	else{
		text_overview_temp->SetTextColor(kGreen);
	}

	std::stringstream ss_lvhv_temp;
	std::string lvhv = (lvhv_check) ? "Everything is awesome!" : "Something is wrong!";
	ss_lvhv_temp << "LVHV Status: " << lvhv << " (" << current_time.str() << ")";
	text_overview_lvhv->SetText(0.06, 0.7, ss_lvhv_temp.str().c_str());
	text_overview_lvhv->SetTextColor(1);
	if (!lvhv_check) {
		text_overview_lvhv->SetTextColor(kRed);
	}
	else if (lvhv_warning) {
		text_overview_lvhv->SetTextColor(kOrange);
	}
	else{
		text_overview_lvhv->SetTextColor(kGreen);
	}

	std::stringstream ss_trigger_temp;
	std::string trigger = (trigger_check) ? "Everything is awesome!" : "Something is wrong!";
	ss_trigger_temp << "Trigger Status: " << trigger << " (" << current_time.str() << ")";
	text_overview_trigger->SetText(0.06, 0.6, ss_trigger_temp.str().c_str());
	text_overview_trigger->SetTextColor(1);
	if (!trigger_check) {
		text_overview_trigger->SetTextColor(kRed);
	}
	else if (trigger_warning){
		text_overview_trigger->SetTextColor(kOrange);
	}
	else{
		text_overview_trigger->SetTextColor(kGreen);
	}

	std::stringstream ss_relay_temp;
	std::string relay = (relay_check) ? "Everything is awesome!" : "Something is wrong!";
	ss_relay_temp << "Relay Status: " << relay << " (" << current_time.str() << ")";
	text_overview_relay->SetText(0.06, 0.5, ss_relay_temp.str().c_str());
	text_overview_relay->SetTextColor(1);
	if (!relay_check) {
		text_overview_relay->SetTextColor(kRed);
	}
	else if (relay_warning) {
		text_overview_relay->SetTextColor(kOrange);
	}
	else{
		text_overview_relay->SetTextColor(kGreen);
	}

	std::stringstream ss_error_temp;
	std::string error = (error_check) ? "Everything is awesome!" : "Something is wrong!";
	ss_error_temp << "Error Status: " << error << " (" << current_time.str() << ")";
	text_overview_error->SetText(0.06, 0.4, ss_error_temp.str().c_str());
	text_overview_error->SetTextColor(1);
	if (!error_check) {
		text_overview_error->SetTextColor(kRed);
	}
	else if (error_warning) {
		text_overview_error->SetTextColor(kOrange);
	}
	else{
		text_overview_error->SetTextColor(kGreen);
	}

	std::stringstream ss_current_time;
	ss_current_time << "Current time: "<< " (" << current_time.str() << ")";
	text_current_time->SetText(0.06,0.3,ss_current_time.str().c_str());
	text_current_time->SetTextColor(1);

	std::string t_sc_string = lappd_SC.timeSinceEpochMilliseconds;
	unsigned long t_sc = std::stoul(t_sc_string, 0, 10);
	if (t_sc > 2048166400000 ) t_sc = 1;
	boost::posix_time::ptime sctime = *Epoch + boost::posix_time::time_duration(int(t_sc / MSEC_to_SEC / SEC_to_MIN / MIN_to_HOUR), int(t_sc / MSEC_to_SEC / SEC_to_MIN) % 60, int(t_sc / MSEC_to_SEC) % 60, t_sc % 1000);
        struct tm sctime_tm = boost::posix_time::to_tm(sctime);
        std::stringstream sc_time;
        sc_time << sctime_tm.tm_year + 1900 << "/" << sctime_tm.tm_mon + 1 << "/" << sctime_tm.tm_mday << "-" << sctime_tm.tm_hour << ":" << sctime_tm.tm_min << ":" << sctime_tm.tm_sec;

	std::stringstream ss_sc_time;
	ss_sc_time << "Slow control time: "<< " (" << sc_time.str() << ")";
	text_sc_time->SetText(0.06,0.2,ss_sc_time.str().c_str());
	text_sc_time->SetTextColor(1);


	//Actual drawing
	text_overview_title->SetTextSize(0.05);
	text_overview_temp->SetTextSize(0.05);
	text_overview_lvhv->SetTextSize(0.05);
	text_overview_trigger->SetTextSize(0.05);
	text_overview_relay->SetTextSize(0.05);
	text_overview_error->SetTextSize(0.05);
	text_current_time->SetTextSize(0.05);
	text_sc_time->SetTextSize(0.05);

	text_overview_title->SetNDC(1);
	text_overview_temp->SetNDC(1);
	text_overview_lvhv->SetNDC(1);
	text_overview_trigger->SetNDC(1);
	text_overview_relay->SetNDC(1);
	text_overview_error->SetNDC(1);
	text_current_time->SetNDC(1);
	text_sc_time->SetNDC(1);

	canvas_status_overview->cd();
	canvas_status_overview->Clear();
	text_overview_title->Draw();
	text_overview_temp->Draw();
	text_overview_lvhv->Draw();
	text_overview_trigger->Draw();
	text_overview_relay->Draw();
	text_overview_error->Draw();
	text_current_time->Draw();
	text_sc_time->Draw();

	std::stringstream ss_path_overviewinfo;
	ss_path_overviewinfo << outpath << "LAPPDSC_overviewinfo_ID"<<lappdid<<"_current." << img_extension;
	canvas_status_overview->SaveAs(ss_path_overviewinfo.str().c_str());
	canvas_status_overview->Clear();

}

void MonitorLAPPDSC::DrawTimeEvolutionLAPPDSC(ULong64_t timestamp_end, double time_frame, std::string file_ending) {

	Log("MonitorLAPPDSC: DrawTimeEvolutionLAPPDSC", v_message, verbosity);

	//--------------------------------------------------------
	//-------------DrawTimeEvolutionLAPPDSC ------------------
	// -------------------------------------------------------

	boost::posix_time::ptime endtime = *Epoch + boost::posix_time::time_duration(int(timestamp_end / MSEC_to_SEC / SEC_to_MIN / MIN_to_HOUR), int(timestamp_end / MSEC_to_SEC / SEC_to_MIN) % 60, int(timestamp_end / MSEC_to_SEC / 1000.) % 60, timestamp_end % 1000);
	struct tm endtime_tm = boost::posix_time::to_tm(endtime);
	std::stringstream end_time;
	end_time << endtime_tm.tm_year + 1900 << "/" << endtime_tm.tm_mon + 1 << "/" << endtime_tm.tm_mday << "-" << endtime_tm.tm_hour << ":" << endtime_tm.tm_min << ":" << endtime_tm.tm_sec;

	if (timestamp_end != readfromfile_tend || time_frame != readfromfile_timeframe)
		ReadFromFile(timestamp_end, time_frame);

	//looping over all files that are in the time interval, each file will be one data point

	std::stringstream ss_timeframe;
	ss_timeframe << round(time_frame * 100.) / 100.;

	std::map<int,int> i_file_id;
	for (int i_id = 0; i_id < (int) vector_lappd_id.size(); i_id++){
		int lappdid = vector_lappd_id.at(i_id);
		map_graph_temp[lappdid]->Set(0);
		map_graph_humidity[lappdid]->Set(0);
		map_graph_thermistor[lappdid]->Set(0);
		map_graph_salt[lappdid]->Set(0);
		map_graph_light[lappdid]->Set(0);
		map_graph_hv_volt[lappdid]->Set(0);
		map_graph_hv_volt_mon[lappdid]->Set(0);
		map_graph_lv_volt1[lappdid]->Set(0);
		map_graph_lv_volt2[lappdid]->Set(0);
		map_graph_lv_volt3[lappdid]->Set(0);
		i_file_id.emplace(lappdid,0);
	}

	for (unsigned int i_file = 0; i_file < temp_plot.size(); i_file++) {

		Log("MonitorLAPPDSC: Stored data (file #" + std::to_string(i_file + 1) + "): ", v_message, verbosity);
		int lappdid = lappdid_plot.at(i_file);
		int ifile = i_file_id[lappdid];
		map_graph_temp[lappdid]->SetPoint(ifile, labels_timeaxis[i_file].Convert(), temp_plot.at(i_file));
		map_graph_humidity[lappdid]->SetPoint(ifile, labels_timeaxis[i_file].Convert(), humidity_plot.at(i_file));
		map_graph_thermistor[lappdid]->SetPoint(ifile, labels_timeaxis[i_file].Convert(), thermistor_plot.at(i_file));
		map_graph_salt[lappdid]->SetPoint(ifile, labels_timeaxis[i_file].Convert(), salt_plot.at(i_file));
		map_graph_light[lappdid]->SetPoint(ifile, labels_timeaxis[i_file].Convert(), light_plot.at(i_file));
		map_graph_hv_volt[lappdid]->SetPoint(ifile, labels_timeaxis[i_file].Convert(), hv_volt_plot.at(i_file));
		map_graph_hv_volt_mon[lappdid]->SetPoint(ifile, labels_timeaxis[i_file].Convert(), hv_returnmon_plot.at(i_file));
		map_graph_lv_volt1[lappdid]->SetPoint(ifile, labels_timeaxis[i_file].Convert(), lv_v33_plot.at(i_file));
		map_graph_lv_volt2[lappdid]->SetPoint(ifile, labels_timeaxis[i_file].Convert(), lv_v25_plot.at(i_file));
		map_graph_lv_volt3[lappdid]->SetPoint(ifile, labels_timeaxis[i_file].Convert(), lv_v12_plot.at(i_file));
		ifile++;
		i_file_id[lappdid] = ifile;
	}

	// Drawing time evolution plots

	double max_canvas = 0;
	double min_canvas = 9999999.;
	double max_canvas_sigma = 0;
	double min_canvas_sigma = 99999999.;
	double max_canvas_rate = 0;
	double min_canvas_rate = 999999999.;

	std::stringstream ss_temp;
	ss_temp << "Temperature time evolution (last " << ss_timeframe.str() << "h) " << end_time.str();
	canvas_temp->cd();
	canvas_temp->Clear();
	for (int i_id=0; i_id < (int) vector_lappd_id.size(); i_id++){
		int lappdid = vector_lappd_id.at(i_id);
		map_graph_temp[lappdid]->SetTitle(ss_temp.str().c_str());
		//map_graph_temp[lappdid]->Draw("apl");
		//double max_temp = TMath::MaxElement(temp_plot.size(), graph_temp->GetY());
		//map_graph_temp[lappdid]->GetYaxis()->SetRangeUser(0.001, 1.1 * max_temp);
		multi_temp->Add(map_graph_temp[lappdid]);
		std::stringstream ss_lappdid;
		ss_lappdid << "LAPPD "<<lappdid;
		leg_temp->AddEntry(map_graph_temp[lappdid],ss_lappdid.str().c_str(),"l");
	}
	multi_temp->Draw("apl");
	multi_temp->GetYaxis()->SetTitle("Temperature [deg C]");
	multi_temp->GetXaxis()->SetTimeDisplay(1);
	multi_temp->GetXaxis()->SetLabelSize(0.03);
	multi_temp->GetXaxis()->SetLabelOffset(0.03);
	multi_temp->GetXaxis()->SetTimeFormat("#splitline{%m/%d}{%H:%M}");
	multi_temp->GetXaxis()->SetTimeOffset(0.);
	leg_temp->Draw("same");
	std::stringstream ss_temp_path;
	ss_temp_path << outpath << "LAPPDSC_TimeEvolution_Temp_" << file_ending << "." << img_extension;
	canvas_temp->SaveAs(ss_temp_path.str().c_str());
	for (int i_id=0; i_id < (int) vector_lappd_id.size(); i_id++){
                int lappdid = vector_lappd_id.at(i_id);
                multi_temp->RecursiveRemove(map_graph_temp[lappdid]);
        }

	std::stringstream ss_hum;
	ss_hum << "Humidity time evolution (last " << ss_timeframe.str() << "h) " << end_time.str();
	canvas_humidity->cd();
	canvas_humidity->Clear();
	for (int i_id=0; i_id < (int) vector_lappd_id.size(); i_id++){
                int lappdid = vector_lappd_id.at(i_id);
		map_graph_humidity[lappdid]->SetTitle(ss_hum.str().c_str());
		multi_humidity->Add(map_graph_humidity[lappdid]);
		std::stringstream ss_lappdid;
                ss_lappdid << "LAPPD "<<lappdid;
		leg_humidity->AddEntry(map_graph_humidity[lappdid],ss_lappdid.str().c_str(),"l");
	}
	multi_humidity->Draw("apl");
	multi_humidity->GetYaxis()->SetTitle("Humidity");
	multi_humidity->GetXaxis()->SetTimeDisplay(1);
	multi_humidity->GetXaxis()->SetLabelSize(0.03);
	multi_humidity->GetXaxis()->SetLabelOffset(0.03);
	multi_humidity->GetXaxis()->SetTimeFormat("#splitline{%m/%d}{%H:%M}");
	multi_humidity->GetXaxis()->SetTimeOffset(0.);
	leg_humidity->Draw("same");
	//double max_humidity = TMath::MaxElement(humidity_plot.size(), graph_humidity->GetY());
	//graph_humidity->GetYaxis()->SetRangeUser(0.001, 1.1 * max_humidity);
	std::stringstream ss_hum_path;
	ss_hum_path << outpath << "LAPPDSC_TimeEvolution_Humidity_" << file_ending << "." << img_extension;
	canvas_humidity->SaveAs(ss_hum_path.str().c_str());
	for (int i_id=0; i_id < (int) vector_lappd_id.size(); i_id++){
                int lappdid = vector_lappd_id.at(i_id);
                multi_humidity->RecursiveRemove(map_graph_humidity[lappdid]);
        }

	std::stringstream ss_thermistor;
	ss_thermistor << "Thermistor time evolution (last " << ss_timeframe.str() << "h) " << end_time.str();
	canvas_thermistor->cd();
	canvas_thermistor->Clear();
	for (int i_id=0; i_id < (int) vector_lappd_id.size(); i_id++){
                int lappdid = vector_lappd_id.at(i_id);
		map_graph_thermistor[lappdid]->SetTitle(ss_thermistor.str().c_str());
		multi_thermistor->Add(map_graph_thermistor[lappdid]);
		std::stringstream ss_lappdid;
		ss_lappdid << "LAPPD "<<lappdid;
		leg_thermistor->AddEntry(map_graph_thermistor[lappdid],ss_lappdid.str().c_str(),"l");
	}
	multi_thermistor->Draw("apl");
	multi_thermistor->GetYaxis()->SetTitle("Thermistor temperature [deg C]");
	multi_thermistor->GetXaxis()->SetTimeDisplay(1);
	multi_thermistor->GetXaxis()->SetLabelSize(0.03);
	multi_thermistor->GetXaxis()->SetLabelOffset(0.03);
	multi_thermistor->GetXaxis()->SetTimeFormat("#splitline{%m/%d}{%H:%M}");
	multi_thermistor->GetXaxis()->SetTimeOffset(0.);
	leg_thermistor->Draw("same");
	//double max_thermistor = TMath::MaxElement(thermistor_plot.size(), graph_thermistor->GetY());
	//graph_thermistor->GetYaxis()->SetRangeUser(0.001, 1.1 * max_thermistor);
	std::stringstream ss_thermistor_path;
	ss_thermistor_path << outpath << "LAPPDSC_TimeEvolution_Thermistor_" << file_ending << "." << img_extension;
	canvas_thermistor->SaveAs(ss_thermistor_path.str().c_str());
	for (int i_id=0; i_id < (int) vector_lappd_id.size(); i_id++){
                int lappdid = vector_lappd_id.at(i_id);
                multi_thermistor->RecursiveRemove(map_graph_thermistor[lappdid]);
        }

	std::stringstream ss_salt;
	ss_salt << "Salt-bridge time evolution (last " << ss_timeframe.str() << "h) " << end_time.str();
	canvas_salt->cd();
	canvas_salt->Clear();
	for (int i_id=0; i_id < (int) vector_lappd_id.size(); i_id++){
                int lappdid = vector_lappd_id.at(i_id);
		map_graph_salt[lappdid]->SetTitle(ss_salt.str().c_str());
		multi_salt->Add(map_graph_salt[lappdid]);
		std::stringstream ss_lappdid;
		ss_lappdid << "LAPPD "<<lappdid;
		leg_salt->AddEntry(map_graph_salt[lappdid],ss_lappdid.str().c_str(),"l");
	}	
	multi_salt->Draw("apl");
	multi_salt->GetYaxis()->SetTitle("Salt-bridge resistance [#Omega]");
	multi_salt->GetXaxis()->SetTimeDisplay(1);
	multi_salt->GetXaxis()->SetLabelSize(0.03);
	multi_salt->GetXaxis()->SetLabelOffset(0.03);
	multi_salt->GetXaxis()->SetTimeFormat("#splitline{%m/%d}{%H:%M}");
	multi_salt->GetXaxis()->SetTimeOffset(0.);
	leg_salt->Draw("same");
	//double max_salt = TMath::MaxElement(salt_plot.size(), graph_salt->GetY());
	//graph_salt->GetYaxis()->SetRangeUser(0.001, 1.1 * max_salt);
	std::stringstream ss_salt_path;
	ss_salt_path << outpath << "LAPPDSC_TimeEvolution_SaltBridge_" << file_ending << "." << img_extension;
	canvas_salt->SaveAs(ss_salt_path.str().c_str());
	for (int i_id=0; i_id < (int) vector_lappd_id.size(); i_id++){
                int lappdid = vector_lappd_id.at(i_id);
                multi_salt->RecursiveRemove(map_graph_salt[lappdid]);
        }

	std::stringstream ss_light;
	ss_light << "Light level time evolution (last " << ss_timeframe.str() << "h) " << end_time.str();
	canvas_light->cd();
	canvas_light->Clear();
	for (int i_id=0; i_id < (int) vector_lappd_id.size(); i_id++){
                int lappdid = vector_lappd_id.at(i_id);
		map_graph_light[lappdid]->SetTitle(ss_light.str().c_str());
		multi_light->Add(map_graph_light[lappdid]);
		std::stringstream ss_lappdid;
		ss_lappdid << "LAPPD "<<lappdid;
		leg_light->AddEntry(map_graph_light[lappdid],ss_lappdid.str().c_str(),"l");
	}
	multi_light->Draw("apl");
	leg_light->Draw("same");
	multi_light->GetYaxis()->SetTitle("Light level");
	multi_light->GetXaxis()->SetTimeDisplay(1);
	multi_light->GetXaxis()->SetLabelSize(0.03);
	multi_light->GetXaxis()->SetLabelOffset(0.03);
	multi_light->GetXaxis()->SetTimeFormat("#splitline{%m/%d}{%H:%M}");
	//double max_light = TMath::MaxElement(light_plot.size(), graph_light->GetY());
	//graph_light->GetYaxis()->SetRangeUser(0.001, 1.1 * max_light);
	std::stringstream ss_light_path;
	ss_light_path << outpath << "LAPPDSC_TimeEvolution_Light_" << file_ending << "." << img_extension;
	canvas_light->SaveAs(ss_light_path.str().c_str());
	for (int i_id=0; i_id < (int) vector_lappd_id.size(); i_id++){
		int lappdid = vector_lappd_id.at(i_id);
		multi_light->RecursiveRemove(map_graph_light[lappdid]);
	}

	std::stringstream ss_hv;
	ss_hv << "HV Set time evolution (last " << ss_timeframe.str() << "h) " << end_time.str();
	canvas_hv->cd();
	canvas_hv->Clear();
	for (int i_id=0; i_id < (int) vector_lappd_id.size(); i_id++){
                int lappdid = vector_lappd_id.at(i_id);
                map_graph_hv_volt[lappdid]->SetTitle(ss_hv.str().c_str());
                multi_hv_volt->Add(map_graph_hv_volt[lappdid]);
                std::stringstream ss_lappdid;
                ss_lappdid << "LAPPD "<<lappdid;
                leg_hv_volt->AddEntry(map_graph_hv_volt[lappdid],ss_lappdid.str().c_str(),"l");
        }
	multi_hv_volt->Draw("apl");
	multi_hv_volt->SetTitle(ss_hv.str().c_str());
	multi_hv_volt->GetYaxis()->SetTitle("HV Set [V]");
	multi_hv_volt->GetXaxis()->SetTimeDisplay(1);
	multi_hv_volt->GetXaxis()->SetLabelSize(0.03);
	multi_hv_volt->GetXaxis()->SetLabelOffset(0.03);
	multi_hv_volt->GetXaxis()->SetTimeFormat("#splitline{%m/%d}{%H:%M}");
	multi_hv_volt->GetXaxis()->SetTimeOffset(0.);
	leg_hv_volt->Draw("same");
	//double max_hv_volt = TMath::MaxElement(hv_volt_plot.size(), graph_hv_volt->GetY());
	//graph_hv_volt->GetYaxis()->SetRangeUser(0.001, 1.1 * max_hv_volt);
	std::stringstream ss_hv_path;
	ss_hv_path << outpath << "LAPPDSC_TimeEvolution_HV_" << file_ending << "." << img_extension;
	canvas_hv->SaveAs(ss_hv_path.str().c_str());
	for (int i_id=0; i_id < (int) vector_lappd_id.size(); i_id++){
                int lappdid = vector_lappd_id.at(i_id);
                multi_hv_volt->RecursiveRemove(map_graph_hv_volt[lappdid]);
        }

	std::stringstream ss_hv_mon;
	ss_hv_mon << "HV Mon time evolution (last " << ss_timeframe.str() << "h) " << end_time.str();
	canvas_hv->Clear();
	for (int i_id=0; i_id < (int) vector_lappd_id.size(); i_id++){
                int lappdid = vector_lappd_id.at(i_id);
                map_graph_hv_volt_mon[lappdid]->SetTitle(ss_hv_mon.str().c_str());
                multi_hv_volt_mon->Add(map_graph_hv_volt_mon[lappdid]);
                std::stringstream ss_lappdid;
                ss_lappdid << "LAPPD "<<lappdid;
                leg_hv_volt_mon->AddEntry(map_graph_hv_volt_mon[lappdid],ss_lappdid.str().c_str(),"l");
        }
	multi_hv_volt_mon->Draw("apl");
	multi_hv_volt_mon->SetTitle(ss_hv_mon.str().c_str());
	multi_hv_volt_mon->GetYaxis()->SetTitle("HV Mon [V]");
	multi_hv_volt_mon->GetXaxis()->SetTimeDisplay(1);
	multi_hv_volt_mon->GetXaxis()->SetLabelSize(0.03);
	multi_hv_volt_mon->GetXaxis()->SetLabelOffset(0.03);
	multi_hv_volt_mon->GetXaxis()->SetTimeFormat("#splitline{%m/%d}{%H:%M}");
	multi_hv_volt_mon->GetXaxis()->SetTimeOffset(0.);
	leg_hv_volt_mon->Draw("same");
	//double max_hv_volt_mon = TMath::MaxElement(hv_returnmon_plot.size(), graph_hv_volt_mon->GetY());
	//graph_hv_volt_mon->GetYaxis()->SetRangeUser(0.001, 1.1 * max_hv_volt_mon);
	std::stringstream ss_hv_path_mon;
	ss_hv_path_mon << outpath << "LAPPDSC_TimeEvolution_HVMon_" << file_ending << "." << img_extension;
	canvas_hv->SaveAs(ss_hv_path_mon.str().c_str());
	for (int i_id=0; i_id < (int) vector_lappd_id.size(); i_id++){
                int lappdid = vector_lappd_id.at(i_id);
                multi_hv_volt_mon->RecursiveRemove(map_graph_hv_volt_mon[lappdid]);
        }

	std::stringstream ss_lv;
	ss_lv << "LV1 time evolution (last " << ss_timeframe.str() << "h) " << end_time.str();
	canvas_lv->cd();
        canvas_lv->Clear();
        for (int i_id=0; i_id < (int) vector_lappd_id.size(); i_id++){
                int lappdid = vector_lappd_id.at(i_id);
                map_graph_lv_volt1[lappdid]->SetTitle(ss_lv.str().c_str());
                multi_lv_volt1->Add(map_graph_lv_volt1[lappdid]);
                std::stringstream ss_lappdid;
                ss_lappdid << "LAPPD "<<lappdid;
                leg_lv_volt1->AddEntry(map_graph_lv_volt1[lappdid],ss_lappdid.str().c_str(),"l");
        }
        multi_lv_volt1->Draw("apl");
	multi_lv_volt1->GetYaxis()->SetTitle("LV Mon [V]");
        multi_lv_volt1->GetXaxis()->SetTimeDisplay(1);
        multi_lv_volt1->GetXaxis()->SetLabelSize(0.03);
        multi_lv_volt1->GetXaxis()->SetLabelOffset(0.03);
        multi_lv_volt1->GetXaxis()->SetTimeFormat("#splitline{%m/%d}{%H:%M}");
        multi_lv_volt1->GetXaxis()->SetTimeOffset(0.);
        leg_lv_volt1->Draw("same");
	std::stringstream ss_lv_path;
	ss_lv_path << outpath << "LAPPDSC_TimeEvolution_LV1_" << file_ending << "." << img_extension;
	canvas_lv->SaveAs(ss_lv_path.str().c_str());
	for (int i_id=0; i_id < (int) vector_lappd_id.size(); i_id++){
                int lappdid = vector_lappd_id.at(i_id);
                multi_lv_volt1->RecursiveRemove(map_graph_lv_volt1[lappdid]);
        }

	ss_lv.str("");
	ss_lv << "LV2 time evolution (last " << ss_timeframe.str() << "h) " << end_time.str();
        canvas_lv->cd();
        canvas_lv->Clear();
        for (int i_id=0; i_id < (int) vector_lappd_id.size(); i_id++){
                int lappdid = vector_lappd_id.at(i_id);
                map_graph_lv_volt2[lappdid]->SetTitle(ss_lv.str().c_str());
                multi_lv_volt2->Add(map_graph_lv_volt2[lappdid]);
                std::stringstream ss_lappdid;
                ss_lappdid << "LAPPD "<<lappdid;
                leg_lv_volt2->AddEntry(map_graph_lv_volt2[lappdid],ss_lappdid.str().c_str(),"l");
        }
        multi_lv_volt2->Draw("apl");
        multi_lv_volt2->GetYaxis()->SetTitle("LV Mon [V]");
        multi_lv_volt2->GetXaxis()->SetTimeDisplay(1);
        multi_lv_volt2->GetXaxis()->SetLabelSize(0.03);
        multi_lv_volt2->GetXaxis()->SetLabelOffset(0.03);
        multi_lv_volt2->GetXaxis()->SetTimeFormat("#splitline{%m/%d}{%H:%M}");
        multi_lv_volt2->GetXaxis()->SetTimeOffset(0.);
        leg_lv_volt2->Draw("same");
        ss_lv_path.str("");
        ss_lv_path << outpath << "LAPPDSC_TimeEvolution_LV2_" << file_ending << "." << img_extension;
        canvas_lv->SaveAs(ss_lv_path.str().c_str());
        for (int i_id=0; i_id < (int) vector_lappd_id.size(); i_id++){
                int lappdid = vector_lappd_id.at(i_id);
                multi_lv_volt2->RecursiveRemove(map_graph_lv_volt2[lappdid]);
        }

	ss_lv.str("");
	ss_lv << "LV3 time evolution (last " << ss_timeframe.str() << "h) " << end_time.str();
        canvas_lv->cd();
        canvas_lv->Clear();
        for (int i_id=0; i_id < (int) vector_lappd_id.size(); i_id++){
                int lappdid = vector_lappd_id.at(i_id);
                map_graph_lv_volt3[lappdid]->SetTitle(ss_lv.str().c_str());
                multi_lv_volt3->Add(map_graph_lv_volt3[lappdid]);
                std::stringstream ss_lappdid;
                ss_lappdid << "LAPPD "<<lappdid;
                leg_lv_volt3->AddEntry(map_graph_lv_volt3[lappdid],ss_lappdid.str().c_str(),"l");
        }
        multi_lv_volt3->Draw("apl");
        multi_lv_volt3->GetYaxis()->SetTitle("LV Mon [V]");
        multi_lv_volt3->GetXaxis()->SetTimeDisplay(1);
        multi_lv_volt3->GetXaxis()->SetLabelSize(0.03);
        multi_lv_volt3->GetXaxis()->SetLabelOffset(0.03);
        multi_lv_volt3->GetXaxis()->SetTimeFormat("#splitline{%m/%d}{%H:%M}");
        multi_lv_volt3->GetXaxis()->SetTimeOffset(0.);
        leg_lv_volt3->Draw("same");
        ss_lv_path.str("");
        ss_lv_path << outpath << "LAPPDSC_TimeEvolution_LV3_" << file_ending << "." << img_extension;
        canvas_lv->SaveAs(ss_lv_path.str().c_str());
        for (int i_id=0; i_id < (int) vector_lappd_id.size(); i_id++){
                int lappdid = vector_lappd_id.at(i_id);
                multi_lv_volt3->RecursiveRemove(map_graph_lv_volt3[lappdid]);
        }

/*
	multi_lv->Add(graph_lv_volt1);
	leg_lv->AddEntry(graph_lv_volt1, "V33", "l");
	multi_lv->Add(graph_lv_volt2);
	leg_lv->AddEntry(graph_lv_volt2, "V31", "l");
	multi_lv->Add(graph_lv_volt3);
	leg_lv->AddEntry(graph_lv_volt3, "V18", "l");
	multi_lv->Draw("apl");
	multi_lv->SetTitle(ss_lv.str().c_str());
	multi_lv->GetYaxis()->SetTitle("LV [V]");
	multi_lv->GetXaxis()->SetTimeDisplay(1);
	multi_lv->GetXaxis()->SetLabelSize(0.03);
	multi_lv->GetXaxis()->SetLabelOffset(0.03);
	multi_lv->GetXaxis()->SetTimeFormat("#splitline{%m/%d}{%H:%M}");
	multi_lv->GetXaxis()->SetTimeOffset(0.);
	leg_lv->Draw();

	multi_lv->RecursiveRemove(graph_lv_volt1);
	multi_lv->RecursiveRemove(graph_lv_volt2);
	multi_lv->RecursiveRemove(graph_lv_volt3);
*/
	leg_lv->Clear();
	canvas_lv->Clear();
	leg_temp->Clear();
	leg_humidity->Clear();
	leg_salt->Clear();
	leg_light->Clear();
	leg_thermistor->Clear();
	leg_hv_volt->Clear();
	leg_hv_volt_mon->Clear();
	leg_lv_volt1->Clear();
	leg_lv_volt2->Clear();
	leg_lv_volt3->Clear();
}
