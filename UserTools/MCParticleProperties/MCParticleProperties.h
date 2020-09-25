/* vim:set noexpandtab tabstop=4 wrap */
#ifndef MCParticleProperties_H
#define MCParticleProperties_H

#include <string>
#include <iostream>

#include "Tool.h"
#include "MRDspecs.hh"
#include "TMath.h"

class MCParticleProperties: public Tool {
	
	public:
	
	MCParticleProperties();
	bool Initialise(std::string configfile,DataModel &data);
	bool Execute();
	bool Finalise();
	
	// hack: make this static and public so we can use it outside this tool.
	static bool ProjectTankIntercepts(Position startvertex, Position stopvertex, Position &Hit, int verbose=0);
	// also sets these from MRDSpecs when this tool is not used.
	// If these diverge from the anniegeom version in the tool, this could be dangerous.
	// We make these public because the returned results are in WCSim coordinates and we need to fix that.
	static double tank_radius;
	static double tank_start;
	static double tank_yoffset;
	static double tank_halfheight;
	
	private:
	// read from config file
	int verbosity;
	
	// internal variables
	double fidcutradius;
	double fidcutz;
	double fidcuty;
	
	// from ANNIEEvent
	Geometry* anniegeom=nullptr;
	std::vector<MCParticle>* MCParticles=nullptr;
	
	// helper functions to find the MRD intersection points
	bool CheckProjectedMRDHit(Position startvertex, Position stopvertex, double mrdwidth, double mrdheight, double mrdstart);
	bool CheckLineBox( Position L1, Position L2, Position B1, Position B2, Position &Hit, Position &Hit2, bool &error, int verbose=0);
	int InBox( Position Hit, Position B1, Position B2, const int Axis);
	int GetIntersection( float fDst1, float fDst2, Position P1, Position P2, Position &Hit);
	// helper function for finding tank intersection points
	bool CheckTankIntercepts( Position startvertex, Position stopvertex, bool trackstartsintank, bool trackstopsintank, Position &Hit, Position &Hit2, int verbose=0);
	// helper function for finding where the track would have left the tank, if it didn't
	std::map<int,std::string>* GeneratePdgMap();
	std::string PdgToString(int code);
	std::map<int,std::string> pdgcodetoname;		//to be saved in CStore for subsequent tools
	std::map<int,double>* GeneratePdgMassMap();
	double PdgToMass(int code);
	std::map<int,double> pdgcodetomass;			//to be saved in CStore for subsequent tools
	
	// verbosity levels: if 'verbosity' < this level, the message type will be logged.
	int v_error=0;
	int v_warning=1;
	int v_message=2;
	int v_debug=3;
	std::string logmessage;
	int get_ok;
	
};

#endif
