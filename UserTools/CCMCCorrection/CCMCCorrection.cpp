#include "CCMCCorrection.h"

CCMCCorrection::CCMCCorrection():Tool(){}


bool CCMCCorrection::Initialise(std::string configfile, DataModel &data){

  /////////////////// Useful header ///////////////////////
  if(configfile!="") m_variables.Initialise(configfile); // loading config file
  //m_variables.Print();

  m_data= &data; //assigning transient data pointer
  /////////////////////////////////////////////////////////////////

  m_variables.Get("verbosity",verbosity);
  m_variables.Get("MRDCalFile",mrd_cal_file);
 
  //Open calibration file
  if(gSystem->AccessPathName(mrd_cal_file.c_str())){
    std::cout << "WARNING: " << mrd_cal_file << " does not exist. Stopping." << std::endl;
    return false;
  }

  //read in calibration file
  std::ifstream cal_file(mrd_cal_file.c_str(), ios::in);
  cal_file >> bins_front_str;
  cal_file >> bins_TL_str;
  cal_file >> factorX_str;
  cal_file >> factorY_str;
  cal_file >> factorTL_str;
  cal_file.close();

  //break into appropriate vectors
  this->breakCSV(bins_front_str, bins_front);
  this->breakCSV(bins_TL_str, bins_TL);
  this->breakCSV(factorX_str, factorX);
  this->breakCSV(factorY_str, factorY);
  this->breakCSV(factorTL_str, factorTL);

  return true;
}

bool CCMCCorrection::Execute(){
  bool got_mrd = false;
  bool got_ntracks = false;
  bool got_reco = false;
  bool got_genie = false
  bool get_clusters = false;

  this->Reset();

  //Check for valid track criteria
  get_clusters = m_data->CStore.Get("MrdTimeClusters",MrdTimeClusters);
  got_mrd = m_data->Stores["MRDTracks"]->Get("MRDTracks",theMrdTracks);
  got_ntracks = m_data->Stores["MRDTracks"]->Get("NumMrdTracks",numtracksinev);
 
  get_reco = m_data->Stores["RecoEvent"]->Get("simpleRecoTrackLengthInMRD",simpletracklength);
  get_genie = m_data->Stores["GenieInfo"]->Get("NuIntxVtx_Z",TrueNuIntxVtx_Z);
 
  if (!(got_mrd && got_ntracks && get_clusters)) {
    std::cout << "No MRDTracks or MRDClusters. Continuing to build tree." << std::endl;
    return;
  }
  if (!got_reco) {
    std::cout << "No Simple Track Length store in RecoEvent. Continuing to build tree." << std::endl;
    return;
  }

  //make call to MRD Eff function
  mrd_eff = this->MRDEfficiency();

  //Make call to Dirt muon function
  dirt_mu = this->DirtScaling();

  //Save to BoostStores
  m_data->Stores.at("RecoEvent")->Set("MRDEff",mrd_eff);
  m_data->Stores.at("RecoEvent")->Set("DirtScale",dirt_mu);

  return true;
}


bool CCMCCorrection::Finalise(){

  return true;
}

void CCMCCorrection::Reset(){
  numtracksinev = 0;
  simpletracklength = -9999.;
  mrd_eff = 0.0;
  dirt_mu = 1.0;
  TrueNuIntxVtx_Z = -9999.;
}

void CCMCCorrection::breakCSV(string line, vector<double> &tokens){
	//change line into stream
	std::stringstream line_in(line);
	std::string temp_token;
	double token;
	while (line_in.good()){
		std::getline(line_in, temp_token, ','); //break up by comma
		std::stringstream to_token(temp_token);
		to_token >> token;          //turn into double
		tokens.push_back(token);    //save to array
	}
}

int CCMCCorrection::findBin(double Y, int iter, vector<double> const &bins){
	if(iter == bins.size()) return 0;
	if(Y >= bins[iter]) return findBin(Y, iter+1, bins);
	else return iter;
}

double CCMCCorrection::MRDEfficiency(){
  int numtracksincluster = (int) MrdTimeClusters.size();

  if(numtracksincluster <= 0) return mrd_eff; //no mrd tracks, no calibration weight

  Position StartVertex;
  bool IsMrdStopped;

  for(int i=0; i < numtracksincluster; i++){
    for(int tracki=0; tracki<numtracksinev; tracki++){
      BoostStore* thisTrackAsBoostStore = &(theMrdTracks->at(tracki));
      int TrackEventID = -1; 
      //get track properties that are needed for the through-going muon selection
      thisTrackAsBoostStore->Get("MrdSubEventID",TrackEventID);
      if(TrackEventID!= i) continue;

      //If we're here, this track is associated with this cluster
      thisTrackAsBoostStore->Get("StartVertex",StartVertex);
      thisTrackAsBoostStore->Get("IsMrdStopped",IsMrdStopped);                // bool
      if(!IsMrdStopped) continue; //calibration applied to stopped tracks only

      //Establish which bin the event falls into
//      int binX = this->findBin(StartVertex.X(), 0, bins_front);
      int binY = this->findBin(StartVertex.Y(), 0, bins_front);
//      int binTL = findBin(simpletracklength*100, 0, bins_TL);
      //Assign weight based on bin
//      mrd_eff = factorX[binX]*factorY[binY]*factorTL[binTL];
      mrd_eff = factorY[binY];
      return mrd_eff;
    }
  }
	
  //if you are here, there were no stopped tracks. NO WEIGHT FOR YOU!
  return mrd_eff;
}

double CCMCCorrection::DirtScaling(){
  if(TrueNuIntxVtx_Z < 0.) dirt_mu = dirt_scale;
  return dirt_mu;
}
