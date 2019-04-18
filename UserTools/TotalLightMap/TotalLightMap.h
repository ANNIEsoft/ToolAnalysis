/* vim:set noexpandtab tabstop=4 wrap */
#ifndef TotalLightMap_H
#define TotalLightMap_H

#include <string>
#include <iostream>

#include "Tool.h"

// for drawing
class TApplication;
class TCanvas;
class TPolyMarker2D;

class TotalLightMap: public Tool {
	
	public:
	
	TotalLightMap();
	bool Initialise(std::string configfile,DataModel &data);
	bool Execute();
	bool Finalise();
	
	private:
	
	int verbosity=1;
	bool drawHistos;
	int get_ok;
	
	// do we want this? do we want what's in the map of hits from given particles? <<yes<<
	// we also need to add to wcsim the truth global hit position, like for LAPPD hits
	Geometry* anniegeom=nullptr;
	TimeClass* EventTime=nullptr;
	uint64_t MCEventNum;
	uint16_t MCTriggernum;
	std::map<unsigned long,std::vector<LAPPDHit>>* MCLAPPDHits=nullptr;
	std::map<unsigned long,std::vector<MCHit>>* MCHits=nullptr;
	std::vector<MCParticle>* MCParticles=nullptr;
	// alt
	std::map<int,std::map<unsigned long,double>>* ParticleId_to_MrdTubeIds; // the MC Truth information
	std::map<unsigned long,int> channelkey_to_mrdpmtid;           // 
	std::vector<BoostStore>* theMrdTracks;                        // the reconstructed tracks
	std::vector<MCParticle>* MCParticles=nullptr;                 // the true particles
	
	// TApplication for making histograms
	TApplication* rootTApp=nullptr;
	TCanvas* lightMapCanv=nullptr;
	TPolyMarker *lightmapmu=nullptr, *lightmappip=nullptr, *lightmappim=nullptr, *lightmappigamma=nullptr
		*lightmapother=nullptr;
	TH2D* blightmapmu=nullptr, *blightmappip=nullptr, *blightmappim=nullptr, *blightmappigamma=nullptr
		*blightmapother=nullptr;
	TPolyMarker2D *plightmapmu=nullptr, *plightmappip=nullptr, *plightmappim=nullptr, *plightmappigamma=nullptr
		*plightmapother=nullptr;
	
	std::string plotDirectory;
	
	// verbosity levels: if 'verbosity' < this level, the message type will be logged.
	int v_error=0;
	int v_warning=1;
	int v_message=2;
	int v_debug=3;
	std::string logmessage;

	
};

#endif
