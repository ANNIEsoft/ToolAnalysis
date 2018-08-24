#include "DigitBuilder.h"


static DigitBuilder* fgDigitBuilder = 0;
DigitBuilder* DigitBuilder::Instance()
{
  if( !fgDigitBuilder ){
    fgDigitBuilder = new DigitBuilder();
  }

  return fgDigitBuilder;
}

DigitBuilder::DigitBuilder():Tool(){}
DigitBuilder::~DigitBuilder() {
	
}

bool DigitBuilder::Initialise(std::string configfile, DataModel &data){

  /////////////////// Usefull header ///////////////////////
  if(verbosity) cout<<"Initializing Tool DigitBuilder"<<endl;
  if(configfile!="")  m_variables.Initialise(configfile); //loading config file
  //m_variables.Print();

  m_data= &data; //assigning transient data pointer
  /////////////////////////////////////////////////////////////////
  fPhotodetectorConfiguration = "All";
  
  /// Get the Tool configuration variables
	m_variables.Get("verbosity",verbosity);
	m_variables.Get("PhotoDetectorConfiguration", fPhotodetectorConfiguration);
	m_variables.Get("MRDRecoCut", fMRDRecoCut);
	m_variables.Get("MCTruthCut", fMCTruthCut);
	
	/// Construct the other objects we'll be setting at event level,
	fMuonStartVertex = new RecoVertex();
	fMuonStopVertex = new RecoVertex();
	fDigitList = new std::vector<RecoDigit>;
		
	// Make the RecoDigit Store if it doesn't exist
	int recoeventexists = m_data->Stores.count("RecoEvent");
	if(recoeventexists==0) m_data->Stores["RecoEvent"] = new BoostStore(false,2);
	fEventCutStatus = false;
	m_data->Stores.at("RecoEvent")->Set("EventCutStatus", fEventCutStatus); 
  return true;
}

bool DigitBuilder::Execute(){
	Log("===========================================================================================",v_debug,verbosity);
	
	// Reset everything
	this->Reset();
	
	// see if "ANNIEEvent" exists
	auto get_annieevent = m_data->Stores.count("ANNIEEvent");
	if(!get_annieevent){
		Log("DigitBuilder Tool: No ANNIEEvent store!",v_error,verbosity); 
		return false;
	};
	
	/// First, see if this is a delayed trigger in the event
	auto get_mctrigger = m_data->Stores.at("ANNIEEvent")->Get("MCTriggernum",fMCTriggernum);
	if(!get_mctrigger){ 
		Log("DigitBuilder Tool: Error retrieving MCTriggernum from ANNIEEvent!",v_error,verbosity); 
		return false; 
	}
	
	/// if so, truth analysis is probably not interested in this trigger. Primary muon will not be in the listed tracks.
	if(fMCTriggernum>0){ 
		Log("DigitBuilder Tool: Skipping delayed trigger",v_debug,verbosity); 
		return true;
	}
	
	/// Retrieve the hit info from ANNIEEvent
	auto get_geometry= m_data->Stores.at("ANNIEEvent")->Header->Get("AnnieGeometry",fGeometry);
	if(!get_geometry){
		Log("DigitBuilder Tool: Error retrieving Geometry from ANNIEEvent!",v_error,verbosity); 
		return false; 
	}
	auto get_mchits = m_data->Stores.at("ANNIEEvent")->Get("MCHits",fMCHits);
	if(!get_mchits){ 
		Log("DigitBuilder Tool: Error retrieving MCHits from ANNIEEvent!",v_error,verbosity); 
		return false; 
	}
	auto get_mclappdhits = m_data->Stores.at("ANNIEEvent")->Get("MCLAPPDHits",fMCLAPPDHits);
	if(!get_mclappdhits){
		Log("DigitBuilder Tool: Error retrieving MCLAPPDHits from ANNIEEvent!",v_error,verbosity); 
		return false;
	}
	
	// Retrieve particle information from ANNIEEvent
  auto get_mcparticles = m_data->Stores.at("ANNIEEvent")->Get("MCParticles",fMCParticles);
	if(!get_mcparticles){ 
		Log("DigitBuilder:: Tool: Error retrieving MCParticles from ANNIEEvent!",v_error,verbosity); 
		return false; 
	}
	
	/// check if "MRDTracks" store exists
	int mrdtrackexists = m_data->Stores.count("MRDTracks");
	if(mrdtrackexists == 0) {
	  Log("DigitBuilder Tool: MRDTracks store doesn't exist!",v_message,verbosity);
	  return false;
	} 	
	
	/// Find true neutrino vertex which is defined by the start point of the Primary muon
  this->FindTrueVertexFromMC();
	
	// Select event by MRD Reconstruction information
	if(fMRDRecoCut) {
	  bool passMRD = this->EventSelectionByMRDReco();
	  if(!passMRD) {
	      Log("DigitBuilder Tool: Event doesn't pass data selection using MRD reconstruction information",v_message,verbosity);
	      return true;
	  }
  } 
	
	// Select event by fiducial volume with MC trueth information
	 if(fMCTruthCut) {
	   bool passMCcut = this->EventSelectionByMCTruthInfo();
	   if(!passMCcut) {
	   	 Log("DigitBuilder Tool: Event doesn't pass data selection using MC information",v_message,verbosity);
	     return true;
	   }
	 }
	 
	// Build RecoDigit
	this->BuildRecoDigit();
	
	if(fDigitList->size()<4) {
		Log("DigitBuilder Tool: Event has less than 4 digits",v_message,verbosity);
		return true;
	}
	
	// event passed selection
	fEventCutStatus = true;
	m_data->Stores.at("RecoEvent")->Set("EventCutStatus", fEventCutStatus);
	
	// Push true vertex to RecoEvent store
  this->PushTrueVertex(true);

	// Push recodigits to RecoEvent
	this->PushRecoDigits(true); 
	
  return true;
}

