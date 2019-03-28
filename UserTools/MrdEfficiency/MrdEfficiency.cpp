/* vim:set noexpandtab tabstop=4 wrap */
#include "MrdEfficiency.h"

MrdEfficiency::MrdEfficiency():Tool(){}

bool MrdEfficiency::Initialise(std::string configfile, DataModel &data){
	
	if(verbosity) cout<<"Initializing tool MrdEfficiency"<<endl;
	
	/////////////////// Useful header ///////////////////////
	if(configfile!="") m_variables.Initialise(configfile);  // loading config file
	//m_variables.Print();
	
	m_data= &data; //assigning transient data pointer
	/////////////////////////////////////////////////////////
	
	// get configuration variables for this tool
	m_variables.Get("verbosity",verbosity);
	m_variables.Get("plotDirectory",plotDirectory);
	m_variables.Get("drawHistos",drawHistos);
	
	// check the output directory exists and is suitable
	bool isdir, plotDirectoryExists=false;
	struct stat s;
	if(stat(plotDirectory.c_str(),&s)==0){
		plotDirectoryExists=true;
		if(s.st_mode & S_IFDIR){        // mask to extract if it's a directory
			isdir=true;  //it's a directory
		} else if(s.st_mode & S_IFREG){ // mask to check if it's a file
			isdir=false; //it's a file
		} else {
			//assert(false&&"Check input path: stat says it's neither file nor directory..?");
		}
	} else {
		plotDirectoryExists=false;
		//assert(false&&"stat failed on input path! Is it valid?"); // error
		// errors could also be because this is a file pattern: e.g. wcsim_0.4*.root
		isdir=false;
	}
	
	if(drawHistos && (!plotDirectoryExists || !isdir)){
		Log("MrdEfficiency Tool: output directory "+plotDirectory+" does not exist or is not a writable directory; please check and re-run.",v_error,verbosity);
		return false;
	}
	
	// histograms
	// ~~~~~~~~~~
	if(drawHistos){
		canvwidth = 700;
		canvheight = 600;
		// create the ROOT application to show histograms
		int myargc=0;
		char *myargv[] = {(const char*)"mrdeff"};
		// get or make the TApplication
		intptr_t tapp_ptr=0;
		get_ok = m_data->CStore.Get("RootTApplication",tapp_ptr);
		if(not get_ok){
			Log("MrdEfficiency Tool: Making global TApplication",v_error,verbosity);
			rootTApp = new TApplication("rootTApp",&myargc,myargv);
			std::cout<<"rootTApp="<<rootTApp<<std::endl;
			tapp_ptr = reinterpret_cast<intptr_t>(rootTApp);
			m_data->CStore.Set("RootTApplication",tapp_ptr);
		} else {
			Log("MrdEfficiency Tool: Retrieving global TApplication",v_error,verbosity);
			rootTApp = reinterpret_cast<TApplication*>(tapp_ptr);
		}
		int tapplicationusers;
		get_ok = m_data->CStore.Get("RootTApplicationUsers",tapplicationusers);
		if(not get_ok) tapplicationusers=1;
		else tapplicationusers++;
		m_data->CStore.Set("RootTApplicationUsers",tapplicationusers);
		
		hnumcorrectlymatched = new TH1F("hnumtruematched","Number of True Tracks Matched",20,0,10);
		hnumtruenotmatched = new TH1F("hnumtruenotmatched","Number of True Tracks Not Matched",20,0,10);
		hnumreconotmatched = new TH1F("hnumreconotmatched","Number of Reconstructed Tracks Not Matched",20,0,10);
		
		// distributions of properties for primary muons that were successfully reconstructed
		hhangle_recod = new TH1F("hhangle_recod","Track Angle in Top View, Reconstructed",100,-TMath::Pi(),TMath::Pi());
		hvangle_recod = new TH1F("hvangle_recod","Track Angle in Side View, Reconstructed",100,-TMath::Pi(),TMath::Pi());
		htotangle_recod = new TH1F("htotangle_recod","Track Angle from Beam Axis, Reconstructed",100,0,TMath::Pi());
		henergyloss_recod = new TH1F("henergyloss_recod","Track Energy Loss in MRD, Reconstructed",100,0,2200);
		htracklength_recod = new TH1F("htracklength_recod","Total Track Length in MRD, Reconstructed",100,0,220);
		htrackpen_recod = new TH1F("htrackpen_recod","Track Penetration in MRD, Reconstructed",100,0,200);
		htrackstart_recod = new TH3D("htrackstart_recod","MRD Track Start Vertices, Reconstructed", 100,-170,170,100,300,480,100,-230,220);
		htrackstop_recod = new TH3D("htrackstop_recod","MRD Track Stop Vertices, Reconstructed", 100,-170,170,100,300,480,100,-230,220);
		hpep_recod = new TH3D("hpep_recod","Back Projected Tank Exit, Reconstructed", 100,-500,500,100,0,480,100,-330,320);
		hmpep_recod = new TH3D("hmpep_recod","Back Projected MRD Entry, Reconstructed", 100,-500,500,100,0,480,100,-330,320);
		
		// distributions of properties for primary muons that were not reconstructed
		hhangle_nrecod = new TH1F("hhangle_nrecod","Track Angle in Top View, Not Reconstructed",100,-TMath::Pi(),TMath::Pi());
		hvangle_nrecod = new TH1F("hvangle_nrecod","Track Angle in Side View, Not Reconstructed",100,-TMath::Pi(),TMath::Pi());
		htotangle_nrecod = new TH1F("htotangle_nrecod","Track Angle from Beam Axis, Not Reconstructed", 100,0,TMath::Pi());
		henergyloss_nrecod = new TH1F("henergyloss_nrecod","Track Energy Loss in MRD, Not Reconstructed", 100,0,2200);
		htracklength_nrecod = new TH1F("htracklength_nrecod","Total Track Length in MRD, Not Reconstructed", 100,0,220);
		htrackpen_nrecod = new TH1F("htrackpen_nrecod","Track Penetration in MRD, Not Reconstructed",100,0,200);
		htrackstart_nrecod = new TH3D("htrackstart_nrecod","MRD Track Start Vertices, Not Reconstructed", 100,-170,170,100,300,480,100,-230,220);
		htrackstop_nrecod = new TH3D("htrackstop_nrecod","MRD Track Stop Vertices, Not Reconstructed", 100,-170,170,100,300,480,100,-230,220);
		hpep_nrecod = new TH3D("hpep_nrecod","Back Projected Tank Exit, Not Reconstructed", 100,-500,500,100,0,480,100,-330,320);
		hmpep_nrecod = new TH3D("hmpep_nrecod","Back Projected MRD Entry, Not Reconstructed", 100,-500,500,100,0,480,100,-330,320);
		
	}
	
	return true;
}


