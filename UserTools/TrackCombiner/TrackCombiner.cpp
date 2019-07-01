/* vim:set noexpandtab tabstop=4 wrap */
#include "TrackCombiner.h"

TrackCombiner::TrackCombiner():Tool(){}


bool TrackCombiner::Initialise(std::string configfile, DataModel &data){
	
	/////////////////// Useful header ///////////////////////
	if(configfile!="") m_variables.Initialise(configfile); //loading config file
	//m_variables.Print();
	
	m_data= &data; //assigning transient data pointer
	/////////////////////////////////////////////////////////////////
	
	return true;
}


bool TrackCombiner::Execute(){
	
	// Get the reconstructed event vertex from the RecoEvent store
	// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	if(m_data->Stores.count("RecoEvent")==0){
		Log("TrackCombiner Tool: No RecoEvent booststore! Need to run vertex reconstruction tools first!",v_error,verbosity);
		return false;
	}
	RecoVertex* theExtendedVertex=nullptr;
	get_ok = m_data->Stores.at("RecoEvent")->Get("ExtendedVertex", theExtendedVertex);
	if((get_ok==0)||(theExtendedVertex==nullptr)){
		Log("TrackCombiner Tool: Failed to retrieve the ExtendedVertex from RecoEvent Store!",v_error,verbosity);
		return false;
	}
	// Check we successfully reconstructed a tank vertex
	int recoStatus = theExtendedVertex->GetStatus();
	double recoVtxFOM = theExtendedVertex->GetFOM();
	Log("TrackCombiner Tool: recoVtxFOM="+to_string(recoVtxFOM),v_debug,verbosity);
	if(recoVtxFOM<0){
		Log("TrackCombiner Tool: Vertex reconstruction failed, skipping",v_message,verbosity);
		return true;
	}
	Log("TrackCombiner Tool: reconstructed event Vertex found",v_debug,verbosity);
	
	// Get the reconstructed MRD tracks from the MRDTracks store
	// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	// Get the counts for this event
	if(m_data->Stores.count("MRDTracks")==0){
		Log("TrackCombiner Tool: No MRDTracks booststore! Need to run FindMrdTracks tool first!",v_error,verbosity);
		return false;
	}
	get_ok = m_data->Stores.at("MRDTracks")->Get("NumMrdSubEvents",numsubevs);
	if(not get_ok){
		Log("TrackCombiner Tool: No NumMrdSubEvents in MRDTracks store!",v_error,verbosity);
		return false;
	}
	get_ok = m_data->Stores.at("MRDTracks")->Get("NumMrdTracks",numtracksinev);
	if(not get_ok){
		Log("TrackCombiner Tool: No NumMrdTracks in MRDTracks store!",v_error,verbosity);
		return false;
	}
	// check if we had any reconstructed tracks
	get_ok = m_data->Stores.at("MRDTracks")->Get("MRDTracks", theMrdTracks);
	if(not get_ok){
		Log("TrackCombiner Tool: No MRDTracks member of the MRDTracks BoostStore!",v_error,verbosity);
		Log("MRDTracks store contents:",v_error,verbosity);
		m_data->Stores.at("MRDTracks")->Print(false);
		return false;
	}
	if(theMrdTracks->size()<numtracksinev){
		Log("TrackCombiner Tool: Too few entries in MRDTracks vector relative to NumMrdTracks!",v_warning,verbosity);
		// more is fine as we don't shrink for efficiency
	}
	
	Log("TrackCombiner Tool: "+to_string(numtracksinev)+" reconstructed MRD tracks in this event",v_debug,verbosity);
	
	// Get the reconstructed event time
	// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	double recoEventStartTime = theExtendedVertex->GetTime();
