/* vim:set noexpandtab tabstop=4 wrap */
#include "Detector.h"
#include "Geometry.h"

Position Detector::GetPositionInTank(){
	if((DetectorElement!="Tank")||(GeometryPtr==nullptr)){
		return DetectorPosition;
	} else {
		return (DetectorPosition-GeometryPtr->GetTankCentre());
	}
}