bool MrdEfficiency::Execute(){
	
	if(verbosity) cout<<"Executing tool MrdEfficiency"<<endl;
	// To measure efficiency we need to try to match true tracks with reconstructed tracks
	// The eaiest way to do this is by comparing the hit paddles that make up the track.
	
	// retrieve the collections of PMTs hit by true particles
	get_ok = m_data->CStore.Get("ParticleId_to_MrdTubeIds", ParticleId_to_MrdTubeIds);
	if(not get_ok){
		Log("MrdEfficiency Tool: Could not find ParticleId_to_MrdTubeIds in CStore!",v_error,verbosity);
		return false;
	}
	m_data->Stores["ANNIEEvent"]->Get("EventNumber",EventNumber);
	
	// Get the true particles and which PMTs they hit
	// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	std::map<int,std::vector<int>> paddlesInTrackTrue;
	for(std::pair<const int,std::map<int,double>>& aparticle : *ParticleId_to_MrdTubeIds){
		std::map<int,double>* pmtshit = &aparticle.second;
		std::vector<int> tempvector;
		for(std::pair<const int,double>& apmt : *pmtshit){
			int pmtsid = apmt.first;
			tempvector.push_back(pmtsid);
		}
		if(tempvector.size()) paddlesInTrackTrue.emplace(aparticle.first, tempvector);
	}
	
	// Get the reconstructed particles from the BoostStore
	// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	get_ok = m_data->Stores["MRDTracks"]->Get("NumMrdSubEvents",numsubevs);
	if(not get_ok){
		Log("MrdEfficiency Tool: No NumMrdSubEvents in ANNIEEvent!",v_error,verbosity);
		return false;
	}
	get_ok = m_data->Stores["MRDTracks"]->Get("NumMrdTracks",numtracksinev);
	if(not get_ok){
		Log("MrdEfficiency Tool: No NumMrdTracks in ANNIEEvent!",v_error,verbosity);
		return false;
	}
	
	get_ok = m_data->Stores["MRDTracks"]->Get("MRDTracks", theMrdTracks);
	if(not get_ok){
		Log("MrdEfficiency Tool: No MRDTracks member of the MRDTracks BoostStore!",v_error,verbosity);
		Log("MRDTracks store contents:",v_error,verbosity);
		m_data->Stores["MRDTracks"]->Print(false);
		return false;
	}
	if(theMrdTracks->size()<numtracksinev){
		cerr<<"Too few entries in MRDTracks vector relative to NumMrdTracks!"<<endl;
		// more is fine as we don't shrink for efficiency
	}
	
	std::cout<<"MRDEfficiency tool reports "<<numtracksinev<<" true tracks found"<<std::endl;
	
	// Loop over reconstructed tracks and which PMTs they hit
	// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	std::map<int,std::vector<int>> paddlesInTrackReco;
	for(int tracki=0; tracki<numtracksinev; tracki++){
		BoostStore* thisTrackAsBoostStore = &(theMrdTracks->at(tracki));
		// Get the track details from the BoostStore
		PMTsHit.clear();
		thisTrackAsBoostStore->Get("PMTsHit",PMTsHit);
		thisTrackAsBoostStore->Get("MrdTrackID",MrdTrackID);
		if(PMTsHit.size()) paddlesInTrackReco.emplace(MrdTrackID,PMTsHit);
	}
	
	logmessage = "MrdEfficiency Tool: Event had "+to_string(paddlesInTrackReco.size()) 
				 + " reconstructed tracks and " + to_string(paddlesInTrackTrue.size())
				 + " true particles that hit MRD PMTs";
	Log(logmessage,v_debug,verbosity);
	
	// Produce a matrix of figures of merit for how well each reco track matches each true particle
	// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	std::vector<std::vector<double> > matchmerits (paddlesInTrackReco.size(), std::vector<double>(paddlesInTrackTrue.size(),0));
	for(int recotracki=0; recotracki<paddlesInTrackReco.size(); recotracki++){
		// get the next reco track
		std::map<int,std::vector<int>>::iterator arecotrack = paddlesInTrackReco.begin();
		std::advance(arecotrack,recotracki);
		int RecoTrackId = arecotrack->first;
		std::vector<int> reco_tracks_paddles = arecotrack->second;
		
		logmessage = "MrdEfficiency Tool: Scanning matches against recotrack "+to_string(recotracki)
						+" which hit "+to_string(reco_tracks_paddles.size())+" tubes";
		Log(logmessage,v_debug,verbosity);
		
		// scan through the true tracks and calculate FOM against this reco track
		for(int truetracki=0; truetracki<paddlesInTrackTrue.size(); truetracki++){
			// get the next true track
			std::map<int,std::vector<int>>::iterator atruetrack = paddlesInTrackTrue.begin();
			std::advance(atruetrack,truetracki);
			int TrueTrackId = atruetrack->first;
			std::vector<int> true_tracks_paddles = atruetrack->second;
			
			logmessage = "MrdEfficiency Tool: Making figure-of-merit with true track "+to_string(truetracki)
							+", which hit "+to_string(true_tracks_paddles.size())+" tubes";
			Log(logmessage,v_debug,verbosity);
			// measure how well these two sets of paddles match and assign a merit
			int num_true_paddles_matched = 0;
			int num_true_paddles_unmatched = 0;
			int num_extra_reco_paddles = 0;
			
			// scan over the reconstructed paddles
			for(int arecopaddlei=0; arecopaddlei<reco_tracks_paddles.size(); arecopaddlei++){
				int therecopaddleid = reco_tracks_paddles.at(arecopaddlei);
				// see if this paddle is also in the true paddles
				bool in_true = std::count(true_tracks_paddles.begin(),true_tracks_paddles.end(),therecopaddleid);
				if(in_true) num_true_paddles_matched++;
				else num_extra_reco_paddles++;
			}
			// scan over the true paddles
			for(int atruepaddlei=0; atruepaddlei<true_tracks_paddles.size(); atruepaddlei++){
				int thetruepaddleid = true_tracks_paddles.at(atruepaddlei);
				bool in_reco = std::count(reco_tracks_paddles.begin(),reco_tracks_paddles.end(),thetruepaddleid);
				if(not in_reco) num_true_paddles_unmatched++;
			}
			
			// combine the three values and the number of paddles in each track to form an FOM
			double thisfom = 0;
			thisfom +=   num_true_paddles_matched;
			thisfom += - num_true_paddles_unmatched;
			thisfom += - num_extra_reco_paddles;
			thisfom += - abs(static_cast<double>(reco_tracks_paddles.size()) -
							 static_cast<double>(true_tracks_paddles.size()));
			
			// add the merit to the matrix
			logmessage = "MrdEfficiency Tool: Final figure-of-merit for recotrack "+to_string(recotracki)
				+" with true track "+to_string(truetracki) + ": " + to_string(thisfom);
			Log(logmessage,v_debug,verbosity);
			matchmerits.at(recotracki).at(truetracki)=thisfom;
		}
	}
	
	Log("MrdEfficiency Tool: Matching reco vs true tracks",v_debug,verbosity);
	// having assigned figures of merit for how well each true track matches each reconstructed track,
	// scan over this matrix and pull out the best matches, stopping once we've run out or hit a FOM
	// lower limit
	double fomthreshold=2.0; // a pair of tracks must have at least this figure-of-merit to be matched
	
	// start of track pairing
	//~~~~~~~~~~~~~~~~~~~~~~~
	std::map<int,int> Reco_to_True_Id_Map;
	std::map<int,int> True_to_Reco_Id_Map;
	while(true){
		double currentmax=0.;
		int maxrow=-1, maxcolumn=-1;
		for(int rowi=0; rowi<matchmerits.size(); rowi++){
			std::vector<double> therow = matchmerits.at(rowi);
			std::vector<double>::iterator thisrowsmaxit = std::max_element(therow.begin(), therow.end());
			if((thisrowsmaxit!=therow.end())&&((*thisrowsmaxit)>currentmax)){
				currentmax=*thisrowsmaxit; 
				maxrow = rowi;
				maxcolumn = std::distance(therow.begin(),thisrowsmaxit);
			}
		}
		
		logmessage="MrdEfficiency Tool: Next max FOM in the matrix is "+to_string(currentmax);
		Log(logmessage,v_debug,verbosity);
		// find the maximum of the matchmerits matrix, allocate that pair, mask off that row/column, and repeat
		if(currentmax>fomthreshold){
			bool makethepair=true;
			std::map<int,std::vector<int>>::iterator aparticle = paddlesInTrackReco.begin();
			std::advance(aparticle,maxrow);
			std::vector<int>* recotrackpmts = &(aparticle->second);
			aparticle = paddlesInTrackTrue.begin();
			std::advance(aparticle,maxcolumn);
			std::vector<int>* truetrackpmts = &(aparticle->second);
			
			// print the matching
			logmessage = "MrdEfficiency Tool: Candidate track "+to_string(Reco_to_True_Id_Map.size())
				+" reco has "+to_string(recotrackpmts->size())+" PMTs; {";
			for(auto&& apmt : (*recotrackpmts)){ logmessage+=to_string(apmt)+", "; }
			logmessage += 
				"}, true track has "+to_string(truetrackpmts->size()) +" PMTs; {";
			for(auto&& apmt : (*truetrackpmts)){ logmessage+=to_string(apmt)+", "; }
			logmessage += "}\n";
			Log(logmessage,v_debug,verbosity);
			
			// add this matching to the list of pairs
			if(makethepair){
				logmessage = "MrdEfficiency Tool: Matching reco track "
					+ to_string(maxrow) +" to true track "+to_string(maxcolumn)
					+ ", match FOM "+to_string(currentmax)+"\n";
				Log(logmessage,v_debug,verbosity);
				Reco_to_True_Id_Map.emplace(std::make_pair(maxrow, maxcolumn));  // TODO needs to get Ids
				True_to_Reco_Id_Map.emplace(std::make_pair(maxcolumn,maxrow));   // TODO needs to get Ids
			}
			// remove the matched tracks from the matchmerits matrix
			matchmerits.at(maxrow).assign(matchmerits.at(maxrow).size(),-1);
			for(auto&& avec : matchmerits) avec.at(maxcolumn) = -1;
			
		} else {
			break;  // the maximum FOM between any remaining tracks is insufficient. We're done here.
		}
	}
	// end of track pairing
	//~~~~~~~~~~~~~~~~~~~~~
	
	// put the matching map into the store
	m_data->CStore.Set("Reco_to_True_Id_Map",Reco_to_True_Id_Map);
	
	// now measure the efficiency by comparing how many tracks were correctly matched,
	// how many true tracks were not reconstructed, and how many reconstructed tracks had
	// no true particle (perhaps several true particles created hits that looked like a track)
	int num_correctly_matched_tracks=0;
	int num_true_tracks_not_reconstructed=0;
	int num_reco_tracks_without_match=0;
	for(std::pair<const int,std::vector<int>>& apaddleset : paddlesInTrackReco){
		int reco_track_id = apaddleset.first;
		bool is_matched = Reco_to_True_Id_Map.count(reco_track_id);
		if(is_matched) num_correctly_matched_tracks++;
		else num_reco_tracks_without_match++;
	}
	for(std::pair<const int,std::vector<int>>& apaddleset : paddlesInTrackTrue){
		int true_track_id = apaddleset.first;
		bool is_matched = True_to_Reco_Id_Map.count(true_track_id);
		if(not is_matched) num_true_tracks_not_reconstructed++;
	}
	
	logmessage = "MrdEfficiency Tool: This event had "+to_string(num_correctly_matched_tracks)
		+ " matched tracks, " + to_string(num_true_tracks_not_reconstructed)
		+ " true tracks not reconstructed and " + to_string(num_reco_tracks_without_match)
		+ " reconstructed tracks that had no recorded true particle";
	Log(logmessage,v_message,verbosity);
	
	// update the histograms
	if(drawHistos){
		hnumcorrectlymatched->Fill(num_correctly_matched_tracks);
		hnumtruenotmatched->Fill(num_true_tracks_not_reconstructed);
		hnumreconotmatched->Fill(num_reco_tracks_without_match);
	}
	
	// find the primary muon to check if it was reconstructed
	// loop over the MCParticles to find the highest enery primary muon
	get_ok = m_data->Stores.at("ANNIEEvent")->Get("MCParticles",MCParticles);
	if(not get_ok){
		Log("MrdEfficiency Tool: Error retrieving MCParticles,true from ANNIEEvent!",v_error,verbosity);
		return false;
	}
	// MCParticles is a std::vector<MCParticle>
	MCParticle primarymuon;  // primary muon
	bool mufound=false;
	if(MCParticles){
		Log("MrdEfficiency Tool: Num MCParticles = "+to_string(MCParticles->size()),v_message,verbosity);
		for(int particlei=0; particlei<MCParticles->size(); particlei++){
			MCParticle aparticle = MCParticles->at(particlei);
			if(aparticle.GetPdgCode()==13){
				logmessage = "True muon found with parent type " + to_string(aparticle.GetParentPdg())
					+ ", Id " + to_string(aparticle.GetParticleID())
					+ ", start vertex (" + to_string(aparticle.GetStartVertex().X())
					+ ", " + to_string(aparticle.GetStartVertex().Y())
					+ ", " + to_string(aparticle.GetStartVertex().Z())
					+ "), and end vertex (" + to_string(aparticle.GetStopVertex().X())
					+ ", " + to_string(aparticle.GetStopVertex().Y())
					+ ", " + to_string(aparticle.GetStopVertex().Z())
					+ ")";
				Log(logmessage,v_debug,verbosity);
			}
			if(aparticle.GetParentPdg()!=0) continue;      // not a primary particle
			if(aparticle.GetPdgCode()!=13) continue;       // not a muon
			primarymuon = aparticle;                       // note the particle
			mufound=true;                                  // note that we found it
			break;                                         // XXX assume we don't have more than one primary muon
		}
	} else {
		Log("MrdEfficiency Tool: No MCParticles in the event!",v_warning,verbosity);
	}
	if(not mufound){
		Log("MrdEfficiency Tool: No muon in this event",v_debug,verbosity);
		return true;
	}
	
	// scan the vector of MRD tubes hit by the primary muon, if any.
	// if none, this will not count toward the efficiency
	if(ParticleId_to_MrdTubeIds->count(primarymuon.GetParticleID()==0)){
		Log("MrdEfficiency Tool: primary muon did not hit the MRD",v_debug,verbosity);
		num_primary_muons_that_missed_MRD++;
	} else {
		num_primary_muons_that_hit_MRD++;
		// check if we reconstructed it
		if(True_to_Reco_Id_Map.count(primarymuon.GetParticleID())){
			num_primary_muons_reconstructed++;
			
			// update the histos
			if(drawHistos){
				hhangle_recod->Fill(primarymuon.GetTrackAngleX());
				hvangle_recod->Fill(primarymuon.GetTrackAngleY());
				htotangle_recod->Fill(primarymuon.GetTrackAngleFromBeam());
				henergyloss_recod->Fill(primarymuon.GetMrdEnergyLoss());
				htracklength_recod->Fill(primarymuon.GetTrackLengthInMrd());
				htrackpen_recod->Fill(primarymuon.GetMrdPenetration());
				// truth tank exit point
				hpep_recod->Fill(primarymuon.GetTankExitPoint().X(),primarymuon.GetTankExitPoint().Y(),primarymuon.GetTankExitPoint().Z());
				// truth mrd entry point
				hmpep_recod->Fill(primarymuon.GetMrdEntryPoint().X(),primarymuon.GetMrdEntryPoint().Y(),primarymuon.GetMrdEntryPoint().Z());
				// truth track endpoint (if in MRD) or MRD exit point
				htrackstop_recod->Fill(primarymuon.GetMrdExitPoint().X(), primarymuon.GetMrdExitPoint().Y(), primarymuon.GetMrdExitPoint().Z());
			}
		} else {
			num_primary_muons_not_reconstructed++;
			
			// update the histos
			if(drawHistos){
				hhangle_nrecod->Fill(primarymuon.GetTrackAngleX());
				hvangle_nrecod->Fill(primarymuon.GetTrackAngleY());
				htotangle_nrecod->Fill(primarymuon.GetTrackAngleFromBeam());
				henergyloss_nrecod->Fill(primarymuon.GetMrdEnergyLoss());
				htracklength_nrecod->Fill(primarymuon.GetTrackLengthInMrd());
				htrackpen_nrecod->Fill(primarymuon.GetMrdPenetration());
				// truth tank exit point
				hpep_nrecod->Fill(primarymuon.GetTankExitPoint().X(),primarymuon.GetTankExitPoint().Y(),primarymuon.GetTankExitPoint().Z());
				// truth mrd entry point
				hmpep_nrecod->Fill(primarymuon.GetMrdEntryPoint().X(),primarymuon.GetMrdEntryPoint().Y(),primarymuon.GetMrdEntryPoint().Z());
				// truth track endpoint (if in MRD) or MRD exit point
				htrackstop_nrecod->Fill(primarymuon.GetMrdExitPoint().X(),primarymuon.GetMrdExitPoint().Y(), primarymuon.GetMrdExitPoint().Z());
			}
		}
	};
	
	return true;
}

