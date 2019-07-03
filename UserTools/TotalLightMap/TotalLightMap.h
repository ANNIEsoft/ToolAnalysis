/* vim:set noexpandtab tabstop=4 wrap */
#ifndef TotalLightMap_H
#define TotalLightMap_H

#include <string>
#include <iostream>

#include "Tool.h"
#include "TH1D.h"
#include "TList.h"
#include "TROOT.h"
#include "TStyle.h"
#include "MRDspecs.hh"

// for drawing
class TApplication;
class TCanvas;
class TPolyMarker;
class TEllipse;
class TBox;
class TColor;
class TH2F;
class TPaveLabel;
class TMarker;

class TotalLightMap: public Tool {
	
	public:
	
	TotalLightMap();
	bool Initialise(std::string configfile,DataModel &data);
	bool Execute();
	bool Finalise();
	
	private:
	
	int verbosity=1;
	bool drawHistos;
	std::string plotDirectory;
	
	// geometry and detectors, needed for positioning etc
	Geometry* anniegeom=nullptr;
	double tank_height;
	double tank_radius;
	// 
	// hits, particles from the ANNIEEvent
	std::map<unsigned long,std::vector<MCLAPPDHit>>* MCLAPPDHits=nullptr;
	std::map<unsigned long,std::vector<MCHit>>* MCHits=nullptr;
	std::vector<MCParticle>* MCParticles=nullptr;                                    // the true particles
	
	// TApplication for making plots
	TApplication* rootTApp=nullptr;
	
	// objects to make the event display
	TCanvas* canvas_ev_display=nullptr;
	TEllipse *top_circle=nullptr;
	TEllipse *bottom_circle=nullptr;
	TBox *box=nullptr;
	double size_top_drawing=0.125;
	double yscale=1.5;  // alter aspect ratio of tank to make it better fill a square canvas
	// for mapping value to colour
	double colour_full_scale=0;
	double colour_offset=0;
	double size_full_scale = 0;
	double size_offset = 0;
	// for the colour legend
	TPaveLabel* colourscalemax;
	TPaveLabel* colourscalemin;
	int Bird_Idx;
	std::vector<TMarker*> legend_dots;
	
	// eventwise markers for making hit maps on the event display gui
	std::vector<TPolyMarker*> marker_pmts_top;    // colour according to the hit charge,
	std::vector<TPolyMarker*> marker_pmts_bottom; // time within the event,
	std::vector<TPolyMarker*> marker_pmts_wall;   // or parentage
	std::vector<TPolyMarker*> marker_lappds;      //
	// eventwise markers for the projected exit point of true particles
	std::vector<TPolyMarker*> marker_vtxs;        // colour according to particle type
	
	// keep note of the Particle IDs of those particles we wish to plot light from -
	// these will be the primary muon, and if applicable the light from (up to) 2 pion decay daughters
	std::vector<int> particleidsofinterest;
	std::string interaction_type; // "CCQE" or "CC1PI"
	
	// we need to create colours for the markers on the heap, and somehow know a free unique number
	int startingcolournum = 1756; // ¯\_(ツ)_/¯
	std::vector<TColor*> eventcolours;
	int color_marker;   // technically a Color_t I think
	std::string mode;
	int threshold_time_high=-999;  // window time if colouring markers by time within a predefined window
	int threshold_time_low=-999;
	double threshold_lappd=0;      // minimum charge for an LAPPD hit to make a marker
	
	////////////////////////////////////////////
	// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	////////////////////////////////////////////
	
	// second set of plots are wiener tripel plots that show the full 360° mapped onto one canvas
	
	// canvases for wiener tripel light maps
	TCanvas* lightmap_by_eventtype_canvas = nullptr;
	TCanvas* lightmap_by_parent_canvas = nullptr;
	TCanvas* mercatorCanv = nullptr;
	
	// unbinned cumulative plots of light from different particles / different event types
	// one TPolyMarker set for CCQE hits and one for CCNPI to compare light distributions (red vs blue)
	// since an event can only be one or the other, this is a cumulative plot only
	TPolyMarker *lightmapccqe=nullptr, *lightmapcc1p=nullptr;
	
