# LoadNUISANCEEvent

The `LoadNUISANCEEvent` tool loads information from the NUISANCE files about the neutrino interaction properties into a custom "NuisanceInfo" BoostStore that can be accessed by other tools.

## Configurations ##

It is possible to look at NUISANCE files on their own (without corresponding WCSim files), in this case the `FileDir` and `FilePattern` need to be specified in the configuration file.
If one wants to get corresponding NUISANCE information for WCSim files, one should specify `LoadWCSimTool` in the `FilePattern` row. In this case the tool will try to extract the information about the corresponding NUISANCE file from the WCSim file and load the respective NUISANCE file automatically. For newer files, the path is SOMETIMES(*see below*) saved alongside the filename and one can set the `FileDir` to `NA`. For older files, only the filename is saved and one needs to specify the `FileDir` in which the NUISANCE files are to be found by hand.
Note that a lot of WCSim files do not have the complete information about their NUISANCE files saved. In this case, a manual matching of NUISANCE files to WCSim files is possible, although the following restrictions to the naming apply: The WCSim files must have the same nomenclature as Marcus' WCSim beam files, i.e. `wcsim_0.X.Y.root`, where `X` is the number of the corresponding NUISANCE file, and `Y` specifies which part of the NUISANCE file is being looked at, with each WCSim file corresponding to 500 entries (1000 entries for James' files) in a NUISANCE file. The matching NUISANCE file would be called `ghtp.X.ghep.root`, with the events `Y*(500) ... (Y+1)*500` (`Y*(1000) ... (Y+1)*1000`) corresponding to the events in the WCSim file. 
If the WCSim file was generated from offset NUISANCE events (a non-zero Y in the WCSim file name), then the WCSim only saved the file path of the NUISANCE file, not the filename. IT IS STRONGLY RECOMMENDED to set the FileDir to the NUISANCE file location.

## NuisanceInfo BoostStore ##

The information is loaded from the NUISANCE file and saved into the "NuisanceInfo" BoostStore. The following variables are saved:

* **file** `string`: The NUISANCE filename
* **evtnum** `unsigned int`: The NUISANCE event number

* **IsQuasiElastic** `bool`: Neutrino interaction was quasi-elastic
* **IsResonant** `bool`: Neutrino interaction was RES
* **IsDeepInelastic** `bool`: Neutrino interaction was DIS
* **IsCoherent** `bool`: Neutrino interaction was COH
* **IsDiffractive** `bool`: Neutrino interaction was Diffractive
* **IsInverseMuDecay** `bool`: Neutrino interaction was Inverse Muon Decay
* **IsIMDAnnihilation** `bool`: Neutrino interaction was Inverse Muon Decay - Annihilation
* **IsSingleKaon** `bool`: Neutrino interaction was Single Kaon (?)
* **IsEM** `bool`: Interaction process was electromagnetic
* **IsWeakCC** `bool`: Interaction process was weak (CC)
* **IsWeakNC** `bool`: Interaction process was weak (NC)
* **IsMEC** `bool`: Interaction process involved Meson Exchange Currents (MEC)
* **InteractionTypeString** `string`: Interaction type
* **NeutCode** `int`: Neutrino code describing the interaction (not filled currently)
* **NuIntVtx_X/Y/Z** `double`: Neutrino interaction vertex (x/y/z)
* **NuIntVtx_T** `double`: Neutrino interaction vertex (time)
* **NuVtxInTank** `bool`: Was neutrino vertex in the ANNIE tank?
* **NuVtxInFidVol** `bool`: Was neutrino vertex in the Fiducial Volume of ANNIE?
* **EventQ2** `double`: Q^2-value of the interaction
* **NeutrinoEnergy** `double`: Neutrino energy
* **NeutrinoMomentum** `Direction`: Neutrino momentum
* **NeutrinoPDG** `double`: PDG code of neutrino
* **MuonEnergy** `double`: Energy of produced muon
* **MuonAngle** `double`: Angle of produced muon
* **FSLeptonName** `string`: Final State Lepton name
* **FSLeptonEnergy** `double`: Final State Lepton energy
* **FSLeptonPdg** `int`: Final State Lepton PDG code
* **FSLeptonMass** `double`: Final State Lepton mass
* **FSLeptonMomentum** `Position`: Final State Lepton momentum vector
* **FSLeptonMomentumDir** `Position`: Final State Lepton momentum unit vector
* **FSLeptonVertex** `Position`: Final State Lepton initial vertex
* **FSLeptonTime** `double`: Final State Lepton time of initial vertex
* **NumFSProtons** `int`: Number of final state protons
* **NumFSNeutrons** `int`: Number of final state neutrons
* **NumFSPi0** `int`: Number of final state pi^0
* **NumFSPiPlus** `int`: Number of final state pi^+
* **NumFSPiPlusCher** `int`: Number of final state pi^+ that pass Cherenkov threshold
* **NumFSPiMinus** `int`: Number of final state pi^-
* **NumFSPiMinusCher** `int`: Number of final state pi^- that pass Cherenkov threshold
* **NumFSKPlus** `int`: Number of final state K^+
* **NumFSKPlusCher** `int`: Number of final state K^+ that pass Cherenkov threshold
* **NumFSKMinus** `int`: Number of final state K^-
* **NumFSKMinusCher** `int`: Number of final state K^- that pass Cherenkov threshold
* **NuisanceInfo** `NuisanceInfo`: NuisanceInfo object containing most of the listed properties (see DataModel header-file)

## Configuration file ##

LoadNUISANCEEvent has the following configuration options:

```
verbosity 1
#FileDir NA     #specify "NA" for newer files: full path is saved in WCSim
FileDir /pnfs/annie/persistent/simulations/genie3/G1810a0211a/standard/tank
#FilePattern nui.*.root  ## for specifying specific files to load
FilePattern LoadWCSimTool      ## use this pattern to load corresponding nuisance info with the LoadWCSimTool
                               ## N.B: FileDir must still be specified for now!
ManualFileMatching 0           ## to manually match NUISANCE event to corresponding WCSim event
FileEvents 1000                ## number of events in the WCSim file
                               ## 1000 for James files
```
