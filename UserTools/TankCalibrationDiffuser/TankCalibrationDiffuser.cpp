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

  m_variables.Get("HitStore",HitStoreName);
  m_variables.Get("OutputFile",outputfile);
  m_variables.Get("NBinsTimeTotal",nBinsTimeTotal);
  m_variables.Get("TimeTotalMin",timeTotalMin);
  m_variables.Get("TimeTotalMax",timeTotalMax);
  m_variables.Get("NBinsChargeTotal",nBinsChargeTotal);
  m_variables.Get("ChargeTotalMin",chargeTotalMin);
  m_variables.Get("ChargeTotalMax",chargeTotalMax);
  m_variables.Get("NBinsTime",nBinsTime);
  m_variables.Get("TimeMin",timeMin);
  m_variables.Get("TimeMax",timeMax);
  m_variables.Get("NBinsCharge",nBinsCharge);
  m_variables.Get("ChargeMin",chargeMin);
  m_variables.Get("ChargeMax",chargeMax);
  m_variables.Get("NBinsStartTimeTotal",nBinsStartTimeTotal);
  m_variables.Get("NBinsStartTime",nBinsStartTime);
  m_variables.Get("StartTimeMin",startTimeMin);
  m_variables.Get("StartTimeMax",startTimeMax);
  m_variables.Get("NBinsPeakTimeTotal",nBinsPeakTimeTotal);
  m_variables.Get("NBinsPeakTime",nBinsPeakTime);
  m_variables.Get("PeakTimeMin",peakTimeMin);
  m_variables.Get("PeakTimeMax",peakTimeMax);
  m_variables.Get("NBinsBaselineTotal",nBinsBaselineTotal);
  m_variables.Get("NBinsBaseline",nBinsBaseline);
  m_variables.Get("BaselineMin",baselineMin);
  m_variables.Get("BaselineMax",baselineMax);
  m_variables.Get("NBinsSigmaBaselineTotal",nBinsSigmaBaselineTotal);
  m_variables.Get("NBinsSigmaBaseline",nBinsSigmaBaseline);
  m_variables.Get("SigmaBaselineMin",sigmaBaselineMin);
  m_variables.Get("SigmaBaselineMax",sigmaBaselineMax); 
  m_variables.Get("NBinsRawAmplitudeTotal",nBinsRawAmplitudeTotal);
  m_variables.Get("NBinsRawAmplitude",nBinsRawAmplitude);
  m_variables.Get("RawAmplitudeMin",rawAmplitudeMin);
  m_variables.Get("RawAmplitudeMax",rawAmplitudeMax);
  m_variables.Get("NBinsAmplitudeTotal",nBinsAmplitudeTotal);
  m_variables.Get("NBinsAmplitude",nBinsAmplitude);
  m_variables.Get("AmplitudeMin",amplitudeMin);
  m_variables.Get("AmplitudeMax",amplitudeMax);
  m_variables.Get("NBinsRawAreaTotal",nBinsRawAreaTotal);
  m_variables.Get("NBinsRawArea",nBinsRawArea);
  m_variables.Get("RawAreaMin",rawAreaMin);
  m_variables.Get("RawAreaMax",rawAreaMax);
  m_variables.Get("NBinsTimeFit",nBinsTimeFit);
  m_variables.Get("TimeFitMin",timeFitMin);
  m_variables.Get("TimeFitMax",timeFitMax);
  m_variables.Get("NBinsTimeDev",nBinsTimeDev);
  m_variables.Get("TimeDevMin",timeDevMin);
  m_variables.Get("TimeDevMax",timeDevMax);
  m_variables.Get("NBinsChargeFit",nBinsChargeFit);
  m_variables.Get("ChargeFitMin",chargeFitMin);
  m_variables.Get("ChargeFitMax",chargeFitMax);
  m_variables.Get("DiffuserX",diffuser_x);
  m_variables.Get("DiffuserY",diffuser_y);
  m_variables.Get("DiffuserZ",diffuser_z);
  m_variables.Get("ToleranceCharge",tolerance_charge);
  m_variables.Get("ToleranceTime",tolerance_time);
  m_variables.Get("FitMethod",FitMethod);
  m_variables.Get("Gaus1Constant",gaus1Constant);
  m_variables.Get("Gaus1Mean",gaus1Mean);
  m_variables.Get("Gaus1Sigma",gaus1Sigma);
  m_variables.Get("Gaus2Constant",gaus2Constant);
  m_variables.Get("Gaus2Mean",gaus2Mean);
  m_variables.Get("Gaus2Sigma",gaus2Sigma);
  m_variables.Get("ExpConstant",expConstant);
  m_variables.Get("ExpDecay",expDecay);
  m_variables.Get("TApplication",use_tapplication);
  m_variables.Get("verbose",verbose);

  help_file = new TFile("configfiles/TankCalibrationDiffuser/help_file.root","RECREATE");      //let the histograms be associated with this file

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

  //FIXME: put PMT radii in geometry class, read directly and not via PMT type mapping
  map_type_radius.emplace("R5912HQE",0.1016);
  map_type_radius.emplace("R7081",0.127);
  map_type_radius.emplace("R7081HQE",0.127);
  map_type_radius.emplace("D784KFLB",0.1397);
  map_type_radius.emplace("EMI9954KB",0.0508);


  //----------------------------------------------------------------------------
  //---------------calculate expected hit times for PMTs------------------------
  //----------------------------------------------------------------------------

  for (std::map<unsigned long,Detector*>::iterator it  = Detectors->at("Tank").begin(); it != Detectors->at("Tank").end(); ++it){

    Detector* apmt = it->second;
    unsigned long detkey = it->first;
    unsigned long chankey = apmt->GetChannels()->begin()->first;
    pmt_detkeys.push_back(detkey);    
    std::string dettype = apmt->GetDetectorType();

    if (verbose > 1) std::cout <<"PMT with detkey: "<<detkey<<", has type "<<dettype<<std::endl;
    if (verbose > 1) std::cout <<"PMT Print:" <<apmt->Print()<<std::endl;

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

    if (verbose > 2) std::cout <<"detectorkey: "<<detkey<<", position: ("<<position_PMT.X()<<","<<position_PMT.Y()<<","<<position_PMT.Z()<<")"<<std::endl;
    if (verbose > 2) std::cout <<"rho PMT "<<detkey<<": "<<rho<<std::endl;
    if (verbose > 2) std::cout <<"y PMT: "<<y_PMT.at(detkey)<<std::endl;

    PMT_ishit.insert(std::pair<unsigned long, int>(detkey,0));

    if (dettype.find("R5912") != std::string::npos) radius_PMT[detkey] = map_type_radius["R5912HQE"];
    else if (dettype.find("R7081") != std::string::npos) radius_PMT[detkey] = map_type_radius["R7081"];
    else if (dettype.find("D784KFLB") != std::string::npos) radius_PMT[detkey] = map_type_radius["D784KFLB"];
    else if (dettype.find("EMI9954KB") != std::string::npos) radius_PMT[detkey] = map_type_radius["EMI9954KB"];

    double expectedT = (sqrt(pow(x_PMT.at(detkey)-diffuser_x,2)+pow(y_PMT.at(detkey)-diffuser_y,2)+pow(z_PMT.at(detkey)-diffuser_z,2))-radius_PMT[detkey])/c_vacuum*n_water*1E9;
    expected_time.insert(std::pair<unsigned long,double>(detkey,expectedT));
  }

  if (verbose > 1) std::cout <<"Number of tank PMTs: "<<n_tank_pmts<<std::endl;

  for (int i_pmt=0;i_pmt<n_tank_pmts;i_pmt++){
    unsigned long detkey = pmt_detkeys.at(i_pmt);
    if (verbose > 1) std::cout <<"Detkey: "<<detkey<<", Radius PMT "<<i_pmt<<": "<<radius_PMT[detkey]<<", expectedT: "<<expected_time[detkey]<<std::endl;
  }

  std::vector<unsigned long>::iterator it_minkey = std::min_element(pmt_detkeys.begin(),pmt_detkeys.end());
  std::vector<unsigned long>::iterator it_maxkey = std::max_element(pmt_detkeys.begin(),pmt_detkeys.end());

  int min_detkey = std::distance(pmt_detkeys.begin(),it_minkey);
  int max_detkey = std::distance(pmt_detkeys.begin(),it_maxkey);
  int n_detkey_bins = max_detkey-min_detkey;

  //----------------------------------------------------------------------------
  //---------------Initialize Stability histograms -----------------------------
  //----------------------------------------------------------------------------

  std::vector<TH1F> charge_hist;
  hist_charge = new TH1F("hist_charge","Overall charge distribution (all PMTs)",nBinsChargeTotal,chargeTotalMin,chargeTotalMax);
  hist_time = new TH1F("hist_time","Overall time distribution (all PMTs)",nBinsTimeTotal,timeTotalMin,timeTotalMax);
  hist_tubeid = new TH1F("hist_tubeid","Overall Tube ID distribution",n_detkey_bins,pmt_detkeys[min_detkey],pmt_detkeys[max_detkey]);
  hist_tubeid_adc = new TH1F("hist_tubeid_adc","Overall Tube ID distribution (ADCReco Store)",n_detkey_bins,pmt_detkeys[min_detkey],pmt_detkeys[max_detkey]);
  hist_starttime = new TH1F("hist_starttime","Overall start time distribution (all PMTs)",nBinsStartTimeTotal,startTimeMin,startTimeMax);
  hist_peaktime = new TH1F("hist_peaktime","Overall peak time distribution (all PMTs)",nBinsPeakTimeTotal,peakTimeMin,peakTimeMax);
  hist_baseline = new TH1F("hist_baseline","Overall baseline distribution (all PMTs)",nBinsBaselineTotal,baselineMin,baselineMax);
  hist_sigmabaseline = new TH1F("hist_sigmabaseline","Overall sigma baseline distribution (all PMTs)",nBinsSigmaBaselineTotal,sigmaBaselineMin,sigmaBaselineMax);
  hist_rawamplitude = new TH1F("hist_rawamplitude","Overall raw amplitude distribution (all PMTs)",nBinsRawAmplitudeTotal,rawAmplitudeMin,rawAmplitudeMax);
  hist_amplitude = new TH1F("hist_amplitude","Overall amplitude distribution (all PMTs)",nBinsAmplitudeTotal,amplitudeMin,amplitudeMax);
  hist_rawarea = new TH1F("hist_rawarea","Overall raw area distribution (all PMTs)",nBinsRawAreaTotal,rawAreaMin,rawAreaMax);
  hist_charge_2D_y_phi = new TH2F("hist_charge_2D_y_phi","Spatial distribution of charge (all PMTs)",100,0,360,25,-2.5,2.5);
  hist_time_2D_y_phi = new TH2F("hist_time_2D_y_phi","Spatial distribution of time deviations (all PMTs)",100,0,360,25,-2.5,2.5);
  hist_time_2D_y_phi_mean = new TH2F("hist_time_2D_y_phi_mean","Spatial distribution of time (all PMTs)",100,0,360,25,-2.5,2.5);
  hist_starttime_2D_y_phi = new TH2F("hist_starttime_2D_y_phi","Spatial distribution of start time (all PMTs)",100,0,360,25,-2.5,2.5);
  hist_peaktime_2D_y_phi = new TH2F("hist_peaktime_2D_y_phi","Spatial distribution of peak time (all PMTs)",100,0,360,25,-2.5,2.5);
  hist_baseline_2D_y_phi = new TH2F("hist_baseline_2D_y_phi","Spatial distribution of baseline (all PMTs)",100,0,360,25,-2.5,2.5);
  hist_sigmabaseline_2D_y_phi = new TH2F("hist_sigmabaseline_2D_y_phi","Spatial distribution of sigma baseline (all PMTs)",100,0,360,25,-2.5,2.5);
  hist_rawamplitude_2D_y_phi = new TH2F("hist_rawamplitude_2D_y_phi","Spatial distribution of raw amplitude (all PMTs)",100,0,360,25,-2.5,2.5);
  hist_amplitude_2D_y_phi = new TH2F("hist_amplitude_2D_y_phi","Spatial distribution of amplitude (all PMTs)",100,0,360,25,-2.5,2.5);
  hist_rawarea_2D_y_phi = new TH2F("hist_rawarea_2D_y_phi","Spatial distribution of raw area (all PMTs)",100,0,360,25,-2.5,2.5);
  hist_detkey_2D_y_phi = new TH2F("hist_detkey_2D_y_phi","Spatial distribution of detkeys",100,0,360,25,-2.5,2.5);
  hist_detkey_charge = new TH1F("hist_detkey_charge","Fit mean charges vs. detkey",n_detkey_bins,pmt_detkeys[min_detkey],pmt_detkeys[max_detkey]);
  hist_detkey_time_mean = new TH1F("hist_detkey_time_mean","Fit mean times vs. detkey",n_detkey_bins,pmt_detkeys[min_detkey],pmt_detkeys[max_detkey]);
  hist_detkey_time_dev = new TH1F("hist_detkey_time_dev","Fit time deviations vs. detkey",n_detkey_bins,pmt_detkeys[min_detkey],pmt_detkeys[max_detkey]);
  hist_detkey_starttime = new TH1F("hist_detkey_starttime","Mean start times vs. detkey",n_detkey_bins,pmt_detkeys[min_detkey],pmt_detkeys[max_detkey]);
  hist_detkey_peaktime = new TH1F("hist_detkey_peaktime","Mean peak times vs. detkey",n_detkey_bins,pmt_detkeys[min_detkey],pmt_detkeys[max_detkey]);
  hist_detkey_baseline = new TH1F("hist_detkey_baseline","Mean baseline vs. detkey",n_detkey_bins,pmt_detkeys[min_detkey],pmt_detkeys[max_detkey]);
  hist_detkey_sigmabaseline = new TH1F("hist_detkey_sigmabaseline","Mean sigma baseline vs. detkey",n_detkey_bins,pmt_detkeys[min_detkey],pmt_detkeys[max_detkey]);
  hist_detkey_rawamplitude = new TH1F("hist_detkey_rawamplitude","Mean raw amplitudes vs. detkey",n_detkey_bins,pmt_detkeys[min_detkey],pmt_detkeys[max_detkey]);
  hist_detkey_amplitude = new TH1F("hist_detkey_amplitude","Mean amplitudes vs. detkey",n_detkey_bins,pmt_detkeys[min_detkey],pmt_detkeys[max_detkey]);
  hist_detkey_rawarea = new TH1F("hist_detkey_rawarea","Mean raw areas vs. detkey",n_detkey_bins,pmt_detkeys[min_detkey],pmt_detkeys[max_detkey]);

  hist_charge->GetXaxis()->SetTitle("charge");
  hist_time->GetXaxis()->SetTitle("time [ns]");
  hist_tubeid->GetXaxis()->SetTitle("detectorkey");
  hist_tubeid_adc->GetXaxis()->SetTitle("detectorkey");
  hist_starttime->GetXaxis()->SetTitle("start time [ns]");
  hist_peaktime->GetXaxis()->SetTitle("peak time [ns]");
  hist_baseline->GetXaxis()->SetTitle("baseline [ADC]");
  hist_sigmabaseline->GetXaxis()->SetTitle("sigma baseline [ADC]");
  hist_rawamplitude->GetXaxis()->SetTitle("raw amplitude [ADC]");
  hist_amplitude->GetXaxis()->SetTitle("amplitude [V]");
  hist_rawarea->GetXaxis()->SetTitle("raw area [ADC x samples]");


  for (int i_tube=0;i_tube<n_tank_pmts;i_tube++){

    unsigned long detkey = pmt_detkeys[i_tube];
    stringstream ss;
    ss<<detkey;
    string detKey=ss.str();
    string name_general_charge="hist_charge_";
    string description_general_charge="Hit charges for detkey ";
    string name_hist_charge=name_general_charge+detKey;
    string description_hist_charge=description_general_charge+detKey;
    hist_charge_singletube[detkey] = new TH1F(name_hist_charge.c_str(),description_hist_charge.c_str(),nBinsCharge,chargeMin,chargeMax);

    string name_general_time="hist_time_";
    string description_general_time="Hit times for detkey ";
    string name_hist_time=name_general_time+detKey;
    string description_hist_time=description_general_time+detKey;
    hist_time_singletube[detkey] = new TH1F(name_hist_time.c_str(),description_hist_time.c_str(),nBinsTime,timeMin,timeMax);

    string name_general_starttime="hist_starttime_";
    string description_general_starttime="Start times for detkey ";
    string name_hist_starttime=name_general_starttime+detKey;
    string description_hist_starttime=description_general_starttime+detKey;
    hist_starttime_singletube[detkey] = new TH1F(name_hist_starttime.c_str(),description_hist_starttime.c_str(),nBinsStartTime,startTimeMin,startTimeMax);  

    string name_general_peaktime="hist_peaktime_";
    string description_general_peaktime="Peak times for detkey ";
    string name_hist_peaktime=name_general_peaktime+detKey;
    string description_hist_peaktime=description_general_peaktime+detKey;
    hist_peaktime_singletube[detkey] = new TH1F(name_hist_peaktime.c_str(),description_hist_peaktime.c_str(),nBinsPeakTime,peakTimeMin,peakTimeMax);  

    string name_general_baseline="hist_baseline_";
    string description_general_baseline="Baselines for detkey ";
    string name_hist_baseline=name_general_baseline+detKey;
    string description_hist_baseline=description_general_baseline+detKey;
    hist_baseline_singletube[detkey] = new TH1F(name_hist_baseline.c_str(),description_hist_baseline.c_str(),nBinsBaseline,baselineMin,baselineMax);  

    string name_general_sigmabaseline="hist_sigmabaseline_";
    string description_general_sigmabaseline="Sigma baselines for detkey ";
    string name_hist_sigmabaseline=name_general_sigmabaseline+detKey;
    string description_hist_sigmabaseline=description_general_sigmabaseline+detKey;
    hist_sigmabaseline_singletube[detkey] = new TH1F(name_hist_sigmabaseline.c_str(),description_hist_sigmabaseline.c_str(),nBinsSigmaBaseline,sigmaBaselineMin,sigmaBaselineMax); 

    string name_general_rawamplitude="hist_rawamplitude_";
    string description_general_rawamplitude="Raw amplitudes for detkey ";
    string name_hist_rawamplitude=name_general_rawamplitude+detKey;
    string description_hist_rawamplitude=description_general_rawamplitude+detKey;
    hist_rawamplitude_singletube[detkey] = new TH1F(name_hist_rawamplitude.c_str(),description_hist_rawamplitude.c_str(),nBinsRawAmplitude,rawAmplitudeMin,rawAmplitudeMax); 

    string name_general_amplitude="hist_amplitude_";
    string description_general_amplitude="Amplitudes for detkey ";
    string name_hist_amplitude=name_general_amplitude+detKey;
    string description_hist_amplitude=description_general_amplitude+detKey;
    hist_amplitude_singletube[detkey] = new TH1F(name_hist_amplitude.c_str(),description_hist_amplitude.c_str(),nBinsAmplitude,amplitudeMin,amplitudeMax);  

    string name_general_rawarea="hist_rawarea_";
    string description_general_rawarea="Raw areas for detkey ";
    string name_hist_rawarea=name_general_rawarea+detKey;
    string description_hist_rawarea=description_general_rawarea+detKey;
    hist_rawarea_singletube[detkey] = new TH1F(name_hist_rawarea.c_str(),description_hist_rawarea.c_str(),nBinsRawArea,rawAreaMin,rawAreaMax);    

    hist_charge_singletube[detkey]->GetXaxis()->SetTitle("charge");
    hist_time_singletube[detkey]->GetXaxis()->SetTitle("time [ns]");
    hist_starttime_singletube[detkey]->GetXaxis()->SetTitle("start time [ns]");
    hist_peaktime_singletube[detkey]->GetXaxis()->SetTitle("peak time [ns]");
    hist_baseline_singletube[detkey]->GetXaxis()->SetTitle("baseline [ADC]");
    hist_sigmabaseline_singletube[detkey]->GetXaxis()->SetTitle("sigma baseline [ADC]");
    hist_rawamplitude_singletube[detkey]->GetXaxis()->SetTitle("raw amplitude [ADC]");
    hist_amplitude_singletube[detkey]->GetXaxis()->SetTitle("amplitude [V]");
    hist_rawarea_singletube[detkey]->GetXaxis()->SetTitle("raw area [ADC x samples]");

  }


  hist_charge_mean = new TH1F("hist_charge_mean","Mean values of detected charges",nBinsChargeFit,chargeFitMin,chargeFitMax);
  hist_time_mean = new TH1F("hist_time_mean","Mean values of detected hit times",nBinsTimeFit,timeFitMin,timeFitMax);
  hist_time_dev = new TH1F("hist_time_dev","Deviation of detected hit times",nBinsTimeDev,timeDevMin,timeDevMax);
  hist_starttime_mean = new TH1F("hist_starttime_mean","Mean values of detected start times",100,startTimeMin,startTimeMax);
  hist_peaktime_mean = new TH1F("hist_peaktime_mean","Mean values of detected peak times",100,peakTimeMin,peakTimeMax);
  hist_baseline_mean = new TH1F("hist_baseline_mean","Mean values of detected baselines",100,baselineMin,baselineMax);
  hist_sigmabaseline_mean = new TH1F("hist_sigmabaseline_mean","Mean values of detected sigma baselines",100,sigmaBaselineMin,sigmaBaselineMax);
  hist_rawamplitude_mean = new TH1F("hist_rawamplitude_mean","Mean values of detected raw amplitudes",100,rawAmplitudeMin,rawAmplitudeMax);
  hist_amplitude_mean = new TH1F("hist_amplitude_mean","Mean values of detected amplitudes",100,amplitudeMin,amplitudeMax);
  hist_rawarea_mean = new TH1F("hist_rawarea_mean","Mean values of detected raw areas",100,rawAreaMin,rawAreaMax);

  //---------------------------------------------------------------------------------
  //create root-file that will contain all analysis graphs for this calibration run--
  //---------------------------------------------------------------------------------

  m_data->Stores["ANNIEEvent"]->Get("RunNumber",runnumber);
  std::stringstream ss_run;
  ss_run<<runnumber;
  std::string newRunNumber = ss_run.str();
  std::string file_out_prefix="_PMTStability_Run";
  std::string file_out_root=".root";
  std::string file_out_name=outputfile+file_out_prefix+newRunNumber+file_out_root;

  file_out=new TFile(file_out_name.c_str(),"RECREATE"); //create one root file for each run to save the detailed plots and fits for all PMTs 
  file_out->cd();

  //----------------------------------------------------------------------------
  //---Make histograms appear during execution by declaring TApplication--------
  //----------------------------------------------------------------------------

  if (use_tapplication){
    if (verbose > 0) std::cout <<"Opening TApplication..."<<std::endl;
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

      std::map<unsigned long, std::vector<std::vector<ADCPulse>>> RecoADCHits; 

  //----------------------------------------------------------------------------
  //---------------get the members of the ANNIEEvent----------------------------
  //----------------------------------------------------------------------------
  m_data->Stores["ANNIEEvent"]->Get("EventNumber", evnum);
  m_data->Stores["ANNIEEvent"]->Get("BeamStatus", BeamStatus);
  bool got_recoadc = m_data->Stores["ANNIEEvent"]->Get("RecoADCHits",RecoADCHits);

  if (HitStoreName == "MCHits"){
    bool got_mchits = m_data->Stores["ANNIEEvent"]->Get("MCHits", MCHits);
    if (!got_mchits){
      std::cout << "No MCHits store in ANNIEEvent!" <<  std::endl;
      return false;
    }
  } else if (HitStoreName == "Hits"){
    bool got_hits = m_data->Stores["ANNIEEvent"]->Get("Hits", Hits);
    if (!got_hits){
      std::cout << "No Hits store in ANNIEEvent! " << std::endl;
      return false;
    }
  } else {
    std::cout << "Selected Hits store invalid.  Must be Hits or MCHits" << std::endl;
    return false;
  }
  // Also load hits from the Hits Store, if available

  //----------------------------------------------------------------------------
  //---------------Read out charge and hit values of PMTs-----------------------
  //----------------------------------------------------------------------------
  
  for (int i_pmt = 0; i_pmt < n_tank_pmts; i_pmt++){
    unsigned long detkey = pmt_detkeys[i_pmt];
    PMT_ishit[detkey] = 0;
  }

  if(HitStoreName=="MCHits"){
    int vectsize = MCHits->size();
    if (verbose > 0) std::cout <<"MCHits size: "<<vectsize<<std::endl; 
    for(std::pair<unsigned long, std::vector<MCHit>>&& apair : *MCHits){
      unsigned long chankey = apair.first;
      Detector* thistube = geom->ChannelToDetector(chankey);
      int detectorkey = thistube->GetDetectorID();
      if (thistube->GetDetectorElement()=="Tank"){
        std::vector<MCHit>& ThisPMTHits = apair.second;
        PMT_ishit[detectorkey] = 1;
        for (MCHit &ahit : ThisPMTHits){
          if (verbose > 2) std::cout <<"Charge "<<ahit.GetCharge()<<", time "<<ahit.GetTime()<<std::endl;
          hist_charge->Fill(ahit.GetCharge());
          hist_time->Fill(ahit.GetTime());
          hist_tubeid->Fill(detectorkey);
          hist_charge_singletube[detectorkey]->Fill(ahit.GetCharge());
          hist_time_singletube[detectorkey]->Fill(ahit.GetTime());
        }
      }
    }
  }
  
  if(HitStoreName=="Hits"){
    int vectsize = Hits->size();
    if (verbose > 0) std::cout <<"Hits size: "<<vectsize<<std::endl; 
    for(std::pair<unsigned long, std::vector<Hit>>&& apair : *Hits){
      unsigned long chankey = apair.first;
      Detector* thistube = geom->ChannelToDetector(chankey);
      int detectorkey = thistube->GetDetectorID();
      if (thistube->GetDetectorElement()=="Tank"){
        std::vector<Hit>& ThisPMTHits = apair.second;
        PMT_ishit[detectorkey] = 1;
        for (Hit &ahit : ThisPMTHits){
          if (verbose > 2) std::cout <<"Charge "<<ahit.GetCharge()<<", time "<<ahit.GetTime()<<std::endl;
          hist_charge->Fill(ahit.GetCharge());
          hist_time->Fill(ahit.GetTime());
          hist_tubeid->Fill(detectorkey);
          hist_charge_singletube[detectorkey]->Fill(ahit.GetCharge());
          hist_time_singletube[detectorkey]->Fill(ahit.GetTime());
        }
      }
    }
  }

  //check whether PMT_ishit is filled correctly
  for (int i_pmt = 0; i_pmt < n_tank_pmts ; i_pmt++){
    unsigned long detkey = pmt_detkeys[i_pmt];  
    if (verbose > 2) std::cout <<"PMT "<<i_pmt<<" is hit: "<<PMT_ishit[detkey]<<std::endl;
  }

  //----------------------------------------------------------------------------
  //---------------Read out RecoADCHits properties of PMTs----------------------
  //---------------------------------------------------------------------------- 

  if (got_recoadc){

    int recoadcsize = RecoADCHits.size();
    int adc_loop = 0;
    if (verbose > 0) std::cout <<"RecoADCHits size: "<<recoadcsize<<std::endl;
    for (std::pair<unsigned long, std::vector<std::vector<ADCPulse>>> apair : RecoADCHits){
      unsigned long chankey = apair.first;
      Detector *thistube = geom->ChannelToDetector(chankey);
      int detectorkey = thistube->GetDetectorID();
      if (thistube->GetDetectorElement()=="Tank"){
        std::vector<std::vector<ADCPulse>> pulses = apair.second;
        for (int i_minibuffer = 0; i_minibuffer < pulses.size(); i_minibuffer++){
          std::vector<ADCPulse> apulsevector = pulses.at(i_minibuffer);
          for (int i_pulse=0; i_pulse < apulsevector.size(); i_pulse++){
            ADCPulse apulse = apulsevector.at(i_pulse);
            double start_time = apulse.start_time();
            double peak_time = apulse.peak_time();
            double baseline = apulse.baseline();
            double sigma_baseline = apulse.sigma_baseline();
            double raw_amplitude = apulse.raw_amplitude();
            double amplitude = apulse.amplitude();
            double raw_area = apulse.raw_area();
            hist_starttime->Fill(start_time);
            hist_peaktime->Fill(peak_time);
            hist_baseline->Fill(baseline);
            hist_sigmabaseline->Fill(sigma_baseline);
            hist_rawamplitude->Fill(raw_amplitude);
            hist_amplitude->Fill(amplitude);
            hist_rawarea->Fill(raw_area);
            hist_tubeid_adc->Fill(detectorkey);
            hist_starttime_singletube[detectorkey]->Fill(start_time);
            hist_peaktime_singletube[detectorkey]->Fill(peak_time);
            hist_baseline_singletube[detectorkey]->Fill(baseline);
            hist_sigmabaseline_singletube[detectorkey]->Fill(sigma_baseline);
            hist_rawamplitude_singletube[detectorkey]->Fill(raw_amplitude);
            hist_amplitude_singletube[detectorkey]->Fill(amplitude);
            hist_rawarea_singletube[detectorkey]->Fill(raw_area);
          }
        }
      }else {
        if (verbose > 0) std::cout <<"TankCalibrationDiffuser: RecoADCHit does not belong to a tank PMT and is ommited. Detector key/element = "<<detectorkey<<" / "<<thistube->GetDetectorElement()<<std::endl;
      }
      adc_loop++;

    }



  } else {

    std::cout <<"TankCalibrationDiffuser: RecoADCHits Store does not exist and is not read out"<<std::endl;
  }

  return true;
}


