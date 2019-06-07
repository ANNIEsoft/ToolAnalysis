# VtxPointPositionFinder

VtxPointPositionFinder

## Data

Describe any data formats VtxPointPositionFinder creates, destroys, changes, or analyzes. E.G.

**RawLAPPDData** `map<Geometry, vector<Waveform<double>>>`
* Takes this data from the `ANNIEEvent` store and finds the number of peaks

## Configuration

Describe any configuration variables for VtxPointPositionFinder.

```
UseTrueVertexAsSeed bool
If true, any information from the seed list is not used.  The True MC Vertex will instead be used
as the input to the Minuit-based Point Position fitting algorithm.

UseMinuitForPos bool
If true, Minuit is used to vary both the position and time to find the highest point position
FOM.  Initial fit parameters will be the vertex seed the highest FOM, or the True MC Vertex.

If false, the seed with the highest FOM (whose vertex time was fit using Minuit) will be returned
 as the point position vertex.

If both of the above are false, the Point Position vertex is set equal to the true MC Vertex.

```
