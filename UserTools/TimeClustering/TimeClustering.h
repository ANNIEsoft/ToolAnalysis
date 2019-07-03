#ifndef TimeClustering_H
#define TimeClustering_H

#include <string>
#include <iostream>

#include "Tool.h"

class TH1D;
class TApplication;
class TCanvas;

/**
* \class TimeClustering
*
* This is a balnk template for a Tool used by the script to generate a new custom tool. Please fill out the descripton and author information.
*
* $Author: B.Richards $
* $Date: 2019/05/28 10:44:00 $
* Contact: b.richards@qmul.ac.uk
*/
class TimeClustering: public Tool {
	
	public:
	TimeClustering(); ///< Simple constructor
	bool Initialise(std::string configfile,DataModel &data); ///< Initialise Function for setting up Tool resorces. @param configfile The path and name of the dynamic configuration file to read in. @param data A reference to the transient data class used to pass information between Tools.
	bool Execute(); ///< Executre function used to perform Tool perpose. 
	bool Finalise(); ///< Finalise funciton used to clean up resorces.
	
	private:
	int minimumdigits=4;                        // a cluster must have at least 4 hits
	double maxsubeventduration=30;              // if all hits within this time, just one subevent
	double minimum_subevent_timeseparation=30;  // minimum empty time to delimit subevents
	
	std::vector<double> mrddigittimesthisevent;
	std::vector<int> mrddigitpmtsthisevent;
	std::vector<double> mrddigitchargesthisevent;
	
	std::vector<std::vector<int>> MrdTimeClusters;
	
	std::map<unsigned long,vector<MCHit>>* TDCData;
	std::map<unsigned long,vector<MCHit>>* MCHits;
	
	bool MakeMrdDigitTimePlot=false;
	TH1D* mrddigitts=nullptr;
	TCanvas* timeClusterCanvas=nullptr;
	TApplication* rootTApp=nullptr;
	double canvwidth;
	double canvheight;
	
	// verbosity levels: if 'verbosity' < this level, the message type will be logged.
	int verbosity;
	int v_error=0;
	int v_warning=1;
	int v_message=2;
	int v_debug=3;
	std::string logmessage;
	int get_ok;
};


#endif
