#ifndef ReweightEventsGenie_H
#define ReweightEventsGenie_H

#include <string>
#include <iostream>
#include <sstream>
#include <fstream>
#include <map>
#include <utility>
#include <memory>
#include <cmath>
#include <set>
#include <vector>

#include "Tool.h"
#include "GenieInfo.h"
#include "CLHEP/Random/RandGaussQ.h"
#include "CLHEP/Random/JamesRandom.h"
#include "Framework/Conventions/KineVar.h"
#include "Framework/EventGen/EventRecord.h"
#include "Framework/Interaction/Interaction.h"
#include "Framework/Interaction/Kinematics.h"
//#include "Framework/Messenger/Messenger.h"
#include "Framework/Utils/AppInit.h"
#include <Tools/Flux/GSimpleNtpFlux.h>
#include <Tools/Flux/GNuMIFlux.h>
#include <Framework/GHEP/GHepUtils.h>               // neut reaction codes
#include <Framework/ParticleData/PDGLibrary.h>
#include <Framework/ParticleData/PDGCodes.h>
#include <Framework/Ntuple/NtpMCEventRecord.h>
#include <Framework/Ntuple/NtpMCTreeHeader.h>
#include <Framework/Conventions/Constants.h>
#include <Framework/GHEP/GHepParticle.h>
#include <Framework/GHEP/GHepStatus.h>
#include <TParticlePDG.h>

#include "RwFramework/GSystSet.h"
#include "RwFramework/GSyst.h"
#include "RwFramework/GReWeight.h"
#include "RwCalculators/GReWeightNuXSecNCEL.h"
#include "RwCalculators/GReWeightNuXSecCCQE.h"
#include "RwCalculators/GReWeightNuXSecCCRES.h"
#include "RwCalculators/GReWeightNuXSecCOH.h"
#include "RwCalculators/GReWeightNonResonanceBkg.h"
#include "RwCalculators/GReWeightFGM.h"
#include "RwCalculators/GReWeightDISNuclMod.h"
#include "RwCalculators/GReWeightResonanceDecay.h"
#include "RwCalculators/GReWeightFZone.h"
#include "RwCalculators/GReWeightINuke.h"
#include "RwCalculators/GReWeightAGKY.h"
#include "RwCalculators/GReWeightNuXSecCCQEaxial.h"
#include "RwCalculators/GReWeightNuXSecCCQEvec.h"
#include "RwCalculators/GReWeightNuXSecNCRES.h"
#include "RwCalculators/GReWeightNuXSecDIS.h"
#include "RwCalculators/GReWeightINukeParams.h"
#include "RwCalculators/GReWeightNuXSecNC.h"
#include "RwCalculators/GReWeightXSecEmpiricalMEC.h"
//UBoone Patch
#include "RwCalculators/GReWeightXSecMEC.h"
#include "RwCalculators/GReWeightDeltaradAngle.h"
#include "RwCalculators/GReWeightNuXSecCOHuB.h"
#include "RwCalculators/GReWeightRESBugFix.h"

#include <Tools/Flux/GSimpleNtpFlux.h>
#include <Tools/Flux/GNuMIFlux.h>
#include <GHEP/GHepUtils.h>               // neut reaction codes
#include <ParticleData/PDGLibrary.h>
#include <ParticleData/PDGCodes.h>
#include <Ntuple/NtpMCEventRecord.h>
#include <Conventions/Constants.h>
#include <GHEP/GHepParticle.h>
#include <GHEP/GHepStatus.h>
#include <EventGen/EventRecord.h>
#include <TParticlePDG.h>
#include <Interaction/Interaction.h>
#include "TChain.h"
#include "TFile.h"
#include "TTree.h"
#include "TVector3.h"
#include "TLorentzVector.h"

#include "MRDspecs.hh"
#include "MCEventWeight.h"
#include "WeightManager.h"

#define LOADED_GENIE 1

/**
 * \class ReweightEventsGenie
 *
* Ported in parts from LoadGenieEvent
* $Author: B.Richards
* $Date: 2019/05/28 10:44:00
* Contact: b.richards@qmul.ac.uk
*
* Updated
* $Author: James Minock
* $Date: 2023/02/09
* Contact: jmm1018@physics.rutgers.edu
*/

class ReweightEventsGenie: public Tool {


 public:
  ReweightEventsGenie(); ///< Simple constructor
  bool Initialise(std::string configfile,DataModel &data); ///< Initialise Function for setting up Tool resources. @param configfile The path and name of the dynamic configuration file to read in. @param data A reference to the transient data class used to pass information between Tools.
  bool Execute(); ///< Execute function used to perform Tool purpose.
  bool Finalise(); ///< Finalise function used to clean up resources.
  bool valid_knob_name( const std::string& knob_name, genie::rew::GSyst_t& knob );
  std::map< std::string, int > CheckForIncompatibleSystematics(const std::vector<genie::rew::GSyst_t>& knob_vec);
  void SetupWeightCalculators(genie::rew::GReWeight& rw, const std::map<std::string, int>& modes_to_use);

  // verbosity levels: if 'verbosity' < this level, the message type will be logged.
  int verbosity;
  int v_error=0;
  int v_warning=1;
  int v_message=2;
  int v_debug=3;
  std::string logmessage;
  int get_ok;

 private:

  //Configuration variables
  std::string weight_options;
  std::string fweight_options;
  std::string sample;
  std::string fGenieModuleWeight;
  vector<string> weight_names;
  vector<string> fweight_names;
  vector<evwgh::fluxconfig> fconfig_funcs;
  evwgh::WeightManager wm;

  vector<vector< genie::rew::GReWeight >> reweightVector;
  int flx_run;
  unsigned int flx_evt;

  // function to load the branch addresses
  void SetBranchAddresses();

  // function to fill the info into the handy genieinfostruct
  void GetGenieEntryInfo(genie::EventRecord* gevtRec, genie::Interaction* genieint,
    GenieInfo& thegenieinfo, bool printneutrinoevent=false);

  BoostStore* geniestore = nullptr;
  int fluxstage;
  std::string filedir, filepattern, outdir, outpattern, fluxdir, fluxfile;
  bool loadwcsimsource;
  TChain* flux = nullptr;
  TFile* curf = nullptr;       // keep track of file changes
  TFile* curflast = nullptr;
  genie::NtpMCEventRecord* genieintx = nullptr; // = new genie::NtpMCEventRecord;
  genie::NtpMCTreeHeader* geniehdr = nullptr;
  // for fluxver 0 files
  genie::flux::GNuMIFluxPassThroughInfo* gnumipassthruentry  = nullptr;
  // for fluxver 1 files
  genie::flux::GSimpleNtpEntry* gsimpleentry = nullptr;
  genie::flux::GSimpleNtpAux* gsimpleauxinfo = nullptr;
  genie::flux::GSimpleNtpNuMI* gsimplenumientry = nullptr;

  // genie file variables
  int fluxver;                         // 0 = old flux, 1 = new flux
  std::string currentfilestring;
  long local_entry=0;           // 
  int tchainentrynum=0;         // 
  int tchainentrynum_fw=0;
  bool fromwcsim;
  bool on_grid;
};


#endif
