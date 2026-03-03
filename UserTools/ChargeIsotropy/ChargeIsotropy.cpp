#include "ChargeIsotropy.h"

ChargeIsotropy::ChargeIsotropy():Tool(){}


bool ChargeIsotropy::Initialise(std::string configfile, DataModel &data){

  /////////////////// Useful header ///////////////////////
  if(configfile!="") m_variables.Initialise(configfile); // loading config file
  //m_variables.Print();

  m_data= &data; //assigning transient data pointer
  /////////////////////////////////////////////////////////////////

  m_variables.Get("verbosity",verbosity);
  m_variables.Get("time_window",time_window);
  m_variables.Get("IsData",IsData);

  return true;
}


bool ChargeIsotropy::Execute(){
    std::map<unsigned long, std::vector<Hit>>* Hits = nullptr;
    std::map<unsigned long, std::vector<MCHit>>* MCHits = nullptr;
    bool got_hits = false;

    if (isData) {
        got_hits = m_data->Stores["ANNIEEvent"]->Get("Hits", Hits);
    } else {
        if (MCWaveform) {
            got_hits = m_data->Stores["ANNIEEvent"]->Get("Hits", Hits);
        } else {
            got_hits = m_data->Stores["ANNIEEvent"]->Get("MCHits", MCHits);
        }
    }

    if (!got_hits) {
        std::cout << "No Hits store in ANNIEEvent. Continuing to build tree." << std::endl;
        return;
    }

    Position detector_center = geom->GetTankCentre();
    double tank_center_x = detector_center.X();
    double tank_center_y = detector_center.Y();
    double tank_center_z = detector_center.Z();
    fNHits = 0;

    bool loop_tank = true;
    int hits_size = (isData || MCWaveform) ? Hits->size() : MCHits->size();
    if (hits_size == 0) loop_tank = false;

    auto it_tank_data = (isData || MCWaveform) ? Hits->begin() : std::map<unsigned long, std::vector<Hit>>::iterator();
    auto it_tank_mc = (!isData && !MCWaveform) ? MCHits->begin() : std::map<unsigned long, std::vector<MCHit>>::iterator();

    while (loop_tank) {
        unsigned long channel_key = (isData || MCWaveform) ? it_tank_data->first : it_tank_mc->first;
        Detector* this_detector = geom->ChannelToDetector(channel_key);
        Position det_position = this_detector->GetDetectorPosition();
        unsigned long detkey = this_detector->GetDetectorID();
        unsigned long channel_key_data = channel_key;

        if (!isData && !MCWaveform) {
            int wcsimid = channelkey_to_pmtid.at(channel_key);
            channel_key_data = pmtid_to_channelkey[wcsimid];
        }

		bool SPE_available = false;

        if (ApplyDeadMask && this_detector->GetStatus() == detectorstatus::OFF) {
            goto skip_channel;  // do not save the hits information for a Dead PMT (if the mask is on), jump to skip_channel
        }

        SPE_available = (isData || MCWaveform) ? 
                             (ChannelKeyToSPEMap.find(channel_key) != ChannelKeyToSPEMap.end()) : 
                             (ChannelKeyToSPEMap.find(channel_key_data) != ChannelKeyToSPEMap.end());

        if (SPE_available) {
            if (isData || MCWaveform) {
                std::vector<Hit> ThisPMTHits = it_tank_data->second;
                fNHits += ThisPMTHits.size();
                for (Hit &ahit : ThisPMTHits) {
                    double hit_charge = ahit.GetCharge();
                    double hit_PE = hit_charge / ChannelKeyToSPEMap.at(channel_key);
                    fHitX.push_back(det_position.X() - tank_center_x);
                    fHitY.push_back(det_position.Y() - tank_center_y);
                    fHitZ.push_back(det_position.Z() - tank_center_z);
                    fHitT.push_back(ahit.GetTime());
                    fHitQ.push_back(hit_charge);
                }
            } else {
                std::vector<MCHit> ThisPMTHits = it_tank_mc->second;
                fNHits += ThisPMTHits.size();
                for (MCHit &ahit : ThisPMTHits) {
                    double hit_PE = ahit.GetCharge();
                    double hit_charge = hit_PE * ChannelKeyToSPEMap.at(channel_key_data);
                    fHitX.push_back(det_position.X() - tank_center_x);
                    fHitY.push_back(det_position.Y() - tank_center_y);
                    fHitZ.push_back(det_position.Z() - tank_center_z);
                    fHitT.push_back(ahit.GetTime());
                    fHitQ.push_back(hit_charge);
                }
            }
        }

        skip_channel:   // skip the block above if the PMT is dead, advance the iterator
        if (isData || MCWaveform) {
            it_tank_data++;
            if (it_tank_data == Hits->end()) loop_tank = false;
        } else {
            it_tank_mc++;
            if (it_tank_mc == MCHits->end()) loop_tank = false;
        }
    }
 

        //insert variables here
	vector<double>* hitT = new vector<double>();
	vector<double>* hitX = new vector<double>();
	vector<double>* hitY = new vector<double>();
	vector<double>* hitZ = new vector<double>();
	vector<double>* hitQ = new vector<double>();

	double Qij;

        double simplevtxx, simplevtxy, simplevtxz; 

        //Set branch addresses
        tTrig->SetBranchAddress("hitT",&hitT);
        tTrig->SetBranchAddress("hitX",&hitX);
        tTrig->SetBranchAddress("hitY",&hitY);
        tTrig->SetBranchAddress("hitZ",&hitZ);
        tTrig->SetBranchAddress("hitQ",&hitQ);
        tTrig->SetBranchAddress("simpleRecoVtxX",&simplevtxx);
        tTrig->SetBranchAddress("simpleRecoVtxY",&simplevtxy);
        tTrig->SetBranchAddress("simpleRecoVtxZ",&simplevtxz);

        TBranch *Qij_branch = tTrig->Branch("Qij",&Qij);

	double time_window = 20.0;
		double qval = 0.0;
		double num = 0.0;
		double minT = 99999.0;
		for(int j = 0; j < hitT->size(); ++j){
			if(hitT->at(j) <= 0) continue;
			if(minT > hitT->at(j)) minT = hitT->at(j);
		}
		for(int j = 0; j < hitT->size(); ++j){
			if(hitT->at(j) > (minT+time_window)) continue; //"prompt" cluster cut
			qval += hitQ->at(j);
			double qi = hitQ->at(j);
			for(int k = 0; k < hitT->size(); ++k){
				if(j == k) continue; //skip same pmts
				if(hitT->at(k) > (minT+time_window)) continue; //"prompt" cluster cut
				double qj = hitQ->at(k);
				num += qi*qj*LawOfCosines(simplevtxx,simplevtxy+0.1446,simplevtxz-1.681,hitX->at(j),hitY->at(j),hitZ->at(j),hitX->at(k),hitY->at(k),hitZ->at(k));
			}
		}

		Qij = num/qval;


  return true;
}


bool ChargeIsotropy::Finalise(){

  return true;
}

double ChargeIsotrpy::LawOfCosines(double vtxx, double vtxy, double vtxz, double pmt1x, double pmt1y, double pmt1z, double pmt2x, double pmt2y, double pmt2z){
  //vtx to pmt1 - dist^2
  double b = (vtxx-pmt1x)*(vtxx-pmt1x) + (vtxy-pmt1y)*(vtxy-pmt1y) + (vtxz-pmt1z)*(vtxz-pmt1z);
  //vtx to pmt2 - dist^2
  double c = (vtxx-pmt2x)*(vtxx-pmt2x) + (vtxy-pmt2y)*(vtxy-pmt2y) + (vtxz-pmt2z)*(vtxz-pmt2z);
  //pmt1 to pmt2 - dist^2
  double a = (pmt2x-pmt1x)*(pmt2x-pmt1x) + (pmt2y-pmt1y)*(pmt2y-pmt1y) + (pmt2z-pmt1z)*(pmt2z-pmt1z);
  //law of cosines
  return std::acos((b + c - a)/(2*std::sqrt(b*c)));
}


