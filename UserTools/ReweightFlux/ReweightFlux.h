#ifndef ReweightFlux_H
#define ReweightFlux_H

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
/*#include "GenieInfo.h"
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
*/
#include <TParticlePDG.h>

/*#include "RwFramework/GSystSet.h"
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
*/
#include "TChain.h"
#include "TFile.h"
#include "TTree.h"
#include "TVector3.h"
#include "TLorentzVector.h"

#include "MRDspecs.hh"
#include "MCEventWeight.h"
#include "WeightManager.h"



/**
 * \class ReweightFlux
 *
 * This is a blank template for a Tool used by the script to generate a new custom tool. Please fill out the description and author information.
*
* $Author: B.Richards $
* $Date: 2019/05/28 10:44:00 $
* Contact: b.richards@qmul.ac.uk
*/
class ReweightFlux: public Tool {


 public:

  ReweightFlux(); ///< Simple constructor
  bool Initialise(std::string configfile,DataModel &data); ///< Initialise Function for setting up Tool resources. @param configfile The path and name of the dynamic configuration file to read in. @param data A reference to the transient data class used to pass information between Tools.
  bool Execute(); ///< Execute function used to perform Tool purpose.
  bool Finalise(); ///< Finalise function used to clean up resources.

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
  vector<string> fweight_names;
  vector<evwgh::fluxconfig> fconfig_funcs;
  evwgh::WeightManager wm;

};


#endif
