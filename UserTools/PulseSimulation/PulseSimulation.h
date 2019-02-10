/* vim:set noexpandtab tabstop=4 wrap */
#ifndef PulseSimulation_H
#define PulseSimulation_H

#include "Tool.h"

#include <string>
#include <iostream>
#include <vector>
#include <map>
#include <algorithm>     // remove and remove_if
#include <regex>
#include <stdlib.h>      // atoi
#include <sys/types.h>   // for stat() test to see if file or folder
#include <sys/stat.h>
#include <unistd.h>
#include <stdint.h>
#include <stdlib.h>

#include "CardData.h"

#include "TTree.h"
#include "TFile.h"
#include "TApplication.h"
#include "TSystem.h"
#include "TROOT.h"
#include "TCanvas.h"
#include "TGraph.h"

// for drawing
class TApplication;
class TCanvas;

namespace {
	// RAW file DAQ constants
	constexpr int channels_per_adc_card = 4; // for converting tube Id to card / channel, or minibuffer number
	
	// for converting digit charge to pulse area in [ADC counts*ADC samples]
	constexpr double E_CHARGE = 1.60217662e-19;                // 
	constexpr uint64_t SEC_TO_NS = 1000000000;                 // 
	constexpr int ADC_NS_PER_SAMPLE=2;                         // sample rate
	constexpr double ADC_INPUT_RESISTANCE = 50.;               // Ohm
	//constexpr double ADC_TO_VOLT = 2.415 / std::pow(2., 12); // * by this constant converts ADC counts to Volts
	
	int MAXEVENTSIZE=10;                             // initial array sizes for TriggerData tree
	int MAXTRIGGERSIZE=10;                           // must not be const
	constexpr int BOGUS_INT = std::numeric_limits<int>::max();
}

class PulseSimulation: public Tool {
	
	public:
	
	PulseSimulation();
	bool Initialise(std::string configfile,DataModel &data);
	bool Execute();
	bool Finalise();
	
	private:
	
	int get_ok;
	int verbosity;
	// verbosity levels: if 'verbosity' < this level, the message type will be logged.
	int v_error=0;
	int v_warning=1;
	int v_message=2;
	int v_debug=3;
	std::string logmessage;
	
	bool DRAW_DEBUG_PLOTS;
	int FILE_VERSION;                 // simulation file version
	TApplication* pulseRootDrawApp=nullptr;
	double canvwidth, canvheight;
	TCanvas* landaucanvas=nullptr;
	TCanvas* buffercanvas=nullptr;
	TGraph* buffergraph=nullptr;      // full channel buffer after adding another pulse
	TCanvas* pulsecanvas=nullptr;
	TGraph* pulsegraph=nullptr;       // a new pulse
	TGraph* fullbuffergraph=nullptr;
	
	void AddPMTDataEntry(Hit* digihit);
	void GenerateMinibufferPulse(int digit_index, double adjusted_digit_q, std::vector<uint16_t> &pulsevector);
	void AddMinibufferStartTime(bool droppingremainingsubtriggers);
	void ConstructEmulatedPmtDataReadout();
	void FillEmulatedPMTData();
	void AddNoiseToWaveforms();
	void RiffleShuffle(bool do_shuffle);
	void LoadOutputFiles();
	void FillInitialFileInfo();
	void FillEmulatedRunInformation();
	void FillEmulatedTrigData();
	std::vector<std::string>* GetTemplateRunInfo();
	
	int num_adc_cards;                       // 
	int pre_trigger_window_ns;               // for converting trigger-relative time to minibuffer index
	int post_trigger_window_ns;              // for calculating buffer size
	int full_buffer_size;                    // for positioning this pulse in the full buffer *
	int buffer_size;                         // datapoints per minibuffer for all 4 channels *
	int minibuffer_id;                       // the current minibuffer we're filling
	int minibuffer_datapoints_per_channel;   // num datapoints per channel per minibuffer *
	int minibuffers_per_fullbuffer;          // 
	int emulated_event_size;                 // * 
	// * = member of CardData
	
	// variables to generate/store just one temporary pulse trace
	TF1* fLandau{nullptr};
	std::vector<uint16_t> pulsevector;
	double PMT_gain;                         // an average
	double PULSE_HEIGHT_FUDGE_FACTOR;        // because we always need to fudge it
	
	// variables for connecting events into a run and filling the other file variables
	TRandom3 R;
	uint64_t runningeventtime;               // beam timing, ns from the start of the run to the beam spill
	double currenteventtime;                 // time from start of the beam spill to first trigger of this event
	int triggertime;                         // time from the simulation event to the trigger
	uint16_t triggernum;
	int sequence_id;
	bool skippingremainders;                 // if we're passed more sub-events with a full buffer, do nothing
	
	// variables to go into the fake raw files
	std::vector<CardData> emulated_pmtdata_readout;
	std::vector<std::vector<uint16_t>> temporary_databuffers; // temp buffers while creating waveforms
	std::vector<int64_t> StartCountVals;
	std::vector<uint64_t> StartTimeNSecVals;
	
	// timing heftydb tree
	// ~~~~~~~~~~~~~~~~~~~
	TFile* timingfileout=nullptr;
	TTree* theftydb;
	Long_t* timefileout_TSinceBeam;          // trigger time to beam time
	Int_t* timefileout_More;                 // did we drop a minibuffer to transfer data
	Int_t* timefileout_Label;                // Beam or Window trigger
	ULong_t* timefileout_Time;               // 
	
	// pmt data tree
	// ~~~~~~~~~~~~~
	std::string rawfilename;
	TFile* rawfileout=nullptr;
	TTree* tPMTData;
	ULong64_t fileout_LastSync, fileout_StartCount;
	Int_t fileout_SequenceID, fileout_StartTimeSec, fileout_StartTimeNSec, fileout_TriggerNumber,
		fileout_CardID, fileout_Channels, fileout_BufferSize, fileout_Eventsize, fileout_FullBufferSize;
	ULong64_t* fileout_TriggerCounts=nullptr;
	UInt_t* fileout_Rates=nullptr;
	UShort_t* fileout_Data=nullptr;
	
	// trigger data tree
	// ~~~~~~~~~~~~~~~~~
	TTree* tTrigData;
	Int_t fileout_FirmwareVersion, fileout_TriggerSize, /*fileout_EventSize, << same variable in tPMTData*/
		fileout_FIFOOverflow, fileout_DriverOverfow;
	UShort_t* fileout_EventIDs=nullptr;
	ULong64_t* fileout_EventTimes=nullptr;
	UInt_t* fileout_TriggerMasks=nullptr, *fileout_TriggerCounters=nullptr;
	
	// run info tree
	// ~~~~~~~~~~~~~
	TTree* tRunInformation;
	std::string fileout_InfoTitle, fileout_InfoMessage;
	
	
};


#endif
