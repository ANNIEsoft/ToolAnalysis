/* vim:set noexpandtab tabstop=4 wrap */
#include "TrackCombiner.h"
#include "MRDTrackClass.hh"
#include "MRDSubEventClass.hh"
#include "StubCluster.h"
#include "MrdStub.h"
#include "TClonesArray.h"

TrackCombiner::TrackCombiner():Tool(){}


bool TrackCombiner::Initialise(std::string configfile, DataModel &data){
	
	/////////////////// Useful header ///////////////////////
	if(configfile!="") m_variables.Initialise(configfile); //loading config file
	//m_variables.Print();
	
	m_data= &data; //assigning transient data pointer
	/////////////////////////////////////////////////////////////////
	
	// these are the number of layers in each view.
	// if other requirements are enough, we could even consider Minimum_H_Layers=1, Minimum_V_Layers=1
	m_variables.Get("Minimum_V_Layers",min_layers_v);
	m_variables.Get("Minimum_H_Layers",min_layers_h);
	m_variables.Get("Max_Matching_Tdiff",max_allowed_tdiff);
	
	get_ok = m_data->Stores.at("ANNIEEvent")->Header->Get("AnnieGeometry", anniegeom);
	if(not get_ok){
		Log("TrackCombiner Tool: Failed to retrieve AnnieGeometry from ANNIEEvent Store!",v_error,verbosity);
		return false;
	}
	
	intptr_t subevptr;
	get_ok = m_data->CStore.Get("MrdSubEventTClonesArray",subevptr);
	if(not get_ok){
		cerr<<"Failed to get MrdSubEventTClonesArray from CStore!!"<<endl;
		return false;
	}
	thesubeventarray = reinterpret_cast<TClonesArray*>(subevptr);
	
	// actually a config file variable passed to FindMrdTracks
	get_ok = m_data->CStore.Get("DrawMrdTruthTracks",DrawMrdTruthTracks);
	
	return true;
}


