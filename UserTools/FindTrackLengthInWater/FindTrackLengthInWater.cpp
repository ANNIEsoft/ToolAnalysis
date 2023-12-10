#include "FindTrackLengthInWater.h"
#include <boost/filesystem.hpp>
#include "TMath.h"

FindTrackLengthInWater::FindTrackLengthInWater():Tool(){}


bool FindTrackLengthInWater::Initialise(std::string configfile, DataModel &data){

  /////////////////// Usefull header ///////////////////////
  if(configfile!="")  m_variables.Initialise(configfile); //loading config file
  //m_variables.Print();

  m_data= &data; //assigning transient data pointer
  /////////////////////////////////////////////////////////////////
  // get configuration variables for this tool
  m_variables.Get("verbosity",verbosity);
  Log("FindTrackLengthInWater Tool: Initializing",v_message,verbosity);
  
  get_ok=m_variables.Get("DoTraining", fDoTraining);//training or predict
  std::string OutputDataFile;
    get_ok = m_variables.Get("OutputDataFile",OutputDataFile);
  if(not get_ok){
    Log("FindTrackLengthInWater Tool: No OutputDataFile specified, will not be written",v_error,verbosity);
  }
  if(get_ok){
  // Writing the file header(s)
  // ========================
    // writing header row
    csvfile.open(OutputDataFile,std::fstream::out);
    if(!csvfile.is_open()){
     Log("FindTrackLengthInWater Tool: Failed to open "+OutputDataFile+" for writing headers",v_error,verbosity);
    }
    for (int i=0; i<maxhits0;++i){
       csvfile<<"l_"<<i<<",";
    }
    for (int i=0; i<maxhits0;++i){
       csvfile<<"T_"<<i<<",";
    }
    csvfile<<"lambda_max,"  //first estimation of track length(using photons projection on track)
           <<"totalPMTs,"   // number of PMT hits, not number of pmts.
           <<"totalLAPPDs," // number of LAPPD hits... 
           <<"lambda_max,"  // again for stand_alone scripts
           <<"TrueTrackLengthInWater,"
           //<<"neutrinoE,"
           <<"trueKE,"      // energy of the primary muon
           <<"diffDirAbs,"
           <<"TrueTrackLengthInMrd,"
           <<"recoDWallR,"
           <<"recoDWallZ,"
           <<"dirX,"        // the reconstructed direction of the muon
           <<"dirY,"
           <<"dirZ,"
           <<"vtxX,"        // the reconstructed vertex of the muon
           <<"vtxY,"
           <<"vtxZ,"
           <<"recoVtxFOM,"
           <<"recoTrackLengthInMrd," //track length in the mrd reconstructed with FindMrdTracks tool
           <<"eventNum"
           <<'\n';
    csvfile.close();
    
  csvfile.open(OutputDataFile,std::fstream::app);  // open file for writing events
  }
  //Create BoostStore to store the variables for track length and energy reco
  BoostStore* energystore;
  energystore = new BoostStore(true,2); // type is multi-entry binary file
  m_data->Stores.emplace("EnergyReco",energystore);// place boost store in the DataModel
  
  // Get values from Config file
  // ===========================
  get_ok = m_variables.Get("MaxTotalHitsToDNN",maxhits0);
  if(not get_ok){
    Log("FindTrackLengthInWater Tool: No MaxTotalHitsToDNN specified: assuming 1100, but this MUST match the value used for DNN training!",v_warning,verbosity);
    maxhits0=1100;
  }
  Log("FindTrackLengthInWater Tool: max number of hits per event: "+to_string(maxhits0),v_debug,verbosity);
  
  // Get geometry variables from ANNIEEvent
  // =============================
  get_ok = m_data->Stores.at("ANNIEEvent")->Header->Get("AnnieGeometry",anniegeom);
  if(not get_ok){
    Log("FindTrackLengthInWater Tool: No Geometry in ANNIEEvent!",v_error,verbosity);
    return false;
  }
  tank_radius = anniegeom->GetTankRadius()*100.;
  std::cout<<"Tank radius is "<<tank_radius<<std::endl;
  tank_halfheight = anniegeom->GetTankHalfheight()*100.;
  std::cout<<"Tank halfheight is "<<tank_halfheight<<std::endl;
  return true;
}

