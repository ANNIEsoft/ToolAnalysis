# EventDisplay toolchain configuration

***********************
# Description
**********************

The `EventDisplay` toolchain is supposed to provide a graphic representation of events in the ANNIE detector. Shown are the tank data (PMTs + LAPPDs, center), the MRD data (lower right + lower left) as well as a text box with general information (top left) and a schematic view which parts of the detector saw a hit (top right).

The three exemplary toolchains in this directory show how to use the tool in order to display either `ANNIEEvent` information or `RecoEvent` information (MC) as well as how to process a data file.

It is possible to run the EventDisplay toolchain in a mode where one can select which event to display next after each event. The configuration variable `UserInput` needs to be set to 1 to enable this mode of operation.

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
* EventDisplay
