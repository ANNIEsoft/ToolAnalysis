#include "LoadGenieEvent.h"
//GENIE
#include <FluxDrivers/GSimpleNtpFlux.h>
#include <FluxDrivers/GNuMIFlux.h>

/*
GENIE INPUTS
============

Robert's Files
--------------
1) gnumi files: /pnfs/annie/persistent/flux/bnb/bnb_annie_0000.root
- these are on parallel with the gsimple files but store less information
- they feed into genie and are used to generate neutrino rays

Zarko's Files
-------------
There are 2 stages of files:
1) ReDecay files: /annie/data/flux/redecay_bnb/beammc_annie_0000.root
- these are the topmost BNB flux files, containing all BNB simulation errors
- these files are used to generate gsimple ntuples
- events in this ntuple have weighting according to systematic errors on generation parameters
- there are 1000 weights for 7 systematics:
K+, K-, K0, pi+, pi-, beamunisims, and total (product of other 6)
beamunisims contains systematics due to horn current miscal, skin depth, variations in total, qe and inelastic cross section of pi on Be and Al
- you can use these to do systematic studies

2) gsimple files: /annie/data/flux/gsimple_bnb/gsimple_beammc_annie_0000.root
- the flux rom the re-decay files is propagated to a detector window
- weights are stripped as genie cannot propagate them
- the resulting gsimple files describe neutrino rays that feed into genie
- zarko's above files have a window of 20x20m, 20m upstream of detector origin at (0,0,100.35m)
  (the origin of detector coordinate system in beam coordinate system)

GENIE OUTPUTS
=============

Robert's Files
--------------
Genie 2.8.6 GNTP files: /pnfs/annie/persistent/users/vfischer/genie/BNB_Water_10k_22-05-17/gntp.1000.ghep.root
- these files contain genie::NtpMCEventRecord objects that contain details of the neutrino event
- input event information is passed into a genie::flux::GNuMIFluxPassThroughInfo object

Zarko's Files
-------------
Genie 2.12 GNTP files: /pnfs/annie/persistent/users/moflaher/genie/BNB_World_10k_11-03-18_gsimpleflux/gntp.1000.ghep.root
- these files also contain genie::NtpMCEventRecord objects to describe details of the neutrino event
- input event information is passed to a genie::flux::GSimpleNtpEntry and genie::flux::GSimpleNtpNuMI object

*/

#ifndef FLUX_STAGE
//#define FLUX_STAGE 0 // gsimple / bnb_annie_* files
#define FLUX_STAGE 1   // gntp files
#endif

#if FLUX_STAGE==1
// for fluxstage=1 (gntp files) we can retrieve info about the neutrino interaction
#include <GHEP/GHepUtils.h>               // neut reaction codes
#include <PDG/PDGLibrary.h>
#include <Ntuple/NtpMCEventRecord.h>
#include <Conventions/Constants.h>
#include "src/genieinfo_struct.cpp"       // definition of a struct to hold genie info


// function to fill the info into the handy genieinfostruct
// TODO: move to header
void GetGenieEntryInfo(genie::EventRecord* gevtRec, genie::Interaction* genieint, GenieInfo& thegenieinfo, bool printneutrinoevent=false);
#endif

LoadGenieEvent::LoadGenieEvent():Tool(){}

bool LoadGenieEvent::Initialise(std::string configfile, DataModel &data){

  /////////////////// Useful header ///////////////////////
  if(configfile!="")  m_variables.Initialise(configfile); //loading config file
  //m_variables.Print();

  m_data= &data; //assigning transient data pointer
  /////////////////////////////////////////////////////////////////

  return true;
}


bool LoadGenieEvent::Execute(){

  return true;
}


bool LoadGenieEvent::Finalise(){

  return true;
}
