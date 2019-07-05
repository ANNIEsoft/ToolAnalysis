/* vim:set noexpandtab tabstop=4 wrap */
#ifndef STUBCLUSTERCLASS_H
#define STUBCLUSTERCLASS_H

#include <iostream>

#include<SerialisableObject.h>

class StubCluster : public SerialisableObject{
	
	friend class boost::serialization::access;
	
	public:
	StubCluster() : {serialise=true;};
	StubCluster(Paddle* apaddle);
	
	// Getters and Setters
	int GetLayer(){ return layer; }
	Position GetOrigin(){ return origin; }
	int GetHalf(){ return half; }
	std::string GetOrientationString(){ return ((orientation) ? "V" : "H"); }
	std::vector<Paddle*> GetPaddles(){ return paddles; }
	
	// reconstruction member functions
	bool Merge(Paddle* apaddle);
	
	// generic member functions
	bool Print(){
		std::cout<<"Cluster of "<<(paddle_number_max-paddle_number_min)
				 <<" "<<GetOrientationString()
				 <<" paddles in the ";
		if(orientation){
			if(half){ std::cout<<"top";   } else { std::cout<<"bottom"; }
			if(side){ std::cout<<"right"; } else { std::cout<<"left"; }
		} else {
			if(half){ std::cout<<"right"; } else { std::cout<<"left"; }
			if(side){ std::cout<<"top";   } else { std::cout<<"bottom"; }
		}
		std::cout<<" of layer "<<layer<<"; cluster origin is ";
		origin.Print(false);
		std::cout<<" and extent is {"<<extents.first<<" -> "<<extents.second<<" }"
				 <<std::endl;
		return true;
	}
	
	// member variables
	private:
	int layer;
	int orientation;   // H is 0, V is 1
	int half;          // MRD layers are split into 2; which half (left/right, top/bottom)
	int side;          // within it's half, is it on the (left/bottom, right/top), or dead center?
	Position origin;
	std::pair<double,double> extents;
	int paddle_number_max;
	int paddle_number_min;
	std::vector<Paddle*> paddles;  // transient
	
	template<class Archive> void serialize(Archive & ar, const unsigned int version){
		if(serialise){
			ar & layer;
			ar & orientation;
			ar & side;
			ar & origin;
			ar & extents;
			ar & paddle_number_max;
			ar & paddle_number_min;
		}
	}
	
};
#endif
