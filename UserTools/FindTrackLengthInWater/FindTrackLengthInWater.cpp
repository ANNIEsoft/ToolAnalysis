#include "FindTrackLengthInWater.h"

FindTrackLengthInWater::FindTrackLengthInWater():Tool(){}


bool FindTrackLengthInWater::Initialise(std::string configfile, DataModel &data){

  /////////////////// Usefull header ///////////////////////
  if(configfile!="")  m_variables.Initialise(configfile); //loading config file
  //m_variables.Print();

  m_data= &data; //assigning transient data pointer
  /////////////////////////////////////////////////////////////////
  // get configuration variables for this tool 
  m_variables.Get("InputFile",infile);

  file= new TFile(infile.c_str(),"READ");
  regTree= (TTree*) file->Get("vertextree");
  std::cout<<"Number of entries in tree: "<<regTree->GetEntries()<<std::endl;

  currententry=1;
    
  //m_variables.Get("verbose",verbose);
  //verbose=10;
  m_variables.Get("WriteTrackLengthToFile",writefile);
  if(writefile==1){
    m_variables.Get("OutputDirectory",outputdir);
    outputFile = new TFile(outputdir.c_str(), "RECREATE");

    nu_eneNEW = new TTree("nu_eneNEW","nu_eneNEW");
  }
  m_variables.Get("Outputfile",myfile);
  csvfile.open(myfile);   

      std::cout<<" open file.. max number of hits: "<<maxhits0<<std::endl;  
      if(maxhits0>1100){ 
        std::cerr<<" Please change the dim of double lambda_vec[1100]={0.}; double digitt[1100]={0.}; from 1100 to max number of hits"<<std::endl; 
      }
      //--- write to file: ---//
      //if(first==1 && deny_access==0){
      //    deny_access=1;
          for (int i=0; i<maxhits0;++i){
             stringstream strs;
             strs << i;
             string temp_str = strs.str();
             string X_name= "l_";
             X_name.append(temp_str);
             const char * xname = X_name.c_str();
             csvfile<<xname<<",";
          }
          for (int ii=0; ii<maxhits0;++ii){
             stringstream strs4;
             strs4 << ii;
             string temp_str4 = strs4.str();
             string T_name= "T_";
             T_name.append(temp_str4);
             const char * tname = T_name.c_str();
             csvfile<<tname<<",";
          }
        csvfile<<"lambda_max"<<","; //first estimation of track length(using photons projection on track)
        csvfile<<"totalPMTs"<<",";
        csvfile<<"totalLAPPDs"<<",";
        csvfile<<"lambda_max"<<",";
        csvfile<<"TrueTrackLengthInWater"<<",";
        csvfile<<"neutrinoE"<<",";
        csvfile<<"trueKE"<<",";
        csvfile<<"diffDirAbs"<<",";
        csvfile<<"TrueTrackLengthInMrd"<<",";
        csvfile<<"recoDWallR"<<",";
        csvfile<<"recoDWallZ"<<",";
        csvfile<<"dirX"<<",";
        csvfile<<"dirY"<<",";
        csvfile<<"dirZ"<<",";
        csvfile<<"vtxX"<<",";
        csvfile<<"vtxY"<<",";
        csvfile<<"vtxZ";
        csvfile<<'\n';
      // }

  return true;
}

