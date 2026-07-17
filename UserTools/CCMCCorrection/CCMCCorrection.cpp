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
  m_variables.Get("NUniverses",n_univ);
  m_variables.Get("RandomSeed",seed);
 
  //read in calibration file
  std::ifstream cal_file(mrd_cal_file.c_str(), ios::in);
  cal_file >> bins_front_str;
  cal_file >> factorY_str;
  cal_file >> uncsY_str;
  cal_file.close();

  //break into appropriate vectors
  this->breakCSV(bins_front_str, bins_front);
  this->breakCSV(factorY_str, factorY);
  this->breakCSV(uncsY_str, uncsY);

  //Set up uncertainty values for each universe
  rnd.SetSeed(seed);
  for(int i = 0; i < uncsY.size(); ++i){
    std::vector<double> universes;
    for(int j = 0; j < n_univ; ++j){
      double mrd_unc = rnd.Gaus(1., uncsY.at(i));
      universes.push_back(mrd_unc);
    }
    mrd_reweight_vector.push_back(universes);
  }
  for(int j = 0; j < n_univ; ++j){
    //0.0028 is calculated uncertainty from total # of events / POT
    dirt_rew_vector.push_back(rnd.Gaus(1., dirt_unc));
  }

  return true;
}

bool CCMCCorrection::Execute(){
  bool got_mrd = false;
  bool got_ntracks = false;
  bool got_genie = false;
  bool got_clusters = false;

  this->Reset();

  //Check for valid track criteria
  got_clusters = m_data->CStore.Get("MrdTimeClusters",MrdTimeClusters);
  got_mrd = m_data->Stores["MRDTracks"]->Get("MRDTracks",theMrdTracks);
  got_ntracks = m_data->Stores["MRDTracks"]->Get("NumMrdTracks",numtracksinev);
 
  got_genie = m_data->Stores["GenieInfo"]->Get("NuIntxVtx_Z",TrueNuIntxVtx_Z);
 
  if (!(got_mrd && got_ntracks && got_clusters)) {
    std::cout << "No MRDTracks or MRDClusters. Continuing to build tree." << std::endl;
    return true;
  }

  //make call to MRD Eff function
  mrd_eff = this->MRDEfficiency();
  if(mrd_eff == 1.0){
    for(int j = 0; j < n_univ; ++j) MRDUnc.push_back(1.);
  }

  //Make call to Dirt muon function
  dirt_mu = this->DirtScaling();

  //Save to BoostStores
  m_data->Stores.at("RecoEvent")->Set("MRDEff",mrd_eff);
  m_data->Stores.at("RecoEvent")->Set("MRDUnc",MRDUnc);
  m_data->Stores.at("RecoEvent")->Set("DirtScale",dirt_mu);
  m_data->Stores.at("RecoEvent")->Set("DirtUnc",DirtUnc);
  return true;
}


bool CCMCCorrection::Finalise(){

  return true;
}

void CCMCCorrection::Reset(){
  numtracksinev = 0;
  mrd_eff = 1.0;
  dirt_mu = 1.0;
  TrueNuIntxVtx_Z = -9999.;
  MRDUnc.clear();
  DirtUnc.clear();
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
	if(Y < bins[iter]) return iter;
	else return findBin(Y, iter+1, bins);
}

double CCMCCorrection::MRDEfficiency(){
  int numtracksincluster = (int) MrdTimeClusters.size();

  if(numtracksincluster <= 0) return 1.0; //no mrd tracks, no calibration weight

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
      int binY = this->findBin(StartVertex.Y(), 0, bins_front);
      //Assign weight based on bin
      mrd_eff = factorY[binY];
      for(int j = 0; j < n_univ; ++j){
        MRDUnc.push_back(mrd_reweight_vector.at(binY).at(j));
      }
      return mrd_eff;
    }
  }
	
  //if you are here, there were no stopped tracks. NO WEIGHT FOR YOU!
  return 1.0;
}

double CCMCCorrection::DirtScaling(){
  if(TrueNuIntxVtx_Z < 0.){
    dirt_mu = dirt_scale;
    for(int j = 0; j < n_univ; ++j){
      DirtUnc.push_back(dirt_rew_vector.at(j));
    }
  } else {
    dirt_mu = 1.0;
    for(int j = 0; j < n_univ; ++j) DirtUnc.push_back(1.0);
  }

  return dirt_mu;
}
