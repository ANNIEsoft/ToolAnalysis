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
	
	// get or construct the TApplication to show it
        int myargc=0;
        //char *myargv[] = {(const char*)"somestring"};
        intptr_t tapp_ptr=0;
        get_ok = m_data->CStore.Get("RootTApplication",tapp_ptr);
        if(not get_ok){
                if(verbosity>2) cout<<"PlotWaveforms Tool: making global TApplication"<<endl;
                rootTApp = new TApplication("rootTApp",&myargc,0);
                tapp_ptr = reinterpret_cast<intptr_t>(rootTApp);
                m_data->CStore.Set("RootTApplication",tapp_ptr);
        } else {
                if(verbosity>2) cout<<"PlotWaveforms Tool: Retrieving global TApplication"<<std::endl;
                rootTApp = reinterpret_cast<TApplication*>(tapp_ptr);
        }
        int tapplicationusers;
        get_ok = m_data->CStore.Get("RootTApplicationUsers",tapplicationusers);
        if(not get_ok) tapplicationusers=1;
        else tapplicationusers++;
        m_data->CStore.Set("RootTApplicationUsers",tapplicationusers);
	
	// construct a RawViewer
	theviewer = new annie::RawViewer();
	
	viewer_closed=false;  // TODO take args from config file to open at a requested sequence_id
	
	return true;
}

// N.B. 'cout' in this tool doesn't seem to work? Log() works fine though. 
bool PlotWaveforms::Execute(){
	// if viewer is closed, do not update
	if(viewer_closed) return true;
	// also prevent the viewer showing if loop is stopping, we may not have any data
	bool loop_stopping;
	m_data->vars.Get("StopLoop", loop_stopping);
	if(loop_stopping) return true;
	
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
	
        int tapplicationusers=0;
        get_ok = m_data->CStore.Get("RootTApplicationUsers",tapplicationusers);
        if(not get_ok || tapplicationusers==1){
                if(rootTApp){
                        std::cout<<"PlotWaveforms Tool: Deleting global TApplication"<<std::endl;
			rootTApp->Terminate();
                        delete rootTApp;
                        rootTApp=nullptr;
                }
        } else if(tapplicationusers>1){
                m_data->CStore.Set("RootTApplicationUsers",tapplicationusers-1);
        }
	
	if(theviewer) delete theviewer;
	return true;
}