bool DigitBuilder::Finalise(){
	//fOutput_tfile->cd();
	//fDigitTree->Write();
	//fOutput_tfile->Close();
	if(verbosity>0) cout<<"DigitBuilder exitting"<<endl;
  return true;
}

bool DigitBuilder::BuildRecoDigit() {
	
	if(fPhotodetectorConfiguration == "PMT_only") {
		this->BuildPMTRecoDigit();
		return true;
	}
	if(fPhotodetectorConfiguration == "LAPPD_only") {
		this->BuildLAPPDRecoDigit();
		return true;
	}
	if(fPhotodetectorConfiguration == "All") {
		this->BuildPMTRecoDigit();
	  this->BuildLAPPDRecoDigit();
	  return true;
	}
	else {
	  cout<<"Wrong PhotoDetector Configuration! Allowed configurations: PMT_only, LAPPD_only, All"<<endl;
	  return false;
	}
	
}

bool DigitBuilder::BuildPMTRecoDigit() {
	
	Log("DigitBuilder Tool: Build PMT reconstructed digits",v_message,verbosity);
	// now move to digit retrieval
	int region = -999;
	double calT;
	double calQ = 0.;
	int digitType = -999;
	Detector det;
	Position  pos_sim, pos_reco;
	// MCHits is a std::map<ChannelKey,std::vector<Hit>>
	if(fMCHits){
		Log("DigitBuilder Tool: Num PMT Digits = "+to_string(fMCHits->size()),v_message, verbosity);
		// iterate over the map of sensors with a measurement
		for(std::pair<ChannelKey,std::vector<Hit>>&& apair : *fMCHits){
			ChannelKey chankey = apair.first;
			// a ChannelKey is a detector descriptor, containing 2 elements:
			// a 'subdetector' (enum class), with types ADC, LAPPD, TDC
			// and a DetectorElementIndex, i.e. the ID of the detector of that type
			// get PMT position
			det = fGeometry.GetDetector(chankey);
			if(det.GetDetectorElement() == "") {
				Log("DigitBuilder Tool: Detector not found! ",v_message,verbosity);
				continue;
			}
			
			// convert the WCSim coordinates to the ANNIEreco coordinates
			// convert the unit from m to cm
			pos_sim = det.GetDetectorPosition();	
			pos_sim.UnitToCentimeter();
			pos_reco.SetX(pos_sim.X());
			pos_reco.SetY(pos_sim.Y()+14.46469);
			pos_reco.SetZ(pos_sim.Z()-168.1);
	
			if(chankey.GetSubDetectorType()==subdetector::ADC){
				std::vector<Hit>& hits = apair.second;	
			  for(Hit& ahit : hits){
			  	//ahit.Print();
					//if(v_message<verbosity) ahit.Print(); // << VERY verbose
					// get calibrated PMT time (Use the MC time for now)
					calT = ahit.GetTime()*1.0;
					calQ = ahit.GetCharge();
					digitType = RecoDigit::PMT8inch;
					RecoDigit recoDigit(region, pos_reco, calT, calQ, digitType);
				  //recoDigit.Print();
				  fDigitList->push_back(recoDigit); 
        }
			}
		} // end loop over MCHits
	} else {
		cout<<"No MCHits"<<endl;
		return false;
	}
	return true;
}

