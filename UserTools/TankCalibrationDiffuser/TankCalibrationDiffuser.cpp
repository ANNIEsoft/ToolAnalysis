#include "TankCalibrationDiffuser.h"

TankCalibrationDiffuser::TankCalibrationDiffuser():Tool(){}


bool TankCalibrationDiffuser::Initialise(std::string configfile, DataModel &data){

  if(verbose > 0) cout <<"Initialising Tool TankCalibrationDiffuser ..."<<endl;

  //only for debugging memory leaks, otherwise comment out
  /*std::cout <<"List of Objects (beginning of initialise): "<<std::endl;
  gObjectTable->Print();*/

  //---------------------------------------------------------------------------
  //---------------Useful header-----------------------------------------------
  //---------------------------------------------------------------------------

  if(configfile!="")  m_variables.Initialise(configfile); //loading config file
  m_data= &data; //assigning transient data pointer
 
  //----------------------------------------------------------------------------
  //---------------Get configuration variables for this tool--------------------
  //----------------------------------------------------------------------------
  
  m_variables.Get("OutputFile",outputfile);
  m_variables.Get("StabilityFile",stabilityfile);  
  m_variables.Get("GeometryFile",geometryfile);
  m_variables.Get("DiffuserX",diffuser_x);
  m_variables.Get("DiffuserY",diffuser_y);
  m_variables.Get("DiffuserZ",diffuser_z);
  m_variables.Get("ToleranceCharge",tolerance_charge);
  m_variables.Get("ToleranceTime",tolerance_time);
  m_variables.Get("TApplication",use_tapplication);
  m_variables.Get("verbose",verbose);

  help_file = new TFile("help_file.root","RECREATE");      //let the histos be associated with this file

  //----------------------------------------------------------------------------
  //---------------Get basic geometry properties -------------------------------
  //----------------------------------------------------------------------------

  m_data->Stores["ANNIEEvent"]->Header->Get("AnnieGeometry",geom);
  n_tank_pmts = geom->GetNumDetectorsInSet("Tank");
  n_lappds = geom->GetNumDetectorsInSet("LAPPD");
  n_mrd_pmts = geom->GetNumDetectorsInSet("MRD");
  n_veto_pmts = geom->GetNumDetectorsInSet("Veto");
  Position detector_center=geom->GetTankCentre();
  tank_center_x = detector_center.X();
  tank_center_y = detector_center.Y();
  tank_center_z = detector_center.Z();

  std::map<std::string, std::map<unsigned long,Detector*>>* Detectors = geom->GetDetectors();
  std::cout <<"Detectors size: "<<Detectors->size()<<std::endl;  // num detector sets...


  //----------------------------------------------------------------------------
  //---------------Read in geometry properties of PMTs--------------------------
  //----------------------------------------------------------------------------

  if (verbose > 0) std::cout <<"Geofile name: "<<geometryfile<<std::endl;
  ifstream file_geo;
  file_geo.open(geometryfile.c_str());
  
  if (file_geo.is_open()) {
    std::string pmt_type;
    int i=0;
    int pmt_case;
    while (getline(file_geo, pmt_type)) {
  	  if (pmt_type=="R5912HQE") pmt_case=1;
  	  else if (pmt_type=="R7081") pmt_case=2;
      else if (pmt_type=="D784KFLB") pmt_case=3;
      else pmt_case = 4;
      switch (pmt_case){
    		case 1: radius_PMT[i] = 0.1016;   //looked up in the geometry config of WCSim
    			break;
    		case 2: radius_PMT[i] = 0.127;  //looked up in the geometry config of WCSim
    			break;
    		case 3: radius_PMT[i] = 0.1397;  //looked up in the geometry config of WCSim
    			break; 
    		case 4: radius_PMT[i] = 0.10;    //FIXME: default value of 10 cm for unknown PMT types?
    			break; 
		  }
      i++;
	   }
	   file_geo.close();
   }

  //----------------------------------------------------------------------------
  //---------------calculate expected hit times for PMTs------------------------
  //----------------------------------------------------------------------------

  for (std::map<unsigned long,Detector*>::iterator it  = Detectors->at("Tank").begin();
                                                    it != Detectors->at("Tank").end();
                                                  ++it){

    Detector* apmt = it->second;
    unsigned long detkey = it->first;
    unsigned long chankey = apmt->GetChannels()->begin()->first;

    Position position_PMT = apmt->GetDetectorPosition();
    if (verbose > 2) std::cout <<"detkey: "<<detkey<<", chankey: "<<chankey<<std::endl;
    if (verbose > 2) std::cout <<"filling PMT position maps"<<std::endl;

    x_PMT.insert(std::pair<unsigned long,double>(detkey,position_PMT.X()-tank_center_x));
    y_PMT.insert(std::pair<unsigned long,double>(detkey,position_PMT.Y()-tank_center_y));
    z_PMT.insert(std::pair<unsigned long,double>(detkey,position_PMT.Z()-tank_center_z));


    double rho = sqrt(pow(x_PMT.at(detkey),2)+pow(y_PMT.at(detkey),2));
    if (x_PMT.at(detkey) < 0) rho*=-1;
    rho_PMT.insert(std::pair<unsigned long, double>(detkey,rho));


    double phi;
    if (x_PMT.at(detkey)>0 && z_PMT.at(detkey)>0) phi = atan(x_PMT.at(detkey)/z_PMT.at(detkey));
    if (x_PMT.at(detkey)>0 && z_PMT.at(detkey)<0) phi = TMath::Pi()/2+atan(x_PMT.at(detkey)/-z_PMT.at(detkey));
    if (x_PMT.at(detkey)<0 && z_PMT.at(detkey)<0) phi = TMath::Pi()+atan(x_PMT.at(detkey)/z_PMT.at(detkey));
    if (x_PMT.at(detkey)<0 && z_PMT.at(detkey)>0) phi = 3*TMath::Pi()/2+atan(-x_PMT.at(detkey)/z_PMT.at(detkey));
    phi_PMT.insert(std::pair<unsigned long,double>(detkey,phi));


    double expectedT = (sqrt(pow(x_PMT.at(detkey)-diffuser_x,2)+pow(y_PMT.at(detkey)-diffuser_y,2)+pow(z_PMT.at(detkey)-diffuser_z,2))-radius_PMT[detkey])/c_vacuum*n_water*1E9;
    expected_time.insert(std::pair<unsigned long,double>(detkey,expectedT));
    if (verbose > 2) std::cout <<"detectorkey: "<<detkey<<", position: ("<<position_PMT.X()<<","<<position_PMT.Y()<<","<<position_PMT.Z()<<")"<<std::endl;
    if (verbose > 2) std::cout <<"rho PMT "<<detkey<<": "<<rho<<std::endl;
    if (verbose > 2) std::cout <<"y PMT: "<<y_PMT.at(detkey)<<std::endl;


    PMT_ishit.insert(std::pair<unsigned long, int>(detkey,0));
  }

  std::cout <<"Number of tank PMTs: "<<n_tank_pmts<<std::endl;

  //----------------------------------------------------------------------------
  //---------------Initialize Stability histograms -----------------------------
  //----------------------------------------------------------------------------
  
  std::vector<TH1F> charge_hist;
  hist_charge = new TH1F("hist_charge","Overall charge distribution (all PMTs)",500,1,0);
  hist_time = new TH1F("hist_time","Overall time distribution (all PMTs)",500,1,0);
  hist_tubeid = new TH1F("hist_tubeid","Overall Tube ID distribution",500,1,0);
  hist_charge_2D_y_phi = new TH2F("hist_charge_2D_y_phi","Spatial distribution of charge (all PMTs)",100,0,360,25,-2.5,2.5);
  hist_time_2D_y_phi = new TH2F("hist_time_2D_y_phi","Spatial distribution of time (all PMTs)",100,0,360,25,-2.5,2.5);

  for (int i_tube=0;i_tube<n_tank_pmts;i_tube++){

  	stringstream ss;
  	ss<<i_tube+1;
  	string tubeN=ss.str();
  	string name_general_charge="hist_charge_";
  	string description_general_charge="Hit charges for tube ";
  	string name_hist_charge=name_general_charge+tubeN;
  	string description_hist_charge=description_general_charge+tubeN;
  	hist_charge_singletube[i_tube] = new TH1F(name_hist_charge.c_str(),description_hist_charge.c_str(),100,0,10);

  	string name_general_time="hist_time_";
  	string description_general_time="Hit times for tube ";
  	string name_hist_time=name_general_time+tubeN;
  	string description_hist_time=description_general_time+tubeN;
  	hist_time_singletube[i_tube] = new TH1F(name_hist_time.c_str(),description_hist_time.c_str(),100,0,20);

  }

  hist_charge_mean = new TH1F("hist_charge_mean","Mean values of detected charges",100,0,5);
  hist_time_mean = new TH1F("hist_time_mean","Mean values of detected hit times",100,-5,5);

  //---------------------------------------------------------------------------------
  //create root-file that will contain all analysis graphs for this calibration run--
  //---------------------------------------------------------------------------------

  m_data->Stores["ANNIEEvent"]->Get("RunNumber",runnumber);
  std::stringstream ss_run;
  ss_run<<runnumber;
  std::string newRunNumber = ss_run.str();
  std::string file_out_prefix="PMTStability_Run";
  std::string file_out_root=".root";
  std::string file_out_name=file_out_prefix+newRunNumber+file_out_root;
 
  file_out=new TFile(file_out_name.c_str(),"RECREATE"); //create one root file for each run to save the detailed plots and fits for all PMTs 
  file_out->cd();

  //----------------------------------------------------------------------------
  //---Make histograms appear during execution by declaring TApplication--------
  //----------------------------------------------------------------------------
   
  if (use_tapplication){
	  if (verbose > 0) std::cout <<"open TApplication..."<<std::endl;
	  int myargc = 0;
	  char *myargv[] {(char*) "options"};
	  app_stability = new TApplication("app_stability",&myargc,myargv);
	  if (verbose > 0) std::cout <<"TApplication initialized..."<<std::endl;
	}

  return true;

}


