# SimpleReconstruction

The `SimpleReconstruction` tool performs a simple reconstruction of the muon vertex and energy properties based on the observed MRD track properties. The main principle is the back-propagation of the muon-track into the water tank volume based on the amount of observed light in the main detector. Please note that this method requires a stopping track in the MRD and does not work for any other event topologies.

## Input data

The tool first checks whether there was coincident tank and MRD activity by getting the "MRDCoincidenceCluster" variable from the ANNIEEvent store. It then checks whether there was a corresponding reconstructed MRD track which also stopped in the MRD. For this purpose, the MRD reconstruction variables are obtained from the "MRDTracks" BoostStore. 

In case there is a stopping MRD track found, the reconstruction proceeds and uses the energy loss in the MRD, the observed light level in the tank and fit parameters obtained in MC simulations in order to predict the muon energy:

```
E_mu [MeV] = 87.3MeV + 200 MeV/cm x dist_pmtvol_tank + Eloss(MRD) + Qmax*0.08534 MeV/p.e.
```

The vertex is subsequently reconstructed via

```
vtx = vtx(exit) - Qmax/9/200. x dir(track)
```

## Output data

The reconstruction properties are then added to the ANNIEEvent store directly via the following variables:
* SimpleRecoFlag (`int`): flags whether reconstruction was successful. 1 if successful, -9999 if not.
* SimpleRecoEnergy (`double`): reconstructed muon energy (in MeV)
* SimpleRecoVtx (`Position`): reconstructed muon vertex
* SimpleRecoStopVtx (`Position`): reconstructed muon stop position (per selection definition in the MRD)
* SimpleRecoCosTheta (`double`): reconstructed cos(theta) of muon direction with respect to beam direction (0,0,1)
* SimpleRecoFV (`bool`): was the reconstructed muon vertex in the Fiducial Volume?
* SimpleRecoMrdEnergyLoss (`double`): reconstructed energy loss in the MRD (in MeV)

Furthermore, the tool also adds the reconstructed muon as a `Particle` object to the Particles vector in the ANNIEEvent store.
* Particles (`std::vector<Particle>`): add muon as a `Particle` to the vector.

## Configuration variables

Currently, the `SimpleReconstruction` tool only has a verbosity setting, but no other configuration variables.

```
verbosity 1
```
