#include "VtxSeedFineGrid.h"

VtxSeedFineGrid::VtxSeedFineGrid():Tool(){}


bool VtxSeedFineGrid::Initialise(std::string configfile, DataModel &data){

  /////////////////// Useful header ///////////////////////
  if(configfile!="") m_variables.Initialise(configfile); // loading config file
  //m_variables.Print();
  m_variables.Get("verbosity", verbosity);
  m_variables.Get("useTrueDir", useTrueDir);
  m_variables.Get("useSimpleDir", useSimpleDir);
  m_variables.Get("useMRDTrack", useMRDTrack);
  m_variables.Get("usePastResolution", usePastResolution);
  m_variables.Get("useDirectionGrid", useDirectionGrid);
  m_variables.Get("multiGrid", multiGrid);
  m_variables.Get("InputFile", InputFile);


  m_data= &data; //assigning transient data pointer
  /////////////////////////////////////////////////////////////////
  vSeedVtxList = nullptr;

  return true;
}


bool VtxSeedFineGrid::Execute(){
	Log("VtxSeedFineGrid Tool: Executing", v_debug, verbosity);

	auto* annie_event = m_data->Stores["RecoEvent"];
	if (!annie_event) {
		Log("Error: VtxSeedFineGrid tool could not find the RecoEvent Store", v_error, verbosity);
		return false;
	}

	m_data->Stores.at("RecoEvent")->Get("vSeedVtxList", vSeedVtxList);

	auto get_vtx = m_data->Stores.at("RecoEvent")->Get("TrueVertex", fTrueVertex);  ///> Get digits from "RecoEvent" 
/*	if (!get_vtx) {
		Log("LikelihoodFitterCheck  Tool: Error retrieving TrueVertex! ", v_error, verbosity);
		return false;
	}*/

	// Retrive digits from RecoEvent
	auto get_digit = m_data->Stores.at("RecoEvent")->Get("RecoDigit", fDigitList);  ///> Get digits from "RecoEvent" 
	if (!get_digit) {
		Log("LikelihoodFitterCheck  Tool: Error retrieving RecoDigits,no digit from the RecoEvent!", v_error, verbosity);
		return false;
	}
	auto get_flagsapp = m_data->Stores.at("RecoEvent")->Get("EventFlagApplied",fEventStatusApplied);
    auto get_flags = m_data->Stores.at("RecoEvent")->Get("EventFlagged",fEventStatusFlagged); 
    if(!get_flagsapp || !get_flags) {
      Log("PhaseITreeMaker tool: No Event status applied or flagged bitmask!!", v_error, verbosity);
      return false;	
    }
    // check if event passes the cut
    if((fEventStatusFlagged) != 0) {
      Log("PhaseIITreeMaker Tool: Event was flagged with one of the active cuts.",v_debug, verbosity);
      return true;	
    }

	if (useSimpleDir) {
		Log("Using simple direction", v_debug, verbosity);
	}

	if (useTrueDir && useMRDTrack) {
		Log("Unable to use two directions; defaulting to true direction", v_debug, verbosity);	//will change to not use true, to be more general, once MRD track usage is tested and acceptable
		useMRDTrack = 0;
	}
	if (multiGrid) {
		Log("Using MultiGrid(3)", v_debug, verbosity); //Currently using only three.  May make variable later
		std::cout << "Using MultiGrid(3)\n";
	}

	this->FindCenter();
	if (multiGrid) {
		for (int i = 0; i < 3; i++) {
			Center.push_back(vSeedVtxList->at(centerIndex[i]).GetPosition());
		}
	}
	else { Center.push_back(vSeedVtxList->at(centerIndex[0]).GetPosition()); }
	this->GenerateFineGrid();
	for (int i = 0; i < 3; i++) {
		std::cout << "Center " << i << ": " << Center.at(i).X() << ", " << Center.at(i).Y() << ", " << Center.at(i).Z() << endl;
	}
	m_data->Stores.at("RecoEvent")->Set("vSeedVtxList", vSeedVtxList, true);
	Center.clear();

  return true;
}


