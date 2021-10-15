#include "MonitorLAPPDData.h"

MonitorLAPPDData::MonitorLAPPDData() :
		Tool() {
}

bool MonitorLAPPDData::Initialise(std::string configfile, DataModel &data) {

	/////////////////// Useful header ///////////////////////
	if (configfile != "")
		m_variables.Initialise(configfile); // loading config file
	//m_variables.Print();

	m_data = &data; //assigning transient data pointer
	/////////////////////////////////////////////////////////////////

	//gObjectTable only for debugging memory leaks, otherwise comment out
	//std::cout <<"MonitorMRDTime: List of Objects (beginning of Initialise): "<<std::endl;
	//gObjectTable->Print();

	//-------------------------------------------------------
	//-----------------Get Configuration---------------------
	//-------------------------------------------------------

	update_frequency = 0.;

	std::string acdc_configuration;

	m_variables.Get("OutputPath", outpath_temp);
	m_variables.Get("StartTime", StartTime);
	m_variables.Get("UpdateFrequency", update_frequency);
	m_variables.Get("PathMonitoring", path_monitoring);
	m_variables.Get("PlotConfiguration", plot_configuration);
	m_variables.Get("ACDCBoardConfiguration", acdc_configuration);
	m_variables.Get("ImageFormat", img_extension);
	m_variables.Get("ForceUpdate", force_update);
	m_variables.Get("DrawMarker", draw_marker);
	m_variables.Get("verbose", verbosity);

	if (verbosity > 1)
		std::cout << "Tool MonitorLAPPDData: Initialising...." << std::endl;
	// Update frequency specifies the frequency at which the File Log Histogram is updated
	// All other monitor plots are updated as soon as a new file is available for readout
	if (update_frequency < 0.1) {
		if (verbosity > 0)
			std::cout << "MonitorLAPPDData: Update Frequency of " << update_frequency << " mins is too low. Setting default value of 5 min." << std::endl;
		update_frequency = 5.;
	}

	//default should be no forced update of the monitoring plots every execute step
	if (force_update != 0 && force_update != 1) {
		force_update = 0;
	}

	//check if the image format is jpg or png
	if (!(img_extension == "png" || img_extension == "jpg" || img_extension == "jpeg")) {
		img_extension = "jpg";
	}

	//Print out path to monitoring files
	if (verbosity > 2)
		std::cout << "MonitorLAPPDData: PathMonitoring: " << path_monitoring << std::endl;

	//Set up Epoch
	Epoch = new boost::posix_time::ptime(boost::gregorian::from_string(StartTime));

	//Evaluating output path for monitoring plots
	if (outpath_temp == "fromStore")
		m_data->CStore.Get("OutPath", outpath);
	else
		outpath = outpath_temp;
	if (verbosity > 2)
		std::cout << "MonitorLAPPDData: Output path for plots is " << outpath << std::endl;

	//-------------------------------------------------------
	//----------Read in configuration option for plots-------
	//-------------------------------------------------------

	ReadInConfiguration();

	//-------------------------------------------------------
	//----------Read in ACDC board numbers-------------------
	//-------------------------------------------------------

	LoadACDCBoardConfig(acdc_configuration);

	//-------------------------------------------------------
	//----------Initialize histograms/canvases---------------
	//-------------------------------------------------------

	this->InitializeHistsLAPPD();

	//-------------------------------------------------------
	//------Setup time variables for periodic updates--------
	//-------------------------------------------------------

	period_update = boost::posix_time::time_duration(0, int(update_frequency), 0, 0);
	last = boost::posix_time::ptime(boost::posix_time::second_clock::local_time());

	// Omit warning messages from ROOT: 1001 - info messages, 2001 - warnings, 3001 - errors
	gROOT->ProcessLine("gErrorIgnoreLevel = 3001;");

	return true;
}

