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
#include "TFile.h"

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
		std::string outpath_temp;
		int custom_range;
		int custom_tdc_min;
		int custom_tdc_max;
		BoostStore* MRDdata;
		MRDOut MRDout;
		std::string output_format;
		std::string output_file;
		int evnum;
		int verbosity;

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
		TH2Poly *hist_facc = nullptr;
		double enlargeBoxes = 0.01;
		double shiftSecRow = 0.04;
		double tank_center_x, tank_center_y, tank_center_z;
		TCanvas *canvas_mrd_evdisplay = nullptr;
		TCanvas *canvas_facc_evdisplay = nullptr;

		//set color palette
		const int n_colors=255;
		double alpha=1.;    //make colors opaque, not transparent
		Int_t Bird_palette[255];
		Int_t Bird_Idx;
		Double_t stops[9] = { 0.0000, 0.1250, 0.2500, 0.3750, 0.5000, 0.6250, 0.7500, 0.8750, 1.0000};
		Double_t red[9]   = { 0.2082, 0.0592, 0.0780, 0.0232, 0.1802, 0.5301, 0.8186, 0.9956, 0.9764};
		Double_t green[9] = { 0.1664, 0.3599, 0.5041, 0.6419, 0.7178, 0.7492, 0.7328, 0.7862, 0.9832};
		Double_t blue[9]  = { 0.5293, 0.8684, 0.8385, 0.7914, 0.6425, 0.4662, 0.3499, 0.1968, 0.0539};

		TFile *rootfile = nullptr;


};


#endif
