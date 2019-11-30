/* vim:set noexpandtab tabstop=4 wrap */
#include "MrdStub.h"

MrdStub::MrdStub(StubCluster& acluster) : current_dxdz(0) {
	// construct a stub from it's first cluster
	serialise=true;
	Init(acluster);
}

bool MrdStub::AddCluster(StubCluster& acluster){
	// check if the candidate cluster is consistent with the stub's
	// current trajectory, and add it if so. return true if it was added.
	
	// if this is the first cluster, we make no checks
	if(clusters.size()==0){
		return Init(acluster);
	}
	
	// check layer consistency; member cluster layer numbers go up in 2's, but
	// we also want to reconsider later clusters in the current layer for a better fit
	
	// first consider the case where this is the first candidate cluster in the next layer
	if(acluster.GetLayer()==(current_layer+2)){
//		std::cout<<"first proposed cluster in this layer"<<std::endl;
		
		// check if the forward projection is within the accepted span
		double projected_pos = (current_pos + current_dxdz*(acluster.GetOrigin().Z()-current_z));
		std::pair<double,double> candidate_span = acluster.GetExtentsPlusTwo();
		
//		double proposed_centre = (orientation) ? acluster.GetOrigin().X() : acluster.GetOrigin().Y();
//		std::cout<<"dxdz = "<<current_dxdz<<", current_x="<<current_pos<<", current_z="<<current_z
//				 <<", new cluster z ="<<acluster.GetOrigin().Z()
//				 <<", z disance to new cluster is "<<(acluster.GetOrigin().Z()-current_z)
//				 <<", projected_pos="<<projected_pos
//				 <<"; new cluster has centre ("<<acluster.GetOrigin().X()<<", "<<acluster.GetOrigin().Y()
//				 <<") of which we would take "<<proposed_centre<<std::endl;
//		std::cout<<"canidate span = "<<candidate_span.first<<" -> "<<candidate_span.second<<std::endl;
		
		if( (candidate_span.first<projected_pos) && (projected_pos<candidate_span.second) ){
//			std::cout<<"successful match"<<std::endl;
			// candidate success! update our properties
			
			// make a note of our properties from the last layer, in case we need to
			// evaluate another cluster's fit with the projection to this layer
			last_pos = current_pos;
			last_dxdz = current_dxdz;
			last_z = current_z;
			
			// update our current parameters
			current_layer = acluster.GetLayer();
			current_z = acluster.GetOrigin().Z();
			current_pos = (orientation) ? acluster.GetOrigin().X() : acluster.GetOrigin().Y();
			current_dxdz = (current_pos-last_pos)/(current_z-last_z);
			
			clusters.push_back(acluster);
			return true;
		}
//		else std::cout<<"projected point is outside candidate span"<<std::endl;
	// next consider the case where we have a fitting candidate in this layer,
	// but we want to check if this alternative candidate is a better fit
	} else if(acluster.GetLayer()==current_layer){
//		std::cout<<"we have an existing cluster in this layer"<<std::endl;
		// check if forward projection is within the accepted span
		double projected_pos = (last_pos + last_dxdz*(acluster.GetOrigin().Z()-last_z));
		std::pair<double,double> candidate_span = acluster.GetExtentsPlusTwo();
		if( (candidate_span.first<projected_pos) && (projected_pos<candidate_span.second) ){
			// candidate success! but now we have a second check: is this a better fit
			// than our current match? Use whichever is closer to our projected point
			double new_candidate_centre = 0.5*(acluster.GetExtents().first+acluster.GetExtents().second);
			StubCluster old_candidate = clusters.back();
			double old_candidate_centre = 0.5*(old_candidate.GetExtents().first+old_candidate.GetExtents().second);
			double new_distance_from_projection = abs(projected_pos-new_candidate_centre);
			double old_distance_from_projection = abs(projected_pos-old_candidate_centre);
			
			if(new_distance_from_projection<old_distance_from_projection){
				// this one's a better match. Update our current parameters.
				current_pos = new_candidate_centre;
				current_dxdz = (current_pos-last_pos)/(current_z-last_z);
				
				// replace the last cluster
				clusters.pop_back();
				clusters.push_back(acluster);
				return true;
			}
		}
	}
	// else layer is incompatible
	return false;
};

bool MrdStub::Init(StubCluster& acluster){
	// first a sanity check
	if(acluster.GetLayer()>(acluster.GetOrientation()+2)){  // layer is z; for MRD it counts from 2.
		std::cerr<<"MrdStub Init called with a cluster in layer>0!"<<std::endl
		         <<"Stub will not be Initialised!"<<std::endl;
		return false;
	}
	
	// set starting properties from first cluster
	current_layer = acluster.GetLayer();
	orientation = acluster.GetOrientation();
	current_pos = (orientation) ? acluster.GetOrigin().X() : acluster.GetOrigin().Y();
	current_z   = acluster.GetOrigin().Z();
//	std::cout<<"constructing stub from first cluster in layaer "<<current_layer
//			 <<" which has orientation "<<orientation<<", with centre ("
//			 <<acluster.GetOrigin().X()<<", "<<acluster.GetOrigin().Y()<<") of which we are taking "
//			 <<current_pos<<" and current zpos "<<current_z<<std::endl;
	
	clusters.push_back(acluster);
	return true;
}
