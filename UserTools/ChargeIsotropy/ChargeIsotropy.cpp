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
  m_variables.Get("IsData",isData);
  m_variables.Get("PMTWaveformSim",MCWaveform);

  auto get_geometry= m_data->Stores.at("ANNIEEvent")->Header->Get("AnnieGeometry",geom);
  if(!get_geometry){
  	Log("ChargeIsotropy Tool: Error retrieving Geometry from ANNIEEvent!",v_error,verbosity); 
  	return false; 
  }

  m_data->CStore.Get("pmt_tubeid_to_channelkey_data",pmtid_to_channelkey);
  m_data->CStore.Get("channelkey_to_pmtid",channelkey_to_pmtid);
  m_data->CStore.Get("ChannelNumToTankPMTSPEChargeMap",ChannelKeyToSPEMap);

  return true;
}


bool ChargeIsotropy::Execute(){
    std::map<unsigned long, std::vector<Hit>>* Hits = nullptr;
    std::map<unsigned long, std::vector<MCHit>>* MCHits = nullptr;
    bool got_hits = false;
    bool got_reco = false;

    if (isData) {
        got_hits = m_data->Stores["ANNIEEvent"]->Get("Hits", Hits);
    } else {
        if (MCWaveform) {
            got_hits = m_data->Stores["ANNIEEvent"]->Get("Hits", Hits);
        } else {
            got_hits = m_data->Stores["ANNIEEvent"]->Get("MCHits", MCHits);
        }
    }

    got_reco = m_data->Stores["RecoEvent"]->Get("SimpleRecoVtx",SimpleRecoVtx);
 
    if (!got_hits) {
        std::cout << "No Hits store in ANNIEEvent. Continuing to build tree." << std::endl;
        return true;
    }
    if (!got_reco) {
        std::cout << "No SimpleRecoVtx store in RecoEvent. Continuing to build tree." << std::endl;
        return true;
    }


    Position detector_center = geom->GetTankCentre();
    double tank_center_x = detector_center.X();
    double tank_center_y = detector_center.Y();
    double tank_center_z = detector_center.Z();
    double recoVtxX = SimpleRecoVtx.X();
    double recoVtxY = SimpleRecoVtx.Y();
    double recoVtxZ = SimpleRecoVtx.Z();
    //If event is not reconstructible, skip
/*    if(recoVtxZ == -9999){
        Qij = -9999;
        m_data->Stores["RecoEvent"]->Set("Qij",Qij);
        return true;
    }*/
    int fNHits = 0;

    bool loop_tank = true;
    int hits_size = (isData || MCWaveform) ? Hits->size() : MCHits->size();
    if (hits_size == 0) loop_tank = false;

    auto it_tank_data = (isData || MCWaveform) ? Hits->begin() : std::map<unsigned long, std::vector<Hit>>::iterator();
    auto it_tank_mc = (!isData && !MCWaveform) ? MCHits->begin() : std::map<unsigned long, std::vector<MCHit>>::iterator();

    //Initialise minimum time [s]
    double minT = 99999.0;	

    //Mask was not turned on for CC Analysis
    bool ApplyDeadMask = false;

    //check get time cluster bounds
    while (loop_tank) {
        unsigned long channel_key = (isData || MCWaveform) ? it_tank_data->first : it_tank_mc->first;
        Detector* this_detector = geom->ChannelToDetector(channel_key);
        unsigned long channel_key_data = channel_key;

        if (!isData && !MCWaveform) {
            int wcsimid = channelkey_to_pmtid.at(channel_key);
            channel_key_data = pmtid_to_channelkey[wcsimid];
        }
	bool SPE_available = false;
        if (ApplyDeadMask && this_detector->GetStatus() == detectorstatus::OFF) {
            goto skip_channel_time;  // do not save the hits information for a Dead PMT (if the mask is on), jump to skip_channel_time
        }
        SPE_available = (isData || MCWaveform) ? 
                             (ChannelKeyToSPEMap.find(channel_key) != ChannelKeyToSPEMap.end()) : 
                             (ChannelKeyToSPEMap.find(channel_key_data) != ChannelKeyToSPEMap.end());

        if (SPE_available) {
            if (isData || MCWaveform) {
                std::vector<Hit> ThisPMTHits = it_tank_data->second;
                for (Hit &ahit : ThisPMTHits) {
                    double time = ahit.GetTime();
                    if(time <= 0) continue;
                    if(minT > time) minT = time;
                }
            } else {
                std::vector<MCHit> ThisPMTHits = it_tank_mc->second;
                for (MCHit &ahit : ThisPMTHits) {
                    double time = ahit.GetTime();
                    if(time <= 0) continue;
                    if(minT > time) minT = time;
                }
            }
        }

        skip_channel_time:   // skip the block above if the PMT is dead, advance the iterator
        if (isData || MCWaveform) {
            it_tank_data++;
            if (it_tank_data == Hits->end()) loop_tank = false;
        } else {
            it_tank_mc++;
            if (it_tank_mc == MCHits->end()) loop_tank = false;
        }
    }

std::cout << "TIME MIN: " << minT << std::endl;
    //reset iterators
    if(isData || MCWaveform) it_tank_data = Hits->begin();
    else it_tank_mc = MCHits->begin();
    loop_tank = true;
    //initialize variables
    double qval = 0.0;
    double num = 0.0;	
    auto it_tank_data2 = (isData || MCWaveform) ? Hits->begin() : std::map<unsigned long, std::vector<Hit>>::iterator();
    auto it_tank_mc2 = (!isData && !MCWaveform) ? MCHits->begin() : std::map<unsigned long, std::vector<MCHit>>::iterator();

    while (loop_tank) {
        unsigned long channel_key = (isData || MCWaveform) ? it_tank_data->first : it_tank_mc->first;
        Detector* this_detector = geom->ChannelToDetector(channel_key);
        Position det_position = this_detector->GetDetectorPosition();
        unsigned long channel_key_data = channel_key;

	bool SPE_available = false;

        if (!isData && !MCWaveform) {
            if (channel_key == 0) break; //Mission failed. We'll get 'em next time.
            int wcsimid = channelkey_to_pmtid.at(channel_key);
            channel_key_data = pmtid_to_channelkey[wcsimid];
        }
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
                    double time = ahit.GetTime();
                    if(time > (minT+time_window)) continue; //"prompt" cluster cut
                    double hit_charge = ahit.GetCharge();
                    double hitX = det_position.X() - tank_center_x;
                    double hitY = det_position.Y() - tank_center_y;
                    double hitZ = det_position.Z() - tank_center_z;
                    qval += hit_charge;
                    double qi = hit_charge;
                    //initialize iterators for second loop
                    it_tank_data2 = Hits->begin();
                    //begin next loop through Tank
                    bool loop_tank2 = true;
                    int fNHits2 = 0;
    while (loop_tank2) {
        unsigned long channel_key2 = it_tank_data2->first;
        Detector* this_detector2 = geom->ChannelToDetector(channel_key2);
        Position det_position2 = this_detector2->GetDetectorPosition();

	bool SPE_available2 = false;

        if(channel_key2 == channel_key) goto skip_channel2data; //skip same PMT
        if (ApplyDeadMask && this_detector2->GetStatus() == detectorstatus::OFF) {
            goto skip_channel2data;  // do not save the hits information for a Dead PMT (if the mask is on), jump to skip_channel2data
        }
        SPE_available2 = (ChannelKeyToSPEMap.find(channel_key2) != ChannelKeyToSPEMap.end());

        if (SPE_available2) {
            std::vector<Hit> ThisPMTHits2 = it_tank_data2->second;
            fNHits2 += ThisPMTHits2.size();
            for (Hit &ahit2 : ThisPMTHits2) {
                double time2 = ahit2.GetTime();
                if(time2 > (minT+time_window)) continue; //"prompt" cluster cut
                double hit_charge2 = ahit2.GetCharge();
                double hitX2 = det_position2.X() - tank_center_x;
                double hitY2 = det_position2.Y() - tank_center_y;
                double hitZ2 = det_position2.Z() - tank_center_z;
                double qj = hit_charge2;
                num += qi*qj*LawOfCosines(recoVtxX, recoVtxY+0.1446, recoVtxZ-1.681, hitX, hitY, hitZ, hitX2, hitY2, hitZ2);
            }
        }

        skip_channel2data:   // skip the block above if the PMT is dead, advance the iterator
            it_tank_data2++;
            if (it_tank_data2 == Hits->end()) loop_tank2 = false;
    }//end second loop for data
                }
            } else {
                std::vector<MCHit> ThisPMTHits = it_tank_mc->second;
                fNHits += ThisPMTHits.size();
                for (MCHit &ahit : ThisPMTHits) {
                    double time = ahit.GetTime();
                    if(time > (minT+time_window)) continue; //"prompt" cluster cut
                    double hit_PE = ahit.GetCharge();
                    double hit_charge = hit_PE * ChannelKeyToSPEMap.at(channel_key_data);
                    double hitX = det_position.X() - tank_center_x;
                    double hitY = det_position.Y() - tank_center_y;
                    double hitZ = det_position.Z() - tank_center_z;
                    qval += hit_charge;
                    double qi = hit_charge;
                    //initialize iterators for second loop
                    it_tank_mc2 = MCHits->begin();
                    //begin next loop through Tank
                    bool loop_tank2 = true;
                    int fNHits2 = 0;
     while (loop_tank2) {
        unsigned long channel_key2 = it_tank_mc2->first;
        Detector* this_detector2 = geom->ChannelToDetector(channel_key2);
        Position det_position2 = this_detector2->GetDetectorPosition();
        unsigned long channel_key_data2 = channel_key2;

        int wcsimid2 = channelkey_to_pmtid.at(channel_key2);
        channel_key_data2 = pmtid_to_channelkey[wcsimid2];
	
	bool SPE_available2 = false;

        if(channel_key2 == channel_key) goto skip_channel2MC; //skip same PMT 
        if (ApplyDeadMask && this_detector2->GetStatus() == detectorstatus::OFF) {
            goto skip_channel2MC;  // do not save the hits information for a Dead PMT (if the mask is on), jump to skip_channel2MC
        }
        SPE_available2 = (ChannelKeyToSPEMap.find(channel_key_data2) != ChannelKeyToSPEMap.end());

        if (SPE_available2) {
            std::vector<MCHit> ThisPMTHits2 = it_tank_mc2->second;
            fNHits2 += ThisPMTHits2.size();
            for (MCHit &ahit2 : ThisPMTHits2) {
                double time2 = ahit2.GetTime();
                if(time2 > (minT+time_window)) continue; //"prompt" cluster cut
                double hit_PE2 = ahit2.GetCharge();
                double hit_charge2 = hit_PE2 * ChannelKeyToSPEMap.at(channel_key_data2);
                double hitX2 = det_position2.X() - tank_center_x;
                double hitY2 = det_position2.Y() - tank_center_y;
                double hitZ2 = det_position2.Z() - tank_center_z;
                double qj = hit_charge2;
                num += qi*qj*LawOfCosines(recoVtxX, recoVtxY+0.1446, recoVtxZ-1.681, hitX, hitY, hitZ, hitX2, hitY2, hitZ2);
            }
        }

        skip_channel2MC:   // skip the block above if the PMT is dead, advance the iterator
            it_tank_mc2++;
            if (it_tank_mc2 == MCHits->end()) loop_tank2 = false;
    }//end second loop for MC
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
    //Charge Isotropy
    if(qval == 0.) Qij = -9999.;
    else Qij = num/qval;

std::cout << "num: " << num << std::endl;
std::cout << "qval: " << qval << std::endl;
    m_data->Stores["RecoEvent"]->Set("Qij",Qij);

    return true;
}


bool ChargeIsotropy::Finalise(){

    return true;
}

double ChargeIsotropy::LawOfCosines(double vtxx, double vtxy, double vtxz, double pmt1x, double pmt1y, double pmt1z, double pmt2x, double pmt2y, double pmt2z){
    //vtx to pmt1 - dist^2
    double b = (vtxx-pmt1x)*(vtxx-pmt1x) + (vtxy-pmt1y)*(vtxy-pmt1y) + (vtxz-pmt1z)*(vtxz-pmt1z);
    //vtx to pmt2 - dist^2
    double c = (vtxx-pmt2x)*(vtxx-pmt2x) + (vtxy-pmt2y)*(vtxy-pmt2y) + (vtxz-pmt2z)*(vtxz-pmt2z);
    //pmt1 to pmt2 - dist^2
    double a = (pmt2x-pmt1x)*(pmt2x-pmt1x) + (pmt2y-pmt1y)*(pmt2y-pmt1y) + (pmt2z-pmt1z)*(pmt2z-pmt1z);
    //law of cosines
    return std::acos((b + c - a)/(2*std::sqrt(b*c)));
}


