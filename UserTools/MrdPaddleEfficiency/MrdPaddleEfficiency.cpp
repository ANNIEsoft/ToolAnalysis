#include "MrdPaddleEfficiency.h"

//for now just copy all includes, evaluate what's important later
#include <numeric>      // std::iota
#include "TROOT.h"
#include "TApplication.h"
#include "TCanvas.h"
#include "TH1.h"
#include "TSystem.h"
#include <thread>          // std::this_thread::sleep_for
#include <chrono>          // std::chrono::seconds

MrdPaddleEfficiency::MrdPaddleEfficiency():Tool(){}


bool MrdPaddleEfficiency::Initialise(std::string configfile, DataModel &data){

	/////////////////// Usefull header ///////////////////////
	if(configfile!="")  m_variables.Initialise(configfile); //loading config file
	//m_variables.Print();
	
	m_data= &data; //assigning transient data pointer

	m_variables.Get("verbosity",verbosity);

	property_file.open("mrdtrackfitproperties_MRDTest28_commonstopp0.dat");
	property_file << "EventNumber - NumTracksInEv - PMTsHit - HtrackFitChi2 - VtrackFitChi2 - LayersHit - TrackLength"<<std::endl;

	hist_file = new TFile("mrdefficiency_hists.root","RECREATE");
	hist_numtracks = new TH1D("hist_numtracks","Number of Tracks",500,1,0);
	hist_pmtshit = new TH1D("hist_pmtshit","PMTs hit",500,1,0);
	hist_htrackfitchi2 = new TH1D("hist_htrackfitchi2","Horizontal Track Fits Chi2",500,1,0);
	hist_vtrackfitchi2 = new TH1D("hist_vtrackfitchi2","Vertical Track Fits Chi2",500,1,0);
	hist_layershit = new TH1D("hist_layershit","Layers hit",500,1,0);
	hist_tracklength = new TH1D("hist_tracklength","Tracklength",500,1,0);

	m_data->Stores["ANNIEEvent"]->Header->Get("AnnieGeometry",geom);

	//get the properties of the MRD paddles
	std::map<std::string,std::map<unsigned long,Detector*> >* Detectors = geom->GetDetectors();

  	for (std::map<unsigned long,Detector*>::iterator it  = Detectors->at("MRD").begin();
                                                    it != Detectors->at("MRD").end();
                                                  ++it){
	    Detector* amrdpmt = it->second;
	    unsigned long detkey = it->first;
	    unsigned long chankey = amrdpmt->GetChannels()->begin()->first;
	    Paddle *mrdpaddle = (Paddle*) geom->GetDetectorPaddle(detkey);

	    double xmin = mrdpaddle->GetXmin();
	    double xmax = mrdpaddle->GetXmax();
	    double xmean = 0.5*(xmin+xmax);
	    double ymin = mrdpaddle->GetYmin();
	    double ymax = mrdpaddle->GetYmax();
	    double ymean = 0.5*(ymin+ymax);
	    double zmin = mrdpaddle->GetZmin();
	    double zmax = mrdpaddle->GetZmax();
	    double zmean = 0.5*(zmin+zmax);
	    int orientation = mrdpaddle->GetOrientation();    //0 is horizontal, 1 is vertical
	    int half = mrdpaddle->GetHalf();                  //0 or 1
	    int side = mrdpaddle->GetSide();
	    int layer = mrdpaddle->GetLayer();

	    std::cout <<"layer "<<layer<<std::endl;

	    double layermin, layermax;
	    if (orientation==0) {layermin = ymin; layermax = ymax;}
	    else {layermin = xmin; layermax = xmax;}

	    if (zLayers.count(layer)==0) {
	    	zLayers.emplace(layer,zmean);
	    	orientationLayers.emplace(layer,orientation);
	    }
	    if (expected_MRDHits.count(layer)==0) {
	    	std::stringstream ss_expectedhist;
	    	std::stringstream ss_layer;
	    	ss_expectedhist << "expectedhits_layer"<<layer;
	    	ss_layer << "Expected Hits Layer "<<layer;
	    	TH1D *hist_expected = new TH1D(ss_expectedhist.str().c_str(),ss_layer.str().c_str(),50,layermin, layermax);
	    	expected_MRDHits.emplace(layer,hist_expected);
	    }
	    if (observed_MRDHits.count(layer)==0) {
	    	std::stringstream ss_observedhist;
	    	std::stringstream ss_layer;
	    	ss_observedhist << "observedhits_layer"<<layer<<"_chkey"<<chankey;
	    	ss_layer <<"Observed Hits Layer "<<layer<<" Chkey "<<chankey;
	    	TH1D *hist_observed = new TH1D(ss_observedhist.str().c_str(),ss_layer.str().c_str(),50,layermin,layermax);
	    	std::map<unsigned long, TH1D*> map_chkey_hist;
	    	map_chkey_hist.emplace(chankey,hist_observed);
	    	observed_MRDHits.emplace(layer,map_chkey_hist);
	    } else {
	    	std::stringstream ss_observedhist;
	    	std::stringstream ss_layer;
	    	ss_observedhist << "observedhits_layer"<<layer<<"_chkey"<<chankey;
	    	ss_layer <<"Observed Hits Layer "<<layer<<" Chkey "<<chankey;
	    	TH1D *hist_observed = new TH1D(ss_observedhist.str().c_str(),ss_layer.str().c_str(),50,layermin,layermax);
	    	observed_MRDHits.at(layer).emplace(chankey,hist_observed);
	    }

	}

	return true;

}

