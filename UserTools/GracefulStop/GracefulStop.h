/* vim:set noexpandtab tabstop=4 wrap */
#ifndef GracefulStop_H
#define GracefulStop_H

#include <string>
#include <iostream>
#include <signal.h>
//#include <stdexcept>
//#include <errno.h>

#include "Tool.h"

/**
* \class GracefulStop
*
* This tool checks for the SIGUSR1 linux signal and sets the 'StopLoop' variable to gracefully end the toolchain if it has been received.
*
* $Author: B.Richards $
* $Date: 2019/05/28 10:44:00 $
* Contact: b.richards@qmul.ac.uk
*/

class GracefulStop: public Tool {
	
	public:
	
	GracefulStop();  ///< Simple constructor
	bool Initialise(std::string configfile,DataModel &data); ///< Initialise Function for setting up Tool resorces. @param configfile The path and name of the dynamic configuration file to read in. @param data A reference to the transient data class used to pass information between Tools.
	bool Execute();  ///< Execute function checks if we received linux SIGUSR1 and sets "StopLoop" to perform a graceful Toolchain stop if we have.
	bool Finalise(); ///< Finalise function cleans up signal handler.
	
	private:
	static void stopSignalHandler(int _ignored);  // we need a function to register as the signal handler
	static bool gotStopSignal;
	
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