bool TrackCombiner::Execute(){
	
	// Get the reconstructed event vertex from the RecoEvent store
	// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	if(m_data->Stores.count("RecoEvent")==0){
		Log("TrackCombiner Tool: No RecoEvent booststore! Need to run vertex reconstruction tools first!",v_error,verbosity);
		return false;
	}
	theExtendedVertex=nullptr;
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
		Log("TrackCombiner Tool: Matched MRD track "+to_string(best_match_index)
			+" to the RecoVertex!",v_debug,verbosity);
		BoostStore* primaryEventMrdTrack = &(theMrdTracks->at(best_match_index));
		m_data->Stores.at("ANNIEEvent")->Set("PrimaryEventRecoMrdTrack",primaryEventMrdTrack);
	} else {
		Log("TrackCombiner Tool: Failed to match any MRD tracks to the RecoVertex, "
		    "looking for matching TDC hits",v_debug,verbosity);
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
	
	Log("TrackCombiner Tool: Scanning "+to_string(TDCData->size())
		  +" TDC channels for hits near the recoVtx time",v_debug,verbosity);
	std::map<unsigned long,std::vector<double>> vetohits;   // map of PMT channelkey to times of the hits on it
	std::map<unsigned long,std::vector<double>> mrdhits;    // as above
	for(std::pair<const unsigned long,vector<MCHit>>& anmrdpmt : (*TDCData)){
		// retrieve the pmt information
		unsigned long chankey = anmrdpmt.first;
		Detector* thedetector = anniegeom->ChannelToDetector(chankey);
		bool thisismrdpmt= (thedetector->GetDetectorElement()=="MRD");  // else vetopmt
		
		// loop over hits on this pmt
		for(auto&& hitsonthismrdpmt : anmrdpmt.second){
			double thishittime = hitsonthismrdpmt.GetTime();
			if(abs(thishittime-recoEventStartTime)<max_allowed_tdiff){
				// this hit was about the time of the reconstructed event time
				// add it to the appropriate map
				std::map<unsigned long,std::vector<double>>* themap = (thisismrdpmt) ? &mrdhits : &vetohits;
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
	
	// process prompt veto digits; e.g. try to identify an in-time veto event
	// by looking for in-time hits on two adjacent paddles in the two veto layers.
	// Note that as our paddle efficiency is expected to be low, we may not wish
	// to apply this as an event cut. Nonetheless, it's worth doing.
	// time cut is already applied: look for adjacency by comparing paddle number in Y
	std::vector<int> first_layer_paddles_hit;
	std::vector<int> second_layer_paddles_hit;
	for(std::pair<const unsigned long,vector<double>>& avetopmt : vetohits){
		unsigned long chankey = avetopmt.first;
		Detector* thedetector = anniegeom->ChannelToDetector(chankey);
		Paddle* thepaddle = anniegeom->GetDetectorPaddle(thedetector->GetDetectorID());
		if(thepaddle->GetLayer()==0){
			first_layer_paddles_hit.push_back(thepaddle->GetPaddleY());
		} else {
			second_layer_paddles_hit.push_back(thepaddle->GetPaddleY());
		}
	}
	// now we have the hit paddles (0-13) in layers 0 and one, search for matches
	bool matchedvetopair=false;
	for(int& layer_one_paddleY : first_layer_paddles_hit){
		for(int & layer_two_paddleY : second_layer_paddles_hit){
			if(layer_one_paddleY==layer_two_paddleY){
				matchedvetopair=true;
				break;
			}
		}
	}
	// pass this to the ANNIEEvent
	m_data->Stores.at("ANNIEEvent")->Set("InTimeDoubleVetoEvent",matchedvetopair);
	// as a fallback we can just check if we had any veto activity by checking the size
	// of in-time veto hits
	m_data->Stores.at("ANNIEEvent")->Set("InTimeVetoHits",vetohits);
	
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
	// we could try the deepest hit with a hit in every preceding layer...
	
	// going with the idea that we'll require a hit in all consecutive layers,
	// let's ensure the hits are consistent by requiring adjacency of hit paddles
	// in adjacent layers.
	
	// first we need to sort hits into layers
	std::map<int,std::vector<Paddle*>> hits_on_layers;
	for(std::pair<const unsigned long,vector<double>>& anmrdpmt : mrdhits){
		unsigned long chankey = anmrdpmt.first;
		Detector* thedetector = anniegeom->ChannelToDetector(chankey);
		Paddle* thepaddle = anniegeom->GetDetectorPaddle(thedetector->GetDetectorID());
		int layer = thepaddle->GetPaddleZ();
		if(hits_on_layers.count(layer)){
			hits_on_layers.at(layer).push_back(thepaddle);
		} else {
			hits_on_layers.emplace(layer,std::vector<Paddle*>{thepaddle});
		}
	}
	
	// before we go any further; what if we had two adjacent paddles in the same layer with a hit?
	// we could consider them independently, but that could result in multiple degenerate
	// reconstructed tracks. Instead, we'll merge adjacent hit paddles into a 'cluster'
	// (this is the same as the first step of full MRD track reconstruction)
	std::map<int,std::vector<StubCluster>> clusters_on_layers;
	// loop over layers
	for(int layeri=2; ; layeri++){  // start from layer 2 because that's the first MRD layer
		// stop when we find a layer with no hits; we won't consider skipped layers
		if(hits_on_layers.count(layeri)==0){ break; }
		// get the hit paddles in this layer
		std::vector<Paddle*> hit_paddles_this_layer = hits_on_layers.at(layeri);
		
		StubCluster* currentcluster = nullptr;
		// loop until we break
		do{
			bool added_paddle=false;
			// scan paddles in this layer and see if we can merge them with the
			// current cluster (if null, we'll create one from the first paddle)
			for(int paddlei=0; paddlei<hit_paddles_this_layer.size(); ++paddlei){
				// get next hit paddle in this layer
				Paddle* apaddle = hit_paddles_this_layer.at(paddlei);
				// check if it's already been allocated to a cluster
				if(apaddle==nullptr) continue;
				// else this paddle isn't yet in a cluster.
				// Next see if we're creating a new cluster, or growing an existing one
				if(currentcluster==nullptr){
					// we're starting a new one; make a cluster from this paddle
					if(clusters_on_layers.count(layeri)==0){
						clusters_on_layers.emplace(layeri,std::vector<StubCluster>{StubCluster(apaddle)});
					} else {
						clusters_on_layers.at(layeri).push_back(StubCluster(apaddle));
					}
					currentcluster = &(clusters_on_layers.at(layeri).back());
					// mark this paddle as in a cluster
					hit_paddles_this_layer.at(paddlei)=nullptr;
					// mark that we took some action this loop
					added_paddle=true;
				} else {
					// we have an active cluster; see if we can add this paddle to it
					bool added = currentcluster->Merge(apaddle);
					// this checks if the requested paddle is adjacent to our cluster
					// and merges it with it if so; otherwise it does nothing.
					// Returns whether the paddle was added or not.
					if(added){
						// mark the paddle as allocated
						hit_paddles_this_layer.at(paddlei)=nullptr;
						// mark that we took some action this loop
						added_paddle=true;
					} // else this paddle isn't adjacent to the current ends of our cluster
				}
			}
			// ok, we've scanned over all paddles
			// did perform any actions? If we did, run it again with the current cluster,
			// and see if we pick up more paddles. Keep doing this until it doesn't.
			// Once it doesn't, we're done growing that cluster, so mark our currentcluster
			// as null, triggering a new one to be made at the start of the next scan.
			// Eventually we'll run a loop over all paddles with a null current cluster,
			// and it'll take no action. This is our indicator that all paddles are allocated
			// and we need to move to the next layer.
			if(added_paddle==false){
				// we're done with this cluster
				if(currentcluster==nullptr){
					// we're done with the layer entirely
					break;
				} else {
					currentcluster=nullptr;
				}
			} // else this scan added more paddles to this cluster;
			  // run it again, see if we pick up more paddles
		} while(true);
	}
	// OK, done converting paddles to clusters. Back to finding tracks...
	
	// We'll work in the two views independently for now and have to merge later.
	// For the first layer (in each view) we'll make a track for every cluster.
	// For all other layers, scan through the clusters in that layer, and see if they're
	// consistent with any tracks. If so, add the cluster to the track.
	std::vector<MrdStub> mrd_stubs_h, mrd_stubs_v;
	for(int orientationi=0; orientationi<2; orientationi++){
		std::vector<MrdStub>* mrd_stubs = (orientationi) ? &mrd_stubs_v : &mrd_stubs_h;
		// scan from layer 2+orientationi again as MRD layers number from 2
		for(int layeri=(2+orientationi); layeri<clusters_on_layers.size(); layeri+=2){
			// for first layer, make a track for every cluster
			if(layeri==(2+orientationi)){
				for(StubCluster& acluster : clusters_on_layers.at(layeri)){
					mrd_stubs->push_back(MrdStub(acluster));
				}
			} else {
				// for every other layer, scan through the clusters in this layer,
				// and see if it matches any tracks
				for(StubCluster& acluster : clusters_on_layers.at(layeri)){
					for(MrdStub& astub : (*mrd_stubs)){
						// check if this track's last layer is one before this one,
						// and if the forward projection at it's present trajectory
						// is consistent with this cluster's position. 
						// MrdStub::AddCluster performs these checks and adds the cluster
						// if it passes, otherwise does nothing.
						bool added_cluster = astub.AddCluster(acluster);
						// for the moment a cluster can be shared by multiple tracks, don't break
					}
				}
			}
		}
	}
	// ok, we've built our stubby tracks
	// at the time we didn't have enough information to know which stub a given cluster
	// should be added to, so we allowed the same cluster to be added to multiple stubs.
	// But, we don't really want multiple stubs being built from the same clusters.
	// Let's scan over the stubs and any time we find multiple stubs sharing a cluster,
	// we'll choose just one.
	
	// first, define a function to check if two MrdStubs share any clusters.
	std::vector<MrdStub>::iterator stub_a;
	auto comparitor = [&stub_a](MrdStub& stub_b) {
		bool overlap=false;
		std::vector<StubCluster>* clusters_a = stub_a->GetClusters();
		std::vector<StubCluster>* clusters_b = stub_b.GetClusters();
		for(StubCluster& clustera : *clusters_a){
			auto foundit = std::find(clusters_b->begin(), clusters_b->end(), clustera);
			if(foundit!=clusters_b->end()){
				overlap=true;
				break;
			}
		}
		return overlap;
	};
	// now we're ready to scan : check H Stubs first, then V stubs
	for(int orientationi=0; orientationi<2; ++orientationi){
		// get the relevant stubs
		std::vector<MrdStub>* mrd_stubs = (orientationi) ? &mrd_stubs_v : &mrd_stubs_h;
		stub_a = mrd_stubs->begin();
		std::vector<MrdStub>::iterator newend = mrd_stubs->end();
		int offset=0;
		// loop over them, removing any with overlaps as we go
		do{
			// search for duplicates of *stub_a starting from offset going until newend; return first found
			std::vector<MrdStub>::iterator doppleganger = std::find_if(stub_a+offset, newend, comparitor);
			if(doppleganger!=newend){
				// we found a pair that share... KILL one of them.
				if(stub_a->GetEndLayer()<doppleganger->GetEndLayer()){
					// stub_a is actually the shorter; "remove" it
					*stub_a = std::move(*--newend);
				} else {
					// doppleganger is shorter, "remove" it
					*doppleganger = std::move(*--newend);
					// so we can pick up where we left off
					offset=std::distance(mrd_stubs->begin(),doppleganger);
				}
			} else {
				++stub_a;
				offset=0;
			}
		} while (stub_a!=newend);
		// by now we should have all our undesirables at the end of mrd_stubs.
		// use vector::erase to destroy them and update the container bounds.
		mrd_stubs->erase(newend,mrd_stubs->end());
	}
	
	// possibly apply a cut on minimum number of layers in each view?
	// It may be that requiring stubs be in-time and consistent with forward-projection
	// of a tank event is strict enough that we can accept even a single hit paddle...
	auto newend = std::remove_if(mrd_stubs_v.begin(), mrd_stubs_v.end(),
		[this](MrdStub& astub){ return (astub.GetEndLayer()>=min_layers_v); } );
	mrd_stubs_v.erase(newend, mrd_stubs_v.end());
	newend = std::remove_if(mrd_stubs_h.begin(), mrd_stubs_h.end(),
		[this](MrdStub& astub){ return (astub.GetEndLayer()>=min_layers_h); } );
	mrd_stubs_h.erase(newend, mrd_stubs_h.end());
	
	// other cuts?
	// maybe a cut on the goodness of fit of a straight line to all the clusters in this stub?
	// ... nah
	
	// Next we could try to do matching of v and h stubs, but for such short stubs
	// we're unlikely to have enough information to do that well.
	// instead, we'll just try to match a tank vertex to an h and v track independently
	
	// to match a stub with a tank event we'll require consistency between the
	// forward-projected MRD entry point and the stub start, so find that forward-projection
	Position recoVertex_vtx = theExtendedVertex->GetPosition();
	Direction recoVertex_dir = theExtendedVertex->GetDirection();
	double distance_to_MRD = anniegeom->GetMrdStart() - recoVertex_vtx.Z();
	Position ProjectedMrdEntry(recoVertex_vtx.X() + (recoVertex_dir.X()/recoVertex_dir.Z())*(distance_to_MRD),
	                           recoVertex_vtx.Y() + (recoVertex_dir.Y()/recoVertex_dir.Z())*(distance_to_MRD),
	                           anniegeom->GetMrdStart());
	
	// scan over the stubs found and look for one that matches our primary event
	// note that only one stub can be consistent with the entry point, since we do not
	// allow multiple stubs to share clusters (i.e., to start from the same paddle)
	int primary_v_stub_index=-1;
	int primary_h_stub_index=-1;
	for(int orientationi=0; orientationi<2; ++orientationi){
		std::vector<MrdStub>* mrd_stubs = (orientationi) ? &mrd_stubs_v : &mrd_stubs_h;
		for(int stubi=0; stubi<mrd_stubs->size(); ++stubi){
			MrdStub& astub = mrd_stubs->at(stubi);
			StubCluster& firstcluster = astub.GetClusters()->front();
			std::pair<double,double> extents = firstcluster.GetExtents();
			if(orientationi){
				if( (extents.first<ProjectedMrdEntry.X()) && (ProjectedMrdEntry.X()<extents.second) ){
					primary_v_stub_index = stubi;
					break;
				}
			} else {
				if( (extents.first<ProjectedMrdEntry.Y()) && (ProjectedMrdEntry.Y()<extents.second) ){
					primary_h_stub_index = stubi;
					break;
				}
			}
		}
	}
	
	
	if((primary_h_stub_index<0)||(primary_v_stub_index)<0){
		// found no consistent stub, at least in one view.
		// perhaps we could relax the requirement of a paddle in both views?
		return nullptr;
	}
	// else, we found a stub for the primary event.
	
	// TODO What about the other stubs we (may have) found? ... for now, do nothing.
	// If in the future we want to use them, then this tool will need to at least pass them out
	
	//////////////////////////////////////////////////////////////////////
	// CONVERSION AND ADDING
	//////////////////////////////////////////////////////////////////////
	// The MrdTrackLib class provides various functions that we would like to take advantage of;
	// primarily track drawing, but also energy conversion from penetration, best-straight-line-fit
	// and errors on fit angles, penetration checks, back projection to tank etc.
	// In order to not duplicate those methods here, we need to convert our MrdStub into a cMRDTrack.
	//
	// Not only that, but the PaddlePlot tool draws dracks by invoking the Draw methods of each
	// cMRDSubEvent in the TClonesArray generated by MrdTrackLib for this Event. 
	// If we want tracks found here to be drawn too, they need to be in a cMRDSubEvent 
	// in that TClonesArray (probably added to the first one, since this is the primary event track).
	
	// Get the stubs
	MrdStub& hstub = mrd_stubs_h.at(primary_h_stub_index);
	MrdStub& vstub = mrd_stubs_v.at(primary_v_stub_index);
	
	// pull other information from ANNIEEvent. We probably don't need to do this,
	// but it should probably be correct just in case a tool tries to use it
	std::string wcsimfile;
	get_ok = m_data->Stores.at("ANNIEEvent")->Get("MCFile",wcsimfile);
	int run_id;
	get_ok = m_data->Stores.at("ANNIEEvent")->Get("RunNumber",run_id);
	int event_id;
	get_ok = m_data->Stores.at("ANNIEEvent")->Get("EventNumber",event_id);
	int trigger;
	get_ok = m_data->Stores.at("ANNIEEvent")->Get("MCTriggernum",trigger);
	std::map<unsigned long,int> channelkey_to_mrdpmtid;
	get_ok = m_data->Stores.at("ANNIEEvent")->Get("channelkey_to_mrdpmtid",channelkey_to_mrdpmtid);
	
	// make the other constructor members of cMRDTrack. Most of them are actually not used.
	double earliest_hit_time=999999999;
	std::vector<int> digitindexes_inthistrack;         // unused, not interesting
	std::vector<int> digitnumphots_inthistrack;        // unused, not interesting
	std::vector<double> digittruetimes_inthistrack;    // unused, not interesting
	std::vector<int> digittrueparents_inthistrack;     // unused, we implement it another way
	std::vector<double> digitqs_inthistrack;           // could be used for paddleplot colours, needs propagating
	
	// we do need to fill these for track drawing
	std::vector<double> digittimes_inthistrack;        // currently used for paddleplot colours
	std::vector<int> tubeids_inthistrack;              // used for truth track drawing in MrdPaddlePlot
	
	// and we need to convert our clusters and stubs to clusters and cells
	std::vector<mrdcell> thehtrackcells;
	thehtrackcells.reserve(hstub.GetClusters()->size()-1);
	std::vector<mrdcell> thevtrackcells;
	thevtrackcells.reserve(vstub.GetClusters()->size()-1);
	std::vector<mrdcluster> htrackclusters;
	htrackclusters.reserve(hstub.GetClusters()->size());
	std::vector<mrdcluster> vtrackclusters;
	vtrackclusters.reserve(vstub.GetClusters()->size());
	
	// calculate any remaining properties: MrdTrack needs a time, so find the earliest hit
	for(int orientationi=0; orientationi<2; ++orientationi){
		MrdStub* astub = (orientationi) ? &vstub : &hstub;
		std::vector<mrdcluster>* theclusters = (orientationi) ? &vtrackclusters : &htrackclusters;
		std::vector<mrdcell>* thecells = (orientationi) ? &thevtrackcells : &thehtrackcells;
		mrdcluster* lastcluster=nullptr;
		for(StubCluster& acluster : *(astub->GetClusters())){
			
			// convert the StubCluster to an mrdcluster
			for(Paddle* apaddle : acluster.GetPaddles()){
				int wcsimtubeid = channelkey_to_mrdpmtid.at(apaddle->GetDetectorID());
				tubeids_inthistrack.push_back(wcsimtubeid);
				std::vector<double>& hittimes = mrdhits.at(apaddle->GetDetectorID());
				digittimes_inthistrack.reserve(digittimes_inthistrack.size()+hittimes.size());
				digittimes_inthistrack.insert(digittimes_inthistrack.end(),hittimes.begin(),hittimes.end());
				double amintime = *std::min_element(hittimes.begin(),hittimes.end());
				if(amintime<earliest_hit_time) earliest_hit_time = amintime;
				
				if(theclusters->size()==0){
					theclusters->emplace_back(0, wcsimtubeid, apaddle->GetLayer(), earliest_hit_time);
				} else {
					theclusters->back().AddDigit(0, wcsimtubeid, earliest_hit_time);
				}
			}
			
			// if this isn't the first cluster, make a cell between it and the previous one
			int nclusters = theclusters->size();
			if(nclusters>0){
				thecells->emplace_back(&theclusters->at(nclusters-2),&theclusters->back());
			}
		}
	}
	
	// if we need to build a cMRDSubEvent, and we want to draw truth straight lines on the PaddlePlots,
	// we'll need to pass the truth track vertices and pdgs(?) to the cMRDSubEvent. (probably obsolete)
	// Taken from FindMrdTracks:
	std::vector<std::pair<TVector3,TVector3>> truetrackvertices;
	std::vector<Int_t> truetrackpdgs;
	if((numsubevs==0)  && (DrawMrdTruthTracks)){
		// pull truth tracks to overlay on reconstructed tracks
		get_ok = m_data->Stores["ANNIEEvent"]->Get("MCParticles",MCParticles);
		if(not get_ok){
			Log("TrackCombiner Tool: Failed to get MCParticles from ANNIEEvent!",v_error,verbosity);
		} else {
			int numtracks = MCParticles->size();
			for(int truetracki=0; truetracki<numtracks; truetracki++){
				MCParticle nextrack = MCParticles->at(truetracki);
				if(true&&nextrack.GetPdgCode()!=11){
					Position startvp = nextrack.GetStartVertex();
					TVector3 startv(startvp.X()*100.,startvp.Y()*100.,startvp.Z()*100.);
					Position stopvp = nextrack.GetStopVertex();
					TVector3 stopv(stopvp.X()*100.,stopvp.Y()*100.,stopvp.Z()*100.);
					truetrackvertices.push_back({startv,stopv});
					truetrackpdgs.push_back(nextrack.GetPdgCode());
				}
			}
		}
	}
	
	/////////////////////////////////////////////////////////////////////
	// BUILD THE CMRDSUBEVENT AND CMRDTRACKS
	/////////////////////////////////////////////////////////////////////
	// we should have all the information we need to build the cMRDSubEvent and cMRDTrack now
	
	// first check if MrdTrackLib found any cMRDSubEvents this event:
	cMRDSubEvent* thesubevent=nullptr;
	if(numsubevs==0){
		// No: then we'll need to make a cMRDSubEvent to put this cMRDTrack in  :(
		++numsubevs;
		m_data->Stores.at("MRDTracks")->Set("NumMrdSubEvents",numsubevs);
		
		thesubevent = new((*thesubeventarray)[0]) cMRDSubEvent(0, wcsimfile, run_id, event_id, trigger, digitindexes_inthistrack, tubeids_inthistrack, digitqs_inthistrack, digittimes_inthistrack, digitnumphots_inthistrack, digittruetimes_inthistrack, digittrueparents_inthistrack, truetrackvertices, truetrackpdgs);
	} else {
		// get the first MrdSubEvent
		thesubevent = (cMRDSubEvent*)thesubeventarray->At(0);
	}
	
	// Add the cMRDTrack to the cMRDSubEvent (in-place construction)
	thesubevent->GetTracks()->emplace_back(numtracksinev, wcsimfile, run_id, event_id, 0, trigger, digitindexes_inthistrack, tubeids_inthistrack, digitqs_inthistrack, digittimes_inthistrack, digitnumphots_inthistrack, digittruetimes_inthistrack, digittrueparents_inthistrack, thehtrackcells, thevtrackcells, htrackclusters, vtrackclusters);
	cMRDTrack* atrack = &thesubevent->GetTracks()->back();
	
	///////////////////////////////////////////////////////
	// ADD TO THE MRDTRACKS BOOSTSTORE
	///////////////////////////////////////////////////////
	// so that other tools don't have to handle TClonesArrays, we also convert the
	// TClonesArray of cMRDSubEvents containing cMRDTracks to a std::vector<BoostStore>
	// We need to also add our new tracks to this.
	
	// increment the track count to accommodate the new track
	numtracksinev++;
	
	// checkif we need to grow the MrdTracks container
	if(numtracksinev>theMrdTracks->size()){
		Log("TrackCombiner Tool: Growing theMrdTracks vector to size "
			+to_string(numtracksinev),v_debug,verbosity);
		theMrdTracks->resize(numtracksinev);
	}
	// Get a pointer to the track entry
	BoostStore* thisTrackAsBoostStore = &(theMrdTracks->at(numtracksinev-1));
	
	// now set the new track properties from the cMRDTrack
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
	thisTrackAsBoostStore->Set("TrackAngleError",atrack->GetTrackAngleError()); // TODO!!!
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
	
	// finally update the count and vector of tracks in the MRDTracks store
	m_data->Stores.at("MRDTracks")->Set("NumMrdTracks",numtracksinev);
	m_data->Stores.at("MRDTracks")->Set("MRDTracks",theMrdTracks,false);
	
	return thisTrackAsBoostStore;
}

