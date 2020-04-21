/* vim:set noexpandtab tabstop=4 wrap */
#ifndef PulseSimulation_H
#define PulseSimulation_H

#include "Tool.h"
#include "Waveform.h"

#include <string>
#include <iostream>
#include <vector>
#include <map>
#include <algorithm>     // remove and remove_if
#include <stdlib.h>      // atoi
#include <sys/types.h>   // for stat() test to see if file or folder
#include <sys/stat.h>
#include <unistd.h>
#include <stdint.h>
#include <stdlib.h>

#include "MCCardData.h"

#include "TTree.h"
#include "TFile.h"
#include "TApplication.h"
#include "TSystem.h"
#include "TROOT.h"
#include "TCanvas.h"
#include "TGraph.h"
#include "TRandom3.h"
#include "TF1.h"

// for drawing
class TApplication;
class TCanvas;

namespace {
	// RAW file DAQ constants
	constexpr int channels_per_adc_card = 4; // for converting tube Id to card / channel, or minibuffer number
	constexpr int channels_per_tdc_card = 32;
	
	// for converting digit charge to pulse area in [ADC counts*ADC samples]
	constexpr double E_CHARGE = 1.60217662e-19;                // 
	constexpr uint64_t SEC_TO_NS = 1000000000;                 // 
	constexpr int ADC_NS_PER_SAMPLE=2;                         // sample rate
	constexpr double ADC_INPUT_RESISTANCE = 50.;               // Ohm
	//constexpr double ADC_TO_VOLT = 2.415 / std::pow(2., 12); // multiply to convert ADC counts to Volts
	constexpr int TDC_NS_PER_SAMPLE = 4;                       //
	constexpr int MRD_TIMEOUT_NS=4200;                         //
	constexpr int MRD_TIMESTAMP_DELAY = static_cast<unsigned long long>(MRD_TIMEOUT_NS);
	
	int MAXEVENTSIZE=10;                             // initial array sizes for TriggerData tree
	int MAXTRIGGERSIZE=10;                           // must not be const
	//	constexpr int BOGUS_INT = std::numeric_limits<int>::max();
}

class PulseSimulation: public Tool {
	
	public:
	
	PulseSimulation();
	bool Initialise(std::string configfile,DataModel &data);
	bool Execute();
	bool Finalise();
	
	private:
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
	bool DRAW_DEBUG_PLOTS;
	TApplication* rootTApp=nullptr;
	double canvwidth, canvheight;
	TCanvas* landaucanvas=nullptr;
	TCanvas* buffercanvas=nullptr;
	TGraph* buffergraph=nullptr;      // full channel buffer after adding another pulse
	TCanvas* pulsecanvas=nullptr;
	TGraph* pulsegraph=nullptr;       // a new pulse
	TGraph* fullbuffergraph=nullptr;
	
	// ANNIEEvent variables
	// --------------------
	Geometry* anniegeom=nullptr;
	std::map<unsigned long,int> channelkey_to_pmtid;
	std::map<unsigned long,int> channelkey_to_mrdpmtid;
	std::map<unsigned long,int> channelkey_to_faccpmtid;
	std::map<int,unsigned long> pmt_tubeid_to_channelkey;
	int NumFaccPMTs;
	
	// Internal Functions
	// ------------------
	void AddPMTDataEntry(MCHit* digihit);
	void GenerateMinibufferPulse(int digit_index, double adjusted_digit_q, std::vector<uint16_t> &pulsevector);
	void AddMinibufferStartTime(bool droppingremainingsubtriggers);
	void ConstructEmulatedPmtDataReadout();
	bool FillEmulatedPMTData();
	void AddNoiseToWaveforms();
	void RiffleShuffle(bool do_shuffle);
	void LoadOutputFiles();
	void FillInitialFileInfo();
	void FillEmulatedRunInformation();
	void FillEmulatedTrigData();
	void AddCCDataEntry(MCHit* digihit);
	void FillEmulatedCCData();
	std::vector<std::string>* GetTemplateRunInfo();
	
	// File format members
	// -------------------
	int FILE_VERSION;                        // simulation file version
	int num_adc_cards;                       // 
	int num_tdc_cards;                       // not strictly used anywhere
	int pre_trigger_window_ns;               // for converting trigger-relative time to minibuffer index
	int post_trigger_window_ns;              // for calculating buffer size
	int full_buffer_size;                    // for positioning this pulse in the full buffer *
	int buffer_size;                         // datapoints per minibuffer for all 4 channels *
	int minibuffer_id;                       // the current minibuffer we're filling
	int minibuffer_datapoints_per_channel;   // num datapoints per channel per minibuffer *
	int minibuffers_per_fullbuffer;          // 
	int emulated_event_size;                 // just (minibuffer_datapoints_per_channel / 4) *
	bool DoPhaseOneRiffle;                   // whether or not to shuffle data in vectors
	// * = member of MCCardData
	
	// Members used in waveform generation
	// ------------------------------------
	TF1* fLandau{nullptr};
	std::vector<uint16_t> pulsevector;
	double PULSE_HEIGHT_FUDGE_FACTOR;        // because we always need to fudge it
	
	// variables for connecting events into a run and filling the other file variables
	// -------------------------------------------------------------------------------
	TRandom3 R;
	std::string runStartDateTime;            // read from config, used to generate RunStartTimeSec
	Int_t RunStartTimeSec;                   // passed to fileout_StartTimeSec
	uint64_t runningeventtime;               // beam timing, ns from the start of the run to the beam spill
	double currenteventtime;                 // time from start of the beam spill to first trigger of this event
	TimeClass* EventTime=nullptr;            // time from the simulation event to the trigger
	int triggertime;                         // unixns from above
	uint16_t triggernum;                     // MCTriggernum
	int sequence_id;                         // ADC readout number
	bool skippingremainders;                 // if we're passed more sub-events with a full buffer, do nothing
	
	// variables to go into the fake raw files
	// ---------------------------------------
	std::vector<MCCardData> emulated_pmtdata_readout;
	std::vector<std::vector<uint16_t>> temporary_databuffers; // temp buffers while creating waveforms
	std::vector<int64_t> StartCountVals;
	std::vector<uint64_t> StartTimeNSecVals;
	
	// variables used when putting stuff into Stores
	// ---------------------------------------------
	bool GenerateFakeRootFiles;      // turn on/off by config file
	bool PutOutputsIntoStore;        // put into store as well / instead
	BoostStore* pmtDataStore;        // equivalent of tPMTData
	BoostStore* heftydbStore;        // equivalant of theftydb
	BoostStore* CCDataStore;         // equivalant of tCCData
	std::vector<intptr_t> pmtDataVector;
	std::map<unsigned long,std::vector<Waveform<uint16_t>>> RawADCData;
	
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
	
	// mrd data tree
	// ~~~~~~~~~~~~~
	TTree* tCCData;
	UInt_t fileout_Trigger, fileout_OutNumber;
	ULong64_t fileout_TimeStamp;
	std::vector<string> fileout_Type;
	std::vector<unsigned int> fileout_Value, fileout_Slot, fileout_Channel;
	// for TTree branches
	std::vector<unsigned int>* pfileout_Value, *pfileout_Slot, *pfileout_Channel;
	std::vector<string>* pfileout_Type;
	
	// run info tree
	// ~~~~~~~~~~~~~
	TTree* tRunInformation;
	std::string fileout_InfoTitle, fileout_InfoMessage;
	
	
};


#endif
