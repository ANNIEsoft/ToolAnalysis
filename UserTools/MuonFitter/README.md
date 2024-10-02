#MuonFitter Tool Documentation
***********************
Created by: Julie He

Maintained by: James Minock 

Last updated: 10-2-2024
***********************

This Tool functions for vertex and energy reconstruction, with the primary purpose being vertex reconstruction. This Tool operates on a ML generated model that fits tank tracks according to MRD information.

This Tool is intended to act as a substitution to Michael's SimpleReconstruction Tool, filling "SimpleReco" values expected in PhaseIITreeMaker.

The vertex saved to "SimpleRecoVtx" is given in meters and is oriented such that the center of the tank is represented by (0,-0.1446,1.681).

There are additional variables created to be passed downstream including:

FittedTrackLengthInWater - estimated track length in water of tank

FittedMuonVertex - vertex of interaction assuming tank center at (0,0,0)

RecoMuonKE - estimated kinetic energy of reconstructed muon

Nlyrs - number of layers penetrated in MRD