bool DigitBuilder::BuildLAPPDRecoDigit() {
	Log("DigitBuilder Tool: Build LAPPD reconstructed digits",v_message,verbosity);
	int region = -999;
	double calT = 0;
	double calQ = 0;
	int digitType = -999;
	Detector det;
	Position  pos_sim, pos_reco;
  // repeat for LAPPD hits
	// MCLAPPDHits is a std::map<ChannelKey,std::vector<LAPPDHit>>
	if(fMCLAPPDHits){
		Log("DigitBuilder Tool: Num LAPPD Digits = "+to_string(fMCLAPPDHits->size()),v_message,verbosity);
		// iterate over the map of sensors with a measurement
		for(std::pair<ChannelKey,std::vector<LAPPDHit>>&& apair : *fMCLAPPDHits){
			ChannelKey chankey = apair.first;
			det = fGeometry.GetDetector(chankey);
			//Tube ID is different from that in ANNIEReco
			//int LAPPDId = det.GetDetectorId();
			//if(LAPPDId != 266 && LAPPDId != 271 && LAPPDId != 236 && LAPPDId != 231 && LAPPDId != 206) continue;
			if(chankey.GetSubDetectorType()==subdetector::LAPPD){ // redundant
				std::vector<LAPPDHit>& hits = apair.second;
				for(LAPPDHit& ahit : hits){
					//if(v_message<verbosity) ahit.Print(); // << VERY verbose
					// an LAPPDHit has adds (global x-y-z) position, (in-tile x-y) local position
					// and time psecs
					// convert the WCSim coordinates to the ANNIEreco coordinates
					// convert the unit from m to cm
					pos_reco.SetX(ahit.GetPosition().at(0)*100.); //cm
					pos_reco.SetY(ahit.GetPosition().at(1)*100.+14.4649); //cm
					pos_reco.SetZ(ahit.GetPosition().at(2)*100.-168.1); //cm
					calT = ahit.GetTime() + ahit.GetTpsec()/1000 + 950.;  // Add 950 ns offset
					calQ = ahit.GetCharge();
					// I found the charge is 0 for all the hits. In order to test the code, 
					// here I just set the charge to 1. We should come back to this later. (Jingbo Wang)
					calQ = 1.;
					digitType = RecoDigit::lappd_v0;
					RecoDigit recoDigit(region, pos_reco, calT, calQ, digitType);
					//if(v_message<verbosity) recoDigit.Print();
				  fDigitList->push_back(recoDigit);
				}
			}
		} // end loop over MCLAPPDHits
	} else {
		cout<<"No MCLAPPDHits"<<endl;
		return false;
	}	
	return true;
}

void DigitBuilder::PushRecoDigits(bool savetodisk) {
	Log("DigitBuilder Tool: Push reconstructed digits to the RecoEvent store",v_message,verbosity);
	m_data->Stores.at("RecoEvent")->Set("RecoDigit", fDigitList, savetodisk);  ///> Add digits to RecoEvent
}