/* recoVtx methods:
	Position GetPosition() { return fPosition;}
	double GetTime() { return fTime; }
	bool FoundVertex() { return fFoundVertex; }
	Direction GetDirection() { return fDirection; }
	bool FoundDirection() { return fFoundDirection; }
	double GetConeAngle() { return fConeAngle; }
	double GetTrackLength() { return fTrackLength; }
	double GetFOM() { return fFOM; }
	int GetIterations(){ return fIterations; }
	bool GetPass() { return fPass; }
	int GetStatus(){ return fStatus; }
*/
	
	// Loop over reconstructed tracks and see whose track time best matches the reconstructed event vertex time
	// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	int best_match_index=-1;
	double mintimediff=99999999999;
	// what criteria do we want to use to choose our best match?
	// closest in time seems sensible, but do we want to prefer longer MRD tracks than stubs?
	for(int tracki=0; tracki<numtracksinev; tracki++){
		BoostStore* thisTrackAsBoostStore = &(theMrdTracks->at(tracki));
		// Get the track details from the BoostStore
		double mrdTrackStartTime;
		get_ok = thisTrackAsBoostStore->Get("StartTime",mrdTrackStartTime);
		if(not get_ok){
			Log("TrackCombiner Tool: Failed to get StartTime from reco MRD Track BoostStore!",v_error,verbosity);
			return false;
		}
/* MrdTrack methods:
		thisTrackAsBoostStore->Get("MrdTrackID",MrdTrackID);                    // int
		thisTrackAsBoostStore->Get("MrdSubEventID",MrdSubEventID);              // int
		thisTrackAsBoostStore->Get("InterceptsTank",InterceptsTank);            // bool
		thisTrackAsBoostStore->Get("StartTime",StartTime);                      // [ns]
		thisTrackAsBoostStore->Get("StartVertex",StartVertex);                  // [m]
		thisTrackAsBoostStore->Get("StopVertex",StopVertex);                    // [m]
		thisTrackAsBoostStore->Get("TrackAngle",TrackAngle);                    // [rad]
		thisTrackAsBoostStore->Get("TrackAngleError",TrackAngleError);          // dx/dz  <not yet implemented>
		thisTrackAsBoostStore->Get("HtrackOrigin",HtrackOrigin);                // x posn at global z=0, [cm]
		thisTrackAsBoostStore->Get("HtrackOriginError",HtrackOriginError);      // [cm]
		thisTrackAsBoostStore->Get("HtrackGradient",HtrackGradient);            // dx/dz
		thisTrackAsBoostStore->Get("HtrackGradientError",HtrackGradientError);  // [no units]
		thisTrackAsBoostStore->Get("VtrackOrigin",VtrackOrigin);                // [cm]
		thisTrackAsBoostStore->Get("VtrackOriginError",VtrackOriginError);      // [cm]
		thisTrackAsBoostStore->Get("VtrackGradient",VtrackGradient);            // dy/dz
		thisTrackAsBoostStore->Get("VtrackGradientError",VtrackGradientError);  // [no units]
		thisTrackAsBoostStore->Get("LayersHit",LayersHit);                      // vector<int>
		thisTrackAsBoostStore->Get("TrackLength",TrackLength);                  // [m]
		thisTrackAsBoostStore->Get("EnergyLoss",EnergyLoss);                    // [MeV]
		thisTrackAsBoostStore->Get("EnergyLossError",EnergyLossError);          // [MeV]
		thisTrackAsBoostStore->Get("IsMrdPenetrating",IsMrdPenetrating);        // bool
		thisTrackAsBoostStore->Get("IsMrdStopped",IsMrdStopped);                // bool
		thisTrackAsBoostStore->Get("IsMrdSideExit",IsMrdSideExit);              // bool
		thisTrackAsBoostStore->Get("PenetrationDepth",PenetrationDepth);        // [m]
		thisTrackAsBoostStore->Get("HtrackFitChi2",HtrackFitChi2);              // 
		thisTrackAsBoostStore->Get("HtrackFitCov",HtrackFitCov);                // 
		thisTrackAsBoostStore->Get("VtrackFitChi2",VtrackFitChi2);              // 
		thisTrackAsBoostStore->Get("VtrackFitCov",VtrackFitCov);                // 
		thisTrackAsBoostStore->Get("PMTsHit",PMTsHit);                          // 
		thisTrackAsBoostStore->Get("TankExitPoint",TankExitPoint);              // [m]
		thisTrackAsBoostStore->Get("MrdEntryPoint",MrdEntryPoint);              // [m]
*/
		double timediff = abs(mrdTrackStartTime - recoEventStartTime);
		if(timediff<mintimediff){
			// FIXME what other matching criteria do we want to use?
			// how aligned the tracks appear to be?
			// how close the projected tank exit point is?
			// other?
			mintimediff = timediff;
			best_match_index=tracki;
		}
	}
	
	// FIXME what criteria do we want to place as a minimum?
	// even if we have 1 MRD track and 1 reco vertex, we should be open to the possiblity
	// that they are not the same track, and it may be better to fall back to matching unassigned
	// MRD hits to the vertex.
	if((best_match_index>=0) && (mintimediff<max_allowed_tdiff)){
		Log("TrackCombiner Tool: Matched MRD track "+to_string(best_match_index)+" to the RecoVertex!",v_debug,verbosity);
		BoostStore* primaryEventMrdTrack = &(theMrdTracks->at(best_match_index));
		m_data->Stores.at("ANNIEEvent")->Set("PrimaryEventRecoMrdTrack",primaryEventMrdTrack);
	} else {
		Log("TrackCombiner Tool: Failed to match any MRD tracks to the RecoVertex, looking for matching TDC hits",v_debug,verbosity);
	}
	
	// If we didn't find a matching MRD track, look for TDC hits in close temporal proximity.
	// We'll try to make a rough estimate of MRD penetration from any MRD hits found.
	
	// Regardless of whether we matched an MRD track, it's worth doing this scan either way,
	// to search for Veto hits to check for upstream activity.
	
	// First get the TDCHits:
	get_ok = m_data->Stores.at("ANNIEEvent")->Get("TDCData",TDCData);  // a std::map<unsigned long,vector<MCHit>>
	if(not get_ok){
		Log("TrackCombiner Tool: Failed to get TDCData from ANNIEEvent",v_error,verbosity);
		return false;
	}
	if(TDCData->size()==0){
		Log("TrackCombiner Tool: No TDC hits to match",v_message,verbosity);
		return true;
	}
	
	Log("TrackCombiner Tool: Scanning "+to_string(TDCData->size())+" TDC channels for hits near the recoVtx time",v_debug,verbosity);
	std::map<unsigned long,std::vector<double>> vetohits;   // map of PMT channelkey to times of the hits on it
	std::map<unsigned long,std::vector<double>> mrdhits;    // as above
	for(std::pair<unsigned long,vector<MCHit>>& anmrdpmt : (*TDCData)){
		// retrieve the pmt information
		unsigned long chankey = anmrdpmt.first;
		Detector* thedetector = geo->ChannelToDetector(chankey);
		bool thisismrdpmt= (thedetector->GetDetectorElement()=="MRD");  // else vetopmt
		
		// loop over hits on this pmt
		for(auto&& hitsonthismrdpmt : anmrdpmt.second){
			double thishittime = hitsonthismrdpmt.GetTime();
			if(abs(thishittime-recoEventStartTime)<max_allowed_tdiff){
				// this hit was about the time of the reconstructed event time
				// add it to the appropriate map
				std::map<unsigned long,std::vector<double>>* themap = (thisismrdpmt) ? mrdhits : vetohits;
				if(themap->count(chankey)==0){
					themap->emplace(chankey,std::vector<double>{thishittime});
				} else {
					themap->at(chankey).push_back(thishittime);
				}
			}  // outside time window
		} // loop over hits on this TDC channel
	} // loop over TDC channels
	
	// if we failed to match a reco MRD track to this recoVtx, see what we can do with the MRD hits we found
	if((best_match_index<0) && (mrdhits.size()>0)){
		
		BoostStore* short_track = FindShortMrdTracks(mrdhits);
		
		if(short_track){
			// hey, we managed to reconstruct a short Mrd track!
			Log("TrackCombiner Tool: Reconstructed a short MRD Track!",v_message,verbosity);
			// can use short_track here to print properties / whatever else if we desire.
			// it has already been added to the stores though, so nothing required.
		}
	}
	
	// TODO processing of the prompt veto digits; e.g. we could try to identify
	// a veto penetration by requiring hits in two adjacent paddles in the two layers...
	// but given our efficiency is expected to be low, we may not wish to apply this as a cut.
	// Nonetheless, it's worth doing
	bool matchedvetopair=false;
	for(std::pair<unsigned long,vector<double>>& avetopmt : vetohits){
		unsigned long chankey = avetopmt.first;
		Detector* thedetector = geo->ChannelToDetector(chankey);
		// TODO find position, check layer, adjacency
		// set matchedvetopair if found
	}
	// TODO pass this to the ANNIEEvent
	
	return true;
}


