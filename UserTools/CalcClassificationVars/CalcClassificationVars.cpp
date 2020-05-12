#include "CalcClassificationVars.h"

CalcClassificationVars::CalcClassificationVars():Tool(){}

bool CalcClassificationVars::Initialise(std::string configfile, DataModel &data){

	// Useful header
	if(configfile!="")  m_variables.Initialise(configfile); 
	m_data= &data;

	// Default variables	
	verbosity = 2;
	isData = 0;
	lateT = 10;
	lowQ = 30;
	neutrino_sample = false;
	pdf_emu = "/annie/app/users/mnieslon/MyToolAnalysis6/pdf_beamlike_emu.root";

	// Configuration variables
	m_variables.Get("verbosity",verbosity);
	m_variables.Get("IsData",isData);
	m_variables.Get("NeutrinoSample",neutrino_sample);
	m_variables.Get("PDF_emu",pdf_emu);

	// Geometry variables
	m_data->Stores["ANNIEEvent"]->Header->Get("AnnieGeometry",geom);
	Position detector_center = geom->GetTankCentre();
	tank_center_x = detector_center.X();
	tank_center_y = detector_center.Y();
	tank_center_z = detector_center.Z();
	tank_R = geom->GetTankRadius();
	n_tank_pmts = geom->GetNumDetectorsInSet("Tank");
	n_lappds = geom->GetNumDetectorsInSet("LAPPD");
	n_mrd_pmts = geom->GetNumDetectorsInSet("MRD");
	n_veto_pmts = geom->GetNumDetectorsInSet("Veto");
  	std::map<std::string,std::map<unsigned long,Detector*> >* Detectors = geom->GetDetectors();

	// PMT x/y/z positions
	tank_ymin = 9999.;
	tank_ymax = -9999.;
	for (std::map<unsigned long,Detector*>::iterator it  = Detectors->at("Tank").begin();
                                                    it != Detectors->at("Tank").end();
                                                  ++it){
    		Detector* apmt = it->second;
    		unsigned long detkey = it->first;
    		unsigned long chankey = apmt->GetChannels()->begin()->first;
    		Position position_PMT = apmt->GetDetectorPosition();
    		pmts_x.insert(std::pair<unsigned long,double>(chankey,position_PMT.X()-tank_center_x));
    		pmts_y.insert(std::pair<unsigned long,double>(chankey,position_PMT.Y()-tank_center_y));
    		pmts_z.insert(std::pair<unsigned long,double>(chankey,position_PMT.Z()-tank_center_z));
    		if (pmts_y[chankey]>tank_ymax) tank_ymax = pmts_y.at(chankey);
    		if (pmts_y[chankey]<tank_ymin) tank_ymin = pmts_y.at(chankey);
  	}

	//Get electron/muon pdfs
	TFile *f_emu = new TFile(pdf_emu.c_str(),"READ");
	pdf_mu_charge = (TH1F*) f_emu->Get("pdf_beamlike_muon_charge");
	pdf_mu_time = (TH1F*) f_emu->Get("pdf_beamlike_muon_time");
	pdf_mu_theta = (TH1F*) f_emu->Get("pdf_beamlike_muon_thetaB");
	pdf_mu_phi = (TH1F*) f_emu->Get("pdf_beamlike_muon_phiB");
	pdf_e_charge = (TH1F*) f_emu->Get("pdf_beamlike_electron_charge");
	pdf_e_time = (TH1F*) f_emu->Get("pdf_beamlike_electron_time");
	pdf_e_theta = (TH1F*) f_emu->Get("pdf_beamlike_electron_thetaB");
	pdf_e_phi = (TH1F*) f_emu->Get("pdf_beamlike_electron_phiB");
	pdf_mu_theta->Rebin(10);
	pdf_mu_phi->Rebin(10);
	pdf_e_theta->Rebin(10);
	pdf_e_phi->Rebin(10);
	event_charge = new TH1F("event_charge","Charge values for event",500,0.,500.);
	event_time = new TH1F("event_time","Time values for event",500,-5.,45.);
	event_theta = new TH1F("event_theta","Theta values for event",50,-3.76,3.76);
	event_phi = new TH1F("event_phi","Phi values for event",50,-2.84,3.32);

	// Create classification BoostStore
	int classstoreexists = m_data->Stores.count("Classification");
	if (classstoreexists==0) m_data->Stores["Classification"] = new BoostStore(false,2);

	//Create mapping of variables to their respective variable type map
	std::vector<std::string> int_variable_names = {"PMTHits","PMTHitsLargeAnglePhi","PMTHitsLargeAngleTheta","LAPPDHits","MrdLayers","MrdPaddles","MrdConsLayers","MrdAdjHits","MrdClusters","MCNRings","EventNumber","MCPDG","MCNeutrons"};
	std::vector<std::string> double_variable_names = {"PMTBaryTheta","PMTAvgDist","PMTAvgT","PMTVarT","PMTQtotal","PMTQPerPMT","PMTQtotalClustered","PMTFracQmax","PMTFracQdownstream","PMTFracClustered","PMTFracLowQ","PMTFracEarly","PMTFracLate","PMTRMSTheta","PMTVarTheta","PMTRMSThetaBary","PMTVarThetaBary","PMTRMSPhi","PMTVarPhi","PMTRMSPhiBary","PMTVarPhiBary","PMTFracLargeAnglePhi","PMTFracLargeAngleTheta","PMTBaryTheta_Clustered","PMTBaryTheta_NonClustered","PMTDeltaBarycenter_Clustered","PMTEllip","PMTLikelihoodQ","PMTLikelihoodT","PMTLikelihoodTheta","PMTLikelihoodPhi","LAPPDBaryTheta","LAPPDAvgDist","LAPPDQtotal","LAPPDAvgT","LAPPDVarT","LAPPDRMSTheta","LAPPDVarTheta","LAPPDRMSThetaBary","LAPPDVarThetaBary","MrdPadPerLayer","MrdXSpread","MrdYSpread","MCPMTFracRing","MCPMTFracRingNoWeight","MCLAPPDFracRing","MCPMTVarTheta","MCPMTRMSTheta","MCPMTBaryTheta","MCPMTRMSThetaBary","MCPMTVarThetaBary","MCLAPPDVarTheta","MCLAPPDBaryTheta","MCLAPPDRMSTheta","MCLAPPDRMSThetaBary","MCLAPPDVarThetaBary","MCVDistVtxWall","MCHDistVtxWall","MCVDistVtxInner","MCHDistVtxInner","MCVtxTrueTime","MCTrueNeutrinoEnergy","MCTrueMuonEnergy"};
	std::vector<std::string> boolean_variable_names = {"MCMultiRing"};
	std::vector<std::string> vector_variable_names = {"PMTQVector","PMTTVector","PMTDistVector","PMTThetaVector","PMTThetaBaryVector","PMTPhiVector","PMTPhiBaryVector","PMTYVector","LAPPDQVector","LAPPDTVector","LAPPDDistVector","LAPPDThetaVector","LAPPDThetaBaryVector","MCPMTThetaBaryVector","MCLAPPDThetaBaryVector","MCPMTTVectorTOF","MCLAPPDTVectorTOF"};
	
	// Map identification number for ints: 1
	for (unsigned int i_int=0; i_int < int_variable_names.size(); i_int++){
		classification_map_map.emplace(int_variable_names.at(i_int),1);
	}
	// Map identification number for doubles: 2
	for (unsigned int i_double = 0; i_double < double_variable_names.size(); i_double++){
		classification_map_map.emplace(double_variable_names.at(i_double),2);
	}
	// Map identification number for bools: 3
	for (unsigned int i_bool = 0; i_bool < boolean_variable_names.size(); i_bool++){
		classification_map_map.emplace(boolean_variable_names.at(i_bool),3);
	}
	// Map identification number for vectors: 4
	for (unsigned int i_vector = 0; i_vector < vector_variable_names.size(); i_vector++){
		classification_map_map.emplace(vector_variable_names.at(i_vector),4);
	}

	m_data->Stores["Classification"]->Set("ClassificationMapMap",classification_map_map);


	//int myargc = 0;
	//char *myargv[] {(char*) "options"};
	//app_calc = new TApplication("app_calc",&myargc,myargv);
	
	return true;
}


