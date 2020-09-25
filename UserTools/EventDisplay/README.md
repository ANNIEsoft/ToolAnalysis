# EventDisplay

EventDisplay produces 2D Event Display plots of ANNIE events. Event Displays can be drawn showing the charge or the time distribution. Thresholds for the PMT and LAPPD charge as well as a time window for the shown events can be set in the config file (see Configuration for details).

The user can specify whether he wants to access the PMT/LAPPD information from the `ANNIEEvent` store or from the digits in the `RecoEvent` store. The latter option is helpful if one wants to use other tools of the reconstruction chain like the vertex fitter or the HitCleaner. The MRD information is always read out from the `ANNIEEvent` store. 

Furthermore, it is possible to only draw event displays for events that passed an event selection cut in the `EventSelector` tool. Event Displays can be saved as images or as canvases in root-files.

If the variables `UserInput` is set to `1`, it is possible to select which event number to show next after each Execute step. (`Event` should be set to -999 in this case)

`MarkerSize` defines the size of the PMTs in the event display. A value of 2 is recommended for a representative size of the PMTs.

If the true vertex and ring of the primary particles are to be shown, the `DrawRing` and `DrawVertex` options are to be set to 1.

It is possible to produce additional 1D time and charge histograms if `HistogramPlots` is set to 1.

If one wants to draw only clusters of hits as found by the `ClusterFinder` (Tank) and `TimeClustering` (MRD) tools, the options `DrawClusterPMT`/`DrawClusterMRD` should be set to 1.

EventDisplays of data can show the charge of events in different units. The variable `ChargeFormat` can be set to `pe` or `nC`.

The variables `RunType` and `RunNumber` are primarily intended for runs that don't contain the RunInformation in form of a BoostStore and hence need this information to be added by hand.

If one wants to plot only hits which passed the selection cuts in the `HitCleaner` tool, one should set `UseFilteredDigits` to 1.

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

**m_all_clusters** `std::map<double,std::vector<Hit>>`
* Hit clusters as found by the `ClusterFinder` tool.

**MRDTimeClusters** `std::vector<std::vector<int>>`
* MRD time clusters as found by the `TimeClustering` tool.

## Configuration

The following settings can be set in the EventDisplay config file to customize the Event Display

```
# EventDisplay config file

# EventDisplay config file

Event -999		#use -999 if want to loop over all events in file, otherwise the specific event number (0 ... N_events - 1)
Mode Charge		#select Display Mode (Charge / Time)
EventType RecoEvent	#choose Store format to read from. ANNIEEvent: Use MCHits/MCLAPPDHits variables & the true MCParticles variables, RecoEvent: use the RecoStore and the associated charge and time information in the Digitized Hits
EventList None 		#/ANNIECode/ToolAnalysis/exemplary_event_list.dat #to be used with Event -999
SelectedEvents 0	#show only events for which the EventSelector cuts were passed
Threshold_Charge 30	#choose threshold for events in charge (lower limit)
Threshold_ChargeLAPPD 1.		#same as threshold, but for LAPPDs
Threshold_TimeLow -999	#for time, use lower and upper limits, use -999 if min and max values should be taken from data
Threshold_TimeHigh -999
Threshold_TimeLowMRD 0	#time threshold for MRD display
Threshold_TimeHighMRD 4000
TextBox	1		#choose if TextBox with information about run & event should be displayed (0: not shown, 1: shown)
LAPPDsSelected 1	#if true, only the LAPPDs specified in LAPPDsFile will be used in the analysis. If false, all LAPPD hits will be displayed 
LAPPDsFile LAPPDIDs_Data.txt	#specify the LAPPD IDs that are active (one ID per line) 
DrawVertex 1		#true vertex is drawn
DrawRing 1		#true expected ring distribution is drawn
SavePlots 1		#decide whether to save plots as png/root or not
OutputFormat root	#options: root/image
HistogramPlots 1	#decide whether histogram plots (charge/time) are shown in addition to EventDisplay
MarkerSize 2		#size of PMT circles (default: 2)
UserInput 0		#If true, manually decide if next event shown
Graphics 0		#should a TApplication be launched?
OutputFile EvDisplay_R1611S0p1_Charge_Test_HitCleaner_5pe
DetectorConfiguration ANNIEp2v6	#specify the detector configuration used in the simulation (options e.g. ANNIEp2v2, ANNIEp2v4, ANNIEp2v6)
IsData 1		#Are we evaluating a data file? (0: MC, 1: data)
HistogramConfig ./configfiles/EventDisplay/Data-RecoEvent/histogram_config_calibration.txt  #Configuration file specifying the range of the histograms
NPMTCut 4		#Only look at events with > NPMTCut PMTs hit above threshold
DrawClusterPMT 1	#Draw Clustered PMT hits instead of whole buffer
DrawClusterMRD 1	#Draw Clustered MRD hits instead of whole buffer
ChargeFormat pe		#Options: pe/nC
SinglePEGains ./configfiles/EventDisplay/Data-ANNIEEvent/ChannelSPEGains_BeamRun20192020.csv
RunNumber 1611
RunType 4
UseFilteredDigits 1	#Use only digits that passed the HitCleaner filtering process

verbose 1
```
