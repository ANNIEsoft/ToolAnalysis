#ifndef MonitorMRDEventDisplay_H
#define MonitorMRDEventDisplay_H

#include <string>
#include <iostream>
#include <vector>

#include "Tool.h"

#include "TObjectTable.h"

#include "TCanvas.h"
#include "TPad.h"
#include "TLegend.h"
#include "TH1F.h"
#include "TH2F.h"
#include "TList.h"
#include "TPaletteAxis.h"
#include "MRDOut.h"
#include "TObjectTable.h"


#include <boost/date_time/posix_time/posix_time.hpp>


class MonitorMRDEventDisplay: public Tool {


	public:

		MonitorMRDEventDisplay();
		bool Initialise(std::string configfile,DataModel &data);
		bool Execute();
		bool Finalise();

		void InitializeVectors();
		void EraseOldData();
		void UpdateMonitorPlots();


	private:

		BoostStore *CCData;
		std::string outpath;
		std::string active_slots;
		std::string inactive_channels;
		BoostStore* MRDdata;
		MRDOut MRDout;
		int verbosity;

		boost::posix_time::ptime current;

		static const int num_crates = 2;
		static const int num_slots = 24;
		static const int num_channels = 32;
		static const int min_crate = 7;
		int num_active_slots;
		int num_active_slots_cr1, num_active_slots_cr2;
		int active_channel[2][24]={{0}};
		std::vector<int> active_slots_cr1;
		std::vector<int> active_slots_cr2;
		std::vector<int> nr_slot;
		std::vector<int> inactive_ch_crate1, inactive_slot_crate1;
		std::vector<int> inactive_ch_crate2, inactive_slot_crate2;

		//MRD store includes the following variables
		unsigned int OutN, Trigger;
		std::vector<unsigned int> Value, Slot, Channel, Crate;
		std::vector<std::string> Type;
		ULong64_t TimeStamp;
		long current_stamp;

		//time variables
		boost::posix_time::time_duration period_update;
		boost::posix_time::time_duration duration;
		boost::posix_time::ptime last;
		time_t t;
  		std::stringstream title_time; 
  		double integration_period = 5.; 	//5 minutes integration for rate calculation?

  		//storing variables
		std::vector<std::vector<unsigned int> > live_tdc, vector_tdc;
		std::vector<unsigned int> vector_nchannels, vector_nslots;
		std::vector<unsigned int> tdc_paddle, tdc_paddle_hour;
		std::vector<std::vector<ULong64_t> > live_timestamp;
		std::vector<ULong64_t> vector_timestamp;

		//plot variables
		TH2F *rate_crate1;
		TH2F *rate_crate2;
		TH1F *TDC_hist;
		TH1F *TDC_hist_coincidence;
		TH1F *n_paddles_hit;
		TH1F *n_paddles_hit_coincidence;
		double min_ch, max_ch;
		bool update_plots;




};


#endif