bool VtxSeedFineGrid::Finalise(){
	Log("VtxSeedFineGrid exitting", v_debug, verbosity);
  return true;
}

Position VtxSeedFineGrid::FindCenter() {
	double recoVtxX, recoVtxY, recoVtxZ, recoVtxT, recoDirX, recoDirY, recoDirZ;
	double trueVtxX, trueVtxY, trueVtxZ, trueVtxT, trueDirX, trueDirY, trueDirZ;
	double seedX, seedY, seedZ, seedT, seedDirX, seedDirY, seedDirZ;
	double peakX, peakY, peakZ;
	double bestFOM[3];

	double ConeAngle = Parameters::CherenkovAngle();
	 
	// Get true Vertex information
	Position vtxPos = fTrueVertex->GetPosition();
	Direction vtxDir = fTrueVertex->GetDirection();
	trueVtxX = vtxPos.X();
	trueVtxY = vtxPos.Y();
	trueVtxZ = vtxPos.Z();
	trueVtxT = fTrueVertex->GetTime();
	trueDirX = vtxDir.X();
	trueDirY = vtxDir.Y();
	trueDirZ = vtxDir.Z();
	peakX = trueVtxX;
	peakY = trueVtxY;
	peakZ = trueVtxZ;
	bestFOM[0] = 0; bestFOM[1] = 0; bestFOM[2] = 0;
	centerIndex[0] = 0; centerIndex[1] = 0; centerIndex[2] = 0;
	RecoVertex iSeed;
	RecoVertex thisCenterSeed;

	if (verbosity > 0) cout << "True vertex  = (" << trueVtxX << ", " << trueVtxY << ", " << trueVtxZ << ", " << trueVtxT << ", " << trueDirX << ", " << trueDirY << ", " << trueDirZ << ")" << endl;

	FoMCalculator myFoMCalculator;
	VertexGeometry* myvtxgeo = VertexGeometry::Instance();
	myvtxgeo->LoadDigits(fDigitList);
	myFoMCalculator.LoadVertexGeometry(myvtxgeo); //Load vertex geometry

	// fom at true vertex position
	double fom = -999.999 * 100;
	double timefom = -999.999 * 100;
	double conefom = -999.999 * 100;
	myvtxgeo->CalcExtendedResiduals(trueVtxX, trueVtxY, trueVtxZ, 0.0, trueDirX, trueDirY, trueDirZ);
	myFoMCalculator.TimePropertiesLnL(trueVtxT, timefom);
//	myFoMCalculator.ConePropertiesFoM(ConeAngle, conefom);
	fom = /*timefom * 0.5 + */conefom; // * 0.5;
	if (verbosity > 0)  cout << "VtxSeedFineGrid Tool: " << "FOM at true vertex = " << fom << endl;
  
	for (int m = 0; m < vSeedVtxList->size(); m++) {
		RecoVertex* tempVertex = 0;
		iSeed = vSeedVtxList->at(m);
		seedX = iSeed.GetPosition().X();
		seedY = iSeed.GetPosition().Y();
		seedZ = iSeed.GetPosition().Z();
		seedT = trueVtxT; //Jingbo: should use median T
		if (useTrueDir) {
			seedDirX = trueDirX;
			seedDirY = trueDirY;
			seedDirZ = trueDirZ;
		}
		else if (useMRDTrack) {
			iSeed.SetDirection(this->findDirectionMRD());
			seedDirY = iSeed.GetDirection().Y();
			seedDirZ = iSeed.GetDirection().Z();
		}
		else 	if (useSimpleDir) {			
			tempVertex = this->FindSimpleDirection(&iSeed);
			seedDirX = tempVertex->GetDirection().X();
			seedDirY = tempVertex->GetDirection().Y();
			seedDirZ = tempVertex->GetDirection().Z();
			delete tempVertex; tempVertex = 0;
		}
	/*	else if (usePastResolution) {
			TFile *f1 = new TFile(InputFile);
			TTree *t1 = (TTree*)f1->Get("phaseIITriggerTree");
			t1->Draw("((trueAngle*TMath::Pi()/180)-MRDTrackAngle)>>hs1", "abs(deltaVtxR)<2000");
			TH1D *hres = (TH1D*)gDirectory->Get("hs1");
			double smear = hres->Random();
			iSeed.GetDirection()->SetPhi((smear)+findDirectionMRD().GetPhi());
			iSeed.GetDirection()->SetTheta(findDirectionMRD().GetTheta());
		}*/
		if (useDirectionGrid) {
			for (int l = 0; l < 50; l++) {
				double theta = (6 * TMath::Pi() / 50) * l;
				double phi = (TMath::Pi() / 200) * l;
				seedDirX = sin(phi)*cos(theta);
				seedDirY = sin(phi)*sin(theta);
				seedDirZ = cos(phi);
				myvtxgeo->CalcExtendedResiduals(seedX, seedY, seedZ, seedT, seedDirX, seedDirY, seedDirZ);
				int nhits = myvtxgeo->GetNDigits();
				double meantime = myFoMCalculator.FindSimpleTimeProperties(ConeAngle);
				Double_t fom = -999.999 * 100;
				double timefom = -999.999 * 100;
				double conefom = -999.999 * 100;
				double coneAngle = 42.0;
				myFoMCalculator.TimePropertiesLnL(meantime, timefom);
				myFoMCalculator.ConePropertiesFoM(coneAngle, conefom);
				fom = timefom * 0.5 + conefom * 0.5;

				if (fom > bestFOM[0]) {
					if (multiGrid) {
						bestFOM[2] = bestFOM[1];
						bestFOM[1] = bestFOM[0];
						centerIndex[2] = centerIndex[1];
						centerIndex[1] = centerIndex[0];
					}
					bestFOM[0] = fom;
					peakX = seedX;
					peakY = seedY;
					peakZ = seedZ;
					SeedDir = iSeed.GetDirection();
					thisCenterSeed = iSeed;
					centerIndex[0] = m;
				}
			}
		}
		else {

		myvtxgeo->CalcExtendedResiduals(seedX, seedY, seedZ, seedT, seedDirX, seedDirY, seedDirZ);
		int nhits = myvtxgeo->GetNDigits();
		double meantime = myFoMCalculator.FindSimpleTimeProperties(ConeAngle);
		Double_t fom = -999.999 * 100;
		double timefom = -999.999 * 100;
		double conefom = -999.999 * 100;
		double coneAngle = 42.0;
		myFoMCalculator.TimePropertiesLnL(meantime, timefom);
		myFoMCalculator.ConePropertiesFoM(coneAngle, conefom);
		fom = timefom * 0.5 + conefom * 0.5;
		std::cout << "seed (" << seedX<<","<<seedY<<","<<seedZ<<") fom= " << fom << " best= " << bestFOM[0] << endl;

		if (fom > bestFOM[0]) {
			if (multiGrid) {
				bestFOM[2] = bestFOM[1];
				bestFOM[1] = bestFOM[0];
				centerIndex[2] = centerIndex[1];
				centerIndex[1] = centerIndex[2];
			}
			bestFOM[0] = fom;
			peakX = seedX;
			peakY = seedY;
			peakZ = seedZ;
			SeedDir = iSeed.GetDirection();
			thisCenterSeed = iSeed;
			centerIndex[0] = m;
		}
	}
	//	return thisCenterSeed.GetPosition();

	}
	return thisCenterSeed.GetPosition();
}

