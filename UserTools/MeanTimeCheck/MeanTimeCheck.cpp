#include "MeanTimeCheck.h"

MeanTimeCheck::MeanTimeCheck():Tool(){}


bool MeanTimeCheck::Initialise(std::string configfile, DataModel &data){

  /////////////////// Useful header ///////////////////////
  if(configfile!="") m_variables.Initialise(configfile); // loading config file
  //m_variables.Print();

	std::string output_filename;
	m_variables.Get("verbosity", verbosity);
	m_variables.Get("OutputFile", output_filename);
	m_variables.Get("ShowEvent", ShowEvent);
	Output_tfile = new TFile(output_filename.c_str(), "recreate");

	delta = new TH1D("delta", "delta", 1000, -10, 30);
	peakTime = new TH1D("Peak Time", "time", 1000, -10, 30);
	basicAverage = new TH1D("basic average", "time", 1000, -10, 30);
	weightedPeak = new TH1D("Weighted Peak", "time", 1000, -10, 30);
	weightedAverage = new TH1D("Weighted Average", "time", 1000, -10, 30);

  m_data= &data; //assigning transient data pointer
  /////////////////////////////////////////////////////////////////

  return true;
}


bool MeanTimeCheck::Execute(){

Log("===========================================================================================", v_debug, verbosity);

	Log("MeanTimeCheck Tool: Executing", v_debug, verbosity);
	auto* reco_event = m_data->Stores["RecoEvent"];
	if (!reco_event) {
		Log("Error: The MeanTimeCheck tool could not find the ANNIEEvent Store", 0, verbosity);
		return false;
	}

	m_data->Stores.at("ANNIEEvent")->Get("MCEventNum", MCEventNum);
	m_data->Stores.at("ANNIEEvent")->Get("MCTriggernum", MCTriggerNum);
	m_data->Stores.at("ANNIEEvent")->Get("EventNumber", EventNumber);

	//std::cout<<ShowEvent<<" "<<EventNumber<<std::endl;
	if (ShowEvent > 0 && EventNumber != ShowEvent) return true;
	bool EventCutstatus = false;
	auto get_evtstatus = m_data->Stores.at("RecoEvent")->Get("EventCutStatus", EventCutstatus);
	if (!get_evtstatus) {
		Log("Error: The MeanTimeCheck tool could not find the Event selection status", v_error, verbosity);
		return false;
	}
	if (!EventCutstatus) {
		Log("Message: This event doesn't pass the event selection. ", v_message, verbosity);
		return true;
	}

	auto get_vtx = m_data->Stores.at("RecoEvent")->Get("TrueVertex", TrueVtx);
	auto get_digit = m_data->Stores.at("RecoEvent")->Get("RecoDigit", DigitList);
	Position vtxPos = TrueVtx->GetPosition();
	Direction vtxDir = TrueVtx->GetDirection();
	double vtxT = TrueVtx->GetTime();
	VtxGeo = VertexGeometry::Instance();
	VtxGeo->LoadDigits(DigitList);
	VtxGeo->CalcExtendedResiduals(vtxPos.X(), vtxPos.Y(), vtxPos.Z(), vtxT, vtxDir.X(), vtxDir.Y(), vtxDir.Z());
	int nhits = VtxGeo->GetNDigits();
	//std::cout<<"nhits: "<<nhits<<std::endl;
	for (int n = 0; n < nhits; n++) {
		delta->Fill(VtxGeo->GetDelta(n));
		//std::cout<<"Delta: "<<VtxGeo->GetDelta(n)<<std::endl;
	}

std::cout<<"Check 1"<<std::endl;
	double PeakTime = this->GetPeakTime();
	double WeightedAverage = this->GetWeightedAverage();
	double WeightedPeak = this-> GetWeightedPeak();
std::cout<<"Check 2"<<std::endl;

	peakTime->Fill(PeakTime);
	weightedAverage->Fill(WeightedAverage);
	weightedPeak->Fill(WeightedPeak);
std::cout<<"Check 3"<<std::endl;
	m_data->Stores.at("RecoEvent")->Set("meanTime", PeakTime);

	return true;

}


