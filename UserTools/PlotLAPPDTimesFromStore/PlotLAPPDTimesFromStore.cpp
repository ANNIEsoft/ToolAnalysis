/* vim:set noexpandtab tabstop=4 wrap */

#include "PlotLAPPDTimesFromStore.h"

// for drawing
#include "TROOT.h"
#include "TApplication.h"
#include "TCanvas.h"
#include "TH1.h"
#include "TPolyMarker3D.h"
#include "TSystem.h"

PlotLAPPDTimesFromStore::PlotLAPPDTimesFromStore():Tool(){}

bool PlotLAPPDTimesFromStore::Initialise(std::string configfile, DataModel &data){
	
	/////////////////// Useful header ///////////////////////
	
	if(verbose) cout<<"Initializing Tool PlotLAPPDTimesFromStore"<<endl;
	
	if(configfile!="") m_variables.Initialise(configfile); //loading config file
	//m_variables.Print();
	
	m_data= &data; //assigning transient data pointer
	/////////////////////////////////////////////////////////////////
	
	// Get the Tool configuration variables
	// ====================================
	m_variables.Get("verbose",verbose);
	
	// Get trigger window parameters from CStore
	// =========================================
	int pretriggerwindow;
	int posttriggerwindow;
	m_data->CStore.Get("WCSimPreTriggerWindow",pretriggerwindow);
	m_data->CStore.Get("WCSimPostTriggerWindow",posttriggerwindow);
	if(verbose>2) cout<<"WCSimPreTriggerWindow="<<pretriggerwindow
					  <<", WCSimPostTriggerWindow="<<posttriggerwindow<<endl;
	
	// create the ROOT application to show histograms
	int myargc=0;
	char *myargv[] = {(const char*)"somestring"};
	lappdRootDrawApp = new TApplication("lappdRootStoreTimesDrawApp",&myargc,myargv);
	digitime = new TH1D("lappd_digitime_store","lappd digitimes",100,-500,1500);
	
	return true;
}

bool PlotLAPPDTimesFromStore::Execute(){
	
	if(verbose) cout<<"Executing tool PlotLAPPDTimesFromStore"<<endl;
	
	MCLAPPDHits=nullptr;
	get_ok = m_data->Stores.at("ANNIEEvent")->Get("MCEventNum",MCEventNum);
	get_ok = m_data->Stores.at("ANNIEEvent")->Get("MCTriggernum",MCTriggernum);
	get_ok = m_data->Stores.at("ANNIEEvent")->Get("EventTime",EventTime);
	get_ok = m_data->Stores.at("ANNIEEvent")->Get("MCLAPPDHits",MCLAPPDHits);
	
	double wcsimtriggertime = 0; // static_cast<double>(EventTime->GetNs()); // returns a uint64_t
	if(verbose>2) cout<<"LAPPDTool: Trigger time for event "<<MCEventNum<<", trigger "
					  <<MCTriggernum<<" was "<<wcsimtriggertime<<"ns"<<endl;
	
	if(MCLAPPDHits){
		for(auto&& achannel : *MCLAPPDHits){
			ChannelKey chankey = achannel.first;
			auto& hits = achannel.second;
			for(auto&& ahit : hits){ digitime->Fill(ahit.GetTime()); }
		}
	} else {
		cout<<"No MCLAPPDHits"<<endl;
	}
	
	return true;
}


bool PlotLAPPDTimesFromStore::Finalise(){
	
	Log("plotlappdtimesfromstore finalising",0,12);
	Double_t canvwidth = 700;
	Double_t canvheight = 600;
	lappdRootCanvas = new TCanvas("lappdTimesFromStoreCanvas","lappdTimesFromStoreCanvas",canvwidth,canvheight);
	Log("canvas created",0,12);
	lappdRootCanvas->SetWindowSize(canvwidth,canvheight);
	lappdRootCanvas->cd();
	Log("canvas selected",0,12);
	
	digitime->Draw();
	Log("histo drawn",0,12);
	//lappdRootCanvas->SetLogy(1);  // spits errors about negative axis
	gPad->SetLogy(1);
	Log("logy set",0,12);
	lappdRootCanvas->Update();
	lappdRootCanvas->SaveAs("lappddigitimes_instore.png");
	Log("saved, deleting hist",0,12);
	
	//lappdRootDrawApp->Run();
	//std::this_thread::sleep_for (std::chrono::seconds(5));
	//lappdRootDrawApp->Terminate(0);
	
	delete digitime;
	Log("deleting canvas",0,12);
	delete lappdRootCanvas;
	Log("deleting app",0,12);
	delete lappdRootDrawApp;
	Log("exiting",0,12);
	return true;
}
