#ifndef GenieInfoStruct_cpp
#define GenieInfoStruct_cpp
/* vim:set noexpandtab tabstop=2 wrap */

#include <SerialisableObject.h>
#include <Position.h>
#include <Direction.h>
#include <map>

class GenieInfo {
	
	friend class boost::serialization::access;

	public:
	
	GenieInfo() : serialise(true){ 
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
	std::string procinfostring="";
	std::string scatteringtypestring="";
	std::string interactiontypestring="";
	std::map<std::string,bool> eventtypes;
	int neutinteractioncode=-1;
	int nuanceinteractioncode=-1;
	//FourVector* IntxVtx=0;                         // [secs,m,m,m]
	double Intx_x=0.;                                // cm
	double Intx_y=0.;                                // cm
	double Intx_z=0.;                                // cm
	double Intx_t=0.;                                // ns
	
	// neutrino information:
	double probeenergy=0.;                           // GeV
	int probepdg=-1;
	std::string probepartname="";
	//FourVector* probemomentum=0;                   // GeV/c?
	Position probethreemomentum=Position(0.,0.,0.);  // GeV/c?
	Position probemomentumdir=Position(0.,0.,0.);    // unit vector
	double probeanglex=0.;                           // rads
	double probeangley=0.;                           // rads
	double probeangle=0.;                            // rads
	
	// target nucleon:
	//genie::GHepParticle* targetnucleon=0;
	int targetnucleonpdg=-1;
	std::string targetnucleonname="";                        // pdg if name not known
	Position targetnucleonthreemomentum=Position(0.,0.,0.);  // GeV/c? only defined if there is a target nucleon: not true for all events
	double targetnucleonenergy=0.;                           // GeV. only defined if there is a target nucleon
	
	// target nucleus:
	int targetnucleuspdg=-1;
	//TParticlePDG* targetnucleus=0;
	std::string targetnucleusname="";                // "unknown" if there is no defined targetnucleus
	int targetnucleusZ=-1;
	int targetnucleusA=-1;
	
	// remnant nucleus:
	std::string remnantnucleusname="";               // "n/a" if not defined
	double remnantnucleusenergy=0.;                  // GeV. -1 if not defined
	
	// final state lepton:
	std::string fsleptonname="";                     // "n/a" if not defined
	double fsleptonenergy=0.;                        // GeV. -1 if not defined
	
	// other remnants: TODO: this information is NOT being correctly read in
	int numfsprotons=-1;
	int numfsneutrons=-1;
	int numfspi0=-1;
	int numfspiplus=-1;
	int numfspiminus=-1;
	int numfskplus=-1;
	int numfskminus=-1;	

	// kinematic information
	//FourVector* k1=0;                               // GeV/c? Neutrino incoming momentum vector
	//FourVector* k2=0;                               // GeV/c? Muon outgoign momentum vector
	FourVector q=FourVector(0.,0.,0.,0.);             // GeV/c? 4-momentum transfer: k1-k2
	double costhfsl=0.;                               // 
	double fslangle=0.;                               // rads?
	double Q2=0.;                                     // GeV/c?
	double Etransf=0.;                                // Energy transferred to nucleus. -1 if not defined
	double x=0.;                                      // Bjorken x. -1 if target nucleon not defined
	double y=0.;                                      // Inelasticity, y = q*P1/k1*P1. -1 if not defined
	double W2=0.;                                     // Hadronic Invariant mass ^ 2. -1 if not defined
	
	bool serialise;
	
	template<class Archive> void serialize(Archive & ar, const unsigned int version){
		if(serialise){
			ar & procinfostring;
			ar & scatteringtypestring;
			ar & interactiontypestring;
			ar & eventtypes;
			ar & neutinteractioncode;
			ar & nuanceinteractioncode;
			ar & Intx_x;
			ar & Intx_y;
			ar & Intx_z;
			ar & Intx_t;
			ar & probeenergy;
			ar & probepdg;
			ar & probepartname;
			ar & probethreemomentum;
			ar & probemomentumdir;
			ar & probeanglex;
			ar & probeangley;
			ar & probeangle;
			ar & targetnucleonpdg;
			ar & targetnucleonname;
			ar & targetnucleonthreemomentum;
			ar & targetnucleonenergy;
			ar & targetnucleuspdg;
			ar & targetnucleusname;
			ar & targetnucleusZ;
			ar & targetnucleusA;
			ar & remnantnucleusname;
			ar & remnantnucleusenergy;
			ar & fsleptonname;
			ar & fsleptonenergy;
			ar & numfsprotons;
			ar & numfsneutrons;
			ar & numfspi0;
			ar & numfspiplus;
			ar & numfspiminus;
			ar & numfskplus;
			ar & numfskminus;
			ar & q;
			ar & costhfsl;
			ar & fslangle;
			ar & Q2;
			ar & Etransf;
			ar & x;
			ar & y;
			ar & W2;
		}
	}
	
};
#endif