void VtxSeedFineGrid::GenerateFineGrid() {
	Position Seed;
	RecoVertex thisFineSeed;
	vSeedVtxList->clear();
	double medianTime;
	//double length = NSeeds something.  TODO for now setting to standard size 25x25x25, with seeds 5cm apart.
	for (int l = 0; l < 3; l++) {
		for (int i = 0; i < 5; i++) {
			for (int j = 0; j < 5; j++) {
				for (int k = 0; k < 5; k++) {
					Seed.SetZ(Center.at(l).Z() - 10 + 5 * i);
					Seed.SetX(Center.at(l).X() - 10 + 5 * j);
					Seed.SetY(Center.at(l).Y() - 10 + 5 * k);


					//medianTime = this->GetMedianSeedTime(Seed);
					thisFineSeed.SetVertex(Seed, medianTime);
					thisFineSeed.SetDirection(SeedDir);
					vSeedVtxList->push_back(thisFineSeed);

				}
			}
		}
	}
}

Direction VtxSeedFineGrid::findDirectionMRD() {
	std::vector<BoostStore>* Tracks;
	m_data->Stores["MRDTracks"]->Get("MRDTracks", Tracks);
	m_data->Stores["MRDTracks"]->Get("NumMrdTracks", numtracksinev);

	if (numtracksinev > 1) Log("Multiple tracks need work; just using first for now", v_debug, verbosity);
	double gradx, grady, theta, phi;
	Direction startVertex, endVertex, result;
	BoostStore* thisTrack = &(Tracks->at(0));
	
	thisTrack->Get("VTrackGradient", gradx);
	thisTrack->Get("HTrackGradient", grady);
	theta = atan(grady / gradx);
	phi = asin(pow((gradx*gradx + grady * grady), 0.5));
	/*TRandom3 smear;
	Direction vtxDir = fTrueVertex->GetDirection();
	Direction result;
	result.SetTheta(smear.Gaus(vtxDir.GetTheta(), 0.4));
	result.SetPhi(smear.Gaus(vtxDir.GetPhi(), 0.4));*/
	result.SetTheta(theta);
	result.SetPhi(phi);

	return result;
}

