/* vim:set noexpandtab tabstop=4 wrap */

#include "PlotWaveforms.h"

// for drawing
#include "TROOT.h"
#include "TApplication.h"
#include "TCanvas.h"
#include "TArrow.h"
#include "TSystem.h"

#include <future>
#include <thread>

PlotWaveforms::PlotWaveforms():Tool(){}


bool PlotWaveforms::Initialise(std::string configfile, DataModel &data){
	
	/////////////////// Useful header ///////////////////////
	if(configfile!="") m_variables.Initialise(configfile); //loading config file
	//m_variables.Print();
	
	m_data= &data; //assigning transient data pointer
	/////////////////////////////////////////////////////////////////
	// get config variables
	m_variables.Get("verbose", verbosity);
	Log("PlotWaveforms Tool: Initializing",v_message,verbosity);
	
	// construct the TApplication to show it
	int myargc=0;
	char *myargv[] = {(const char*)"spudleymunchkin"};
	annieviewerRootDrawApp = new TApplication("annieviewerRootDrawApp",&myargc,myargv);
	
	// construct a RawViewer
	theviewer = new annie::RawViewer();
	
	viewer_closed=false;  // TODO take args from config file to open at a requested sequence_id
	
	return true;
}

// N.B. 'cout' in this tool doesn't seem to work? Log() works fine though. 
bool PlotWaveforms::Execute(){
	// if viewer is closed, do not update
	if(viewer_closed) return true;
	
	// update the viewer data
	annie::RawReadout* raw_readout;
	get_ok = m_data->Stores.at("ANNIEEvent")->Get("RawReadout",raw_readout);
	if(not get_ok){
		Log("PlotWaveforms Tool: RawReadout pointer not in ANNIEEvent store",v_error,verbosity);
		return false;
	}
	
	std::promise<int> barrier;  // will hold until we're done viewing waveforms
	std::future<int> barrier_future = barrier.get_future();
	theviewer->next_readout(raw_readout, std::move(barrier));
	std::chrono::milliseconds span(100);
	std::future_status thestatus;
	do {
		gSystem->ProcessEvents();
		thestatus = barrier_future.wait_for(span);
	} while (thestatus==future_status::timeout);
	if(barrier_future.get()==0){ viewer_closed = true; delete theviewer; theviewer=nullptr; }
	// TODO maybe we could hide it for triggering at later events?
	
	return true;
}


bool PlotWaveforms::Finalise(){
	annieviewerRootDrawApp->Terminate();
	delete annieviewerRootDrawApp;
	if(theviewer) delete theviewer;
	return true;
}