	// A plot of charge from parentage for CC1PI events may also be worthwhile
	// this can be made both cumulative and on eventwise basis
	// for the cumulative plot we just need to accumulate the TPolyMarkers over all events
	std::vector<TPolyMarker*> chargemapcc1p;
	std::vector<TPolyMarker*> chargemapcc1p_cum;
	
	// accumulative over all events
	std::map<int, TPolyMarker*> marker_vtxs_map;
	//marker_vtxs_mu, marker_vtxs_pip << delete
	
	// spherical polar binned light map, rotated to place the muon exit at the origin
	// these plots are only drawn in Finalize; canvas is made locally at the time
	TH2F* lmccqe = nullptr;
	TH2F* lmcc1p = nullptr;
	TH2F* lmdiff = nullptr;
	TH2F* lmmuon = nullptr;
	TH2F* lmpigammas = nullptr;
	TH2F* lmdiff2 = nullptr;
	
	// debug plots
	TH1D* vertexphihist = nullptr;
	TH1D* vertexthetahist = nullptr;
	TH1D* vertexyhist = nullptr;
	
	////////////////////////////////////////////
	// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	////////////////////////////////////////////
	
	// we'll keep track of event maxima to set the full scale of the colouring
	double event_earliest_hit_time = 9999999;
	double event_latest_hit_time = 0;
	double maximum_charge_on_a_pmt = 0;
	
	// function to build the event display canvas and draw the detector outlines
	void make_gui();
	// function to scan over PMT hits in the ANNIEEvent and make a marker for each PMT
	void make_pmt_markers(MCParticle primarymuon);
	// function to scan over LAPPD hits in the ANNIEEvent and make a marker for each LAPPD hit
	void make_lappd_markers(MCParticle primarymuon);
	// function to project a particle and make a marker for it's exit point
	void make_vertex_markers(MCParticle aparticle, MCParticle primarymuon);
	// function to draw the markers on the event display
	void DrawMarkers();
	// function to draw the polymarkers for the accumulative event displays
	void DrawCumulativeMarkers();
	// function to cleanup memory
	void FinalCleanup();
	
	// other useful functions from michael
	void translate_xy(double vtxX, double vtxY, double vtxZ, double &xWall, double &yWall);
	void find_projected_xyz(double vtxX, double vtxY, double vtxZ, double dirX, double dirY, double dirZ, double &projected_x, double &projected_y, double &projected_z);
	// void draw_true_ring();  // this one's a beast!
	
	// other functions 
	void DoTheWinkelTripel(double x, double y, double z, double& x_winkel, double& y_winkel);
	void DoTheWinkelTripel(double latitude, double longitude, double& x_winkel, double& y_winkel);
	void DoTheMercator(double latitude, double longitude, double& x_mercater, double& y_mercater, double mapHeight=1., double mapWidth=1.);
	void InverseMercator(double x_mercater, double y_mercater, double& latitude, double& longitude, double mapHeight=1., double mapWidth=1.);
	void CartesianToStereoGraphic(double x, double y, double z, double& xout, double& yout);
	
	//std::map<int,std::map<unsigned long,double>>* ParticleId_to_TankTubeIds=nullptr; // unused alt method
	
	// verbosity levels: if 'verbosity' < this level, the message type will be logged.
	int v_error=0;
	int v_warning=1;
	int v_message=2;
	int v_debug=3;
	std::string logmessage;
	int get_ok;
	
};

#endif

//////////////////
/*
ok, crux of the matter here:
We want to make cumulative plots that can show how an 'average' CCQE event
differs from an 'average' CCNPi event.

For plotting 'CCQE' vs 'CCPi' hitmaps on the same canvas, we need to have blue/red marker colours
that distinguish the event type, so we do not need a collection of TPolyMarkers, just one. XXX

If we want to compare more than just their postions, we must plot the two on separate canvases.
But what else is there to plot in such a case? 
Hit time doesn't make sense: the pion lifetime is so short all parents are ejected simultaneously.
Hit charge likewise; features would be washed out, rather than enhanced, accumulating over many events.
Charge from parentage doesn't even make sense for a CCQE event.

A cumulative plot of charge from parentage for CC1PI events *would* be worthwhile;
for this we just need to accumulate the appropriate polymarkers. XXX
*/
//////////////////
