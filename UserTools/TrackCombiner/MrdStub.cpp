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
	
	// check layer consistency; in current implementation we have a stub for each view,
	// so member cluster layer numbers go up in 2's
	if(acluster.GetLayer()!=(current_layer+2)){
		return false;
	}
	
	// next check if the forward projection is within the accepted span
	double projected_pos = (pos + current_dxdz*(acluster->GetOrigin().Z()-current_z));
	std::pair<double,double> candidate_span = GetClusterSpanPlusOne(acluster);
	if( (candidate_span.first<projected_pos) && (projected_pos<candidate_span.second) ){
		// candidate success! update our properties
		current_layer = acluster.GetLayer();
		double new_pos = (orientation) ? acluster.GetOrigin().X() : acluster.GetOrigin().Y();
		current_dxdz = (new_pos-current_pos)/(acluster->GetOrigin().Z()-current_z);
		current_pos = new_pos;
		current_z = acluster.GetOrigin().Z();
		
		clusters.push_back(acluster);
		return true;
	}
	
	return false;
};

std::pair<double,double> MrdStub::GetClusterSpanPlusOne(StubCluster& acluster){
	double xmin = acluster.GetExtents.first;
	double xmax = acluster.GetExtents.second;
	double paddle_width = acluster.GetPaddles().begin()->GetPaddleWidth();
	return std::pair<double,double>{xmin-paddle_width,xmax+paddle_width};
}

bool MrdStub::Init(StubCluster& acluster){
	// first a sanity check
	if(acluster.GetLayer()){
		std::cerr<<"MrdStub Init called with a cluster in layer>0!"<<std::endl
		         <<"Stub will not be Initialised!"<<std::endl;
		return false;
	}
	
	// set starting properties from first cluster
	current_layer = acluster.GetLayer();
	orientation = acluster.GetOrientation();
	current_pos = (orientation) ? acluster.GetOrigin().X() : acluster.GetOrigin().Y();
	current_z   = acluster.GetOrigin().Z();
	
	clusters.push_back(acluster);
}
