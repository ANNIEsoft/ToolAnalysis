#include "VtxSeedFineGrid.h"

VtxSeedFineGrid::VtxSeedFineGrid():Tool(){}


bool VtxSeedFineGrid::Initialise(std::string configfile, DataModel &data){

  /////////////////// Useful header ///////////////////////
  if(configfile!="") m_variables.Initialise(configfile); // loading config file
  //m_variables.Print();
  m_variables.Get("verbosity", verbosity);


  m_data= &data; //assigning transient data pointer
  /////////////////////////////////////////////////////////////////
  vSeedVtxList = new std::vector<RecoVertex>;

  return true;
}


bool VtxSeedFineGrid::Execute(){
	m_data->Stores.at("RecoEvent")->Get("vSeedVtxList", vSeedVtxList);
	auto get_recoevent = m_data->Stores.count("RecoEvent");
	auto get_recodigit = m_data->Stores.at("RecoEvent")->Get("RecoDigit",fDigitList);
	if (!vSeedVtxList) {
		Log("VtxSeedFineGrid Tool: Must run after VtxSeedGenerator", v_error, verbosity);
		return false;
	}
	if (!get_recoevent){
		Log("VtxSeedFineGrid Tool: failed to get event", v_error, verbosity);
		return false;
	}
	if(!fDigitList){
		Log("VtxSeedFineGrid Tool: Failed to get event digits; did you run DigitBuilder?", v_error, verbosity);
		return false;
	}
	Log("VtxSeedFineGrid Tool: Executing", v_message, verbosity);
	  // form list of golden digits used in class methods
	  // Here, the digit type (PMT or LAPPD or all) is specified.
	  Log("VtxSeedGenerator Tool: Getting clean RecoDigits for event", v_debug,verbosity);
	  vSeedDigitList.clear();  
	  RecoDigit digit;
	  fThisDigit = 0;
	  for( fThisDigit=0; fThisDigit<fDigitList->size(); fThisDigit++ ){
	  	digit = fDigitList->at(fThisDigit);
	  	if( digit.GetFilterStatus() ){ 
	        	if( digit.GetDigitType() == fSeedType){ 
	        	vSeedDigitList.push_back(fThisDigit);
	  	}
	        else if (fSeedType == 2){
	       //Use all digits available
	        vSeedDigitList.push_back(fThisDigit);
	        }
	  }
     }
	
	Center = this->FindCenter();
	//this->GenerateFineGrid();

  return true;
}


bool VtxSeedFineGrid::Finalise(){
	//delete vSeedVtxList; vSeedVtxList = 0;
	Log("VtxSeedFineGrid exitting", v_debug, verbosity);
  return true;
}

