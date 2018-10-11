/* vim:set noexpandtab tabstop=4 wrap */

#ifndef	PlotWaveforms_H
#define	PlotWaveforms_H

#include <string>
#include <iostream>

#include "Tool.h"

class PlotWaveforms: public Tool {

public:
	
	PlotWaveforms();
	bool Initialise(std::string configfile, DataModel &data);
	bool Execute();
	bool Finalise();
	
private:
	annie::RawViewer* theviewer;
	
};


#endif