bool MrdPaddleEfficiency::Execute(){

	m_data->Stores["ANNIEEvent"]->Get("EventNumber",EventNumber);

	m_data->CStore.Get("MRDTriggerType",MRDTriggertype);

	if (MRDTriggertype == "Cosmic"){
	// demonstrate retrieving tracks from the BoostStore
		m_data->Stores["MRDTracks"]->Get("NumMrdSubEvents",numsubevs);
		m_data->Stores["MRDTracks"]->Get("NumMrdTracks",numtracksinev);

		if(verbosity>2) cout<<"Event "<<EventNumber<<" had "<<numtracksinev<<" tracks in "<<numsubevs<<" subevents"<<endl;
			
		//only evaluate Cosmic events for Efficiency calculations:
	
		std::cout <<"fill numtracksinev"<<std::endl;
		hist_numtracks->Fill(numtracksinev);
		if (numtracksinev > 0) 	{

			std::cout <<"get mrdtracks"<<std::endl;
			m_data->Stores["MRDTracks"]->Get("MRDTracks",theMrdTracks);

			//loop over reconstructed tracks
			std::cout <<"define paddlesInTrackReco"<<std::endl;
			paddlesInTrackReco.clear();


			for(int tracki=0; tracki<numtracksinev; tracki++){
				std::cout <<"tracki: "<<tracki<<std::endl;
				BoostStore* thisTrackAsBoostStore = &(theMrdTracks->at(tracki));
				PMTsHit.clear();
				LayersHit.clear();

				//get track properties that are needed for the efficiency analysis
				thisTrackAsBoostStore->Get("StartVertex",StartVertex);
				thisTrackAsBoostStore->Get("StopVertex",StopVertex);
				thisTrackAsBoostStore->Get("PMTsHit",PMTsHit);
				thisTrackAsBoostStore->Get("HtrackFitChi2",HtrackFitChi2);
				thisTrackAsBoostStore->Get("VtrackFitChi2",VtrackFitChi2);
				thisTrackAsBoostStore->Get("LayersHit",LayersHit);
				thisTrackAsBoostStore->Get("MrdTrackID",MrdTrackID);

				double tracklength = sqrt(pow((StopVertex.X()-StartVertex.X()),2)+pow(StopVertex.Y()-StartVertex.Y(),2)+pow(StopVertex.Z()-StartVertex.Z(),2));

				if(PMTsHit.size()) paddlesInTrackReco.emplace(MrdTrackID,PMTsHit);
				if(verbosity>2) cout<<"StartVertex: ("<<StartVertex.X()<<", "<<StartVertex.Y()<<", "<<StartVertex.Z()<<"), Stop Vertex: ("<<StopVertex.X()<<", "<<StopVertex.Y()<<", "<<StopVertex.Z()<<"), PMTsHit: "<<PMTsHit.size()<<", HtrackFitChi2: "<<HtrackFitChi2<<"< VtrackFitChi2: "<<VtrackFitChi2<<", LayersHit: "<<LayersHit.size()<<", tracklength: "<<tracklength<<endl;
				property_file << EventNumber <<", "<<numtracksinev<<", "<<PMTsHit.size()<<", "<<HtrackFitChi2<<", "<<VtrackFitChi2<<", "<<LayersHit.size()<<", "<<tracklength<<std::endl;

				hist_pmtshit->Fill(PMTsHit.size());
				hist_htrackfitchi2->Fill(HtrackFitChi2);
				hist_vtrackfitchi2->Fill(VtrackFitChi2);
				hist_layershit->Fill(LayersHit.size());
				hist_tracklength->Fill(tracklength);

				if (PMTsHit.size() < 30 && HtrackFitChi2 > 0.005 && VtrackFitChi2 > 0.005){		//some selection cuts to only select well fit tracks

					for (int i_layer = 0; i_layer < zLayers.size(); i_layer++){
						
						if (i_layer == 0 || i_layer == zLayers.size() -1) continue;


							double x_layer, y_layer;
							FindPaddleIntersection(StartVertex, StopVertex, x_layer, y_layer, zLayers.at(i_layer));
							if (orientationLayers.at(i_layer) == 0) {
								expected_MRDHits.at(i_layer)->Fill(y_layer);
								observed_MRDHits.at(i_layer).at(0)->Fill(y_layer);
							}
							else {
								expected_MRDHits.at(i_layer)->Fill(x_layer);
								observed_MRDHits.at(i_layer).at(0)->Fill(x_layer);
							}
					}
				}
			}
		}
	}

	return true;

}

bool MrdPaddleEfficiency::Finalise(){

	property_file.close();

	hist_file->cd();
	hist_numtracks->Write();
	hist_pmtshit->Write();
	hist_htrackfitchi2->Write();
	hist_vtrackfitchi2->Write();
	hist_layershit->Write();
	hist_tracklength->Write();
	hist_file->Close();
	delete hist_file;

	return true;


}

bool MrdPaddleEfficiency::FindPaddleIntersection(Position startpos, Position endpos, double &x, double &y, double z){

	double DirX = endpos.X()-startpos.X();
	double DirY = endpos.Y()-startpos.Y();
	double DirZ = endpos.Z()-startpos.Z();

	if (fabs(DirZ) < 0.001) Log("MrdPaddleEfficiency tool: StartVertex = EndVertex! Track was not fitted well",v_error,verbosity);

	double frac = (z - startpos.Z())/DirZ;

	x = startpos.X()+frac*DirX;
	y = startpos.Y()+frac*DirY;

	return true;

}