bool CalcClassificationVars::Execute(){

	Log("CalcClassificationVars Tool: Executing",v_debug,verbosity);

	// Check if BoostStores exist
	get_ok = m_data->Stores.count("ANNIEEvent");
	if(!get_ok){
		Log("CalcClassificationVars Tool: No ANNIEEvent store!",v_error,verbosity);
		return false;
	};

	get_ok = m_data->Stores.count("RecoEvent");
	if(!get_ok){
		Log("CalcClassificationVars Tool: No RecoEvent store!",v_error,verbosity);
		return false;
	}

	// Get ANNIEEvent,RecoEvent,CStore variables
	this->GetBoostStoreVariables();
	
	// Clear maps and set variables to default settings
	classification_map_int.clear();
	classification_map_bool.clear();
	classification_map_double.clear();
	classification_map_vector.clear();
	
	multi_ring = false;
	if (!isData && nrings > 1) multi_ring = true;

	
	//Check first if the cuts were passed
	
	bool sensible_energy = true;
	if (!isData && fabs(TrueMuonEnergy+9999)<0.01) sensible_energy = false;
	if (!EventCutStatus || !sensible_energy){

		//If EventSelection cuts were not passed, do not calculate any variables

		Log("CalcClassificationVars Tool: EventCutStatus is false! Selected event cuts were not passed...",v_message,verbosity);

		m_data->Stores["Classification"]->Set("isData",isData);
		m_data->Stores["Classification"]->Set("MLDataPresent",0);
		m_data->Stores["Classification"]->Set("SelectionPassed",0);

	} else {

		//If EventSelection cuts were passed, get the relevant information from the data
		
		//Evaluate true information
		if (!isData) this->ClassificationVarsMCTruth();
		
		//Evaluate PMT/LAPPD classification variables
		this->ClassificationVarsPMTLAPPD();

		//Evaluate MRD classification variables
		this->ClassificationVarsMRD();


		//General variables
		m_data->Stores["Classification"]->Set("isData",isData);
		m_data->Stores["Classification"]->Set("SelectionPassed",1);
		m_data->Stores["Classification"]->Set("MLDataPresent",1);
		
		//Variable maps
		m_data->Stores["Classification"]->Set("ClassificationMapInt",classification_map_int);
		m_data->Stores["Classification"]->Set("ClassificationMapDouble",classification_map_double);
		m_data->Stores["Classification"]->Set("ClassificationMapBool",classification_map_bool);
		m_data->Stores["Classification"]->Set("ClassificationMapVector",classification_map_vector);


	}

	Log("CalcClassificationVars tool: Execution step finished",v_message,verbosity);

	return true;

}


bool CalcClassificationVars::Finalise(){

  
	TFile *f = new TFile("temp.root","RECREATE");
	f->cd();
	pdf_mu_charge->Write();
	pdf_mu_time->Write();
	pdf_mu_theta->Write();
	pdf_mu_phi->Write();
	pdf_e_charge->Write();
	pdf_e_time->Write();
	pdf_e_theta->Write();
	pdf_e_phi->Write();
	event_charge->Write();
	event_time->Write();
	event_theta->Write();
	event_phi->Write();
	f->Close();
	delete f;
	Log("CalcClassificationVars Tool: Finalisation complete",v_message,verbosity);

	return true;

}

double CalcClassificationVars::CalcArcTan(double x, double z){

	double atan_result=0;
	if (x > 0. && z > 0. ) atan_result = atan(z/x)+TMath::Pi()/2.;
	if (x > 0. && z < 0. ) atan_result = atan(x/-z);
	if (x < 0. && z < 0. ) atan_result = 3*TMath::Pi()/2.+atan(z/x);
	if (x < 0. && z > 0. ) atan_result = TMath::Pi()+atan(-x/z);

	return atan_result;
}

