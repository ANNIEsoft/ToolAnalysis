# EventDisplay

EventDisplay produces 2D Event Display plots of ANNIE events. Event Displays can be drawn showing the charge or the time distribution. Thresholds for the PMT and LAPPD charge as well as a time window for the shown events can be set in the config file (see Configuration for details).

## Data

EventDisplay needs access to the hit PMT information in ANNIEEvent. So far, it is only used on simulation therefore needing access to

**MCParticles** `map<ChannelKey, std::vector<Hit>>`
* Takes this data from the `ANNIEEvent` store and loops over the hit times / charges for all PMTs

**MCParticlesLAPPD** `map<ChannelKey, std::vector<LAPPDHit>>`
* Takes this data from the `ANNIEEvent` store and loops over the hit times / charges for all LAPPDs

## Configuration

The following settings can be set in the EventDisplay config file to customize the Event Display

```
Event -999		#use -999 if want to loop over all events in file, otherwise the specific event number (0 ... N_events - 1)
Mode Time		#select Display Mode (Charge / Time)
Format Simulation	#choose Store format to read from. Simulation: use ANNIEEvent variables with the true MCParticles variables, Reco: use the RecoStore and the associated charge and time classes
Threshold_Charge 40		#choose threshold for events in charge (lower limit)
EventList None #/ANNIECode/ToolAnalysis/exemplary_event_list.dat #possibility to plot only certain Event Numbers within a WCSim file, specified in a text file
Threshold_ChargeLAPPD 1.		#same as threshold, but for LAPPDs 
Threshold_TimeLow -999	#for time, use lower and upper limits, use -999 if min and max values should be taken from data
Threshold_TimeHigh -999
TextBox	1		#choose if TextBox with information about run & event should be displayed (0: not shown, 1: shown)
LAPPDsSelected 0	#if true, only the LAPPDs specified in LAPPDsFile will be used in the analysis. If false, all LAPPD hits will be displayed 
LAPPDsFile lappd_active.txt	#specify the LAPPD IDs that are active (one ID per line) 
DrawVertex 1            #true vertex is drawn
DrawRing 1              #true expected ring shape is drawn
SavePlots 0		#decide whether to save plots as png or not
HistogramPlots 0	#decide whether histogram plots (charge/time) are shown in addition to EventDisplay
UserInput 0             #manually decide if next event shown
Graphics 0              #should a TApplication be launched?
OutputFile electrons_iso	#output file name for saved Event Display plots	
DetectorConfiguration ANNIEp2v6 #specify the detector configuration used in the simulation (options e.g. ANNIEp2v2, ANNIEp2v4, ANNIEp2v6)

verbose 1
```
