# NeutronMultiplicity

The `NeutronMultiplicity` creates the plots of the neutron multiplicity analysis and has the possibility of storing them to an output ROOT file or an output BoostStore file (or both).

## Input data

The `NeutronMultiplicity` tool can be operated in two modes: Either in a toolchain where the necessary reconstruction tools are included, or as a single-handed tool when the neutron reconstruction information was already saved beforehand in BoostStores. 

The complete toolchain (`configfiles/NeutronMultiplicity/ToolsConfig`) includes the following tools:
* LoadANNIEEvent
* LoadGeometry
* TimeClustering
* FindMrdTracks
* ClusterFinder
* ClusterClassifiers
* EventSelector
* SimpleReconstruction
* FindNeutrons
* NeutronMultiplicity

The single-handed toolchain relies on BoostStores created beforehand by the full toolchain mentioned above. If those are present, the `ReadFromBoostStore` toolchain (`configfiles/NeutronMultiplicity/ReadFromBoostStore/ToolChainConfig`) can be used.

## Output data

The neutron multiplicity tool will fill the information about the neutron multiplicity and the muon properties in histograms and graphs and save them to the output ROOT file. The output ROOT file is structed into the following directories:
* hist_muon: Histograms about the reconstructed muon properties
* hist_neutron: Histograms about the reconstructed neutron properties
* hist_mc: Histograms about MC specific properties
* hist_eff: Histograms about the neutron detection efficiency as a function of muon properties
* graph_neutron: Resulting graphs showing the neutron multiplicity as a function of the muon properties
* graph_neutron_mc: Resulting graphs showing the TRUE neutron multiplicity as a function of the RECONSTRUCTED muon properties. True neutron multiplicity is divided into three categories: primary (number of primary neutrons), total (number of neutron captures from all primary + secondary neutrons in the whole experiment volume), pmtvol (primary + secondary neutron captures, but restricted to the water volume)
* graph_neutron_eff: Efficiency graphs
* graph_neutron_corr: Neutron multiplicity graphs, corrected by efficiency graphs
* neutron_tree: the neutron_tree saves the information additionally in the form of a TTree for more interactive investigations

## Configuration variables

The configuration options for the `NeutronMultiplicity` tool are listed below:

```
verbosity 1        #verbosity setting

SaveROOT 0/1       
#should an output ROOT file be produced? 
#This ROOT file will contain all the histograms, graphs, and a tree

SaveBoostStore 0/1 
#Should an output BoostStore be produced?
#The BoostStore will store all the necessary variables needed to create the plots in an output BoostStore file
#The BoostStores can then be read in later via the ReadFromBoostStore option (see below)
#Saving to a BoostStore is very convenient since it will allow to first run the ToolChain for each run separately, and then read in all the BoostStores again afterwards to combine all the information

Filename filename
#This is the output filename string
#For output rootfiles, .root will be appended, for output BoostStore files, .bs will be appended

ReadFromBoostStore None/filelist.txt
#This option can be used to read the neutron multiplicity information from a list of BoostStore files
#If this option is selected, make sure to only have the NeutronMultiplicity tool in your toolchain, no LoadANNIEEvent or reconstruction tools (since the reconstruction information is already saved to the input BoostStores)

MRDTRackRestriction 0/1
#Selection variable which describes whether only events with 1 MRD track should be considered for the analysis.
```