bool TankCalibrationDiffuser::Execute(){

  if (verbose > 0) std::cout <<"Executing Tool TankCalibrationDiffuser ..."<<endl;

 // get the ANNIEEvent

  int annieeventexists = m_data->Stores.count("ANNIEEvent");
  if(!annieeventexists){ std::cerr<<"Error: No ANNIEEvent store!"<<endl; /*return false;*/};

  //----------------------------------------------------------------------------
  //---------------get the members of the ANNIEEvent----------------------------
  //----------------------------------------------------------------------------

  m_data->Stores["ANNIEEvent"]->Get("MCHits", MCHits);
  m_data->Stores["ANNIEEvent"]->Get("EventNumber", evnum);
  m_data->Stores["ANNIEEvent"]->Get("BeamStatus",BeamStatus);


  //----------------------------------------------------------------------------
  //---------------Read out charge and hit values of PMTs-----------------------
  //----------------------------------------------------------------------------
  
  for (int i_pmt = 0; i_pmt < n_tank_pmts; i_pmt++){
  	PMT_ishit.at(i_pmt) = 0;
  }

  int vectsize = MCHits->size();
  if (verbose > 0) std::cout <<"MCHits size: "<<vectsize<<std::endl; 
  for(std::pair<unsigned long, std::vector<MCHit>>&& apair : *MCHits){
    unsigned long chankey = apair.first;
    Detector* thistube = geom->ChannelToDetector(chankey);
    int detectorkey = thistube->GetDetectorID();
    if (thistube->GetDetectorElement()=="Tank"){
      std::vector<MCHit>& Hits = apair.second;
      int wcsim_id;
      PMT_ishit[detectorkey] = 1;
      for (MCHit &ahit : Hits){
      	if (verbose > 2) std::cout <<"charge "<<ahit.GetCharge()<<", time "<<ahit.GetTime()<<", tubeid: "<<ahit.GetTubeId()<<std::endl;
        hist_charge->Fill(ahit.GetCharge());
        hist_time->Fill(ahit.GetTime());
        hist_tubeid->Fill(ahit.GetTubeId());
		    hist_charge_singletube[ahit.GetTubeId()]->Fill(ahit.GetCharge());
		    hist_time_singletube[ahit.GetTubeId()]->Fill(ahit.GetTime());
		  }
    }
  }

  //check whether PMT_ishit is filled correctly
  for (int i_pmt = 0; i_pmt < n_tank_pmts ; i_pmt++){
  	if (verbose > 2) std::cout <<"PMT "<<i_pmt<<" is hit: "<<PMT_ishit[i_pmt]<<std::endl;
  }

  return true;
}


