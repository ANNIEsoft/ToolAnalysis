# EventDisplay toolchain configuration

***********************
# Description
**********************

The `EventDisplay` toolchain is supposed to provide a graphic representation of events in the ANNIE detector. Shown are the tank data (PMTs + LAPPDs, center), the MRD data (lower right + lower left) as well as a text box with general information (top left) and a schematic view which parts of the detector saw a hit (top right).

The three exemplary toolchains in this directory show how to use the tool in order to display either `ANNIEEvent` information or `RecoEvent` information both for MC and data.

It is possible to run the EventDisplay toolchain in a mode where one can select which event to display next after each event. The configuration variable `UserInput` needs to be set to 1 to enable this mode of operation.

If additional histograms about the time & charge behavior of the event are to be drawn, the variable `HistogramPlots` is to be set to 1. Thresholds in time & charge can be set for LAPPDs, tank PMTs and MRD PMTs separately. It is possible to only draw clusters of hits by setting `DrawClusterPMT/DrawClusterMRD` to 1. In order to use events cleaned by the `HitCleaner` tool, the variable `UseFilteredDigits` needs to be set to 1. Events can be displayed in units p.e. or nC, to be specified via the variable `ChargeFormat`. Custom RunNumber & RunTypes can be set in case the information was not stored propertly in the raw file.

************************
# Tools in EventDisplay toolchain
************************

The recommended sequence of tools is (MC)

* LoadWCSim
* LoadWCSimLAPPD
* MCParticleProperties
* MCRecoEventLoader
* DigitBuilder
* HitCleaner
* EventSelector
* EventDisplay

The tools which are executed before `EventDisplay` ensure the availability of additional information to be displayed. The `EventSelector` e.g. provides the possibility to select only specific types of events that are to be displayed. The user can decide in the configuration file of `EventDisplay` whether he wants to access the PMT and LAPPD information from the `ANNIEEvent` store or from the `RecoEvent` store. The MRD information is always accessed from the `ANNIEEvent` store. The configuration file of `EventDisplay` also gives the choice whether to save the event displays as images (jpg/png) or as canvases in a root file.

The recommended sequence of tools for data is

* LoadGeometry
* LoadANNIEEvent
* PhaseIIADCCalibrator
* PhaseIIADCHitFinder
* ClusterFinder
* TimeClustering
* DigitBuilder
* HitCleaner
* EventSelector
* EventDisplay

The tools `DigitBuilder`,`HitCleaner` and `EventSelector` are mostly necessary if one wants to read out the information from the reco event store and perform some reconstruction before passing the information to the `EventDisplay` tool.
