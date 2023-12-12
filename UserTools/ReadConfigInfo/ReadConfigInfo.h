#ifndef ReadConfigInfo_H
#define ReadConfigInfo_H

#include <string>
#include <iostream>
#include <fstream>

#include "Tool.h"


/**
* \class ReadConfigInfo
*
* ReadConfigInfo reads the ConfigInfo at the ANNIEEvent->Header and saves it in a single text file or multiple files, one for each entry in my_inputs.txt of the LoadANNIEEvent Tool
*
* $Author: Johann Martyn $
* $Date: 2023/06/20
* Contact: jomartyn@uni-mainz.de
*/
class ReadConfigInfo: public Tool {


	public:

	ReadConfigInfo(); ///< Simple constructor
	bool Initialise(std::string configfile,DataModel &data); ///< Initialise Function for setting up Tool resources. @param configfile The path and name of the dynamic configuration file to read in. @param data A reference to the transient data class used to pass information between Tools.
	bool Execute(); ///< Execute function used to perform Tool purpose.
	bool Finalise(); ///< Finalise function used to clean up resources.


	private:
	std::string filename;
	int verbosity;
	int old_run_number;
	int old_subrun_number;
	int old_part_number;
	ofstream outfile;
	bool write_seperate_files;
	const int v_error=0;
	const int v_warning=1;
	const int v_message=2;
	const int v_debug=3;


};


#endif