bool FindTrackLengthInWater::Execute(){

   //------------ write tree to file: 
   int ievt=0; float totalLAPPDs2=0; float totalPMTs2=0;
   float trueNeuE=0; float trueE=0;
   float TrueTrackLengthInWater2=0; float TrueTrackLengthInMrd2=0; float recoDWallR2=0; float recoDWallZ2=0; 
   float lambda_max_2=0; float diffDirAbs2=0; float recolength=0;
   float dirX2=0; float dirY2=0; float dirZ2=0;
   float vtxX2=0; float vtxY2=0; float vtxZ2=0;

   nu_eneNEW->Branch("ievt", &ievt, "ievt/I");
   nu_eneNEW->Branch("neutrinoE", &trueNeuE, "neutrinoE/F");
   nu_eneNEW->Branch("trueKE", &trueE, "trueKE/F");
   nu_eneNEW->Branch("diffDirAbs2", &diffDirAbs2, "diffDirAbs2/F");
   nu_eneNEW->Branch("TrueTrackLengthInWater", &TrueTrackLengthInWater2, "TrueTrackLengthInWater/F");
   nu_eneNEW->Branch("TrueTrackLengthInMrd", &TrueTrackLengthInMrd2, "TrueTrackLengthInMrd/F");
   nu_eneNEW->Branch("recoDWallR2", &recoDWallR2, "recoDWallR2/F");
   nu_eneNEW->Branch("recoDWallZ2", &recoDWallZ2, "recoDWallZ2/F");
   nu_eneNEW->Branch("totalPMTs", &totalPMTs2, "totalPMTs/F");
   nu_eneNEW->Branch("totalLAPPDs", &totalLAPPDs2, "totalLAPPDs/F");
   nu_eneNEW->Branch("lambda_max_2", &lambda_max_2, "lambda_max_2/F");
   nu_eneNEW->Branch("dirX",&dirX2, "dirX/F");
   nu_eneNEW->Branch("dirY",&dirY2, "dirY/F");
   nu_eneNEW->Branch("dirZ",&dirZ2, "dirZ/F");
   nu_eneNEW->Branch("vtxX",&vtxX2, "vtxX/F");
   nu_eneNEW->Branch("vtxY",&vtxY2, "vtxY/F");
   nu_eneNEW->Branch("vtxZ",&vtxZ2, "vtxZ/F");

   //----------- read the tree from file:
   //deny_access=1;
   Int_t run, event, nhits, trigger,recoStatus;
   double vtxX,vtxY,vtxZ,dirX,dirY,dirZ,TrueTrackLengthInMrd,TrueTrackLengthInWater,TrueNeutrinoEnergy,trueEnergy,TrueMomentumTransfer,TrueMuonAngle;
   std::string *TrueInteractionType = 0;
   std::vector<double> *digitX=0; std::vector<double> *digitY=0;  std::vector<double> *digitZ=0; 
   std::vector<double> *digitT=0; std::vector<string>  *digitType=0;

   regTree->GetEntry(currententry);

   regTree->SetBranchAddress("run", &run);
   regTree->SetBranchAddress("event", &event);
   regTree->SetBranchAddress("trueEnergy", &trueEnergy);
   regTree->SetBranchAddress("TrueNeutrinoEnergy", &TrueNeutrinoEnergy);
   regTree->SetBranchAddress("trigger", &trigger);
   regTree->SetBranchAddress("nhits", &nhits);
   regTree->SetBranchAddress("vtxX", &vtxX);
   regTree->SetBranchAddress("vtxY", &vtxY);
   regTree->SetBranchAddress("vtxZ", &vtxZ);
   regTree->SetBranchAddress("dirX", &dirX);
   regTree->SetBranchAddress("dirY", &dirY);
   regTree->SetBranchAddress("dirZ", &dirZ);
   regTree->SetBranchAddress("digitT", &digitT);
   regTree->SetBranchAddress("digitX", &digitX);
   regTree->SetBranchAddress("digitY", &digitY);
   regTree->SetBranchAddress("digitZ", &digitZ);
   regTree->SetBranchAddress("digitType", &digitType);
   regTree->SetBranchAddress("recoStatus", &recoStatus);
   regTree->SetBranchAddress("TrueInteractionType", &TrueInteractionType);
   regTree->SetBranchAddress("TrueTrackLengthInMrd", &TrueTrackLengthInMrd);
   regTree->SetBranchAddress("TrueTrackLengthInWater", &TrueTrackLengthInWater);
   regTree->SetBranchAddress("TrueMomentumTransfer", &TrueMomentumTransfer);
   regTree->SetBranchAddress("TrueMuonAngle", &TrueMuonAngle);

   double lambda_min = 10000000;  double lambda_max = -99999999.9; double lambda = 0; 
   int totalPMTs=0; int totalLAPPDs=0; recoDWallR2=0; recoDWallZ2=0; diffDirAbs2=0;
   double lambda_vec[1100]={0.}; double digitt[1100]={0.};

   std::cout<<"currententry: "<<currententry<<endl;
   if(recoStatus == 0){ count1++;
     if((*TrueInteractionType == "QES - Weak[CC]") && TrueTrackLengthInMrd>0.){ 
   	//std::cout<<"current entry: "<<currententry<<" with nhits: "<<nhits<<std::endl;

        //calculate diff dir with (0,0,1)  
        double diffDirAbs0 = TMath::ACos(dirZ)*TMath::RadToDeg();
        //cout<<"diffDirAbs0: "<<diffDirAbs0<<endl;    
        diffDirAbs2=diffDirAbs0/90.;
        double recoVtxR2 = vtxX*vtxX + vtxZ*vtxZ;//vtxY*vtxY;
        double recoDWallR = 152.4-TMath::Sqrt(recoVtxR2);
        double recoDWallZ = 198-TMath::Abs(vtxY);
        recoDWallR2      = recoDWallR/152.4;
        recoDWallZ2      = recoDWallZ/198.;

	for(int k=0; k<nhits; k++){
          //std::cout<<"k: "<<k<<", "<<digitT->at(k)<<" | "<<digitType->at(k)<<std::endl;
          digitt[k]=digitT->at(k);
          if( (digitType->at(k)) == "PMT8inch"){ totalPMTs++; }
          if( (digitType->at(k)) == "lappd_v0"){ totalLAPPDs++; }

	   //------ Find rack Length as the distance between the reconstructed vertex last photon emission point ----/
          lambda = find_lambda(vtxX,vtxY,vtxZ,dirX,dirY,dirZ,digitX->at(k),digitY->at(k),digitZ->at(k),42.);
          if( lambda <= lambda_min ){
	      lambda_min = lambda;
	  }
	  if( lambda >= lambda_max ){
	      lambda_max = lambda;
	  }
          lambda_vec[k]=lambda;
         //m_data->Stores["ANNIEEvent"]->Set("WaterRecoTrackLength",lambda_max);
  	}
        //std::cout<<"the track length in the water tank (1st approx) is: "<<lambda_max<<std::endl;

       //---------------------------------
       //---- add values to tree for energy reconstruction: 
       ievt=currententry;
       trueNeuE=1.*TrueNeutrinoEnergy;
       trueE=1.*trueEnergy;
       dirX2=dirX; dirY2=dirY; dirZ2=dirZ;
       vtxX2=vtxX; vtxY2=vtxY; vtxZ2=vtxZ;
       recoDWallR2      = recoDWallR/152.4;
       recoDWallZ2      = recoDWallZ/198.;
       lambda_max_2     = TMath::Abs(lambda_max)/500;
       totalPMTs2=1.*totalPMTs/1000.;
       totalLAPPDs2=1.*totalLAPPDs/1000.;
       TrueTrackLengthInWater2 = TrueTrackLengthInWater/500.;
       TrueTrackLengthInMrd2 = TrueTrackLengthInMrd/200.;       

        //----- write to .csv file - including variables for track length & energy reconstruction:
        for(int i=0; i<maxhits0;++i){
           csvfile<<lambda_vec[i]<<",";
        }
        for(int i=0; i<maxhits0;++i){
           csvfile<<digitt[i]<<",";
        }
        csvfile<<lambda_max<<",";
        csvfile<<totalPMTs<<",";
        csvfile<<totalLAPPDs<<",";
        csvfile<<lambda_max<<",";
        csvfile<<TrueTrackLengthInWater<<",";
        csvfile<<trueNeuE<<",";
        csvfile<<trueE<<",";
        csvfile<<diffDirAbs2<<",";
        csvfile<<TrueTrackLengthInMrd2<<",";
        csvfile<<recoDWallR2<<",";
        csvfile<<recoDWallZ2<<",";
        csvfile<<dirX2<<",";
        csvfile<<dirY2<<",";
        csvfile<<dirZ2<<",";
        csvfile<<vtxX2<<",";
        csvfile<<vtxY2<<",";
        csvfile<<vtxZ2;
        csvfile<<'\n';
        //------------------------  
 


       nu_eneNEW->Fill();
     }
   }
   currententry++;

   if(currententry==regTree->GetEntries()) m_data->vars.Set("StopLoop",1);
   //if(currententry==5) m_data->vars.Set("StopLoop",1);

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
 
  nu_eneNEW->Write();
  return true;
}
