/* vim:set noexpandtab tabstop=4 wrap */

#ifndef	PlotWaveforms_H
#define	PlotWaveforms_H

#include <string>
#include <iostream>

#include "Tool.h"
#include "RawViewer.h"
class TApplication;

class PlotWaveforms: public Tool {

public:
	
	PlotWaveforms();
	bool Initialise(std::string configfile, DataModel &data);
	bool Execute();
	bool Finalise();
	
private:
	
	annie::RawViewer* theviewer;
	TApplication* rootTApp=nullptr;
	bool viewer_closed;
	
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
