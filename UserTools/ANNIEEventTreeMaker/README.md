# ANNIEEventTreeMaker

ANNIEEventTreeMaker

## Data

Describe any data formats ANNIEEventTreeMaker creates, destroys, changes, or analyzes. E.G.

**RawLAPPDData** `map<Geometry, vector<Waveform<double>>>`
* Takes this data from the `ANNIEEvent` store and finds the number of peaks


## Per-hit MC truth

`DirectParent_MCHit_fill 1` (default 0) writes `BackTracker`'s per-hit truth maps (`UseDirectParentClockTickMatching 1`) as one entry per reconstructed PMT pulse: `DirectParent_PMTID`, `DirectParent_HitTime`, `DirectParent_TrackIDs`, `DirectParent_PDGs`, `DirectParent_IsDarknoise`, `DirectParent_NeutronAncestor{TrackID,PDG,Class}`, `DirectParent_NeutronParent{TrackID,PDG}`, `DirectParent_ImmediateAncestor{TrackID,PDG,Class}`, `DirectParent_RootAncestor{TrackID,PDG}`, `DirectParent_Lineage{TrackID,PDG,Depth,Status}`. See the `BackTracker` README for their meaning.

## Configuration

Describe any configuration variables for ANNIEEventTreeMaker.

```
param1 value1
param2 value2
```