bool TankCalibrationDiffuser::Finalise(){


  std::stringstream ss_run;
  ss_run<<runnumber;
  std::string newRunNumber = ss_run.str();

  //----------------------------------------------------------------------------
  //---------------Create and write single run root files------------------------
  //----------------------------------------------------------------------------
 

  //cout <<"pointer histogram charge: "<<hist_charge<<endl;
  //cout <<"pointer singletube hist charge: "<<hist_charge_singletube[23]<<endl;
 // hist_charge->Print();
  hist_charge->Write();
  hist_time->Write();
  hist_tubeid->Write();

  // create txt file to store calibration data for the specific run
  std::string filename_pre = "Run";
  std::string filename_post = "_pmts_laser_calibration.txt";
  std::string filename_complete = filename_pre+newRunNumber+filename_post;
  std::cout <<"writing to txt file: "<<filename_complete<<std::endl;
  ofstream result_file(filename_complete.c_str());

  //----------------------------------------------------------------------------
  //---------------Perform fits for PMT distributions---------------------------
  //----------------------------------------------------------------------------

  for (int i_tube=0;i_tube<n_tank_pmts;i_tube++){
    double mean_charge=hist_charge_singletube[i_tube]->GetMean();
    Double_t par[8]={10,0.3,0.1,10,1.0,0.5,1,-1};

    TF1 *total = new TF1("total","gaus(0)+gaus(3)+expo(6)",0,10);
    total->SetLineColor(2);
 
    total->SetParameters(par);
    
    //total = new TF1("total",total_value,0,10,8);
    //total->SetParameters(10,0.3,0.1,10,1.,0.3,1,1);
    hist_charge_singletube[i_tube]->Fit(total,"QR+");

    //hist_charge_singletube[i_tube]->Fit("gaus[0]+gaus[3]+exp[6]");
    hist_charge_singletube[i_tube]->Write();
    TF1 *fit_result_charge=hist_charge_singletube[i_tube]->GetFunction("total");
    mean_charge_fit[i_tube]=fit_result_charge->GetParameter(4);
    hist_charge_mean->Fill(mean_charge_fit[i_tube]);  
  

    double mean_time=hist_time_singletube[i_tube]->GetMean();
    hist_time_singletube[i_tube]->Fit("gaus","Q");
    hist_time_singletube[i_tube]->Write();
    TF1 *fit_result_time=hist_time_singletube[i_tube]->GetFunction("gaus");
    mean_time_fit[i_tube]=fit_result_time->GetParameter(1);
    hist_time_mean->Fill(mean_time_fit[i_tube]-expected_time[i_tube]);

    if (verbose > 2){
      std::cout <<"expected hit time: "<<expected_time[i_tube]<<endl;
      std::cout <<"detected hit time: "<<mean_time_fit[i_tube]<<endl;
      std::cout << "deviation: "<<mean_time_fit[i_tube]-expected_time[i_tube]<<endl;
    }

  //fill spatial detector plots as well

  if (hist_time_2D_y_phi->GetBinContent(int(phi_PMT[i_tube]/2/TMath::Pi()*100)+1,int((y_PMT[i_tube]+2.5)/5.*25)+1)==0){
    hist_time_2D_y_phi->SetBinContent(int(phi_PMT[i_tube]/2/TMath::Pi()*100)+1,int((y_PMT[i_tube]+2.5)/5.*25)+1,fabs(mean_time_fit[i_tube]-expected_time[i_tube]));  
    hist_charge_2D_y_phi->SetBinContent(int(phi_PMT[i_tube]/2/TMath::Pi()*100)+1,int((y_PMT[i_tube]+2.5)/5.*25)+1,mean_charge_fit[i_tube]);
  }
  else {
    hist_time_2D_y_phi->SetBinContent(int(phi_PMT[i_tube]/2/TMath::Pi()*100)+1,int((y_PMT[i_tube]+2.5)/5.*25)+2,fabs(mean_time_fit[i_tube]-expected_time[i_tube]));
    hist_charge_2D_y_phi->SetBinContent(int(phi_PMT[i_tube]/2/TMath::Pi()*100)+1,int((y_PMT[i_tube]+2.5)/5.*25)+2,mean_charge_fit[i_tube]);
  }
  result_file<<i_tube+1<<"  "<<mean_charge_fit[i_tube]<<"  "<<mean_time_fit[i_tube]-expected_time[i_tube]<<endl;

  vector_tf1.push_back(total);
}
  result_file<<1000<<"  "<<hist_charge_mean->GetMean()<<"  "<<hist_charge_mean->GetRMS()<<"  "<<hist_time_mean->GetMean()<<"  "<<hist_time_mean->GetRMS()<<endl; //1000 is identifier key for average value
  result_file.close();


  hist_time_mean->GetXaxis()->SetTitle("t_{arrival} - t_{expected} [ns]");
  hist_charge_mean->GetXaxis()->SetTitle("charge [p.e.]");

  hist_charge_2D_y_phi->GetXaxis()->SetTitle("#phi [deg]");
  hist_charge_2D_y_phi->GetYaxis()->SetTitle("y [m]");
  hist_time_2D_y_phi->GetXaxis()->SetTitle("#phi [deg]");
  hist_time_2D_y_phi->GetYaxis()->SetTitle("y [m]");
  hist_time_2D_y_phi->SetStats(0);
  hist_charge_2D_y_phi->SetStats(0);

  hist_time_mean->Write();
  hist_charge_mean->Write();

  hist_time_2D_y_phi->Write();
  hist_charge_2D_y_phi->Write();
  
  //----------------------------------------------------------------------------
  //---------------Create and write stability plots------------------------
  //----------------------------------------------------------------------------

  gr_stability = new TGraphErrors();
  gr_stability->SetName("gr_stability");
  gr_stability->SetTitle("Stability PMT charge calibration");
  gr_stability_time = new TGraphErrors();
  gr_stability_time->SetName("gr_stability_time");
  gr_stability_time->SetTitle("Stability PMT time calibration");

  //read in information from 100 last runs (if they do not exist, just skip them...)
  const int n_entries=100;
  double run_numbers[n_entries];
  double entries_charge[n_entries];
  double rms_charge[n_entries];
  double entries_time[n_entries];
  double rms_time[n_entries];
  bool file_exists[n_entries];

  int runnumber_temp;


  //get stability values for time and charge from txt-files
  for (int i_run=n_entries-1;i_run>=0;i_run--){
    
    std::stringstream ss_runnumber;
    runnumber_temp = runnumber-i_run;
    ss_runnumber<<runnumber_temp;
    std::string filename_mid = ss_runnumber.str();
    std::string filename_temp = filename_pre+filename_mid+filename_post;
    if (verbose > 1) std::cout <<"Stability: Filename: "<<filename_temp<<", file exists: "<<does_file_exist(filename_temp.c_str())<<std::endl;
    double dummy_temp;
    if (does_file_exist(filename_temp.c_str())){
      ifstream file_temp(filename_temp.c_str());
      file_exists[n_entries-1-i_run]=true;
      for (int i_pmt=0;i_pmt<n_tank_pmts+1;i_pmt++){
        if (i_pmt<n_tank_pmts) file_temp>>dummy_temp>>dummy_temp>>dummy_temp;
        else file_temp>>dummy_temp>>entries_charge[n_entries-1-i_run]>>rms_charge[n_entries-1-i_run]>>entries_time[n_entries-1-i_run]>>rms_time[n_entries-1-i_run];
      }
      file_temp.close();
    }
    else {
      file_exists[n_entries-1-i_run]=false;
    }

  }

  int i_point_graph=0;
  //fill TGraphErrors with stability values for time and charge
  for (int i_run=n_entries-1;i_run>=0;i_run--){
    if (file_exists[n_entries-1-i_run]){
      if (verbose > 1) std::cout <<"Stability, run "<<n_entries-1-i_run<<": Charge: "<<entries_charge[n_entries-1-i_run]<<", rms charge: "<<rms_charge[n_entries-1-i_run]<<", time: "<<entries_time[n_entries-1-i_run]<<"rms time: "<<rms_time[n_entries-1-i_run]<<std::endl;
      gr_stability->SetPoint(i_point_graph,runnumber-i_run,entries_charge[n_entries-1-i_run]);
      gr_stability->SetPointError(i_point_graph,0.,rms_charge[n_entries-1-i_run]);
      gr_stability_time->SetPoint(i_point_graph,runnumber-i_run,entries_time[n_entries-1-i_run]);
      gr_stability_time->SetPointError(i_point_graph,0.,rms_time[n_entries-1-i_run]);
      i_point_graph++;
    }

  }

  gr_stability->SetMarkerStyle(22);
  gr_stability->SetMarkerSize(1.0);
  gr_stability->SetLineColor(1);
  gr_stability->SetLineWidth(2);
  gr_stability->GetXaxis()->SetTitle("Run Number");
  gr_stability->GetYaxis()->SetTitle("charge [p.e.]");
  gr_stability_time->SetMarkerStyle(22);
  gr_stability_time->SetMarkerSize(1.0);
  gr_stability_time->SetLineColor(1);
  gr_stability_time->SetLineWidth(2);
  gr_stability_time->GetXaxis()->SetTitle("Run Number");
  gr_stability_time->GetYaxis()->SetTitle("hit time deviaton [ns]");
  gr_stability->Write("gr_stability");
  gr_stability_time->Write("gr_stability_time");

  //fill new entries into histogram
  std::map<int,double>::iterator it_time = bad_time.begin();
  std::map<int,double>::iterator it_charge = bad_charge.begin();

  std::cout <<"///////////////////////////////////////////////////////////////////"<<std::endl;
  std::cout <<"-------------------------------------------------------------------"<<std::endl;
  std::cout <<"-------------PMTs with deviating properties:-----------------------"<<std::endl;
  std::cout <<"-------------------------------------------------------------------"<<std::endl;  
  std::cout <<"///////////////////////////////////////////////////////////////////"<<std::endl;

  for (int i_tube=0;i_tube<n_tank_pmts;i_tube++){

  if (fabs(mean_time_fit[i_tube]-expected_time[i_tube])>tolerance_time) {
    std::cout <<"Abnormally high time deviation for tube "<<i_tube<<": "<<mean_time_fit[i_tube]-expected_time[i_tube]<<std::endl;
    bad_time.insert(it_time,std::pair<int,double>(i_tube,mean_time_fit[i_tube]-expected_time[i_tube]));    
  }
  if (mean_charge_fit[i_tube]<(1.0-tolerance_charge) || mean_charge_fit[i_tube]>(1.0+tolerance_charge)){
    std::cout <<"Abnormal charge for tube "<<i_tube<<": "<<mean_charge_fit[i_tube]<<std::endl;
    bad_charge.insert(it_charge,std::pair<int,double>(i_tube,mean_charge_fit[i_tube])); 
  }
}
  
  //----------------------------------------------------------------------------
  //---------------Save stability root plots------------------------------------
  //----------------------------------------------------------------------------
  
 canvas_overview = new TCanvas("canvas_overview","Stability PMTs",900,600);
 canvas_overview->Divide(3,2);
 canvas_overview->cd(1);
 hist_charge_2D_y_phi->Draw("colz");
 std::map<int,double>::iterator it_charge_read;                                  //draw red boxes around the misaligned PMTs (in charge as well as in time)
 if (verbose > 0) std::cout << "PMTs with bad charge: "<<std::endl;
 for (it_charge_read=bad_charge.begin();it_charge_read!=bad_charge.end();it_charge_read++){
      int pmt = it_charge_read->first;
      int bin_charge_x = hist_charge_2D_y_phi->GetXaxis()->FindBin(phi_PMT[pmt]/TMath::Pi()*180.);
      int bin_charge_y = hist_charge_2D_y_phi->GetYaxis()->FindBin(y_PMT[pmt]);
      if (verbose > 0) std::cout << "PMT: "<<pmt<<", bin: "<<bin_charge_x<<", "<<bin_charge_y<<std::endl;
      draw_red_box(hist_charge_2D_y_phi,bin_charge_x, bin_charge_y,verbose);
 }
 canvas_overview->cd(2);
 hist_charge_mean->Draw();
 canvas_overview->cd(3);
 gr_stability->Draw("AEL");
 canvas_overview->cd(4);
 hist_time_2D_y_phi->Draw("colz");
 std::map<int,double>::iterator it_time_read;
 if (verbose > 0) std::cout << "PMTs with bad timing: "<<std::endl;
 for (it_time_read=bad_time.begin();it_time_read!=bad_time.end();it_time_read++){
      int pmt = it_time_read->first;
      //if (verbose > 0) std::cout <<"PMT: "<<pmt<<", phi PMT: "<<phi_PMT[pmt]<<", y PMT: "<<y_PMT[pmt]<<std::endl;
      int bin_time_x = hist_time_2D_y_phi->GetXaxis()->FindBin(phi_PMT[pmt]/TMath::Pi()*180.);
      int bin_time_y = hist_time_2D_y_phi->GetYaxis()->FindBin(y_PMT[pmt]);
      if (verbose > 0) std::cout << "PMT: "<<pmt<<", bin: "<<bin_time_x<<", "<<bin_time_y<<std::endl;
      draw_red_box(hist_time_2D_y_phi,bin_time_x, bin_time_y, verbose);
 }
 canvas_overview->cd(5);
 hist_time_mean->Draw();
 canvas_overview->cd(6);
 gr_stability_time->Draw("ALE");
 canvas_overview->Modified();
 canvas_overview->Update();
 canvas_overview->Write();

 file_out->Close();

  //----------------------------------------------------------------------------
  //---------------Prompt user inputs (to avoid abortion of canvas)-------------
  //----------------------------------------------------------------------------

 int valid_calibration;
 if (verbose > 0) {
 	std::cout << "//////////////////////////////////////////////////////////////"<<std::endl;
 	std::cout << "--------------------------------------------------------------"<<std::endl;
 	std::cout << "Calibration run finished. Summary: "<<endl;
 	std::cout << "Number of misaligned PMTs (time): "<<bad_time.size()<<endl;
 	std::cout << "Number of misaligned PMTs (charge): "<<bad_charge.size()<<endl;
 	std::cout << "--------------------------------------------------------------"<<std::endl;
 	std::cout << "//////////////////////////////////////////////////////////////"<<std::endl;

 	if (use_tapplication){
 		std::cout <<"Press enter to close the TApplication window..."<<std::endl;
 		std::cin >>valid_calibration;
	 	app_stability->Terminate();
	 }
 }

 help_file->Close();

  //----------------------------------------------------------------------------
  //------------------------Deleting all objects -------------------------------
  //----------------------------------------------------------------------------

 //histograms deleted by closing the files ealier

 std::cout <<"delete TBoxes..."<<std::endl;
 for (int i_box = 0; i_box < vector_tbox.size();i_box++){
  delete vector_tbox.at(i_box);
 }
 std::cout <<"delete tf1s..."<<std::endl;
 for (int i_tf = 0; i_tf< vector_tf1.size(); i_tf++){
  delete vector_tf1.at(i_tf);
 } 

 std::cout <<"delete canvas, application, files, geom, mchits"<<std::endl;
 delete canvas_overview;
 if (use_tapplication) delete app_stability;
 delete help_file;
 delete file_out;
 delete geom;
 delete MCHits;

  //only for debugging memory leaks, otherwise comment out
  /*std::cout <<"List of Objects (end of finalise): "<<std::endl;
  gObjectTable->Print();*/


 return true;

}

  //----------------------------------------------------------------------------
  //---------------------Helper functions --------------------------------------
  //----------------------------------------------------------------------------

  bool TankCalibrationDiffuser::does_file_exist(const char *fileName){
    std::ifstream infile(fileName);
    return infile.good();
  }

  void TankCalibrationDiffuser::draw_red_box(TH2 *hist, int bin_x, int bin_y, int verbosity){

    Int_t bin_global = hist->GetBin(bin_x, bin_y,0);

    double size_x=2.0;
    double size_y=0.05;
    TBox *b = new TBox(hist->GetXaxis()->GetBinLowEdge(bin_x)-size_x,hist->GetYaxis()->GetBinLowEdge(bin_y)-size_y,hist->GetXaxis()->GetBinWidth(bin_x)+hist->GetXaxis()->GetBinLowEdge(bin_x)+size_x, hist->GetYaxis()->GetBinWidth(bin_y)+hist->GetYaxis()->GetBinLowEdge(bin_y)+size_y);
    
    if (verbosity > 2) {
    	std::cout <<"Draw Red Box: lower x: "<<hist->GetXaxis()->GetBinLowEdge(bin_x)-0.1<<std::endl;
    	std::cout <<"Draw Red Box: lower y: "<<hist->GetYaxis()->GetBinLowEdge(bin_y)-0.1<<std::endl;
    	std::cout <<"Draw Red Box: upper x: "<<hist->GetXaxis()->GetBinWidth(bin_x)+hist->GetXaxis()->GetBinLowEdge(bin_x)+0.1<<std::endl;
    	std::cout <<"Draw Red Box: upper y: "<<hist->GetXaxis()->GetBinWidth(bin_y)+hist->GetXaxis()->GetBinLowEdge(bin_y)+0.1<<std::endl;
    }

    b->SetFillStyle(0);
    b->SetLineColor(2);
    b->SetLineWidth(2);
    b->Draw();

    vector_tbox.push_back(b);
  }



