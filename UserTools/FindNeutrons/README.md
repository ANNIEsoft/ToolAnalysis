# FindNeutrons

The `FindNeutrons` tools looks for neutrons in the ANNIE data and saves them as `Particle` object in the "Particles" vector in the ANNIEEvent BoostStore.

## Input data

The tool expects the `ClsuterChargeBalances` and `ClusterTotalPEs` objects to be present in the ANNIEEvent BoostStore. In order for them to be present, it is essential to add the `ClusterClassifiers` tool to the toolchain.

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

Currently, the user can choose between two cut-based selection techniques for neutrons, "CB" or "CBStrict". The former uses a charge balance cut of CB < 0.4 and a maximum total charge of 150 p.e., whereas the strict cut also applies a combined cut in the CB - total charge plane. The method can be chosen in the configuration file via

```
Method CB/CBStrict
```

Additional selection techniques can simply be added by adding a new keyword to the available Methods and implementing them in the code.
