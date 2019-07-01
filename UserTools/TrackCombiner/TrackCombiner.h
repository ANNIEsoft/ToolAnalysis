/* vim:set noexpandtab tabstop=4 wrap */
#ifndef TrackCombiner_H
#define TrackCombiner_H

#include <string>
#include <iostream>

#include "Tool.h"


/**
* \class TrackCombiner
*
* This tool combines matches reconstructed tank vertices with their reconstructed MRD tracks.
* If no suitable match is found, it will associate the tank vertex with any close-in-time MRD hits,
* which may later be used to do a cruder form of MRD reconstruction.
*
* $Author: M.O'Flaherty $
* $Date: 2019/06/28 10:44:00 $
* Contact: msoflaherty1@sheffield.ac.uk
*/
class TrackCombiner: public Tool {
	public:
	
	TrackCombiner(); ///< Simple constructor
	bool Initialise(std::string configfile,DataModel &data); ///< Initialise Function for setting up Tool resorces. @param configfile The path and name of the dynamic configuration file to read in. @param data A reference to the transient data class used to pass information between Tools.
	bool Execute(); ///< Execute function used to perform Tool purpose.
	bool Finalise(); ///< Finalise funciton used to clean up resorces.
	
	private:
	std::map<unsigned long,vector<MCHit>>* TDCData;
	
	// verbosity levels: if 'verbosity' < this level, the message type will be logged.
	int verbosity=1;
	int v_error=0;
	int v_warning=1;
	int v_message=2;
	int v_debug=3;
	std::string logmessage="";
	int get_ok=0;
};


#endif
