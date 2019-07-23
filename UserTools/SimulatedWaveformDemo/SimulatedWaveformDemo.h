/* vim:set noexpandtab tabstop=4 wrap */
#ifndef SimulatedWaveformDemo_H
#define SimulatedWaveformDemo_H

#include <string>
#include <iostream>
#include <thread>
#include <chrono>

#include "Tool.h"

#include "TApplication.h"
#include "TSystem.h"
#include "TROOT.h"
#include "TCanvas.h"
#include "TGraph.h"

/**
 * \class SimulatedWaveformDemo
 *
 * This tool is a demonstration on how to retrieve simulated waveforms made by the PulseSimulation tool from the BoostStores.
*
* $Author: M.O'Flaherty $
* $Date: 2019/07/21 15:35:00 $
* Contact: msoflaherty1@sheffield.ac.uk
*/
class SimulatedWaveformDemo: public Tool {
	
	public:
	SimulatedWaveformDemo(); ///< Simple constructor
	bool Initialise(std::string configfile,DataModel &data); ///< Initialise Function for setting up Tool resorces. @param configfile The path and name of the dynamic configuration file to read in. @param data A reference to the transient data class used to pass information between Tools.
	bool Execute();          ///< Execute function used to perform Tool purpose. 
	bool Finalise();         ///< Finalise function used to clean up resources.
	
	private:
	// container BoostStores
	BoostStore* pmtDataStore=nullptr;            // equivalent of tPMTData
	BoostStore* heftydbStore=nullptr;            // equivalant of theftydb
	BoostStore* CCDataStore=nullptr;             // equivalant of tCCData
	
	// CCDataStore members:
	UInt_t CCDataReadout;                        // CCData readout number, cumulative number of minibuffers.
	UInt_t CCHitsThisReadout;                    // num hits this event/readout
	ULong64_t ReadoutTimeUnixMs;                 // time of CCData readout start, unix MILLI seconds
	std::vector<string>* CardType;               // card type string, "TDC", "ADC"
	std::vector<unsigned int>* Slot;             // card position in crate
	std::vector<unsigned int>* Channel;          // channel in card
	std::vector<unsigned int>* TDCTickValue;     // TDC ticks (4ns each) from readout start to this hit
	
	// heftydbStore members:
	Int_t heftydbSequenceID;                     // readout number, cumulative number of full buffers.
	ULong_t* minibuffer_starts_unixns=nullptr;   // start time of this minibuffer [unix ns]
	Int_t* minibuffer_label=nullptr;             // trigger type
	Long_t* minibuffer_tsincebeam=nullptr;       // trigger time rel. to sim event start [ns]
	Int_t* dropped_delayed_triggers=nullptr;     // were other MC delayed triggers dropped
	
	// pmtDataStore members:
	Int_t pmtDataSequenceID;                     // readout number, cumulative number of full buffers.
	Int_t RunStartTimeSec;                       // unix seconds to run start, from config file
	Int_t ChannelsPerAdcCard;                    // channels_per_adc_card
	Int_t MiniBuffersPerFullBuffer;              // minibuffers_per_fullbuffer
	Int_t BufferSize;                            // minibuf_samples_per_ch * minibufs_per_fullbuf
	std::vector< const std::vector<uint16_t>* > pmtDataVector;   // vector (per card) of waveforms
	
	// ANNIEEvent members:
	std::map<unsigned long,std::vector<Waveform<uint16_t>>> RawADCData;
	
	// internal members:
	int SamplesPerMinibuffer;
	std::vector<int> numberline;
	std::vector<int> upcastdata;
	int maxwfrmamp=0;
	int WaveformSource;
	
	// Logging variables
	// -----------------
	int get_ok;
	int verbosity;
	// verbosity levels: if 'verbosity' < this level, the message type will be logged.
	int v_error=0;
	int v_warning=1;
	int v_message=2;
	int v_debug=3;
	std::string logmessage;
	
	// ROOT TApplication variables
	// ---------------------------
	TApplication* rootTApp=nullptr;
	double canvwidth, canvheight;
	TCanvas* mb_canv=nullptr;
	TGraph* mb_graph=nullptr;
	
};


#endif
