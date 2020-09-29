# LoadGenieEvent

The `LoadGenieEvent` tool loads information from the GENIE files about the neutrino interaction properties into a custom "GenieInfo" BoostStore that can be accessed by other tools.

## Configurations ##

It is possible to look at GENIE files on their own (without corresponding WCSim files), in this case the `FileDir` and `FilePattern` need to be specified in the configuration file.
If one wants to get corresponding GENIE information for WCSim files, one should specify `LoadWCSimTool` in the `FilePattern` row. In this case the tool will try to extract the information about the corresponding GENIE file from the WCSim file and load the respective GENIE file automatically. For newer files, the path should be saved alongside the filename and one can set the `FileDir` to `NA`. For older files, only the filename is saved and one needs to specify the `FileDir` in which the GENIE files are to be found by hand.
Note that a lot of WCSim files do not have the complete information about their GENIE files saved. In this case, a manual matching of GENIE files to WCSim files is possible, although the following restrictions to the naming apply: The WCSim files must have the same nomenclature as Marcus' WCSim beam files, i.e. `wcsim_0.X.Y.root`, where `X` is the number of the corresponding GENIE file, and `Y` specifies which part of the GENIE file is being looked at, with each WCSim file corresponding to 500 entries in a GENIE file. The matching GENIE file would be called `ghtp.X.ghep.root`, with the events `Y*(500) ... (Y+1)*500` corresponding to the events in the WCSim file. 

## GenieInfo BoostStore ##

The information is loaded from the GENIE file and saved into the "GenieInfo" BoostStore. The following variables are saved:

* **file** `string`: The GENIE filename
* **fluxver** `int`: Flux version number (0/1)
* **evtnum** `unsigned int`: The GENIE event number
* **ParentPdg** `int`: PDG code of parent particle that produced neutrino
* **ParentTypeString** `string`: The type of the parent particle that produced neutrino
* **ParentDecayMode** `int`: The decay mode of the parent particle that produced the neutrino
* **ParentDecayVtx** `Position`: The decay vertex of the parent particle that produced the neutrino
* **ParentDecayVtx_X/Y/Z** `float`: The x/y/z component of the parent particle decay vertex
* **ParentDecayMom** `Position`: The momentum of the parent particle that produced the neutrino
* **ParentDecayMom_X/Y/Z** `float`: The x/y/z/ component of the parent particle decay momentum
* **ParentProdMom** `Position`: The momentum of the parent particle at production
* **ParentProdMom_X/Y/Z** `float`: The x/y/z/ component of the parent particle production momentum
* **ParentProdMedium** `int`: Gnumi code for material where parent particle was produced
* **ParentProdMediumString** `string`: Material where parent particle was produced
* **ParentPdgAtTgtExit** `int`: PDG code of parent particle at exit of target
* **ParentTypeAtTgtExitString** `string`: Name of parent particle at exit of target
* **ParentTgtExitMom** `Position`: momentum of parent particle at exit of target
* **ParentTgtExitMom_X/Y/Z** `float`: x/y/z component of parent particle momentum at exit of target

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
* **NeutrinoPDG** `double`: PDG code of neutrino
* **MuonEnergy** `double`: Energy of produced muon
* **MuonAngle** `double`: Angle of produced muon
* **FSLeptonName** `string`: Final State Lepton name
* **NumFSProtons** `int`: Number of final state protons
* **NumFSNeutrons** `int`: Number of final state neutrons
* **NumFSPi0** `int`: Number of final state pi^0
* **NumFSPiPlus** `int`: Number of final state pi^+
* **NumFSPiMinus** `int`: Number of final state pi^-
* **NumFSKPlus** `int`: Number of final state K^+
* **NumFSKMinus** `int`: Number of final state K^-
* **GenieInfo** `GenieInfo`: GenieInfo object containing most of the listed properties (see DataModel header-file)

## Configuration file ##

LoadGenieEvent has the following configuration options:

```
verbosity 1
FluxVersion 0   #0: rhatcher files, 1: zarko files
#FileDir NA     #specify "NA" for newer files: full path is saved in WCSim
FileDir /pnfs/annie/persistent/users/vfischer/genie_files/BNB_Water_10k_22-05-17
#FileDir /pnfs/annie/persistent/users/moflaher/genie/BNB_World_10k_11-03-18_gsimpleflux
#FilePattern gntp.*.ghep.root  ## for specifying specific files to load
FilePattern LoadWCSimTool      ## use this pattern to load corresponding genie info with the LoadWCSimTool
                               ## N.B: FileDir must still be specified for now! (WCSim files do not record their directory)
ManualFileMatching 1           ## If the corresponding GENIE files are not saved reliably, a manual matching of WCSim files to GENIE files can take place
```
