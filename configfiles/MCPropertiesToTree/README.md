# MCPropertiesToTree toolchain #

*************************
## MCPropertiesToTree ##
*************************

This toolchain centers around the `MCPropertiesToTree` tool, which creates a ROOT tree containing true properties of the event. Such true properties include the PDG-codes, energies, positions and directions of primary and secondary events as well as the detector response (like how many p.e. were registered for each event). It is possible to additionally extract information from the corresponding GENIE files, if the first step of the simulation was performed with GENIE.

************************
## Tools in toolchain ##
************************

An exemplary list of tools for this toolchain is the one which follows. Please note that the `LoadGenie` tool should only be used in case there is a corresponding GENIE-file for the loaded WCSim-file.

* `LoadWCSim`
* `LoadWCSimLAPPD`
* `LoadGenieEvent`
* `MCParticleProperties`
* `MCRecoEventLoader`
* `ClusterFinder`
* `TimeClustering`
* `EventSelector`
* `MCPropertiesToTree`

*****************************************
## Configuration of MCPropertiesToTree ##
*****************************************

If GENIE-information should be loaded, the configuration parameter `HasGENIE` needs to be set to 1. Furthermore, the tool `LoadGenieEvent` needs to be executed after the `LoadWCSim`-tools. If those criteria are met, the additional genie-tree will also be saved within the ROOT-file.

```
OutFile mcproperties_genie_test.root  #Output ROOT file
SaveHistograms 1  #Should histograms be saved in addition to the tree in the ROOT-file
SaveTree 1  #Should the tree be saved to the ROOT-file
HasGENIE 1  #Is there GENIE event information available?
```
