#include "MonitorLAPPDData.h"

MonitorLAPPDData::MonitorLAPPDData() : Tool()
{
}

bool MonitorLAPPDData::Initialise(std::string configfile, DataModel &data)
{

	/////////////////// Useful header ///////////////////////
	if (configfile != "")
		m_variables.Initialise(configfile); // loading config file
	// m_variables.Print();

	m_data = &data; // assigning transient data pointer
	/////////////////////////////////////////////////////////////////

	// gObjectTable only for debugging memory leaks, otherwise comment out
	// std::cout <<"MonitorLAPPDData: List of Objects (beginning of Initialise): "<<std::endl;
	// gObjectTable->Print();

	//-------------------------------------------------------
	//-----------------Get Configuration---------------------
	//-------------------------------------------------------

	update_frequency = 0.;
	threshold_pulse = 5.;
	sync_reference_time = false;

	std::string acdc_configuration;

	m_variables.Get("OutputPath", outpath_temp);
	m_variables.Get("StartTime", StartTime);
	m_variables.Get("ReferenceDate", referenceDate);
	m_variables.Get("ReferenceTime", referenceTime);
	m_variables.Get("UpdateFrequency", update_frequency);
	m_variables.Get("PathMonitoring", path_monitoring);
	m_variables.Get("PlotConfiguration", plot_configuration);
	m_variables.Get("ACDCBoardConfiguration", acdc_configuration);
	m_variables.Get("ImageFormat", img_extension);
	m_variables.Get("ForceUpdate", force_update);
	m_variables.Get("DrawMarker", draw_marker);
	m_variables.Get("ThresholdPulse", threshold_pulse);
	m_variables.Get("verbose", verbosity);

	if (verbosity > 1)
		std::cout << "Tool MonitorLAPPDData: Initialising...." << std::endl;
	// Update frequency specifies the frequency at which the File Log Histogram is updated
	// All other monitor plots are updated as soon as a new file is available for readout
	if (update_frequency < 0.1)
	{
		if (verbosity > 0)
			std::cout << "MonitorLAPPDData: Update Frequency of " << update_frequency << " mins is too low. Setting default value of 5 min." << std::endl;
		update_frequency = 5.;
	}

	// default should be no forced update of the monitoring plots every execute step
	if (force_update != 0 && force_update != 1)
	{
		force_update = 0;
	}

	// check if the image format is jpg or png
	if (!(img_extension == "png" || img_extension == "jpg" || img_extension == "jpeg"))
	{
		img_extension = "jpg";
	}

	// Print out path to monitoring files
	if (verbosity > 2)
		std::cout << "MonitorLAPPDData: PathMonitoring: " << path_monitoring << std::endl;

	// Set up Epoch
	Epoch = new boost::posix_time::ptime(boost::gregorian::from_string(StartTime));

	referenceTimeDate = referenceDate + " " + referenceTime;
	boost::posix_time::ptime reference = boost::posix_time::time_from_string(referenceTimeDate);

	// Set up reference time
	boost::posix_time::time_duration reference_stamp = boost::posix_time::time_duration(reference - *Epoch);
	reference_time = reference_stamp.total_milliseconds();
	if (verbosity > 1)
		std::cout << "ReferenceTime: " << referenceTimeDate << ", total milliseconds: " << reference_time << std::endl;

	// Evaluating output path for monitoring plots
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

bool MonitorLAPPDData::Execute()
{

	if (verbosity > 10)
		std::cout << "MonitorLAPPDData: Executing ...." << std::endl;

	current = (boost::posix_time::second_clock::local_time());
	duration = boost::posix_time::time_duration(current - last);
	current_stamp_duration = boost::posix_time::time_duration(current - *Epoch);
	current_stamp = current_stamp_duration.total_milliseconds();
	utc = (boost::posix_time::second_clock::universal_time());
	current_utc_duration = boost::posix_time::time_duration(utc - current);
	current_utc = current_utc_duration.total_milliseconds();
	utc_to_t = (ULong64_t)current_utc;
	t_current = (ULong64_t)current_stamp;

	//------------------------------------------------------------
	//---------Checking the state of LAPPD SC data stream---------
	//------------------------------------------------------------

	bool has_lappd;
	m_data->CStore.Get("HasLAPPDData", has_lappd);

	std::string State;
	m_data->CStore.Get("State", State);

	if (has_lappd)
	{
		if (State == "Wait" || State == "LAPPDSC")
		{
			if (verbosity > 2)
				std::cout << "MonitorLAPPDData: State is " << State << std::endl;
		}
		else if (State == "DataFile")
		{
			if (verbosity > 1)
				std::cout << "MonitorLAPPDData: New PSEC data available." << std::endl;

			// TODO
			// TODO: Need to confirm data format of LAPPDData, is it PsecData, or a vec of PsecData, or map<timestamp,PsecData>?
			// TODO

			// m_data->Stores["LAPPDData"]->Get("LAPPDData", LAPPDData);

			// Process data
			// TODO: Adapt when data type is understood
			this->ProcessLAPPDData();

			// Write the event information to a file
			// TODO: change this to a database later on!
			// Check if data has already been written included in WriteToFile function
			this->WriteToFile();

			// Plot plots only associated to current file
			this->DrawLastFilePlots();

			// Draw customly defined plots
			this->UpdateMonitorPlotsLAPPD(config_timeframes, config_endtime_long, config_label, config_plottypes);

			// last = current;	//Why was this here in the first place?
		}
		else
		{
			if (verbosity > 1)
				std::cout << "MonitorLAPPDData: State not recognized: " << State << std::endl;
		}
	}

	// if force_update is specified, the plots will be updated no matter whether there has been a new file or not
	if (force_update)
		this->UpdateMonitorPlotsLAPPD(config_timeframes, config_endtime_long, config_label, config_plottypes);

	//-------------------------------------------------------
	//-----------Has enough time passed for update?----------
	//-------------------------------------------------------

	if (duration >= period_update)
	{

		Log("MonitorLAPPDData: " + std::to_string(update_frequency) + " mins passed... Updating file history plot.", v_message, verbosity);

		last = current;
		// TODO: Maybe implement the file history plots
		DrawFileHistoryLAPPD(current_stamp, 24., "current_24h", 1); // show 24h history of LAPPD files
		PrintFileTimeStampLAPPD(current_stamp, 24., "current_24h");
		DrawFileHistoryLAPPD(current_stamp, 2., "current_2h", 3);
	}

	// gObjectTable only for debugging memory leaks, otherwise comment out
	// std::cout <<"MonitorLAPPDData: List of Objects (after execute step): "<<std::endl;
	// gObjectTable->Print();

	return true;
}

bool MonitorLAPPDData::Finalise()
{

	if (verbosity > 1)
		std::cout << "Tool MonitorLAPPDData: Finalising ...." << std::endl;

	// gDirectory->ls();
	// gDirectory->pwd();

	// timing pointers
	delete Epoch;

	// canvas

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
	delete canvas_align_1000files;
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
	delete canvas_logfile_lappd;
	delete canvas_file_timestamp_lappd;
	delete canvas_events_per_channel;
	delete canvas_ped_lappd;
	delete canvas_sigma_lappd;
	delete canvas_rate_lappd;
	delete canvas_frame_count;
	delete canvas_pps_count;
	delete canvas_pps_event_counter;

	// histograms
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

	// graphs
	for (int i_board = 0; i_board < (int)board_configuration.size(); i_board++)
	{

		int board_nr = board_configuration.at(i_board);
		delete graph_pps_rate.at(board_nr);
		delete graph_frame_rate.at(board_nr);
		delete graph_buffer_size.at(board_nr);
		delete graph_int_charge.at(board_nr);

		if (board_nr != -1)
		{
			int board_min = board_channel.at(i_board);
			for (int i = 0; i < 30; i++)
			{
				int chkey = board_min + i;
				delete graph_ped.at(chkey);
				delete graph_sigma.at(chkey);
				delete graph_rate.at(chkey);
			}
		}
	}

	// multi-graphs
	delete multi_ped_lappd;
	delete multi_sigma_lappd;
	delete multi_rate_lappd;

	// legends
	delete leg_ped_lappd;
	delete leg_sigma_lappd;
	delete leg_rate_lappd;

	// text
	delete text_data_title;
	delete text_pps_rate;
	delete text_frame_rate;
	delete text_buffer_size;
	delete text_int_charge;
	delete text_pps_count;
	delete text_frame_count;

	return true;
}

void MonitorLAPPDData::InitializeHistsLAPPD()
{

	if (verbosity > 2)
		std::cout << "MonitorLAPPDData: InitializeHists" << std::endl;

	gROOT->cd();
	// Canvas
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
	canvas_align_1000files = new TCanvas("canvas_align_1000files", "Time alignment 1000 files", 900, 600);
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
	canvas_logfile_lappd = new TCanvas("canvas_logfile_lappd", "LAPPD File History", 900, 600);
	canvas_file_timestamp_lappd = new TCanvas("canvas_file_timestamp_lappd", "Timestamp Last File", 900, 600);
	canvas_events_per_channel = new TCanvas("canvas_events_per_channel", "LAPPD Events per Channel", 900, 600);
	canvas_ped_lappd = new TCanvas("canvas_ped_lappd", "LAPPD Pedestals", 900, 600);
	canvas_sigma_lappd = new TCanvas("canvas_sigma_lappd", "LAPPD Sigmas", 900, 600);
	canvas_rate_lappd = new TCanvas("canvas_rate_lappd", "LAPPD Rates", 900, 600);
	canvas_frame_count = new TCanvas("canvas_frame_count", "LAPPD Data Count", 900, 600);
	canvas_pps_count = new TCanvas("canvas_pps_count", "LAPPD PPS Count", 900, 600);
	canvas_pps_event_counter = new TCanvas("canvas_pps_event_counter", "LAPPD PPS Event Counter", 900, 600);
	canvas_pps_accumulated_number_vs_psec_timestamp = new TCanvas("canvas_pps_accumulated_number_vs_psec_timestamp", "LAPPD PPS Accumulated Number vs System Time", 900, 600); // PSec timestamp is also referred to as "System Time"
	canvas_pps_time_vs_accumulated_number = new TCanvas("canvas_pps_time_vs_accumulated_number", "LAPPD PPS Time vs Accumulated Number", 900, 600);
	// Histograms
	// ToDo: Not hardcode the number of channels here
	int numberOfChannels = 30;
	int numberOfCards = (int)board_configuration.size();

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

	for (int i_label_x = 0; i_label_x < numberOfChannels; i_label_x++)
	{
		std::stringstream ss_channel;
		ss_channel << i_label_x;
		std::string str_ch = "ch " + ss_channel.str();
		hist_pedestal_all->GetXaxis()->SetBinLabel(i_label_x + 1, str_ch.c_str());
		hist_pedestal_difference_all->GetXaxis()->SetBinLabel(i_label_x + 1, str_ch.c_str());
		hist_buffer_size_all->GetXaxis()->SetBinLabel(i_label_x + 1, str_ch.c_str());
		hist_rate_threshold_all->GetXaxis()->SetBinLabel(i_label_x + 1, str_ch.c_str());
		hist_events_per_channel->GetXaxis()->SetBinLabel(i_label_x + 1, str_ch.c_str());
	}
	for (int i_label_y = 0; i_label_y < numberOfCards; i_label_y++)
	{
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
	hist_pedestal_all->GetZaxis()->SetTitle("Mean Baseline [ADC count]");
	hist_pedestal_difference_all->GetZaxis()->SetTitle("Mean Baseline [ADC count]");
	hist_buffer_size_all->GetZaxis()->SetTitle("Buffer Size");
	hist_rate_threshold_all->GetZaxis()->SetTitle("Number of Samples > Mean Baseline");
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

	// TODO: Change this number
	// In principle, here we need the number of events in the file
	for (int i_event = 0; i_event < 100; i_event++)
	{
		std::map<int, std::vector<TH1F *>> tempMap;
		for (int i_board = 0; i_board < (int)board_configuration.size(); i_board++)
		{
			int board_nr = board_configuration.at(i_board);
			std::vector<TH1F *> hist_waveform_temp_vec;
			for (int i = 0; i < numberOfChannels; i++)
			{
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

	// Create separate histograms for all board numbers
	for (int i_board = 0; i_board < (int)board_configuration.size(); i_board++)
	{

		int board_nr = board_configuration.at(i_board);
		int min_board = board_channel.at(i_board);
		std::stringstream ss_align_1file, ss_align_5files, ss_align_10files, ss_align_20files, ss_align_100files, ss_align_1000files;
		std::stringstream ss_adc_channel, ss_waveform_channel, ss_buffer_channel, ss_buffer, ss_waveform_voltages;
		ss_align_1file << "hist_align_1file_board" << board_nr;
		ss_align_5files << "hist_align_5files_board" << board_nr;
		ss_align_10files << "hist_align_10files_board" << board_nr;
		ss_align_20files << "hist_align_20files_board" << board_nr;
		ss_align_100files << "hist_align_100files_board" << board_nr;
		ss_align_1000files << "hist_align_1000files_board" << board_nr;
		ss_adc_channel << "hist_adc_channel_board" << board_nr;
		ss_waveform_channel << "hist_waveform_channel_board" << board_nr;
		ss_buffer_channel << "hist_buffer_channel_board" << board_nr;
		ss_buffer << "hist_buffer_board" << board_nr;
		ss_waveform_voltages << "hist_waveform_voltages" << board_nr;

		TH1F *hist_align_1file_single = new TH1F(ss_align_1file.str().c_str(), ss_align_1file.str().c_str(), 100, 0, 20000);
		TH1F *hist_align_5files_single = new TH1F(ss_align_5files.str().c_str(), ss_align_5files.str().c_str(), 100, 0, 20000);
		TH1F *hist_align_10files_single = new TH1F(ss_align_10files.str().c_str(), ss_align_10files.str().c_str(), 100, 0, 20000);
		TH1F *hist_align_20files_single = new TH1F(ss_align_20files.str().c_str(), ss_align_20files.str().c_str(), 100, 0, 20000);
		TH1F *hist_align_100files_single = new TH1F(ss_align_100files.str().c_str(), ss_align_100files.str().c_str(), 100, 0, 20000);
		TH1F *hist_align_1000files_single = new TH1F(ss_align_1000files.str().c_str(), ss_align_1000files.str().c_str(), 100, 0, 20000);
		TH2F *hist_adc_channel_single = new TH2F(ss_adc_channel.str().c_str(), ss_adc_channel.str().c_str(), 200, -500, 0, 30, min_board, min_board + 30);
		TH2F *hist_waveform_channel_single = new TH2F(ss_waveform_channel.str().c_str(), ss_waveform_channel.str().c_str(), 256, 0, 256, 30, min_board, min_board + 30);
		TH2F *hist_buffer_channel_single = new TH2F(ss_buffer_channel.str().c_str(), ss_buffer_channel.str().c_str(), 50, 0, 2000, 30, min_board, min_board + 30);
		TH1F *hist_buffer_single = new TH1F(ss_buffer.str().c_str(), ss_buffer.str().c_str(), 50, 0, 2000);
		TH2F *hist_waveform_voltages_single = new TH2F(ss_waveform_voltages.str().c_str(), ss_waveform_voltages.str().c_str(), 256, 0, 256, 30, min_board, min_board + 30);

		// TODO: Title, XAxis, YAxis for timing histos
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

		hist_align_1000files_single->GetXaxis()->SetTitle("Time [ns]");
		hist_align_1000files_single->GetYaxis()->SetTitle("Entries");
		hist_align_1000files_single->SetStats(0);

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
		hist_align_1000files.emplace(board_nr, hist_align_1000files_single);
		hist_adc_channel.emplace(board_nr, hist_adc_channel_single);
		hist_waveform_channel.emplace(board_nr, hist_waveform_channel_single);
		hist_buffer_channel.emplace(board_nr, hist_buffer_channel_single);
		hist_buffer.emplace(board_nr, hist_buffer_single);
		hist_waveform_voltages.emplace(board_nr, hist_waveform_voltages_single);

		std::vector<TH1F *> hist_pedestal_temp_vec;

		for (int i = 0; i < numberOfChannels; i++)
		{
			std::stringstream ss_pedestal_text;
			ss_pedestal_text << "hist_pedestal" << board_nr << "channel" << i;
			// ToDo: change this values to more reasonable numbers
			TH1F *hist_pedestal_single = new TH1F(ss_pedestal_text.str().c_str(), ss_pedestal_text.str().c_str(), 200, -100, 0);
			hist_pedestal_single->GetXaxis()->SetTitle("ADC value");
			hist_pedestal_single->GetYaxis()->SetTitle("Entries");
			hist_pedestal_single->SetStats(0);
			hist_pedestal_temp_vec.push_back(hist_pedestal_single);
		}
		hist_pedestal.emplace(board_nr, hist_pedestal_temp_vec);
		hist_pedestal_temp_vec.clear();
	}

	num_history_lappd = 10;
	log_files_lappd = new TH1F("log_files_lappd", "LAPPD Files History", num_history_lappd, 0, num_history_lappd);
	log_files_lappd->GetXaxis()->SetTimeDisplay(1);
	log_files_lappd->GetXaxis()->SetLabelSize(0.03);
	log_files_lappd->GetXaxis()->SetLabelOffset(0.03);
	log_files_lappd->GetXaxis()->SetTimeFormat("#splitline{%m/%d}{%H:%M}");
	log_files_lappd->GetYaxis()->SetTickLength(0.);
	log_files_lappd->GetYaxis()->SetLabelOffset(999);
	log_files_lappd->SetStats(0);

	// Graphs
	// Make one of each graph type for each board
	for (int i_board = 0; i_board < (int)board_configuration.size(); i_board++)
	{

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

		if (draw_marker)
		{
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

		if (board_nr != -1)
		{

			int min_board = board_channel.at(i_board);
			for (int i = 0; i < 30; i++)
			{
				int chkey = min_board + i;
				std::stringstream ss_ped, ss_sigma, ss_rate;
				ss_ped << "graph_ped_chkey" << chkey;
				ss_sigma << "graph_sigma_chkey" << chkey;
				ss_rate << "graph_rate_chkey" << chkey;

				TGraph *graph_rate_single = new TGraph();
				TGraph *graph_ped_single = new TGraph();
				TGraph *graph_sigma_single = new TGraph();

				graph_rate_single->SetName(ss_rate.str().c_str());
				graph_ped_single->SetName(ss_ped.str().c_str());
				graph_sigma_single->SetName(ss_sigma.str().c_str());

				std::stringstream title_rate, title_ped, title_sigma;
				title_rate << "LAPPD Rate time evolution - Channel " << chkey;
				title_ped << "LAPPD Baseline time evolution - Channel " << chkey;
				title_sigma << "LAPPD Sigma time evolution - Channel " << chkey;

				graph_rate_single->SetTitle(title_rate.str().c_str());
				graph_ped_single->SetTitle(title_ped.str().c_str());
				graph_sigma_single->SetTitle(title_sigma.str().c_str());

				if (draw_marker)
				{
					graph_rate_single->SetMarkerStyle(20);
					graph_ped_single->SetMarkerStyle(20);
					graph_sigma_single->SetMarkerStyle(20);
				}

				graph_rate_single->SetMarkerColor((i % 5) + 1);
				graph_ped_single->SetMarkerColor((i % 5) + 1);
				graph_sigma_single->SetMarkerColor((i % 5) + 1);

				graph_rate_single->SetLineColor((i % 5) + 1);
				graph_ped_single->SetLineColor((i % 5) + 1);
				graph_sigma_single->SetLineColor((i % 5) + 1);

				graph_rate_single->SetLineWidth(2);
				graph_ped_single->SetLineWidth(2);
				graph_sigma_single->SetLineWidth(2);

				graph_rate_single->SetFillColor(0);
				graph_ped_single->SetFillColor(0);
				graph_sigma_single->SetFillColor(0);

				graph_rate_single->GetYaxis()->SetTitle("Rate [Hz]");
				graph_ped_single->GetYaxis()->SetTitle("Baseline");
				graph_sigma_single->GetYaxis()->SetTitle("Sigma");

				graph_rate_single->GetXaxis()->SetTimeDisplay(1);
				graph_ped_single->GetXaxis()->SetTimeDisplay(1);
				graph_sigma_single->GetXaxis()->SetTimeDisplay(1);

				graph_rate_single->GetXaxis()->SetLabelSize(0.03);
				graph_ped_single->GetXaxis()->SetLabelSize(0.03);
				graph_sigma_single->GetXaxis()->SetLabelSize(0.03);

				graph_rate_single->GetXaxis()->SetLabelOffset(0.03);
				graph_ped_single->GetXaxis()->SetLabelOffset(0.03);
				graph_sigma_single->GetXaxis()->SetLabelOffset(0.03);

				graph_rate_single->GetXaxis()->SetTimeFormat("#splitline(%m/%d}{%H:%M}");
				graph_ped_single->GetXaxis()->SetTimeFormat("#splitline(%m/%d}{%H:%M}");
				graph_sigma_single->GetXaxis()->SetTimeFormat("#splitline(%m/%d}{%H:%M}");

				graph_rate.emplace(chkey, graph_rate_single);
				graph_ped.emplace(chkey, graph_ped_single);
				graph_sigma.emplace(chkey, graph_sigma_single);
			}
		}
	}

	// Normal Graphs without board number
	graph_pps_count = new TGraph();
	graph_pps_event_counter = new TGraph();
	graph_pps_accumulated_number_vs_psec_timestamp = new TGraph();
	graph_pps_time_vs_accumulated_number = new TGraph();
	graph_frame_count = new TGraph();

	graph_pps_count->SetName("graph_pps_count");
	graph_pps_event_counter->SetName("graph_pps_event_counter");
	graph_pps_accumulated_number_vs_psec_timestamp->SetName("graph_pps_accumulated_number_vs_psec_timestamp");
	graph_pps_time_vs_accumulated_number->SetName("graph_pps_time_vs_accumulated_number");
	graph_frame_count->SetName("graph_frame_count");

	graph_pps_count->SetTitle("LAPPD PPS Events Time Evolution");
	graph_pps_event_counter->SetTitle("LAPPD PPS Event Counter Time Evolution");
	graph_pps_accumulated_number_vs_psec_timestamp->SetTitle("LAPPD PPS Accumulated Number vs System Time");
	graph_pps_time_vs_accumulated_number->SetTitle("LAPPD PPS Time vs Accumulated Number");
	graph_frame_count->SetTitle("LAPPD Data Events Time Evolution");

	if (draw_marker)
	{
		graph_pps_count->SetMarkerStyle(20);
		graph_frame_count->SetMarkerStyle(20);
	}

	graph_pps_count->SetMarkerColor(kBlack);
	graph_pps_event_counter->SetMarkerColor(kBlack);
	graph_pps_accumulated_number_vs_psec_timestamp->SetMarkerColor(kBlack);
	graph_pps_time_vs_accumulated_number->SetMarkerColor(kBlack);
	graph_frame_count->SetMarkerColor(kBlack);

	graph_pps_count->SetLineColor(kBlack);
	graph_pps_event_counter->SetLineColor(kBlack);
	graph_pps_accumulated_number_vs_psec_timestamp->SetLineColor(kBlack);
	graph_pps_time_vs_accumulated_number->SetLineColor(kBlack);
	graph_frame_count->SetLineColor(kBlack);

	graph_pps_count->SetLineWidth(2);
	graph_pps_event_counter->SetLineWidth(2);
	graph_pps_accumulated_number_vs_psec_timestamp->SetLineWidth(2);
	graph_pps_time_vs_accumulated_number->SetLineWidth(2);
	graph_frame_count->SetLineWidth(2);

	graph_pps_count->SetFillColor(0);
	graph_pps_event_counter->SetFillColor(0);
	graph_pps_accumulated_number_vs_psec_timestamp->SetFillColor(0);
	graph_pps_time_vs_accumulated_number->SetFillColor(0);
	graph_frame_count->SetFillColor(0);

	graph_pps_count->GetYaxis()->SetTitle("PPS Events");
	graph_pps_event_counter->GetYaxis()->SetTitle("PPS Event Counter");
	graph_pps_accumulated_number_vs_psec_timestamp->GetYaxis()->SetTitle("PPS Accumulated Number");
	graph_pps_time_vs_accumulated_number->GetYaxis()->SetTitle("PPS Accumulated Number");
	graph_frame_count->GetYaxis()->SetTitle("Data Events");

	graph_pps_count->GetXaxis()->SetTimeDisplay(1);
	graph_pps_event_counter->GetXaxis()->SetTimeDisplay(1);
	graph_pps_accumulated_number_vs_psec_timestamp->GetXaxis()->SetTimeDisplay(1);
	graph_pps_time_vs_accumulated_number->GetXaxis()->SetTimeDisplay(1);
	graph_frame_count->GetXaxis()->SetTimeDisplay(1);

	graph_pps_count->GetXaxis()->SetLabelSize(0.03);
	graph_pps_event_counter->GetXaxis()->SetLabelSize(0.03);
	graph_pps_accumulated_number_vs_psec_timestamp->GetXaxis()->SetLabelSize(0.03);
	graph_pps_time_vs_accumulated_number->GetXaxis()->SetLabelSize(0.03);
	graph_frame_count->GetXaxis()->SetLabelSize(0.03);

	graph_pps_count->GetXaxis()->SetLabelOffset(0.03);
	graph_pps_event_counter->GetXaxis()->SetLabelOffset(0.03);
	graph_pps_accumulated_number_vs_psec_timestamp->GetXaxis()->SetLabelOffset(0.03);
	graph_pps_time_vs_accumulated_number->GetXaxis()->SetLabelOffset(0.03);
	graph_frame_count->GetXaxis()->SetLabelOffset(0.03);

	graph_pps_count->GetXaxis()->SetTimeFormat("#splitline(%m/%d}{%H:%M}");
	graph_pps_event_counter->GetXaxis()->SetTimeFormat("#splitline(%m/%d}{%H:%M}");
	graph_pps_accumulated_number_vs_psec_timestamp->GetXaxis()->SetTimeFormat("#splitline(%m/%d}{%H:%M}");
	graph_pps_time_vs_accumulated_number->GetXaxis()->SetTimeFormat("#splitline(%m/%d}{%H:%M}");
	graph_frame_count->GetXaxis()->SetTimeFormat("#splitline(%m/%d}{%H:%M}");

	// Multi-Graphs
	multi_ped_lappd = new TMultiGraph();
	multi_sigma_lappd = new TMultiGraph();
	multi_rate_lappd = new TMultiGraph();

	// Legends
	leg_ped_lappd = new TLegend(0.7, 0.7, 0.88, 0.88);
	leg_sigma_lappd = new TLegend(0.7, 0.7, 0.88, 0.88);
	leg_rate_lappd = new TLegend(0.7, 0.7, 0.88, 0.88);
	leg_ped_lappd->SetLineColor(0);
	leg_sigma_lappd->SetLineColor(0);
	leg_rate_lappd->SetLineColor(0);

	// Text
	text_data_title = new TText();
	text_pps_rate = new TText();
	text_buffer_size = new TText();
	text_frame_rate = new TText();
	text_int_charge = new TText();
	text_pps_count = new TText();
	text_frame_count = new TText();

	text_data_title->SetNDC(1);
	text_pps_rate->SetNDC(1);
	text_buffer_size->SetNDC(1);
	text_frame_rate->SetNDC(1);
	text_int_charge->SetNDC(1);
	text_pps_count->SetNDC(1);
	text_frame_count->SetNDC(1);
}

void MonitorLAPPDData::ReadInConfiguration()
{

	//-------------------------------------------------------
	//----------------ReadInConfiguration -------------------
	//-------------------------------------------------------

	Log("MonitorLAPPDData::ReadInConfiguration", v_message, verbosity);

	ifstream file(plot_configuration.c_str());

	std::string line;
	if (file.is_open())
	{
		while (std::getline(file, line))
		{
			if (line.find("#") != std::string::npos)
				continue;
			std::vector<std::string> values;
			std::stringstream ss;
			ss.str(line);
			std::string item;
			while (std::getline(ss, item, '\t'))
			{
				values.push_back(item);
			}
			if (values.size() < 4)
			{
				if (verbosity > 0)
					std::cout << "ERROR (MonitorLAPPDData): ReadInConfiguration: Need at least 4 arguments in one line: TimeFrame - TimeEnd - FileLabel - PlotType1. Please look over the configuration file and adjust it accordingly." << std::endl;
				continue;
			}
			double double_value = std::stod(values.at(0));
			config_timeframes.push_back(double_value);
			config_endtime.push_back(values.at(1));
			config_label.push_back(values.at(2));
			std::vector<std::string> plottypes;
			for (unsigned int i = 3; i < values.size(); i++)
			{
				plottypes.push_back(values.at(i));
			}
			config_plottypes.push_back(plottypes);
		}
	}
	else
	{
		if (verbosity > 0)
			std::cout << "ERROR (MonitorLAPPDData): ReadInConfiguration: Could not open file " << plot_configuration << "! Check if path is valid..." << std::endl;
	}
	file.close();

	if (verbosity > 2)
	{
		std::cout << "---------------------------------------------------------------------" << std::endl;
		std::cout << "MonitorLAPPDData: ReadInConfiguration: Read in the following data into configuration variables: " << std::endl;
		for (unsigned int i_t = 0; i_t < config_timeframes.size(); i_t++)
		{
			std::cout << config_timeframes.at(i_t) << ", " << config_endtime.at(i_t) << ", " << config_label.at(i_t) << ", ";
			for (unsigned int i_plot = 0; i_plot < config_plottypes.at(i_t).size(); i_plot++)
			{
				std::cout << config_plottypes.at(i_t).at(i_plot) << ", ";
			}
			std::cout << std::endl;
		}
		std::cout << "-----------------------------------------------------------------------" << std::endl;
	}

	if (verbosity > 2)
		std::cout << "MonitorLAPPDData: ReadInConfiguration: Parsing dates: " << std::endl;
	for (unsigned int i_date = 0; i_date < config_endtime.size(); i_date++)
	{
		if (config_endtime.at(i_date) == "TEND_LASTFILE")
		{
			if (verbosity > 2)
				std::cout << "TEND_LASTFILE: Starting from end of last read-in file" << std::endl;
			ULong64_t zero = 0;
			config_endtime_long.push_back(zero);
		}
		else if (config_endtime.at(i_date).size() == 15)
		{
			boost::posix_time::ptime spec_endtime(boost::posix_time::from_iso_string(config_endtime.at(i_date)));
			boost::posix_time::time_duration spec_endtime_duration = boost::posix_time::time_duration(spec_endtime - *Epoch);
			ULong64_t spec_endtime_long = spec_endtime_duration.total_milliseconds();
			config_endtime_long.push_back(spec_endtime_long);
		}
		else
		{
			if (verbosity > 2)
				std::cout << "Specified end date " << config_endtime.at(i_date) << " does not have the desired format YYYYMMDDTHHMMSS. Please change the format in the config file in order to use this tool. Starting from end of last file" << std::endl;
			ULong64_t zero = 0;
			config_endtime_long.push_back(zero);
		}
	}
}

void MonitorLAPPDData::LoadACDCBoardConfig(std::string acdc_config)
{

	Log("MonitorLAPPDData: LoadACDCBoardConfig", v_message, verbosity);

	// Load the active ACDC board numbers specified in the configuration file
	ifstream acdc_file(acdc_config);
	int board_number;
	int min_bin;
	while (!acdc_file.eof())
	{
		acdc_file >> board_number >> min_bin;
		if (acdc_file.eof())
			break;
		Log("MonitorLAPPDData: Setting Board number >>>" + std::to_string(board_number) + "<<< as an active LAPPD ACDC board number", v_message, verbosity);
		board_configuration.push_back(board_number);
		board_channel.push_back(min_bin);

		current_pps_rate.push_back(0.0);
		current_frame_rate.push_back(0.0);
		current_beamgate_rate.push_back(0.0);
		current_int_charge.push_back(0.0);
		current_buffer_size.push_back(0.0);
		current_num_entries.push_back(0);
		n_buffer.push_back(0);
		n_data.push_back(0);
		n_pps.push_back(0);

		first_beamgate_timestamp.push_back(0);
		last_beamgate_timestamp.push_back(0);
		first_timestamp.push_back(0);
		last_timestamp.push_back(0);
		first_pps_timestamps.push_back(0);
		last_pps_timestamps.push_back(0);

		current_board_index.push_back(board_number);
		first_entry.push_back(true);
		first_entry_pps.push_back(true);

		std::vector<ULong64_t> empty_unsigned_long;
		std::vector<double> empty_double;
		std::vector<int> empty_int;
		std::vector<TDatime> empty_datime;
		std::vector<uint64_t> empty_int_64;
		std::vector<std::vector<uint64_t>> empty_int_64_vec;
		data_times_plot.emplace(board_number, empty_unsigned_long);
		data_times_end_plot.emplace(board_number, empty_unsigned_long);
		pps_rate_plot.emplace(board_number, empty_double);
		pps_event_counter_plot.emplace(board_number, empty_double);
		frame_rate_plot.emplace(board_number, empty_double);
		beamgate_rate_plot.emplace(board_number, empty_double);
		int_charge_plot.emplace(board_number, empty_double);
		buffer_size_plot.emplace(board_number, empty_double);
		num_channels_plot.emplace(board_number, empty_int);
		labels_timeaxis.emplace(board_number, empty_datime);
		data_beamgate_lastfile.emplace(board_number, empty_int_64);
		data_beamgate_last5files.emplace(board_number, empty_int_64_vec);
		data_beamgate_last10files.emplace(board_number, empty_int_64_vec);
		data_beamgate_last20files.emplace(board_number, empty_int_64_vec);
		data_beamgate_last100files.emplace(board_number, empty_int_64_vec);
		data_beamgate_last1000files.emplace(board_number, empty_int_64_vec);
		if (board_number != -1)
		{
			for (int i = 0; i < 30; i++)
			{
				ped_plot.emplace(min_bin + i, empty_double);
				sigma_plot.emplace(min_bin + i, empty_double);
				rate_plot.emplace(min_bin + i, empty_double);
			}
		}
	}
	acdc_file.close();
	current_run = 0;
	current_subrun = 0;
	current_partrun = 0;
	current_pps_count = 0;
	current_frame_count = 0;
}

std::string MonitorLAPPDData::convertTimeStamp_to_Date(ULong64_t timestamp)
{

	// format of date is YYYY_MM-DD

	boost::posix_time::ptime convertedtime = *Epoch + boost::posix_time::time_duration(int(timestamp / MSEC_to_SEC / SEC_to_MIN / MIN_to_HOUR), int(timestamp / MSEC_to_SEC / SEC_to_MIN) % 60, int(timestamp / MSEC_to_SEC / 1000.) % 60, timestamp % 1000);
	struct tm converted_tm = boost::posix_time::to_tm(convertedtime);
	std::stringstream ss_date;
	ss_date << converted_tm.tm_year + 1900 << "_" << converted_tm.tm_mon + 1 << "-" << converted_tm.tm_mday;
	return ss_date.str();
}

bool MonitorLAPPDData::does_file_exist(std::string filename)
{

	std::ifstream infile(filename.c_str());
	bool file_good = infile.good();
	infile.close();
	return file_good;
}

void MonitorLAPPDData::WriteToFile()
{

	Log("MonitorLAPPDData: WriteToFile", v_message, verbosity);

	//-------------------------------------------------------
	//------------------WriteToFile -------------------------
	//-------------------------------------------------------

	t_file_end.clear();
	t_file_end_global = 0;
	for (int i_board = 0; i_board < (int)board_configuration.size(); i_board++)
	{
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
	std::vector<double> *t_pps_event_counter = new std::vector<double>;
	std::vector<double> *t_frame_rate = new std::vector<double>;
	std::vector<double> *t_beamgate_rate = new std::vector<double>;
	std::vector<double> *t_int_charge = new std::vector<double>;
	std::vector<double> *t_buffer_size = new std::vector<double>;
	std::vector<int> *t_board_idx = new std::vector<int>;
	std::vector<int> *t_chkey = new std::vector<int>;
	std::vector<double> *t_rate = new std::vector<double>;
	std::vector<double> *t_ped = new std::vector<double>;
	std::vector<double> *t_sigma = new std::vector<double>;
	std::vector<int> *t_raw_lappd_data_pps_counts = new std::vector<int>;
	std::vector<long> *t_raw_lappd_data_pps_timestamps = new std::vector<long>;
	std::vector<int> *t_pps_accumulated_number = new std::vector<int>; // Accumulated PPS event number, taken from current_pps_count, 
	std::vector<long> *t_pps_accumulated_psec_timestamp = new std::vector<long>; 
	std::vector<long> *t_raw_lappd_data_pps_timestamp_per_accumulated_number = new std::vector<long>;
	
	int t_run, t_subrun, t_partrun;
	int t_pps_count, t_frame_count;
	ULong64_t t_lappd_offset;

	TTree *t;
	if (f->GetListOfKeys()->Contains("lappddatamonitor_tree"))
	{
		Log("MonitorLAPPDData: WriteToFile: Tree already exists", v_message, verbosity);
		t = (TTree *)f->Get("lappddatamonitor_tree");
		t->SetBranchAddress("t_start", &t_time);
		t->SetBranchAddress("t_end", &t_end);
		t->SetBranchAddress("pps_rate", &t_pps_rate);
		t->SetBranchAddress("frame_rate", &t_frame_rate);
		t->SetBranchAddress("beamgate_rate", &t_beamgate_rate);
		t->SetBranchAddress("int_charge", &t_int_charge);
		t->SetBranchAddress("buffer_size", &t_buffer_size);
		t->SetBranchAddress("board_idx", &t_board_idx);
		t->SetBranchAddress("chkey", &t_chkey);
		t->SetBranchAddress("rate", &t_rate);
		t->SetBranchAddress("ped", &t_ped);
		t->SetBranchAddress("sigma", &t_sigma);
		t->SetBranchAddress("run", &t_run);
		t->SetBranchAddress("subrun", &t_subrun);
		t->SetBranchAddress("partrun", &t_partrun);
		t->SetBranchAddress("pps_count", &t_pps_count);
		t->SetBranchAddress("frame_count", &t_frame_count);
		t->SetBranchAddress("lappd_offset", &t_lappd_offset);
		t->SetBranchAddress("raw_lappd_data_pps_counts", &t_raw_lappd_data_pps_counts);
		t->SetBranchAddress("raw_lappd_data_pps_timestamps", &t_raw_lappd_data_pps_timestamps);
		t->SetBranchAddress("pps_accumulated_number", &t_pps_accumulated_number);
		t->SetBranchAddress("pps_accumulated_psec_timestamp", &t_pps_accumulated_psec_timestamp);
		t->SetBranchAddress("raw_lappd_data_pps_timestamp_per_accumulated_number", &t_raw_lappd_data_pps_timestamp_per_accumulated_number);
	}
	else
	{
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
		t->Branch("chkey", &t_chkey);
		t->Branch("rate", &t_rate);
		t->Branch("ped", &t_ped);
		t->Branch("sigma", &t_sigma);
		t->Branch("run", &t_run);
		t->Branch("subrun", &t_subrun);
		t->Branch("partrun", &t_partrun);
		t->Branch("pps_count", &t_pps_count);
		t->Branch("frame_count", &t_frame_count);
		t->Branch("lappd_offset", &t_lappd_offset);
		t->Branch("raw_lappd_data_pps_counts", &t_raw_lappd_data_pps_counts);
		t->Branch("raw_lappd_data_pps_timestamps", &t_raw_lappd_data_pps_timestamps);
		t->Branch("pps_accumulated_number", &t_pps_accumulated_number);
		t->Branch("pps_accumulated_psec_timestamp", &t_pps_accumulated_psec_timestamp);
		t->Branch("raw_lappd_data_pps_timestamp_per_accumulated_number", &t_raw_lappd_data_pps_timestamp_per_accumulated_number);
	}

	int n_entries = t->GetEntries();
	bool omit_entries = false;
	for (int i_entry = 0; i_entry < n_entries; i_entry++)
	{
		t->GetEntry(i_entry);
		if (t_board_idx->at(0) == -1)
			continue;
		if (t_end->at(0) == t_file_end.at(0))
		{
			Log("WARNING (MonitorLAPPDData): WriteToFile: Wanted to write data from file that is already written to DB. Omit entries", v_warning, verbosity);
			omit_entries = true;
		}
	}

	// if data is already written to DB/File, do not write it again
	if (omit_entries)
	{
		// don't write file again, but still delete TFile and TTree object!!!
		f->Close();
		delete t_time;
		delete t_end;
		delete t_pps_rate;
		delete t_frame_rate;
		delete t_beamgate_rate;
		delete t_int_charge;
		delete t_buffer_size;
		delete t_board_idx;
		delete t_chkey;
		delete t_rate;
		delete t_ped;
		delete t_sigma;
		delete t_raw_lappd_data_pps_counts;
		delete t_raw_lappd_data_pps_timestamps;
		delete t_pps_accumulated_number;
		delete t_pps_accumulated_psec_timestamp;
		delete t_raw_lappd_data_pps_timestamp_per_accumulated_number;
		delete f;

		gROOT->cd();

		return;
	}

	// If we have vectors, they need to be cleared
	t_time->clear();
	t_end->clear();
	t_pps_rate->clear();
	t_frame_rate->clear();
	t_beamgate_rate->clear();
	t_int_charge->clear();
	t_buffer_size->clear();
	t_board_idx->clear();
	t_chkey->clear();
	t_rate->clear();
	t_ped->clear();
	t_sigma->clear();

	// Get data that was processed
	for (int i_current = 0; i_current < (int)current_pps_rate.size(); i_current++)
	{
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
			"MonitorLAPPDData: WriteToFile: Board " + std::to_string(current_board_index.at(i_current)) + ". Writing data to file: " + std::to_string(starttime_tm.tm_year + 1900) + "/" + std::to_string(starttime_tm.tm_mon + 1) + "/" + std::to_string(starttime_tm.tm_mday) + "-" + std::to_string(starttime_tm.tm_hour) + ":" + std::to_string(starttime_tm.tm_min) + ":" + std::to_string(starttime_tm.tm_sec), v_message, verbosity);
	}

	// Once every partfun file
	if (t_partrun < current_partrun) {
		// Push raw lappd data
		for (int i_current = 0; i_current < raw_lappd_data_pps_timestamps.size(); i_current++)
		{
			t_raw_lappd_data_pps_counts->push_back(raw_lappd_data_pps_counts.at(i_current));
			t_raw_lappd_data_pps_timestamps->push_back(raw_lappd_data_pps_timestamps.at(i_current));
		}
		
		// Push accumulated PPS event number and PSec timestamp
		int total_pps_count = current_pps_count;
		for (int i_current = 0; i_current < t_pps_accumulated_number->size(); i_current++) {
			total_pps_count += t_pps_accumulated_number->at(i_current);
		}
		t_pps_accumulated_number->push_back(total_pps_count);
		std::string psec_timestamp_string;
		// Get the PSecTimestamp given by ParseDataMonitoring tool
		m_data->CStore.Get("PSecTimestamp", psec_timestamp_string);
		// Cast PSec timestamp to long, then push it
		t_pps_accumulated_psec_timestamp->push_back(std::stol(psec_timestamp_string));

		// Push the latest LAPPD PPS timestamp, as we will use it to plot against accumulated PPS event number
		auto current_raw_lappd_pps_timestamp = raw_lappd_data_pps_timestamps.back();
		t_raw_lappd_data_pps_timestamp_per_accumulated_number->push_back(current_raw_lappd_pps_timestamp);
	}

	for (int i_current = 0; i_current < (int)current_ped.size(); i_current++)
	{
		t_chkey->push_back(current_chkey.at(i_current));
		t_rate->push_back(current_rate.at(i_current));
		t_ped->push_back(current_ped.at(i_current));
		t_sigma->push_back(current_sigma.at(i_current));
	}

	t_run = current_run;
	t_subrun = current_subrun;
	t_partrun = current_partrun;
	t_lappd_offset = reference_time;
	t_pps_count = current_pps_count;
	t_frame_count = current_frame_count;

	if (verbosity > 3)
		std::cout << "before fill" << std::endl;
	t->Fill();
	if (verbosity > 3)
		std::cout << "after fill" << std::endl;
	t->Write("", TObject::kOverwrite); // prevent ROOT from making endless keys for the same tree when updating the tree
	if (verbosity > 3)
		std::cout << "after write" << std::endl;
	f->Close();
	if (verbosity > 3)
		std::cout << "after close" << std::endl;

	if (verbosity > 3)
		std::cout << "t_time" << std::endl;
	if (verbosity > 3)
		std::cout << "t_time: " << t_time << std::endl;
	if (verbosity > 3)
		std::cout << "t_time->size(): " << t_time->size() << std::endl;

	// Delete potential vectors
	delete t_time;
	delete t_end;
	delete t_pps_rate;
	delete t_pps_event_counter;
	delete t_frame_rate;
	delete t_beamgate_rate;
	delete t_int_charge;
	delete t_buffer_size;
	delete t_board_idx;
	delete t_chkey;
	delete t_rate;
	delete t_ped;
	delete t_sigma;
	delete t_raw_lappd_data_pps_counts;
	delete t_raw_lappd_data_pps_timestamps;
	delete t_raw_lappd_data_pps_timestamp_per_accumulated_number;
	delete f;

	gROOT->cd();
}

void MonitorLAPPDData::ReadFromFile(ULong64_t timestamp, double time_frame)
{

	Log("MonitorLAPPDData: ReadFromFile", v_message, verbosity);

	//-------------------------------------------------------
	//------------------ReadFromFile ------------------------
	//-------------------------------------------------------

	for (int i_board = 0; i_board < (int)board_configuration.size(); i_board++)
	{
		int board_nr = board_configuration.at(i_board);
		data_times_plot.at(board_nr).clear();
		data_times_end_plot.at(board_nr).clear();
		pps_rate_plot.at(board_nr).clear();
		pps_event_counter_plot.at(board_nr).clear();
		frame_rate_plot.at(board_nr).clear();
		beamgate_rate_plot.at(board_nr).clear();
		int_charge_plot.at(board_nr).clear();
		buffer_size_plot.at(board_nr).clear();
		num_channels_plot.at(board_nr).clear();
		labels_timeaxis.at(board_nr).clear();
	}
	for (int i_ch = 0; i_ch < (int)ped_plot.size(); i_ch++)
	{
		ped_plot.at(i_ch).clear();
		sigma_plot.at(i_ch).clear();
		rate_plot.at(i_ch).clear();
	}
	run_plot.clear();
	subrun_plot.clear();
	partrun_plot.clear();
	lappdoffset_plot.clear();
	ppscount_plot.clear();
	framecount_plot.clear();
	raw_lappd_data_pps_counts.clear();
	raw_lappd_data_pps_timestamps.clear();
	pps_accumulated_number.clear();
	pps_accumulated_psec_timestamp.clear();

	// take the end time and calculate the start time with the given time_frame
	ULong64_t timestamp_start = timestamp - time_frame * MIN_to_HOUR * SEC_to_MIN * MSEC_to_SEC;
	boost::posix_time::ptime starttime = *Epoch + boost::posix_time::time_duration(int(timestamp_start / MSEC_to_SEC / SEC_to_MIN / MIN_to_HOUR), int(timestamp_start / MSEC_to_SEC / SEC_to_MIN) % 60, int(timestamp_start / MSEC_to_SEC / 1000.) % 60, timestamp_start % 1000);
	struct tm starttime_tm = boost::posix_time::to_tm(starttime);
	boost::posix_time::ptime endtime = *Epoch + boost::posix_time::time_duration(int(timestamp / MSEC_to_SEC / SEC_to_MIN / MIN_to_HOUR), int(timestamp / MSEC_to_SEC / SEC_to_MIN) % 60, int(timestamp / MSEC_to_SEC / 1000.) % 60, timestamp % 1000);
	struct tm endtime_tm = boost::posix_time::to_tm(endtime);

	Log(
		"MonitorLAPPDData: ReadFromFile: Reading in data for time frame " + std::to_string(starttime_tm.tm_year + 1900) + "/" + std::to_string(starttime_tm.tm_mon + 1) + "/" + std::to_string(starttime_tm.tm_mday) + "-" + std::to_string(starttime_tm.tm_hour) + ":" + std::to_string(starttime_tm.tm_min) + ":" + std::to_string(starttime_tm.tm_sec) + " ... " + std::to_string(endtime_tm.tm_year + 1900) + "/" + std::to_string(endtime_tm.tm_mon + 1) + "/" + std::to_string(endtime_tm.tm_mday) + "-" + std::to_string(endtime_tm.tm_hour) + ":" + std::to_string(endtime_tm.tm_min) + ":" + std::to_string(endtime_tm.tm_sec), v_message, verbosity);

	std::stringstream ss_startdate, ss_enddate;
	ss_startdate << starttime_tm.tm_year + 1900 << "-" << starttime_tm.tm_mon + 1 << "-" << starttime_tm.tm_mday;
	ss_enddate << endtime_tm.tm_year + 1900 << "-" << endtime_tm.tm_mon + 1 << "-" << endtime_tm.tm_mday;

	int number_of_days;
	if (endtime_tm.tm_mon == starttime_tm.tm_mon)
		number_of_days = endtime_tm.tm_mday - starttime_tm.tm_mday;
	else
	{
		boost::gregorian::date endtime_greg(boost::gregorian::from_simple_string(ss_enddate.str()));
		boost::gregorian::date starttime_greg(boost::gregorian::from_simple_string(ss_startdate.str()));
		boost::gregorian::days days_dataframe = endtime_greg - starttime_greg;
		number_of_days = days_dataframe.days();
	}

	for (int i_day = 0; i_day <= number_of_days; i_day++)
	{

		ULong64_t timestamp_i = timestamp_start + i_day * HOUR_to_DAY * MIN_to_HOUR * SEC_to_MIN * MSEC_to_SEC;
		std::string string_date_i = convertTimeStamp_to_Date(timestamp_i);
		std::stringstream root_filename_i;
		root_filename_i << path_monitoring << "LAPPDData_" << string_date_i << ".root";
		bool tree_exists = true;

		if (does_file_exist(root_filename_i.str()))
		{
			TFile *f = new TFile(root_filename_i.str().c_str(), "READ");
			TTree *t;
			if (f->GetListOfKeys()->Contains("lappddatamonitor_tree"))
				t = (TTree *)f->Get("lappddatamonitor_tree");
			else
			{
				Log("WARNING (MonitorLAPPDData): File " + root_filename_i.str() + " does not contain lappddatamonitor_tree. Omit file.", v_warning, verbosity);
				tree_exists = false;
			}

			if (tree_exists)
			{

				Log("MonitorLAPPDData: Tree exists, start reading in data", v_message, verbosity);

				std::vector<ULong64_t> *t_time = new std::vector<ULong64_t>;
				std::vector<ULong64_t> *t_end = new std::vector<ULong64_t>;
				std::vector<double> *t_pps_rate = new std::vector<double>;
				std::vector<double> *t_frame_rate = new std::vector<double>;
				std::vector<double> *t_beamgate_rate = new std::vector<double>;
				std::vector<double> *t_int_charge = new std::vector<double>;
				std::vector<double> *t_buffer_size = new std::vector<double>;
				std::vector<int> *t_board_idx = new std::vector<int>;
				std::vector<int> *t_chkey = new std::vector<int>;
				std::vector<double> *t_rate = new std::vector<double>;
				std::vector<double> *t_ped = new std::vector<double>;
				std::vector<double> *t_sigma = new std::vector<double>;
				std::vector<int> *t_raw_lappd_data_pps_counts = new std::vector<int>;
				std::vector<long> *t_raw_lappd_data_pps_timestamps = new std::vector<long>;
				std::vector<double> *t_pps_accumulated_number = new std::vector<double>;
				std::vector<long> *t_pps_accumulated_psec_timestamp = new std::vector<long>;
				std::vector<long> *t_raw_lappd_data_pps_timestamp_per_accumulated_number = new std::vector<long>;

				int t_run, t_subrun, t_partrun;
				int t_pps_count, t_frame_count;
				ULong64_t t_lappd_offset;

				int nentries_tree;

				t->SetBranchAddress("t_start", &t_time);
				t->SetBranchAddress("t_end", &t_end);
				t->SetBranchAddress("pps_rate", &t_pps_rate);
				t->SetBranchAddress("frame_rate", &t_frame_rate);
				t->SetBranchAddress("beamgate_rate", &t_beamgate_rate);
				t->SetBranchAddress("int_charge", &t_int_charge);
				t->SetBranchAddress("buffer_size", &t_buffer_size);
				t->SetBranchAddress("board_idx", &t_board_idx);
				t->SetBranchAddress("chkey", &t_chkey);
				t->SetBranchAddress("rate", &t_rate);
				t->SetBranchAddress("ped", &t_ped);
				t->SetBranchAddress("sigma", &t_sigma);
				t->SetBranchAddress("run", &t_run);
				t->SetBranchAddress("subrun", &t_subrun);
				t->SetBranchAddress("partrun", &t_partrun);
				t->SetBranchAddress("pps_count", &t_pps_count);
				t->SetBranchAddress("frame_count", &t_frame_count);
				t->SetBranchAddress("lappd_offset", &t_lappd_offset);
				t->SetBranchAddress("raw_lappd_data_pps_counts", &t_raw_lappd_data_pps_counts);
				t->SetBranchAddress("raw_lappd_data_pps_timestamps", &t_raw_lappd_data_pps_timestamps);
				t->SetBranchAddress("pps_accumulated_number", &t_pps_accumulated_number);
				t->SetBranchAddress("pps_accumulated_psec_timestamp", &t_pps_accumulated_psec_timestamp);
				t->SetBranchAddress("raw_lappd_data_pps_timestamp_per_accumulated_number", &t_raw_lappd_data_pps_timestamp_per_accumulated_number);

				nentries_tree = t->GetEntries();

				for (int i_board = 0; i_board < (int)board_configuration.size(); i_board++)
				{
					int board_nr = board_configuration[i_board];

					// Sort timestamps for the case that they are not in order

					std::vector<ULong64_t> vector_timestamps;
					std::map<ULong64_t, int> map_timestamp_entry;
					for (int i_entry = 0; i_entry < nentries_tree; i_entry++)
					{
						t->GetEntry(i_entry);
						if (t_time->at(i_board) >= timestamp_start && t_end->at(i_board) <= timestamp)
						{
							vector_timestamps.push_back(t_time->at(i_board));
							map_timestamp_entry.emplace(t_time->at(i_board), i_entry);
						}
					}

					std::sort(vector_timestamps.begin(), vector_timestamps.end());
					std::vector<int> vector_sorted_entry;

					for (int i_entry = 0; i_entry < (int)vector_timestamps.size(); i_entry++)
					{
						vector_sorted_entry.push_back(map_timestamp_entry.at(vector_timestamps.at(i_entry)));
					}

					for (int i_entry = 0; i_entry < (int)vector_sorted_entry.size(); i_entry++)
					{

						int next_entry = vector_sorted_entry.at(i_entry);

						t->GetEntry(next_entry);

						// std::cout <<"i_board: "<<i_board<<", t_time->at(i_board): "<<t_time->at(i_board)<<std::endl;
						// std::cout <<"timestamp_start: "<<timestamp_start<<", timestamp: "<<timestamp<<std::endl;
						if (t_time->at(i_board) >= timestamp_start && t_end->at(i_board) <= timestamp && t_end->at(i_board) != 0)
						{
							data_times_plot.at(board_nr).push_back(t_time->at(i_board));
							data_times_end_plot.at(board_nr).push_back(t_end->at(i_board));
							pps_rate_plot.at(board_nr).push_back(t_pps_rate->at(i_board));
							frame_rate_plot.at(board_nr).push_back(t_frame_rate->at(i_board));
							beamgate_rate_plot.at(board_nr).push_back(t_beamgate_rate->at(i_board));
							int_charge_plot.at(board_nr).push_back(t_int_charge->at(i_board));
							buffer_size_plot.at(board_nr).push_back(t_buffer_size->at(i_board));

							boost::posix_time::ptime boost_tend = *Epoch + boost::posix_time::time_duration(int(t_end->at(i_board) / MSEC_to_SEC / SEC_to_MIN / MIN_to_HOUR), int(t_end->at(i_board) / MSEC_to_SEC / SEC_to_MIN) % 60, int(t_end->at(i_board) / MSEC_to_SEC / 1000.) % 60, t_end->at(i_board) % 1000);
							struct tm label_timestamp = boost::posix_time::to_tm(boost_tend);
							//
							TDatime datime_timestamp(1900 + label_timestamp.tm_year, label_timestamp.tm_mon + 1, label_timestamp.tm_mday, label_timestamp.tm_hour, label_timestamp.tm_min, label_timestamp.tm_sec);
							// TDatime datime_timestamp(1995,1,1,1,27,6);
							// TDatimes initialised to dates before 1995/1/1 will be screwed up since this is the minimum date for the TDatime class
							// std::cout <<"t_end: "<<t_end->at(i_board)<<", datime: "<<datime_timestamp.Convert()<<", datime string: "<<datime_timestamp.AsString()<<std::endl;
							labels_timeaxis.at(board_nr).push_back(datime_timestamp);
							int min_board = board_channel[i_board];
							for (int i = 0; i < 30; i++)
							{
								int chankey = min_board + i;
								ped_plot.at(chankey).push_back(t_ped->at(chankey));
								sigma_plot.at(chankey).push_back(t_sigma->at(chankey));
								// std::cout <<"chankey "<<chankey<<", sigma: "<<t_sigma->at(chankey)<<std::endl;
								rate_plot.at(chankey).push_back(t_rate->at(chankey));
							}
							if (i_board == 0)
							{ // No need to have separate run information for different boards
								run_plot.push_back(t_run);
								subrun_plot.push_back(t_subrun);
								partrun_plot.push_back(t_partrun);
								lappdoffset_plot.push_back(t_lappd_offset);
								ppscount_plot.push_back(t_pps_count);
								framecount_plot.push_back(t_frame_count);
							}
						}
					}
				}

				for (int i = 0; i < t_raw_lappd_data_pps_timestamps->size(); i++) {
					raw_lappd_data_pps_timestamps.push_back(t_raw_lappd_data_pps_timestamps->at(i));
					raw_lappd_data_pps_counts.push_back(t_raw_lappd_data_pps_counts->at(i));
				}

				for (int i = 0; i < t_pps_accumulated_psec_timestamp->size(); i++) {
					pps_accumulated_psec_timestamp.push_back(t_pps_accumulated_psec_timestamp->at(i));
					pps_accumulated_number.push_back(t_pps_accumulated_number->at(i));

					// This and the vectors above the same length, as they are both added in an accumulated fashion,
					// so we can just add them together
					raw_lappd_data_pps_timestamp_per_accumulated_number.push_back(t_raw_lappd_data_pps_timestamp_per_accumulated_number->at(i));
				}
				
				// Delete vectors, if we have any
				delete t_time;
				delete t_end;
				delete t_pps_rate;
				delete t_frame_rate;
				delete t_beamgate_rate;
				delete t_int_charge;
				delete t_buffer_size;
				delete t_board_idx;
				delete t_chkey;
				delete t_rate;
				delete t_ped;
				delete t_sigma;
				delete t_raw_lappd_data_pps_counts;
				delete t_raw_lappd_data_pps_timestamps;
			}

			f->Close();
			delete f;
			gROOT->cd();
		}
		else
		{
			Log("MonitorLAPPDData: ReadFromFile: File " + root_filename_i.str() + " does not exist. Omit file.", v_warning, verbosity);
		}
	}

	// Set the readfromfile time variables to make sure data is not read twice for the same time window
	readfromfile_tend = timestamp;
	readfromfile_timeframe = time_frame;
}

void MonitorLAPPDData::DrawLastFilePlots()
{

	Log("MonitorLAPPDData: DrawLastFilePlots", v_message, verbosity);

	//-------------------------------------------------------
	//------------------DrawLastFilePlots -------------------
	//-------------------------------------------------------

	// Draw status of PSecData in last data file
	DrawStatus_PsecData();

	// Draw some histograms about the last file
	DrawLastFileHists();

	// Draw time alignment plots
	DrawTimeAlignment();
}

void MonitorLAPPDData::UpdateMonitorPlotsLAPPD(std::vector<double> timeFrames, std::vector<ULong64_t> endTimes, std::vector<std::string> fileLabels, std::vector<std::vector<std::string>> plotTypes)
{

	Log("MonitorLAPPDData: UpdateMonitorPlotsLAPPD", v_message, verbosity);

	//-------------------------------------------------------
	//------------------UpdateMonitorPlots ------------------
	//-------------------------------------------------------

	// Draw the monitoring plots according to the specifications in the configfiles

	for (unsigned int i_time = 0; i_time < timeFrames.size(); i_time++)
	{

		ULong64_t zero = 0;
		if (endTimes.at(i_time) == zero)
			endTimes.at(i_time) = t_file_end_global; // set 0 for t_file_end since we did not know what that was at the beginning of initialise

		for (unsigned int i_plot = 0; i_plot < plotTypes.at(i_time).size(); i_plot++)
		{
			if (plotTypes.at(i_time).at(i_plot) == "TimeEvolution")
				DrawTimeEvolutionLAPPDData(endTimes.at(i_time), timeFrames.at(i_time), fileLabels.at(i_time));
			else
			{
				if (verbosity > 0)
					std::cout << "ERROR (MonitorLAPPDData): UpdateMonitorPlotsLAPPD: Specified plot type -" << plotTypes.at(i_time).at(i_plot) << "- does not exist! Omit entry." << std::endl;
			}
		}
	}
}

void MonitorLAPPDData::DrawStatus_PsecData()
{

	Log("MonitorLAPPDData: DrawStatus_PsecData", v_message, verbosity);

	//-------------------------------------------------------
	//-------------DrawStatus_PsecData ----------------------
	//-------------------------------------------------------

	// TODO: Add status board for PSEC data status
	// TODO: One Status board for each ACDC board
	//
	//  ----Rough sketch-----
	//  Title: LAPPD PSEC Data
	//  PPS Rate:
	//  Frame Rate:
	//  Buffer size:
	//  Integrated charge:
	//  PPS count:
	//  Data count:
	//  ----End of rough sketch---
	//
	//  Number of rows in canvas: 5 (maximum of 10 rows, so it fits)

	// TODO: Implement checks for bad values and draw them red

	double meanPPSRate = 0.0;
	double meanFrameRate = 0.0;
	double meanBufferSize = 0.0;
	double meanIntegratedCharge = 0.0;
	// int totalPPSCount = 0;
	// int totalFrameCount = 0;
	// int totalRun = 0;

	bool has_board_minus = false;
	for (int i_board = 0; i_board < current_pps_rate.size(); i_board++)
	{
		if (board_configuration.at(i_board) == -1)
			has_board_minus = true;
	}
	// Calculate mean values
	for (int i_board = 0; i_board < current_pps_rate.size(); i_board++)
	{
		int board_nr = board_configuration.at(i_board);
		int size_vectors = current_pps_rate.size();
		if (has_board_minus)
			size_vectors -= 1;
		if (board_nr != -1)
		{
			meanPPSRate += (current_pps_rate.at(i_board) / (double)(size_vectors));
			meanFrameRate += (current_frame_rate.at(i_board) / (double)(size_vectors));
			meanBufferSize += (current_buffer_size.at(i_board) / (double)(size_vectors));
			meanIntegratedCharge += (current_int_charge.at(i_board) / (double)(size_vectors));
		}
	}
	// Calculate integrated values
	if (current_run == totalRun)
	{
		totalPPSCount += current_pps_count;
		totalFrameCount += current_frame_count;
	}
	else
	{
		Log("MonitorLAPPDData: New run encountered, resetting PPS and data counters. New run: " + std::to_string(current_run) + ", old run: " + std::to_string(totalRun), v_message, verbosity);
		totalRun = current_run;
		totalPPSCount = current_pps_count;
		totalFrameCount = current_frame_count;
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

	std::stringstream ss_text_psec_pps_count;
	ss_text_psec_pps_count << "PPS Events : " << totalPPSCount << " (" << current_time.str() << ")";
	text_pps_count->SetText(0.06, 0.5, ss_text_psec_pps_count.str().c_str());
	text_pps_count->SetTextColor(1);

	std::stringstream ss_text_psec_frame_count;
	ss_text_psec_frame_count << "Data Events : " << totalFrameCount << " (" << current_time.str() << ")";
	text_frame_count->SetText(0.06, 0.4, ss_text_psec_frame_count.str().c_str());
	text_frame_count->SetTextColor(1);

	text_data_title->SetTextSize(0.05);
	text_pps_rate->SetTextSize(0.05);
	text_frame_rate->SetTextSize(0.05);
	text_buffer_size->SetTextSize(0.05);
	text_int_charge->SetTextSize(0.05);
	text_pps_count->SetTextSize(0.05);
	text_frame_count->SetTextSize(0.05);

	text_data_title->SetNDC(1);
	text_pps_rate->SetNDC(1);
	text_frame_rate->SetNDC(1);
	text_buffer_size->SetNDC(1);
	text_int_charge->SetNDC(1);
	text_pps_count->SetNDC(1);
	text_frame_count->SetNDC(1);

	canvas_status_data->cd();
	canvas_status_data->Clear();
	text_data_title->Draw();
	text_pps_rate->Draw();
	text_frame_rate->Draw();
	text_buffer_size->Draw();
	//	text_int_charge->Draw();
	text_pps_count->Draw();
	text_frame_count->Draw();

	std::stringstream ss_path_psecinfo;
	ss_path_psecinfo << outpath << "LAPPDData_PSECData_current." << img_extension;
	canvas_status_data->SaveAs(ss_path_psecinfo.str().c_str());
	canvas_status_data->Clear();

	for (int i_board = 0; i_board < (int)board_configuration.size(); i_board++)
	{

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

// ToDo: Also implement an overall status board showing whether everything is awesome and if there is something wrong, which board has issues

void MonitorLAPPDData::DrawLastFileHists()
{
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

	// TODO: Add histogram that shows the number of active channelkeys for each board

	std::stringstream current_time;
	// Draw histograms for each board
	for (int i_board = 0; i_board < (int)board_configuration.size(); i_board++)
	{

		int board_nr = board_configuration.at(i_board);
		uint64_t t_fileend = t_file_end.at(i_board);
		boost::posix_time::ptime currenttime = *Epoch + boost::posix_time::time_duration(int(t_fileend / MSEC_to_SEC / SEC_to_MIN / MIN_to_HOUR), int(t_fileend / MSEC_to_SEC / SEC_to_MIN) % 60, int(t_fileend / MSEC_to_SEC) % 60, t_fileend % 1000);
		struct tm currenttime_tm = boost::posix_time::to_tm(currenttime);
		current_time.str("");
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
		ss_text_waveform_voltages << "LAPPD waveform Board " << board_nr << " (" << current_time.str() << ")";
		hist_waveform_voltages.at(board_nr)->SetTitle(ss_text_waveform_voltages.str().c_str());
		hist_waveform_voltages.at(board_nr)->SetStats(0);
		hist_waveform_voltages.at(board_nr)->Draw("COLZ");
		std::stringstream ss_path_waveform_voltages;
		ss_path_waveform_voltages << outpath << "LAPPDWaveform_Voltages_Board" << board_nr << "_current." << img_extension;
		canvas_waveform_voltages->SaveAs(ss_path_waveform_voltages.str().c_str());

		// std::cout <<"PedestalFits"<<std::endl;
		// PedestalFits(board_nr, i_board);

		int entries = (int)data_beamgate_lastfile.at(board_nr).size();
		std::vector<bool> isPlotted;
		for (size_t i_channel = 0; i_channel < 30; i_channel++)
		{
			isPlotted.push_back(false);
		}

		for (int i_event = 0; i_event < 100 /*entries*/; i_event++)
		{
			for (size_t i_channel = 0; i_channel < hist_waveforms_onedim.at(i_event).at(board_nr).size(); i_channel++)
			{
				canvas_waveform_onedim->Clear();
				canvas_waveform_onedim->cd();
				std::stringstream ss_text_waveform_onedim;
				ss_text_waveform_onedim << "LAPPD waveform board " << board_nr << " channel " << i_channel << " event number " << i_event << " (" << current_time.str() << ")";
				hist_waveforms_onedim.at(i_event).at(board_nr).at(i_channel)->SetTitle(ss_text_waveform_onedim.str().c_str());
				hist_waveforms_onedim.at(i_event).at(board_nr).at(i_channel)->SetStats(0);

				// Set color (not used right now)
				// hist_waveforms_onedim.at(board_nr).at(i_channel)->SetMarkerColor(colorVec.at(i_channel));
				// hist_waveforms_onedim.at(board_nr).at(i_channel)->SetLineColor(colorVec.at(i_channel));

				hist_waveforms_onedim.at(i_event).at(board_nr).at(i_channel)->Draw("HIST");

				// Only save plot if pulse was seen (or if no pulse was observed)
				if ((hist_waveforms_onedim.at(i_event).at(board_nr).at(i_channel)->GetMaximum() > mean_pedestal.at(board_nr).at(i_channel) + 5 * sigma_pedestal.at(board_nr).at(i_channel)) || (/*i_event + 1 == 100*/ /*entries*/ /*&&*/ !isPlotted.at(i_channel)))
				{
					if (hist_waveforms_onedim.at(i_event).at(board_nr).at(i_channel)->GetEntries() > 0)
					{
						// std::cout <<"Save canvas for channel "<< i_channel <<std::endl;
						std::stringstream ss_path_waveform_onedim;
						ss_path_waveform_onedim << outpath << "LAPPDWaveform_waveform_onedim_Board" << board_nr << "channel" << i_channel << "_current." << img_extension;
						canvas_waveform_onedim->SaveAs(ss_path_waveform_onedim.str().c_str());
						isPlotted.at(i_channel) = true;
					}

					//				Testing
					//				std::cout << "i_event " << i_event << " i_channel " << i_channel << std::endl;
					//				std::cout << "Baseline " << mean_pedestal.at(board_nr).at(i_channel) << std::endl;
					//				std::cout << "Maximum " << hist_waveforms_onedim.at(i_event).at(board_nr).at(i_channel)->GetMaximum() << std::endl;
				}
				// hist_waveforms_onedim.at(i_event).at(board_nr).at(i_channel)->Reset();
			}
		}

	} // end board loop

	canvas_pedestal_all->Clear();
	canvas_pedestal_all->cd();
	canvas_pedestal_all->SetRightMargin(0.15);
	std::stringstream ss_title_pedestals;
	ss_title_pedestals << "Baselines (" << current_time.str() << ")";
	hist_pedestal_all->SetTitle(ss_title_pedestals.str().c_str());
	TPad *p = (TPad *)canvas_pedestal_all->cd();
	p->SetGrid();
	hist_pedestal_all->Draw("COLZ");
	p->Update();
	std::stringstream ss_path_pedestals;
	ss_path_pedestals << outpath << "LAPPD_Pedestals_current." << img_extension;
	canvas_pedestal_all->SaveAs(ss_path_pedestals.str().c_str());

	canvas_buffer_size_all->Clear();
	canvas_buffer_size_all->cd();
	canvas_buffer_size_all->SetRightMargin(0.15);
	std::stringstream ss_title_buffer;
	ss_title_buffer << "Buffer Size (" << current_time.str() << ")";
	hist_buffer_size_all->SetTitle(ss_title_buffer.str().c_str());
	TPad *prate = (TPad *)canvas_buffer_size_all->cd();
	prate->SetGrid();
	hist_buffer_size_all->Draw("COLZ");
	prate->Update();
	std::stringstream ss_path_rate;
	ss_path_rate << outpath << "LAPPD_Buffer_Size_current." << img_extension;
	canvas_buffer_size_all->SaveAs(ss_path_rate.str().c_str());

	std::stringstream ss_title_rate_threshold;
	ss_title_rate_threshold << "Rate over Threshold - (" << current_time.str() << ")";
	hist_rate_threshold_all->SetTitle(ss_title_rate_threshold.str().c_str());
	canvas_rate_threshold_all->Clear();
	canvas_rate_threshold_all->cd();
	canvas_rate_threshold_all->SetRightMargin(0.15);
	TPad *pthre = (TPad *)canvas_rate_threshold_all->cd();
	pthre->SetGrid();
	hist_rate_threshold_all->Draw("COLZ");
	pthre->Update();
	std::stringstream ss_path_rate_threshold;
	ss_path_rate_threshold << outpath << "LAPPD_Rates_Threshold_current." << img_extension;
	canvas_rate_threshold_all->SaveAs(ss_path_rate_threshold.str().c_str());

	std::stringstream ss_title_events_per_channel;
	ss_title_events_per_channel << "Events (" << current_time.str() << ")";
	hist_events_per_channel->SetTitle(ss_title_events_per_channel.str().c_str());
	canvas_events_per_channel->Clear();
	canvas_events_per_channel->cd();
	canvas_events_per_channel->SetRightMargin(0.15);
	TPad *pevents = (TPad *)canvas_events_per_channel->cd();
	pevents->SetGrid();
	hist_events_per_channel->Draw("COLZ");
	pevents->Update();
	std::stringstream ss_path_events_channel;
	ss_path_events_channel << outpath << "LAPPD_Events_Per_Channel_current." << img_extension;
	canvas_events_per_channel->SaveAs(ss_path_events_channel.str().c_str());
}

void MonitorLAPPDData::PedestalFits(int board_nr, int i_board)
{
	uint64_t t_fileend = t_file_end.at(i_board);
	boost::posix_time::ptime currenttime = *Epoch + boost::posix_time::time_duration(int(t_fileend / MSEC_to_SEC / SEC_to_MIN / MIN_to_HOUR), int(t_fileend / MSEC_to_SEC / SEC_to_MIN) % 60, int(t_fileend / MSEC_to_SEC) % 60, t_fileend % 1000);
	struct tm currenttime_tm = boost::posix_time::to_tm(currenttime);
	std::stringstream current_time;
	current_time << currenttime_tm.tm_year + 1900 << "/" << currenttime_tm.tm_mon + 1 << "/" << currenttime_tm.tm_mday << "-" << currenttime_tm.tm_hour << ":" << currenttime_tm.tm_min << ":" << currenttime_tm.tm_sec;

	std::vector<int> entries_vec;
	std::vector<double> mean_vec;
	std::vector<double> sigma_vec;
	std::vector<double> rate_vec;
	for (size_t i_channel = 0; i_channel < hist_pedestal.at(board_nr).size(); i_channel++)
	{
		canvas_pedestal->Clear();
		canvas_pedestal->cd();
		std::stringstream ss_path_pedestal;
		ss_path_pedestal << outpath << "LAPPDPedestal_Board" << board_nr << "channel" << i_channel << "_current." << img_extension;
		std::stringstream ss_text_pedestal;
		ss_text_pedestal << "LAPPD pedestal board " << board_nr << "channel " << i_channel << " (" << current_time.str() << ")";
		hist_pedestal.at(board_nr).at(i_channel)->SetTitle(ss_text_pedestal.str().c_str());
		hist_pedestal.at(board_nr).at(i_channel)->SetStats(0);
		entries_vec.push_back(hist_pedestal.at(board_nr).at(i_channel)->GetEntries());
		double minimum_adc = -100;
		double maximum_adc = 0;
		TF1 *fgaus = new TF1("fgaus", "gaus", minimum_adc, maximum_adc);
		fgaus->SetParameter(1, hist_pedestal.at(board_nr).at(i_channel)->GetMean());
		fgaus->SetParameter(2, hist_pedestal.at(board_nr).at(i_channel)->GetRMS());
		TFitResultPtr gaussFitResult = hist_pedestal.at(board_nr).at(i_channel)->Fit("fgaus", "Q");
		int gaussFitResultInt = gaussFitResult;

		// TODO: Put this in own function and tune parameters

		hist_pedestal.at(board_nr).at(i_channel)->Draw();
		fgaus->Draw("SAME");
		canvas_pedestal->SaveAs(ss_path_pedestal.str().c_str());

		int counter = 0;
		int counterThreshold = 0;

		if (gaussFitResultInt == 0)
		{
			bool outOfBounds = ((fgaus->GetParameter(1) < 2000.) || (fgaus->GetParameter(1) > 3200.) || (fgaus->GetParameter(1) > 500.) || (fgaus->GetParameter(1) < 50.));
			bool suddenChange = false;
			// Look for a sudden change if we do not perform the first fit per board and channel
			if (mean_pedestal.size() > i_board)
			{
				if (mean_pedestal.at(board_nr).size() > i_channel)
				{
					suddenChange = (fabs(fgaus->GetParameter(1) - mean_pedestal.at(board_nr).at(i_channel)) > 10 || fabs(fgaus->GetParameter(2) - sigma_pedestal.at(board_nr).at(i_channel)) > 0.2);
				}
			}
			if (!outOfBounds && !suddenChange)
			{
				mean_vec.push_back(fgaus->GetParameter(1));
				sigma_vec.push_back(fgaus->GetParameter(2));
				hist_pedestal_all->SetBinContent(i_channel + 1, i_board + 1, fgaus->GetParameter(1));
			}
			else
			{
				mean_vec.push_back(hist_pedestal.at(board_nr).at(i_channel)->GetMean());
				sigma_vec.push_back(hist_pedestal.at(board_nr).at(i_channel)->GetRMS());
				hist_pedestal_all->SetBinContent(i_channel + 1, i_board + 1, hist_pedestal.at(board_nr).at(i_channel)->GetMean());
			}
		}
		else
		{
			mean_vec.push_back(hist_pedestal.at(board_nr).at(i_channel)->GetMean());
			sigma_vec.push_back(hist_pedestal.at(board_nr).at(i_channel)->GetRMS());
			hist_pedestal_all->SetBinContent(i_channel + 1, i_board + 1, hist_pedestal.at(board_nr).at(i_channel)->GetMean());
		}

		for (int i_bin = 0; i_bin < hist_pedestal.at(board_nr).at(i_channel)->GetNbinsX(); i_bin++)
		{
			double center = hist_pedestal.at(board_nr).at(i_channel)->GetBinCenter(i_bin + 1);
			int content = hist_pedestal.at(board_nr).at(i_channel)->GetBinContent(i_bin + 1);
			// if (center > 0) {
			counter += content;
			//}
			// TODO: Find better parameter
			// int x = 5;
			if (center > (mean_vec.at(i_channel) + threshold_pulse * sigma_vec.at(i_channel)))
			{
				counterThreshold += content;
			}
			else if (center < (mean_vec.at(i_channel) - threshold_pulse * sigma_vec.at(i_channel)))
			{
				counterThreshold += content;
			}
		}

		// int entries = (int) data_beamgate_lastfile.at(board_nr).size();
		// int entries = (int) hist_pedestal.at(board_nr).at(i_channel)->GetNbinsX();
		int entries = n_data.at(board_nr);
		if (i_channel == 5)
			counter = 256 * entries; // manually set trigger channel to 256 buffer size
		int bufferSize;
		if (entries == 0)
		{
			bufferSize = 0;
		}
		else
		{
			bufferSize = counter / entries;
		}

		// ToDo: Fix this number in config file?
		int numberOfSamples = 256;
		int numberOfEvents = counter / numberOfSamples;
		hist_buffer_size_all->SetBinContent(i_channel + 1, i_board + 1, bufferSize);
		hist_rate_threshold_all->SetBinContent(i_channel + 1, i_board + 1, counterThreshold);
		hist_events_per_channel->SetBinContent(i_channel + 1, i_board + 1, numberOfEvents);
		rate_vec.push_back(counterThreshold);
		delete fgaus;
	}
	num_entries[board_nr] = entries_vec;
	mean_pedestal[board_nr] = mean_vec;
	sigma_pedestal[board_nr] = sigma_vec;
	rate_pedestal[board_nr] = rate_vec;
}

void MonitorLAPPDData::DrawTimeAlignment()
{

	Log("MonitorLAPPDData: DrawTimeAlignment", v_message, verbosity);

	//-------------------------------------------------------
	//-------------DrawTimeAlignment ------------------------
	//-------------------------------------------------------

	// TODO: Implement function to fill time alignment histograms
	// Probably for different time frames: Last file, last 10 files, last 20 files, ...

	// TODO: Add titles to histograms
	// TODO: Write canvas as image to disk

	for (int i_board = 0; i_board < (int)board_configuration.size(); i_board++)
	{

		int board_nr = board_configuration.at(i_board);
		uint64_t t_fileend = t_file_end.at(i_board);
		boost::posix_time::ptime currenttime = *Epoch + boost::posix_time::time_duration(int(t_fileend / MSEC_to_SEC / SEC_to_MIN / MIN_to_HOUR), int(t_fileend / MSEC_to_SEC / SEC_to_MIN) % 60, int(t_fileend / MSEC_to_SEC) % 60, t_fileend % 1000);
		struct tm currenttime_tm = boost::posix_time::to_tm(currenttime);
		std::stringstream current_time;
		current_time << currenttime_tm.tm_year + 1900 << "/" << currenttime_tm.tm_mon + 1 << "/" << currenttime_tm.tm_mday << "-" << currenttime_tm.tm_hour << ":" << currenttime_tm.tm_min << ":" << currenttime_tm.tm_sec;

		//--------Last File-----------
		// std::cout << "size before " << hist_align_1file.at(board_nr)->GetEntries() << std::endl;

		hist_align_1file.at(board_nr)->Reset();

		// std::cout << "size after " << hist_align_1file.at(board_nr)->GetEntries() << std::endl;

		std::stringstream ss_1file;
		ss_1file << "hist_align_1file_board" << i_board;
		//  hist_align_1file.at(board_nr)->SetName(ss_1file.str().c_str());
		//  hist_align_1file.at(board_nr)->SetTitle(ss_1file.str().c_str());
		for (int i_align = 0; i_align < (int)data_beamgate_lastfile.at(board_nr).size(); i_align++)
		{
			hist_align_1file.at(board_nr)->Fill(data_beamgate_lastfile.at(board_nr).at(i_align) * 3.125);
		}

		//-------Last 5 Files---------
		hist_align_5files.at(board_nr)->Reset();
		for (int i_align = 0; i_align < (int)data_beamgate_last5files.at(board_nr).size(); i_align++)
		{
			for (int i_data = 0; i_data < (int)data_beamgate_last5files.at(board_nr).at(i_align).size(); i_data++)
			{
				hist_align_5files.at(board_nr)->Fill(data_beamgate_last5files.at(board_nr).at(i_align).at(i_data) * 3.125);
			}
		}

		//-------Last 10 Files--------
		hist_align_10files.at(board_nr)->Reset();
		for (int i_align = 0; i_align < (int)data_beamgate_last10files.at(board_nr).size(); i_align++)
		{
			for (int i_data = 0; i_data < (int)data_beamgate_last10files.at(board_nr).at(i_align).size(); i_data++)
			{
				hist_align_10files.at(board_nr)->Fill(data_beamgate_last10files.at(board_nr).at(i_align).at(i_data) * 3.125);
				if (verbosity > 4)
					std::cout << "MonitorLAPPDData: Filling time alignment of " << (data_beamgate_last10files.at(board_nr).at(i_align).at(i_data)) << std::endl;
			}
		}

		//------Last 20 Files---------
		hist_align_20files.at(board_nr)->Reset();
		for (int i_align = 0; i_align < (int)data_beamgate_last20files.at(board_nr).size(); i_align++)
		{
			for (int i_data = 0; i_data < (int)data_beamgate_last20files.at(board_nr).at(i_align).size(); i_data++)
			{
				hist_align_20files.at(board_nr)->Fill(data_beamgate_last20files.at(board_nr).at(i_align).at(i_data) * 3.125);
			}
		}

		//------Last 100 Files---------
		hist_align_100files.at(board_nr)->Reset();
		for (int i_align = 0; i_align < (int)data_beamgate_last100files.at(board_nr).size(); i_align++)
		{
			for (int i_data = 0; i_data < (int)data_beamgate_last100files.at(board_nr).at(i_align).size(); i_data++)
			{
				hist_align_100files.at(board_nr)->Fill(data_beamgate_last100files.at(board_nr).at(i_align).at(i_data) * 3.125);
			}
		}

		//------Last 1000 Files---------
		hist_align_1000files.at(board_nr)->Reset();
		for (int i_align = 0; i_align < (int)data_beamgate_last1000files.at(board_nr).size(); i_align++)
		{
			for (int i_data = 0; i_data < (int)data_beamgate_last1000files.at(board_nr).at(i_align).size(); i_data++)
			{
				hist_align_1000files.at(board_nr)->Fill(data_beamgate_last1000files.at(board_nr).at(i_align).at(i_data) * 3.125);
			}
		}

		// The drawing happens here
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

		canvas_align_1000files->Clear();
		canvas_align_1000files->cd();
		std::stringstream ss_text_align1000;
		ss_text_align1000 << "Alignment Thousand Files Board " << board_nr << " (" << current_time.str() << ")";
		hist_align_1000files.at(board_nr)->SetTitle(ss_text_align1000.str().c_str());

		hist_align_1000files.at(board_nr)->SetStats(0);
		hist_align_1000files.at(board_nr)->Draw("");
		std::stringstream ss_path_align1000;
		ss_path_align1000 << outpath << "LAPPD_Time_Alignment_Thousand_Files_Board" << board_nr << "_current." << img_extension;
		canvas_align_1000files->SaveAs(ss_path_align1000.str().c_str());
	}
}

void MonitorLAPPDData::DrawTimeEvolutionLAPPDData(ULong64_t timestamp_end, double time_frame, std::string file_ending)
{

	Log("MonitorLAPPDData: DrawTimeEvolutionLAPPDData", v_message, verbosity);

	//--------------------------------------------------------
	//-------------DrawTimeEvolutionLAPPDData ----------------
	// -------------------------------------------------------

	boost::posix_time::ptime endtime = *Epoch + boost::posix_time::time_duration(int(timestamp_end / MSEC_to_SEC / SEC_to_MIN / MIN_to_HOUR), int(timestamp_end / MSEC_to_SEC / SEC_to_MIN) % 60, int(timestamp_end / MSEC_to_SEC / 1000.) % 60, timestamp_end % 1000);
	struct tm endtime_tm = boost::posix_time::to_tm(endtime);
	std::stringstream end_time;
	end_time << endtime_tm.tm_year + 1900 << "/" << endtime_tm.tm_mon + 1 << "/" << endtime_tm.tm_mday << "-" << endtime_tm.tm_hour << ":" << endtime_tm.tm_min << ":" << endtime_tm.tm_sec;

	if (verbosity > 2)
		std::cout << "MonitorLAPPDData:DrawTimeEvolutionLAPPDData. timestamp_end: " << timestamp_end << ", time_frame; " << time_frame << ", end_time: " << end_time.str() << std::endl;

	if (int(time_frame * 60 * 60 * 1000) > timestamp_end)
		time_frame = 0.99 * (timestamp_end / 60 / 60 / 1000.);

	if (timestamp_end != readfromfile_tend || time_frame != readfromfile_timeframe)
		ReadFromFile(timestamp_end, time_frame);

	// looping over all files that are in the time interval, each file will be one data point

	std::stringstream ss_timeframe;
	ss_timeframe << round(time_frame * 100.) / 100.;

	// Set 0 rates in case no data was found for a board
	for (int i_board = 0; i_board < (int)board_configuration.size(); i_board++)
	{
		// std::cout <<"board: "<<i_board<<std::endl;

		int board_nr = board_configuration.at(i_board);
		// std::cout <<"board_nr: "<<board_nr<<std::endl;
		if (frame_rate_plot.at(board_nr).size() == 0)
		{
			bool found_partner = false;
			for (int j_board = 0; j_board < (int)board_configuration.size(); j_board++)
			{
				if (found_partner)
					continue;
				// std::cout <<"j_board: "<<j_board<<std::endl;
				if (i_board == j_board)
					continue;
				int board_nr2 = board_configuration.at(j_board);
				if (frame_rate_plot.at(board_nr2).size() != 0)
				{
					// for (int i_vec=0; i_vec<frame_rate_plot.at(board_nr2).size(); i_vec++){
					int i_vec = 0;
					pps_rate_plot.at(board_nr).push_back(0.);
					frame_rate_plot.at(board_nr).push_back(0.);
					buffer_size_plot.at(board_nr).push_back(0.);
					int_charge_plot.at(board_nr).push_back(0.);
					labels_timeaxis.at(board_nr).push_back(labels_timeaxis.at(board_nr2).at(i_vec));
					found_partner = true;
					//}
				}
			}
		}
	}

	graph_pps_count->Set(0);
	graph_pps_event_counter->Set(0);
	graph_pps_accumulated_number_vs_psec_timestamp->Set(0);
	graph_frame_count->Set(0);

	for (int i_board = 0; i_board < (int)board_configuration.size(); i_board++)
	{
		// std::cout <<"board: "<<i_board<<std::endl;
		int board_nr = board_configuration.at(i_board);
		int min_board = board_channel.at(i_board);

		graph_pps_rate.at(board_nr)->Set(0);
		graph_frame_rate.at(board_nr)->Set(0);
		graph_buffer_size.at(board_nr)->Set(0);
		graph_int_charge.at(board_nr)->Set(0);

		if (board_nr != -1)
		{
			for (int i = 0; i < 30; i++)
			{
				int chkey = min_board + i;
				graph_rate.at(chkey)->Set(0);
				graph_sigma.at(chkey)->Set(0);
				graph_ped.at(chkey)->Set(0);
			}
		}

		int integrated_pps = 0;
		int integrated_data = 0;
		int integrated_run = 0;
		/*std::cout <<"board_nr: "<<board_nr<<std::endl;
		std::cout <<"pps_rate_plot.size(): "<<pps_rate_plot.at(board_nr).size()<<std::endl;
		std::cout <<"labels_timeaxis.size(): "<<labels_timeaxis.at(board_nr).size()<<std::endl;
		std::cout <<"frame_rate_plot.size(): "<<frame_rate_plot.at(board_nr).size()<<std::endl;
		std::cout <<"buffer_size_plot.size(): "<<buffer_size_plot.at(board_nr).size()<<std::endl;
		std::cout <<"int_charge_plot.size(): "<<int_charge_plot.at(board_nr).size()<<std::endl;*/
		for (unsigned int i_file = 0; i_file < pps_rate_plot.at(board_nr).size(); i_file++)
		{
			Log("MonitorLAPPDData: Stored data (file #" + std::to_string(i_file + 1) + ") and i_board: " + std::to_string(board_nr), v_message, verbosity);
			graph_pps_rate.at(board_nr)->SetPoint(i_file, labels_timeaxis.at(board_nr)[i_file].Convert(), pps_rate_plot.at(board_nr).at(i_file));
			graph_frame_rate.at(board_nr)->SetPoint(i_file, labels_timeaxis.at(board_nr)[i_file].Convert(), frame_rate_plot.at(board_nr).at(i_file));
			graph_buffer_size.at(board_nr)->SetPoint(i_file, labels_timeaxis.at(board_nr)[i_file].Convert(), buffer_size_plot.at(board_nr).at(i_file));
			graph_int_charge.at(board_nr)->SetPoint(i_file, labels_timeaxis.at(board_nr)[i_file].Convert(), int_charge_plot.at(board_nr).at(i_file));
			if (board_nr != -1 && buffer_size_plot.at(board_nr).at(i_file) > 0)
			{
				for (int i = 0; i < 30; i++)
				{
					int chkey = min_board + i;
					graph_rate.at(chkey)->SetPoint(i_file, labels_timeaxis.at(board_nr)[i_file].Convert(), rate_plot.at(chkey).at(i_file));
					graph_ped.at(chkey)->SetPoint(i_file, labels_timeaxis.at(board_nr)[i_file].Convert(), ped_plot.at(chkey).at(i_file));
					graph_sigma.at(chkey)->SetPoint(i_file, labels_timeaxis.at(board_nr)[i_file].Convert(), sigma_plot.at(chkey).at(i_file));
				}
			}

			// PPS and data count plots
			if (i_board == 0)
			{
				if (integrated_run == run_plot.at(i_file))
				{
					integrated_pps += ppscount_plot.at(i_file);
					integrated_data += framecount_plot.at(i_file);
				}
				else
				{
					integrated_pps = ppscount_plot.at(i_file);
					integrated_data = framecount_plot.at(i_file);
					integrated_run = run_plot.at(i_file);
				}
				graph_pps_count->SetPoint(i_file, labels_timeaxis.at(board_nr)[i_file].Convert(), integrated_pps);
				graph_frame_count->SetPoint(i_file, labels_timeaxis.at(board_nr)[i_file].Convert(), integrated_data);
			}
		}

		// Drawing time evolution plots

		// std::cout <<"max_canvas"<<std::endl;
		double max_canvas = 0;
		double min_canvas = 9999999.;

		// std::cout <<"board_nr: "<<board_nr<<std::endl;
		// std::cout <<"pps_rate_plot.at(board_nr).size(): "<<pps_rate_plot.at(board_nr).size()<<std::endl;
		if (i_board == 0)
		{
			// Add graph points for PPS event counter
			for (int i_timestamp = 0; i_timestamp < raw_lappd_data_pps_timestamps.size(); i_timestamp++)
			{
				double timestamp_to_e13 = (double)raw_lappd_data_pps_timestamps.at(i_timestamp) / pow(10, 13);
				graph_pps_event_counter->SetPoint(i_timestamp, timestamp_to_e13, raw_lappd_data_pps_counts.at(i_timestamp));
			}
			// Add graph points for PPS accumulated number
			for (int i_timestamp = 0; i_timestamp < pps_accumulated_psec_timestamp.size(); i_timestamp++)
			{
				auto acc_timestamp = pps_accumulated_psec_timestamp.at(i_timestamp);
				auto acc_number = pps_accumulated_number.at(i_timestamp);
				graph_pps_accumulated_number_vs_psec_timestamp->SetPoint(i_timestamp, acc_timestamp, acc_number);

				// Also add graph points for PPS time vs accumulated number
				auto acc_pps_timestamp_to_e13 = (double)raw_lappd_data_pps_timestamp_per_accumulated_number.at(i_timestamp) / pow(10, 13);
				graph_pps_time_vs_accumulated_number->SetPoint(i_timestamp, acc_number, acc_pps_timestamp_to_e13);
			}

			std::stringstream ss_pps_count;
			ss_pps_count << "PPS events time evolution (last " << ss_timeframe.str() << "h) " << end_time.str();
			canvas_pps_count->cd();
			canvas_pps_count->Clear();
			graph_pps_count->SetTitle(ss_pps_count.str().c_str());
			graph_pps_count->GetYaxis()->SetTitle("PPS events");
			graph_pps_count->GetXaxis()->SetTimeDisplay(1);
			graph_pps_count->GetXaxis()->SetLabelSize(0.03);
			graph_pps_count->GetXaxis()->SetLabelOffset(0.03);
			graph_pps_count->GetXaxis()->SetTimeFormat("#splitline{%m/%d}{%H:%M}");
			graph_pps_count->GetXaxis()->SetTimeOffset(0.);
			graph_pps_count->Draw("apl");
			double max_pps_count = 1.;
			double max_pps_count2 = 1.;
			if (ppscount_plot.size() > 0)
				max_pps_count2 = TMath::MaxElement(ppscount_plot.size(), graph_pps_count->GetY());
			;
			if (max_pps_count2 > max_pps_count)
				max_pps_count = max_pps_count2;
			graph_pps_count->GetYaxis()->SetRangeUser(0.000, 1.1 * max_pps_count);
			std::stringstream ss_pps_count_path;
			ss_pps_count_path << outpath << "LAPPDData_TimeEvolution_PPSEvents_" << file_ending << "." << img_extension;
			canvas_pps_count->SaveAs(ss_pps_count_path.str().c_str());

			std::stringstream ss_pps_event_counter;
			ss_pps_event_counter << "PPS event counter time evolution (last " << ss_timeframe.str() << "h) " << end_time.str();
			canvas_pps_event_counter->cd();
			canvas_pps_event_counter->Clear();
			graph_pps_event_counter->SetTitle(ss_pps_event_counter.str().c_str());
			graph_pps_event_counter->GetYaxis()->SetTitle("PPS event counter");
			graph_pps_event_counter->GetXaxis()->SetTitle("ns (1e13)");
			graph_pps_event_counter->GetXaxis()->SetTimeDisplay(0);
			graph_pps_event_counter->GetXaxis()->SetLabelSize(0.03);
			graph_pps_event_counter->GetXaxis()->SetLabelOffset(0.01);
			graph_pps_event_counter->GetXaxis()->SetTimeOffset(0.);
			graph_pps_event_counter->SetMarkerSize(0.4F);
			graph_pps_event_counter->SetMarkerColor(kBlue);
			graph_pps_event_counter->SetMarkerStyle(kFullCircle);
			graph_pps_event_counter->Draw("AP");
			std::stringstream ss_pps_event_counter_path;
			ss_pps_event_counter_path << outpath << "LAPPDData_TimeEvolution_PPSEventCounter_" << file_ending << "." << img_extension;
			canvas_pps_event_counter->SaveAs(ss_pps_event_counter_path.str().c_str());

			std::stringstream ss_pps_accumulated_number_vs_psec_timestamp;
			ss_pps_accumulated_number_vs_psec_timestamp << "PPS Accumulated Number vs System Time (last " << ss_timeframe.str() << "h) " << end_time.str();
			canvas_pps_accumulated_number_vs_psec_timestamp->cd();
			canvas_pps_accumulated_number_vs_psec_timestamp->Clear();
			graph_pps_accumulated_number_vs_psec_timestamp->SetTitle(ss_pps_accumulated_number_vs_psec_timestamp.str().c_str());
			graph_pps_accumulated_number_vs_psec_timestamp->GetYaxis()->SetTitle("PPS Accumulated Number");
			graph_pps_accumulated_number_vs_psec_timestamp->GetXaxis()->SetTimeDisplay(1);
			graph_pps_accumulated_number_vs_psec_timestamp->GetXaxis()->SetLabelSize(0.03);
			graph_pps_accumulated_number_vs_psec_timestamp->GetXaxis()->SetLabelOffset(0.03);
			graph_pps_accumulated_number_vs_psec_timestamp->GetXaxis()->SetTimeFormat("#splitline{%m/%d}{%H:%M}");
			graph_pps_accumulated_number_vs_psec_timestamp->GetXaxis()->SetTimeOffset(0.);
			graph_pps_accumulated_number_vs_psec_timestamp->Draw("apl");
			std::stringstream ss_pps_accumulated_number_vs_psec_timestamp_path;
			ss_pps_accumulated_number_vs_psec_timestamp_path << outpath << "LAPPDData_TimeEvolution_PPSAccumulatedNumber_vs_PsecTimestamp_" << file_ending << "." << img_extension;
			canvas_pps_accumulated_number_vs_psec_timestamp->SaveAs(ss_pps_accumulated_number_vs_psec_timestamp_path.str().c_str());

			std::stringstream ss_pps_time_vs_accumulated_number;
			ss_pps_time_vs_accumulated_number << "PPS time vs accumulated number time evolution (last " << ss_timeframe.str() << "h) " << end_time.str();
			canvas_pps_time_vs_accumulated_number->cd();
			canvas_pps_time_vs_accumulated_number->Clear();
			graph_pps_time_vs_accumulated_number->SetTitle(ss_pps_time_vs_accumulated_number.str().c_str());
			graph_pps_time_vs_accumulated_number->GetYaxis()->SetTitle("PPS timestamp ns (1e13)");
			graph_pps_time_vs_accumulated_number->GetXaxis()->SetTitle("PPS accumulated number");
			graph_pps_time_vs_accumulated_number->GetYaxis()->SetTimeDisplay(0);
			graph_pps_time_vs_accumulated_number->GetXaxis()->SetTimeDisplay(0);
			graph_pps_time_vs_accumulated_number->GetXaxis()->SetLabelSize(0.03);
			graph_pps_time_vs_accumulated_number->GetXaxis()->SetLabelOffset(0.01);
			graph_pps_time_vs_accumulated_number->GetXaxis()->SetTimeOffset(0.);
			graph_pps_time_vs_accumulated_number->Draw("apl");
			std::stringstream ss_pps_time_vs_accumulated_number_path;
			ss_pps_time_vs_accumulated_number_path << outpath << "LAPPDData_TimeEvolution_PPSTime_vs_AccumulatedNumber_" << file_ending << "." << img_extension;
			canvas_pps_time_vs_accumulated_number->SaveAs(ss_pps_time_vs_accumulated_number_path.str().c_str());

			std::stringstream ss_frame_count;
			ss_frame_count << "Data events time evolution (last " << ss_timeframe.str() << "h) " << end_time.str();
			canvas_frame_count->cd();
			canvas_frame_count->Clear();
			graph_frame_count->SetTitle(ss_frame_count.str().c_str());
			graph_frame_count->GetYaxis()->SetTitle("LAPPD data events");
			graph_frame_count->GetXaxis()->SetTimeDisplay(1);
			graph_frame_count->GetXaxis()->SetLabelSize(0.03);
			graph_frame_count->GetXaxis()->SetLabelOffset(0.03);
			graph_frame_count->GetXaxis()->SetTimeFormat("#splitline{%m/%d}{%H:%M}");
			graph_frame_count->GetXaxis()->SetTimeOffset(0.);
			graph_frame_count->Draw("apl");
			double max_frame_count = 1.;
			double max_frame_count2 = 1.;
			if (framecount_plot.size() > 0)
				max_frame_count2 = TMath::MaxElement(framecount_plot.size(), graph_frame_count->GetY());
			;
			if (max_frame_count2 > max_frame_count)
				max_frame_count = max_frame_count2;
			graph_frame_count->GetYaxis()->SetRangeUser(0.000, 1.1 * max_frame_count);
			std::stringstream ss_frame_count_path;
			ss_frame_count_path << outpath << "LAPPDData_TimeEvolution_DataEvents_" << file_ending << "." << img_extension;
			canvas_frame_count->SaveAs(ss_frame_count_path.str().c_str());
		}

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
		double max_pps2 = 1.;
		if (pps_rate_plot.at(board_nr).size() > 0)
			max_pps2 = TMath::MaxElement(pps_rate_plot.at(board_nr).size(), graph_pps_rate.at(board_nr)->GetY());
		;
		if (max_pps2 > max_pps)
			max_pps = max_pps2;
		// if (pps_rate_plot.at(board_nr).size() > 0)
		//	max_pps = TMath::MaxElement(pps_rate_plot.at(board_nr).size(), graph_pps_rate.at(board_nr)->GetY());
		graph_pps_rate.at(board_nr)->GetYaxis()->SetRangeUser(0.000, 1.1 * max_pps);
		std::stringstream ss_pps_path;
		ss_pps_path << outpath << "LAPPDData_TimeEvolution_PPSRate_Board" << board_nr << "_" << file_ending << "." << img_extension;
		canvas_pps_rate->SaveAs(ss_pps_path.str().c_str());

		// std::cout <<"frame rate"<<std::endl;
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
		double max_frame2 = 1.;
		if (frame_rate_plot.at(board_nr).size() > 0)
			max_frame2 = TMath::MaxElement(frame_rate_plot.at(board_nr).size(), graph_frame_rate.at(board_nr)->GetY());
		if (max_frame2 > max_frame)
			max_frame = max_frame2;
		// if (frame_rate_plot.at(board_nr).size() > 0 && *std::max_element(frame_rate_plot.at(board_nr).begin(),frame_rate_plot.at(board_nr).end())>0)
		// max_frame = TMath::MaxElement(frame_rate_plot.at(board_nr).size(), graph_frame_rate.at(board_nr)->GetY());
		graph_frame_rate.at(board_nr)->GetYaxis()->SetRangeUser(0.000, 1.1 * max_frame);
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
		double max_charge2 = 1.;
		if (int_charge_plot.at(board_nr).size() > 0)
			max_charge2 = TMath::MaxElement(int_charge_plot.at(board_nr).size(), graph_int_charge.at(board_nr)->GetY());
		if (max_charge2 > max_charge)
			max_charge = max_charge2;
		//		if (int_charge_plot.at(board_nr).size() > 0)
		//			max_charge = TMath::MaxElement(int_charge_plot.at(board_nr).size(), graph_int_charge.at(board_nr)->GetY());
		graph_int_charge.at(board_nr)->GetYaxis()->SetRangeUser(0.000, 1.1 * max_charge);
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
		double max_buffer2 = 1.;
		if (buffer_size_plot.at(board_nr).size() > 0)
			max_buffer2 = TMath::MaxElement(buffer_size_plot.at(board_nr).size(), graph_buffer_size.at(board_nr)->GetY());
		if (max_buffer2 > max_buffer)
			max_buffer = max_buffer2;
		// if (buffer_size_plot.at(board_nr).size() > 0)
		//	max_buffer = TMath::MaxElement(buffer_size_plot.at(board_nr).size(), graph_buffer_size.at(board_nr)->GetY());
		graph_buffer_size.at(board_nr)->GetYaxis()->SetRangeUser(0.000, 1.1 * max_buffer);
		std::stringstream ss_buffer_path;
		ss_buffer_path << outpath << "LAPPDData_TimeEvolution_BufferSize_Board" << board_nr << "_" << file_ending << "." << img_extension;
		canvas_buffer_size->SaveAs(ss_buffer_path.str().c_str());

		// std::cout <<"ped, sigma, ped"<<std::endl;
		// Channel-wise rate, ped, sigma plots
		if (board_nr != -1)
		{

			double max_canvas_ped = 0;
			double min_canvas_ped = 9999999.;
			double max_canvas_sigma = 0;
			double min_canvas_sigma = 99999999.;
			double max_canvas_rate = 0;
			double min_canvas_rate = 999999999.;

			int CH_per_CANVAS = 5; // channels per canvas

			for (int i = 0; i < 30; i++)
			{

				int chkey = min_board + i;
				std::stringstream ss_ped, ss_sigma, ss_rate, ss_leg;
				if (i % CH_per_CANVAS == 0 || i == 29)
				{
					if (i != 0)
					{
						ss_ped.str("");
						ss_sigma.str("");
						ss_rate.str("");

						ss_ped << "Board " << board_nr << " Channel " << chkey << " (" << ss_timeframe.str() << "h) " << end_time.str();
						ss_sigma << "Board " << board_nr << " Channel " << chkey << " (" << ss_timeframe.str() << "h) " << end_time.str();
						ss_rate << "Board " << board_nr << " Channel " << chkey << " (" << ss_timeframe.str() << "h) " << end_time.str();
						if (i == 29)
						{
							ss_leg.str("");
							ss_leg << "ch " << chkey;
							multi_ped_lappd->Add(graph_ped.at(chkey));
							leg_ped_lappd->AddEntry(graph_ped.at(chkey), ss_leg.str().c_str(), "l");
							multi_sigma_lappd->Add(graph_sigma.at(chkey));
							leg_sigma_lappd->AddEntry(graph_sigma.at(chkey), ss_leg.str().c_str(), "l");
							multi_rate_lappd->Add(graph_rate.at(chkey));
							leg_rate_lappd->AddEntry(graph_rate.at(chkey), ss_leg.str().c_str(), "l");
						}

						canvas_ped_lappd->cd();
						multi_ped_lappd->Draw("apl");
						multi_ped_lappd->SetTitle(ss_ped.str().c_str());
						multi_ped_lappd->GetYaxis()->SetTitle("Baseline");
						multi_ped_lappd->GetXaxis()->SetTimeDisplay(1);
						multi_ped_lappd->GetXaxis()->SetLabelSize(0.03);
						multi_ped_lappd->GetXaxis()->SetLabelOffset(0.03);
						multi_ped_lappd->GetXaxis()->SetTimeFormat("#splitline{%m/%d}{%H:%M}");
						multi_ped_lappd->GetXaxis()->SetTimeOffset(0.);
						leg_ped_lappd->Draw();

						canvas_sigma_lappd->cd();
						multi_sigma_lappd->Draw("apl");
						multi_sigma_lappd->SetTitle(ss_sigma.str().c_str());
						multi_sigma_lappd->GetYaxis()->SetTitle("Sigma");
						multi_sigma_lappd->GetXaxis()->SetTimeDisplay(1);
						multi_sigma_lappd->GetXaxis()->SetLabelSize(0.03);
						multi_sigma_lappd->GetXaxis()->SetLabelOffset(0.03);
						multi_sigma_lappd->GetXaxis()->SetTimeFormat("#splitline{%m/%d}{%H:%M}");
						multi_sigma_lappd->GetXaxis()->SetTimeOffset(0.);
						leg_sigma_lappd->Draw();

						canvas_rate_lappd->cd();
						multi_rate_lappd->Draw("apl");
						multi_rate_lappd->SetTitle(ss_rate.str().c_str());
						multi_rate_lappd->GetYaxis()->SetTitle("Rate");
						multi_rate_lappd->GetXaxis()->SetTimeDisplay(1);
						multi_rate_lappd->GetXaxis()->SetLabelSize(0.03);
						multi_rate_lappd->GetXaxis()->SetLabelOffset(0.03);
						multi_rate_lappd->GetXaxis()->SetTimeFormat("#splitline{%m/%d}{%H:%M}");
						multi_rate_lappd->GetXaxis()->SetTimeOffset(0.);
						leg_rate_lappd->Draw();

						ss_ped.str("");
						ss_sigma.str("");
						ss_rate.str("");

						ss_ped << outpath << "LAPPDTimeEvolution_Ped_Ch" << chkey << "_" << file_ending << "." << img_extension;
						ss_sigma << outpath << "LAPPDTimeEvolution_Sigma_Ch" << chkey << "_" << file_ending << "." << img_extension;
						ss_rate << outpath << "LAPPDTimeEvolution_Rate_Ch" << chkey << "_" << file_ending << "." << img_extension;

						canvas_ped_lappd->SaveAs(ss_ped.str().c_str());
						canvas_sigma_lappd->SaveAs(ss_sigma.str().c_str());
						canvas_rate_lappd->SaveAs(ss_rate.str().c_str());

						for (int i_gr = 0; i_gr < CH_per_CANVAS; i_gr++)
						{
							int i_balance = (i == 29) ? 1 : 0;
							multi_ped_lappd->RecursiveRemove(graph_ped.at(chkey - CH_per_CANVAS + i_gr + i_balance));
							multi_sigma_lappd->RecursiveRemove(graph_sigma.at(chkey - CH_per_CANVAS + i_gr + i_balance));
							multi_rate_lappd->RecursiveRemove(graph_rate.at(chkey - CH_per_CANVAS + i_gr + i_balance));
						}
					}
					leg_ped_lappd->Clear();
					leg_sigma_lappd->Clear();
					leg_rate_lappd->Clear();

					canvas_ped_lappd->Clear();
					canvas_sigma_lappd->Clear();
					canvas_rate_lappd->Clear();
				}

				if (i != 29)
				{
					ss_leg.str("");
					ss_leg << "ch " << chkey;

					multi_ped_lappd->Add(graph_ped.at(chkey));
					if (graph_ped.at(chkey)->GetMaximum() > max_canvas_ped)
						max_canvas_ped = graph_ped.at(chkey)->GetMaximum();
					if (graph_ped.at(chkey)->GetMinimum() > min_canvas_ped)
						min_canvas_ped = graph_ped.at(chkey)->GetMaximum();
					leg_ped_lappd->AddEntry(graph_ped.at(chkey), ss_leg.str().c_str(), "l");

					multi_sigma_lappd->Add(graph_sigma.at(chkey));
					if (graph_sigma.at(chkey)->GetMaximum() > max_canvas_sigma)
						max_canvas_sigma = graph_sigma.at(chkey)->GetMaximum();
					if (graph_sigma.at(chkey)->GetMinimum() > min_canvas_sigma)
						min_canvas_sigma = graph_sigma.at(chkey)->GetMaximum();
					leg_sigma_lappd->AddEntry(graph_sigma.at(chkey), ss_leg.str().c_str(), "l");

					multi_rate_lappd->Add(graph_rate.at(chkey));
					if (graph_rate.at(chkey)->GetMaximum() > max_canvas_rate)
						max_canvas_rate = graph_rate.at(chkey)->GetMaximum();
					if (graph_rate.at(chkey)->GetMinimum() > min_canvas_rate)
						min_canvas_rate = graph_rate.at(chkey)->GetMaximum();
					leg_rate_lappd->AddEntry(graph_rate.at(chkey), ss_leg.str().c_str(), "l");
				}
			}
		}
	}
	// std::cout <<"End of TimeEvolution"<<std::endl;
}

void MonitorLAPPDData::ProcessLAPPDData()
{

	//--------------------------------------------------------
	//-------------ProcessLAPPDData --------------------------
	//--------------------------------------------------------

	Log("MonitorLAPPDData::ProcessLAPPDData", v_message, verbosity);

	// TODO: Add code to handle PPS Frames once they are available in the data file
	// TODO: Extract pedestal values from 2D ADC Value histogram and also save values in a vector
	// std::vector<std::string> lappdType;
	// long entries = (long) lappdType.size();

	BoostStore *Temp = new BoostStore(false, 2);
	Temp->Initialise("LAPPDTemp");
	long entries;
	Temp->Header->Get("TotalEntries", entries);
	if (verbosity > 2)
		std::cout << "MonitorLAPPDData: Got Temp entries: " << entries << std::endl;

	for (int i_board = 0; i_board < (int)board_configuration.size(); i_board++)
	{

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
		n_data.at(i_board) = 0;
		n_pps.at(i_board) = 0;

		first_beamgate_timestamp.at(i_board) = 0;
		last_beamgate_timestamp.at(i_board) = 0;
		first_timestamp.at(i_board) = 0;
		last_timestamp.at(i_board) = 0;
		first_pps_timestamps.at(i_board) = 0;
		last_pps_timestamps.at(i_board) = 0;
		first_entry.at(i_board) = true;
		first_entry_pps.at(i_board) = true;

		data_beamgate_lastfile.at(board_nr).clear();

		for (size_t i_channel = 0; i_channel < hist_pedestal.at(board_nr).size(); i_channel++)
		{
			// hist_pedestal.at(board_nr).at(i_channel)->Clear();
			hist_pedestal.at(board_nr).at(i_channel)->Reset();
		}

		// ToDo: entries not equal to events
		//		long entries;
		//		LAPPDData->Header->Get("TotalEntries", entries);
		//
		for (int i_entry = 0; i_entry < 100; i_entry++)
		{
			for (size_t i_channel = 0; i_channel < hist_waveforms_onedim.at(i_entry).at(board_nr).size(); i_channel++)
			{
				hist_waveforms_onedim.at(i_entry).at(board_nr).at(i_channel)->Reset();
			}
		}
	}

	beamgate_timestamp.clear();
	data_timestamp.clear();
	num_channels.clear();
	average_buffer.clear();
	board_index.clear();

	current_chkey.clear();
	current_rate.clear();
	current_ped.clear();
	current_sigma.clear();

	raw_lappd_data_pps_counts.clear();
	raw_lappd_data_pps_timestamps.clear();

	/*
		long entries;
		LAPPDData->Header->Get("TotalEntries", entries);
	*/
	/*std::vector<std::map<unsigned long, vector<Waveform<double>>>> lappdDATA;
	std::vector<std::vector<unsigned short>> lappdPPS;
	std::vector<std::vector<unsigned short>> lappdACC;
	std::vector<std::vector<unsigned short>> lappdMETA;

	m_data->CStore.Get("LAPPD_PPS",lappdPPS);
	m_data->CStore.Get("LAPPD_Data",lappdDATA);
	m_data->CStore.Get("LAPPD_ACC",lappdACC);
	m_data->CStore.Get("LAPPD_Meta",lappdMETA);
	m_data->CStore.Get("LAPPD_Type",lappdType);*/

	int entry_data = 0;
	int entry_pps = 0;
	current_pps_count = 0;
	current_frame_count = 0;
	bool have_pps = false;

	for (int i_entry = 0; i_entry < (int)entries; i_entry++)
	{
		if (verbosity > 1)
			std::cout << "MonitorLAPPDData: i_entry: " << i_entry << "/" << entries << std::endl;

		std::map<unsigned long, std::vector<Waveform<double>>> RawLAPPDData;
		std::vector<unsigned short> Metadata;
		std::vector<unsigned short> AccInfoFrame;
		std::vector<unsigned short> pps;
		std::string entry_type;

		Temp->GetEntry(i_entry);

		Temp->Get("Type", entry_type);

		if (entry_type == "Data")
			Temp->Get("Data", RawLAPPDData);
		else if (entry_type == "PPS")
			Temp->Get("PPS", pps);

		Temp->Get("ACC", AccInfoFrame);
		Temp->Get("Meta", Metadata);

		if (verbosity > 1)
			std::cout << "MonitorLAPPDData: entry_type: " << entry_type << std::endl;

		/*LAPPDData->GetEntry(i_entry);
LAPPDData->Get("RawLAPPDData", RawLAPPDData);
LAPPDData->Get("Metadata", Metadata);
LAPPDData->Get("AccInfoFrame", AccInfoFrame);*/

		/*std::string entry_type = lappdType.at(i_entry);
		if (entry_type == "PPS"){
			pps = lappdPPS.at(entry_pps);
			AccInfoFrame = lappdACC.at(entry_pps);
			entry_pps++;
			current_pps_count++;
			have_pps=true;
		} else if (entry_type == "DATA"){
			RawLAPPDData = lappdDATA.at(entry_data);
			AccInfoFrame = lappdACC.at(entry_data);
			Metadata = lappdMETA.at(entry_data);
			entry_data++;
			current_frame_count++;
		}*/

		bool do_continue = false;
		if (entry_type == "PPS")
		{
			std::vector<int>::iterator it;
			int vector_idx = -1;
			current_pps_count++;

			// Parse PPS data according to Data Frame (PPS) metadata
			if (pps.size() == 16 || pps.size() == 32)
			{
				// Parse PPS Timestamp
				// Get words from 2 to 5
				unsigned short pps_63_48 = pps.at(2);
				unsigned short pps_47_32 = pps.at(3);
				unsigned short pps_31_16 = pps.at(4);
				unsigned short pps_15_0 = pps.at(5);
				std::bitset<16> bits_pps_63_48(pps_63_48);
				std::bitset<16> bits_pps_47_32(pps_47_32);
				std::bitset<16> bits_pps_31_16(pps_31_16);
				std::bitset<16> bits_pps_15_0(pps_15_0);

				// Combine all the bits to create PPS timestamp
				unsigned long pps_63_0 = (static_cast<unsigned long>(pps_63_48) << 48) + (static_cast<unsigned long>(pps_47_32) << 32) + (static_cast<unsigned long>(pps_31_16) << 16) + (static_cast<unsigned long>(pps_15_0));
				if (verbosity > 2)
					std::cout << "pps combined: " << pps_63_0 << std::endl;
				std::bitset<64> bits_pps_63_0(pps_63_0);
				last_pps_timestamp = pps_63_0 * (CLOCK_to_SEC * 1000);

				it = std::find(board_configuration.begin(), board_configuration.end(), 0);
				if (it != board_configuration.end())
				{
					vector_idx = std::distance(board_configuration.begin(), it);
					if (verbosity > 2)
						std::cout << "Found board index " << 0 << " at position " << vector_idx << " inside of the vector" << std::endl;
				}
				else
				{
					Log("MonitorLAPPDData: ERROR!!! Board index " + std::to_string(0) + " was not found as one of the configured board index options!!! Abort LAPPD data entry", v_error, verbosity);
					continue;
				}
				if (first_entry_pps.at(vector_idx) == true)
				{
					first_pps_timestamps.at(vector_idx) = last_pps_timestamp;
					first_entry_pps.at(vector_idx) = false;
				}
				last_pps_timestamps.at(vector_idx) = last_pps_timestamp;
				n_pps.at(vector_idx)++;

				// Parse PPS count
				unsigned short pps_count_31_16 = pps.at(8);
				unsigned short pps_count_15_0 = pps.at(9);
				std::bitset<16> bits_pps_count_31_16(pps_count_31_16);
				std::bitset<16> bits_pps_count_15_0(pps_count_15_0);

				// Combine all the bits to create PPS count
				unsigned int pps_count_31_0 = (static_cast<unsigned long>(pps_count_31_16) << 16) + (static_cast<unsigned long>(pps_count_15_0));
				std::bitset<32> bits_pps_count_31_0(pps_count_31_0);
				last_pps_count = pps_count_31_0;

				// Add pps count and timestamp for plotting
				raw_lappd_data_pps_counts.push_back(last_pps_count);
				raw_lappd_data_pps_timestamps.push_back(pps_63_0 * CLOCK_to_NSEC); // Use nanoseconds
			}

			if (pps.size() == 32)
			{
				unsigned short pps_63_48 = pps.at(18);
				unsigned short pps_47_32 = pps.at(19);
				unsigned short pps_31_16 = pps.at(20);
				unsigned short pps_15_0 = pps.at(21);
				std::bitset<16> Bits_pps_63_48(pps_63_48);
				std::bitset<16> Bits_pps_47_32(pps_47_32);
				std::bitset<16> Bits_pps_31_16(pps_31_16);
				std::bitset<16> Bits_pps_15_0(pps_15_0);
				unsigned short pps_63_0 = (static_cast<unsigned long>(pps_63_48) << 48) + (static_cast<unsigned long>(pps_47_32) << 32) + (static_cast<unsigned long>(pps_31_16) << 16) + (static_cast<unsigned long>(pps_15_0));
				if (verbosity > 2)
					std::cout << "pps combined: " << pps_63_0 << std::endl;
				std::bitset<64> Bits_pps_63_0(pps_63_0);
				// std::cout << "bits_pps_63_0: " << Bits_pps_63_0 << std::endl;
				last_pps_timestamp = pps_63_0 * (CLOCK_to_SEC * 1000);

				it = std::find(board_configuration.begin(), board_configuration.end(), 1);
				if (it != board_configuration.end())
				{
					vector_idx = std::distance(board_configuration.begin(), it);
					if (verbosity > 2)
						std::cout << "Found board index " << 1 << " at position " << vector_idx << " inside of the vector" << std::endl;
				}
				else
				{
					Log("MonitorLAPPDData: ERROR!!! Board index " + std::to_string(1) + " was not found as one of the configured board index options!!! Abort LAPPD data entry", v_error, verbosity);
					continue;
				}
				if (first_entry_pps.at(vector_idx) == true)
				{
					first_pps_timestamps.at(vector_idx) = last_pps_timestamp;
					first_entry_pps.at(vector_idx) = false;
				}
				last_pps_timestamps.at(vector_idx) = (last_pps_timestamp);
				n_pps.at(vector_idx)++;
			}
			
			have_pps = true;
			do_continue = true;
		}
		if (do_continue)
			continue;

		if (verbosity > 2)
		{
			std::cout << "******************************" << std::endl;
			std::cout << "Entry: " << i_entry << ", loop through LAPPD Raw data" << std::endl;
		}

		// Only look at raw data if there was a PPS entry at some point
		// if (!have_pps) continue;

		// Check if we have a Board Index entry
		// The entry after the Board Index will always be the startword of chip 0 (0xCA00, or 51712)
		// Create "offset" variable based on position of the Board Index
		unsigned short metadata_0 = Metadata.at(0);
		unsigned short metadata_1 = Metadata.at(1);
		// std::cout <<"metadata_0: "<<metadata_0<<", metadata_1: "<<metadata_1<<std::endl;
		int offset = 0;
		int board_idx = -1;
		if (metadata_0 == 56496 /*51712*/)
			offset = 1;
		else if (metadata_1 == 56496 /*51712*/)
		{
			offset = 0;
			board_idx = metadata_0;
		}

		if (board_idx != -1)
		{
			if (verbosity > 3)
				std::cout << "Metadata Board #: " << board_idx << std::endl;
			std::bitset<16> bits_metadata(board_idx);
			if (verbosity > 3)
				std::cout << "Metadata Bits: " << bits_metadata << std::endl;
		}
		else
		{
			Log("MonitorLAPPDData: ERROR!!! Found no board index in the data!", v_error, verbosity);
		}

		board_index.push_back(board_idx);

		// Look for board index in the board_configuration vector
		int vector_idx = -1;
		std::vector<int>::iterator it = std::find(board_configuration.begin(), board_configuration.end(), board_idx);
		if (it != board_configuration.end())
		{
			vector_idx = std::distance(board_configuration.begin(), it);
			if (verbosity > 2)
				std::cout << "Found board index " << board_idx << " at position " << vector_idx << " inside of the vector" << std::endl;
		}
		else
		{
			Log("MonitorLAPPDData: ERROR!!! Board index " + std::to_string(board_idx) + " was not found as one of the configured board index options!!! Abort LAPPD data entry", v_error, verbosity);
			continue;
		}
		int min_board = board_channel.at(vector_idx);

		if (vector_idx == 0)
			current_frame_count++; // Only increase event counter once per event, not once per board

		// Build beamgate timestamp
		unsigned short beamgate_63_48 = Metadata.at(7 - offset);  // Shift everything by 1 for the test file
		unsigned short beamgate_47_32 = Metadata.at(27 - offset); // Shift everything by 1 for the test file
		unsigned short beamgate_31_16 = Metadata.at(47 - offset); // Shift everything by 1 for the test file
		unsigned short beamgate_15_0 = Metadata.at(67 - offset);  // Shift everything by 1 for the test file
		std::bitset<16> bits_beamgate_63_48(beamgate_63_48);
		std::bitset<16> bits_beamgate_47_32(beamgate_47_32);
		std::bitset<16> bits_beamgate_31_16(beamgate_31_16);
		std::bitset<16> bits_beamgate_15_0(beamgate_15_0);
		if (verbosity > 2)
			std::cout << "bits_beamgate_63_48: " << bits_beamgate_63_48 << std::endl;
		if (verbosity > 2)
			std::cout << "bits_beamgate_47_32: " << bits_beamgate_47_32 << std::endl;
		if (verbosity > 2)
			std::cout << "bits_beamgate_31_16: " << bits_beamgate_31_16 << std::endl;
		if (verbosity > 2)
			std::cout << "bits_beamgate_15_0: " << bits_beamgate_15_0 << std::endl;
		unsigned long beamgate_63_0 = (static_cast<unsigned long>(beamgate_63_48) << 48) + (static_cast<unsigned long>(beamgate_47_32) << 32) + (static_cast<unsigned long>(beamgate_31_16) << 16) + (static_cast<unsigned long>(beamgate_15_0));
		if (verbosity > 2)
			std::cout << "beamgate combined: " << beamgate_63_0 << std::endl;
		std::bitset<64> bits_beamgate_63_0(beamgate_63_0);
		if (verbosity > 2)
			std::cout << "bits_beamgate_63_0: " << bits_beamgate_63_0 << std::endl;

		std::stringstream str_beamgate_15_0;
		str_beamgate_15_0 << std::hex << (beamgate_15_0);
		std::stringstream str_beamgate_31_16;
		str_beamgate_31_16 << std::hex << (beamgate_31_16);
		std::stringstream str_beamgate_47_32;
		str_beamgate_47_32 << std::hex << (beamgate_47_32);
		std::stringstream str_beamgate_63_48;
		str_beamgate_63_48 << std::hex << (beamgate_63_48);
		const char *hexstring = str_beamgate_63_48.str().c_str();
		unsigned int meta7_1 = (unsigned int)strtol(hexstring, NULL, 16);
		hexstring = str_beamgate_47_32.str().c_str();
		unsigned int meta27_1 = (unsigned int)strtol(hexstring, NULL, 16);
		hexstring = str_beamgate_31_16.str().c_str();
		unsigned int meta47_1 = (unsigned int)strtol(hexstring, NULL, 16);
		hexstring = str_beamgate_15_0.str().c_str();
		unsigned int meta67_1 = (unsigned int)strtol(hexstring, NULL, 16);
		meta7_1 = meta7_1 << 16;
		meta47_1 = meta47_1 << 16;

		unsigned int beamcounter = meta47_1 + meta67_1;
		unsigned int beamcounterL = meta7_1 + meta27_1;

		double largetime = (double)beamcounterL * 13.1;
		double smalltime = ((double)beamcounter / 1E9) * 3.125;

		if (verbosity > 2)
		{
			std::cout << "meta7_1: " << meta7_1 << std::endl;
			std::cout << "meta27_1: " << meta27_1 << std::endl;
			std::cout << "meta47_1: " << meta47_1 << std::endl;
			std::cout << "meta67_1: " << meta67_1 << std::endl;
			std::cout << "beamcounter: " << beamcounter << std::endl;
			std::cout << "beamcounterL: " << beamcounterL << std::endl;
			std::cout << "largetime: " << largetime << std::endl;
			std::cout << "smalltime: " << smalltime << std::endl;
			std::cout << "eventtime: " << (largetime + smalltime) << std::endl;
			std::cout << "beamgate old: " << ((beamgate_63_0 / 1E9) * 3.125) << std::endl;
		}

		// Build data timestamp
		unsigned short timestamp_63_48 = Metadata.at(70 - offset); // Shift everything by 1 for the test file
		unsigned short timestamp_47_32 = Metadata.at(50 - offset); // Shift everything by 1 for the test file
		unsigned short timestamp_31_16 = Metadata.at(30 - offset); // Shift everything by 1 for the test file
		unsigned short timestamp_15_0 = Metadata.at(10 - offset);  // Shift everything by 1 for the test file
		std::bitset<16> bits_timestamp_63_48(timestamp_63_48);
		std::bitset<16> bits_timestamp_47_32(timestamp_47_32);
		std::bitset<16> bits_timestamp_31_16(timestamp_31_16);
		std::bitset<16> bits_timestamp_15_0(timestamp_15_0);
		// std::cout << "bits_timestamp_63_48: " << bits_timestamp_63_48 << std::endl;
		// std::cout << "bits_timestamp_47_32: " << bits_timestamp_47_32 << std::endl;
		// std::cout << "bits_timestamp_31_16: " << bits_timestamp_31_16 << std::endl;
		// std::cout << "bits_timestamp_15_0: " << bits_timestamp_15_0 << std::endl;
		unsigned long timestamp_63_0 = (static_cast<unsigned long>(timestamp_63_48) << 48) + (static_cast<unsigned long>(timestamp_47_32) << 32) + (static_cast<unsigned long>(timestamp_31_16) << 16) + (static_cast<unsigned long>(timestamp_15_0));
		if (verbosity > 2)
			std::cout << "timestamp combined: " << timestamp_63_0 << std::endl;
		std::bitset<64> bits_timestamp_63_0(timestamp_63_0);
		// std::cout << "bits_timestamp_63_0: " << bits_timestamp_63_0 << std::endl;
		beamgate_timestamp.push_back(beamgate_63_0);
		data_timestamp.push_back(timestamp_63_0);

		// for (int i=0; i<first_entry.size(); i++) std::cout << first_entry.at(i)<<std::endl;

		data_beamgate_lastfile.at(board_idx).push_back(timestamp_63_0 - beamgate_63_0);
		if (first_entry.at(vector_idx) == true)
		{
			// std::cout <<"true"<<std::endl;
			first_beamgate_timestamp.at(vector_idx) = beamgate_63_0;
			first_timestamp.at(vector_idx) = timestamp_63_0;
			first_entry.at(vector_idx) = false;
		}
		// Just overwrite "last" entry every time
		last_beamgate_timestamp.at(vector_idx) = beamgate_63_0;
		last_timestamp.at(vector_idx) = timestamp_63_0;

		if (verbosity > 3)
			std::cout << "vector_idx: " << vector_idx << ", first_timestamp: " << first_timestamp.at(vector_idx) << ", last_timestamp: " << last_timestamp.at(vector_idx) << std::endl;

		// Loop through LAPPD data
		if (verbosity > 2)
			std::cout << "Loop through LAPPD Data" << std::endl;
		int n_channels = 0;
		double buffer_size = 0;
		n_data.at(vector_idx)++;
		for (std::map<unsigned long, std::vector<Waveform<double>>>::iterator it = RawLAPPDData.begin(); it != RawLAPPDData.end(); it++)
		{
			if (verbosity > 4)
				std::cout << "Got channel " << it->first << std::endl;
			n_channels++;
			std::vector<Waveform<double>> waveforms = it->second;
			for (int i_vec = 0; i_vec < (int)waveforms.size(); i_vec++)
			{
				// std::cout <<"i_vec: "<<i_vec<<std::endl;
				// std::cout <<"board_idx: "<<board_idx<<std::endl;
				std::vector<double> waveform = *(waveforms.at(i_vec).GetSamples());
				hist_buffer_channel.at(board_idx)->Fill(waveform.size(), it->first);
				hist_buffer.at(board_idx)->Fill(waveform.size());
				current_buffer_size.at(vector_idx) += waveform.size();
				buffer_size += waveform.size();
				n_buffer.at(vector_idx)++;
				// std::cout <<"waveform.size(): "<<waveform.size()<<std::endl;
				for (int i_wave = 0; i_wave < (int)waveform.size(); i_wave++)
				{
					// std::cout <<"hist_adc_channel"<<std::endl;
					hist_adc_channel.at(board_idx)->Fill(waveform.at(i_wave), it->first);
					// std::cout <<"hist_waveform_channel"<<std::endl;
					hist_waveform_channel.at(board_idx)->SetBinContent(i_wave + 1, (it->first) % 30 + 1, hist_waveform_channel.at(board_idx)->GetBinContent(i_wave + 1, (it->first) % 30 + 1) + waveform.at(i_wave));
					// std::cout <<"hist_waveform_voltages"<<std::endl;
					hist_waveform_voltages.at(board_idx)->Fill(i_wave, it->first, waveform.at(i_wave));
					// std::cout <<"hist_waveforms_onedim"<<std::endl;
					// std::cout <<i_entry<<"/"<<board_idx<<"/"<<it->first-min_board<<std::endl;
					// std::cout <<"hist_waveforms_onedim.size(): "<<hist_waveforms_onedim.size()<<std::endl;
					// std::cout <<"hist_waveforms_onedim.at(i_entry).size(): "<<hist_waveforms_onedim.at(i_entry).size()<<std::endl;
					// std::cout <<"hist_waveforms_onedim.at(i_entry).at(board_idx).size(): "<<hist_waveforms_onedim.at(i_entry).at(board_idx).size()<<std::endl;
					if (i_entry < 100)
						hist_waveforms_onedim.at(i_entry).at(board_idx).at(it->first - min_board)->Fill(i_wave, waveform.at(i_wave));
					// std::cout <<"hist_pedestal"<<std::endl;
					hist_pedestal.at(board_idx).at(it->first - min_board)->Fill(waveform.at(i_wave));
				}
			}
		}
		if (verbosity > 2)
			std::cout << "*******************************" << std::endl;
		num_channels.push_back(n_channels);
		if (n_channels != 0)
			buffer_size /= n_channels;
		average_buffer.push_back(buffer_size);
	} // end entry loop

	// Check whether file time has already been synced
	std::time_t current_filetime;
	m_data->CStore.Get("CurrentFileTime", current_filetime);
	boost::posix_time::ptime reference_new = boost::posix_time::from_time_t(current_filetime);
	boost::posix_time::time_duration reference_stamp_new = boost::posix_time::time_duration(reference_new - *Epoch);

	// Sync reference time every Execute step since it will be reset if the LAPPD is restarted
	sync_reference_time = false;
	if (!sync_reference_time)
	{
		if (have_pps)
		{
			reference_time = reference_stamp_new.total_milliseconds();
			reference_time -= last_pps_timestamps.at(0);
			sync_reference_time = true;
		}
	}
	else
	{
		if (have_pps)
		{
			ULong64_t reference_time_temp = reference_stamp_new.total_milliseconds();
			reference_time_temp -= last_pps_timestamps.at(0);
			double diff = double(reference_time_temp) - double(reference_time);
			if (verbosity > 2)
				std::cout << "MonitorLAPPDData: Synced time offset for PPS timestamps: " << diff << " milliseconds" << std::endl;
		}
	}
	t_file_end.clear();

	std::string rawfilename;
	m_data->CStore.Get("CurrentFileName", rawfilename);
	this->GetRunSubPart(rawfilename);

	bool got_tfile_start = false;
	for (int i_board = 0; i_board < (int)board_configuration.size(); i_board++)
	{
		int board_nr = board_configuration.at(i_board);
		ModifyBeamgateData(5, board_nr, data_beamgate_last5files);
		ModifyBeamgateData(10, board_nr, data_beamgate_last10files);
		ModifyBeamgateData(20, board_nr, data_beamgate_last20files);
		ModifyBeamgateData(100, board_nr, data_beamgate_last100files);
		ModifyBeamgateData(1000, board_nr, data_beamgate_last1000files);
		if (entries > 0)
			hist_waveform_channel.at(board_nr)->Scale(1. / entries);
		double diff_timestamps = (last_timestamp.at(i_board) - first_timestamp.at(i_board)) * CLOCK_to_SEC;
		double diff_timestamps_beam = (last_beamgate_timestamp.at(i_board) - first_beamgate_timestamp.at(i_board)) * CLOCK_to_SEC;
		double diff_timestamps_pps = (last_pps_timestamps.at(i_board) - first_pps_timestamps.at(i_board)) / 1000.;
		if (verbosity > 3)
			std::cout << "first_pps_timestamp: " << first_pps_timestamps.at(i_board) << ", last_pps_timestamp: " << last_pps_timestamps.at(i_board) << ", diff: " << diff_timestamps_pps << std::endl;
		if (verbosity > 3)
			std::cout << "n_pps: " << n_pps.at(i_board) << ", rate: " << (n_pps.at(i_board) / diff_timestamps_pps) << std::endl;
		if (verbosity > 3)
			std::cout << "i_board: " << i_board << ", first_timestamp: " << first_timestamp.at(i_board) << ", last_timestamp: " << last_timestamp.at(i_board) << ", diff_timestamps: " << diff_timestamps << std::endl;
		if (diff_timestamps > 0.)
			current_frame_rate.at(i_board) = n_data.at(i_board) / diff_timestamps_pps; // Need to convert difference of timestamps into seconds or something similar
		if (diff_timestamps_beam > 0.)
			current_beamgate_rate.at(i_board) = n_data.at(i_board) / diff_timestamps_beam; // Need to convert difference of timestamps into seconds or something similar
		if (n_buffer.at(i_board) != 0)
			current_buffer_size.at(i_board) /= n_buffer.at(i_board);
		if (diff_timestamps_pps > 0.)
			current_pps_rate.at(i_board) = (n_pps.at(i_board)) / diff_timestamps_pps;

		// Convert timestamps to msec
		// TODO: Need to add absolute time reference from PPS signal to get milliseconds since 1970/1/1
		first_timestamp.at(i_board) *= (CLOCK_to_SEC * 1000);
		last_timestamp.at(i_board) *= (CLOCK_to_SEC * 1000);
		first_beamgate_timestamp.at(i_board) *= (CLOCK_to_SEC * 1000);
		last_beamgate_timestamp.at(i_board) *= (CLOCK_to_SEC * 1000);

		// Add reference time offset
		first_timestamp.at(i_board) += reference_time;
		last_timestamp.at(i_board) += reference_time;
		first_beamgate_timestamp.at(i_board) += reference_time;
		last_beamgate_timestamp.at(i_board) += reference_time;

		if (!got_tfile_start && first_timestamp.at(i_board) > 0)
		{
			t_file_start = first_timestamp.at(i_board);
			if (board_nr != -1)
				got_tfile_start = true;
		}

		t_file_end.push_back(last_timestamp.at(i_board));

		PedestalFits(board_nr, i_board);
		int min_board = board_channel.at(i_board);
		if (board_nr != -1)
		{
			for (int i = 0; i < 30; i++)
			{
				current_chkey.push_back(min_board + i);
				current_ped.push_back(mean_pedestal.at(board_nr).at(i));
				current_sigma.push_back(sigma_pedestal.at(board_nr).at(i));
				current_rate.push_back(rate_pedestal.at(board_nr).at(i));
			}
		}
	}

	Temp->Close();
	delete Temp;
}

void MonitorLAPPDData::ModifyBeamgateData(size_t numberOfFiles, int boardNumber, std::map<int, std::vector<std::vector<uint64_t>>> &dataVector)
{
	if (dataVector.at(boardNumber).size())
	{
		if (dataVector.at(boardNumber).size() == numberOfFiles)
		{
			for (size_t i_file = 0; i_file < dataVector.at(boardNumber).size() - 1; i_file++)
			{
				dataVector.at(boardNumber).at(i_file) = dataVector.at(boardNumber).at(i_file + 1);
			}
			dataVector.at(boardNumber).at(numberOfFiles - 1) = data_beamgate_lastfile.at(boardNumber);
		}
		else
		{
			dataVector.at(boardNumber).push_back(data_beamgate_lastfile.at(boardNumber));
		}
	}
	else
	{
		if (data_beamgate_lastfile.at(boardNumber).size())
		{
			dataVector.at(boardNumber).push_back(data_beamgate_lastfile.at(boardNumber));
		}
	}
}

void MonitorLAPPDData::DrawFileHistoryLAPPD(ULong64_t timestamp_end, double time_frame, std::string file_ending, int _linewidth)
{
	Log("MonitorLAPPDData: DrawFileHistoryLAPPD", v_message, verbosity);

	//------------------------------------------------------------
	//------------------DrawFileHistoryLAPPD ---------------------
	//------------------------------------------------------------

	// Creates a plot showing the time stamps for all the files within the last time_frame mins
	// The plot is updated with the update_frequency specified in the configuration file (default: 5 mins)

	if (timestamp_end != readfromfile_tend || time_frame != readfromfile_timeframe)
		ReadFromFile(timestamp_end, time_frame);

	timestamp_end += utc_to_t;

	ULong64_t timestamp_start = timestamp_end - time_frame * MSEC_to_SEC * SEC_to_MIN * MIN_to_HOUR;
	std::stringstream ss_timeframe;
	ss_timeframe << round(time_frame * 100.) / 100.;

	canvas_logfile_lappd->cd();
	log_files_lappd->SetBins(num_history_lappd, timestamp_start / MSEC_to_SEC, timestamp_end / MSEC_to_SEC);
	log_files_lappd->GetXaxis()->SetTimeOffset(0.);
	log_files_lappd->Draw();

	std::stringstream ss_title_filehistory;
	ss_title_filehistory << "LAPPD Files History (last " << ss_timeframe.str() << "h)";

	log_files_lappd->SetTitle(ss_title_filehistory.str().c_str());

	std::vector<TLine *> file_markers;
	for (int i_board = 0; i_board < (int)board_configuration.size(); i_board++)
	{
		int board_nr = board_configuration.at(i_board);
		std::vector<ULong64_t> tend_plot = data_times_end_plot.at(board_nr);
		for (unsigned int i_file = 0; i_file < tend_plot.size(); i_file++)
		{
			TLine *line_file = new TLine((tend_plot.at(i_file) + utc_to_t) / MSEC_to_SEC, 0., (tend_plot.at(i_file) + utc_to_t) / MSEC_to_SEC, 1.);
			line_file->SetLineColor(1);
			line_file->SetLineStyle(1);
			line_file->SetLineWidth(_linewidth);
			line_file->Draw("same");
			file_markers.push_back(line_file);
		}
	}

	std::stringstream ss_logfiles;
	ss_logfiles << outpath << "LAPPD_FileHistory_" << file_ending << "." << img_extension;
	canvas_logfile_lappd->SaveAs(ss_logfiles.str().c_str());

	for (unsigned int i_line = 0; i_line < file_markers.size(); i_line++)
	{
		delete file_markers.at(i_line);
	}

	log_files_lappd->Reset();
	canvas_logfile_lappd->Clear();
}

void MonitorLAPPDData::PrintFileTimeStampLAPPD(ULong64_t timestamp_end, double time_frame, std::string file_ending)
{

	if (verbosity > 2)
		std::cout << "MonitorLAPPDData: PrintFileTimeStampLAPPD" << std::endl;

	//------------------------------------------------------------
	//-----------------PrintFileTimeStampLAPPD--------------------
	//------------------------------------------------------------

	if (timestamp_end != readfromfile_tend || time_frame != readfromfile_timeframe)
		ReadFromFile(timestamp_end, time_frame);

	boost::posix_time::ptime endtime = *Epoch + boost::posix_time::time_duration(int(timestamp_end / MSEC_to_SEC / SEC_to_MIN / MIN_to_HOUR), int(timestamp_end / MSEC_to_SEC / SEC_to_MIN) % 60, int(timestamp_end / MSEC_to_SEC / 1000.) % 60, timestamp_end % 1000);
	struct tm endtime_tm = boost::posix_time::to_tm(endtime);
	std::stringstream end_time;
	end_time << "Current time: " << endtime_tm.tm_year + 1900 << "/" << endtime_tm.tm_mon + 1 << "/" << endtime_tm.tm_mday << "-" << endtime_tm.tm_hour << ":" << endtime_tm.tm_min << ":" << endtime_tm.tm_sec;

	TText *label_lastfile = nullptr;
	label_lastfile = new TText(0.04, 0.9, end_time.str().c_str());
	label_lastfile->SetNDC(1);
	label_lastfile->SetTextSize(0.05);

	// TLatex *label_timediff = nullptr;

	std::vector<TLatex *> vec_label_timediff;

	for (int i_board = 0; i_board < (int)board_configuration.size(); i_board++)
	{
		int board_nr = board_configuration.at(i_board);
		std::vector<ULong64_t> tend_plot = data_times_end_plot.at(board_nr);
		if (tend_plot.size() == 0)
		{
			std::stringstream time_diff;
			time_diff << "#Delta t Last LAPPD File (Board " << board_nr << "): >" << time_frame << "h";
			TLatex *label_timediff = new TLatex(0.04, 0.7 - 0.2 * (i_board), time_diff.str().c_str());
			label_timediff->SetNDC(1);
			label_timediff->SetTextSize(0.05);
			vec_label_timediff.push_back(label_timediff);
		}
		else
		{
			ULong64_t timestamp_lastfile = tend_plot.at(tend_plot.size() - 1);
			boost::posix_time::ptime filetime = *Epoch + boost::posix_time::time_duration(int(timestamp_lastfile / MSEC_to_SEC / SEC_to_MIN / MIN_to_HOUR), int(timestamp_lastfile / MSEC_to_SEC / SEC_to_MIN) % 60, int(timestamp_lastfile / MSEC_to_SEC / 1000.) % 60, timestamp_lastfile % 1000);
			boost::posix_time::time_duration t_since_file = boost::posix_time::time_duration(endtime - filetime);
			int t_since_file_min = int(t_since_file.total_milliseconds() / MSEC_to_SEC / SEC_to_MIN);
			std::stringstream time_diff;
			time_diff << "#Delta t Last LAPPD File (Board " << board_nr << "): " << t_since_file_min / 60 << "h:" << t_since_file_min % 60 << "min";
			TLatex *label_timediff = new TLatex(0.04, 0.7 - 0.2 * (i_board), time_diff.str().c_str());
			label_timediff->SetNDC(1);
			label_timediff->SetTextSize(0.05);
			vec_label_timediff.push_back(label_timediff);
		}
	}

	canvas_file_timestamp_lappd->cd();
	label_lastfile->Draw();
	for (int i_vec = 0; i_vec < (int)vec_label_timediff.size(); i_vec++)
	{
		vec_label_timediff.at(i_vec)->Draw();
	}
	std::stringstream ss_file_timestamp;
	ss_file_timestamp << outpath << "LAPPD_FileTimeStamp_" << file_ending << "." << img_extension;
	canvas_file_timestamp_lappd->SaveAs(ss_file_timestamp.str().c_str());

	delete label_lastfile;
	for (int i_vec = 0; i_vec < (int)vec_label_timediff.size(); i_vec++)
	{
		delete vec_label_timediff.at(i_vec);
	}

	canvas_file_timestamp_lappd->Clear();
}

void MonitorLAPPDData::GetRunSubPart(std::string filename)
{

	std::cout << "filename: " << filename << std::endl;
	int p1 = filename.find_last_of("R", filename.size());
	int p2 = filename.find("S");
	int p3 = filename.find_last_of("p", filename.size());
	// int p3 = filename.find("p");
	int p4 = filename.size();
	std::cout << p1 << "," << p2 << "," << p3 << "," << p4 << std::endl;
	std::string runnumber = filename.substr(p1 + 1, p2 - p1 - 1);
	std::string subrunnumber = filename.substr(p2 + 1, p3 - p2 - 1);
	std::string partrunnumber = filename.substr(p3 + 1, p4 - p3 - 1);
	current_run = std::stoi(runnumber);
	current_subrun = std::stoi(subrunnumber);
	current_partrun = std::stoi(partrunnumber);
	std::cout << "extracted run: " << current_run << ", subrun: " << current_subrun << ", part: " << current_partrun << std::endl;
}
