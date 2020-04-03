# EventDisplay

EventDisplay produces 2D Event Display plots of ANNIE events. Event Displays can be drawn showing the charge or the time distribution. Thresholds for the PMT and LAPPD charge as well as a time window for the shown events can be set in the config file (see Configuration for details).

The user can specify whether he wants to access the PMT/LAPPD information from the `ANNIEEvent` store or from the digits in the `RecoEvent` store. The MRD information is always read out from the `ANNIEEvent` store. Furthermore, it is possible to only draw event displays for events that passed an event selection cut in the `EventSelector` tool. Event Displays can be saved as images or as canvases in root-files.

If the variables `UserInput` is set to `1`, it is possible to select which other event to show next after each Execute step. (`Event` should be set to -999 in this case)

`MarkerSize` defines the size of the PMTs in the event display. A value of 2 is recommended for a representative size of the PMTs.

If the true vertex and ring of the primary particles are to be shown, the `DrawRing` and `DrawVertex` options are to be set to 1.

It is possible to produce additional 1D time and charge histograms if `HistogramPlots` is set to 1.

## Data

EventDisplay needs access to the hit PMT information in `ANNIEEvent`/`RecoEvent`. It is possible to use the `EventDisplay` tool both on simulation and on real data, the read-in objects will then be either `Hit` or `MCHit` object derivatives. The variable `IsData` should be set in the configuration file to signalize whether we are looking at a data or MC file. 

**MCHits** `map<unsigned long, std::vector<MCHit>>`
* Takes this data from the `ANNIEEvent` store and loops over the hit times / charges for all PMTs

**MCLAPPDHits** `map<unsigned long, std::vector<MCLAPPDHit>>`
* Takes this data from the `ANNIEEvent` store and loops over the hit times / charges for all LAPPDs

**TDCData** `map<unsigned long, std::vector<MCHit>>`
* Takes this data from the `ANNIEEvent` store and loops over the hit times for the MRD PMTs

**Hits** `map<unsigned long, std::vector<Hit>>`
* Equivalent of `MCHits` for data

**LAPPDHits** `map<unsigned long, std::vector<LAPPDHit>>`
* Equivalent of `MCLAPPDHits` for data

**TDCData_Data** `map<unsigned long, std::vector<Hit>>`
* Equivalent of `TDCData` for data

**RecoDigits** `std::vector<RecoDigit>`
* Takes this data from the `RecoEvent` store and loops over all digits (times/charges for all PMTs + LAPPDs)

## Configuration

The following settings can be set in the EventDisplay config file to customize the Event Display

```
# EventDisplay config file

Event -999		#use -999 if want to loop over all events in file, otherwise the specific event number (0 ... N_events - 1)
Mode Charge		#select Display Mode (Charge / Time)
EventType ANNIEEvent	#choose Store format to read from. ANNIEEvent: Use MCHits/MCLAPPDHits variables & the true MCParticles variables, RecoEvent: use the RecoStore and the associated charge and time information in the Digitized Hits
EventList None 		#/ANNIECode/ToolAnalysis/exemplary_event_list.dat #to be used with Event -999
SelectedEvents 1	#show only events for which the EventSelector cuts were passed
Threshold_Charge 1	#choose threshold for events in charge (lower limit)
Threshold_ChargeLAPPD 1.		#same as threshold, but for LAPPDs
Threshold_TimeLow -10	#for time, use lower and upper limits, use -999 if min and max values should be taken from data
Threshold_TimeHigh 40
TextBox	1		#choose if TextBox with information about run & event should be displayed (0: not shown, 1: shown)
LAPPDsSelected 0	#if true, only the LAPPDs specified in LAPPDsFile will be used in the analysis. If false, all LAPPD hits will be displayed 
LAPPDsFile lappd_active.txt	#specify the LAPPD IDs that are active (one ID per line) 
DrawVertex 1		#true vertex is drawn
DrawRing 1		#true expected ring distribution is drawn
SavePlots 1		#decide whether to save plots as png/root or not
OutputFormat root	#options: root/image
HistogramPlots 0	#decide whether histogram plots (charge/time) are shown in addition to EventDisplay
MarkerSize 2		#size of PMT circles (default: 2)
UserInput 0		#If true, manually decide if next event shown
Graphics 0		#should a TApplication be launched?
OutputFile evdisplay_annieevent_beam_muon
DetectorConfiguration ANNIEp2v6	#specify the detector configuration used in the simulation (options e.g. ANNIEp2v2, ANNIEp2v4, ANNIEp2v6)
IsData 0    # Are we looking at a MC file (0) or a data file (1)?
HistogramConfig ./configfiles/EventDisplay/Data/histogram_config_calibration.txt # Configuration file for the histograms to be plotted (optional, otherwise default values are taken)

verbose 1
```