void DigitBuilder::Reset() {
	// Reset 
	fMuonStartVertex->Reset();
	fMuonStopVertex->Reset();
  fDigitList->clear();
  fEventCutStatus = false;
  m_data->Stores.at("RecoEvent")->Set("EventCutStatus", fEventCutStatus); 
}

RecoVertex* DigitBuilder::FindTrueVertexFromMC() {
	
	// loop over the MCParticles to find the highest enery primary muon
	// MCParticles is a std::vector<MCParticle>
	MCParticle primarymuon;  // primary muon
	bool mufound=false;
	if(fMCParticles){
		Log("DigitBuilder::  Tool: Num MCParticles = "+to_string(fMCParticles->size()),v_message,verbosity);
		for(int particlei=0; particlei<fMCParticles->size(); particlei++){
			MCParticle aparticle = fMCParticles->at(particlei);
			//if(v_debug<verbosity) aparticle.Print();       // print if we're being *really* verbose
			if(aparticle.GetParentPdg()!=0) continue;      // not a primary particle
			if(aparticle.GetPdgCode()!=13) continue;       // not a muon
			primarymuon = aparticle;                       // note the particle
			mufound=true;                                  // note that we found it
			//primarymuon.Print();
			break;                                         // won't have more than one primary muon
		}
	} else {
		Log("DigitBuilder::  Tool: No MCParticles in the event!",v_error,verbosity);
	}
	if(not mufound){
		Log("DigitBuilder::  Tool: No muon in this event",v_warning,verbosity);
		return 0;
	}
	
	// retrieve desired information from the particle
	Position muonstartpos = primarymuon.GetStartVertex();    // only true if the muon is primary
	double muonstarttime = primarymuon.GetStartTime().GetNs() + primarymuon.GetStopTime().GetTpsec()/1000;
	Position muonstoppos = primarymuon.GetStopVertex();    // only true if the muon is primary
	double muonstoptime = primarymuon.GetStopTime().GetNs() + primarymuon.GetStopTime().GetTpsec()/1000;
	
	Direction muondirection = primarymuon.GetStartDirection();
	double muonenergy = primarymuon.GetStartEnergy();
	// set true vertex
	// change unit
	muonstartpos.UnitToCentimeter(); // convert unit from meter to centimeter
	muonstoppos.UnitToCentimeter(); // convert unit from meter to centimeter
	// change coordinate for muon start vertex
	muonstartpos.SetY(muonstartpos.Y()+14.46469);
	muonstartpos.SetZ(muonstartpos.Z()-168.1);
	fMuonStartVertex->SetVertex(muonstartpos, muonstarttime);
  fMuonStartVertex->SetDirection(muondirection);
  //  charge coordinate for muon stop vertex
  muonstoppos.SetY(muonstoppos.Y()+14.46469);
	muonstoppos.SetZ(muonstoppos.Z()-168.1);
	fMuonStopVertex->SetVertex(muonstoppos, muonstoptime); 
  
  logmessage = "  trueVtx=(" +to_string(muonstartpos.X()) + ", " + to_string(muonstartpos.Y()) + ", " + to_string(muonstartpos.Z()) +", "+to_string(muonstarttime)+ "\n"
            + "           " +to_string(muondirection.X()) + ", " + to_string(muondirection.Y()) + ", " + to_string(muondirection.Z()) + ") " + "\n";
  Log(logmessage,v_debug,verbosity);
  return fMuonStartVertex;
}

void DigitBuilder::PushTrueVertex(bool savetodisk) {
	Log("DigitBuilder Tool: Push true vertex to the RecoEvent store",v_message,verbosity);
	m_data->Stores.at("RecoEvent")->Set("TrueVertex", fMuonStartVertex, savetodisk); 
}

