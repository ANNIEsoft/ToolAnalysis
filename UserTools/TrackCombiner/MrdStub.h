/* vim:set noexpandtab tabstop=4 wrap */
#ifndef MRDSTUBCLASS_H
#define MRDSTUBCLASS_H

#include <iostream>

#include<SerialisableObject.h>
#include "StubCluster.h"

class MrdStub : public SerialisableObject{
	
	friend class boost::serialization::access;
	
	public:
	MrdStub(){serialise=true;};
	MrdStub(StubCluster& acluster);
	
	// Getters and Setters
	int GetEndLayer(){ return current_layer; }
	std::vector<StubCluster>* GetClusters(){ return &clusters; }
	double GetDxDz(){ return current_dxdz; }
	
	// Member functions for reconstruction
	bool AddCluster(StubCluster& acluster); // add a cluster to this stub, if it's consistent
	bool Init(StubCluster& acluster);       // initalize stub from it's first cluster
	
	// Generic member functions
	bool Print(){
		std::cout<<"Mrd Stub from "<<((orientation) ? "V" : "H")
				 <<" layers, starting from the front of the MRD and going to layer z="
				 <<current_layer<<". Current in-layer position is "<<current_pos<<std::endl;
		return true;
	}
	
	// member variables
	private:
	std::vector<StubCluster> clusters;
	int orientation;     // H is 0, V is 1
	int current_layer;   // MRD z number
	double current_pos;  // x or y position depending on orientation, meters
	double current_z;    // z position, meters
	double current_dxdz; // for projecting to next layer to check consistency
	// for projecting to next layer to check consistency, when we already made a match
	// but want to evaluate a potential better one in the same layer
	double last_pos;
	double last_z;
	double last_dxdz;
	
	template<class Archive> void serialize(Archive & ar, const unsigned int version){
		if(serialise){
			ar & orientation;
			ar & current_layer;
			ar & current_pos;
			ar & current_z;
			ar & current_dxdz;
			ar & clusters;
		}
	}
	
};
#endif
