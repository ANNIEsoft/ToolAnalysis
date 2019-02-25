#include <GHEP/GHepParticle.h>
#include <EVGCore/EventRecord.h>
#include <TParticlePDG.h>
#include <Interaction/Interaction.h>
#include <Ntuple/NtpMCEventRecord.h>
struct GenieInfo {
	
	GenieInfo(){ 
		eventtypes.emplace("IsQuasiElastic",false);
		eventtypes.emplace("IsResonant",false);
		eventtypes.emplace("IsDeepInelastic",false);
		eventtypes.emplace("IsCoherent",false);
		eventtypes.emplace("IsDiffractive",false);
		eventtypes.emplace("IsInverseMuDecay",false);
		eventtypes.emplace("IsIMDAnnihilation",false);
		eventtypes.emplace("IsSingleKaon",false);
		eventtypes.emplace("IsNuElectronElastic",false);
		eventtypes.emplace("IsEM",false);
		eventtypes.emplace("IsWeakCC",false);
		eventtypes.emplace("IsWeakNC",false);
		eventtypes.emplace("IsMEC",false);
	}
	
	// process information:
	TString procinfostring="";
	TString scatteringtypestring="";
	TString interactiontypestring="";
	std::map<std::string,bool> eventtypes;
	Int_t neutinteractioncode=-1;
	Int_t nuanceinteractioncode=-1;
	TLorentzVector* IntxVtx=0;                        // [secs,m,m,m]
	Double_t Intx_x=0.;                               // cm
	Double_t Intx_y=0.;                               // cm
	Double_t Intx_z=0.;                               // cm
	Double_t Intx_t=0.;                               // ns

	// neutrino information:
	Double_t probeenergy=0.;                           // GeV
	Int_t probepdg=-1;
	TString probepartname="";
	TLorentzVector* probemomentum=0;                   // GeV/c?
	TVector3 probethreemomentum=TVector3(0.,0.,0.);    // GeV/c?
	TVector3 probemomentumdir=TVector3(0.,0.,0.);      // unit vector
	Double_t probeanglex=0.;                           // rads
	Double_t probeangley=0.;                           // rads
	Double_t probeangle=0.;                            // rads

	// target nucleon:
	genie::GHepParticle* targetnucleon=0;
	int targetnucleonpdg=-1;
	TString targetnucleonname="";                      // pdg if name not known
	TVector3 targetnucleonthreemomentum=TVector3(0.,0.,0.);  // GeV/c? only defined if there is a target nucleon: not true for all events
	Double_t targetnucleonenergy=0.;                   // GeV. only defined if there is a target nucleon

	// target nucleus:
	Int_t targetnucleuspdg=-1;
	TParticlePDG* targetnucleus=0;
	TString targetnucleusname="";                      // "unknown" if there is no defined targetnucleus
	Int_t targetnucleusZ=-1;
	Int_t targetnucleusA=-1;

	// remnant nucleus:
	TString remnantnucleusname="";                     // "n/a" if not defined
	Double_t remnantnucleusenergy=0.;                  // GeV. -1 if not defined

	// final state lepton:
	TString fsleptonname="";                           // "n/a" if not defined
	Double_t fsleptonenergy=0.;                        // GeV. -1 if not defined

	// other remnants: TODO: this information is NOT being correctly read in
	Int_t numfsprotons=-1;
	Int_t numfsneutrons=-1;
	Int_t numfspi0=-1;
	Int_t numfspiplus=-1;
	Int_t numfspiminus=-1;

	// kinematic information
	TLorentzVector* k1=0;                               // GeV/c? Neutrino incoming momentum vector
	TLorentzVector* k2=0;                               // GeV/c? Muon outgoign momentum vector
	TLorentzVector q=TLorentzVector(0.,0.,0.,0.);       // GeV/c? 4-momentum transfer: k1-k2
	Double_t costhfsl=0.;                               // 
	Double_t fslangle=0.;                               // rads?
	Double_t Q2=0.;                                     // GeV/c?
	Double_t Etransf=0.;                                // Energy transferred to nucleus. -1 if not defined
	Double_t x=0.;                                      // Bjorken x. -1 if target nucleon not defined
	Double_t y=0.;                                      // Inelasticity, y = q*P1/k1*P1. -1 if not defined
	Double_t W2=0.;                                     // Hadronic Invariant mass ^ 2. -1 if not defined

};