// This function isn't working now, because the MRDTracks store must have been changed. 
// We have to contact Marcus and ask how we can retieve the MRD track information. 
bool DigitBuilder::EventSelectionByMRDReco() {
  /// Get number of subentries
	int NumMrdTracks = 0;
	double mrdtracklength = 0.;
	double MaximumMRDTrackLength = 0;
	int mrdlayershit = 0;
	bool mrdtrackisstoped = false;
	int longesttrackEntryNumber = -9999;
	
	auto getmrdtracks = m_data->Stores.at("MRDTracks")->Get("NumMrdTracks",NumMrdTracks);
	if(!getmrdtracks) {
	  Log("DigitBuilder Tool: Error retrieving MRDTracks Store!",v_error,verbosity); 
		return false;	
	}
	logmessage = "DigitBuilder Tool: Found " + to_string(NumMrdTracks) + " MRD tracks";
	Log(logmessage,v_message,verbosity);
	
	// If mrd track isn't found
	if(NumMrdTracks == 0) {
	  Log("DigitBuilder Tool: Found no MRD Tracks",v_message,verbosity);
	  return false;
	}
	// If mrd track is found
	// MRD tracks are saved in the "MRDTracks" Store. Each entry is an MRD track

	for(int entrynum=0; entrynum<NumMrdTracks; entrynum++) {
		//m_data->Stores["MRDTracks"]->GetEntry(entrynum);
	  m_data->Stores.at("MRDTracks")->Get("TrackLength",mrdtracklength);
	  m_data->Stores.at("MRDTracks")->Get("LayersHit",mrdlayershit);
	  if(MaximumMRDTrackLength<mrdtracklength) {
	    MaximumMRDTrackLength = mrdtracklength;
	    longesttrackEntryNumber = entrynum;
	  }
	}
	// check if the longest track is stopped inside MRD
	//m_data->Stores["MRDTracks"]->GetEntry(longesttrackEntryNumber);
	m_data->Stores.at("MRDTracks")->Get("IsMrdStopped",mrdtrackisstoped);
	if(!mrdtrackisstoped) return false;
	return true;
}

bool DigitBuilder::EventSelectionByMCTruthInfo() {
	if(!fMuonStartVertex) return false;
		
	double trueVtxX, trueVtxY, trueVtxZ;
	Position vtxPos = fMuonStartVertex->GetPosition();
	Direction vtxDir = fMuonStartVertex->GetDirection();
	trueVtxX = vtxPos.X();
  trueVtxY = vtxPos.Y();
  trueVtxZ = vtxPos.Z();
  
  // fiducial volume cut
  double tankradius = ANNIEGeometry::Instance()->GetCylRadius();	
  double fidcutradius = 0.8 * tankradius;
  double fidcuty = 50.;
	double fidcutz = 0.;
	if(trueVtxZ > fidcutz) return false;
	if( (TMath::Sqrt(TMath::Power(trueVtxX, 2) + TMath::Power(trueVtxZ,2)) > fidcutradius) 
		  || (TMath::Abs(trueVtxY) > fidcuty) 
		  || (trueVtxZ > fidcutz) ){
    return false;
	}	 	
	
//	// mrd cut
//	double muonStopX, muonStopY, muonStopZ;
//	Position muonStopPos = fMuonStopVertex->GetPosition();
//	muonStopX = muonStopPos.X();
//	muonStopY = muonStopPos.Y();
//	muonStopZ = muonStopPos.Z();
//	
//	// The following dimensions are inaccurate. We need to get these numbers from the geometry class
//	double distanceBetweenTankAndMRD = 10.; // about 10 cm
//	double mrdThicknessZ = 60.0;
//	double mrdWidthX = 305.0;
//	double mrdHeightY = 274.0;
//	if(muonStopZ<tankradius + distanceBetweenTankAndMRD || muonStopZ>tankradius + distanceBetweenTankAndMRD + mrdThicknessZ
//		|| muonStopX<-1.0*mrdWidthX/2 || muonStopX>mrdWidthX/2
//		|| muonStopY<-1.0*mrdHeightY/2 || muonStopY>mrdHeightY/2) {
//	  return false;	
//	}
	
  return true;	
}