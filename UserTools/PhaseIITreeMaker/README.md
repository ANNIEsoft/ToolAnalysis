# PhaseIITreeMaker

PhaseIITreeMaker

## Data

Describe any data formats PhaseIITreeMaker creates, destroys, changes, or analyzes. E.G.

**RawLAPPDData** `map<Geometry, vector<Waveform<double>>>`
* Takes this data from the `ANNIEEvent` store and finds the number of peaks

## Configuration

Describe any configuration variables for PhaseIITreeMaker.

```
verbose (1 or 0)
Defines the level of verbosity for outputs of PhaseIITreeMaker algorithm.

muonMCTruth_fill (1 or 0)
Input will determine if Truth information from files given is saved to the
reco tree.  Will output to tree if 1.

muonTruthRecoDiff_fill (1 or 0)
Input determines if the difference in truth and reco information is saved to
the reco tree.  Will output to tree if 1.

muonRecoDebug_fill (1 or 0)
Input determines if reconstruction variables at each step in the muon event
reconstruction chain are saved to the tree.  Values include seeds from SeedVtxFinder,
fits from PointPosFinder, and FOMs for likelihood fits at each reconstruction step.
Will output to tree if 1.

param1 value1
param2 value2
```
