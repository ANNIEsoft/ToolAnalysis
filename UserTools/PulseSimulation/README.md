# PulseSimulation

PulseSimulation

## Data

**----------------------------------------------------------------------------**

**theftydb Tree:**
`Int_t fileout_SequenceID` // readout number, number of full buffers.
`timefileout_Time          = new ULong_t[minibuffers_per_fullbuffer];`  // run start time + this beam trigger offset + this minibuffer offset
`timefileout_Label         = new Int_t[minibuffers_per_fullbuffer];`   // trigger type: Beam(16) or Window(16777216)
`timefileout_TSinceBeam    = new Long_t[minibuffers_per_fullbuffer];`  // trigger time rel. to sim event start [ns]
`timefileout_More          = new Int_t[minibuffers_per_fullbuffer];`   // were there further delayed triggers in this MC event that we're dropping because our "buffer" is full?

**----------------------------------------------------------------------------**

**tPMTData Tree:**
`fileout_TriggerCounts    = new ULong64_t[minibuffers_per_fullbuffer];`     // arbitrary
`fileout_Rates            = new UInt_t[channels_per_adc_card];`             // arbitrary
`ULong64_t fileout_LastSync`     // arbitrary
`Int_t fileout_StartTimeNSec`    // arbitrary
`ULong64_t fileout_StartCount`   // arbitrary
`Int_t fileout_SequenceID`       // readout number, number of full buffers.
`Int_t fileout_StartTimeSec`     // unix seconds to run start, from config file
`Int_t fileout_TriggerNumber`    // minibuffers_per_fullbuffer
`Int_t fileout_CardID`           // VME card num
`Int_t fileout_Channels`         // channels_per_adc_card
`Int_t fileout_BufferSize`       // minibuf_samples_per_ch * minibufs_per_fullbuf
// MinibuffersPerFullbuffer is set in config file
// where minibuf_samples_per_ch = window_duration_ns / ADC_NS_PER_SAMPLE
// with window_duration_ns = (pre_trigger_window_ns+post_trigger_window_ns)
`Int_t fileout_Eventsize`        // minibuffer_datapoints_per_channel / 4.
`Int_t fileout_FullBufferSize`   // fileout_BufferSize * channels_per_adc_card;
`fileout_Data             = new UShort_t[full_buffer_size];`
// the PMTData tree has one entry per VME card, per readout; i.e. 16 entries (cards) per readout.
// Entries are ordered according to the card position in the vme crate, so are consistent but not
// necessarily monotonic (although here they are, in reality a crate has 16 cards, with numbers up to 21).
// ([Card 0 TTree Entry][Card 1 TTree Entry][Card 4 TTree Entry]...)
// Each TTree entry contains a Data[] array that concatenates all channels on that card:
// [chan 1][chan 2][chan 3][chan 4]
// furthermore, within each channel, there are 40 concatenated minibuffers:
// [{ch1:mb1}{ch1:mb2}...{ch1:mb40}][{ch2:mb1}{ch2:mb2}...{ch2:mb40}] --- [{ch4:mb1}{ch4:mb2}...{ch4:mb40}]

**............................................................................**

**tCCData Tree:**
`UInt_t fileout_Trigger`                      // CCData readout number == ANNIEEvent EventNum
`UInt_t fileout_OutNumber`                    // num hits this event/readout
`std::vector<string> fileout_Type`            // card type string, "TDC", "ADC"
`std::vector<unsigned int> fileout_Slot`      // card position in crate
`std::vector<unsigned int> fileout_Channel`   // channel in card
`std::vector<unsigned int> fileout_Value`     // TDC ticks from readout start to this hit (TDC_NS_PER_SAMPLE=4)
`ULong64_t fileout_TimeStamp`                 // CCUSB readout start time, [unix ms]
// Timestamp is applied by the PC post-readout so is actually delayed from the trigger!
```
unsigned long long timestamp_ms = ( (1./1000000.) * (        // NS TO MS
	fileout_StartTimeSec*SEC_TO_NS +                     // run start time
	currenteventtime +                                   // ns from Run start to this event start
	EventTime->GetNs() +                                 // trigger ns since event start
	MRD_TIMESTAMP_DELAY                                  // delay between trigger card and mrd PC
) );

**............................................................................**

**tTrigData Tree:**               // effectively entirely arbitrary at this point
`fileout_EventIDs          = new UShort_t[MAXEVENTSIZE];`    // arbitrary
`fileout_EventTimes        = new ULong64_t[MAXEVENTSIZE];`   // arbitrary
`fileout_TriggerMasks      = new UInt_t[MAXTRIGGERSIZE];`    // arbitrary
`fileout_TriggerCounters   = new UInt_t[MAXTRIGGERSIZE];`    // arbitrary
`Int_t fileout_FIFOOverflow`    // arbitrary
`Int_t fileout_TriggerSize`     // arbitrary
`Int_t fileout_DriverOverfow`   // arbitrary
`Int_ fileout_FirmwareVersion`  // WCSim version
`Int_t fileout_SequenceID`      // readout number, number of full buffers.  (also in tPMTData Tree)
`Int_t fileout_Eventsize`       // minibuffer_datapoints_per_channel / 4.   (also in tPMTData Tree)

**............................................................................**

**tRunInformation Tree:**              // arbitrary at this point
`std::string fileout_InfoTitle`      // both are placeholders
`std::string fileout_InfoMessage`    // see get template run info for details

**............................................................................**

## Configuration

* verbosity 1
* MinibuffersPerFullbuffer 1
* PulseHeightFudgeFactor 0.003333
* DrawDebugPlots 0
* RunStartDate "23/01/2010_13:05:01"  # UK date format ;)
* PhaseOneRiffleShuffle 0  # whether to do phase 1 interleaving
* GenerateFakeRootFiles 0  # whether to generate phase 1 data format root files
* PutOutputsIntoStore 1    # whether to put data into BoostStores