bool MrdEfficiency::Finalise(){
	
	logmessage = "MrdEfficiency Tool: Counted "+to_string(num_primary_muons_that_hit_MRD)
		+ " primary muons that entered the MRD, of which "+to_string(num_primary_muons_reconstructed)
		+ " (" + to_string(100.*double(num_primary_muons_reconstructed)/double(num_primary_muons_that_hit_MRD))
		+ "%) were successfully reconstructed while "+to_string(num_primary_muons_not_reconstructed)
		+ " (" + to_string(100.*double(num_primary_muons_not_reconstructed)/double(num_primary_muons_that_hit_MRD))
		+ "%) were not reconstructed. "+to_string(num_primary_muons_that_missed_MRD)
		+" muons missed the MRD";
	Log(logmessage,v_message,verbosity);
	
	if(drawHistos){
		mrdEffCanv = new TCanvas("mrdEffCanv","",canvwidth,canvheight);
		mrdEffCanv->cd();
		std::string imgname;
		
		hhangle_recod->Draw();
		hhangle_nrecod->Draw("same");
		imgname=hhangle_recod->GetTitle();
		std::replace(imgname.begin(), imgname.end(), ' ', '_');
		mrdEffCanv->SaveAs(TString::Format("%s/%s.png",plotDirectory.c_str(),imgname.c_str()));
		hvangle_recod->Draw();
		hvangle_nrecod->Draw("same");
		imgname=hvangle_recod->GetTitle();
		std::replace(imgname.begin(), imgname.end(), ' ', '_');
		mrdEffCanv->SaveAs(TString::Format("%s/%s.png",plotDirectory.c_str(),imgname.c_str()));
		htotangle_recod->Draw();
		htotangle_nrecod->Draw("same");
		imgname=htotangle_recod->GetTitle();
		std::replace(imgname.begin(), imgname.end(), ' ', '_');
		mrdEffCanv->SaveAs(TString::Format("%s/%s.png",plotDirectory.c_str(),imgname.c_str()));
		henergyloss_recod->Draw();
		henergyloss_nrecod->Draw("same");
		imgname=henergyloss_recod->GetTitle();
		std::replace(imgname.begin(), imgname.end(), ' ', '_');
		mrdEffCanv->SaveAs(TString::Format("%s/%s.png",plotDirectory.c_str(),imgname.c_str()));
		htracklength_recod->Draw();
		htracklength_nrecod->Draw("same");
		imgname=htracklength_recod->GetTitle();
		std::replace(imgname.begin(), imgname.end(), ' ', '_');
		mrdEffCanv->SaveAs(TString::Format("%s/%s.png",plotDirectory.c_str(),imgname.c_str()));
		htrackpen_recod->Draw();
		htrackpen_nrecod->Draw("same");
		imgname=htrackpen_recod->GetTitle();
		std::replace(imgname.begin(), imgname.end(), ' ', '_');
		mrdEffCanv->SaveAs(TString::Format("%s/%s.png",plotDirectory.c_str(),imgname.c_str()));
		hpep_recod->Draw();
		hpep_nrecod->Draw("same");
		imgname=hpep_recod->GetTitle();
		std::replace(imgname.begin(), imgname.end(), ' ', '_');
		mrdEffCanv->SaveAs(TString::Format("%s/%s.png",plotDirectory.c_str(),imgname.c_str()));
		hmpep_recod->Draw();
		hmpep_nrecod->Draw("same");
		imgname=hmpep_recod->GetTitle();
		std::replace(imgname.begin(), imgname.end(), ' ', '_');
		mrdEffCanv->SaveAs(TString::Format("%s/%s.png",plotDirectory.c_str(),imgname.c_str()));
		htrackstop_recod->Draw();
		htrackstop_nrecod->Draw("same");
		imgname=htrackstop_recod->GetTitle();
		std::replace(imgname.begin(), imgname.end(), ' ', '_');
		mrdEffCanv->SaveAs(TString::Format("%s/%s.png",plotDirectory.c_str(),imgname.c_str()));
		
		// cleanup
		std::vector<TH1*> histos {hhangle_recod, hvangle_recod, htotangle_recod, henergyloss_recod, htracklength_recod, htrackpen_recod, hpep_recod, htrackstop_recod, hhangle_nrecod, hvangle_nrecod, htotangle_nrecod, henergyloss_nrecod, htracklength_nrecod, htrackpen_nrecod, hpep_nrecod, htrackstop_nrecod};
		std::cout<<"deleting histograms"<<std::endl;
		for(TH1* ahisto : histos){ if(ahisto) delete ahisto; ahisto=nullptr; }
		
		Log("MrdEfficiency Tool: deleting canvas",v_debug,verbosity);
		if(gROOT->FindObject("mrdEffCanv")){ delete mrdEffCanv; mrdEffCanv=nullptr; }
		
		int tapplicationusers=0;
		get_ok = m_data->CStore.Get("RootTApplicationUsers",tapplicationusers);
		if(not get_ok || tapplicationusers==1){
			if(rootTApp){
				Log("MrdDistributions Tool: deleting gloabl TApplication",v_debug,verbosity);
				delete rootTApp;
			}
		} else if (tapplicationusers>1){
			tapplicationusers--;
			m_data->CStore.Set("RootTApplicationUsers",tapplicationusers);
		}
	}
	
	return true;
}