RecoVertex* VtxSeedFineGrid::FindSimpleDirection(RecoVertex* myVertex) {

	/// get vertex position
	double vtxX = myVertex->GetPosition().X();
	double vtxY = myVertex->GetPosition().Y();
	double vtxZ = myVertex->GetPosition().Z();
	double vtxTime = myVertex->GetTime();

	std::cout<<"Simple Direction Input Position: (" << vtxX << "," << vtxY << "," << vtxZ << ")\n";
	// current status
	// ==============
	int status = myVertex->GetStatus();

	/// loop over digits
	/// ================
	double Swx = 0.0;
	double Swy = 0.0;
	double Swz = 0.0;
	double Sw = 0.0;
	double digitq = 0.;
	double dx, dy, dz, ds, px, py, pz, q;

	RecoDigit digit;
	for (int idigit = 0; idigit < fDigitList->size(); idigit++) {
		digit = fDigitList->at(idigit);
		if (digit.GetFilterStatus()) {
			q = digit.GetCalCharge();
			dx = digit.GetPosition().X() - vtxX;
			dy = digit.GetPosition().Y() - vtxY;
			dz = digit.GetPosition().Z() - vtxZ;
			ds = sqrt(dx*dx + dy * dy + dz * dz);
			px = dx / ds;
			py = dx / ds;
			pz = dz / ds;
			Swx += q * px;
			Swy += q * py;
			Swz += q * pz;
			Sw += q;
		}
	}

	/// average direction
	/// =================
	double dirX = 0.0;
	double dirY = 0.0;
	double dirZ = 0.0;

	int itr = 0;
	bool pass = 0;
	double fom = 0.0;

	if (Sw > 0.0) {
		double qx = Swx / Sw;
		double qy = Swy / Sw;
		double qz = Swz / Sw;
		double qs = sqrt(qx*qx + qy * qy + qz * qz);

		dirX = qx / qs;
		dirY = qy / qs;   
		pass = 1;
	}

	// set vertex and direction
	// ========================
	RecoVertex* newVertex = new RecoVertex(); // Note: pointer must be deleted by the invoker

	if (pass) {
		newVertex->SetVertex(vtxX, vtxY, vtxZ, vtxTime);
		newVertex->SetDirection(dirX, dirY, dirZ);
		newVertex->SetFOM(fom, itr, pass);
	}

	// set status
	// ==========
	if (!pass) status |= RecoVertex::kFailSimpleDirection;
	newVertex->SetStatus(status);

	// return vertex
	// =============
	return newVertex;
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