bool CalcClassificationVars::GetBoostStoreVariables(){

	//-----------------------------------------------------------------------
	//------------------- Getting objects from ANNIEEvent -------------------
	//-----------------------------------------------------------------------

	get_ok = m_data->Stores.at("ANNIEEvent")->Get("EventNumber",evnum);
	if(not get_ok){ Log("CalcClassificationVars Tool: Error retrieving EventNumber,true from ANNIEEvent!",v_error,verbosity); return false; }
	get_ok = m_data->Stores.at("ANNIEEvent")->Get("MCEventNum",mcevnum);
	if(not get_ok){ Log("CalcClassificationVars Tool: Error retrieving MCEventNum,true from ANNIEEvent!",v_error,verbosity); return false; }
	get_ok = m_data->Stores.at("ANNIEEvent")->Get("TDCData",TDCData);
	if(not get_ok){ Log("CalcClassificationVars Tool: Error retrieving TDCData,true from ANNIEEvent!",v_error,verbosity); return false; }

	if (!isData){
		get_ok = m_data->Stores.at("ANNIEEvent")->Get("MCFile",filename);
		if(not get_ok){ Log("CalcClassificationVars Tool: Error retrieving MCFilename, true from ANNIEEvent!",v_error, verbosity); return false;}
		get_ok = m_data->Stores.at("ANNIEEvent")->Get("MCParticles",mcparticles);
		if(not get_ok){ Log("CalcClassificationVars Tool: Error retrieving MCParticles, true from ANNIEEvent!",v_error,verbosity); return false;}
		if (neutrino_sample){
			get_ok = m_data->Stores.at("ANNIEEvent")->Get("NeutrinoParticle",neutrino);
			if(not get_ok){ Log("CalcClassificationVars Tool: Error retrieving NeutrinoParticle, true from ANNIEEvent!",v_error,verbosity); return false;}
		}
	}

	//-----------------------------------------------------------------------
	//------------------- Getting objects from RecoEvent -------------------
	//-----------------------------------------------------------------------

	get_ok = m_data->Stores.at("RecoEvent")->Get("EventCutStatus",EventCutStatus);
	if(not get_ok){ Log("CalcClassificationVars Tool: Error retrieving EventCutStatus,true from RecoEvent!",v_error,verbosity); return false; }
	get_ok = m_data->Stores.at("RecoEvent")->Get("RecoDigit",RecoDigits);
	if(not get_ok){ Log("CalcClassificationVars Tool: Error retrieving RecoDigit,true from RecoEvent!",v_error,verbosity); return false; }
	if (!isData){
		get_ok = m_data->Stores.at("RecoEvent")->Get("TrueMuonEnergy",TrueMuonEnergy);
		if(not get_ok){ Log("CalcClassificationVars Tool: Error retrieving TrueMuonEnergy,true from RecoEvent!",v_error,verbosity); return false; }
		get_ok = m_data->Stores.at("RecoEvent")->Get("TrueVertex",TrueVertex);
		if(not get_ok){ Log("CalcClassificationVars Tool: Error retrieving TrueVertex,true from RecoEvent!",v_error,verbosity); return false; }
		get_ok = m_data->Stores.at("RecoEvent")->Get("TrueStopVertex",TrueStopVertex);
		if(not get_ok){ Log("CalcClassificationVars Tool: Error retrieving TrueStopVertex,true from RecoEvent!",v_error,verbosity); return false; }
		get_ok = m_data->Stores.at("RecoEvent")->Get("NRings",nrings);
		if(not get_ok){ Log("ParticleIDPDF Tool: Error retrieving NRings, true from RecoEvent!",v_error,verbosity); return false; }
		get_ok = m_data->Stores.at("RecoEvent")->Get("PdgPrimary",pdg);
		if(not get_ok){ Log("ParticleIDPDF Tool: Error retrieving PdgPrimary, true from RecoEvent!",v_error,verbosity); return false; }
	}

	//-----------------------------------------------------------------------
	//------------------- Getting objects from CStore -----------------------
	//-----------------------------------------------------------------------
	
	get_ok = m_data->CStore.Get("NumMrdTimeClusters",NumMrdTimeClusters);
	if(not get_ok){ Log("CalcClassificationVars Tool: Error retrieving NumMrdTimeClusters, did you run TimeClustering beforehand?",v_error,verbosity); return false; }

	get_ok = m_data->CStore.Get("PdgCherenkovMap",pdgcodetocherenkov);
	if(not get_ok){ Log("CalcClassificationVars Tool: Error retrieving PdgCherenkovMap, did you run MCRecoEventLoader beforehand?",v_error,verbosity); return false; }

	return true;

}

void CalcClassificationVars::ClassificationVarsMCTruth(){
		

	// Neutrino-related information (true)		
	if (neutrino_sample) TrueNeutrinoEnergy = neutrino.GetStartEnergy();
	else TrueNeutrinoEnergy = -999;

	// Vertex-related information (true)
	pos_x=-999;
	pos_y=-999;
	pos_z=-999;
	dir_x=-999;
	dir_y=-999;
	dir_z=-999;
	double distWallVert=-999, distWallHor=-999, distInnerStrHor=-999, distInnerStrVert=-999, true_time=-999;
	Direction dir;	

	pos = TrueVertex->GetPosition();
	pos.UnitToMeter();
	pos_x = pos.X();
	pos_y = pos.Y();
	pos_z = pos.Z();
	dir = TrueVertex->GetDirection();
	dir_x = dir.X();
	dir_y = dir.Y();
	dir_z = dir.Z();
	distWallVert = fabs(1.98-pos_y);
	distWallHor = 1.524 - sqrt(pos_x*pos_x+pos_z*pos_z);
	distWallVert /= 1.98;
	distWallHor /= 1.524;
	distInnerStrHor = tank_innerR - sqrt(pos_x*pos_x+pos_z*pos_z);
	distInnerStrVert = (pos_y > 0)? (tank_ymax - pos_y) : (pos_y - tank_ymin);
	true_time = TrueVertex->GetTime();

	logmessage = "CalcClassificationVars Tool: Interaction Vertex is at ("+to_string(pos_x)
	    +", "+to_string(pos_y)+", "+to_string(pos_z)+")\n"
	    +"CalcClassificationVars Tool: Primary particle has energy "+to_string(TrueMuonEnergy)+"MeV and direction ("
	    +to_string(dir_x)+", "+to_string(dir_y)+", "+to_string(dir_z)+" )";
	Log(logmessage,v_message,verbosity);
	
	this->StorePionEnergies();
	
	classification_map_double.emplace("MCVDistVtxWall",distWallVert);
	classification_map_double.emplace("MCHDistVtxWall",distWallHor);
	classification_map_double.emplace("MCVDistVtxInner",distInnerStrVert);
	classification_map_double.emplace("MCHDistVtxInner",distInnerStrHor);
	classification_map_double.emplace("MCVtxTrueTime",true_time);
	classification_map_double.emplace("MCTrueNeutrinoEnergy",TrueNeutrinoEnergy);
	classification_map_double.emplace("MCTrueMuonEnergy",TrueMuonEnergy);
	classification_map_int.emplace("MCNRings",nrings);
	classification_map_bool.emplace("MCMultiRing",multi_ring);
	classification_map_int.emplace("EventNumber",evnum);
	//classification_map.emplace("MCFilename",filename);  //TODO: sort somewhere
	classification_map_int.emplace("MCPDG",pdg);

}

