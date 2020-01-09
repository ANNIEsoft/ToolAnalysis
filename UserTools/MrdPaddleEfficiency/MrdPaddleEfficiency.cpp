#include "MrdPaddleEfficiency.h"



MrdPaddleEfficiency::MrdPaddleEfficiency():Tool(){}


bool MrdPaddleEfficiency::Initialise(std::string configfile, DataModel &data){

	// Get configuration variables

	if(configfile!="")  m_variables.Initialise(configfile); //loading config file
	m_data= &data; //assigning transient data pointer

	outputfile = "MRDTestXX";
	m_variables.Get("verbosity",verbosity);
	m_variables.Get("OutputFile",outputfile);

	// Define histograms for storing general properties of the fitted Mrd tracks

	std::stringstream rootfilename;
	rootfilename << outputfile << "_mrdefficiency.root";

	hist_file = new TFile(rootfilename.str().c_str(),"RECREATE");
	
	trackfit_tree = new TTree("trackfit_tree","Track fit properties");
	hist_numtracks = new TH1D("hist_numtracks","Number of Tracks",500,1,0);
	hist_pmtshit = new TH1D("hist_pmtshit","PMTs hit",500,1,0);
	hist_htrackfitchi2 = new TH1D("hist_htrackfitchi2","Horizontal Track Fits Chi2",500,1,0);
	hist_vtrackfitchi2 = new TH1D("hist_vtrackfitchi2","Vertical Track Fits Chi2",500,1,0);
	hist_layershit = new TH1D("hist_layershit","Layers hit",500,1,0);
	hist_tracklength = new TH1D("hist_tracklength","Tracklength",500,1,0);


	//Setup the tree variables
	trackfit_tree->Branch("numtracks",&numtracksinev);
	trackfit_tree->Branch("numpmtshit",&numpmtshit);
	trackfit_tree->Branch("htrackfitchi2",&HtrackFitChi2);
	trackfit_tree->Branch("vtrackfitchi2",&VtrackFitChi2);
	trackfit_tree->Branch("numlayershit",&numlayershit);
	trackfit_tree->Branch("tracklength",&tracklength);


	// Read in the geometry & get the properties (extents, orientations, etc) of the MRD paddles

	m_data->Stores["ANNIEEvent"]->Header->Get("AnnieGeometry",geom);

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

	    //MRD layer enumeration starts at 2, set to 0
	    layer-=2;

	    double layermin, layermax;
	    if (orientation==0) {layermin = ymin; layermax = ymax;}
	    else {layermin = xmin; layermax = xmax;}

	    if (zLayers.count(layer)==0) {
	    	zLayers.emplace(layer,zmean);
	    	orientationLayers.emplace(layer,orientation);
		std::vector<unsigned long> vec_chankey;
		vec_chankey.push_back(chankey);
		channelsLayers.emplace(layer,vec_chankey);
	    } else {
		channelsLayers.at(layer).push_back(chankey);
	    }
	    if (expected_MRDHits.count(layer)==0) {
	    	std::stringstream ss_expectedhist;
	    	std::stringstream ss_layer;
	    	ss_expectedhist << "expectedhits_layer"<<layer<<"_chkey"<<chankey;
	    	ss_layer << "Expected Hits Layer "<<layer<<" Chkey "<<chankey;
	    	TH1D *hist_expected = new TH1D(ss_expectedhist.str().c_str(),ss_layer.str().c_str(),50,layermin, layermax);
		if (orientation == 0) hist_expected->GetXaxis()->SetTitle("y [m]");
		else hist_expected->GetXaxis()->SetTitle("x [m]");
	    	std::map<unsigned long, TH1D*> map_chkey_hist_expected;
		map_chkey_hist_expected.emplace(chankey,hist_expected);
		expected_MRDHits.emplace(layer,map_chkey_hist_expected);
	    } else {
		std::stringstream ss_expectedhist;
		std::stringstream ss_layer;
		ss_expectedhist <<"expectedhits_layer"<<layer<<"_chkey"<<chankey;
		ss_layer << "Expected Hits Layer "<<layer<<" Chkey "<<chankey;
		TH1D *hist_expected = new TH1D(ss_expectedhist.str().c_str(),ss_layer.str().c_str(),50,layermin,layermax);
		if (orientation == 0) hist_expected->GetXaxis()->SetTitle("y [m]");
		else hist_expected->GetXaxis()->SetTitle("x [m]");
		expected_MRDHits.at(layer).emplace(chankey,hist_expected);	
	    }
	    if (observed_MRDHits.count(layer)==0) {
	    	std::stringstream ss_observedhist;
	    	std::stringstream ss_layer;
	    	ss_observedhist << "observedhits_layer"<<layer<<"_chkey"<<chankey;
	    	ss_layer <<"Observed Hits Layer "<<layer<<" Chkey "<<chankey;
	    	TH1D *hist_observed = new TH1D(ss_observedhist.str().c_str(),ss_layer.str().c_str(),50,layermin,layermax);
		if (orientation == 0) hist_observed->GetXaxis()->SetTitle("y [m]");
		else hist_observed->GetXaxis()->SetTitle("x [m]");
	    	std::map<unsigned long, TH1D*> map_chkey_hist_observed;
	    	map_chkey_hist_observed.emplace(chankey,hist_observed);
	    	observed_MRDHits.emplace(layer,map_chkey_hist_observed);
	    } else {
	    	std::stringstream ss_observedhist;
	    	std::stringstream ss_layer;
	    	ss_observedhist << "observedhits_layer"<<layer<<"_chkey"<<chankey;
	    	ss_layer <<"Observed Hits Layer "<<layer<<" Chkey "<<chankey;
	    	TH1D *hist_observed = new TH1D(ss_observedhist.str().c_str(),ss_layer.str().c_str(),50,layermin,layermax);
		if (orientation == 0) hist_observed->GetXaxis()->SetTitle("y [m]");
		else hist_observed->GetXaxis()->SetTitle("x [m]");
	    	observed_MRDHits.at(layer).emplace(chankey,hist_observed);
	    }

	}

	// Define text file to store MRD track fit variables
	
	std::stringstream prop_filename;
	prop_filename << outputfile << "trackfitproperties.dat";
	property_file.open(prop_filename.str().c_str());
	property_file << "EventNumber - NumTracksInEv - PMTsHit - HtrackFitChi2 - VtrackFitChi2 - LayersHit - TrackLength - StartX - StartY - StartZ - EndX - EndY - EndZ"<<std::endl;

	m_data->CStore.Get("channelkey_to_mrdpmtid",channelkey_to_mrdpmtid);

	return true;

}

