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
#include "TH2Poly.h"
#include "TList.h"
#include "TPaletteAxis.h"
#include "MRDOut.h"
#include "TObjectTable.h"
#include "TROOT.h"
#include "TColor.h"

#include "Detector.h"
#include "Paddle.h"

#include <boost/date_time/posix_time/posix_time.hpp>


class MonitorMRDEventDisplay: public Tool {


	public:

		MonitorMRDEventDisplay();
		bool Initialise(std::string configfile,DataModel &data);
		bool Execute();
		bool Finalise();

		void PlotMRDEvent();


	private:

		BoostStore *CCData;
		std::string outpath;
		std::string active_slots;
		std::string inactive_channels;
		BoostStore* MRDdata;
		MRDOut MRDout;
		int verbosity;

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
		time_t t;
		std::stringstream title_time; 

		//geometry conversion table
		Geometry *geom = nullptr;
		std::map<std::vector<int>,int>* CrateSpaceToChannelNumMap = nullptr;
		TH2Poly *hist_mrd_top = nullptr;
		TH2Poly *hist_mrd_side = nullptr;
		double enlargeBoxes = 0.01;
		double shiftSecRow = 0.05;
		double tank_center_x, tank_center_y, tank_center_z;


		//set color palette
		const int n_colors=255;
		double alpha=1.;    //make colors opaque, not transparent
		Int_t Bird_palette[255];
		Int_t Bird_Idx;
		Double_t stops[9] = { 0.0000, 0.1250, 0.2500, 0.3750, 0.5000, 0.6250, 0.7500, 0.8750, 1.0000};
		Double_t red[9]   = { 0.2082, 0.0592, 0.0780, 0.0232, 0.1802, 0.5301, 0.8186, 0.9956, 0.9764};
		Double_t green[9] = { 0.1664, 0.3599, 0.5041, 0.6419, 0.7178, 0.7492, 0.7328, 0.7862, 0.9832};
		Double_t blue[9]  = { 0.5293, 0.8684, 0.8385, 0.7914, 0.6425, 0.4662, 0.3499, 0.1968, 0.0539};


};


#endif
