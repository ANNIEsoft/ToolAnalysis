/* vim:set noexpandtab tabstop=4 wrap */
#include "StubCluster.h"

StubCluster::StubCluster(Paddle* apaddle){
	// construct a cluster from it's first paddle
	serialise=true;
	layer = apaddle->GetLayer();
	orientation = apaddle->GetOrientation();
	side = apaddle->GetSide();
	origin = apaddle->GetOrigin();
	extents = (orientation) ? apaddle->GetExtentsX() : apaddle->GetExtentsY();
	
	// all we need to know to check if we can merge a new paddle 
	// is the in-layer paddle numbers of the our current end paddles
	paddle_number_min = (orientation) ? apaddle->GetPaddleX() : apaddle->GetPaddleY();
	paddle_number_max = paddle_number_min;
	
	// keep the paddles, just in case.
	paddles.push_back(apaddle);
}

bool StubCluster::Merge(Paddle* apaddle){
	// this checks if the requested paddle is adjacent to our cluster and merges it if so
	// Returns whether the paddle was added or not.
	
	// first, should always be true but check we're in the same layer
	if(apaddle->GetLayer()!=layer){
		std::cerr<<"StubCluster Error! Tried to Merge two Clusters in different z layers!"<<std::endl;
		return false;
	}
	// we'll also only merge paddles on the same side
	if(apaddle->GetSide()!=side){
		return false;
	}
	
	// next see if the in-layer number of this paddle is off-by-one from either of our end paddle numbers
	int in_layer_number = (orientation) ? apaddle->GetPaddleX() : apaddle->GetPaddleY();
	if( (in_layer_number==(paddle_number_max+1)) || (in_layer_number==(paddle_number_min-1)) ){
		// this paddle is next to one of our ends! Merge!
		paddles.push_back(apaddle); // just in case. We don't really need them.
		
		// update the paddle numbers of our extremeties
		if(in_layer_number==(paddle_number_max+1)){
			paddle_number_max=in_layer_number;
		} else {
			paddle_number_min=in_layer_number;
		}
		
		// update our physical span
		std::pair<double,double> theextents =
			 (orientation) ?  apaddle->GetExtentsX() : apaddle->GetExtentsY();
		bool lower_extend = (theextents.first  < extents.first);
		bool upper_extend = (theextents.second > extents.second);
		if(lower_extend&&upper_extend){
			std::cerr<<"StubCluster Error! "
					 <<" Merge with paddle that has a greater span than our cluster?!"
					 <<std::endl;
			return false;
		}
		if(lower_extend){
			extents.first = theextents.first;
		} else {
			extents.second = theextents.second;
		}
		
		// update our origin
		if(orientation){
			// vertical paddle; update our x origin
			origin.SetX((extents.second-extents.first)/2.);
		} else {
			origin.SetY((extents.second-extents.first)/2.);
		}
		
		return true;
		
	} else {
		
		// not adjacent to our current span.
		return false;
		
	}
}
