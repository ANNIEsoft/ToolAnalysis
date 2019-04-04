# VtxExtendedVertexFinder

VtxExtendedVertexFinder

## Data

Runs the extended vertex finder using RecoDigits in the store.

The extended vertex finder uses the MinuitOptimizer and VertexGeometry classes
in the DataModel to reconstruct the muon vertex and direction.  Minuit searches
for the vertex position, time, and direction that minimizes the negative log likelihood
function formed using extended hit residuals. For any given RecoDigit, the 
extended hit residual is evaluated by assuming a muon travels along the vertex
direction at the speed of light, and that a photon left the track at the Cherenkov
angle to hit the Digit.


## Configuration

Describe any configuration variables for VtxExtendedVertexFinder.

```
verbosity int
Controls the level of information printed out while running ToolAnalysis.

UseTrueVertexAsSeed bool
If Using Monte Carlo data, the true muon position and direction are given to
Minuit as the seed for the extended vertex finder.

FitAllOnGridSeed bool
If 1, the Extended Vertex Fitter is executed using every position on the seed
grid generated with the VtxSeedFinder tool.  For each position, a direction seed
is generated using the "FindSimpleDirection" tool.  The fit that has the highest FOM
and converges in Minuit is accepted as the reconstructed vertex.

If the above two bools are false, the Extended Vertex Finder is executed assuming
that the usual full reconstruction chain has been executed.  Specifically, the
Extended Vertex Finder is ran using the PointVertexFinder's result as the seed.

```