bool FindTrackLengthInWater::Execute(){
   Log("FindTrackLengthInWater Tool: Executing",v_message,verbosity);
   count4++;
   m_data->Stores.at("EnergyReco")->Delete();//clear the last entry from RAM

   // See if this event passes selection: we have several potential cuts.
   // First check the EventCutstatus: event was in fiducial volume, had a stopping muon etc.
   bool EventCutstatus;
   get_ok = m_data->Stores.at("RecoEvent")->Get("EventCutStatus",EventCutstatus);
   if(not get_ok){
     Log("FindTrackLengthInWater Tool: No EventCutStatus in the ANNIEEvent!",v_error,verbosity);
     return false;
   }
   if(not EventCutstatus){
     Log("FindTrackLengthInWater Tool: Event did not pass the reconstruction selection cuts, skipping",v_message, verbosity);
     return true;
   }
   count2++;
   
   // Check for the reconstructed tank vertex
   RecoVertex* theExtendedVertex=nullptr;
   get_ok = m_data->Stores.at("RecoEvent")->Get("ExtendedVertex", theExtendedVertex);
   if((get_ok==0)||(theExtendedVertex==nullptr)){
   	Log("FindTrackLengthInWater Tool: Failed to retrieve the ExtendedVertex from RecoEvent Store!",v_error,verbosity);
   	return false;
   }
   Int_t recoStatus = theExtendedVertex->GetStatus();
   double recoVtxFOM = theExtendedVertex->GetFOM();
   Log("FindTrackLengthInWater Tool: recoVtxFOM="+to_string(recoVtxFOM),v_debug,verbosity);
   if(recoVtxFOM<=0.){
     Log("FindTrackLengthInWater Tool: Vertex reconstruction failed, skipping",v_message,verbosity);
     return true;
   }
   Log("FindTrackLengthInWater Tool: Vertex reconstruction cuts passed",v_debug,verbosity);
   count3++;
   
     //Get the reconstructed MRDTrackLength from CStore
     std::vector<std::vector<int>> MrdTimeClusters;
     std::vector<BoostStore>* theMrdTracks;
     int numtracksinev;
     double MRDTrackLength=-9999;
     Position StartVertex;
     Position StopVertex;
     //Load the mrd time clusters from CStore
     bool get_clusters = m_data->CStore.Get("MrdTimeClusters",MrdTimeClusters);
      if(!get_clusters){
        std::cout << "FindTrackLengthInWater tool: No MRD time clusters found.  Will be no tracks." << std::endl;
        return false;
      }
      
      //loop through the time clusters and get the mrd tracks
      for(int i=0; i < (int) MrdTimeClusters.size(); i++){
      m_data->Stores["MRDTracks"]->Get("MRDTracks",theMrdTracks);
      m_data->Stores["MRDTracks"]->Get("NumMrdTracks",numtracksinev);
      //loop through the mrd tracks and get the necessary info to calculate MRDTrackLength for each track
      for(int tracki=0; tracki<numtracksinev; tracki++){
    BoostStore* thisTrackAsBoostStore = &(theMrdTracks->at(tracki));
    int TrackEventID = -1; 
    //get track properties that are needed for the through-going muon selection
	thisTrackAsBoostStore->Get("MrdSubEventID",TrackEventID);
    if(TrackEventID!= i) continue;
    thisTrackAsBoostStore->Get("StartVertex",StartVertex);
    thisTrackAsBoostStore->Get("StopVertex",StopVertex);
    MRDTrackLength = sqrt(pow((StopVertex.X()-StartVertex.X()),2)+pow(StopVertex.Y()-StartVertex.Y(),2)+pow(StopVertex.Z()-StartVertex.Z(),2)) * 100.0;
      }
      }
      //Don't use events without reconstructed track length in the MRD
      if(MRDTrackLength<=0.){
      Log("FindTrackLengthInWater Tool: MRD reconstruction failed, skipping",v_message,verbosity);
      return true;
      }
      count1++;

   // That's all the cuts!
   // ====================
   //get info for mc vertex
   RecoVertex *trueVtx = 0;
   auto get_truevtx = m_data->Stores.at("RecoEvent")->Get("TrueVertex", trueVtx);
   
   //Get mc track length in the mrd stored in the RecoEvent store from previous tools
   double TrueTrackLengthInMrd;
   auto get_MRDTrackLength = m_data->Stores.at("RecoEvent")->Get("TrueTrackLengthInMRD", TrueTrackLengthInMrd);
   Log("FindTrackLengthInWater Tool: TrueTrackLengthInMrd="+to_string(TrueTrackLengthInMrd),v_debug,verbosity);
   
   // proceed with getting event info we need to calculate the lambda values for the track length in water reconstruction
   double vtxX,vtxY,vtxZ,dirX,dirY,dirZ,TrueNeutrinoEnergy,trueEnergy;
   std::vector<double> digitX; std::vector<double> digitY;  std::vector<double> digitZ;
   std::vector<double> digitT;
   
   // get reconstructed vertex and direction info
   Log("FindTrackLengthInWater Tool: Getting reco vertex and direction components",v_debug,verbosity);
   vtxX = theExtendedVertex->GetPosition().X();
   vtxY = theExtendedVertex->GetPosition().Y();
   vtxZ = theExtendedVertex->GetPosition().Z();
   dirX = theExtendedVertex->GetDirection().X();
   dirY = theExtendedVertex->GetDirection().Y();
   dirZ = theExtendedVertex->GetDirection().Z();
   
   // get additional primary muon info from RecoEvent store
   Log("FindTrackLengthInWater Tool: Getting primary muon info",v_debug,verbosity);
   double TrueTrackLengthInWater;
   auto get_tankTrackLength = m_data->Stores.at("RecoEvent")->Get("TrueTrackLengthInWater", TrueTrackLengthInWater);
   if(not get_tankTrackLength){
   Log("FindTrackLengthInWater Tool: Failed to retrieve the TrueTrackLengthInWater from RecoEvent Store!",v_error,verbosity);
   return false;
   }
   auto get_muonMCEnergy = m_data->Stores.at("RecoEvent")->Get("TrueMuonEnergy", trueEnergy);
   if(not get_muonMCEnergy){
   Log("FindTrackLengthInWater Tool: Failed to retrieve the TrueMuonEnergy from RecoEvent Store!",v_error,verbosity);
   return false;
   }
   
   /*// Get neutrino info needed for neutrino energy BDT training
   get_ok = m_data->Stores.at("GenieEvent")->Get("TrueNeutrinoEnergy", TrueNeutrinoEnergy);
   if(not get_ok){
   	Log("FindTrackLengthInWater Tool: Failed to retrieve TrueNeutrinoEnergy!",v_error,verbosity);
   	return false;
   }
   */
   
   // get digits from RecoDigit store
   std::vector<RecoDigit>* digitList;
   auto get_digits=m_data->Stores.at("RecoEvent")->Get("RecoDigit", digitList);
   if(not get_digits){
   Log("FindTrackLengthInWater Tool: Failed to retrieve the RecoDigit from RecoEvent Store!",v_error,verbosity);
   return false;
   }
   // Extract the PMT & LAPPD digit information
   // ===============================
   int totalPMTs =0; // number of digits from PMT hits in the event
  int totalLAPPDs = 0; // number of digits from LAPPD hits in the event
  //loop through all digits
  for(RecoDigit &adigit : *digitList){
	  digitX.push_back(adigit.GetPosition().X());
	  digitY.push_back(adigit.GetPosition().Y());
	  digitZ.push_back(adigit.GetPosition().Z());
	  digitT.push_back(adigit.GetCalTime());
	  if(adigit.GetDigitType()==0){totalPMTs+=1;} //when the digit type is zero we have a PMT digit
	  else{totalLAPPDs+=1;}// when it is 1 we have LAPPD
  }
   Log("FindTrackLengthInWater Tool: Got "+to_string(totalPMTs)+" PMT digits; "+to_string(digitT.size())
       +" total digits so far",v_debug,verbosity);
   Log("FindTrackLengthInWater Tool: Got "+to_string(totalLAPPDs)+" LAPPD digits; "+to_string(digitT.size())
        +" total digits",v_debug,verbosity);
   
   // Estimate the track length in the tank
   // =====================================
        //calculate diff dir with (0,0,1)
        double diffDirAbs0 = TMath::ACos(dirZ)*TMath::RadToDeg();
        float diffDirAbs2=diffDirAbs0/90.;
        double recoVtxR2 = vtxX*vtxX + vtxZ*vtxZ;
        double recoDWallR = tank_radius-TMath::Sqrt(recoVtxR2);
        double recoDWallZ = tank_halfheight/2.-TMath::Abs(vtxY);
	//Get true vtx and dir (optional)
	double truevtxX=trueVtx->GetPosition().X();
	double truevtxY=trueVtx->GetPosition().Y();
	double truevtxZ=trueVtx->GetPosition().Z();
        double truedirX=trueVtx->GetDirection().X();
	double truedirY=trueVtx->GetDirection().Y();
	double truedirZ=trueVtx->GetDirection().Z();
	// Estimate the track length
	// =========================
        Log("FindTrackLengthInWater Tool: Estimating track length in tank",v_debug,verbosity);
	double lambda_min = 10000000;  double lambda_max = -99999999.9; double lambda = 0;
	std::vector<double> lambda_vector;
	for(int k=0; k<digitT.size(); k++){

	  // Estimate length as the distance between the reconstructed vertex and last photon emission point
          lambda = find_lambda(vtxX,vtxY,vtxZ,dirX,dirY,dirZ,digitX.at(k),digitY.at(k),digitZ.at(k),42.);
          if( lambda <= lambda_min ){
	      lambda_min = lambda;
	  }
	  if( lambda >= lambda_max ){
	      lambda_max = lambda;
	  }
          lambda_vector.push_back(lambda);
  	}
  	
       // Post-processing of variables to store
       // =====================================
       float recoDWallR2      = recoDWallR/tank_radius;
       float recoDWallZ2      = recoDWallZ/tank_halfheight*2.;
       float TrueTrackLengthInWater2 = TrueTrackLengthInWater*100.;  // convert to [cm]
       float TrueTrackLengthInMrd2 = TrueTrackLengthInMrd;      // it is already in [cm]
       // we need to normalise the digit time and lambda vectors to fixed dimensions to match the MaxTotalHitsToDNN
       
       lambda_vector.resize(maxhits0);
       
       digitT.resize(maxhits0);
       
       //put MaxTotalHitsToDNN in store for use in the next tools
       m_data->Stores.at("EnergyReco")->Set("MaxTotalHitsToDNN",maxhits0);
       
       // put the last successfully processed event number in the EnergyReco store as well.
       // write the current event to file
       uint32_t EventNumber;
       get_ok = m_data->Stores.at("ANNIEEvent")->Get("EventNumber", EventNumber);
       

        // Put these variables in the EnergyReco BoostStore
        // ================================================
        Log("FindTrackLengthInWater Tool: putting event "+to_string(EventNumber)+" into the EnergyReco store",v_debug,verbosity);
        m_data->Stores.at("EnergyReco")->Set("ThisEvtNum",EventNumber);
        //std::cout<<"This is the Eventnumber you are looking for"<<EventNumber<<std::endl;
        std::cout<<"This is the lambda_vec before being set into the boostStore with size "<<lambda_vector.size()<<std::endl;
        for (int i=0; i < (int) lambda_vector.size(); i++){std::cout << lambda_vector.at(i)<<std::endl;}
        Log("FindTrackLengthInWater Tool: lambda_vector: "+to_string(lambda_vector.size())+"  FINISHED",v_debug,verbosity);
        m_data->Stores.at("EnergyReco")->Set("lambda_vec",lambda_vector);
        m_data->Stores.at("EnergyReco")->Set("digit_ts_vec",digitT);
        m_data->Stores.at("EnergyReco")->Set("lambda_max",lambda_max);
        m_data->Stores.at("EnergyReco")->Set("num_pmt_hits",totalPMTs);
        m_data->Stores.at("EnergyReco")->Set("num_lappd_hits",totalLAPPDs);
        m_data->Stores.at("EnergyReco")->Set("TrueTrackLengthInWater",TrueTrackLengthInWater2);
        //m_data->Stores.at("EnergyReco")->Set("trueNeuE",TrueNeutrinoEnergy);
        m_data->Stores.at("EnergyReco")->Set("trueMuonEnergy",trueEnergy);
        m_data->Stores.at("EnergyReco")->Set("diffDirAbs",diffDirAbs2);
        //m_data->Stores.at("EnergyReco")->Set("TrueTrackLengthInMrd",TrueTrackLengthInMrd2);
        m_data->Stores.at("EnergyReco")->Set("recoDWallR",recoDWallR2);
        m_data->Stores.at("EnergyReco")->Set("recoDWallZ",recoDWallZ2);
        m_data->Stores.at("EnergyReco")->Set("dirVec",theExtendedVertex->GetDirection());
        m_data->Stores.at("EnergyReco")->Set("vtxVec",theExtendedVertex->GetPosition());
        m_data->Stores.at("EnergyReco")->Set("recoTrackLengthInMrd",MRDTrackLength);

  if(not fDoTraining){
    Log("FindTrackLengthInWater Tool: Not doing training, so BoostStore will not be saved locally",v_error,verbosity);
    }
// only save all data to disk when training
if(fDoTraining){ 
     m_data->Stores.at("EnergyReco")->Save("EnergyReco.bs");    // write this Energy Reco entry to disk. If the file exists, it appends as a new entry.
     }     
//---------------------------------------------------------------------------------------------------------------------------------------------------------//
  if(lambda_vector.size()!=maxhits0){
    Log("FindTrackLengthInWater Tool: Error! lambdavector size is not maxhits0! Check dimensions!",v_error,verbosity);
    return false;
  }
  if(digitT.size()!=maxhits0){
    Log("FindTrackLengthInWater Tool: Error! digitT size is not maxhits0! Check dimensions!",v_error,verbosity);
    return false;
  }
  
  // Write to .csv file
  // ==================
  if(not csvfile.is_open()){
     Log("FindTrackLengthInWater Tool: output file is closed, skipping write",v_debug,verbosity);
     return true;
  }
  
  for(int i=0; i<maxhits0;++i){
     csvfile<<lambda_vector.at(i)<<",";
  }
  for(int i=0; i<maxhits0;++i){
     csvfile<<digitT.at(i)<<",";
  }
  csvfile<<lambda_max<<","
         <<totalPMTs<<","
         <<totalLAPPDs<<","
         <<lambda_max<<","
         <<TrueTrackLengthInWater2<<","
         //<<TrueNeutrinoEnergy<<","
         <<trueEnergy<<","
         <<diffDirAbs2<<","
         <<TrueTrackLengthInMrd2<<","
         <<recoDWallR2<<","
         <<recoDWallZ2<<","
         <<dirX<<","
         <<dirY<<","
         <<dirZ<<","
         <<vtxX<<","
         <<vtxY<<","
         <<vtxZ<<","
         <<recoVtxFOM<<","
         <<MRDTrackLength<<","
         <<EventNumber
         <<'\n';

  return true;
}

