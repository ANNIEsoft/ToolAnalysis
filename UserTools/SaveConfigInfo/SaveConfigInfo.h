#ifndef SaveConfigInfo_H
#define SaveConfigInfo_H

#include <string>
#include <sstream>
#include <iostream>
#include <fstream>

#include "Tool.h"

#include <BoostStore.h>


/**
* \class SaveConfigInfo
*
* SaveConfigInfo reads a list of textfiles and the git commit hash and saves them to the CStore with std::string config_info
*
* $Author: Johann Martyn $
* $Date: 2023/06/20
* Contact: jomartyn@uni-mainz.de
*/
class SaveConfigInfo: public Tool {


	public:
	SaveConfigInfo(); ///< Simple constructor
	bool Initialise(std::string configfile,DataModel &data); ///< Initialise Function for setting up Tool resources. @param configfile The path and name of the dynamic configuration file to read in. @param data A reference to the transient data class used to pass information between Tools.
	bool Execute(); ///< Execute function used to perform Tool purpose.
	bool Finalise(); ///< Finalise function used to clean up resources.


	private:
	std::vector<std::string> vec_configfiles;
	std::stringstream buffer;
	std::string outfilename;
	std::string config_info;
	std::ofstream outfile;
	std::ifstream infile;
	bool full_depth;

	/// \brief verbosity levels: if 'verbosity' < this level, the message type will be logged.
	int verbosity=0;
	const int v_error=0;
	const int v_warning=1;
	const int v_message=2;
	const int v_debug=3;

};


#endif