bool MonitorLAPPDData::Execute() {

	if (verbosity > 10)
		std::cout << "MonitorLAPPDData: Executing ...." << std::endl;

	current = (boost::posix_time::second_clock::local_time());
	duration = boost::posix_time::time_duration(current - last);
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

	bool has_lappd;
	m_data->CStore.Get("HasLAPPDData", has_lappd);

	std::string State;
	m_data->CStore.Get("State", State);

	if (has_lappd) {
		if (State == "Wait" || State == "LAPPDSC") {
			if (verbosity > 2)
				std::cout << "MonitorLAPPDData: State is " << State << std::endl;
		} else if (State == "DataFile") {
			if (verbosity > 1)
				std::cout << "MonitorLAPPDData: New PSEC data available." << std::endl;

			//TODO
			//TODO: Need to confirm data format of LAPPDData, is it PsecData, or a vec of PsecData, or map<timestamp,PsecData>?
			//TODO

			m_data->Stores["LAPPDData"]->Get("LAPPDData", LAPPDData);

			//Process data
			//TODO: Adapt when data type is understood
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

		} else {
			if (verbosity > 1)
				std::cout << "MonitorLAPPDData: State not recognized: " << State << std::endl;
		}
	}

	// if force_update is specified, the plots will be updated no matter whether there has been a new file or not
	if (force_update)
		UpdateMonitorPlots(config_timeframes, config_endtime_long, config_label, config_plottypes);

	//-------------------------------------------------------
	//-----------Has enough time passed for update?----------
	//-------------------------------------------------------

	if (duration >= period_update) {

		Log("MonitorLAPPDData: " + std::to_string(update_frequency) + " mins passed... Updating file history plot.", v_message, verbosity);

		last = current;
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

bool MonitorLAPPDData::Finalise() {

	if (verbosity > 1)
		std::cout << "Tool MonitorLAPPDData: Finalising ...." << std::endl;

	// gDirectory->ls();
	// gDirectory->pwd();

	//timing pointers
	delete Epoch;

	std::cout << "canvas" << std::endl;
	//canvas

	delete canvas_status_data;
	delete canvas_pps_rate;
	delete canvas_frame_rate;
	delete canvas_buffer_size;
	delete canvas_int_charge;
	delete canvas_align_1file;
	delete canvas_align_5files;
	delete canvas_align_10files;
	delete canvas_align_20files;
	delete canvas_align_100files;
	delete canvas_adc_channel;
	delete canvas_buffer_channel;
	delete canvas_buffer;
	delete canvas_waveform_voltages;
	delete canvas_waveform_onedim;
	delete canvas_pedestal;
	delete canvas_pedestal_all;
	delete canvas_pedestal_difference;
	delete canvas_buffer_size_all;
	delete canvas_rate_threshold_all;
	delete canvas_events_per_channel;

	std::cout << "histograms" << std::endl;
	//histograms
	//
	/* Somehow deleting histograms creates a segfault, omit for now
	 for (int i_board = 0; i_board < (int) board_configuration.size(); i_board++){

	 int board_nr = board_configuration.at(i_board);
	 delete hist_align_1file.at(board_nr);

	 delete hist_align_5files.at(board_nr);
	 delete hist_align_10files.at(board_nr);
	 delete hist_align_20files.at(board_nr);
	 delete hist_align_100files.at(board_nr);
	 delete hist_adc_channel.at(board_nr);
	 delete hist_buffer_channel.at(board_nr);
	 delete hist_buffer.at(board_nr);

	 }
	 */

	std::cout << "tgraphs" << std::endl;
	//graphs
	for (int i_board = 0; i_board < (int) board_configuration.size(); i_board++) {

		int board_nr = board_configuration.at(i_board);
		delete graph_pps_rate.at(board_nr);
		delete graph_frame_rate.at(board_nr);
		delete graph_buffer_size.at(board_nr);
		delete graph_int_charge.at(board_nr);
	}

	//multi-graphs

	//legends

	std::cout << "text" << std::endl;
	//text
	delete text_data_title;
	delete text_pps_rate;
	delete text_frame_rate;
	delete text_buffer_size;
	delete text_int_charge;

	return true;
}

void MonitorLAPPDData::InitializeHistsLAPPD() {

	if (verbosity > 2)
		std::cout << "MonitorLAPPDData: InitializeHists" << std::endl;

	gROOT->cd();
	//Canvas
	canvas_status_data = new TCanvas("canvas_status_data", "Status of PSEC data", 900, 600);
	canvas_pps_rate = new TCanvas("canvas_pps_rate", "PPS Rate Time evolution", 900, 600);
	canvas_frame_rate = new TCanvas("canvas_frame_rate", "Frame Rate Time evolution", 900, 600);
	canvas_buffer_size = new TCanvas("canvas_buffer_size", "Buffer Size Time evolution", 900, 600);
	canvas_int_charge = new TCanvas("canvas_int_charge", "Integrated charge Time evolution", 900, 600);
	canvas_align_1file = new TCanvas("canvas_align_1file", "Time alignment 1 file", 900, 600);
	canvas_align_5files = new TCanvas("canvas_align_5files", "Time alignment 5 file", 900, 600);
	canvas_align_10files = new TCanvas("canvas_align_10files", "Time alignment 10 files", 900, 600);
	canvas_align_20files = new TCanvas("canvas_align_20files", "Time alignment 20 file", 900, 600);
	canvas_align_100files = new TCanvas("canvas_align_100files", "Time alignment 100 files", 900, 600);
	canvas_adc_channel = new TCanvas("canvas_adc_channel", "ADC values vs. channels", 900, 600);
	canvas_waveform = new TCanvas("canvas_waveform", "Waveform canvas", 900, 600);
	canvas_buffer_channel = new TCanvas("canvas_buffer_channel", "LAPPD Buffer size vs. channels", 900, 600);
	canvas_buffer = new TCanvas("canvas_buffer", "LAPPD buffer size", 900, 600);
	canvas_waveform_voltages = new TCanvas("canvas_waveform_voltages", "LAPPD Waveform Voltages", 900, 600);
	canvas_waveform_onedim = new TCanvas("canvas_waveform_onedim", "LAPPD Waveform One Dim", 900, 600);
	canvas_pedestal = new TCanvas("canvas_pedestal", "LAPPD individual pedestals", 900, 600);
	canvas_pedestal_all = new TCanvas("canvas_pedestal_all", "LAPPD Pedestals", 900, 600);
	canvas_pedestal_difference = new TCanvas("canvas_pedestal_difference", "LAPPD Pedestals Differences", 900, 600);
	canvas_buffer_size_all = new TCanvas("canvas_buffer_size_all", "LAPPD Buffer Sizes", 900, 600);
	canvas_rate_threshold_all = new TCanvas("canvas_rate_threshold_all", "LAPPD Hits", 900, 600);
	canvas_events_per_channel = new TCanvas("canvas_events_per_channel", "LAPPD Events per Channel", 900, 600);

	//Histograms
	//ToDo: Not hardcode the number of channels here
	int numberOfChannels = 30;
	int numberOfCards = (int) board_configuration.size();

	hist_pedestal_all = new TH2F("Fitted pedestal values", "Fitted pedestal values", numberOfChannels, 0, numberOfChannels, numberOfCards, 0, numberOfCards);
	hist_pedestal_difference_all = new TH2F("Fitted pedestal difference values", "Fitted pedestal difference values", numberOfChannels, 0, numberOfChannels, numberOfCards, 0, numberOfCards);
	hist_buffer_size_all = new TH2F("Buffer Size", "Buffer Size", numberOfChannels, 0, numberOfChannels, numberOfCards, 0, numberOfCards);
	hist_rate_threshold_all = new TH2F("Rate above threshold", "Rate above threshold", numberOfChannels, 0, numberOfChannels, numberOfCards, 0, numberOfCards);
	hist_events_per_channel = new TH2F("Events per Channel", "Events per Channel", numberOfChannels, 0, numberOfChannels, numberOfCards, 0, numberOfCards);

	hist_pedestal_all->SetStats(0);
	hist_pedestal_difference_all->SetStats(0);
	hist_buffer_size_all->SetStats(0);
	hist_rate_threshold_all->SetStats(0);
	hist_events_per_channel->SetStats(0);

	hist_pedestal_all->GetXaxis()->SetNdivisions(numberOfChannels);
	hist_pedestal_difference_all->GetXaxis()->SetNdivisions(numberOfChannels);
	hist_buffer_size_all->GetXaxis()->SetNdivisions(numberOfChannels);
	hist_rate_threshold_all->GetXaxis()->SetNdivisions(numberOfChannels);
	hist_events_per_channel->GetXaxis()->SetNdivisions(numberOfChannels);

	hist_pedestal_all->GetYaxis()->SetNdivisions(numberOfCards);
	hist_pedestal_difference_all->GetYaxis()->SetNdivisions(numberOfCards);
	hist_buffer_size_all->GetYaxis()->SetNdivisions(numberOfCards);
	hist_rate_threshold_all->GetYaxis()->SetNdivisions(numberOfCards);
	hist_events_per_channel->GetYaxis()->SetNdivisions(numberOfCards);

	for (int i_label_x = 0; i_label_x < numberOfChannels; i_label_x++) {
		std::stringstream ss_channel;
		ss_channel << i_label_x;
		std::string str_ch = "ch " + ss_channel.str();
		hist_pedestal_all->GetXaxis()->SetBinLabel(i_label_x + 1, str_ch.c_str());
		hist_pedestal_difference_all->GetXaxis()->SetBinLabel(i_label_x + 1, str_ch.c_str());
		hist_buffer_size_all->GetXaxis()->SetBinLabel(i_label_x + 1, str_ch.c_str());
		hist_rate_threshold_all->GetXaxis()->SetBinLabel(i_label_x + 1, str_ch.c_str());
		hist_events_per_channel->GetXaxis()->SetBinLabel(i_label_x + 1, str_ch.c_str());
	}
	for (int i_label_y = 0; i_label_y < numberOfCards; i_label_y++) {
		std::stringstream ss_card;
		int board_nr = board_configuration.at(i_label_y);
		ss_card << board_nr;
		std::string str_card = "board " + ss_card.str();
		hist_pedestal_all->GetYaxis()->SetBinLabel(i_label_y + 1, str_card.c_str());
		hist_pedestal_difference_all->GetYaxis()->SetBinLabel(i_label_y + 1, str_card.c_str());
		hist_buffer_size_all->GetYaxis()->SetBinLabel(i_label_y + 1, str_card.c_str());
		hist_rate_threshold_all->GetYaxis()->SetBinLabel(i_label_y + 1, str_card.c_str());
		hist_events_per_channel->GetYaxis()->SetBinLabel(i_label_y + 1, str_card.c_str());
	}
	hist_pedestal_all->GetZaxis()->SetTitle("Mean Pedestal [ADC count]");
	hist_pedestal_difference_all->GetZaxis()->SetTitle("Mean Pedestal [ADC count]");
	hist_buffer_size_all->GetZaxis()->SetTitle("Buffer Size");
	hist_rate_threshold_all->GetZaxis()->SetTitle("Number of Samples > Mean Pedestal");
	hist_events_per_channel->GetZaxis()->SetTitle("Number of Events");
	hist_pedestal_all->GetZaxis()->SetTitleOffset(1.3);
	hist_pedestal_difference_all->GetZaxis()->SetTitleOffset(1.3);
	hist_buffer_size_all->GetZaxis()->SetTitleOffset(1.3);
	hist_rate_threshold_all->GetZaxis()->SetTitleOffset(1.3);
	hist_events_per_channel->GetZaxis()->SetTitleOffset(1.3);
	hist_pedestal_all->LabelsOption("v");
	hist_pedestal_difference_all->LabelsOption("v");
	hist_buffer_size_all->LabelsOption("v");
	hist_rate_threshold_all->LabelsOption("v");
	hist_events_per_channel->LabelsOption("v");

//TODO: Change this number
//In principle, here we need the number of events in the file
	for (int i_event = 0; i_event < 100; i_event++) {
		std::map<int, std::vector<TH1F*> > tempMap;
		for (int i_board = 0; i_board < (int) board_configuration.size(); i_board++) {
		int board_nr = board_configuration.at(i_board);
		std::vector<TH1F*> hist_waveform_temp_vec;
		for (int i = 0; i < numberOfChannels; i++) {
			std::stringstream ss_waveform_text;
			ss_waveform_text << "hist_waveforms" << board_nr << "channel" << i << "event" << i_event;
			TH1F *hist_waveforms_onedim_single = new TH1F(ss_waveform_text.str().c_str(), ss_waveform_text.str().c_str(), 256, 0, 256);
			hist_waveforms_onedim_single->GetXaxis()->SetTitle("Buffer position");
			hist_waveforms_onedim_single->GetYaxis()->SetTitle("ADC value");
			hist_waveforms_onedim_single->SetStats(0);
			hist_waveform_temp_vec.push_back(hist_waveforms_onedim_single);
		}
		tempMap.emplace(board_nr, hist_waveform_temp_vec);
		hist_waveform_temp_vec.clear();
		}
		hist_waveforms_onedim.push_back(tempMap);
	}





	//Create separate histograms for all board numbers
	for (int i_board = 0; i_board < (int) board_configuration.size(); i_board++) {

		int board_nr = board_configuration.at(i_board);
		std::stringstream ss_align_1file, ss_align_5files, ss_align_10files, ss_align_20files, ss_align_100files;
		std::stringstream ss_adc_channel, ss_waveform_channel, ss_buffer_channel, ss_buffer, ss_waveform_voltages;
		ss_align_1file << "hist_align_1file_board" << board_nr;
		ss_align_5files << "hist_align_5files_board" << board_nr;
		ss_align_10files << "hist_align_10files_board" << board_nr;
		ss_align_20files << "hist_align_20files_board" << board_nr;
		ss_align_100files << "hist_align_100files_board" << board_nr;
		ss_adc_channel << "hist_adc_channel_board" << board_nr;
		ss_waveform_channel << "hist_waveform_channel_board" << board_nr;
		ss_buffer_channel << "hist_buffer_channel_board" << board_nr;
		ss_buffer << "hist_buffer_board" << board_nr;
		ss_waveform_voltages << "hist_waveform_voltages" << board_nr;

		TH1F *hist_align_1file_single = new TH1F(ss_align_1file.str().c_str(), ss_align_1file.str().c_str(), 100, -4000, 4000);
		TH1F *hist_align_5files_single = new TH1F(ss_align_5files.str().c_str(), ss_align_5files.str().c_str(), 100, -4000, 4000);
		TH1F *hist_align_10files_single = new TH1F(ss_align_10files.str().c_str(), ss_align_10files.str().c_str(), 100, -4000, 4000);
		TH1F *hist_align_20files_single = new TH1F(ss_align_20files.str().c_str(), ss_align_20files.str().c_str(), 100, -4000, 4000);
		TH1F *hist_align_100files_single = new TH1F(ss_align_100files.str().c_str(), ss_align_100files.str().c_str(), 100, -4000, 4000);
		TH2F *hist_adc_channel_single = new TH2F(ss_adc_channel.str().c_str(), ss_adc_channel.str().c_str(), 200, 0, 4096, 30, 0, 30);
		TH2F *hist_waveform_channel_single = new TH2F(ss_waveform_channel.str().c_str(), ss_waveform_channel.str().c_str(), 256, 0, 256, 30, 0, 30);
		TH2F *hist_buffer_channel_single = new TH2F(ss_buffer_channel.str().c_str(), ss_buffer_channel.str().c_str(), 50, 0, 2000, 30, 0, 30);
		TH1F *hist_buffer_single = new TH1F(ss_buffer.str().c_str(), ss_buffer.str().c_str(), 50, 0, 2000);
		TH2F *hist_waveform_voltages_single = new TH2F(ss_waveform_voltages.str().c_str(), ss_waveform_voltages.str().c_str(), 256, 0, 256, 30, 0, 30);

		//TODO: Title, XAxis, YAxis for timing histos
		hist_align_1file_single->GetXaxis()->SetTitle("Time [ns]");
		hist_align_1file_single->GetYaxis()->SetTitle("Entries");
		hist_align_1file_single->SetStats(0);

		hist_align_5files_single->GetXaxis()->SetTitle("Time [ns]");
		hist_align_5files_single->GetYaxis()->SetTitle("Entries");
		hist_align_5files_single->SetStats(0);

		hist_align_10files_single->GetXaxis()->SetTitle("Time [ns]");
		hist_align_10files_single->GetYaxis()->SetTitle("Entries");
		hist_align_10files_single->SetStats(0);

		hist_align_20files_single->GetXaxis()->SetTitle("Time [ns]");
		hist_align_20files_single->GetYaxis()->SetTitle("Entries");
		hist_align_20files_single->SetStats(0);

		hist_align_100files_single->GetXaxis()->SetTitle("Time [ns]");
		hist_align_100files_single->GetYaxis()->SetTitle("Entries");
		hist_align_100files_single->SetStats(0);

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

		hist_waveform_voltages_single->GetXaxis()->SetTitle("Buffer position");
		hist_waveform_voltages_single->GetYaxis()->SetTitle("Channelkey");
		hist_waveform_voltages_single->GetZaxis()->SetTitle("ADC value");
		hist_waveform_voltages_single->SetStats(0);

		hist_align_1file.emplace(board_nr, hist_align_1file_single);
		hist_align_5files.emplace(board_nr, hist_align_5files_single);
		hist_align_10files.emplace(board_nr, hist_align_10files_single);
		hist_align_20files.emplace(board_nr, hist_align_20files_single);
		hist_align_100files.emplace(board_nr, hist_align_100files_single);
		hist_adc_channel.emplace(board_nr, hist_adc_channel_single);
		hist_waveform_channel.emplace(board_nr, hist_waveform_channel_single);
		hist_buffer_channel.emplace(board_nr, hist_buffer_channel_single);
		hist_buffer.emplace(board_nr, hist_buffer_single);
		hist_waveform_voltages.emplace(board_nr, hist_waveform_voltages_single);

		std::vector<TH1F*> hist_pedestal_temp_vec;

		for (int i = 0; i < numberOfChannels; i++) {
			std::stringstream ss_pedestal_text;
			ss_pedestal_text << "hist_pedestal" << board_nr << "channel" << i;
			//ToDo: change this values to more reasonable numbers
			TH1F *hist_pedestal_single = new TH1F(ss_pedestal_text.str().c_str(), ss_pedestal_text.str().c_str(), 1000, 0, 4000);
			hist_pedestal_single->GetXaxis()->SetTitle("ADC value");
			hist_pedestal_single->GetYaxis()->SetTitle("Entries");
			hist_pedestal_single->SetStats(0);
			hist_pedestal_temp_vec.push_back(hist_pedestal_single);
		}
		hist_pedestal.emplace(board_nr, hist_pedestal_temp_vec);
		hist_pedestal_temp_vec.clear();
	}
	//Graphs
	//Make one of each graph type for each board
	for (int i_board = 0; i_board < (int) board_configuration.size(); i_board++) {

		int board_nr = board_configuration.at(i_board);

		TGraph *graph_pps_rate_single = new TGraph();
		TGraph *graph_frame_rate_single = new TGraph();
		TGraph *graph_buffer_size_single = new TGraph();
		TGraph *graph_int_charge_single = new TGraph();

		std::stringstream ss_pps_rate, ss_frame_rate, ss_buffer_size, ss_int_charge;
		ss_pps_rate << "graph_pps_rate_board" << board_nr;
		ss_frame_rate << "graph_frame_rate_board" << board_nr;
		ss_buffer_size << "graph_buffer_size_board" << board_nr;
		ss_int_charge << "graph_int_charge_board" << board_nr;

		graph_pps_rate_single->SetName(ss_pps_rate.str().c_str());
		graph_frame_rate_single->SetName(ss_frame_rate.str().c_str());
		graph_buffer_size_single->SetName(ss_buffer_size.str().c_str());
		graph_int_charge_single->SetName(ss_int_charge.str().c_str());

		std::stringstream title_pps_rate, title_frame_rate, title_buffer_size, title_int_charge;
		title_pps_rate << "LAPPD PPS Rate time evolution - Board " << board_nr;
		title_frame_rate << "LAPPD Frame Rate time evolution - Board " << board_nr;
		title_buffer_size << "LAPPD Buffer Size time evolution - Board " << board_nr;
		title_int_charge << "LAPPD Integrated Charge time evolution - Board " << board_nr;

		graph_pps_rate_single->SetTitle(title_pps_rate.str().c_str());
		graph_frame_rate_single->SetTitle(title_frame_rate.str().c_str());
		graph_buffer_size_single->SetTitle(title_buffer_size.str().c_str());
		graph_int_charge_single->SetTitle(title_int_charge.str().c_str());

		if (draw_marker) {
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

		graph_pps_rate.emplace(board_nr, graph_pps_rate_single);
		graph_frame_rate.emplace(board_nr, graph_frame_rate_single);
		graph_buffer_size.emplace(board_nr, graph_buffer_size_single);
		graph_int_charge.emplace(board_nr, graph_int_charge_single);

	}
	//Multi-Graphs

	//Legends

	//Text
	text_data_title = new TText();
	text_pps_rate = new TText();
	text_buffer_size = new TText();
	text_frame_rate = new TText();
	text_int_charge = new TText();

	text_data_title->SetNDC(1);
	text_pps_rate->SetNDC(1);
	text_buffer_size->SetNDC(1);
	text_frame_rate->SetNDC(1);
	text_int_charge->SetNDC(1);

}

void MonitorLAPPDData::ReadInConfiguration() {

	//-------------------------------------------------------
	//----------------ReadInConfiguration -------------------
	//-------------------------------------------------------

	Log("MonitorLAPPDData::ReadInConfiguration", v_message, verbosity);

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
					std::cout << "ERROR (MonitorLAPPDData): ReadInConfiguration: Need at least 4 arguments in one line: TimeFrame - TimeEnd - FileLabel - PlotType1. Please look over the configuration file and adjust it accordingly." << std::endl;
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
			std::cout << "ERROR (MonitorLAPPDData): ReadInConfiguration: Could not open file " << plot_configuration << "! Check if path is valid..." << std::endl;
	}
	file.close();

	if (verbosity > 2) {
		std::cout << "---------------------------------------------------------------------" << std::endl;
		std::cout << "MonitorLAPPDData: ReadInConfiguration: Read in the following data into configuration variables: " << std::endl;
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
		std::cout << "MonitorLAPPDData: ReadInConfiguration: Parsing dates: " << std::endl;
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

void MonitorLAPPDData::LoadACDCBoardConfig(std::string acdc_config) {

	Log("MonitorLAPPDData: LoadACDCBoardConfig", v_message, verbosity);

	//Load the active ACDC board numbers specified in the configuration file
	ifstream acdc_file(acdc_config);
	int board_number;
	while (!acdc_file.eof()) {
		acdc_file >> board_number;
		if (acdc_file.eof())
			break;
		Log("MonitorLAPPDData: Setting Board number >>>" + std::to_string(board_number) + "<<< as an active LAPPD ACDC board number", v_message, verbosity);
		board_configuration.push_back(board_number);

		current_pps_rate.push_back(0.0);
		current_frame_rate.push_back(0.0);
		current_beamgate_rate.push_back(0.0);
		current_int_charge.push_back(0.0);
		current_buffer_size.push_back(0.0);
		current_num_entries.push_back(0);
		n_buffer.push_back(0);

		first_beamgate_timestamp.push_back(0);
		last_beamgate_timestamp.push_back(0);
		first_timestamp.push_back(0);
		last_timestamp.push_back(0);
		current_board_index.push_back(board_number);
		first_entry.push_back(true);

		std::vector<ULong64_t> empty_unsigned_long;
		std::vector<double> empty_double;
		std::vector<int> empty_int;
		std::vector<TDatime> empty_datime;
		data_times_plot.emplace(board_number, empty_unsigned_long);
		data_times_end_plot.emplace(board_number, empty_unsigned_long);
		pps_rate_plot.emplace(board_number, empty_double);
		frame_rate_plot.emplace(board_number, empty_double);
		beamgate_rate_plot.emplace(board_number, empty_double);
		int_charge_plot.emplace(board_number, empty_double);
		buffer_size_plot.emplace(board_number, empty_double);
		num_channels_plot.emplace(board_number, empty_int);
		labels_timeaxis.emplace(board_number, empty_datime);

	}
	acdc_file.close();

}

std::string MonitorLAPPDData::convertTimeStamp_to_Date(ULong64_t timestamp) {

	//format of date is YYYY_MM-DD

	boost::posix_time::ptime convertedtime = *Epoch + boost::posix_time::time_duration(int(timestamp / MSEC_to_SEC / SEC_to_MIN / MIN_to_HOUR), int(timestamp / MSEC_to_SEC / SEC_to_MIN) % 60, int(timestamp / MSEC_to_SEC / 1000.) % 60, timestamp % 1000);
	struct tm converted_tm = boost::posix_time::to_tm(convertedtime);
	std::stringstream ss_date;
	ss_date << converted_tm.tm_year + 1900 << "_" << converted_tm.tm_mon + 1 << "-" << converted_tm.tm_mday;
	return ss_date.str();

}

bool MonitorLAPPDData::does_file_exist(std::string filename) {

	std::ifstream infile(filename.c_str());
	bool file_good = infile.good();
	infile.close();
	return file_good;

}

void MonitorLAPPDData::WriteToFile() {

	Log("MonitorLAPPDData: WriteToFile", v_message, verbosity);

	//-------------------------------------------------------
	//------------------WriteToFile -------------------------
	//-------------------------------------------------------

	t_file_end.clear();
	t_file_end_global = 0;
	for (int i_board = 0; i_board < (int) board_configuration.size(); i_board++) {
		int board_nr = board_configuration.at(i_board);
		t_file_end.push_back(last_timestamp.at(i_board));
		if (last_timestamp.at(i_board) > t_file_end_global)
			t_file_end_global = last_timestamp.at(i_board);
	}

	std::string file_start_date = convertTimeStamp_to_Date(t_file_start);
	std::stringstream root_filename;
	root_filename << path_monitoring << "LAPPDData_" << file_start_date << ".root";

	Log("MonitorLAPPDData: ROOT filename: " + root_filename.str(), v_message, verbosity);

	std::string root_option = "RECREATE";
	if (does_file_exist(root_filename.str()))
		root_option = "UPDATE";
	TFile *f = new TFile(root_filename.str().c_str(), root_option.c_str());

	std::vector<ULong64_t> *t_time = new std::vector<ULong64_t>;
	std::vector<ULong64_t> *t_end = new std::vector<ULong64_t>;
	std::vector<double> *t_pps_rate = new std::vector<double>;
	std::vector<double> *t_frame_rate = new std::vector<double>;
	std::vector<double> *t_beamgate_rate = new std::vector<double>;
	std::vector<double> *t_int_charge = new std::vector<double>;
	std::vector<double> *t_buffer_size = new std::vector<double>;
	std::vector<int> *t_board_idx = new std::vector<int>;

	TTree *t;
	if (f->GetListOfKeys()->Contains("lappddatamonitor_tree")) {
		Log("MonitorLAPPDData: WriteToFile: Tree already exists", v_message, verbosity);
		t = (TTree*) f->Get("lappddatamonitor_tree");
		t->SetBranchAddress("t_start", &t_time);
		t->SetBranchAddress("t_end", &t_end);
		t->SetBranchAddress("pps_rate", &t_pps_rate);
		t->SetBranchAddress("frame_rate", &t_frame_rate);
		t->SetBranchAddress("beamgate_rate", &t_beamgate_rate);
		t->SetBranchAddress("int_charge", &t_int_charge);
		t->SetBranchAddress("buffer_size", &t_buffer_size);
		t->SetBranchAddress("board_idx", &t_board_idx);
	} else {
		t = new TTree("lappddatamonitor_tree", "LAPPD Data Monitoring tree");
		Log("MonitorLAPPDData: WriteToFile: Tree is created from scratch", v_message, verbosity);
		t->Branch("t_start", &t_time);
		t->Branch("t_end", &t_end);
		t->Branch("pps_rate", &t_pps_rate);
		t->Branch("frame_rate", &t_frame_rate);
		t->Branch("beamgate_rate", &t_beamgate_rate);
		t->Branch("int_charge", &t_int_charge);
		t->Branch("buffer_size", &t_buffer_size);
		t->Branch("board_idx", &t_board_idx);
	}

	int n_entries = t->GetEntries();
	bool omit_entries = false;
	for (int i_entry = 0; i_entry < n_entries; i_entry++) {
		t->GetEntry(i_entry);
		if (t_end->at(0) == t_file_end.at(0)) {
			Log("WARNING (MonitorLAPPDData): WriteToFile: Wanted to write data from file that is already written to DB. Omit entries", v_warning, verbosity);
			omit_entries = true;
		}
	}

	//if data is already written to DB/File, do not write it again
	if (omit_entries) {
		//don't write file again, but still delete TFile and TTree object!!!
		f->Close();
		delete t_time;
		delete t_end;
		delete t_pps_rate;
		delete t_frame_rate;
		delete t_beamgate_rate;
		delete t_int_charge;
		delete t_buffer_size;
		delete t_board_idx;
		delete f;

		gROOT->cd();

		return;
	}

	//If we have vectors, they need to be cleared
	t_time->clear();
	t_end->clear();
	t_pps_rate->clear();
	t_frame_rate->clear();
	t_beamgate_rate->clear();
	t_int_charge->clear();
	t_buffer_size->clear();
	t_board_idx->clear();

	//Get data that was processed
	for (int i_current = 0; i_current < (int) current_pps_rate.size(); i_current++) {
		t_time->push_back(first_timestamp.at(i_current));
		t_end->push_back(last_timestamp.at(i_current));
		t_pps_rate->push_back(current_pps_rate.at(i_current));
		t_frame_rate->push_back(current_frame_rate.at(i_current));
		t_beamgate_rate->push_back(current_beamgate_rate.at(i_current));
		t_int_charge->push_back(current_int_charge.at(i_current));
		t_buffer_size->push_back(current_buffer_size.at(i_current));
		t_board_idx->push_back(current_board_index.at(i_current));

		ULong64_t time = first_timestamp.at(i_current);
		boost::posix_time::ptime starttime = *Epoch + boost::posix_time::time_duration(int(time / MSEC_to_SEC / SEC_to_MIN / MIN_to_HOUR), int(time / MSEC_to_SEC / SEC_to_MIN) % 60, int(time / MSEC_to_SEC / 1000.) % 60, time % 1000);
		struct tm starttime_tm = boost::posix_time::to_tm(starttime);
		Log(
				"MonitorLAPPDData: WriteToFile: Board " + std::to_string(current_board_index.at(i_current)) + ". Writing data to file: " + std::to_string(starttime_tm.tm_year + 1900) + "/" + std::to_string(starttime_tm.tm_mon + 1) + "/" + std::to_string(starttime_tm.tm_mday) + "-"
						+ std::to_string(starttime_tm.tm_hour) + ":" + std::to_string(starttime_tm.tm_min) + ":" + std::to_string(starttime_tm.tm_sec), v_message, verbosity);
	}

	t->Fill();
	t->Write("", TObject::kOverwrite);     //prevent ROOT from making endless keys for the same tree when updating the tree
	f->Close();

	//Delete potential vectors
	delete t_time;
	delete t_end;
	delete t_pps_rate;
	delete t_frame_rate;
	delete t_beamgate_rate;
	delete t_int_charge;
	delete t_buffer_size;
	delete t_board_idx;
	delete f;

	gROOT->cd();

}

void MonitorLAPPDData::ReadFromFile(ULong64_t timestamp, double time_frame) {

	Log("MonitorLAPPDData: ReadFromFile", v_message, verbosity);

	//-------------------------------------------------------
	//------------------ReadFromFile ------------------------
	//-------------------------------------------------------

	for (int i_board = 0; i_board < (int) board_configuration.size(); i_board++) {
		int board_nr = board_configuration.at(i_board);
		data_times_plot.at(board_nr).clear();
		data_times_end_plot.at(board_nr).clear();
		pps_rate_plot.at(board_nr).clear();
		frame_rate_plot.at(board_nr).clear();
		beamgate_rate_plot.at(board_nr).clear();
		int_charge_plot.at(board_nr).clear();
		buffer_size_plot.at(board_nr).clear();
		num_channels_plot.at(board_nr).clear();
		labels_timeaxis.at(board_nr).clear();
	}

	//take the end time and calculate the start time with the given time_frame
	ULong64_t timestamp_start = timestamp - time_frame * MIN_to_HOUR * SEC_to_MIN * MSEC_to_SEC;
	boost::posix_time::ptime starttime = *Epoch + boost::posix_time::time_duration(int(timestamp_start / MSEC_to_SEC / SEC_to_MIN / MIN_to_HOUR), int(timestamp_start / MSEC_to_SEC / SEC_to_MIN) % 60, int(timestamp_start / MSEC_to_SEC / 1000.) % 60, timestamp_start % 1000);
	struct tm starttime_tm = boost::posix_time::to_tm(starttime);
	boost::posix_time::ptime endtime = *Epoch + boost::posix_time::time_duration(int(timestamp / MSEC_to_SEC / SEC_to_MIN / MIN_to_HOUR), int(timestamp / MSEC_to_SEC / SEC_to_MIN) % 60, int(timestamp / MSEC_to_SEC / 1000.) % 60, timestamp % 1000);
	struct tm endtime_tm = boost::posix_time::to_tm(endtime);

	Log(
			"MonitorLAPPDData: ReadFromFile: Reading in data for time frame " + std::to_string(starttime_tm.tm_year + 1900) + "/" + std::to_string(starttime_tm.tm_mon + 1) + "/" + std::to_string(starttime_tm.tm_mday) + "-" + std::to_string(starttime_tm.tm_hour) + ":"
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
		root_filename_i << path_monitoring << "LAPPDData_" << string_date_i << ".root";
		bool tree_exists = true;

		if (does_file_exist(root_filename_i.str())) {
			TFile *f = new TFile(root_filename_i.str().c_str(), "READ");
			TTree *t;
			if (f->GetListOfKeys()->Contains("lappddatamonitor_tree"))
				t = (TTree*) f->Get("lappddatamonitor_tree");
			else {
				Log("WARNING (MonitorLAPPDData): File " + root_filename_i.str() + " does not contain lappddatamonitor_tree. Omit file.", v_warning, verbosity);
				tree_exists = false;
			}

			if (tree_exists) {

				Log("MonitorLAPPDData: Tree exists, start reading in data", v_message, verbosity);

				std::vector<ULong64_t> *t_time = new std::vector<ULong64_t>;
				std::vector<ULong64_t> *t_end = new std::vector<ULong64_t>;
				std::vector<double> *t_pps_rate = new std::vector<double>;
				std::vector<double> *t_frame_rate = new std::vector<double>;
				std::vector<double> *t_beamgate_rate = new std::vector<double>;
				std::vector<double> *t_int_charge = new std::vector<double>;
				std::vector<double> *t_buffer_size = new std::vector<double>;
				std::vector<int> *t_board_idx = new std::vector<int>;

				int nentries_tree;

				t->SetBranchAddress("t_start", &t_time);
				t->SetBranchAddress("t_end", &t_end);
				t->SetBranchAddress("pps_rate", &t_pps_rate);
				t->SetBranchAddress("frame_rate", &t_frame_rate);
				t->SetBranchAddress("beamgate_rate", &t_beamgate_rate);
				t->SetBranchAddress("int_charge", &t_int_charge);
				t->SetBranchAddress("buffer_size", &t_buffer_size);
				t->SetBranchAddress("board_idx", &t_board_idx);

				nentries_tree = t->GetEntries();

				for (int i_board = 0; i_board < (int) board_configuration.size(); i_board++) {
					int board_nr = board_configuration[i_board];

					//Sort timestamps for the case that they are not in order

					std::vector<ULong64_t> vector_timestamps;
					std::map<ULong64_t, int> map_timestamp_entry;
					for (int i_entry = 0; i_entry < nentries_tree; i_entry++) {
						t->GetEntry(i_entry);
						if (t_time->at(i_board) >= timestamp_start && t_end->at(i_board) <= timestamp) {
							vector_timestamps.push_back(t_time->at(i_board));
							map_timestamp_entry.emplace(t_time->at(i_board), i_entry);
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
						if (t_time->at(i_board) >= timestamp_start && t_end->at(i_board) <= timestamp) {
							data_times_plot.at(board_nr).push_back(t_time->at(i_board));
							data_times_end_plot.at(board_nr).push_back(t_end->at(i_board));
							pps_rate_plot.at(board_nr).push_back(t_pps_rate->at(i_board));
							frame_rate_plot.at(board_nr).push_back(t_frame_rate->at(i_board));
							beamgate_rate_plot.at(board_nr).push_back(t_beamgate_rate->at(i_board));
							int_charge_plot.at(board_nr).push_back(t_int_charge->at(i_board));
							buffer_size_plot.at(board_nr).push_back(t_buffer_size->at(i_board));

							boost::posix_time::ptime boost_tend = *Epoch
									+ boost::posix_time::time_duration(int(t_end->at(i_board) / MSEC_to_SEC / SEC_to_MIN / MIN_to_HOUR), int(t_end->at(i_board) / MSEC_to_SEC / SEC_to_MIN) % 60, int(t_end->at(i_board) / MSEC_to_SEC / 1000.) % 60, t_end->at(i_board) % 1000);
							struct tm label_timestamp = boost::posix_time::to_tm(boost_tend);

							TDatime datime_timestamp(1900 + label_timestamp.tm_year, label_timestamp.tm_mon + 1, label_timestamp.tm_mday, label_timestamp.tm_hour, label_timestamp.tm_min, label_timestamp.tm_sec);
							labels_timeaxis.at(board_nr).push_back(datime_timestamp);
						}
					}
				}

				//Delete vectors, if we have any
				delete t_time;
				delete t_end;
				delete t_pps_rate;
				delete t_frame_rate;
				delete t_beamgate_rate;
				delete t_int_charge;
				delete t_buffer_size;
				delete t_board_idx;
			}

			f->Close();
			delete f;
			gROOT->cd();

		} else {
			Log("MonitorLAPPDData: ReadFromFile: File " + root_filename_i.str() + " does not exist. Omit file.", v_warning, verbosity);
		}

	}

	//Set the readfromfile time variables to make sure data is not read twice for the same time window
	readfromfile_tend = timestamp;
	readfromfile_timeframe = time_frame;
}

void MonitorLAPPDData::DrawLastFilePlots() {

	Log("MonitorLAPPDData: DrawLastFilePlots", v_message, verbosity);

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

void MonitorLAPPDData::UpdateMonitorPlots(std::vector<double> timeFrames, std::vector<ULong64_t> endTimes, std::vector<std::string> fileLabels, std::vector<std::vector<std::string>> plotTypes) {

	Log("MonitorLAPPDData: UpdateMonitorPlots", v_message, verbosity);

	//-------------------------------------------------------
	//------------------UpdateMonitorPlots ------------------
	//-------------------------------------------------------

	//Draw the monitoring plots according to the specifications in the configfiles

	for (unsigned int i_time = 0; i_time < timeFrames.size(); i_time++) {

		ULong64_t zero = 0;
		if (endTimes.at(i_time) == zero)
			endTimes.at(i_time) = t_file_end_global;        //set 0 for t_file_end since we did not know what that was at the beginning of initialise

		for (unsigned int i_plot = 0; i_plot < plotTypes.at(i_time).size(); i_plot++) {
			if (plotTypes.at(i_time).at(i_plot) == "TimeEvolution")
				DrawTimeEvolutionLAPPDData(endTimes.at(i_time), timeFrames.at(i_time), fileLabels.at(i_time));
			else {
				if (verbosity > 0)
					std::cout << "ERROR (MonitorLAPPDData): UpdateMonitorPlots: Specified plot type -" << plotTypes.at(i_time).at(i_plot) << "- does not exist! Omit entry." << std::endl;
			}
		}
	}

}

void MonitorLAPPDData::DrawStatus_PsecData() {

	Log("MonitorLAPPDData: DrawStatus_PsecData", v_message, verbosity);

	//-------------------------------------------------------
	//-------------DrawStatus_PsecData ----------------------
	//-------------------------------------------------------

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

	//TODO: Implement checks for bad values and draw them red

	double meanPPSRate = 0.0;
	double meanFrameRate = 0.0;
	double meanBufferSize = 0.0;
	double meanIntegratedCharge = 0.0;
	//Calculate mean values
	for (int i_board = 0; i_board < current_pps_rate.size(); i_board++) {
		int board_nr = board_configuration.at(i_board);
		if(board_nr != -1){
			meanPPSRate += (current_pps_rate.at(i_board) / (double) current_pps_rate.size());
			meanFrameRate += (current_frame_rate.at(i_board) / (double) current_frame_rate.size());
			meanBufferSize += (current_buffer_size.at(i_board) / (double) current_buffer_size.size());
			meanIntegratedCharge += (current_int_charge.at(i_board) / (double) current_int_charge.size());
		}
	}

	boost::posix_time::ptime currenttime = *Epoch + boost::posix_time::time_duration(int(t_current / MSEC_to_SEC / SEC_to_MIN / MIN_to_HOUR), int(t_current / MSEC_to_SEC / SEC_to_MIN) % 60, int(t_current / MSEC_to_SEC) % 60, t_current % 1000);
	struct tm currenttime_tm = boost::posix_time::to_tm(currenttime);
	std::stringstream current_time;
	current_time << currenttime_tm.tm_year + 1900 << "/" << currenttime_tm.tm_mon + 1 << "/" << currenttime_tm.tm_mday << "-" << currenttime_tm.tm_hour << ":" << currenttime_tm.tm_min << ":" << currenttime_tm.tm_sec;

	text_data_title->SetText(0.06, 0.9, "LAPPD PSEC Data");
	std::stringstream ss_text_psec_pps_rate;
	ss_text_psec_pps_rate << "Mean PPS Rate: " << meanPPSRate << " (" << current_time.str() << ")";
	text_pps_rate->SetText(0.06, 0.8, ss_text_psec_pps_rate.str().c_str());
	text_pps_rate->SetTextColor(1);
//	if (lappd_sc.temperature_mon < lappd_sc.LIMIT_temperature_low || lappd_sc.temperature_mon > lappd_sc.LIMIT_temperature_high) {
//		Log("MonitorLAPPDSC: SEVERE ERROR: Monitored temperature >>>" + std::to_string(lappd_sc.temperature_mon) + "<<< is outside of expected range [" + std::to_string(lappd_sc.LIMIT_temperature_low) + " , " + std::to_string(lappd_sc.LIMIT_temperature_high) + "]!!!", v_error, verbosity);
//		text_temp->SetTextColor(kRed);
//		temp_humid_check = false;
//	}

	std::stringstream ss_text_psec_frame_rate;
	ss_text_psec_frame_rate << "Mean Frame Rate: " << meanFrameRate << " (" << current_time.str() << ")";
	text_frame_rate->SetText(0.06, 0.7, ss_text_psec_frame_rate.str().c_str());
	text_frame_rate->SetTextColor(1);

	std::stringstream ss_text_psec_buffer_size;
	ss_text_psec_buffer_size << "Mean Buffer size: " << meanBufferSize << " (" << current_time.str() << ")";
	text_buffer_size->SetText(0.06, 0.6, ss_text_psec_buffer_size.str().c_str());
	text_buffer_size->SetTextColor(1);

	std::stringstream ss_text_psec_integrated_charge;
	ss_text_psec_integrated_charge << "Mean Integrated Charge : " << meanIntegratedCharge << " (" << current_time.str() << ")";
	text_int_charge->SetText(0.06, 0.5, ss_text_psec_integrated_charge.str().c_str());
	text_int_charge->SetTextColor(1);

	text_data_title->SetTextSize(0.05);
	text_pps_rate->SetTextSize(0.05);
	text_frame_rate->SetTextSize(0.05);
	text_buffer_size->SetTextSize(0.05);
	text_int_charge->SetTextSize(0.05);

	text_data_title->SetNDC(1);
	text_pps_rate->SetNDC(1);
	text_frame_rate->SetNDC(1);
	text_buffer_size->SetNDC(1);
	text_int_charge->SetNDC(1);

	canvas_status_data->cd();
	canvas_status_data->Clear();
	text_data_title->Draw();
	text_pps_rate->Draw();
	text_frame_rate->Draw();
	text_buffer_size->Draw();
//	text_int_charge->Draw();

	std::stringstream ss_path_psecinfo;
	ss_path_psecinfo << outpath << "LAPPDData_PSECData_current." << img_extension;
	canvas_status_data->SaveAs(ss_path_psecinfo.str().c_str());
	canvas_status_data->Clear();

	for (int i_board = 0; i_board < (int) board_configuration.size(); i_board++) {

		int board_nr = board_configuration.at(i_board);

		uint64_t t_fileend = t_file_end.at(i_board);
		boost::posix_time::ptime currenttime = *Epoch + boost::posix_time::time_duration(int(t_fileend / MSEC_to_SEC / SEC_to_MIN / MIN_to_HOUR), int(t_fileend / MSEC_to_SEC / SEC_to_MIN) % 60, int(t_fileend / MSEC_to_SEC) % 60, t_fileend % 1000);
		struct tm currenttime_tm = boost::posix_time::to_tm(currenttime);
		std::stringstream current_time;
		current_time << currenttime_tm.tm_year + 1900 << "/" << currenttime_tm.tm_mon + 1 << "/" << currenttime_tm.tm_mday << "-" << currenttime_tm.tm_hour << ":" << currenttime_tm.tm_min << ":" << currenttime_tm.tm_sec;
		std::stringstream ss_board_title;
		ss_board_title << "LAPPD PSEC Data Board " << board_nr;
		text_data_title->SetText(0.06, 0.9, ss_board_title.str().c_str());

		std::stringstream ss_text_psec_pps_rate_board;
		ss_text_psec_pps_rate_board << "PPS Rate: " << current_pps_rate.at(i_board) << " (" << current_time.str() << ")";
		text_pps_rate->SetText(0.06, 0.8, ss_text_psec_pps_rate_board.str().c_str());
		text_pps_rate->SetTextColor(1);

		std::stringstream ss_text_psec_frame_rate_board;
		ss_text_psec_frame_rate_board << "Frame Rate: " << current_frame_rate.at(i_board) << " (" << current_time.str() << ")";
		text_frame_rate->SetText(0.06, 0.7, ss_text_psec_frame_rate_board.str().c_str());
		text_frame_rate->SetTextColor(1);

		std::stringstream ss_text_psec_buffer_size_board;
		ss_text_psec_buffer_size_board << "Buffer size: " << current_buffer_size.at(i_board) << " (" << current_time.str() << ")";
		text_buffer_size->SetText(0.06, 0.6, ss_text_psec_buffer_size_board.str().c_str());
		text_buffer_size->SetTextColor(1);

		std::stringstream ss_text_psec_integrated_charge_board;
		ss_text_psec_integrated_charge_board << "Integrated Charge : " << current_int_charge.at(i_board) << " (" << current_time.str() << ")";
		text_int_charge->SetText(0.06, 0.5, ss_text_psec_integrated_charge_board.str().c_str());
		text_int_charge->SetTextColor(1);

		text_data_title->SetTextSize(0.05);
		text_pps_rate->SetTextSize(0.05);
		text_frame_rate->SetTextSize(0.05);
		text_buffer_size->SetTextSize(0.05);
		text_int_charge->SetTextSize(0.05);

		text_data_title->SetNDC(1);
		text_pps_rate->SetNDC(1);
		text_frame_rate->SetNDC(1);
		text_buffer_size->SetNDC(1);
		text_int_charge->SetNDC(1);

		canvas_status_data->cd();
		canvas_status_data->Clear();
		text_data_title->Draw();
		text_pps_rate->Draw();
		text_frame_rate->Draw();
		text_buffer_size->Draw();
//		text_int_charge->Draw();

		std::stringstream ss_path_boardpsecinfo;
		ss_path_boardpsecinfo << outpath << "LAPPDData_PSECData_currentBoardNr" << board_nr << "." << img_extension;
		canvas_status_data->SaveAs(ss_path_boardpsecinfo.str().c_str());
		canvas_status_data->Clear();

	}

}

//ToDo: Also implement an overall status board showing whether everything is awesome and if there is something wrong, which board has issues

void MonitorLAPPDData::DrawLastFileHists() {
	std::vector<int> colorVec;
	colorVec.push_back(632);
	colorVec.push_back(416);
	colorVec.push_back(600);
	colorVec.push_back(400);
	colorVec.push_back(616);
	colorVec.push_back(432);

	Log("MonitorLAPPDData: DrawLastFileHists", v_message, verbosity);

	//-------------------------------------------------------
	//-------------DrawLastFileHists ------------------------
	//-------------------------------------------------------

	//TODO: Add histogram that shows the number of active channelkeys for each board

	//Draw histograms for each board
	for (int i_board = 0; i_board < (int) board_configuration.size(); i_board++) {

		int board_nr = board_configuration.at(i_board);
		uint64_t t_fileend = t_file_end.at(i_board);
		boost::posix_time::ptime currenttime = *Epoch + boost::posix_time::time_duration(int(t_fileend / MSEC_to_SEC / SEC_to_MIN / MIN_to_HOUR), int(t_fileend / MSEC_to_SEC / SEC_to_MIN) % 60, int(t_fileend / MSEC_to_SEC) % 60, t_fileend % 1000);
		struct tm currenttime_tm = boost::posix_time::to_tm(currenttime);
		std::stringstream current_time;
		current_time << currenttime_tm.tm_year + 1900 << "/" << currenttime_tm.tm_mon + 1 << "/" << currenttime_tm.tm_mday << "-" << currenttime_tm.tm_hour << ":" << currenttime_tm.tm_min << ":" << currenttime_tm.tm_sec;

		canvas_adc_channel->Clear();
		canvas_adc_channel->cd();
		std::stringstream ss_text_adc;
		ss_text_adc << "ADC value vs. channelkey Board " << board_nr << " (" << current_time.str() << ")";
		hist_adc_channel.at(board_nr)->SetTitle(ss_text_adc.str().c_str());
		hist_adc_channel.at(board_nr)->SetStats(0);
		hist_adc_channel.at(board_nr)->Draw("colz");
		std::stringstream ss_path_adc;
		ss_path_adc << outpath << "LAPPDData_ADC_Chkey_Board" << board_nr << "_current." << img_extension;
		canvas_adc_channel->SaveAs(ss_path_adc.str().c_str());

		canvas_waveform->Clear();
		canvas_waveform->cd();
		canvas_waveform->SetRightMargin(0.15);
		std::stringstream ss_text_waveform;
		ss_text_waveform << "Exemplary waveform Board " << board_nr << " (" << current_time.str() << ")";
		hist_waveform_channel.at(board_nr)->SetTitle(ss_text_waveform.str().c_str());
		hist_waveform_channel.at(board_nr)->SetStats(0);
		hist_waveform_channel.at(board_nr)->Draw("colz");
		std::stringstream ss_path_waveform;
		ss_path_waveform << outpath << "LAPPDData_Waveform_Board" << board_nr << "_current." << img_extension;
		canvas_waveform->SaveAs(ss_path_waveform.str().c_str());

		canvas_buffer_channel->Clear();
		canvas_buffer_channel->cd();
		std::stringstream ss_text_buffer_ch;
		ss_text_buffer_ch << "LAPPD buffer size vs. channelkey Board " << board_nr << " (" << current_time.str() << ")";
		hist_buffer_channel.at(board_nr)->SetTitle(ss_text_buffer_ch.str().c_str());
		hist_buffer_channel.at(board_nr)->SetStats(0);
		hist_buffer_channel.at(board_nr)->Draw("colz");
		std::stringstream ss_path_buffer_ch;
		ss_path_buffer_ch << outpath << "LAPPDData_Buffer_Ch_Board" << board_nr << "_current." << img_extension;
		canvas_buffer_channel->SaveAs(ss_path_buffer_ch.str().c_str());

		canvas_buffer->Clear();
		canvas_buffer->cd();
		std::stringstream ss_text_buffer;
		ss_text_buffer << "LAPPD buffer size Board " << board_nr << " (" << current_time.str() << ")";
		hist_buffer.at(board_nr)->SetTitle(ss_text_buffer.str().c_str());
		hist_buffer.at(board_nr)->SetStats(0);
		hist_buffer.at(board_nr)->Draw();
		std::stringstream ss_path_buffer;
		ss_path_buffer << outpath << "LAPPDData_Buffer_Board" << board_nr << "_current." << img_extension;
		canvas_buffer->SaveAs(ss_path_buffer.str().c_str());

		canvas_waveform_voltages->Clear();
		canvas_waveform_voltages->cd();
		canvas_waveform_voltages->SetRightMargin(0.15);
		std::stringstream ss_text_waveform_voltages;
		ss_text_waveform_voltages << "LAPPD waverform Board " << board_nr << " (" << current_time.str() << ")";
		hist_waveform_voltages.at(board_nr)->SetTitle(ss_text_waveform_voltages.str().c_str());
		hist_waveform_voltages.at(board_nr)->SetStats(0);
		hist_waveform_voltages.at(board_nr)->Draw("COLZ");
		std::stringstream ss_path_waveform_voltages;
		ss_path_waveform_voltages << outpath << "LAPPDWaveform_Voltages_Board" << board_nr << "_current." << img_extension;
		canvas_waveform_voltages->SaveAs(ss_path_waveform_voltages.str().c_str());

		PedestalFits(board_nr, i_board);


		//ToDo: Entries not equal events
		long entries;
		LAPPDData->Header->Get("TotalEntries", entries);
		std::vector<bool> isPlotted;
		for (size_t i_channel = 0; i_channel < 30; i_channel++) {
			isPlotted.push_back(false);
		}


		for (int i_event = 0; i_event < (int) entries; i_event++) {
			for (size_t i_channel = 0; i_channel < hist_waveforms_onedim.at(i_event).at(board_nr).size(); i_channel++) {
				canvas_waveform_onedim->Clear();
				canvas_waveform_onedim->cd();
				std::stringstream ss_text_waveform_onedim;
				ss_text_waveform_onedim << "LAPPD waveform board " << board_nr << " channel " << i_channel << " event number " << i_event << " (" << current_time.str() << ")";
				hist_waveforms_onedim.at(i_event).at(board_nr).at(i_channel)->SetTitle(ss_text_waveform_onedim.str().c_str());
				hist_waveforms_onedim.at(i_event).at(board_nr).at(i_channel)->SetStats(0);
//			hist_waveforms_onedim.at(board_nr).at(i_channel)->SetMarkerColor(colorVec.at(i_channel));
//			hist_waveforms_onedim.at(board_nr).at(i_channel)->SetLineColor(colorVec.at(i_channel));
				hist_waveforms_onedim.at(i_event).at(board_nr).at(i_channel)->Draw("HIST");
				//ToDo: Find reasonable threshold for displaying pulses
				if((hist_waveforms_onedim.at(i_event).at(board_nr).at(i_channel)->GetMaximum() > mean_pedestal.at(board_nr).at(i_channel) + 2*sigma_pedestal.at(board_nr).at(i_channel))
						|| (i_event + 1 == (int)entries && !isPlotted.at(i_channel))){
				std::stringstream ss_path_waveform_onedim;
				ss_path_waveform_onedim << outpath << "LAPPDWaveform_waveform_onedim_Board" << board_nr << "channel" << i_channel << "_current." << img_extension;
				canvas_waveform_onedim->SaveAs(ss_path_waveform_onedim.str().c_str());
				isPlotted.at(i_channel) = true;

//				Testing
//				std::cout << "i_event " << i_event << " i_channel " << i_channel << std::endl;
//				std::cout << "Pedestal " << mean_pedestal.at(board_nr).at(i_channel) << std::endl;
//				std::cout << "Maximum " << hist_waveforms_onedim.at(i_event).at(board_nr).at(i_channel)->GetMaximum() << std::endl;

				}
			}
		}




	}// end board loop

	canvas_pedestal_all->Clear();
	canvas_pedestal_all->cd();
	canvas_pedestal_all->SetRightMargin(0.15);
	TPad *p = (TPad*) canvas_pedestal_all->cd();
	p->SetGrid();
	hist_pedestal_all->Draw("COLZ");
	p->Update();
	std::stringstream ss_path_pedestals;
	ss_path_pedestals << outpath << "LAPPD_Pedestals_current." << img_extension;
	canvas_pedestal_all->SaveAs(ss_path_pedestals.str().c_str());

	canvas_buffer_size_all->Clear();
	canvas_buffer_size_all->cd();
	canvas_buffer_size_all->SetRightMargin(0.15);
	TPad *prate = (TPad*) canvas_buffer_size_all->cd();
	prate->SetGrid();
	hist_buffer_size_all->Draw("COLZ");
	prate->Update();
	std::stringstream ss_path_rate;
	ss_path_rate << outpath << "LAPPD_Buffer_Size_current." << img_extension;
	canvas_buffer_size_all->SaveAs(ss_path_rate.str().c_str());

	canvas_rate_threshold_all->Clear();
	canvas_rate_threshold_all->cd();
	canvas_rate_threshold_all->SetRightMargin(0.15);
	TPad *pthre = (TPad*) canvas_rate_threshold_all->cd();
	pthre->SetGrid();
	hist_rate_threshold_all->Draw("COLZ");
	pthre->Update();
	std::stringstream ss_path_rate_threshold;
	ss_path_rate_threshold << outpath << "LAPPD_Rates_Threshold_current." << img_extension;
	canvas_rate_threshold_all->SaveAs(ss_path_rate_threshold.str().c_str());

	canvas_events_per_channel->Clear();
	canvas_events_per_channel->cd();
	canvas_events_per_channel->SetRightMargin(0.15);
	TPad *pevents = (TPad*) canvas_events_per_channel->cd();
	pevents->SetGrid();
	hist_events_per_channel->Draw("COLZ");
	pevents->Update();
	std::stringstream ss_path_events_channel;
	ss_path_events_channel << outpath << "LAPPD_Events_Per_Channel_current." << img_extension;
	canvas_events_per_channel->SaveAs(ss_path_events_channel.str().c_str());


}

void MonitorLAPPDData::PedestalFits(int board_nr, int i_board){
	uint64_t t_fileend = t_file_end.at(i_board);
	boost::posix_time::ptime currenttime = *Epoch + boost::posix_time::time_duration(int(t_fileend / MSEC_to_SEC / SEC_to_MIN / MIN_to_HOUR), int(t_fileend / MSEC_to_SEC / SEC_to_MIN) % 60, int(t_fileend / MSEC_to_SEC) % 60, t_fileend % 1000);
	struct tm currenttime_tm = boost::posix_time::to_tm(currenttime);
	std::stringstream current_time;
	current_time << currenttime_tm.tm_year + 1900 << "/" << currenttime_tm.tm_mon + 1 << "/" << currenttime_tm.tm_mday << "-" << currenttime_tm.tm_hour << ":" << currenttime_tm.tm_min << ":" << currenttime_tm.tm_sec;



	std::vector<int> entries_vec;
	std::vector<double> mean_vec;
	std::vector<double> sigma_vec;
	for (size_t i_channel = 0; i_channel < hist_pedestal.at(board_nr).size(); i_channel++) {
		canvas_pedestal->Clear();
		canvas_pedestal->cd();
		std::stringstream ss_path_pedestal;
		ss_path_pedestal << outpath << "LAPPDPedestal_Board" << board_nr << "channel" << i_channel << "_current." << img_extension;
		std::stringstream ss_text_pedestal;
		ss_text_pedestal << "LAPPD pedestal board " << board_nr << "channel " << i_channel << " (" << current_time.str() << ")";
		hist_pedestal.at(board_nr).at(i_channel)->SetTitle(ss_text_pedestal.str().c_str());
		hist_pedestal.at(board_nr).at(i_channel)->SetStats(0);
		entries_vec.push_back(hist_pedestal.at(board_nr).at(i_channel)->GetEntries());
		double minimum_adc = 0;
		double maximum_adc = 4000;
		TF1 *fgaus = new TF1("fgaus", "gaus", minimum_adc, maximum_adc);
		fgaus->SetParameter(1, hist_pedestal.at(board_nr).at(i_channel)->GetMean());
		fgaus->SetParameter(2, hist_pedestal.at(board_nr).at(i_channel)->GetRMS());
		TFitResultPtr gaussFitResult = hist_pedestal.at(board_nr).at(i_channel)->Fit("fgaus", "Q");
		int gaussFitResultInt = gaussFitResult;

		//TODO: Put this in own function and tune parameters

		hist_pedestal.at(board_nr).at(i_channel)->Draw("C");
		fgaus->Draw("SAME");
		canvas_pedestal->SaveAs(ss_path_pedestal.str().c_str());

		int counter = 0;
		int counterThreshold = 0;

		if (gaussFitResultInt == 0) {
			bool outOfBounds = ((fgaus->GetParameter(1) < 2000.) || (fgaus->GetParameter(1) > 3200.) || (fgaus->GetParameter(1) > 500.) || (fgaus->GetParameter(1) < 50.));
			bool suddenChange = false;
			//Look for a sudden change if we do not perform the first fit per board and channel
			if (mean_pedestal.size() > i_board) {
				if (mean_pedestal.at(board_nr).size() > i_channel) {
					suddenChange = (fabs(fgaus->GetParameter(1) - mean_pedestal.at(board_nr).at(i_channel)) > 10 || fabs(fgaus->GetParameter(2) - sigma_pedestal.at(board_nr).at(i_channel)) > 0.2);
				}
			}
			if (!outOfBounds && !suddenChange) {
				mean_vec.push_back(fgaus->GetParameter(1));
				sigma_vec.push_back(fgaus->GetParameter(2));
				hist_pedestal_all->SetBinContent(i_channel + 1, i_board + 1, fgaus->GetParameter(1));
			} else {
				mean_vec.push_back(hist_pedestal.at(board_nr).at(i_channel)->GetMean());
				sigma_vec.push_back(hist_pedestal.at(board_nr).at(i_channel)->GetRMS());
				hist_pedestal_all->SetBinContent(i_channel + 1, i_board + 1, hist_pedestal.at(board_nr).at(i_channel)->GetMean());
			}
		} else {
			mean_vec.push_back(hist_pedestal.at(board_nr).at(i_channel)->GetMean());
			sigma_vec.push_back(hist_pedestal.at(board_nr).at(i_channel)->GetRMS());
			hist_pedestal_all->SetBinContent(i_channel + 1, i_board + 1, hist_pedestal.at(board_nr).at(i_channel)->GetMean());
		}

		for (int i_bin = 0; i_bin < hist_pedestal.at(board_nr).at(i_channel)->GetNbinsX(); i_bin++) {
			double center = hist_pedestal.at(board_nr).at(i_channel)->GetBinCenter(i_bin);
			int content = hist_pedestal.at(board_nr).at(i_channel)->GetBinContent(i_bin);
			if (center > 0) {
				counter += content;
			}
			//TODO: Find better parameter
			int x = 5;
			if (center > (mean_vec.at(i_channel) + x*sigma_vec.at(i_channel))) {
				counterThreshold += content;
			}
		}

		//ToDo: Entries not equal to events
		long entries;
		LAPPDData->Header->Get("TotalEntries", entries);
		int bufferSize = counter/entries;
		//ToDo: Fix this number in config file?
		int numberOfSamples = 256;
		int numberOfEvents = counter/numberOfSamples;
		hist_buffer_size_all->SetBinContent(i_channel + 1, i_board + 1, bufferSize);
		hist_rate_threshold_all->SetBinContent(i_channel + 1, i_board + 1, counterThreshold);
		hist_events_per_channel->SetBinContent(i_channel + 1, i_board + 1, numberOfEvents);
		delete fgaus;
	}
	num_entries.emplace(board_nr, entries_vec);
	mean_pedestal.emplace(board_nr, mean_vec);
	sigma_pedestal.emplace(board_nr, sigma_vec);
}



void MonitorLAPPDData::DrawTimeAlignment() {

	Log("MonitorLAPPDData: DrawTimeAlignment", v_message, verbosity);

	//-------------------------------------------------------
	//-------------DrawTimeAlignment ------------------------
	//-------------------------------------------------------

	//TODO: Implement function to fill time alignment histograms
	//Probably for different time frames: Last file, last 10 files, last 20 files, ...

	//TODO: Add titles to histograms
	//TODO: Write canvas as image to disk

	for (int i_board = 0; i_board < (int) board_configuration.size(); i_board++) {

		int board_nr = board_configuration.at(i_board);
		uint64_t t_fileend = t_file_end.at(i_board);
		boost::posix_time::ptime currenttime = *Epoch + boost::posix_time::time_duration(int(t_fileend / MSEC_to_SEC / SEC_to_MIN / MIN_to_HOUR), int(t_fileend / MSEC_to_SEC / SEC_to_MIN) % 60, int(t_fileend / MSEC_to_SEC) % 60, t_fileend % 1000);
		struct tm currenttime_tm = boost::posix_time::to_tm(currenttime);
		std::stringstream current_time;
		current_time << currenttime_tm.tm_year + 1900 << "/" << currenttime_tm.tm_mon + 1 << "/" << currenttime_tm.tm_mday << "-" << currenttime_tm.tm_hour << ":" << currenttime_tm.tm_min << ":" << currenttime_tm.tm_sec;

		//--------Last File-----------
		hist_align_1file.at(board_nr)->Clear();
		std::stringstream ss_1file;
		ss_1file << "hist_align_1file_board" << i_board;
		//  hist_align_1file.at(board_nr)->SetName(ss_1file.str().c_str());
		//  hist_align_1file.at(board_nr)->SetTitle(ss_1file.str().c_str());
		for (int i_align = 0; i_align < (int) data_beamgate_lastfile.size(); i_align++) {
			hist_align_1file.at(board_nr)->Fill(data_beamgate_lastfile.at(i_align));
		}

		//-------Last 5 Files---------
		hist_align_5files.at(board_nr)->Clear();
		for (int i_align = 0; i_align < (int) data_beamgate_last5files.size(); i_align++) {
			for (int i_data = 0; i_data < (int) data_beamgate_last5files.at(i_align).size(); i_data++) {
				hist_align_5files.at(board_nr)->Fill(data_beamgate_last5files.at(i_align).at(i_data));
			}
		}

		//-------Last 10 Files--------
		hist_align_10files.at(board_nr)->Clear();
		for (int i_align = 0; i_align < (int) data_beamgate_last10files.size(); i_align++) {
			for (int i_data = 0; i_data < (int) data_beamgate_last10files.at(i_align).size(); i_data++) {
				hist_align_10files.at(board_nr)->Fill(data_beamgate_last10files.at(i_align).at(i_data));
			}
		}

		//------Last 20 Files---------
		hist_align_20files.at(board_nr)->Clear();
		for (int i_align = 0; i_align < (int) data_beamgate_last20files.size(); i_align++) {
			for (int i_data = 0; i_data < (int) data_beamgate_last20files.at(i_align).size(); i_data++) {
				hist_align_20files.at(board_nr)->Fill(data_beamgate_last20files.at(i_align).at(i_data));
			}
		}

		//------Last 100 Files---------
		hist_align_100files.at(board_nr)->Clear();
		for (int i_align = 0; i_align < (int) data_beamgate_last100files.size(); i_align++) {
			for (int i_data = 0; i_data < (int) data_beamgate_last100files.at(i_align).size(); i_data++) {
				hist_align_100files.at(board_nr)->Fill(data_beamgate_last100files.at(i_align).at(i_data));
			}
		}
		//The drawing happens here
		canvas_align_1file->Clear();
		canvas_align_1file->cd();
		std::stringstream ss_text_align1;
		ss_text_align1 << "Alignment One File Board " << board_nr << " (" << current_time.str() << ")";
		hist_align_1file.at(board_nr)->SetTitle(ss_text_align1.str().c_str());
		hist_align_1file.at(board_nr)->SetStats(0);
		hist_align_1file.at(board_nr)->Draw("");
		std::stringstream ss_path_align1;
		ss_path_align1 << outpath << "LAPPD_Time_Alignment_One_File_Board" << board_nr << "_current." << img_extension;
		canvas_align_1file->SaveAs(ss_path_align1.str().c_str());

		canvas_align_5files->Clear();
		canvas_align_5files->cd();
		std::stringstream ss_text_align5;
		ss_text_align5 << "Alignment Five Files Board " << board_nr << " (" << current_time.str() << ")";
		hist_align_5files.at(board_nr)->SetTitle(ss_text_align5.str().c_str());
		hist_align_5files.at(board_nr)->SetStats(0);
		hist_align_5files.at(board_nr)->Draw("");
		std::stringstream ss_path_align5;
		ss_path_align5 << outpath << "LAPPD_Time_Alignment_Five_Files_Board" << board_nr << "_current." << img_extension;
		canvas_align_5files->SaveAs(ss_path_align5.str().c_str());

		canvas_align_10files->Clear();
		canvas_align_10files->cd();
		std::stringstream ss_text_align10;
		ss_text_align10 << "Alignment Ten Files Board " << board_nr << " (" << current_time.str() << ")";
		hist_align_10files.at(board_nr)->SetTitle(ss_text_align10.str().c_str());
		hist_align_10files.at(board_nr)->SetStats(0);
		hist_align_10files.at(board_nr)->Draw("");
		std::stringstream ss_path_align10;
		ss_path_align10 << outpath << "LAPPD_Time_Alignment_Ten_Files_Board" << board_nr << "_current." << img_extension;
		canvas_align_10files->SaveAs(ss_path_align10.str().c_str());

		canvas_align_20files->Clear();
		canvas_align_20files->cd();
		std::stringstream ss_text_align20;
		ss_text_align20 << "Alignment Twenty Files Board " << board_nr << " (" << current_time.str() << ")";
		hist_align_20files.at(board_nr)->SetTitle(ss_text_align20.str().c_str());
		hist_align_20files.at(board_nr)->SetStats(0);
		hist_align_20files.at(board_nr)->Draw("");
		std::stringstream ss_path_align20;
		ss_path_align20 << outpath << "LAPPD_Time_Alignment_Twenty_Files_Board" << board_nr << "_current." << img_extension;
		canvas_align_20files->SaveAs(ss_path_align20.str().c_str());

		canvas_align_100files->Clear();
		canvas_align_100files->cd();
		std::stringstream ss_text_align100;
		ss_text_align100 << "Alignment Hundred Files Board " << board_nr << " (" << current_time.str() << ")";
		hist_align_100files.at(board_nr)->SetTitle(ss_text_align100.str().c_str());
		hist_align_100files.at(board_nr)->SetStats(0);
		hist_align_100files.at(board_nr)->Draw("");
		std::stringstream ss_path_align100;
		ss_path_align100 << outpath << "LAPPD_Time_Alignment_Hundred_Files_Board" << board_nr << "_current." << img_extension;
		canvas_align_100files->SaveAs(ss_path_align100.str().c_str());

	}
}

void MonitorLAPPDData::DrawTimeEvolutionLAPPDData(ULong64_t timestamp_end, double time_frame, std::string file_ending) {

	Log("MonitorLAPPDData: DrawTimeEvolutionLAPPDData", v_message, verbosity);

	//--------------------------------------------------------
	//-------------DrawTimeEvolutionLAPPDData ----------------
	// -------------------------------------------------------

	boost::posix_time::ptime endtime = *Epoch + boost::posix_time::time_duration(int(timestamp_end / MSEC_to_SEC / SEC_to_MIN / MIN_to_HOUR), int(timestamp_end / MSEC_to_SEC / SEC_to_MIN) % 60, int(timestamp_end / MSEC_to_SEC / 1000.) % 60, timestamp_end % 1000);
	struct tm endtime_tm = boost::posix_time::to_tm(endtime);
	std::stringstream end_time;
	end_time << endtime_tm.tm_year + 1900 << "/" << endtime_tm.tm_mon + 1 << "/" << endtime_tm.tm_mday << "-" << endtime_tm.tm_hour << ":" << endtime_tm.tm_min << ":" << endtime_tm.tm_sec;

	if (int(time_frame * 60 * 60 * 1000) > timestamp_end)
		time_frame = 0.99 * (timestamp_end / 60 / 60 / 1000.);

	if (timestamp_end != readfromfile_tend || time_frame != readfromfile_timeframe)
		ReadFromFile(timestamp_end, time_frame);

	//looping over all files that are in the time interval, each file will be one data point

	std::stringstream ss_timeframe;
	ss_timeframe << round(time_frame * 100.) / 100.;

	for (int i_board = 0; i_board < (int) board_configuration.size(); i_board++) {

		int board_nr = board_configuration.at(i_board);

		graph_pps_rate.at(board_nr)->Set(0);
		graph_frame_rate.at(board_nr)->Set(0);
		graph_buffer_size.at(board_nr)->Set(0);
		graph_int_charge.at(board_nr)->Set(0);

		for (unsigned int i_file = 0; i_file < pps_rate_plot.at(board_nr).size(); i_file++) {

			Log("MonitorLAPPDData: Stored data (file #" + std::to_string(i_file + 1) + "): ", v_message, verbosity);
			graph_pps_rate.at(board_nr)->SetPoint(i_file, labels_timeaxis.at(board_nr)[i_file].Convert(), pps_rate_plot.at(board_nr).at(i_file));
			graph_frame_rate.at(board_nr)->SetPoint(i_file, labels_timeaxis.at(board_nr)[i_file].Convert(), frame_rate_plot.at(board_nr).at(i_file));
			graph_buffer_size.at(board_nr)->SetPoint(i_file, labels_timeaxis.at(board_nr)[i_file].Convert(), buffer_size_plot.at(board_nr).at(i_file));
			graph_int_charge.at(board_nr)->SetPoint(i_file, labels_timeaxis.at(board_nr)[i_file].Convert(), int_charge_plot.at(board_nr).at(i_file));

		}

		// Drawing time evolution plots

		double max_canvas = 0;
		double min_canvas = 9999999.;

		std::stringstream ss_pps;
		ss_pps << "PPS rate time evolution Board " << board_nr << " (last " << ss_timeframe.str() << "h) " << end_time.str();
		canvas_pps_rate->cd();
		canvas_pps_rate->Clear();
		graph_pps_rate.at(board_nr)->SetTitle(ss_pps.str().c_str());
		graph_pps_rate.at(board_nr)->GetYaxis()->SetTitle("PPS rate [Hz]");
		graph_pps_rate.at(board_nr)->GetXaxis()->SetTimeDisplay(1);
		graph_pps_rate.at(board_nr)->GetXaxis()->SetLabelSize(0.03);
		graph_pps_rate.at(board_nr)->GetXaxis()->SetLabelOffset(0.03);
		graph_pps_rate.at(board_nr)->GetXaxis()->SetTimeFormat("#splitline{%m/%d}{%H:%M}");
		graph_pps_rate.at(board_nr)->GetXaxis()->SetTimeOffset(0.);
		graph_pps_rate.at(board_nr)->Draw("apl");
		double max_pps = 1.;
		if (pps_rate_plot.at(board_nr).size() > 0)
			max_pps = TMath::MaxElement(pps_rate_plot.at(board_nr).size(), graph_pps_rate.at(board_nr)->GetY());
		graph_pps_rate.at(board_nr)->GetYaxis()->SetRangeUser(0.001, 1.1 * max_pps);
		std::stringstream ss_pps_path;
		ss_pps_path << outpath << "LAPPDData_TimeEvolution_PPSRate_Board" << board_nr << "_" << file_ending << "." << img_extension;
		canvas_pps_rate->SaveAs(ss_pps_path.str().c_str());

		std::stringstream ss_frame;
		ss_frame << "Frame rate time evolution Board " << board_nr << " (last " << ss_timeframe.str() << "h) " << end_time.str();
		canvas_frame_rate->cd();
		canvas_frame_rate->Clear();
		graph_frame_rate.at(board_nr)->SetTitle(ss_frame.str().c_str());
		graph_frame_rate.at(board_nr)->GetYaxis()->SetTitle("Frame rate [Hz]");
		graph_frame_rate.at(board_nr)->GetXaxis()->SetTimeDisplay(1);
		graph_frame_rate.at(board_nr)->GetXaxis()->SetLabelSize(0.03);
		graph_frame_rate.at(board_nr)->GetXaxis()->SetLabelOffset(0.03);
		graph_frame_rate.at(board_nr)->GetXaxis()->SetTimeFormat("#splitline{%m/%d}{%H:%M}");
		graph_frame_rate.at(board_nr)->GetXaxis()->SetTimeOffset(0.);
		graph_frame_rate.at(board_nr)->Draw("apl");
		double max_frame = 1.;
		if (frame_rate_plot.at(board_nr).size() > 0)
			max_frame = TMath::MaxElement(frame_rate_plot.at(board_nr).size(), graph_frame_rate.at(board_nr)->GetY());
		graph_frame_rate.at(board_nr)->GetYaxis()->SetRangeUser(0.001, 1.1 * max_frame);
		std::stringstream ss_frame_path;
		ss_frame_path << outpath << "LAPPDData_TimeEvolution_FrameRate_Board" << board_nr << "_" << file_ending << "." << img_extension;
		canvas_frame_rate->SaveAs(ss_frame_path.str().c_str());

		std::stringstream ss_charge;
		ss_charge << "Integrated charge time evolution Board " << board_nr << " (last " << ss_timeframe.str() << "h) " << end_time.str();
		canvas_int_charge->cd();
		canvas_int_charge->Clear();
		graph_int_charge.at(board_nr)->SetTitle(ss_charge.str().c_str());
		graph_int_charge.at(board_nr)->GetYaxis()->SetTitle("Integrated charge");
		graph_int_charge.at(board_nr)->GetXaxis()->SetTimeDisplay(1);
		graph_int_charge.at(board_nr)->GetXaxis()->SetLabelSize(0.03);
		graph_int_charge.at(board_nr)->GetXaxis()->SetLabelOffset(0.03);
		graph_int_charge.at(board_nr)->GetXaxis()->SetTimeFormat("#splitline{%m/%d}{%H:%M}");
		graph_int_charge.at(board_nr)->GetXaxis()->SetTimeOffset(0.);
		graph_int_charge.at(board_nr)->Draw("apl");
		double max_charge = 1.;
		if (int_charge_plot.at(board_nr).size() > 0)
			max_charge = TMath::MaxElement(int_charge_plot.at(board_nr).size(), graph_int_charge.at(board_nr)->GetY());
		graph_int_charge.at(board_nr)->GetYaxis()->SetRangeUser(0.001, 1.1 * max_charge);
		std::stringstream ss_charge_path;
		ss_charge_path << outpath << "LAPPDData_TimeEvolution_IntCharge_Board" << board_nr << "_" << file_ending << "." << img_extension;
		canvas_int_charge->SaveAs(ss_charge_path.str().c_str());

		std::stringstream ss_buffer;
		ss_buffer << "Buffer size time evolution Board " << board_nr << " (last " << ss_timeframe.str() << "h) " << end_time.str();
		canvas_buffer_size->cd();
		canvas_buffer_size->Clear();
		graph_buffer_size.at(board_nr)->SetTitle(ss_buffer.str().c_str());
		graph_buffer_size.at(board_nr)->GetYaxis()->SetTitle("Buffer size");
		graph_buffer_size.at(board_nr)->GetXaxis()->SetTimeDisplay(1);
		graph_buffer_size.at(board_nr)->GetXaxis()->SetLabelSize(0.03);
		graph_buffer_size.at(board_nr)->GetXaxis()->SetLabelOffset(0.03);
		graph_buffer_size.at(board_nr)->GetXaxis()->SetTimeFormat("#splitline{%m/%d}{%H:%M}");
		graph_buffer_size.at(board_nr)->GetXaxis()->SetTimeOffset(0.);
		graph_buffer_size.at(board_nr)->Draw("apl");
		double max_buffer = 1.;
		if (buffer_size_plot.at(board_nr).size() > 0)
			max_buffer = TMath::MaxElement(buffer_size_plot.at(board_nr).size(), graph_buffer_size.at(board_nr)->GetY());
		graph_buffer_size.at(board_nr)->GetYaxis()->SetRangeUser(0.001, 1.1 * max_buffer);
		std::stringstream ss_buffer_path;
		ss_buffer_path << outpath << "LAPPDData_TimeEvolution_BufferSize_Board" << board_nr << "_" << file_ending << "." << img_extension;
		canvas_buffer_size->SaveAs(ss_buffer_path.str().c_str());
	}
}

void MonitorLAPPDData::ProcessLAPPDData() {

	//--------------------------------------------------------
	//-------------ProcessLAPPDData --------------------------
	//--------------------------------------------------------

	//TODO: Add code to handle PPS Frames once they are available in the data file
	//TODO: Extract pedestal values from 2D ADC Value histogram and also save values in a vector

	for (int i_board = 0; i_board < (int) board_configuration.size(); i_board++) {
		int board_nr = board_configuration.at(i_board);

		hist_adc_channel.at(board_nr)->Clear();
		hist_waveform_channel.at(board_nr)->Clear();
		hist_buffer_channel.at(board_nr)->Clear();
		hist_buffer.at(board_nr)->Clear();
		hist_waveform_voltages.at(board_nr)->Clear();

		current_pps_rate.at(i_board) = 0.0;
		current_frame_rate.at(i_board) = 0.0;
		current_beamgate_rate.at(i_board) = 0.0;
		current_int_charge.at(i_board) = 0.0;
		current_buffer_size.at(i_board) = 0.0;
		current_num_entries.at(i_board) = 0;
		n_buffer.at(i_board) = 0;

		first_beamgate_timestamp.at(i_board) = 0;
		last_beamgate_timestamp.at(i_board) = 0;
		first_timestamp.at(i_board) = 0;
		last_timestamp.at(i_board) = 0;
		first_entry.at(i_board) = true;

		for (size_t i_channel = 0; i_channel < hist_pedestal.at(board_nr).size(); i_channel++) {
			hist_pedestal.at(board_nr).at(i_channel)->Clear();
		}

		//ToDo: entries not equal to events
		long entries;
		LAPPDData->Header->Get("TotalEntries", entries);

		for (int i_entry = 0; i_entry < (int) entries; i_entry++) {
			for (size_t i_channel = 0; i_channel < hist_waveforms_onedim.at(i_entry).at(board_nr).size(); i_channel++) {
				hist_waveforms_onedim.at(i_entry).at(board_nr).at(i_channel)->Clear();
			}
		}
	}

	beamgate_timestamp.clear();
	data_timestamp.clear();
	num_channels.clear();
	average_buffer.clear();
	data_beamgate_lastfile.clear();
	board_index.clear();

	long entries;
	LAPPDData->Header->Get("TotalEntries", entries);

	for (int i_entry = 0; i_entry < (int) entries; i_entry++) {
		LAPPDData->GetEntry(i_entry);
		std::map<unsigned long, std::vector < Waveform<double>> > RawLAPPDData;
		std::vector<unsigned short> Metadata;
		std::vector<unsigned short> AccInfoFrame;
		LAPPDData->Get("RawLAPPDData", RawLAPPDData);
		LAPPDData->Get("Metadata", Metadata);
		LAPPDData->Get("AccInfoFrame", AccInfoFrame);
		std::cout << "******************************" << std::endl;
		std::cout << "Entry: " << i_entry << ", loop through LAPPD Raw data" << std::endl;

		//Check if we have a Board Index entry
		//The entry after the Board Index will always be the startword of chip 0 (0xCA00, or 51712)
		//Create "offset" variable based on position of the Board Index
		unsigned short metadata_0 = Metadata.at(0);
		unsigned short metadata_1 = Metadata.at(1);
		int offset = 0;
		int board_idx = -1;
		if (metadata_0 == 51712)
			offset = 1;
		else if (metadata_1 == 51712) {
			offset = 0;
			board_idx = metadata_0;
		}

		if (board_idx != -1) {
			std::cout << "Metadata Board #: " << board_idx << std::endl;
			std::bitset < 16 > bits_metadata(board_idx);
			std::cout << "Metadata Bits: " << bits_metadata << std::endl;
		} else {
			Log("MonitorLAPPDData: ERROR!!! Found no board index in the data!", v_error, verbosity);
		}

		board_index.push_back(board_idx);

		//Look for board index in the board_configuration vector
		int vector_idx = -1;
		std::vector<int>::iterator it = std::find(board_configuration.begin(), board_configuration.end(), board_idx);
		if (it != board_configuration.end()) {
			vector_idx = std::distance(board_configuration.begin(), it);
			std::cout << "Found board index " << board_idx << " at position " << vector_idx << " inside of the vector" << std::endl;
		} else {
			Log("MonitorLAPPDData: ERROR!!! Board index " + std::to_string(board_idx) + " was not found as one of the configured board index options!!! Abort LAPPD data entry", v_error, verbosity);
			continue;
		}

		//Build beamgate timestamp
		unsigned short beamgate_63_48 = Metadata.at(7 - offset);	//Shift everything by 1 for the test file
		unsigned short beamgate_47_32 = Metadata.at(27 - offset);	//Shift everything by 1 for the test file
		unsigned short beamgate_31_16 = Metadata.at(47 - offset);	//Shift everything by 1 for the test file
		unsigned short beamgate_15_0 = Metadata.at(67 - offset);	//Shift everything by 1 for the test file
		std::bitset < 16 > bits_beamgate_63_48(beamgate_63_48);
		std::bitset < 16 > bits_beamgate_47_32(beamgate_47_32);
		std::bitset < 16 > bits_beamgate_31_16(beamgate_31_16);
		std::bitset < 16 > bits_beamgate_15_0(beamgate_15_0);
		std::cout << "bits_beamgate_63_48: " << bits_beamgate_63_48 << std::endl;
		std::cout << "bits_beamgate_47_32: " << bits_beamgate_47_32 << std::endl;
		std::cout << "bits_beamgate_31_16: " << bits_beamgate_31_16 << std::endl;
		std::cout << "bits_beamgate_15_0: " << bits_beamgate_15_0 << std::endl;
		unsigned long beamgate_63_0 = (static_cast<unsigned long>(beamgate_63_48) << 48) + (static_cast<unsigned long>(beamgate_47_32) << 32) + (static_cast<unsigned long>(beamgate_31_16) << 16) + (static_cast<unsigned long>(beamgate_15_0));
		std::cout << "beamgate combined: " << beamgate_63_0 << std::endl;
		std::bitset < 64 > bits_beamgate_63_0(beamgate_63_0);
		std::cout << "bits_beamgate_63_0: " << bits_beamgate_63_0 << std::endl;

		//Build data timestamp
		unsigned short timestamp_63_48 = Metadata.at(70 - offset);	//Shift everything by 1 for the test file
		unsigned short timestamp_47_32 = Metadata.at(50 - offset);	//Shift everything by 1 for the test file
		unsigned short timestamp_31_16 = Metadata.at(30 - offset);	//Shift everything by 1 for the test file
		unsigned short timestamp_15_0 = Metadata.at(10 - offset);		//Shift everything by 1 for the test file
		std::bitset < 16 > bits_timestamp_63_48(timestamp_63_48);
		std::bitset < 16 > bits_timestamp_47_32(timestamp_47_32);
		std::bitset < 16 > bits_timestamp_31_16(timestamp_31_16);
		std::bitset < 16 > bits_timestamp_15_0(timestamp_15_0);
		std::cout << "bits_timestamp_63_48: " << bits_timestamp_63_48 << std::endl;
		std::cout << "bits_timestamp_47_32: " << bits_timestamp_47_32 << std::endl;
		std::cout << "bits_timestamp_31_16: " << bits_timestamp_31_16 << std::endl;
		std::cout << "bits_timestamp_15_0: " << bits_timestamp_15_0 << std::endl;
		unsigned long timestamp_63_0 = (static_cast<unsigned long>(timestamp_63_48) << 48) + (static_cast<unsigned long>(timestamp_47_32) << 32) + (static_cast<unsigned long>(timestamp_31_16) << 16) + (static_cast<unsigned long>(timestamp_15_0));
		std::cout << "timestamp combined: " << timestamp_63_0 << std::endl;
		std::bitset < 64 > bits_timestamp_63_0(timestamp_63_0);
		std::cout << "bits_timestamp_63_0: " << bits_timestamp_63_0 << std::endl;
		beamgate_timestamp.push_back(beamgate_63_0);
		data_timestamp.push_back(timestamp_63_0);
		data_beamgate_lastfile.push_back(timestamp_63_0 - beamgate_63_0);

		if (first_entry.at(vector_idx) == true) {
			first_beamgate_timestamp.at(vector_idx) = beamgate_63_0;
			first_timestamp.at(vector_idx) = timestamp_63_0;
			first_entry.at(vector_idx) = false;
		}
		//Just overwrite "last" entry every time
		last_beamgate_timestamp.at(vector_idx) = beamgate_63_0;
		last_timestamp.at(vector_idx) = timestamp_63_0;

		//Loop through LAPPD data
		int n_channels = 0;
		double buffer_size = 0;
		for (std::map<unsigned long, std::vector<Waveform<double>>>::iterator it = RawLAPPDData.begin(); it != RawLAPPDData.end(); it++) {
			//std::cout <<"Got channel "<<it->first<<std::endl;
			n_channels++;
			std::vector<Waveform<double>> waveforms = it->second;
			for (int i_vec = 0; i_vec < (int) waveforms.size(); i_vec++) {
//			std::cout <<"Waveform # "<<i_vec<<" contains ADC values: "<<std::endl;
//        	waveforms.at(i_vec).Print();
				std::vector<double> waveform = *(waveforms.at(i_vec).GetSamples());
				//std::cout <<"waveform size: "<<waveform.size()<<std::endl;
				hist_buffer_channel.at(board_idx)->Fill(waveform.size(), it->first);
				hist_buffer.at(board_idx)->Fill(waveform.size());
				current_buffer_size.at(vector_idx) += waveform.size();
				buffer_size += waveform.size();
				n_buffer.at(vector_idx)++;
				for(int i_wave=0; i_wave < (int) waveform.size(); i_wave++) {
					hist_adc_channel.at(board_idx)->Fill(waveform.at(i_wave),it->first);
					hist_waveform_channel.at(board_idx)->SetBinContent(i_wave+1,it->first+1,hist_waveform_channel.at(board_idx)->GetBinContent(i_wave+1,it->first+1)+waveform.at(i_wave));
					hist_waveform_voltages.at(board_idx)->Fill(i_wave, it->first, waveform.at(i_wave));
					hist_waveforms_onedim.at(i_entry).at(board_idx).at(it->first)->Fill(i_wave,waveform.at(i_wave));
					hist_pedestal.at(board_idx).at(it->first)->Fill(waveform.at(i_wave));
				}
				//std::cout << std::endl;
			}
		}
		std::cout << "*******************************" << std::endl;
		num_channels.push_back(n_channels);
		if (n_channels != 0)
			buffer_size /= n_channels;
		average_buffer.push_back(buffer_size);
	}		//end entry loop

	ModifyBeamgateData(5, data_beamgate_last5files);
	ModifyBeamgateData(10, data_beamgate_last10files);
	ModifyBeamgateData(20, data_beamgate_last20files);
	ModifyBeamgateData(100, data_beamgate_last100files);

	bool got_tfile_start = false;
	for (int i_board = 0; i_board < (int) board_configuration.size(); i_board++) {
		int board_nr = board_configuration.at(i_board);
		if (entries > 0)
			hist_waveform_channel.at(board_nr)->Scale(1. / entries);
		double diff_timestamps = (last_timestamp.at(i_board) - first_timestamp.at(i_board)) * CLOCK_to_SEC;
		double diff_timestamps_beam = (last_beamgate_timestamp.at(i_board) - first_beamgate_timestamp.at(i_board)) * CLOCK_to_SEC;
		if (diff_timestamps > 0.)
			current_frame_rate.at(i_board) = n_buffer.at(i_board) / diff_timestamps; // Need to convert difference of timestamps into seconds or something similar
		if (diff_timestamps_beam > 0.)
			current_beamgate_rate.at(i_board) = n_buffer.at(i_board) / diff_timestamps_beam; // Need to convert difference of timestamps into seconds or something similar
		if (n_buffer.at(i_board) != 0)
			current_buffer_size.at(i_board) /= n_buffer.at(i_board);

		//Convert timestamps to msec
		//TODO: Need to add absolute time reference from PPS signal to get milliseconds since 1970/1/1
		first_timestamp.at(i_board) *= (CLOCK_to_SEC * 1000);
		last_timestamp.at(i_board) *= (CLOCK_to_SEC * 1000);
		first_beamgate_timestamp.at(i_board) *= (CLOCK_to_SEC * 1000);
		last_beamgate_timestamp.at(i_board) *= (CLOCK_to_SEC * 1000);

		if (!got_tfile_start && first_timestamp.at(i_board) > 0) {
			t_file_start = first_timestamp.at(i_board);
			got_tfile_start = true;
		}
	}
}

void MonitorLAPPDData::ModifyBeamgateData(size_t numberOfFiles, std::vector<std::vector<uint64_t>> &dataVector) {
	if (dataVector.size()) {
		if (dataVector.size() == numberOfFiles) {
			for (size_t i_file = 0; i_file < dataVector.size() - 1; i_file++) {
				dataVector.at(i_file) = dataVector.at(i_file + 1);
			}
			dataVector.at(numberOfFiles - 1) = data_beamgate_lastfile;
		} else {
			dataVector.push_back(data_beamgate_lastfile);
		}
	} else {
		dataVector.push_back(data_beamgate_lastfile);
	}
}
