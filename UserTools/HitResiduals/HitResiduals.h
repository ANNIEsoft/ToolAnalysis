/* vim:set noexpandtab tabstop=4 wrap */
#ifndef HitResiduals_H
#define HitResiduals_H

#include <string>
#include <iostream>

#include "Tool.h"
#include "TApplication.h"
#include "TCanvas.h"
#include "TH1D.h"

class HitResiduals: public Tool {
	
	public:
	HitResiduals();
	bool Initialise(std::string configfile,DataModel &data);
	bool Execute();
	bool Finalise();
	
	private:
	TimeClass* EventTime=nullptr;
	std::vector<MCParticle>* MCParticles=nullptr;
	std::map<unsigned long,std::vector<Hit>>* MCHits=nullptr;
	std::map<unsigned long,std::vector<LAPPDHit>>* MCLAPPDHits=nullptr;
	Geometry* anniegeom=nullptr;
	
	TApplication* hitResidualApp = nullptr;
	TCanvas* hitResidCanv=nullptr;
	TH1D* htimeresidpmt = nullptr;
	TH1D* htimeresidlappd = nullptr;
	
	int get_ok;
	
};


#endif