bool MrdPaddleEfficiency::Execute(){

	// Get all relevant data from ANNIEEvent BoostStore

	m_data->Stores["ANNIEEvent"]->Get("EventNumber",EventNumber);
	m_data->Stores["ANNIEEvent"]->Get("MRDTriggerType",MRDTriggertype);

	// In case of a cosmic event, use the fitted track for an efficiency calculation

	if (MRDTriggertype == "Cosmic"){
		
		// Get MRD track information from MRDTracks BoostStore

		m_data->Stores["MRDTracks"]->Get("NumMrdSubEvents",numsubevs);
		m_data->Stores["MRDTracks"]->Get("NumMrdTracks",numtracksinev);

		if(verbosity>2) std::cout<<"MrdPaddleEfficiency tool: Event "<<EventNumber<<" had "<<numtracksinev<<" tracks in "<<numsubevs<<" subevents"<<endl;
			
		hist_numtracks->Fill(numtracksinev);
		
		// Only look at events with a track		

		if (numtracksinev > 0) 	{

			// Get reconstructed tracks

			m_data->Stores["MRDTracks"]->Get("MRDTracks",theMrdTracks);

			// Loop over reconstructed tracks
			
			paddlesInTrackReco.clear();

			for(int tracki=0; tracki<numtracksinev; tracki++){
				
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

				numpmtshit = PMTsHit.size();
				numlayershit = LayersHit.size();
				tracklength = sqrt(pow((StopVertex.X()-StartVertex.X()),2)+pow(StopVertex.Y()-StartVertex.Y(),2)+pow(StopVertex.Z()-StartVertex.Z(),2));
				if(PMTsHit.size()) paddlesInTrackReco.emplace(MrdTrackID,PMTsHit);

				if(verbosity>2) cout<<"MrdPaddleEfficiency tool: StartVertex: ("<<StartVertex.X()<<", "<<StartVertex.Y()<<", "<<StartVertex.Z()<<"), Stop Vertex: ("<<StopVertex.X()<<", "<<StopVertex.Y()<<", "<<StopVertex.Z()<<"), PMTsHit: "<<PMTsHit.size()<<", HtrackFitChi2: "<<HtrackFitChi2<<"< VtrackFitChi2: "<<VtrackFitChi2<<", LayersHit: "<<LayersHit.size()<<", tracklength: "<<tracklength<<endl;
				property_file << EventNumber <<", "<<numtracksinev<<", "<<PMTsHit.size()<<", "<<HtrackFitChi2<<", "<<VtrackFitChi2<<", "<<LayersHit.size()<<", "<<tracklength<<","<<StartVertex.X()<<","<<StartVertex.Y()<<","<<StartVertex.Z()<<","<<StopVertex.X()<<","<<StopVertex.Y()<<","<<StopVertex.Z()<<std::endl;

				// Fill properties into histograms

				hist_pmtshit->Fill(PMTsHit.size());
				hist_htrackfitchi2->Fill(HtrackFitChi2);
				hist_vtrackfitchi2->Fill(VtrackFitChi2);
				hist_layershit->Fill(LayersHit.size());
				hist_tracklength->Fill(tracklength);
				trackfit_tree->Fill();

				// Minor selection cuts to only select well fit tracks

				//if (PMTsHit.size() < 30 && HtrackFitChi2 > 0.005 && VtrackFitChi2 > 0.005){
				if (PMTsHit.size() < 50 ){

					for (int i_layer = 0; i_layer < zLayers.size(); i_layer++){
						
						// Exclude first and layer from efficiency determination						
						if (i_layer == 0 || i_layer == zLayers.size() -1) continue;

							double x_layer, y_layer;
							FindPaddleIntersection(StartVertex, StopVertex, x_layer, y_layer, zLayers.at(i_layer));
							if (verbosity > 2) std::cout <<"MrdPaddleEfficiency tool: FindPaddleIntersection found x_layer = "<<x_layer<<"& y_layer = "<<y_layer<<" for track with start position ("<<StartVertex.X()<<","<<StartVertex.Y()<<","<<StartVertex.Z()<<"), stop position ("<<StopVertex.X()<<","<<StopVertex.Y()<<","<<StopVertex.Z()<<") and z intersection point "<<zLayers.at(i_layer)<<std::endl;

							unsigned long hit_chankey=99999;
							FindPaddleChankey(x_layer, y_layer, i_layer,hit_chankey); 
							if (hit_chankey == 99999) {
								std::cout <<"FindMrdPaddleEfficiency: Did not find paddle with the desired intersection properties for this MRD layer, abort. "<<std::endl;
								continue;
							}
	
							if (orientationLayers.at(i_layer) == 0) {
								expected_MRDHits.at(i_layer).at(hit_chankey)->Fill(y_layer);
								if (std::find(PMTsHit.begin(),PMTsHit.end(),channelkey_to_mrdpmtid[hit_chankey])!=PMTsHit.end())  observed_MRDHits.at(i_layer).at(hit_chankey)->Fill(y_layer);
							}
							else {
								expected_MRDHits.at(i_layer).at(hit_chankey)->Fill(x_layer);
								if (std::find(PMTsHit.begin(),PMTsHit.end(),channelkey_to_mrdpmtid[hit_chankey])!=PMTsHit.end()) observed_MRDHits.at(i_layer).at(hit_chankey)->Fill(x_layer);
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
	trackfit_tree->Write();

	for (unsigned int i_layer = 0; i_layer < zLayers.size(); i_layer++){
		for (unsigned int i_ch = 0; i_ch < channelsLayers.at(i_layer).size(); i_ch++){
			unsigned long temp_chkey = channelsLayers.at(i_layer).at(i_ch);
			observed_MRDHits.at(i_layer).at(temp_chkey)->Write();
			expected_MRDHits.at(i_layer).at(temp_chkey)->Write();
		}
	}

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

bool MrdPaddleEfficiency::FindPaddleChankey(double x, double y, int layer, unsigned long &chankey){

	bool found_chankey = false;
	for (unsigned int i_channel = 0; i_channel < channelsLayers.at(layer).size(); i_channel++){

		if (found_chankey) break;

		unsigned long chankey_tmp = channelsLayers.at(layer).at(i_channel);
		Detector *mrdpmt = geom->ChannelToDetector(chankey_tmp);
		unsigned long detkey = mrdpmt->GetDetectorID();
		Paddle *mrdpaddle = (Paddle*) geom->GetDetectorPaddle(detkey);
		double xmin = mrdpaddle->GetXmin();
		double xmax = mrdpaddle->GetXmax();
		double ymin = mrdpaddle->GetYmin();
		double ymax = mrdpaddle->GetYmax();

		//Check if expected hit was within the channel or not
		if (xmin <= x && xmax >= x && ymin <= y && ymax >= y){
			chankey = chankey_tmp;
			found_chankey = true;
		}
	}	

	return true;

} 