Position VtxSeedFineGrid::FindCenter() {
	double timefom = -999.999 * 100;
	double conefom = -999.999 * 100;
	Double_t fom = -9999;
	Double_t bestfom = -9999;
	double meantime;
	double ConeAngle = Parameters::CherenkovAngle();
	RecoVertex *thisTrialSeed = 0;
	RecoVertex *thisCenterSeed = new RecoVertex;
	Position vtxPos;
	Direction vtxDir;
	Position truePos;
	Direction trueDir;
	FoMCalculator * trialFoM = new FoMCalculator();
	RecoVertex *iSeed = new RecoVertex;
	int nhits;
	auto get_truevtx = m_data->Stores.at("RecoEvent")->Get("TrueVertex", thisTrialSeed);
	truePos = thisTrialSeed->GetPosition();
	std::cout << "true position: " << truePos.X() << "," << truePos.Y() << "," << truePos.Z() << endl;
	trueDir = thisTrialSeed->GetDirection();
	std::cout << "true direction: " << trueDir.X() << "," << trueDir.Y() << "," << trueDir.Z() << endl;

	VertexGeometry * thisVtxGeo = VertexGeometry::Instance();

	Log("VtxSeedGeneratorTool: Making fine Grid", v_message, verbosity);
	for (int i = 0; i < vSeedVtxList->size(); i++) {
		*iSeed = vSeedVtxList->at(i);
		std::cout<<vSeedVtxList->size()<<endl;
		vtxPos = iSeed->GetPosition();
		vtxDir = trueDir;
		thisVtxGeo->CalcExtendedResiduals(vtxPos.X(), vtxPos.Y(), vtxPos.Z(), 0.0, vtxDir.X(), vtxDir.Y(), vtxDir.Z());
		nhits = thisVtxGeo->GetNDigits();
		std::cout<<nhits<<endl;
		trialFoM->LoadVertexGeometry(thisVtxGeo);
		meantime = trialFoM->FindSimpleTimeProperties(ConeAngle);
		thisTrialSeed->SetVertex(vtxPos, meantime);
		thisTrialSeed->SetDirection(trueDir);
		timefom = -999.999 * 100;
		conefom = -999.999 * 100;
		trialFoM->TimePropertiesLnL(meantime, timefom);
		trialFoM->ConePropertiesFoM(ConeAngle, conefom);
		fom = 0.5*timefom + 0.5*conefom;
		if (fom > bestfom) {
			bestfom = fom;
			thisCenterSeed = thisTrialSeed;
		}
		if (pow(pow(vtxPos.X() - truePos.X(), 2) + pow(vtxPos.Y() - truePos.Y(), 2) + pow(vtxPos.Z() - truePos.Z(), 2), 0.5) < 10) {
			std::cout << "nearvtxPos: " << vtxPos.X() << "," << vtxPos.Y() << "," << vtxPos.Z() << endl;
			std::cout << "nearfom: " << fom << endl;
		}
	}
	std::cout << "Center= " << thisCenterSeed->GetPosition().X() << ", " << thisCenterSeed->GetPosition().Y() << ", " << thisCenterSeed->GetPosition().Z() << endl;
	std::cout << "Fom: " << bestfom << endl;
	//delete thisTrialSeed;
	return thisCenterSeed->GetPosition();

}

void VtxSeedFineGrid::GenerateFineGrid() {
	Position Seed;
	RecoVertex thisFineSeed;
	double medianTime;
	//double length = NSeeds something.  TODO for now setting to standard size 25x25x25, with seeds 5cm apart.
	for (int i = 0; i < 5; i++) {
		for (int j = 0; j < 5; j++) {
			for (int k = 0; k < 5; k++) {
				Seed.SetZ(Center.Z() - 10 + 5 * i);
				Seed.SetX(Center.X() - 10 + 5 * j);
				Seed.SetY(Center.Y() - 10 + 5 * k);
				//medianTime = this->GetMedianSeedTime(Seed);
				thisFineSeed.SetVertex(Seed, medianTime);
				vSeedVtxList->push_back(thisFineSeed);

			}
		}
	}

}

/*double VtxSeedFineGrid::GetMedianSeedTime(Position pos) {
	double digitx, digity, digitz, digittime;
	double dx, dy, dz, dr;
	double fC, fN;
	double seedtime;
	int fThisDigit;
	std::vector<double> extraptimes;
	for (int entry = 0; entry < vSeedDigitList.size(); entry++) {
		fThisDigit = vSeedDigitList.at(entry);
		digitx = fDigitList->at(fThisDigit).GetPosition().X();
		digity = fDigitList->at(fThisDigit).GetPosition().Y();
		digitz = fDigitList->at(fThisDigit).GetPosition().Z();
		digittime = fDigitList->at(fThisDigit).GetCalTime();
		//Now, find distance to seed position
		dx = digitx - pos.X();
		dy = digity - pos.Y();
		dz = digitz - pos.Z();
		dr = sqrt(pow(dx, 2) + pow(dy, 2) + pow(dz, 2));

		//Back calculate to the vertex time using speed of light in H20
		//Very rough estimate; ignores muon path before Cherenkov production
		//TODO: add charge weighting?  Kinda like CalcSimpleVertex?
		fC = Parameters::SpeedOfLight();
		fN = Parameters::Index0();
		seedtime = digittime - (dr / (fC / fN));
		extraptimes.push_back(seedtime);
	}
	//return the median of the extrapolated vertex times
	size_t median_index = extraptimes.size() / 2;
	std::nth_element(extraptimes.begin(), extraptimes.begin() + median_index, extraptimes.end());
	return extraptimes[median_index];
}*/
