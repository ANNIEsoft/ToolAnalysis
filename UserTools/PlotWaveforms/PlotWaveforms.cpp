/* vim:set noexpandtab tabstop=4 wrap */

#include "PlotWaveforms.h"

PlotWaveforms::PlotWaveforms():Tool(){}


bool PlotWaveforms::Initialise(std::string configfile, DataModel &data){
	
	/////////////////// Useful header ///////////////////////
	if(configfile!="") m_variables.Initialise(configfile); //loading config file
	//m_variables.Print();
	
	m_data= &data; //assigning transient data pointer
	/////////////////////////////////////////////////////////////////
	
	// construct a RawViewer
	theviewer = new annie::RawViewer();
	
	// construct the TApplication to show it
	int myargc=0;
	char *myargv[] = {(const char*)"spudleymunchkin"};
	annieviewerRootDrawApp = new TApplication("annieviewerRootDrawApp",&myargc,myargv);
	
	return true;
}


bool PlotWaveforms::Execute(){
	// update the viewer data
	theviewer->next_readout(raw_readout);
	annieviewerRootDrawApp->Run();
	
	return true;
}


bool PlotWaveforms::Finalise(){
	delete annieviewerRootDrawApp;
	return true;
}