void CalcClassificationVars::ClassificationVarsPMTLAPPD(){

	Log("CalcClassificationVars tool: Reading out PMT/LAPPD data",v_message,verbosity);
	
	event_charge->Reset();
	event_time->Reset();
	event_theta->Reset();
	event_phi->Reset();

	// Information available both in data & MC
	double pmt_QDownstream=0.;
	double pmt_avgT=0.;
	double pmt_varT=0.;
	Position pmtBaryQ(0.,0.,0.);
	Position pmtBaryQ_Clustered(0.,0.,0.);
	Position pmtBaryQ_NonClustered(0.,0.,0.);
	double pmt_totalQ=0.;
	double pmt_totalQ_Clustered=0.;
	double pmt_totalQ_NonClustered=0.;
	double pmt_highestQ=0.;
	int pmt_hits=0;
	int pmt_hits_late=0;
	int pmt_hits_early=0;
	int pmt_hits_lowq=0;
	Position lappdBaryQ(0.,0.,0.);
	double lappd_totalQ=0.;
	double lappd_avgT=0.;
	double lappd_varT=0.;
	int lappd_hits=0;
	std::vector<double> pmtQ, lappdQ, pmtT, pmtT_tof, lappdT, lappdT_tof;
	std::vector<Position> pmtPos, lappdPos;
	std::vector<double> pmtDist, pmtTheta, pmtPhi, pmtThetaBary, pmtPhiBary, pmtY, lappdDist, lappdTheta, lappdThetaBary, pmtThetaMC, pmtThetaBaryMC, lappdThetaMC, lappdThetaBaryMC;
	double pmt_rmsTheta=0.; 
	double pmt_rmsThetaMC=0.;
	double pmt_rmsPhi=0.;
	double pmt_avgDist=0.;
	double lappd_rmsTheta=0.;
	double lappd_rmsThetaMC=0.;
	double lappd_avgDist=0.;

 	// Information only available when using mctruth information
	double pmt_QRing=0.;
	int pmt_hitsRing=0;
	double lappd_QRing=0.;
	int lappd_hitsRing=0;
	Log("CalcClassificationVars tool: Reading in RecoDigits object of size: "+std::to_string(RecoDigits->size()),v_debug,verbosity);

	for (unsigned int i_digit = 0; i_digit < RecoDigits->size(); i_digit++){
		RecoDigit thisdigit = RecoDigits->at(i_digit);
		Position detector_pos = thisdigit.GetPosition();
		detector_pos.UnitToMeter();
		Direction detector_dir(detector_pos.X()-pos_x,detector_pos.Y()-pos_y,detector_pos.Z()-pos_z);
		double detector_dirX = detector_pos.X()-pos_x;
		double detector_dirY = detector_pos.Y()-pos_y;
		double detector_dirZ = detector_pos.Z()-pos_z;
		int digittype = thisdigit.GetDigitType();		//0 - PMTs, 1 - LAPPDs	
		double digitQ = thisdigit.GetCalCharge();
		double digitT = thisdigit.GetCalTime();
		double detDist, detTheta, detPhi;
		double MCdetDist, MCdetTheta;
			

		//Standard calculations with respect to the center of the tank
		detDist = sqrt(pow(detector_pos.X(),2)+pow(detector_pos.Y(),2)+pow(detector_pos.Z(),2));
		detTheta = acos((detector_pos.Z()*1.)/detDist);
		detPhi = CalcArcTan(detector_pos.X(), detector_pos.Z());

		if (!isData){
			//For MC, do additional calculations with respect to interaction vertex
			MCdetDist = sqrt(pow(detector_dirX,2)+pow(detector_dirY,2)+pow(detector_dirZ,2));
			MCdetTheta = acos((detector_dirX*dir_x+detector_dirY*dir_y+detector_dirZ*dir_z)/MCdetDist);
		}

		if (digittype == 0){   //PMT hit

			// Time/charge variables
			pmtQ.push_back(digitQ);
			pmtT.push_back(digitT);
			event_charge->Fill(digitQ);
			event_time->Fill(digitT);
			pmtPos.push_back(detector_pos);
			pmt_totalQ+=digitQ;
			pmt_avgT+=digitT;
			pmt_hits++;
			pmtBaryQ += digitQ*detector_pos;
			pmt_avgDist+=detDist;

			// Early/late/lowQ classification
			if (digitQ < 30) pmt_hits_lowq++;
			if (digitT > 10) pmt_hits_late++;	
			else if (digitT < 4) pmt_hits_early++;
			if (digitQ > pmt_highestQ) pmt_highestQ = digitQ;
			if (detTheta < TMath::Pi()/2.) pmt_QDownstream+=digitQ;

			// Angular variables
			pmt_rmsTheta+=(detTheta*detTheta);
			pmt_rmsPhi+=(detPhi*detPhi);
			pmtDist.push_back(detDist);
			pmtTheta.push_back(detTheta);
			pmtPhi.push_back(detPhi);
			pmtY.push_back(detector_dirY);
			if (!isData) {
				pmtThetaMC.push_back(MCdetTheta);
				pmt_rmsThetaMC+=(MCdetTheta)*(MCdetTheta);
			}

			// HitCleaner filter status
			if (thisdigit.GetFilterStatus() == 1) {
				pmt_totalQ_Clustered+=digitQ;
				pmtBaryQ_Clustered += digitQ*detector_pos;
			} else {
				pmt_totalQ_NonClustered+=digitQ;
				pmtBaryQ_NonClustered += digitQ*detector_pos;
			}

			// Cherenkov angle-related variables
			if (!isData){

				double tof = MCdetDist/3.0e8*1e9;
				double t_tof = digitT-tof;
				pmtT_tof.push_back(t_tof);

				if (MCdetTheta < cherenkov_angle) {
					pmt_QRing+=digitQ;
					pmt_hitsRing++;
				}
			}


		} else if (digittype == 1){		//LAPPD hit

			// Time/charge variables
			lappdQ.push_back(digitQ);
			lappdT.push_back(digitT);
			lappdPos.push_back(detector_pos);
			lappdBaryQ += digitQ*detector_pos;
			lappd_totalQ+=digitQ;
			lappd_hits++;
			lappd_avgT+=digitT;
			lappd_avgDist+=detDist;
			
			// Angular variables
			lappd_rmsTheta+=(detTheta*detTheta);
			lappdDist.push_back(detDist);
			lappdTheta.push_back(detTheta);
			if (!isData){
				lappd_rmsThetaMC+=(MCdetTheta)*(MCdetTheta);
				lappdThetaMC.push_back(MCdetTheta);
			}
			// Cherenkov-angle related variables
			if (!isData){
				double tof = MCdetDist/3.0e8*1e9;
				double t_tof = digitT-tof;
				lappdT_tof.push_back(t_tof);

				if (MCdetTheta < cherenkov_angle) {
					lappd_QRing+=digitQ;
					lappd_hitsRing++;
				}
			}

		} else {
			
			std::string logmessage = "CalcClassificationVars tool: Digit type " + std::to_string(digittype) + "was not recognized. Omit reading in entry from RecoEvent store.";
			Log(logmessage,v_message,verbosity);
			
		}

	}
	
	//-----------------------------------------------------------------------
	//--------------- Calculate classification variables --------------------
	//-----------------------------------------------------------------------

	double pmt_fracRing=0.;
	double pmt_fracRingNoWeight=0.;	
	double pmt_frachighestQ=0.;
	double pmt_fracQDownstream=0.;
	double pmt_fracClustered=0.;
	double pmt_fracLowQ=0.;
	double pmt_fracLate=0.;
	double pmt_fracEarly=0.;
	double lappd_fracRing=0.;
	double pmt_ellip = 0.;
	double pmt_qpmt=0.;

	if (pmtQ.size()!=0) {

		// Average PMT charge/time/barycenter
		pmt_avgT /= pmtQ.size();
		pmt_avgDist /= pmtQ.size();
		pmt_rmsTheta = sqrt(pmt_rmsTheta/pmtQ.size());
		pmt_rmsPhi = sqrt(pmt_rmsPhi/pmtQ.size());
		pmtBaryQ = (1./pmt_totalQ)*pmtBaryQ;
		m_data->CStore.Set("pmtBaryQ",pmtBaryQ);
		if (pmt_totalQ_Clustered > 0.) pmtBaryQ_Clustered = (1./pmt_totalQ_Clustered)*pmtBaryQ_Clustered;
		if (pmt_totalQ_NonClustered > 0.) pmtBaryQ_NonClustered = (1./pmt_totalQ_NonClustered)*pmtBaryQ_NonClustered;
		pmt_qpmt = pmt_totalQ/pmtQ.size();

		// PMT fractions
		pmt_fracQDownstream = pmt_QDownstream/pmt_totalQ;
		pmt_frachighestQ = pmt_highestQ/pmt_totalQ;
		pmt_fracClustered = pmt_totalQ_Clustered/pmt_totalQ;
		pmt_fracLowQ = double(pmt_hits_lowq)/pmt_hits;
		pmt_fracLate = double(pmt_hits_late)/pmt_hits;		
		pmt_fracEarly = double(pmt_hits_early)/pmt_hits;

		// PMT fractions related to Cherenkov ring
		if (!isData){
			pmt_fracRing = pmt_QRing/pmt_totalQ;
			pmt_fracRingNoWeight = double(pmt_hitsRing)/pmt_hits;
			pmt_rmsThetaMC = sqrt(pmt_rmsThetaMC/pmtQ.size());
		}
	}
	if (lappdQ.size()!=0){
		
		// Average LAPPD charge/time/barycenter
		lappd_avgT /= lappdQ.size();
		lappd_avgDist /= lappdQ.size();
		lappd_rmsTheta = sqrt(lappd_rmsTheta/lappdQ.size());
		lappdBaryQ = (1./lappd_totalQ)*lappdBaryQ;

		// LAPPD fractions related to Cherenkov ring
		if (!isData){
			lappd_fracRing = lappd_QRing/lappd_totalQ;
			lappd_rmsThetaMC = sqrt(lappd_rmsThetaMC/lappdQ.size());
		}
	}

	double pmtBaryDist, pmtBaryTheta, pmtBaryPhi, pmtBaryY, lappdBaryDist, lappdBaryTheta;
	double pmtBaryDist_Clustered, pmtBaryDist_NonClustered, pmtBaryTheta_Clustered, pmtBaryTheta_NonClustered;
	double pmtBaryDistMC, pmtBaryThetaMC, lappdBaryDistMC, lappdBaryThetaMC;

	if (!isData){

		// Angle and distance of barycenter calculated with respect to interaction point and primary particle direction
		Position dirBaryQ = pmtBaryQ-pos;
		pmtBaryDistMC = dirBaryQ.Mag();
		pmtBaryThetaMC = acos((dirBaryQ.X()*dir_x+dirBaryQ.Y()*dir_y+dirBaryQ.Z()*dir_z)/pmtBaryDistMC);
		Position lappd_dirBaryQ = lappdBaryQ-pos;
		lappdBaryDistMC = lappd_dirBaryQ.Mag();
		lappdBaryThetaMC = acos((lappd_dirBaryQ.X()*dir_x+lappd_dirBaryQ.Y()*dir_y+lappd_dirBaryQ.Z()*dir_z)/lappdBaryDistMC);

	} 
	
	// Angle and distance of barycenter calculated with respect to (0,0,0)-position and (0,0,1)-direction
	pmtBaryDist = pmtBaryQ.Mag();
	pmtBaryDist_Clustered = pmtBaryQ_Clustered.Mag();
	pmtBaryDist_NonClustered = pmtBaryQ_NonClustered.Mag();
	if (fabs(pmtBaryDist)<0.0001) {
		pmtBaryTheta = 0;
		pmtBaryPhi = 0.;
		pmtBaryY = 0.;
		pmtBaryTheta_Clustered = 0.;
		pmtBaryTheta_NonClustered = 0.;
	}
	else {
		pmtBaryTheta = acos(pmtBaryQ.Z()/pmtBaryDist);
		pmtBaryPhi = CalcArcTan(pmtBaryQ.X(),pmtBaryQ.Z());
		pmtBaryY = pmtBaryQ.Y();
		pmtBaryTheta_Clustered = acos(pmtBaryQ_Clustered.Z()/pmtBaryDist_Clustered);
		pmtBaryTheta_NonClustered = acos(pmtBaryQ_NonClustered.Z()/pmtBaryDist_NonClustered);
	}
	lappdBaryDist = lappdBaryQ.Mag();
	if (fabs(lappdBaryDist)<0.001) lappdBaryTheta=0.;
	else lappdBaryTheta = acos(lappdBaryQ.Z()/lappdBaryDist);

	double diff_barycenter_clustered = fabs(pmtBaryTheta_Clustered-pmtBaryTheta_NonClustered);

	// Calculate variance/RMS of variables
	double pmt_varTheta=0.;
	double pmt_varThetaBary=0.;
	double pmt_varThetaMC=0.;
	double pmt_varThetaBaryMC=0.;
	double pmt_varPhi=0.;
	double pmt_varPhiBary = 0.;
	double pmt_theta_bary = 0.;
	double pmt_theta_baryMC = 0.;
	double lappd_theta_bary = 0.;
	double lappd_theta_baryMC = 0.;
	double lappd_varTheta=0.;
	double lappd_varThetaBary=0.;
	double lappd_varThetaMC=0.;
	double lappd_varThetaBaryMC=0.;
	double pmt_rmsThetaBary=0.;
	double pmt_rmsThetaBaryMC=0.;
	double pmt_rmsPhiBary = 0.;
	double lappd_rmsThetaBary=0.;
	double lappd_rmsThetaBaryMC=0.;
	double pmt_fracLargeAnglePhi=0.;
	double pmt_fracLargeAngleTheta=0.;
	int pmt_hits_largeangle_theta=0;
	int pmt_hits_largeangle_phi=0;	

	// Variances and RMS of angles/times with respect to barycenter
	for (unsigned int i_pmt=0; i_pmt < pmtQ.size(); i_pmt++){
		
		pmt_varT+=pow(pmtT.at(i_pmt)-pmt_avgT,2);
		pmt_varTheta+=(pow(pmtTheta.at(i_pmt),2)*pmtQ.at(i_pmt)/pmt_totalQ);
		pmt_theta_bary = pmtTheta.at(i_pmt) - pmtBaryTheta;
		event_theta->Fill(pmt_theta_bary);
		pmtThetaBary.push_back(pmt_theta_bary);
		pmt_rmsThetaBary+=pow(pmt_theta_bary,2);
		pmt_varThetaBary+=(pow(pmt_theta_bary,2)*pmtQ.at(i_pmt)/pmt_totalQ);
		if (!isData){
			pmt_varThetaMC+=(pow(pmtThetaMC.at(i_pmt),2)*pmtQ.at(i_pmt)/pmt_totalQ);
			pmt_theta_baryMC = pmtThetaMC.at(i_pmt) - pmtBaryThetaMC;
			pmtThetaBaryMC.push_back(pmt_theta_baryMC);
			pmt_rmsThetaBaryMC+=pow(pmt_theta_baryMC,2);
			pmt_varThetaBaryMC+=(pow(pmt_theta_bary,2)*pmtQ.at(i_pmt)/pmt_totalQ);
		}			

		pmt_varPhi+=(pow(pmtPhi.at(i_pmt),2)*pmtQ.at(i_pmt)/pmt_totalQ);
		double pmt_phi_bary = (pmtPhi.at(i_pmt)-pmtBaryPhi);
		if (pmt_phi_bary > TMath::Pi()) pmt_phi_bary = -(2*TMath::Pi()-pmt_phi_bary);
		else if (pmt_phi_bary < -TMath::Pi()) pmt_phi_bary = 2*TMath::Pi()+pmt_phi_bary;
		event_phi->Fill(pmt_phi_bary);
		pmtPhiBary.push_back(pmt_phi_bary);
		pmt_rmsPhiBary+=(pow(pmt_phi_bary,2));
		pmt_varPhiBary+=(pow(pmt_phi_bary,2)*pmtQ.at(i_pmt)/pmt_totalQ);
		
		if (fabs(pmt_theta_bary) > 0.9) pmt_hits_largeangle_theta++;
		if (fabs(pmt_phi_bary) > 1.) pmt_hits_largeangle_phi++;
	}
	
	if (pmtQ.size()>0) {

		pmt_varT = sqrt(pmt_varT/pmtQ.size());
		pmt_rmsThetaBary = sqrt(pmt_rmsThetaBary/pmtQ.size());
		pmt_rmsPhiBary = sqrt(pmt_rmsPhiBary/pmtQ.size());
		pmt_ellip = pmt_rmsPhiBary/pmt_rmsThetaBary;
		pmt_fracLargeAngleTheta = double(pmt_hits_largeangle_theta)/pmtQ.size();
		pmt_fracLargeAnglePhi = double(pmt_hits_largeangle_phi)/pmtQ.size();	
		pmt_varTheta = sqrt(pmt_varTheta);
		pmt_varPhi = sqrt(pmt_varPhi);
		if (!isData){
			pmt_rmsThetaBaryMC = sqrt(pmt_rmsThetaBaryMC/pmtQ.size());
			pmt_varThetaMC = sqrt(pmt_varThetaMC);
		}
	}	

	for (unsigned int i_lappd=0; i_lappd< lappdQ.size(); i_lappd++){
	
		lappd_varT+=pow(lappdT.at(i_lappd)-lappd_avgT,2);
		lappd_varTheta+=(pow(lappdTheta.at(i_lappd),2)*lappdQ.at(i_lappd)/lappd_totalQ);
		lappd_theta_bary = lappdTheta.at(i_lappd)-lappdBaryTheta;
		lappd_rmsThetaBary+=pow(lappd_theta_bary,2);
		lappd_varThetaBary+=(pow(lappd_theta_bary,2)*lappdQ.at(i_lappd)/lappd_totalQ);
		lappdThetaBary.push_back(lappd_theta_bary);
		if (!isData){
			lappd_varThetaMC+=(pow(lappdThetaMC.at(i_lappd),2)*lappdQ.at(i_lappd)/lappd_totalQ);
			lappd_theta_baryMC = lappdThetaMC.at(i_lappd)-lappdBaryThetaMC;
			lappd_rmsThetaBaryMC+=pow(lappd_theta_baryMC,2);
			lappd_varThetaBaryMC+=(pow(lappd_theta_baryMC,2)*lappdQ.at(i_lappd)/lappd_totalQ);
			lappdThetaBaryMC.push_back(lappd_theta_baryMC);
		}
	}
	if (lappdQ.size()>0) {
		lappd_varT = sqrt(lappd_varT/lappdQ.size());
		lappd_rmsThetaBary = sqrt(lappd_rmsThetaBary/lappdQ.size());
		lappd_varTheta = sqrt(lappd_varTheta);
		if (!isData){
			lappd_rmsThetaBaryMC = sqrt(lappd_rmsThetaBaryMC/lappdQ.size());
			lappd_varThetaMC = sqrt(lappd_varThetaMC);
		}
	}

	pdf_mu_charge->Scale(event_charge->Integral());
	pdf_mu_time->Scale(event_time->Integral());
	pdf_mu_theta->Scale(event_theta->Integral());
	pdf_mu_phi->Scale(event_phi->Integral());
	pdf_e_charge->Scale(event_charge->Integral());
	pdf_e_time->Scale(event_time->Integral());
	pdf_e_theta->Scale(event_theta->Integral());
	pdf_e_phi->Scale(event_phi->Integral());
	double pmt_charge_mu = pdf_mu_charge->Chi2Test(event_charge,"UUPNORMCHI2");
	double pmt_time_mu = pdf_mu_time->Chi2Test(event_time,"UUPNORMCHI2");
	double pmt_theta_mu = pdf_mu_theta->Chi2Test(event_theta,"UUPNORMCHI2");
	double pmt_phi_mu = pdf_mu_phi->Chi2Test(event_phi,"UUPNORMCHI2");
	double pmt_charge_e = pdf_e_charge->Chi2Test(event_charge,"UUPNORMCHI2");
	double pmt_time_e = pdf_e_time->Chi2Test(event_time,"UUPNORMCHI2");
	double pmt_theta_e = pdf_e_theta->Chi2Test(event_theta,"UUPNORMCHI2");
	double pmt_phi_e = pdf_e_phi->Chi2Test(event_phi,"UUPNORMCHI2");
	double pmt_charge_likelihood = pmt_charge_mu - pmt_charge_e;
	double pmt_time_likelihood = pmt_time_mu - pmt_time_e;
	double pmt_theta_likelihood = pmt_theta_mu - pmt_theta_e;
	double pmt_phi_likelihood = pmt_phi_mu - pmt_phi_e;
	pdf_mu_charge->Scale(1./event_charge->Integral());
	pdf_mu_time->Scale(1./event_time->Integral());
	pdf_mu_theta->Scale(1./event_theta->Integral());
	pdf_mu_phi->Scale(1./event_phi->Integral());
	pdf_e_charge->Scale(1./event_charge->Integral());
	pdf_e_time->Scale(1./event_time->Integral());
	pdf_e_theta->Scale(1./event_theta->Integral());
	pdf_e_phi->Scale(1./event_phi->Integral());


	// PMT variables
	classification_map_double.emplace("PMTBaryTheta",pmtBaryTheta);
	classification_map_double.emplace("PMTAvgDist",pmt_avgDist);
	classification_map_double.emplace("PMTAvgT",pmt_avgT);
	classification_map_double.emplace("PMTVarT",pmt_varT);
	classification_map_double.emplace("PMTQtotal",pmt_totalQ);
	classification_map_double.emplace("PMTQtotalClustered",pmt_totalQ_Clustered);
	classification_map_int.emplace("PMTHits",pmt_hits);
	classification_map_double.emplace("PMTQPerPMT",pmt_qpmt);
	classification_map_double.emplace("PMTFracQmax",pmt_frachighestQ);
	classification_map_double.emplace("PMTFracQdownstream",pmt_fracQDownstream);
	classification_map_double.emplace("PMTFracClustered",pmt_fracClustered);
	classification_map_double.emplace("PMTFracLowQ",pmt_fracLowQ);
	classification_map_double.emplace("PMTFracEarly",pmt_fracEarly);
	classification_map_double.emplace("PMTFracLate",pmt_fracLate);
	classification_map_double.emplace("PMTRMSTheta",pmt_rmsTheta);
	classification_map_double.emplace("PMTVarTheta",pmt_varTheta);
	classification_map_double.emplace("PMTRMSThetaBary",pmt_rmsThetaBary);
	classification_map_double.emplace("PMTVarThetaBary",pmt_varThetaBary);
	classification_map_double.emplace("PMTRMSPhi",pmt_rmsPhi);
	classification_map_double.emplace("PMTVarPhi",pmt_varPhi);
	classification_map_double.emplace("PMTRMSPhiBary",pmt_rmsPhiBary);
	classification_map_double.emplace("PMTVarPhiBary",pmt_varPhiBary);
	classification_map_double.emplace("PMTFracLargeAnglePhi",pmt_fracLargeAnglePhi);
	classification_map_double.emplace("PMTFracLargeAngleTheta",pmt_fracLargeAngleTheta);
	classification_map_int.emplace("PMTHitsLargeAngleTheta",pmt_hits_largeangle_theta);
	classification_map_int.emplace("PMTHitsLargeAnglePhi",pmt_hits_largeangle_phi);
	classification_map_double.emplace("PMTEllip",pmt_ellip);
	classification_map_double.emplace("PMTBaryTheta_Clustered",pmtBaryTheta_Clustered);
	classification_map_double.emplace("PMTBaryTheta_NonClustered",pmtBaryTheta_NonClustered);
	classification_map_double.emplace("PMTDeltaBarycenter_Clustered",diff_barycenter_clustered);
	classification_map_double.emplace("PMTLikelihoodQ",pmt_charge_likelihood);
	classification_map_double.emplace("PMTLikelihoodT",pmt_time_likelihood);
	classification_map_double.emplace("PMTLikelihoodTheta",pmt_theta_likelihood);
	classification_map_double.emplace("PMTLikelihoodPhi",pmt_phi_likelihood);


	// LAPPD variables
	classification_map_double.emplace("LAPPDBaryTheta",lappdBaryTheta);
	classification_map_double.emplace("LAPPDAvgDist",lappd_avgDist);
	classification_map_double.emplace("LAPPDQtotal",lappd_totalQ);
	classification_map_double.emplace("LAPPDAvgT",lappd_avgT);
	classification_map_double.emplace("LAPPDVarT",lappd_varT);
	classification_map_int.emplace("LAPPDHits",lappd_hits);
	classification_map_double.emplace("LAPPDRMSTheta",lappd_rmsTheta);
	classification_map_double.emplace("LAPPDVarTheta",lappd_varTheta);
	classification_map_double.emplace("LAPPDRMSThetaBary",lappd_rmsThetaBary);
	classification_map_double.emplace("LAPPDVarThetaBary",lappd_varThetaBary);


	// PMT & LAPPD vector variables
	classification_map_vector.emplace("PMTQVector",pmtQ);
	classification_map_vector.emplace("PMTTVector",pmtT);
	classification_map_vector.emplace("PMTDistVector",pmtDist);
	classification_map_vector.emplace("PMTThetaVector",pmtTheta);
	classification_map_vector.emplace("PMTThetaBaryVector",pmtThetaBary);
	classification_map_vector.emplace("PMTPhiVector",pmtPhi);
	classification_map_vector.emplace("PMTPhiBaryVector",pmtPhiBary);
	classification_map_vector.emplace("PMTYVector",pmtY);
	classification_map_vector.emplace("LAPPDQVector",lappdQ);
	classification_map_vector.emplace("LAPPDTVector",lappdT);
	classification_map_vector.emplace("LAPPDDistVector",lappdDist);
	classification_map_vector.emplace("LAPPDThetaVector",lappdTheta);
	classification_map_vector.emplace("LAPPDThetaBaryVector",lappdThetaBary);

	// PMT & LAPPD mctruth variables
	if (!isData){

		classification_map_double.emplace("MCPMTFracRing",pmt_fracRing);
		classification_map_double.emplace("MCPMTFracRingNoWeight",pmt_fracRingNoWeight);
		classification_map_double.emplace("MCLAPPDFracRing",lappd_fracRing);
	
		classification_map_double.emplace("MCPMTBaryTheta",pmtBaryThetaMC);
		classification_map_double.emplace("MCPMTVarTheta",pmt_varThetaMC);
		classification_map_double.emplace("MCPMTRMSTheta",pmt_rmsThetaMC);
		classification_map_double.emplace("MCPMTRMSThetaBary",pmt_rmsThetaBaryMC);
		classification_map_double.emplace("MCPMTVarThetaBary",pmt_varThetaBaryMC);
		classification_map_vector.emplace("MCPMTThetaBaryVector",pmtThetaBaryMC);
                       
                classification_map_double.emplace("MCLAPPDBaryTheta",lappdBaryThetaMC);
		classification_map_double.emplace("MCLAPPDVarTheta",lappd_varThetaMC);
		classification_map_double.emplace("MCLAPPDRMSTheta",lappd_rmsThetaMC);
                classification_map_double.emplace("MCLAPPDRMSThetaBary",lappd_rmsThetaBaryMC);
                classification_map_double.emplace("MCLAPPDVarThetaBary",lappd_varThetaBaryMC);
		classification_map_vector.emplace("MCLAPPDThetaBaryVector",lappdThetaBaryMC);
		
		classification_map_vector.emplace("MCPMTTVectorTOF",pmtT_tof);
		classification_map_vector.emplace("MCLAPPDTVectorTOF",lappdT_tof);
	}

}