bool TankCalibrationDiffuser::Finalise(){

  if (verbose > 0) std::cout <<"TankCalibrationDiffuser: Finalise"<<std::endl;

  std::stringstream ss_run;
  ss_run<<runnumber;
  std::string newRunNumber = ss_run.str();

  //----------------------------------------------------------------------------
  //---------------Create and write single run root files------------------------
  //----------------------------------------------------------------------------

  //cout <<"pointer histogram charge: "<<hist_charge<<endl;
  //cout <<"pointer singletube hist charge: "<<hist_charge_singletube[23]<<endl;
  //hist_charge->Print();
  hist_charge->Write();
  hist_time->Write();
  hist_tubeid->Write();
  hist_tubeid_adc->Write();
  hist_starttime->Write();
  hist_peaktime->Write();
  hist_baseline->Write();
  hist_sigmabaseline->Write();
  hist_rawamplitude->Write();
  hist_amplitude->Write();
  hist_rawarea->Write();

  // create txt file to store calibration data for the specific run
  std::string filename_pre = "_Run";
  std::string filename_post = "_pmts_laser_calibration.txt";
  std::string filename_complete = outputfile+filename_pre+newRunNumber+filename_post;
  std::cout <<"writing to txt file: "<<filename_complete<<std::endl;
  ofstream result_file(filename_complete.c_str());

  //----------------------------------------------------------------------------
  //---------------Perform fits for PMT distributions---------------------------
  //----------------------------------------------------------------------------

  for (int i_tube=0;i_tube<n_tank_pmts;i_tube++){

    unsigned long detkey = pmt_detkeys[i_tube];
    double mean_charge=hist_charge_singletube[detkey]->GetMean();

    Double_t par_gaus2exp[8] = {gaus1Constant,gaus1Mean,gaus1Sigma,gaus2Constant,gaus2Mean,gaus2Sigma,expConstant,expDecay};    //old default: {10,0.3,0.1,10,1.0,0.5,1,-1}
    Double_t par_gaus2[6] = {gaus1Constant,gaus1Mean,gaus1Sigma,gaus2Constant,gaus2Mean,gaus2Sigma};  //old default: {10,0.3,0.1,10,1.0,0.5}
    Double_t par_gaus[3] = {gaus1Constant,gaus1Mean,gaus1Sigma};    //old default: {10,1.0,0.5}

    TF1 *total;
    if (FitMethod == "Gaus2Exp") total = new TF1("total","gaus(0)+gaus(3)+expo(6)",chargeMin,chargeMax);
    else if (FitMethod == "Gaus2") total = new TF1("total","gaus(0)+gaus(3)",chargeMin,chargeMax);
    else if (FitMethod == "Gaus") total = new TF1("total","gaus",chargeMin,chargeMax);
    else {
      std::cout <<"ERROR (TankCalibrationDiffuser): FitFunction is not part of the options, please extend the options Using standard Gaus."<<std::endl;
      total = new TF1("total","gaus",chargeMin,chargeMax);
    }
    total->SetLineColor(2);
    if (FitMethod == "Gaus2Exp") total->SetParameters(par_gaus2exp);
    else if (FitMethod == "Gaus2") total->SetParameters(par_gaus2);
    else total->SetParameters(par_gaus2);

    //total = new TF1("total",total_value,0,10,8);
    //total->SetParameters(10,0.3,0.1,10,1.,0.3,1,1);
    TFitResultPtr FitResult = hist_charge_singletube[detkey]->Fit(total,"QR+");
    Int_t FitResult_int = FitResult;

    //hist_charge_singletube[i_tube]->Fit("gaus[0]+gaus[3]+exp[6]");
    hist_charge_singletube[detkey]->Write();
    if (FitResult_int ==0){
      TF1 *fit_result_charge=hist_charge_singletube[detkey]->GetFunction("total");
      if (FitMethod == "Gaus2Exp") {
        mean_charge_fit[detkey] = fit_result_charge->GetParameter(4);
        rms_charge_fit[detkey] = fit_result_charge->GetParameter(5);
      } 
      else if (FitMethod == "Gaus2") {
        mean_charge_fit[detkey]=fit_result_charge->GetParameter(4);
        mean_charge_fit[detkey]=fit_result_charge->GetParameter(4);
      }
      else {
      mean_charge_fit[detkey]=fit_result_charge->GetParameter(1);
      rms_charge_fit[detkey]=fit_result_charge->GetParameter(2);
      }
    } else {
      mean_charge_fit[detkey] = 0.;
      rms_charge_fit[detkey] = 0.;
    }
    hist_charge_mean->Fill(mean_charge_fit[detkey]);  

    double mean_time=hist_time_singletube[detkey]->GetMean();
    TFitResultPtr FitResultTime = hist_time_singletube[detkey]->Fit("gaus","Q");      //time fit is always the same for now, assume simple Gaussian
    Int_t FitResultTime_int = FitResultTime;
    hist_time_singletube[detkey]->Write();
    if (FitResultTime_int == 0) { //fit was okay and has a result
      TF1 *fit_result_time=hist_time_singletube[detkey]->GetFunction("gaus");
      mean_time_fit[detkey]=fit_result_time->GetParameter(1);
      rms_time_fit[detkey]=fit_result_time->GetParameter(2);
    }
    else {
      mean_time_fit[detkey] = 0.;
      rms_time_fit[detkey] = 0.;
    }
    hist_time_dev->Fill(mean_time_fit[detkey]-expected_time[detkey]);
    hist_time_mean->Fill(mean_time_fit[detkey]);

    if (verbose > 2){
      std::cout <<"expected hit time: "<<expected_time[detkey]<<endl;
      std::cout <<"detected hit time: "<<mean_time_fit[detkey]<<endl;
      std::cout << "deviation: "<<mean_time_fit[detkey]-expected_time[detkey]<<endl;
    }

    if (mean_charge_fit[detkey] < 0.){        //unphysical charge information --> set to 0
      mean_charge_fit[detkey] = 0.;
      rms_charge_fit[detkey] = 0.;
    }

    //
    //write ADCRecoPulse histograms to file as well
    //


    hist_starttime_singletube[detkey]->Write();
    hist_peaktime_singletube[detkey]->Write();
    hist_baseline_singletube[detkey]->Write();
    hist_sigmabaseline_singletube[detkey]->Write();
    hist_rawamplitude_singletube[detkey]->Write();
    hist_amplitude_singletube[detkey]->Write();
    hist_rawarea_singletube[detkey]->Write();

    starttime_mean[detkey] = hist_starttime_singletube[detkey]->GetMean();
    peaktime_mean[detkey] = hist_peaktime_singletube[detkey]->GetMean();
    baseline_mean[detkey] = hist_baseline_singletube[detkey]->GetMean();
    sigmabaseline_mean[detkey] = hist_sigmabaseline_singletube[detkey]->GetMean();
    rawamplitude_mean[detkey] = hist_rawamplitude_singletube[detkey]->GetMean();
    amplitude_mean[detkey] = hist_amplitude_singletube[detkey]->GetMean();
    rawarea_mean[detkey] = hist_rawarea_singletube[detkey]->GetMean();
	
    hist_starttime_mean->Fill(starttime_mean[detkey]);
    hist_peaktime_mean->Fill(peaktime_mean[detkey]);
    hist_baseline_mean->Fill(baseline_mean[detkey]);
    hist_sigmabaseline_mean->Fill(sigmabaseline_mean[detkey]);
    hist_rawamplitude_mean->Fill(rawamplitude_mean[detkey]);
    hist_amplitude_mean->Fill(amplitude_mean[detkey]);
    hist_rawarea_mean->Fill(rawarea_mean[detkey]);
	
    //
    //fill spatial detector plots as well
    //

    if (hist_time_2D_y_phi->GetBinContent(int(phi_PMT[detkey]/2/TMath::Pi()*100)+1,int((y_PMT[detkey]+2.5)/5.*25)+1)==0){
      hist_time_2D_y_phi->SetBinContent(int(phi_PMT[detkey]/2/TMath::Pi()*100)+1,int((y_PMT[detkey]+2.5)/5.*25)+1,fabs(mean_time_fit[detkey]-expected_time[detkey]));  
      hist_time_2D_y_phi_mean->SetBinContent(int(phi_PMT[detkey]/2/TMath::Pi()*100)+1,int((y_PMT[detkey]+2.5)/5.*25)+1,mean_time_fit[detkey]);  
      hist_charge_2D_y_phi->SetBinContent(int(phi_PMT[detkey]/2/TMath::Pi()*100)+1,int((y_PMT[detkey]+2.5)/5.*25)+1,mean_charge_fit[detkey]);
      hist_starttime_2D_y_phi->SetBinContent(int(phi_PMT[detkey]/2/TMath::Pi()*100)+1,int((y_PMT[detkey]+2.5)/5.*25)+1,starttime_mean[detkey]);
      hist_peaktime_2D_y_phi->SetBinContent(int(phi_PMT[detkey]/2/TMath::Pi()*100)+1,int((y_PMT[detkey]+2.5)/5.*25)+1,peaktime_mean[detkey]);
      hist_baseline_2D_y_phi->SetBinContent(int(phi_PMT[detkey]/2/TMath::Pi()*100)+1,int((y_PMT[detkey]+2.5)/5.*25)+1,baseline_mean[detkey]);
      hist_sigmabaseline_2D_y_phi->SetBinContent(int(phi_PMT[detkey]/2/TMath::Pi()*100)+1,int((y_PMT[detkey]+2.5)/5.*25)+1,sigmabaseline_mean[detkey]);
      hist_rawamplitude_2D_y_phi->SetBinContent(int(phi_PMT[detkey]/2/TMath::Pi()*100)+1,int((y_PMT[detkey]+2.5)/5.*25)+1,rawamplitude_mean[detkey]);
      hist_amplitude_2D_y_phi->SetBinContent(int(phi_PMT[detkey]/2/TMath::Pi()*100)+1,int((y_PMT[detkey]+2.5)/5.*25)+1,amplitude_mean[detkey]);
      hist_rawarea_2D_y_phi->SetBinContent(int(phi_PMT[detkey]/2/TMath::Pi()*100)+1,int((y_PMT[detkey]+2.5)/5.*25)+1,rawarea_mean[detkey]);
      hist_detkey_2D_y_phi->SetBinContent(int(phi_PMT[detkey]/2/TMath::Pi()*100)+1,int((y_PMT[detkey]+2.5)/5.*25)+1,detkey);    
    }
    else {
      hist_time_2D_y_phi->SetBinContent(int(phi_PMT[detkey]/2/TMath::Pi()*100)+1,int((y_PMT[detkey]+2.5)/5.*25)+2,fabs(mean_time_fit[detkey]-expected_time[detkey]));
      hist_time_2D_y_phi_mean->SetBinContent(int(phi_PMT[detkey]/2/TMath::Pi()*100)+1,int((y_PMT[detkey]+2.5)/5.*25)+2,mean_time_fit[detkey]);
      hist_charge_2D_y_phi->SetBinContent(int(phi_PMT[detkey]/2/TMath::Pi()*100)+1,int((y_PMT[detkey]+2.5)/5.*25)+2,mean_charge_fit[detkey]);
      hist_starttime_2D_y_phi->SetBinContent(int(phi_PMT[detkey]/2/TMath::Pi()*100)+1,int((y_PMT[detkey]+2.5)/5.*25)+2,starttime_mean[detkey]);
      hist_peaktime_2D_y_phi->SetBinContent(int(phi_PMT[detkey]/2/TMath::Pi()*100)+1,int((y_PMT[detkey]+2.5)/5.*25)+2,peaktime_mean[detkey]);
      hist_baseline_2D_y_phi->SetBinContent(int(phi_PMT[detkey]/2/TMath::Pi()*100)+1,int((y_PMT[detkey]+2.5)/5.*25)+2,baseline_mean[detkey]);
      hist_sigmabaseline_2D_y_phi->SetBinContent(int(phi_PMT[detkey]/2/TMath::Pi()*100)+1,int((y_PMT[detkey]+2.5)/5.*25)+2,sigmabaseline_mean[detkey]);
      hist_rawamplitude_2D_y_phi->SetBinContent(int(phi_PMT[detkey]/2/TMath::Pi()*100)+1,int((y_PMT[detkey]+2.5)/5.*25)+2,rawamplitude_mean[detkey]);
      hist_amplitude_2D_y_phi->SetBinContent(int(phi_PMT[detkey]/2/TMath::Pi()*100)+1,int((y_PMT[detkey]+2.5)/5.*25)+2,amplitude_mean[detkey]);
      hist_rawarea_2D_y_phi->SetBinContent(int(phi_PMT[detkey]/2/TMath::Pi()*100)+1,int((y_PMT[detkey]+2.5)/5.*25)+2,rawarea_mean[detkey]);
      hist_detkey_2D_y_phi->SetBinContent(int(phi_PMT[detkey]/2/TMath::Pi()*100)+1,int((y_PMT[detkey]+2.5)/5.*25)+2,detkey);
    }
    result_file<<detkey<<"  "<<mean_charge_fit[detkey]<<"  "<<rms_charge_fit[detkey]<<"  "<<mean_time_fit[detkey]<<"  "<<rms_time_fit[detkey]<<"  "<<mean_time_fit[detkey]-expected_time[detkey]<<endl;

    //
    //fill detkey 1D histograms as well
    //

    int bin_nr = hist_detkey_charge->FindBin(detkey);
    hist_detkey_charge->SetBinContent(bin_nr,mean_charge_fit[detkey]);
    hist_detkey_time_mean->SetBinContent(bin_nr,mean_time_fit[detkey]);
    hist_detkey_time_dev->SetBinContent(bin_nr,mean_time_fit[detkey]-expected_time[detkey]);
    hist_detkey_starttime->SetBinContent(bin_nr,starttime_mean[detkey]);
    hist_detkey_peaktime->SetBinContent(bin_nr,peaktime_mean[detkey]);
    hist_detkey_baseline->SetBinContent(bin_nr,baseline_mean[detkey]);
    hist_detkey_sigmabaseline->SetBinContent(bin_nr,sigmabaseline_mean[detkey]);
    hist_detkey_rawamplitude->SetBinContent(bin_nr,rawamplitude_mean[detkey]);
    hist_detkey_amplitude->SetBinContent(bin_nr,amplitude_mean[detkey]);
    hist_detkey_rawarea->SetBinContent(bin_nr,rawarea_mean[detkey]);

    vector_tf1.push_back(total);
  }
  result_file<<1000<<"  "<<hist_charge_mean->GetMean()<<"  "<<hist_charge_mean->GetRMS()<<"  "<<hist_time_mean->GetMean()<<"  "<<hist_time_mean->GetRMS()<<"  "<<hist_time_dev->GetMean()<<"  "<<hist_time_dev->GetRMS()<<endl; //1000 is identifier key for average value
  result_file.close();

  hist_time_mean->GetXaxis()->SetTitle("t_{arrival} [ns]");
  hist_time_dev->GetXaxis()->SetTitle("t_{arrival} - t_{expected} [ns]");
  hist_charge_mean->GetXaxis()->SetTitle("charge [p.e.]");
  hist_starttime_mean->GetXaxis()->SetTitle("start time [ns]");
  hist_peaktime_mean->GetXaxis()->SetTitle("peak time [ns]");
  hist_baseline_mean->GetXaxis()->SetTitle("baseline [ADC]");
  hist_sigmabaseline_mean->GetXaxis()->SetTitle("sigma baseline [ADC]");
  hist_rawamplitude_mean->GetXaxis()->SetTitle("raw amplitude [ADC]");
  hist_amplitude_mean->GetXaxis()->SetTitle("amplitude [V]");
  hist_rawarea_mean->GetXaxis()->SetTitle("raw area [ADC x pulses]");

  hist_charge_2D_y_phi->GetXaxis()->SetTitle("#phi [deg]");
  hist_charge_2D_y_phi->GetYaxis()->SetTitle("y [m]");
  hist_charge_2D_y_phi->SetStats(0);
  hist_detkey_2D_y_phi->GetXaxis()->SetTitle("#phi [deg]");
  hist_detkey_2D_y_phi->GetYaxis()->SetTitle("y [m]");
  hist_detkey_2D_y_phi->SetStats(0);
  hist_detkey_2D_y_phi->SetDrawOption("colz text");
  hist_time_2D_y_phi->GetXaxis()->SetTitle("#phi [deg]");
  hist_time_2D_y_phi->GetYaxis()->SetTitle("y [m]");
  hist_time_2D_y_phi->SetStats(0);
  hist_time_2D_y_phi_mean->GetXaxis()->SetTitle("#phi [deg]");
  hist_time_2D_y_phi_mean->GetYaxis()->SetTitle("y [m]");
  hist_time_2D_y_phi_mean->SetStats(0);
  hist_starttime_2D_y_phi->GetXaxis()->SetTitle("#phi [deg]");
  hist_starttime_2D_y_phi->GetYaxis()->SetTitle("y [m]");
  hist_starttime_2D_y_phi->SetStats(0);
  hist_peaktime_2D_y_phi->GetXaxis()->SetTitle("#phi [deg]");
  hist_peaktime_2D_y_phi->GetYaxis()->SetTitle("y [m]");
  hist_peaktime_2D_y_phi->SetStats(0);
  hist_baseline_2D_y_phi->GetXaxis()->SetTitle("#phi [deg]");
  hist_baseline_2D_y_phi->GetYaxis()->SetTitle("y [m]");
  hist_baseline_2D_y_phi->SetStats(0);
  hist_sigmabaseline_2D_y_phi->GetXaxis()->SetTitle("#phi [deg]");
  hist_sigmabaseline_2D_y_phi->GetYaxis()->SetTitle("y [m]");
  hist_sigmabaseline_2D_y_phi->SetStats(0);
  hist_rawamplitude_2D_y_phi->GetXaxis()->SetTitle("#phi [deg]");
  hist_rawamplitude_2D_y_phi->GetYaxis()->SetTitle("y [m]");
  hist_rawamplitude_2D_y_phi->SetStats(0);
  hist_amplitude_2D_y_phi->GetXaxis()->SetTitle("#phi [deg]");
  hist_amplitude_2D_y_phi->GetYaxis()->SetTitle("y [m]");
  hist_amplitude_2D_y_phi->SetStats(0);
  hist_rawarea_2D_y_phi->GetXaxis()->SetTitle("#phi [deg]");
  hist_rawarea_2D_y_phi->GetYaxis()->SetTitle("y [m]");
  hist_rawarea_2D_y_phi->SetStats(0);

  hist_detkey_charge->GetXaxis()->SetTitle("detkey");
  hist_detkey_charge->GetYaxis()->SetTitle("Fit mean charge");
  hist_detkey_charge->SetStats(0);
  hist_detkey_time_mean->GetXaxis()->SetTitle("detkey");
  hist_detkey_time_mean->GetYaxis()->SetTitle("Fit mean time [ns]");
  hist_detkey_time_mean->SetStats(0);
  hist_detkey_time_dev->GetXaxis()->SetTitle("detkey");
  hist_detkey_time_dev->GetYaxis()->SetTitle("Fit time deviation [ns]");
  hist_detkey_time_dev->SetStats(0);
  hist_detkey_starttime->GetXaxis()->SetTitle("detkey");
  hist_detkey_starttime->GetYaxis()->SetTitle("Mean start time [ns]");
  hist_detkey_starttime->SetStats(0);
  hist_detkey_peaktime->GetXaxis()->SetTitle("detkey");
  hist_detkey_peaktime->GetYaxis()->SetTitle("Mean peak time [ns]");
  hist_detkey_peaktime->SetStats(0);
  hist_detkey_baseline->GetXaxis()->SetTitle("detkey");
  hist_detkey_baseline->GetYaxis()->SetTitle("Mean baseline [ADC]");
  hist_detkey_baseline->SetStats(0);
  hist_detkey_sigmabaseline->GetXaxis()->SetTitle("detkey");
  hist_detkey_sigmabaseline->GetYaxis()->SetTitle("Mean sigma baseline [ADC]");
  hist_detkey_sigmabaseline->SetStats(0);
  hist_detkey_rawamplitude->GetXaxis()->SetTitle("detkey");
  hist_detkey_rawamplitude->GetYaxis()->SetTitle("Mean raw amplitude [ADC]");
  hist_detkey_rawamplitude->SetStats(0);
  hist_detkey_amplitude->GetXaxis()->SetTitle("detkey");
  hist_detkey_amplitude->GetYaxis()->SetTitle("Mean amplitude [V]");
  hist_detkey_amplitude->SetStats(0);
  hist_detkey_rawarea->GetXaxis()->SetTitle("detkey");
  hist_detkey_rawarea->GetYaxis()->SetTitle("Mean raw area [ADC x samples]");
  hist_detkey_rawarea->SetStats(0);

  hist_time_mean->Write();
  hist_time_dev->Write();
  hist_charge_mean->Write();
  hist_starttime_mean->Write();
  hist_peaktime_mean->Write();
  hist_baseline_mean->Write();
  hist_sigmabaseline_mean->Write();
  hist_rawamplitude_mean->Write();
  hist_amplitude_mean->Write();
  hist_rawarea_mean->Write();

  hist_time_2D_y_phi->Write();
  hist_time_2D_y_phi_mean->Write();
  hist_charge_2D_y_phi->Write();
  hist_starttime_2D_y_phi->Write();
  hist_peaktime_2D_y_phi->Write();
  hist_baseline_2D_y_phi->Write();
  hist_sigmabaseline_2D_y_phi->Write();
  hist_rawamplitude_2D_y_phi->Write();
  hist_amplitude_2D_y_phi->Write();
  hist_rawarea_2D_y_phi->Write();
  hist_detkey_2D_y_phi->Write();

  hist_detkey_charge->Write();
  hist_detkey_time_mean->Write();
  hist_detkey_time_dev->Write();
  hist_detkey_starttime->Write();
  hist_detkey_peaktime->Write();
  hist_detkey_baseline->Write();
  hist_detkey_sigmabaseline->Write();
  hist_detkey_rawamplitude->Write();
  hist_detkey_amplitude->Write();
  hist_detkey_rawarea->Write();



  //----------------------------------------------------------------------------
  //---------------Create and write stability plots------------------------
  //----------------------------------------------------------------------------

  gr_stability = new TGraphErrors();
  gr_stability->SetName("gr_stability");
  gr_stability->SetTitle("Stability PMT charge calibration");
  gr_stability_time = new TGraphErrors();
  gr_stability_time->SetName("gr_stability_time");
  gr_stability_time->SetTitle("Stability PMT time calibration");
  gr_stability_time_mean = new TGraphErrors();
  gr_stability_time_mean->SetName("gr_stability_time_mean");
  gr_stability_time_mean->SetTitle("Stability PMT time (mean)");

  //
  //read in information from 100 last runs to produce stability/time evolution plots (if they do not exist, just skip them...)
  //

  const int n_entries=100;
  double run_numbers[n_entries];
  double entries_charge[n_entries];
  double rms_charge[n_entries];
  double entries_time[n_entries];
  double rms_time[n_entries];
  double entries_time_mean[n_entries];
  double rms_time_mean[n_entries];
  bool file_exists[n_entries];

  int runnumber_temp;

  //
  //get stability values for time and charge from txt-files
  //

  for (int i_run=n_entries-1;i_run>=0;i_run--){

    std::stringstream ss_runnumber;
    runnumber_temp = runnumber-i_run;
    ss_runnumber<<runnumber_temp;
    std::string filename_mid = ss_runnumber.str();
    std::string filename_temp = outputfile+filename_pre+filename_mid+filename_post;
    if (verbose > 1) std::cout <<"Stability: Filename: "<<filename_temp<<", file exists: "<<does_file_exist(filename_temp.c_str())<<std::endl;
    double dummy_temp;
    if (does_file_exist(filename_temp.c_str())){
      ifstream file_temp(filename_temp.c_str());
      file_exists[n_entries-1-i_run]=true;
      for (int i_pmt=0;i_pmt<n_tank_pmts+1;i_pmt++){
        if (i_pmt<n_tank_pmts) file_temp>>dummy_temp>>dummy_temp>>dummy_temp>>dummy_temp>>dummy_temp>>dummy_temp;
        else file_temp>>dummy_temp>>entries_charge[n_entries-1-i_run]>>rms_charge[n_entries-1-i_run]>>entries_time_mean[n_entries-1-i_run]>>rms_time_mean[n_entries-1-i_run]>>entries_time[n_entries-1-i_run]>>rms_time[n_entries-1-i_run];
      }
      file_temp.close();
    }
    else {
      file_exists[n_entries-1-i_run]=false;
    }
  }

  //
  //fill TGraphErrors with stability values for time and charge
  //

  int i_point_graph=0;
  for (int i_run=n_entries-1;i_run>=0;i_run--){
    if (file_exists[n_entries-1-i_run]){
      if (verbose > 1) std::cout <<"Stability, Run "<<n_entries-1-i_run<<": Charge: "<<entries_charge[n_entries-1-i_run]<<", rms charge: "<<rms_charge[n_entries-1-i_run]<<", time: "<<entries_time_mean[n_entries-1-i_run]<<"rms time: "<<rms_time_mean[n_entries-1-i_run]<<", time deviation: "<<entries_time_mean[n_entries-1-i_run]<<"rms time deviation: "<<rms_time_mean[n_entries-1-i_run]<<std::endl;
      gr_stability->SetPoint(i_point_graph,runnumber-i_run,entries_charge[n_entries-1-i_run]);
      gr_stability->SetPointError(i_point_graph,0.,rms_charge[n_entries-1-i_run]);
      gr_stability_time->SetPoint(i_point_graph,runnumber-i_run,entries_time[n_entries-1-i_run]);
      gr_stability_time->SetPointError(i_point_graph,0.,rms_time[n_entries-1-i_run]);
      gr_stability_time_mean->SetPoint(i_point_graph,runnumber-i_run,entries_time_mean[n_entries-1-i_run]);
      gr_stability_time_mean->SetPointError(i_point_graph,0.,rms_time_mean[n_entries-1-i_run]);
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
  gr_stability_time_mean->SetMarkerStyle(22);
  gr_stability_time_mean->SetMarkerSize(1.0);
  gr_stability_time_mean->SetLineColor(1);
  gr_stability_time_mean->SetLineWidth(2);
  gr_stability_time_mean->GetXaxis()->SetTitle("Run Number");
  gr_stability_time_mean->GetYaxis()->SetTitle("hit time [ns]");
  gr_stability->Write("gr_stability");
  gr_stability_time->Write("gr_stability_time");
  gr_stability_time_mean->Write("gr_stability_time_mean");

  //
  //Fill new entries into histogram
  //

  std::map<int,double>::iterator it_time = bad_time.begin();
  std::map<int,double>::iterator it_charge = bad_charge.begin();

  std::cout <<"///////////////////////////////////////////////////////////////////"<<std::endl;
  std::cout <<"-------------------------------------------------------------------"<<std::endl;
  std::cout <<"-------------PMTs with deviating properties:-----------------------"<<std::endl;
  std::cout <<"-------------------------------------------------------------------"<<std::endl;  
  std::cout <<"///////////////////////////////////////////////////////////////////"<<std::endl;

  for (int i_tube=0;i_tube<n_tank_pmts;i_tube++){
    unsigned long detkey = pmt_detkeys[i_tube];
    if (fabs(mean_time_fit[detkey]-expected_time[detkey])>tolerance_time) {
      std::cout <<"Abnormally high time deviation for detkey "<<detkey<<": "<<mean_time_fit[detkey]-expected_time[detkey]<<std::endl;
      bad_time.insert(it_time,std::pair<int,double>(detkey,mean_time_fit[detkey]-expected_time[detkey]));    
    }
    if (mean_charge_fit[detkey]<(1.0-tolerance_charge) || mean_charge_fit[detkey]>(1.0+tolerance_charge)){
      std::cout <<"Abnormal charge for detkey "<<detkey<<": "<<mean_charge_fit[detkey]<<std::endl;
      bad_charge.insert(it_charge,std::pair<int,double>(detkey,mean_charge_fit[detkey])); 
    }
  }

  //----------------------------------------------------------------------------
  //---------------Save summary root plots--------------------------------------
  //----------------------------------------------------------------------------

  canvas_overview = new TCanvas("canvas_overview_QTdev","Stability PMTs (Deviations)",900,600);
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
  hist_time_dev->Draw();
  canvas_overview->cd(6);
  gr_stability_time->Draw("ALE");
  canvas_overview->Modified();
  canvas_overview->Update();
  canvas_overview->Write();

  //----------------------------------------------------------------------------
  //---------------Save summary root plots v2 (no deviations)-----------------
  //----------------------------------------------------------------------------

  canvas_overview2 = new TCanvas("canvas_overview_QT","Stability PMTs (Mean)",900,600);
  canvas_overview2->Divide(3,2);
  canvas_overview2->cd(1);
  hist_charge_2D_y_phi->Draw("colz");
  canvas_overview2->cd(2);
  hist_charge_mean->Draw();
  canvas_overview2->cd(3);
  gr_stability->Draw("AEL");
  canvas_overview2->cd(4);
  hist_time_2D_y_phi_mean->Draw("colz");
  canvas_overview2->cd(5);
  hist_time_mean->Draw();
  canvas_overview2->cd(6);
  gr_stability_time_mean->Draw("ALE");
  canvas_overview2->Modified();
  canvas_overview2->Update();
  canvas_overview2->Write();

  //----------------------------------------------------------------------------
  //---------------Save summary root plots v3 (RecoADC Values)----------------
  //----------------------------------------------------------------------------

  canvas_overview3 = new TCanvas("canvas_overview_recoadc","Stability PMTs (RecoADC)",900,1000);
  canvas_overview3->Divide(4,2);
  canvas_overview3->cd(1);
  hist_starttime_2D_y_phi->Draw("colz");
  canvas_overview3->cd(2);
  hist_peaktime_2D_y_phi->Draw("colz");
  canvas_overview3->cd(3);
  hist_baseline_2D_y_phi->Draw("colz");
  canvas_overview3->cd(4);
  hist_sigmabaseline_2D_y_phi->Draw("colz");
  canvas_overview3->cd(5);
  hist_rawamplitude_2D_y_phi->Draw("colz");
  canvas_overview3->cd(6);
  hist_amplitude_2D_y_phi->Draw("colz");
  canvas_overview3->cd(7);
  hist_rawarea_2D_y_phi->Draw("colz");
  canvas_overview3->cd(8);
  hist_detkey_2D_y_phi->Draw("colz text");
  canvas_overview3->Modified();
  canvas_overview3->Update();
  canvas_overview3->Write();
  
  //----------------------------------------------------------------------------
  //---------------Save summary root plots v4 (RecoADC Values,1D)---------------
  //----------------------------------------------------------------------------

  canvas_overview4 = new TCanvas("canvas_overview_recoadc_1D","Stability PMTs (RecoADC)",900,1000);
  canvas_overview4->Divide(4,2);
  canvas_overview4->cd(1);
  hist_detkey_starttime->Draw();
  canvas_overview4->cd(2);
  hist_detkey_peaktime->Draw();
  canvas_overview4->cd(3);
  hist_detkey_baseline->Draw();
  canvas_overview4->cd(4);
  hist_detkey_sigmabaseline->Draw();
  canvas_overview4->cd(5);
  hist_detkey_rawamplitude->Draw();
  canvas_overview4->cd(6);
  hist_detkey_amplitude->Draw();
  canvas_overview4->cd(7);
  hist_detkey_rawarea->Draw();
  canvas_overview4->Modified();
  canvas_overview4->Update();
  canvas_overview4->Write();

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

  //Histograms already deleted by closing the files ealier

  if (verbose > 0) std::cout <<"Deleting TBoxes..."<<std::endl;
  for (int i_box = 0; i_box < vector_tbox.size();i_box++){
    delete vector_tbox.at(i_box);
  }
  
  if (verbose > 0) std::cout <<"Deleting TF1s..."<<std::endl;
  for (int i_tf = 0; i_tf< vector_tf1.size(); i_tf++){
    delete vector_tf1.at(i_tf);
  } 

  if (verbose > 0) std::cout <<"Deleting canvas, application, files, geom, mchits"<<std::endl;
  delete canvas_overview;
  delete canvas_overview2;
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