bool TrackCombiner::Finalise(){
	
	return true;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////


BoostStore* TrackCombiner::FindShortMrdTracks(std::map<unsigned long,vector<double>> mrdhits){
	// should we require hits in the front 2 layers, at least?
	// or hits in N layers, at least?
	// should we require these hit paddles be within a given proximity of each other?
	// how do we determine penetration without a proper track?
	// taking the deepest hit might be noisy...
	// we could try to find the deepest hit with a hit in every preceding layer...
	std::vector<int> hitlayers;
	for(std::pair<unsigned long,vector<double>>& anmrdpmt : mrdhits){
		unsigned long chankey = anmrdpmt.first;
		Detector* thedetector = geo->ChannelToDetector(chankey);
		
	}
	
	if(tracksfound==0){
		return nullptr;
	}
	// else, we found a short track! Let's add it to the MrdTracks BoostStore.
	
	// first handle the subevent count: if we have no subevents, we should set the counter to 1,
	// just in case other tools assume no subevents imply no tracks.
	if(numsubevs==0){
		numsubevs++;
		m_data->Stores.at("MRDTracks")->Set("NumMrdSubEvents",numsubevs);
	}
	
	// similarly increment the track count to accommodate the new track
	numtracksinev++;
	m_data->Stores.at("MRDTracks")->Set("NumMrdTracks",numtracksinev);
	
	// And add this Track to the MrdTracks BoostStore.
	// first checkif we need to grow the container
	if(numtracksinev>theMrdTracks->size()){
		Log("TrackCombiner Tool: Growing theMrdTracks vector to size "+to_string(numtracksinev),v_debug,verbosity);
		theMrdTracks->resize(numtracksinev);
	}
	// Get the track
	BoostStore* thisTrackAsBoostStore = &(theMrdTracks->at(numtracksinev-1));
	
	// and set the new track's properties
	// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	// Since we don't remove and re-create the BoostStores each Execute, any members set here
	// which are not updated by the FindMrdTracks tool will result in spurious track properties!
	// !!!!!!!!!!!!! It is therefore crucial to keep these two Tools in sync! !!!!!!!!!!!!!!!
	// XXX XXX XXX XXX XXX XXX XXX XXX XXX XXX XXX XXX XXX XXX XXX XXX XXX XXX XXX XXX XXX XXX
	// IF ADDING MEMBERS TO THE MRDTRACK BOOSTSTORE HERE, ADD THEM TO THE FINDMRDTRACKS TOO!
	// XXX XXX XXX XXX XXX XXX XXX XXX XXX XXX XXX XXX XXX XXX XXX XXX XXX XXX XXX XXX XXX XXX
	// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	thisTrackAsBoostStore->Set("MrdTrackID",atrack->GetTrackID());
	thisTrackAsBoostStore->Set("MrdSubEventID",atrack->GetMrdSubEventID());
	thisTrackAsBoostStore->Set("InterceptsTank",atrack->GetInterceptsTank());
	thisTrackAsBoostStore->Set("StartTime",atrack->GetStartTime());
	// convert start posn from TVector3 to ANNIEEVENT Position class
	Position startpos(atrack->GetStartVertex().X() / 100.,
					  atrack->GetStartVertex().Y() / 100.,
					  atrack->GetStartVertex().Z() / 100.);
	// same for endpos
	Position endpos(  atrack->GetStopVertex().X() / 100.,
					  atrack->GetStopVertex().Y() / 100.,
					  atrack->GetStopVertex().Z() / 100.);
	thisTrackAsBoostStore->Set("StartVertex",startpos);
	thisTrackAsBoostStore->Set("StopVertex",endpos);
	thisTrackAsBoostStore->Set("TrackAngle",atrack->GetTrackAngle());
	thisTrackAsBoostStore->Set("TrackAngleError",atrack->GetTrackAngleError()); // TODO
	thisTrackAsBoostStore->Set("LayersHit",atrack->GetLayersHit());
	thisTrackAsBoostStore->Set("TrackLength",atrack->GetTrackLength() / 100.);
	thisTrackAsBoostStore->Set("IsMrdPenetrating",atrack->GetIsPenetrating());
	thisTrackAsBoostStore->Set("EnergyLoss",atrack->GetEnergyLoss());
	thisTrackAsBoostStore->Set("EnergyLossError",atrack->GetEnergyLossError());
	thisTrackAsBoostStore->Set("IsMrdStopped",atrack->GetIsStopped());
	thisTrackAsBoostStore->Set("IsMrdSideExit",atrack->GetIsSideExit());
	thisTrackAsBoostStore->Set("PenetrationDepth",atrack->GetPenetrationDepth() / 100.);
	thisTrackAsBoostStore->Set("HtrackFitChi2",atrack->GetHtrackFitChi2());
	thisTrackAsBoostStore->Set("HtrackFitCov",atrack->GetHtrackFitCov());
	thisTrackAsBoostStore->Set("VtrackFitChi2",atrack->GetVtrackFitChi2());
	thisTrackAsBoostStore->Set("VtrackFitCov",atrack->GetVtrackFitCov());
	thisTrackAsBoostStore->Set("PMTsHit",atrack->GetPMTsHit());
	thisTrackAsBoostStore->Set("HtrackOrigin",atrack->GetHtrackOrigin());
	thisTrackAsBoostStore->Set("HtrackOriginError",atrack->GetHtrackOriginError());
	thisTrackAsBoostStore->Set("HtrackGradient",atrack->GetHtrackGradient());
	thisTrackAsBoostStore->Set("HtrackGradientError",atrack->GetHtrackGradientError());
	thisTrackAsBoostStore->Set("VtrackOrigin",atrack->GetVtrackOrigin());
	thisTrackAsBoostStore->Set("VtrackOriginError",atrack->GetVtrackOriginError());
	thisTrackAsBoostStore->Set("VtrackGradient",atrack->GetVtrackGradient());
	thisTrackAsBoostStore->Set("VtrackGradientError",atrack->GetVtrackGradientError());
	// convert back projections to Position class
	Position TankExitPoint( atrack->GetTankExitPoint().X() / 100.,
							atrack->GetTankExitPoint().Y() / 100.,
							atrack->GetTankExitPoint().Z() / 100.);
	Position MrdEntryPoint( atrack->GetMrdEntryPoint().X() / 100.,
							atrack->GetMrdEntryPoint().Y() / 100.,
							atrack->GetMrdEntryPoint().Z() / 100.);
	thisTrackAsBoostStore->Set("TankExitPoint",TankExitPoint);
	thisTrackAsBoostStore->Set("MrdEntryPoint",MrdEntryPoint);
	
	// finally update the pointer to the vector of tracks in the MRDTracks store
	m_data->Stores.at("MRDTracks")->Set("MRDTracks",theMrdTracks,false);
	
	return thisTrackAsBoostStore;
}
