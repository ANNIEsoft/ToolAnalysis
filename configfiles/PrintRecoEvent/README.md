# PrintRecoEvent ToolChain

***********************
# Description
**********************

The `PrintRecoEvent` toolchain prints out information that is stored in the `m_data->Stores["RecoEvent"]` BoostStore. The `RecoEvent` BoostStore contains information about the characteristics of the reconstruction and in case of MC also some truth information from the simulation. Depending on how many reconstruction tools are in the toolchain, a different level of detail will be presented when using `PrintRecoEvent`.

************************
# Usage
************************

All reconstruction tools rely on a data format of `RecoDigit` objects, it is hence important to run the tool `DigitBuilder` first. 

Exemplary toolchain for data without vertex reconstruction:

* LoadGeometry
* LoadANNIEEvent
* PhaseIIADCCalibrator
* PhaseIIADCHitFinder
* ClusterFinder
* DigitBuilder
* HitCleaner
* TimeClustering
* EventSelector
* PrintRecoEvent