void CalcClassificationVars::ClassificationVarsMRD(){

	Log("CalcClassificationVars tool: Reading out MRD data",v_message,verbosity);

	int num_mrd_paddles=0;
	int num_mrd_layers=0;
	int num_mrd_conslayers=0;
	int num_mrd_adjacent=0;
	double mrd_mean_xspread = 0.;
	double mrd_mean_yspread = 0.;
	double mrd_padperlayer = 0.;
	bool layer_occupied[11] = {0};
	double mrd_paddlesize[11];

	if(!TDCData){
		Log("CalcClassificationVars tool: No TDC data to process!",v_warning,verbosity);
	} else {
		//helper histograms to determine MRD spread in x/y direction
		TH1F *x_layer[11], *y_layer[11];
		for (int i_layer=0;i_layer<11;i_layer++){
			std::stringstream ss_x_layer, ss_y_layer;
			ss_x_layer <<"hist_x_layer"<<i_layer;
			ss_y_layer <<"hist_y_layer"<<i_layer;
			x_layer[i_layer] = new TH1F(ss_x_layer.str().c_str(),ss_x_layer.str().c_str(),100,1,0);
			y_layer[i_layer] = new TH1F(ss_y_layer.str().c_str(),ss_y_layer.str().c_str(),100,1,0);
		}
		if(TDCData->size()==0){
			//No entries in TDCData object, don't read out anything
			Log("CalcClassificationVars tool: No TDC hits.",v_message,verbosity);
		} else {
			std::vector<std::vector<double>> mrd_hits;
			for (int i_layer=0; i_layer<11; i_layer++){
				std::vector<double> empty_hits;
				mrd_hits.push_back(empty_hits);
			}
			std::vector<int> temp_cons_layers;
			for(auto&& anmrdpmt : (*TDCData)){
				unsigned long chankey = anmrdpmt.first;
				Detector *thedetector = geom->ChannelToDetector(chankey);
				if(thedetector->GetDetectorElement()!="MRD") {
					continue;                 // this is a veto hit, not an MRD hit.
				}
				num_mrd_paddles++;
				int detkey = thedetector->GetDetectorID();
				Paddle *apaddle = geom->GetDetectorPaddle(detkey);
				int layer = apaddle->GetLayer();
				layer_occupied[layer-1]=true;
				if (apaddle->GetOrientation()==1) {
					x_layer[layer-2]->Fill(0.5*(apaddle->GetXmin()+apaddle->GetXmax()));
					mrd_hits.at(layer-2).push_back(0.5*(apaddle->GetXmin()+apaddle->GetXmax()));
					mrd_paddlesize[layer-2]=apaddle->GetPaddleWidth();
				}
				else if (apaddle->GetOrientation()==0) {
					y_layer[layer-2]->Fill(0.5*(apaddle->GetYmin()+apaddle->GetYmax()));
					mrd_hits.at(layer-2).push_back(0.5*(apaddle->GetYmin()+apaddle->GetYmax()));
					mrd_paddlesize[layer-2]=apaddle->GetPaddleWidth();
				}
			}
			if (num_mrd_paddles > 0) {
				for (int i_layer=0;i_layer<11;i_layer++){
					if (layer_occupied[i_layer]==true) {
						num_mrd_layers++;
						if (num_mrd_conslayers==0) num_mrd_conslayers++;
						else {
							if (layer_occupied[i_layer-1]==true) num_mrd_conslayers++;
							else {
								temp_cons_layers.push_back(num_mrd_conslayers);
								num_mrd_conslayers=0;
							}
						}
					}
					for (unsigned int i_hitpaddle=0; i_hitpaddle<mrd_hits.at(i_layer).size(); i_hitpaddle++){
						for (unsigned int j_hitpaddle= i_hitpaddle+1; j_hitpaddle < mrd_hits.at(i_layer).size(); j_hitpaddle++){
							if (fabs(mrd_hits.at(i_layer).at(i_hitpaddle)-mrd_hits.at(i_layer).at(j_hitpaddle))-mrd_paddlesize[i_layer] < 0.001) num_mrd_adjacent++;
						}
					} 		
				}
			}
			std::vector<int>::iterator it = std::max_element(temp_cons_layers.begin(),temp_cons_layers.end());
			if (it != temp_cons_layers.end()) num_mrd_conslayers = *it;
			else num_mrd_conslayers=0;
			if (num_mrd_layers > 0.) mrd_padperlayer = double(num_mrd_paddles)/num_mrd_layers;
			mrd_mean_xspread=0.;
			mrd_mean_yspread=0.;
			int num_xspread=0;
			int num_yspread=0;
			for (int i_layer=0; i_layer < 11; i_layer++){
				if (x_layer[i_layer]->GetEntries()>0){
					mrd_mean_xspread+=(x_layer[i_layer]->GetRMS());
					num_xspread++;
				}
				if (y_layer[i_layer]->GetEntries()>0){
					mrd_mean_yspread+=(y_layer[i_layer]->GetRMS());
					num_yspread++;	
				}
			}
			if (num_xspread>0) mrd_mean_xspread/=num_xspread;
			if (num_yspread>0) mrd_mean_yspread/=num_yspread;
		}
		for (int i_layer = 0; i_layer < 11; i_layer++){
			delete x_layer[i_layer];
			delete y_layer[i_layer];
		}
	}


	classification_map_int.emplace("MrdLayers",num_mrd_layers);
	classification_map_int.emplace("MrdPaddles",num_mrd_paddles);
	classification_map_int.emplace("MrdConsLayers",num_mrd_conslayers);
	classification_map_int.emplace("MrdAdjHits",num_mrd_adjacent);
	classification_map_double.emplace("MrdPadPerLayer",mrd_padperlayer);
	classification_map_double.emplace("MrdXSpread",mrd_mean_xspread);
	classification_map_double.emplace("MrdYSpread",mrd_mean_yspread);
	classification_map_int.emplace("MrdClusters",NumMrdTimeClusters);

}

