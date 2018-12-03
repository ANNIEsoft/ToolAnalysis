# VtxSeedGenerator

VtxSeedGenerator

## Data

Describe any data formats VtxSeedGenerator creates, destroys, changes, or analyzes. E.G.

**RawLAPPDData** `map<Geometry, vector<Waveform<double>>>`
* Takes this data from the `ANNIEEvent` store and finds the number of peaks

## Configuration

Describe any configuration variables for VtxSeedGenerator.

```
	m_variables.Get("SeedType",fSeedType);
	m_variables.Get("NumberOfSeeds", fNumSeeds);
	m_variables.Get("verbosity", verbosity);
	m_variables.Get("UseSeedGrid", UseSeedGrid);

SeedType (int)
NumberOfSeeds (int)
verbosity (int)
UseSeedGrid (bool)

SeedType specifies whether to use PMTs, LAPPDs, or all. 
SeedType 0: Use only PMTs for calculating median seed time
SeedType 1: User only LAPPDs for calculating median seed time
SeedType 2: Use both the LAPPDs and the PMTs
 
NumberOfSeeds specifies how many points to generate in the grid, or how
many seeds to predict using the quad fitting technique.

If UseSeedGrid is used, vertex seeds are generated evenly through the ANNIE
cylinder.  The vertex time is the median of the time distribution calculated
extrapolating each hit back to the vertex position via speed of light in the
medium.

```