bool MeanTimeCheck::Finalise(){

	Output_tfile->cd();
	Output_tfile->Write();
	Output_tfile->Close();
	Log("MeanTimeCheck exitting", v_debug, verbosity);

  return true;
}

double MeanTimeCheck::GetPeakTime() {
	return delta->GetBinCenter(delta->GetMaximumBin());
}

double MeanTimeCheck::GetWeightedAverage() {
	double Swx = 0.0;
	double Sw = 0.0;

	double delta = 0.0;
	double sigma = 0.0;
	double weight = 0.0;
	double deweight = 0.0;
	double deltaAngle = 0.0;
	double meanTime = 0.0;

	double myConeEdgeSigma = 7.0;  // [degrees]

	for (int idigit = 0; idigit < this->VtxGeo->GetNDigits(); idigit++) {
		int detType = this->VtxGeo->GetDigitType(idigit);
		if (this->VtxGeo->IsFiltered(idigit)) {
			delta = this->VtxGeo->GetDelta(idigit);
			sigma = this->VtxGeo->GetDeltaSigma(idigit);
			weight = 1.0 / (sigma*sigma);
			// profile in angle
			deltaAngle = this->VtxGeo->GetAngle(idigit) - Parameters::CherenkovAngle();
			// deweight hits outside cone
			if (deltaAngle <= 0.0) {
			deweight = 1.0;
			}
			else {
			deweight = 1.0 / (1.0 + (deltaAngle*deltaAngle) / (myConeEdgeSigma*myConeEdgeSigma));
			}
			Swx += deweight * weight*delta; //delta is expected vertex time 
			Sw += deweight * weight;
			}
			}
			if (Sw > 0.0) {
			meanTime = Swx * 1.0 / Sw;
			}
			
return meanTime;
}

double MeanTimeCheck::GetWeightedPeak() {
	double sigma = 0.0;
	double deltaAngle = 0.0;
	double weight = 0.0;
	double deweight = 0.0;
	double myConeEdgeSigma = 7.0;  // [degrees]
	double meanTime = 0.0;
	vector<double> deltaTime1;
	vector<double> deltaTime2;
	vector<double> TimeWeight;

	for (int idigit = 0; idigit < this->VtxGeo->GetNDigits(); idigit++) {
		if (this->VtxGeo->IsFiltered(idigit)) {
			deltaTime1.push_back(this->VtxGeo->GetDelta(idigit));
			deltaTime2.push_back(this->VtxGeo->GetDelta(idigit));
			sigma = this->VtxGeo->GetDeltaSigma(idigit);
			weight = 1.0 / (sigma*sigma);
			deltaAngle = this->VtxGeo->GetAngle(idigit) - Parameters::CherenkovAngle();
			if (deltaAngle <= 0.0) {
				deweight = 1.0;
			}
			else {
				deweight = 1.0 / (1.0 + (deltaAngle*deltaAngle) / (myConeEdgeSigma*myConeEdgeSigma));
			}
			TimeWeight.push_back(deweight*weight);
		}
	}
	int n = deltaTime1.size();
	std::sort(deltaTime1.begin(), deltaTime1.end());
	double timeMin = deltaTime1.at(int((n - 1)*0.05)); // 5% of the total entries
	double timeMax = deltaTime1.at(int((n - 1)*0.90)); // 90% of the total entries
	int nbins = int(n / 5);
	TH1D *hDeltaTime = new TH1D("hDeltaTime", "hDeltaTime", nbins, timeMin, timeMax);
	for (int i = 0; i < n; i++) {
	hDeltaTime->Fill(deltaTime2.at(i), TimeWeight.at(i));	
	hDeltaTime->Fill(deltaTime2.at(i));
	}
	meanTime = hDeltaTime->GetBinCenter(hDeltaTime->GetMaximumBin());
	delete hDeltaTime; hDeltaTime = 0;
	
	return meanTime;		
}