//---------------------------------------------------------------------------------------------------------//
// Find Distance between Muon Vertex and Photon Production Point (lambda) 
//---------------------------------------------------------------------------------------------------------//
double FindTrackLengthInWater::find_lambda(double xmu_rec,double ymu_rec,double zmu_rec,double xrecDir,double yrecDir,double zrecDir,double x_pmtpos,double y_pmtpos,double z_pmtpos,double theta_cher)
{
     double lambda1 = 0.0;    double lambda2 = 0.0;    double length = 0.0 ;
     double xmupos_t1 = 0.0;  double ymupos_t1 = 0.0;  double zmupos_t1 = 0.0;
     double xmupos_t2 = 0.0;  double ymupos_t2 = 0.0;  double zmupos_t2 = 0.0;
     double xmupos_t = 0.0;   double ymupos_t = 0.0;   double zmupos_t = 0.0;

     double theta_muDir_track = 0.0;
     double theta_muDir_track1 = 0.0;  double theta_muDir_track2 = 0.0;
     double cos_thetacher = cos(theta_cher*TMath::DegToRad());
     double xmupos_tmin = 0.0; double ymupos_tmin = 0.0; double zmupos_tmin = 0.0;
     double xmupos_tmax = 0.0; double ymupos_tmax = 0.0; double zmupos_tmax = 0.0;
     double lambda_min = 10000000;  double lambda_max = -99999999.9;  double lambda = 0.0;

     double alpha = (xrecDir*xrecDir + yrecDir*yrecDir + zrecDir*zrecDir) * ( (xrecDir*xrecDir + yrecDir*yrecDir + zrecDir*zrecDir) - (cos_thetacher*cos_thetacher) );
     double beta = ( (-2)*(xrecDir*(x_pmtpos - xmu_rec) + yrecDir*(y_pmtpos - ymu_rec) + zrecDir*(z_pmtpos - zmu_rec) )*((xrecDir*xrecDir + yrecDir*yrecDir + zrecDir*zrecDir) - (cos_thetacher*cos_thetacher)) );
     double gamma = ( ( (xrecDir*(x_pmtpos - xmu_rec) + yrecDir*(y_pmtpos - ymu_rec) + zrecDir*(z_pmtpos - zmu_rec))*(xrecDir*(x_pmtpos - xmu_rec) + yrecDir*(y_pmtpos - ymu_rec) + zrecDir*(z_pmtpos - zmu_rec)) ) - (((x_pmtpos - xmu_rec)*(x_pmtpos - xmu_rec) + (y_pmtpos - ymu_rec)*(y_pmtpos - ymu_rec) + (z_pmtpos - zmu_rec)*(z_pmtpos - zmu_rec))*(cos_thetacher*cos_thetacher)) );


     double discriminant = ( (beta*beta) - (4*alpha*gamma) );

     lambda1 = ( (-beta + sqrt(discriminant))/(2*alpha));
     lambda2 = ( (-beta - sqrt(discriminant))/(2*alpha));

     xmupos_t1 = xmu_rec + xrecDir*lambda1;  xmupos_t2 = xmu_rec + xrecDir*lambda2;
     ymupos_t1 = ymu_rec + yrecDir*lambda1;  ymupos_t2 = ymu_rec + yrecDir*lambda2;
     zmupos_t1 = zmu_rec + zrecDir*lambda1;  zmupos_t2 = zmu_rec + zrecDir*lambda2;

     double tr1 = sqrt((x_pmtpos - xmupos_t1)*(x_pmtpos - xmupos_t1) + (y_pmtpos - ymupos_t1)*(y_pmtpos - ymupos_t1) + (z_pmtpos - zmupos_t1)*(z_pmtpos - zmupos_t1));
     double tr2 = sqrt((x_pmtpos - xmupos_t2)*(x_pmtpos - xmupos_t2) + (y_pmtpos - ymupos_t2)*(y_pmtpos - ymupos_t2) + (z_pmtpos - zmupos_t2)*(z_pmtpos - zmupos_t2));
     theta_muDir_track1 = (acos( (xrecDir*(x_pmtpos - xmupos_t1) + yrecDir*(y_pmtpos - ymupos_t1) + zrecDir*(z_pmtpos - zmupos_t1))/(tr1) )*TMath::RadToDeg());
     theta_muDir_track2 = (acos( (xrecDir*(x_pmtpos - xmupos_t2) + yrecDir*(y_pmtpos - ymupos_t2) + zrecDir*(z_pmtpos - zmupos_t2))/(tr2) )*TMath::RadToDeg());

     //---------------------------- choose lambda!! ---------------------------------
     if( theta_muDir_track1 < theta_muDir_track2 ){
       lambda = lambda1;
       xmupos_t = xmupos_t1;
       ymupos_t = ymupos_t1;
       zmupos_t = zmupos_t1;
       theta_muDir_track=theta_muDir_track1;
     }else if( theta_muDir_track2 < theta_muDir_track1 ){
       lambda = lambda2;
       xmupos_t = xmupos_t2;
       ymupos_t = ymupos_t2;
       zmupos_t = zmupos_t2;
       theta_muDir_track=theta_muDir_track2;
     }
     return lambda;
}

bool FindTrackLengthInWater::Finalise(){
 m_data->Stores.at("EnergyReco")->Close();
 if(csvfile.is_open()) csvfile.close();
  Log("FindTrackLengthInWater Tool: processed "+to_string(count1)+" events",v_message,verbosity);
  std::cout<<"processed a total of "<<count4<<" events, of which "
           <<count2<<" passed event selection cuts, "<<count3
           <<" had a reconstructed vertex and "<<count1
           <<" also had a positive reconstructed MRD track length"<<std::endl;
  std::cout<<"FindTrackLengthInWater Tool: processed "<<count1<<" events"<<std::endl;

  return true;
}
