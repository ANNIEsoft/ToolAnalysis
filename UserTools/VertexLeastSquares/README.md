# VertexLeastSquares

VertexLeastSquares is a vertex reconstruction tool that relies only on PMT hit timing. It recommended for use in low energy events that do not have an MRD track to assist in reconstruction. It utilizes a matrix least squares method for vertexing interactions. It also does hit filtering / cleaning prior to reconstruction to only fit the set of PMT hits compatible with the assumption all light in the event originated from a single point. Prior to fitting, it populates the tank with seeds, then uses the Gauss-Newton method to update an initial guess until a global minimum is found. The tool will return the best-fit vertex position to the available PMT hits information. 

## Data

**VertexLeastSquaresMap** `std::map<double, Position>`
* Stored in `ANNIEEvent`, the reconstructed vertex (x,y,z) [m] for a cluster. The vertices share the same coordinate system as WCSim.
* To access the reconstructed vertices:
```
# grab vertex map
m_data->Stores.at("ANNIEEvent")->Get("VertexLeastSquaresMap", fVertexMap);

# index the map with the cluster time for a given cluster
Position vertex = fVertexMap->at(cluster_time);      # Position is a ToolAnalysis class

# grab vertex
recoVtxX = vertex.X() // [m]
recoVtxY = vertex.Y() // [m]
recoVtxZ = vertex.Z() // [m]
```
* To shift the vertex positions back to a coordinate system with the ANNIE tank center at (0,0,0):
```
recoVtxX = (vertex.X() + 0)      // [m]
recoVtxY = (vertex.Y() + 0.1446) // [m]
recoVtxZ = (vertex.Z() - 1.681)  // [m]
```

Note that the reconstructed vertex is given as the best fit to the hit times within a cluster. When comparing this information to GENIE, any reconstructed, delayed clusters will necessarily be "off" from the true interaction vertex. If neutrons, the reconstructed vertex could be compared to the neutron capture point instead.

If a suitable vertex is not found, the map is populated with `-99`.

**VertexLeastSquaresStdevMap** `std::map<double, double>`
* Stored in `ANNIEEvent`, the stdev from the timing residuals of the fitted vertex [unitless].

* To access the stdev of the fitted vertex:
```
# grab stdev
m_data->Stores.at("ANNIEEvent")->Get("VertexLeastSquaresStdevMap", fVertexStdevMap);

# index the map with the cluster time for a given cluster
fVertexStdevMap->at(cluster_time);
```

The stdev will also get a value of `-99` if a suitable vertex is not found.

## Configuration

```
verbosity 0
UseMCHits 0                     # MC hits (1) or data (0). For PMTWaveformSim, use data (0)

BreakDist 0.05                  # how close two guesses have to be in order to end the vertex search [m]
Regularizer 1                   # Tikhonov (L2) regularization term --> controls how far vertices are allowed to wander away from the center
FittingSteps 100                # how many iterations the algorithm will run through

YSpacing 0.5                    # vertical spacing of the sunflower seed layers [m]
NPlanarPoints 20                # how many seeds are arranged in a sunflower pattern per layer

FilterHitsCompatibleWindow 0    # (0) = keep first 4 hits as "seed" hits, then keep later hits only if they are casually disconnected
                                # (1) = search for the most compatible (causally disconnected) set of hits with a sliding time window

OnlyUseReliablePMTs 0           # mask out PMTs with poor timing uncertainties (determined from the laser analysis)
MinimumReliableTimingStd 1.5    # maximum timing uncertainty for PMT masking

ExternalSeeding 0               # Add extend seeding outside tank (to help reconstruct external events), and allow guesses to wander outside tank volume
YBuffer 1.0                     # additional (Y) buffer height added to the seeding volume (top and bottom, with respect to center) [m]
RadialBuffer 1.0                # additional radial distance added to the seeding volume (with respect to center) [m]
ExternalVertexMaxRadius 4.0     # maximum allowable radial distance a guess is allowed to be from the tank center (if external seeding is enabled) [m]
ExternalVertexMaxHeight 4.0     # maximum allowable Y distance a guess is allowed to be from the tank center (if external seeding is enabled) [m]
```

## Working toolchain

For the parametric MC hits:
```
LoadGeometry
LoadWCSim
ClusterFinder
VertexLeastSquares
```

For the waveform MC hits:
```
LoadGeometry
LoadWCSim
PMTWaveformSim
PhaseIIADCHitFinder
ClusterFinder
VertexLeastSquares
```

For the data:
```
LoadANNIEEvent
LoadGeometry
ClusterFinder
VertexLeastSquares
```

### Additional Information
More information on the tool can be found here: https://annie-docdb.fnal.gov/cgi-bin/sso/ShowDocument?docid=5621 