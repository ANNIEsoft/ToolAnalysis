/* vim:set noexpandtab tabstop=4 wrap */
#ifndef MrdDiscriminatorScan_H
#define MrdDiscriminatorScan_H

#include <string>
#include <iostream>
#include <map>

#include "Tool.h"
class TApplication;
class TCanvas;
class TGraphErrors;

class MrdDiscriminatorScan: public Tool {
	
	public:
	
	MrdDiscriminatorScan();
	bool Initialise(std::string configfile,DataModel &data);
	bool Execute();
	bool Finalise();
	
	private:
	
	int verbosity=1;
	bool drawHistos;
	std::string plotDirectory;
	std::string filelist; // name of a file listing the names of the mrd files
	std::string filedir;  // directory of the files
	std::string filepath; // full path to next mrd file
	int filei;            // index of file we're reading
	std::ifstream fin;    // file reader
	std::string x_variable; // "threshold" for threshold, or "time" for relative time
	
	bool GetNextFile();
	bool CountChannelHits(BoostStore* MRDData, std::map<int,std::map<int,std::map<int,int>>> &hit_counts_on_channels, TimeClass& first_timestamp, TimeClass& last_timestamp);
	unsigned long current_threshold;
	unsigned long very_first_timestamp=0;
	
	// TApplication for making histograms
	TApplication* rootTApp=nullptr;
	TCanvas* mrdScanCanv=nullptr;
	std::map<int,std::map<int,std::map<int,TGraphErrors*>>> rategraphs;
	
	// verbosity levels: if 'verbosity' < this level, the message type will be logged.
	int get_ok;
	int v_error=0;
	int v_warning=1;
	int v_message=2;
	int v_debug=3;
	std::string logmessage;
	
};

#endif
