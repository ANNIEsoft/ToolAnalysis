# NeutronMultiplicity toolchain


# Purpose

The creation of neutron multiplicity analysis plots for ANNIE data and MC files.

# Tools

The following tools are present in the toolchain:

* `LoadANNIEEvent`: Loading the event
* `LoadGeometry`: Loading the geometry
* `TimeClustering`: Looking for time clusters in the MRD
* `FindMrdTracks`: Finding MRD tracks
* `ClusterFinder`: Looking for time clusters in the tank PMTs
* `ClusterClassifiers`: Classifying those tank PMT clusters
* `EventSelector`: Apply event selection cuts
* `SimpleReconstruction`: Simple MRD-track-based reconstruction of muon properties & vertex
* `FindNeutrons`: Find neutrons by specifying a desired selection cut. New selection cuts can easily be added.
* `NeutronMultiplicity`: Combine the information about the muon and the neutrons and create the neutron multiplicity plots.

# Usage

The output of the toolchain is either a ROOT-file or a BoostStore file with the relevant information about the neutrons and associated reconstructed muon information. If selecting the BoostStore option, the user can then read in all the necessary information for the toolchain from these NeutronMultiplicity BoostStore files at a later stage. This is especially useful when combining multiple runs.

The toolchain can either be used by reading in regular processed ANNIEEvent BoostStore files, by reading in filtered ANNIEEvent BoostStore files, or by reading in NeutronMultiplicity BoostStore files (as mentioned above). 

The toolchain for reading in multiple NeutronMultiplicity BoostStore files and creating an overall NeutronMultiplicity BoostStore+ROOT output file can be found at `configfiles/NeutronMultiplicity/ReadFromBoostStore/ToolChainConfig`.

