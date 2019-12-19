#include "CalcClassificationVars.h"

CalcClassificationVars::CalcClassificationVars():Tool(){}

bool CalcClassificationVars::Initialise(std::string configfile, DataModel &data){

	//---------------------------------------------------------------
	//----------------- Useful header -------------------------------
	//---------------------------------------------------------------

	if(configfile!="")  m_variables.Initialise(configfile); 
	m_data= &data;

	//---------------------------------------------------------------
	//----------------- Configuration variables ---------------------
	//---------------------------------------------------------------
	
	m_variables.Get("verbosity",verbosity);
	m_variables.Get("UseMCTruth",use_mctruth);

	//---------------------------------------------------------------
	//----------------- Geometry variables --------------------------
	//---------------------------------------------------------------

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

	//----------------------------------------------------------------------
	//-----------read in PMT x/y/z positions into vectors-------------------
	//----------------------------------------------------------------------

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
	
        //----------------------------------------------------------------------
	//-----------Make classification store if it does not exist yet---------
	//----------------------------------------------------------------------
	
	int classstoreexists = m_data->Stores.count("Classification");
	if (classstoreexists==0) m_data->Stores["Classification"] = new BoostStore(false,2);

	return true;
}


bool CalcClassificationVars::Execute(){

	Log("CalcClassificationVars Tool: Executing",v_debug,verbosity);

	//-----------------------------------------------------------------------
	//--------------------- Getting variables from stores -------------------
	//-----------------------------------------------------------------------
	
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
	
	//-----------------------------------------------------------------------
	//------------------- Getting objects from ANNIEEvent -------------------
	//-----------------------------------------------------------------------

	get_ok = m_data->Stores.at("ANNIEEvent")->Get("EventNumber",evnum);
	if(not get_ok){ Log("CalcClassificationVars Tool: Error retrieving EventNumber,true from ANNIEEvent!",v_error,verbosity); return false; }
	get_ok = m_data->Stores.at("ANNIEEvent")->Get("MCEventNum",mcevnum);
	if(not get_ok){ Log("CalcClassificationVars Tool: Error retrieving MCEventNum,true from ANNIEEvent!",v_error,verbosity); return false; }
	get_ok = m_data->Stores.at("ANNIEEvent")->Get("MCHits",MCHits);
	if(not get_ok){ Log("CalcClassificationVars Tool: Error retrieving MCHits,true from ANNIEEvent!",v_error,verbosity); return false; }
	get_ok = m_data->Stores.at("ANNIEEvent")->Get("MCLAPPDHits",MCLAPPDHits);
	if(not get_ok){ Log("CalcClassificationVars Tool: Error retrieving MCLAPPDHits,true from ANNIEEvent!",v_error,verbosity); return false; }
	get_ok = m_data->Stores.at("ANNIEEvent")->Get("TDCData",TDCData);
	if(not get_ok){ Log("CalcClassificationVars Tool: Error retrieving TDCData,true from ANNIEEvent!",v_error,verbosity); return false; }
	get_ok = m_data->Stores.at("ANNIEEvent")->Get("NumMrdTimeClusters",NumMrdTimeClusters);
	if(not get_ok){ Log("CalcClassificationVars Tool: Error retrieving NumMrdTimeClusters,true from ANNIEEvent!",v_error,verbosity); return false; }
	

	//-----------------------------------------------------------------------
	//------------------- Getting objects from RecoEvent -------------------
	//-----------------------------------------------------------------------

	get_ok = m_data->Stores.at("RecoEvent")->Get("TrueVertex",TrueVertex);
	if(not get_ok){ Log("CalcClassificationVars Tool: Error retrieving TrueVertex,true from RecoEvent!",v_error,verbosity); return false; }
	get_ok = m_data->Stores.at("RecoEvent")->Get("TrueStopVertex",TrueStopVertex);
	if(not get_ok){ Log("CalcClassificationVars Tool: Error retrieving TrueStopVertex,true from RecoEvent!",v_error,verbosity); return false; }
	get_ok = m_data->Stores.at("RecoEvent")->Get("EventCutStatus",EventCutStatus);
	if(not get_ok){ Log("CalcClassificationVars Tool: Error retrieving EventCutStatus,true from RecoEvent!",v_error,verbosity); return false; }
	get_ok = m_data->Stores.at("RecoEvent")->Get("RecoDigit",RecoDigits);
	if(not get_ok){ Log("CalcClassificationVars Tool: Error retrieving RecoDigit,true from RecoEvent!",v_error,verbosity); return false; }
	get_ok = m_data->Stores.at("RecoEvent")->Get("TrueMuonEnergy",TrueMuonEnergy);
	if(not get_ok){ Log("CalcClassificationVars Tool: Error retrieving TrueMuonEnergy,true from RecoEvent!",v_error,verbosity); return false; }
	get_ok = m_data->Stores.at("RecoEvent")->Get("NRings",nrings);
	if(not get_ok){ Log("CalcClassificationVars Tool: Error retrieving NRings, true from RecoEvent!",v_error,verbosity); return false; }
	get_ok = m_data->Stores.at("RecoEvent")->Get("NoPiK",no_pik);
	if(not get_ok){ Log("CalcClassificationVars Tool: Error retrieving NoPiK, true from RecoEvent!",v_error,verbosity); return false; }


	//-----------------------------------------------------------------------
	//------------- Getting true info from primary particle -----------------
	//-----------------------------------------------------------------------

	//check first if the cuts were passed
	
	bool sensible_energy = true;
	if (fabs(TrueMuonEnergy+9999)<0.01) sensible_energy = false;
	if (!EventCutStatus || !sensible_energy){
		Log("CalcClassificationVars Tool: EventCutStatus is false! Selected event cuts were not passed...",v_message,verbosity);
	} else {

	//if yes, get the relevant information from the data
	
	//Vertex-related information
	Position pos = TrueVertex->GetPosition();
	pos.UnitToMeter();
	double pos_x = pos.X();
	double pos_y = pos.Y();
	double pos_z = pos.Z();
	Direction dir = TrueVertex->GetDirection();
	double dir_x = dir.X();
	double dir_y = dir.Y();
	double dir_z = dir.Z();
	double distWallVert = fabs(1.98-pos_y);
	double distWallHor = 1.524 - sqrt(pos_x*pos_x+pos_z*pos_z);
	distWallVert /= 1.98;
	distWallHor /= 1.524;
	double distInnerStrHor = tank_innerR - sqrt(pos_x*pos_x+pos_z*pos_z);
	double distInnerStrVert = (pos_y > 0)? (tank_ymax - pos_y) : (pos_y - tank_ymin);
	double true_time = TrueVertex->GetTime();

	logmessage = "CalcClassificationVars Tool: Interaction Vertex is at ("+to_string(pos_x)
		    +", "+to_string(pos_y)+", "+to_string(pos_z)+")\n"
		    +"CalcClassificationVars Tool: Primary particle has energy "+to_string(TrueMuonEnergy)+"MeV and direction ("
		    +to_string(dir_x)+", "+to_string(dir_y)+", "+to_string(dir_z)+" )";
	Log(logmessage,v_message,verbosity);


	//-----------------------------------------------------------------------
	//--------------- Read out Digits (PMT/LAPPD hits) ----------------------
	//-----------------------------------------------------------------------

	//information available directly from data
	double pmt_QDownstream=0.;
	double pmt_avgT=0.;
	Position pmtBaryQ(0.,0.,0.);
	double pmt_totalQ=0.;
	double pmt_totalQ_Clustered=0.;
	double pmt_highestQ=0.;
	int pmt_hits=0;
	int pmt_hits_late=0;
	int pmt_hits_early=0;
	int pmt_hits_lowq=0;
	Position lappdBaryQ(0.,0.,0.);
	double lappd_totalQ=0.;
	double lappd_avgT=0.;
	int lappd_hits=0;
	std::vector<double> pmtQ, lappdQ, pmtT, lappdT;
	std::vector<Position> pmtPos, lappdPos;

	//information only available when using mctruth information
	double pmt_rmsTheta=0.; 
	double pmt_avgDist=0.;
	double pmt_QRing=0.;
	int pmt_hitsRing=0;
	double lappd_rmsTheta=0.;
	double lappd_avgDist=0.;
	double lappd_QRing=0.;
	int lappd_hitsRing=0;
	std::vector<double> pmtDist, pmtTheta, pmtTheta2, pmtPhi, pmtY, lappdDist, lappdAngle;

	Log("CalcClassificationVars tool: Reading in RecoDigits object of size: "+std::to_string(RecoDigits.size()),v_debug,verbosity);

	for (unsigned int i_digit = 0; i_digit < RecoDigits.size(); i_digit++){

		RecoDigit thisdigit = RecoDigits.at(i_digit);
		Position detector_pos = thisdigit.GetPosition();
		detector_pos.UnitToMeter();
		Direction detector_dir(detector_pos.X()-pos_x,detector_pos.Y()-pos_y,detector_pos.Z()-pos_z);
		double detector_dirX = detector_pos.X()-pos_x;
		double detector_dirY = detector_pos.Y()-pos_y;
		double detector_dirZ = detector_pos.Z()-pos_z;
		int digittype = thisdigit.GetDigitType();		//0 - PMTs, 1 - LAPPDs	
		double digitQ = thisdigit.GetCalCharge();
		double digitT = thisdigit.GetCalTime();
		double detDist, detTheta, detTheta2, detPhi;
		if (use_mctruth){	
			//do calculations with respect to interaction vertex
			detDist = sqrt(pow(detector_pos.X()-pos_x,2)+pow(detector_pos.Y()-pos_y,2)+pow(detector_pos.Z()-pos_z,2));
			detTheta = acos((detector_dirX*dir_x+detector_dirY*dir_y+detector_dirZ*dir_z)/detDist);
		} else {
			//otherwise calculations in relation to the center of the tank
			detDist = sqrt(pow(detector_pos.X(),2)+pow(detector_pos.Y(),2)+pow(detector_pos.Z(),2));
			detTheta = acos((detector_pos.Z()*1.)/detDist);
			if (fabs(detector_dirX) < 0.001 || fabs(detector_dirY) < 0.001)  detTheta2 = 0.;
			if (detector_dirX > 0. && detector_dirY > 0. ) detTheta2 = acos(detector_dirY/detector_dirX);
			if (detector_dirX < 0. && detector_dirY > 0. ) detTheta2 = TMath::Pi()/2.+acos(-detector_dirX/detector_dirY);
			if (detector_dirX < 0. && detector_dirY < 0. ) detTheta2 = TMath::Pi()+acos(detector_dirY/detector_dirX);
			if (detector_dirX > 0. && detector_dirY < 0. ) detTheta2 = 3./2*TMath::Pi()+acos(detector_dirX/-detector_dirY);
	                if (detector_dirX > 0. && detector_dirZ > 0. ) detPhi = atan(detector_dirZ/detector_dirX)+TMath::Pi()/2.;
                        if (detector_dirX > 0. && detector_dirZ < 0. ) detPhi = atan(detector_dirX/-detector_dirZ);
                        if (detector_dirX < 0. && detector_dirZ < 0. ) detPhi = 3*TMath::Pi()/2.+atan(detector_dirZ/detector_dirX);
                        if (detector_dirX < 0. && detector_dirZ > 0. ) detPhi = TMath::Pi()+atan(-detector_dirX/detector_dirZ);			
		}
		pmt_avgDist+=detDist;

		if (digittype == 0){

			hist_pmtPE->Fill(digitQ);
			hist_pmtTime->Fill(digitT);
			hist_pmtTheta->Fill(detTheta);
			hist_pmtDist->Fill(detDist);
			hist_pmtTheta2->Fill(detTheta2);
			hist_pmtPhi->Fill(detPhi);
			hist_pmtY->Fill(detector_dirY);
			pmtQ.push_back(digitQ);
			pmtT.push_back(digitT);
			pmtPos.push_back(detector_pos);
			pmt_totalQ+=digitQ;
			if (thisdigit.GetFilterStatus() == 1) pmt_totalQ_Clustered+=digitQ;
			pmt_avgT+=digitT;
			pmt_hits++;
			if (digitQ < 30) pmt_hits_lowq++;
			if (digitT > 10) pmt_hits_late++;	
			else if (digitT < 4) pmt_hits_early++;
			if (digitQ > pmt_highestQ) pmt_highestQ = digitQ;
			if (detTheta < TMath::Pi()/2.) pmt_QDownstream+=digitQ;
			pmt_rmsTheta+=(detTheta*detTheta);
			pmtDist.push_back(detDist);
			pmtTheta.push_back(detTheta);
			pmtTheta2.push_back(detTheta2);
			pmtPhi.push_back(detPhi);
			pmtY.push_back(detector_dirY);
                        pmtBaryQ += digitQ*detector_pos;

			if (use_mctruth){
                        	if (detTheta < cherenkov_angle) {
                                	pmt_QRing+=digitQ;
                                	pmt_hitsRing++;
                        	}
			}


		} else if (digittype == 1){

			hist_lappdPE->Fill(digitQ);
			hist_lappdTime->Fill(digitT);
			hist_lappdAngle->Fill(detTheta);
			hist_lappdDist->Fill(detDist);
			lappdQ.push_back(digitQ);
			lappdT.push_back(digitT);
			lappdPos.push_back(detector_pos);
			lappd_totalQ+=digitQ;
			lappd_hits++;
			lappd_avgT+=digitT;
			lappd_rmsTheta+=(detTheta*detTheta);
			lappdDist.push_back(detDist);
			lappdAngle.push_back(detTheta);
			lappdBaryQ += digitQ*detector_pos;

			if (use_mctruth){
				if (detTheta < cherenkov_angle) {
					lappd_QRing+=digitQ;
					lappd_hitsRing++;
				}
			}

		} else {
			
			std::string logmessage = "Digit type " + std::to_string(digittype) + "was not recognized. Omit reading in entry from RecoEvent store.";
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
	double lappd_fracRingNoWeight=0.;

	if (pmtQ.size()!=0) {

		pmt_avgT /= pmtQ.size();
		pmt_avgDist /= pmtQ.size();
		pmt_rmsTheta = sqrt(pmt_rmsTheta/pmtQ.size());
		pmtBaryQ = (1./pmt_totalQ)*pmtBaryQ;
		m_data->CStore.Set("pmtBaryQ",pmtBaryQ);
		pmt_fracQDownstream = pmt_QDownstream/pmt_totalQ;
		pmt_frachighestQ = pmt_highestQ/pmt_totalQ;
		if (verbosity > v_debug) std::cout <<"pmt_totalQ_Clustered = "<<pmt_totalQ_Clustered<<", pmt_totalQ = "<<pmt_totalQ;
		pmt_fracClustered = pmt_totalQ_Clustered/pmt_totalQ;
		if (verbosity > v_debug) std::cout <<", pmt_fracClustered = "<<pmt_fracClustered<<std::endl;
		pmt_fracLowQ = double(pmt_hits_lowq)/pmt_hits;
		pmt_fracLate = double(pmt_hits_late)/pmt_hits;		
		pmt_fracEarly = double(pmt_hits_early)/pmt_hits;

		if (use_mctruth){
			pmt_fracRing = pmt_QRing/pmt_totalQ;
			pmt_fracRingNoWeight = double(pmt_hitsRing)/pmt_hits;
		}
	}
	if (lappdQ.size()!=0){
		
		lappd_avgT /= lappdQ.size();
		lappd_avgDist /= lappdQ.size();
		lappd_rmsTheta = sqrt(lappd_rmsTheta/lappdQ.size());
		lappdBaryQ = (1./lappd_totalQ)*lappdBaryQ;
		if (use_mctruth){
			lappd_fracRing = lappd_QRing/lappd_totalQ;
			lappd_fracRingNoWeight = double(lappd_hitsRing)/lappd_hits;
		}
	}

	double pmtBaryDist, pmtBaryAngle, pmtBaryAngle2, pmtBaryY, pmtBaryPhi, lappdBaryDist, lappdBaryAngle;

	if (use_mctruth){
		//angle and distance of barycenter calculated with respect to interaction point and primary particle direction
		Position dirBaryQ = pmtBaryQ-pos;
		pmtBaryDist = dirBaryQ.Mag();
		pmtBaryAngle = acos((dirBaryQ.X()*dir_x+dirBaryQ.Y()*dir_y+dirBaryQ.Z()*dir_z)/pmtBaryDist);
		Position lappd_dirBaryQ = lappdBaryQ-pos;
		lappdBaryDist = lappd_dirBaryQ.Mag();
		lappdBaryAngle = acos((lappd_dirBaryQ.X()*dir_x+lappd_dirBaryQ.Y()*dir_y+lappd_dirBaryQ.Z()*dir_z)/lappdBaryDist);
	} else {
		//angle and distance of barycenter calculated with respect to (0,0,0) and (0,0,1)-direction
		pmtBaryDist = pmtBaryQ.Mag();
		if (fabs(pmtBaryDist)<0.0001) {
			pmtBaryAngle = 0;
			pmtBaryAngle2 = 0.;
			pmtBaryPhi = 0.;
			pmtBaryY = 0.;
		}
		else {
			pmtBaryAngle = acos(pmtBaryQ.Z()/pmtBaryDist);
                        if (pmtBaryQ.X() > 0. && pmtBaryQ.Y() > 0. ) pmtBaryAngle2 = atan(pmtBaryQ.Y()/pmtBaryQ.X());
                        if (pmtBaryQ.X() < 0. && pmtBaryQ.Y() > 0. ) pmtBaryAngle2 = TMath::Pi()/2.+atan(-pmtBaryQ.X()/pmtBaryQ.Y());
                        if (pmtBaryQ.X() < 0. && pmtBaryQ.Y() < 0. ) pmtBaryAngle2 = TMath::Pi()+atan(pmtBaryQ.Y()/pmtBaryQ.X());
                        if (pmtBaryQ.X() > 0. && pmtBaryQ.Y() < 0. ) pmtBaryAngle2 = 3./2*TMath::Pi()+atan(pmtBaryQ.X()/-pmtBaryQ.Y());
                        if (pmtBaryQ.X() > 0. && pmtBaryQ.Z() > 0. ) pmtBaryPhi = atan(pmtBaryQ.Z()/pmtBaryQ.X())+TMath::Pi()/2.;
                        if (pmtBaryQ.X() > 0. && pmtBaryQ.Z() < 0. ) pmtBaryPhi = atan(pmtBaryQ.X()/-pmtBaryQ.Z());
                        if (pmtBaryQ.X() < 0. && pmtBaryQ.Z() < 0. ) pmtBaryPhi = 3*TMath::Pi()/2.+atan(pmtBaryQ.Z()/pmtBaryQ.X());
                        if (pmtBaryQ.X() < 0. && pmtBaryQ.Z() > 0. ) pmtBaryPhi = TMath::Pi()+atan(-pmtBaryQ.X()/pmtBaryQ.Z());
			pmtBaryY = pmtBaryQ.Y();
		}
		lappdBaryDist = lappdBaryQ.Mag();
		if (fabs(lappdBaryDist)<0.001) lappdBaryAngle=0.;
		else lappdBaryAngle = acos(lappdBaryQ.Z()/lappdBaryDist);
	}


	double pmt_varTheta=0.;
	double pmt_skewTheta=0.;
	double pmt_kurtTheta=0.;
	double lappd_varTheta=0.;
	double lappd_skewTheta=0.;
	double lappd_kurtTheta=0.;
	double pmt_rmsThetaBary=0.;
	double pmt_varThetaBary=0.;
	double pmt_skewThetaBary=0.;
	double pmt_kurtThetaBary=0.;
	double lappd_rmsThetaBary=0.;
	double lappd_varThetaBary=0.;
	double lappd_skewThetaBary=0.;
	double lappd_kurtThetaBary=0.;
	double pmt_rmsPhiBary = 0.;
	double pmt_varPhiBary = 0.;
	double pmt_fracLargeAnglePhi=0.;
	double pmt_fracLargeAngle=0.;
	int pmt_hits_largeangle=0;
	int pmt_hits_largeangle_phi=0;	

	//std::cout <<"CalcClassificationVars tool: Calculate variance/skewness/etc for csv file: "<<std::endl;

	for (unsigned int i_pmt=0; i_pmt < pmtQ.size(); i_pmt++){
		pmt_varTheta+=(pow(pmtTheta.at(i_pmt),2)*pmtQ.at(i_pmt)/pmt_totalQ);
		pmt_skewTheta+=(pow(pmtTheta.at(i_pmt),3)*pmtQ.at(i_pmt)/pmt_totalQ);
		pmt_kurtTheta+=(pow(pmtTheta.at(i_pmt),4)*pmtQ.at(i_pmt)/pmt_totalQ);
		hist_pmtThetaBary_all->Fill(pmtTheta.at(i_pmt)-pmtBaryAngle);
		hist_pmtThetaBary_all_ChargeWeighted->Fill((pmtTheta.at(i_pmt)-pmtBaryAngle),pmtQ.at(i_pmt)/pmt_totalQ);
		double diff_angle2 = (pmtTheta2.at(i_pmt)-pmtBaryAngle2);
		if (diff_angle2 > TMath::Pi()) diff_angle2 = -(2*TMath::Pi()-diff_angle2);
		else if (diff_angle2 < -TMath::Pi()) diff_angle2 = 2*TMath::Pi()+diff_angle2;
		hist_pmtTheta2Bary_all->Fill(diff_angle2);
		hist_pmtTheta2Bary_all_ChargeWeighted->Fill(diff_angle2,pmtQ.at(i_pmt)/pmt_totalQ);
		double diff_phi = (pmtPhi.at(i_pmt)-pmtBaryPhi);
		if (diff_phi > TMath::Pi()) diff_phi = -(2*TMath::Pi()-diff_phi);
		else if (diff_phi < -TMath::Pi()) diff_phi = 2*TMath::Pi()+diff_phi;
		hist_pmtPhiBary_all->Fill(diff_phi);
		hist_pmtPhiBary_all_ChargeWeighted->Fill(diff_phi,pmtQ.at(i_pmt)/pmt_totalQ);
		hist_pmtYBary_all->Fill(pmtY.at(i_pmt)-pmtBaryY);
		hist_pmtYBary_all_ChargeWeighted->Fill(pmtY.at(i_pmt)-pmtBaryY,pmtQ.at(i_pmt));
		pmt_rmsThetaBary+=pow(pmtTheta.at(i_pmt)-pmtBaryAngle,2);
		pmt_varThetaBary+=(pow(pmtTheta.at(i_pmt)-pmtBaryAngle,2)*pmtQ.at(i_pmt)/pmt_totalQ);
		pmt_skewThetaBary+=((pow(pmtTheta.at(i_pmt)-pmtBaryAngle,3)*pmtQ.at(i_pmt)/pmt_totalQ));
		pmt_kurtThetaBary+=((pow(pmtTheta.at(i_pmt)-pmtBaryAngle,4)*pmtQ.at(i_pmt)/pmt_totalQ));
		if (fabs(pmtTheta.at(i_pmt)-pmtBaryAngle) > 0.9) pmt_hits_largeangle++;
		if (fabs(diff_phi) > 1.) pmt_hits_largeangle_phi++;
		pmt_rmsPhiBary+=(pow(diff_phi,2));
		pmt_varPhiBary+=(pow(diff_phi,2)*pmtQ.at(i_pmt)/pmt_totalQ);
	}
	pmt_kurtTheta-=(3*pmt_varTheta*pmt_varTheta);
      	pmt_kurtThetaBary-=(3*pmt_varThetaBary*pmt_varThetaBary);
	
	if (pmtQ.size()>0) {
		pmt_rmsThetaBary = sqrt(pmt_rmsThetaBary/pmtQ.size());
		pmt_rmsPhiBary = sqrt(pmt_rmsPhiBary/pmtQ.size());
		pmt_fracLargeAngle = double(pmt_hits_largeangle)/pmtQ.size();
		pmt_fracLargeAnglePhi = double(pmt_hits_largeangle_phi)/pmtQ.size();	
	}	

	for (unsigned int i_lappd=0; i_lappd< lappdQ.size(); i_lappd++){
		lappd_varTheta+=(pow(lappdAngle.at(i_lappd),2)*lappdQ.at(i_lappd)/lappd_totalQ);
		lappd_skewTheta+=(pow(lappdAngle.at(i_lappd),3)*lappdQ.at(i_lappd)/lappd_totalQ);
		lappd_kurtTheta+=(pow(lappdAngle.at(i_lappd),4)*lappdQ.at(i_lappd)/lappd_totalQ);
		lappd_rmsThetaBary+=pow(lappdAngle.at(i_lappd)-lappdBaryAngle,2);
		lappd_varThetaBary+=(pow(lappdAngle.at(i_lappd)-lappdBaryAngle,2)*lappdQ.at(i_lappd)/lappd_totalQ);
		lappd_skewThetaBary+=((pow(lappdAngle.at(i_lappd)-lappdBaryAngle,3)*lappdQ.at(i_lappd)/lappd_totalQ));
		lappd_kurtThetaBary+=((pow(lappdAngle.at(i_lappd)-lappdBaryAngle,4)*lappdQ.at(i_lappd)/lappd_totalQ));
	}
	lappd_kurtTheta-=(3*lappd_varTheta*lappd_varTheta);
	lappd_kurtThetaBary-=(3*lappd_varThetaBary*lappd_varThetaBary);
	if (lappdQ.size()>0) lappd_rmsThetaBary = sqrt(lappd_rmsThetaBary/lappdQ.size());


        //-----------------------------------------------------------------------
        //----------------------- Read out MC - MRD hits ------------------------
        //-----------------------------------------------------------------------	


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
    			mrd_padperlayer = double(num_mrd_paddles)/num_mrd_layers;
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
	

        //-----------------------------------------------------------------------
        //----------------------- Setting store variables -----------------------
        //-----------------------------------------------------------------------   

	//General variables
	m_data->Stores["Classification"]->Set("MC-based",use_mctruth);
        
	//PMT variables
	m_data->Stores["Classification"]->Set("PMTBary",pmtBaryQ);
	m_data->Stores["Classification"]->Set("PMTAvgDist",pmt_avgDist);
	m_data->Stores["Classification"]->Set("PMTAvgT",pmt_avgT);
	m_data->Stores["Classification"]->Set("PMTQtotal",pmt_totalQ);
	m_data->Stores["Classification"]->Set("PMTQtotalClustered",pmt_totalQ_Clustered);
	m_data->Stores["Classification"]->Set("PMTHits",pmt_hits);
	m_data->Stores["Classification"]->Set("PMTFracQmax",pmt_frachighestQ);
	m_data->Stores["Classification"]->Set("PMTFracQdownstream",pmt_fracQDownstream);
	m_data->Stores["Classification"]->Set("PMTFracClustered",pmt_fracClustered);
	m_data->Stores["Classification"]->Set("PMTFracLowQ",pmt_fracLowQ);
	m_data->Stores["Classification"]->Set("PMTFracEarly",pmt_fracEarly);
	m_data->Stores["Classification"]->Set("PMTFracLate",pmt_fracLate);
	m_data->Stores["Classification"]->Set("PMTFracRing",pmt_fracRing);
	m_data->Stores["Classification"]->Set("PMTFracRingNoWeight",pmt_fracRingNoWeight);
	m_data->Stores["Classification"]->Set("PMTRMSTheta",pmt_rmsTheta);
	m_data->Stores["Classification"]->Set("PMTVarTheta",pmt_varTheta);
        m_data->Stores["Classification"]->Set("PMTSkewTheta",pmt_skewTheta);
        m_data->Stores["Classification"]->Set("PMTKurtTheta",pmt_kurtTheta);
        m_data->Stores["Classification"]->Set("PMTRMSThetaBary",pmt_rmsThetaBary);
        m_data->Stores["Classification"]->Set("PMTVarThetaBary",pmt_varThetaBary);
        m_data->Stores["Classification"]->Set("PMTSkewThetaBary",pmt_skewThetaBary);
        m_data->Stores["Classification"]->Set("PMTKurtThetaBary",pmt_kurtThetaBary);
        m_data->Stores["Classification"]->Set("PMTRMSPhiBary",pmt_rmsPhiBary);
        m_data->Stores["Classification"]->Set("PMTVarPhiBary",pmt_varPhiBary);
        m_data->Stores["Classification"]->Set("PMTFracLargeAnglePhi",pmt_fracLargeAnglePhi);
        m_data->Stores["Classification"]->Set("PMTFracLargeAngleTheta",pmt_fracLargeAngle);
        m_data->Stores["Classification"]->Set("PMTHitsLargeAngleTheta",pmt_hits_largeangle);
        m_data->Stores["Classification"]->Set("PMTHitsLargeAnglePhi",pmt_hits_largeangle_phi);

	//LAPPD variables	
	m_data->Stores["Classification"]->Set("LAPPDBary",lappdBaryQ);
	m_data->Stores["Classification"]->Set("LAPPDAvgDistance",lappdAvgDistance);
	m_data->Stores["Classification"]->Set("LAPPDQtotal",lappd_totalQ);
	m_data->Stores["Classification"]->Set("LAPPDAvgT",lappd_avgT);
	m_data->Stores["Classification"]->Set("LAPPDHits",lappd_hits);
	m_data->Stores["Classification"]->Set("LAPPDFracRing",lappd_fracRing);
	m_data->Stores["Classification"]->Set("LAPPDFracRingNoWeight",lappd_fracRingNoWeight);
        m_data->Stores["Classification"]->Set("LAPPDRMSTheta",lappd_rmsTheta);
        m_data->Stores["Classification"]->Set("LAPPDVarTheta",lappd_varTheta);
        m_data->Stores["Classification"]->Set("LAPPDSkewTheta",lappd_skewTheta);
        m_data->Stores["Classification"]->Set("LAPPDKurtTheta",lappd_kurtTheta);
        m_data->Stores["Classification"]->Set("LAPPDRMSThetaBary",lappd_rmsThetaBary);
        m_data->Stores["Classification"]->Set("LAPPDVarThetaBary",lappd_varThetaBary);
        m_data->Stores["Classification"]->Set("LAPPDSkewThetaBary",lappd_skewThetaBary);
        m_data->Stores["Classification"]->Set("LAPPDKurtThetaBary",lappd_kurtThetaBary);

	//MRD variables
	m_data->Stores["Classification"]->Set("MrdLayers",num_mrd_layers);
	m_data->Stores["Classification"]->Set("MrdPaddles",num_mrd_paddles);
	m_data->Stores["Classification"]->Set("MrdConsLayers",num_mrd_conslayers);
	m_data->Stores["Classification"]->Set("MrdAdjHits",num_mrd_adjacent);
	m_data->Stores["Classification"]->Set("MrdPadPerLayer",mrd_padperlayer);
	m_data->Stores["Classification"]->Set("MrdXSpread",mrd_mean_xspread);
	m_data->Stores["Classification"]->Set("MrdYSpread",mrd_mean_yspread);
	m_data->Stores["Classification"]->Set("NumMrdTimeClusters",NumMrdTimeClusters);
	
	Log("CalcClassificationVars tool: Execution step finished",v_message,verbosity);

	return true;

}


bool CalcClassificationVars::Finalise(){

  
	Log("CalcClassificationVars Tool: Finalisation complete",v_message,verbosity);

	return true;

}