void CalcClassificationVars::StorePionEnergies(){

  map_pion_energies.clear();
  int n_neutrons = 0;

  for (unsigned int i_particle = 0; i_particle < mcparticles->size(); i_particle++){

    MCParticle temp_particle = mcparticles->at(i_particle);
    int pdg_code = temp_particle.GetPdgCode();

    if (pdg_code == 2112) {
      n_neutrons++;
      continue;
    }    

    //Only use energies of pions and kaons 
    if (pdg_code != 211 && pdg_code != 221 && pdg_code != -221 && pdg_code != 311 && pdg_code != 321 && pdg_code != -321) continue;

    double cherenkov_threshold = pdgcodetocherenkov[pdg_code];
    double particle_energy = temp_particle.GetStartEnergy();
    
    if (particle_energy > cherenkov_threshold){
      if (map_pion_energies.find(pdg_code) != map_pion_energies.end()){
        map_pion_energies.at(pdg_code).push_back(particle_energy);
      }  
      else {
        std::vector<double> temp_energy_vector{particle_energy};
        map_pion_energies.emplace(pdg_code,temp_energy_vector);
      }
    }   

  }

  m_data->Stores["Classification"]->Set("MCPionEnergies",map_pion_energies);
  classification_map_bool.emplace("MCNeutrons",n_neutrons);

}
