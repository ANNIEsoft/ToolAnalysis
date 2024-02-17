# FindNeutrons

The `FindNeutrons` tools looks for neutrons in the ANNIE data and saves them as `Particle` object in the "Particles" vector in the ANNIEEvent BoostStore.

## Input data

The tool expects the `ClusterChargeBalances` and `ClusterTotalPEs` objects to be present in the ANNIEEvent BoostStore. In order for them to be present, it is essential to add the `ClusterClassifiers` tool to the toolchain.

## Output data

The `FindNeutrons` tool defines `Particle` objects for the clusters that passed the specified neutron selection cuts. Since at this point many of the neutron properties (e.g. the vertex) are undefined and not defined by a fit, they are set to default values. Only the PDG value and the stop time are set correctly based on the observed properties:
* PDG value: 2112
* Stop Time: time at which the candidate cluster occured.

For each identified neutron candidate, a `Particle` object is defined and appended to the ANNIEEvent variable "Particles", which is a vector of all identified `Particle` objects.

In addition to the Particles vector, additional information about the clusters are also saved to the ANNIEEvent:
* Particles (`std::vector<Particle>`)
* ClusterIndicesNeutron (`std::vector<int>`): Indices of clusters which were identified as neutron candidates
* ClusterTimesNeutron (`std::vector<double>`): Times of neutron candidate clusters
* ClusterChargesNeutron (`std::vector<double>`): Charges (in p.e.) of neutron candidate clusters
* ClusterCBNeutron (`std::vector<double>`): Charge Balancce values of neutron candidate clusters
* ClusterTimes (`std::vector<double>`): Times of all clusters
* ClusterCharges (`std::vector<double>`): Charges of all clusters
* ClusterCB (`std::vector<double>`): Charge Balance values of all clusters

## Configuration options

Currently, the user can choose between three cut-based selection techniques for neutrons, "CB", "CBStrict", or "NHits10". The former uses a charge balance cut of CB < 0.4 and a maximum total charge of 150 p.e., whereas the strict cut also applies a combined cut in the CB - total charge plane. The "NHits10" method classifies all clusters with at least 10 hits as neutrons.

In addition to the selection cut, the user also has to provide the neutron efficiency map for the selected cut, in the form of a txt file. The txt file should be tab separated and should consist of 7 columns, indicating the port, height within the port, x-positiion, y-position, z-position, efficiency of the cut, and efficiency of the selected time window. The path to the efficiency map file needs to be provided with the `EfficiencyMapPath` keyword.

An exemplary efficiency map file with two data points (and very good efficiency) would look like this:

```
1	100	0	85.6	93.1	0.99	0.99
1	50	0	35.6	93.1	0.99	0.99
```

The options can be chosen in the configuration file via

```
Method CB/CBStrict/NHits10
EfficiencyMapPath /path/to/file
```

Additional selection techniques can simply be added by adding a new keyword to the available Methods and implementing them in the code.
