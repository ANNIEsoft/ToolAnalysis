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
double Detector::GetR(){
	if((DetectorElement!="Tank")||(GeometryPtr==nullptr)) return 0;  // N/A or can't determine
	Position tankoriginpos = GetPositionInTank();
	return sqrt(pow(tankoriginpos.X(),2.)+pow(tankoriginpos.Z(),2.));
}
