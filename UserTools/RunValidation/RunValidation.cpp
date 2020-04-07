#include "RunValidation.h"

RunValidation::RunValidation():Tool(){}


bool RunValidation::Initialise(std::string configfile, DataModel &data){

  /////////////////// Useful header ///////////////////////
  if(configfile!="") m_variables.Initialise(configfile); // loading config file
  //m_variables.Print();

  m_data= &data; //assigning transient data pointer
  /////////////////////////////////////////////////////////////////

  m_variables.Get("verbosity",verbosity);
  m_variables.Get("OutputPath",outfile_path);
  m_variables.Get("InvertMRDTimes",invert_mrd_times); 
  m_variables.Get("RunNumber",user_runnumber);
  m_variables.Get("SubRunNumber",user_subrunnumber);
  m_variables.Get("RunType",user_runtype);
  //InvertMRDTimes should only be necessary if the times have not been saved correctly from the common stop mark in common stop mode
  m_variables.Get("SinglePEGains",singlePEgains);

  Log("RunValidation tool: Initialise",v_message,verbosity);

  //Get ANNIE Geometry
  m_data->Stores["ANNIEEvent"]->Header->Get("AnnieGeometry",geom);

  //Read in single p.e. gains
  ifstream file_singlepe(singlePEgains.c_str());
  unsigned long temp_chankey;
  double temp_gain;
  while (!file_singlepe.eof()){
    file_singlepe >> temp_chankey >> temp_gain;
    if (file_singlepe.eof()) break;
    pmt_gains.emplace(temp_chankey,temp_gain);
  }
  file_singlepe.close();

  //Initialize counting variables
  n_entries=0;
  n_pmt_clusters=0;
  n_pmt_clusters_threshold=0;
  n_mrd_clusters=0;
  n_pmt_mrd_clusters=0;
  n_pmt_mrd_nofacc=0;
  n_pmt_mrd_time=0;
  n_pmt_mrd_time_facc=0;
  n_facc=0;
  n_facc_pmt=0;
  n_facc_mrd=0;
  n_facc_pmt_mrd=0;

  first_entry = true;

  return true;
}


bool RunValidation::Execute(){

  Log("RunValidation tool: Execute",v_message,verbosity);
  
  //-------------------------------------------------------------------------
  //Get necessary objects related to PMT clusters (from ClusterFinder tool)
  //-------------------------------------------------------------------------

  int get_ok;
  get_ok = m_data->CStore.Get("ClusterMap",m_all_clusters);
  if (not get_ok) { Log("RunValidation Tool: Error retrieving ClusterMap from CStore, did you run ClusterFinder beforehand?",v_error,verbosity); return false; }
  get_ok = m_data->CStore.Get("ClusterMapDetkey",m_all_clusters_detkey);
  if (not get_ok) { Log("RunValidation Tool: Error retrieving ClusterMapDetkey from CStore, did you run ClusterFinder beforehand?",v_error,verbosity); return false; }


  //-------------------------------------------------------------------------
  //Get necessary objects related to MRD clusters (from TimeClustering tool)
  //-------------------------------------------------------------------------

  get_ok = m_data->CStore.Get("MrdTimeClusters",MrdTimeClusters);
  if (not get_ok) { Log("RunValidation Tool: Error retrieving MrdTimeClusters map from CStore, did you run TimeClustering beforehand?",v_error,verbosity); return false; }
  if (MrdTimeClusters.size()!=0){
    get_ok = m_data->CStore.Get("MrdDigitTimes",MrdDigitTimes);
    if (not get_ok) { Log("RunValidation Tool: Error retrieving MrdDigitTimes map from CStore, did you run TimeClustering beforehand?",v_error,verbosity); return false; }
    get_ok = m_data->CStore.Get("MrdDigitChankeys",mrddigitchankeysthisevent);
    if (not get_ok) { Log("RunValidation Tool: Error retrieving MrdDigitChankeys, did you run TimeClustering beforehand",v_error,verbosity); return false;}
  }

  //-------------------------------------------------------------------------
  //--------------------- Get ANNIEEvent objects ----------------------------
  //-------------------------------------------------------------------------
   
  int RunNumber, SubRunNumber, RunType, EventNumber;
  ULong64_t RunStartTime,EventTimeTank;
  
  
  m_data->Stores["ANNIEEvent"]->Get("RunNumber",RunNumber);
  m_data->Stores["ANNIEEvent"]->Get("SubrunNumber",SubRunNumber);
  m_data->Stores["ANNIEEvent"]->Get("RunType",RunType);
  m_data->Stores["ANNIEEvent"]->Get("RunStartTime",RunStartTime);
  
  //Check if stored run information is sensible and replace with user input, if not
  if (RunNumber == -1) {
    RunNumber = user_runnumber;
    RunStartTime = 0;
  }
  if (SubRunNumber == -1) SubRunNumber = user_subrunnumber;
  if (RunType == -1) RunType = user_runtype;

  m_data->Stores["ANNIEEvent"]->Get("EventNumber",EventNumber);
  m_data->Stores["ANNIEEvent"]->Get("EventTimeTank",EventTimeTank);
  EventTimeTank/=1.e6;
  m_data->Stores["ANNIEEvent"]->Get("TDCData",TDCData);
  Log("RunValidation tool: RunNumber: "+std::to_string(RunNumber)+", SubrunNumber: "+std::to_string(SubRunNumber)+", RunType: "+std::to_string(RunType)+", RunStartTime: "+std::to_string(RunStartTime)+", EventNumber: "+std::to_string(EventNumber)+", EventTimeTank: "+std::to_string(EventTimeTank),v_message,verbosity);


  //-------------------------------------------------------------------------
  //--------------------- Initialise histograms------------------------------
  //-------------------------------------------------------------------------
  
  if (first_entry) {
 
    GlobalRunNumber = RunNumber;
    GlobalSubRunNumber = SubRunNumber;
    start_time = EventTimeTank;

    std::stringstream file_name;
    file_name << outfile_path << "RunValidation_R"<<GlobalRunNumber<<"S"<<GlobalSubRunNumber<<"T"<<RunType<<".root";
    outfile = new TFile(file_name.str().c_str(),"RECREATE");
    outfile->cd();

    std::stringstream title_mrd, title_pmt, title_pmt_2pe, title_pmt_5pe, title_pmt_10pe, title_pmt_30pe, title_mrd_pmt, title_mrd_pmt_100pe, title_mrd_pmt_delta, title_mrd_pmt_delta_100pe, title_pmt_prompt, title_pmt_prompt_10, title_chargeperpmt, title_chargeperpmt_100pe, title_pmt_delayed, title_pmt_delayed_10, title_counts, title_rates, title_fractions;
    title_mrd << "MRD Cluster Times - Run "<<GlobalRunNumber;
    title_pmt << "PMT Cluster Times - Run "<<GlobalRunNumber;
    title_pmt_2pe << "PMT Cluster Times (>2 p.e.) - Run "<<GlobalRunNumber;
    title_pmt_5pe << "PMT Cluster Times (>5 p.e.) - Run "<<GlobalRunNumber;
    title_pmt_10pe << "PMT Cluster Times (>10 p.e.) - Run "<<GlobalRunNumber;
    title_pmt_30pe << "PMT Cluster Times (>30 p.e.) - Run "<<GlobalRunNumber;
    title_mrd_pmt << "MRD vs. PMT Cluster Times - Run "<<GlobalRunNumber;
    title_mrd_pmt_100pe << "MRD vs. PMT Cluster Times (>100 p.e.) - Run "<<GlobalRunNumber;
    title_mrd_pmt_delta << "Difference MRD/PMT Cluster Times - Run "<<GlobalRunNumber;
    title_mrd_pmt_delta_100pe << "Difference MRD/PMT Cluster Times (>100 p.e.) - Run "<<GlobalRunNumber;
    title_pmt_prompt << "PMT Prompt Charge - Run "<<GlobalRunNumber;
    title_pmt_prompt_10 << "PMT Prompt Charge (>10 hits) - Run "<<GlobalRunNumber;
    title_chargeperpmt << "PMT Charge/PMT - Run "<<GlobalRunNumber;
    title_chargeperpmt_100pe << "PMT Charge/PMT (>100 p.e.) - Run "<<GlobalRunNumber;
    title_pmt_delayed << "PMT Delayed Charge - Run "<<GlobalRunNumber;
    title_pmt_delayed_10 << "PMT Delayed Charge (>10 hits)- Run "<<GlobalRunNumber;
    title_counts << "ANNIE Counts - Run "<<GlobalRunNumber;
    title_rates << " ANNIE Rates - Run "<<GlobalRunNumber;
    title_fractions << " ANNIE Event Fractions - Run "<<GlobalRunNumber;
   
    MRD_t_clusters = new TH1D("MRD_t_clusters",title_mrd.str().c_str(),250,0,4000);
    PMT_t_clusters = new TH1D("PMT_t_clusters",title_pmt.str().c_str(),250,0,2000);
    PMT_t_clusters_2pe = new TH1D("PMT_t_clusters_2pe",title_pmt_2pe.str().c_str(),250,0,2000);
    PMT_t_clusters_5pe = new TH1D("PMT_t_clusters_5pe",title_pmt_5pe.str().c_str(),250,0,2000);
    PMT_t_clusters_10pe = new TH1D("PMT_t_clusters_10pe",title_pmt_10pe.str().c_str(),250,0,2000);
    PMT_t_clusters_30pe = new TH1D("PMT_t_clusters_30pe",title_pmt_30pe.str().c_str(),250,0,2000);
    PMT_t_clusters_full = new TH1D("PMT_t_clusters_full",title_pmt.str().c_str(),500,0,75000);
    PMT_t_clusters_2pe_full = new TH1D("PMT_t_clusters_2pe_full",title_pmt_2pe.str().c_str(),500,0,75000);
    PMT_t_clusters_5pe_full = new TH1D("PMT_t_clusters_5pe_full",title_pmt_5pe.str().c_str(),500,0,75000);
    PMT_t_clusters_10pe_full = new TH1D("PMT_t_clusters_10pe_full",title_pmt_10pe.str().c_str(),500,0,75000);
    PMT_t_clusters_30pe_full = new TH1D("PMT_t_clusters_30pe_full",title_pmt_30pe.str().c_str(),500,0,75000);
    MRD_PMT_t = new TH2D("MRD_PMT_t",title_mrd_pmt.str().c_str(),50,0,4000,50,0,2000);
    MRD_PMT_t_100pe = new TH2D("MRD_PMT_t_100pe",title_mrd_pmt_100pe.str().c_str(),50,0,4000,50,0,2000);
    MRD_PMT_Deltat = new TH1D("MRD_PMT_Deltat",title_mrd_pmt_delta.str().c_str(),200,-2000,4000);
    MRD_PMT_Deltat_100pe = new TH1D("MRD_PMT_Deltat_100pe",title_mrd_pmt_delta_100pe.str().c_str(),200,-2000,4000);
    PMT_prompt_charge = new TH1D("PMT_prompt_charge",title_pmt_prompt.str().c_str(),200,0,5000);
    PMT_delayed_charge = new TH1D("PMT_delayed_charge",title_pmt_delayed.str().c_str(),200,0,5000);
    PMT_prompt_charge_zoom = new TH1D("PMT_prompt_charge_zoom",title_pmt_prompt.str().c_str(),200,0,500);
    PMT_delayed_charge_zoom = new TH1D("PMT_delayed_charge_zoom",title_pmt_delayed.str().c_str(),200,0,200);
    PMT_prompt_charge_10hits = new TH1D("PMT_prompt_charge_10hits",title_pmt_prompt_10.str().c_str(),200,0,500);
    PMT_delayed_charge_10hits = new TH1D("PMT_delayed_charge_10hits",title_pmt_delayed_10.str().c_str(),200,0,200);
    PMT_chargeperpmt = new TH1D("PMT_chargeperpmt",title_chargeperpmt.str().c_str(),50,0,30);
    PMT_chargeperpmt_100pe = new TH1D("PMT_chargeperpmt_100pe",title_chargeperpmt_100pe.str().c_str(),50,0,30);
    ANNIE_counts = new TH1D("ANNIE_counts",title_counts.str().c_str(),10,0,10);  
    ANNIE_rates = new TH1D("ANNIE_rates",title_rates.str().c_str(),10,0,10);  
    ANNIE_fractions = new TH1D("ANNIE_fractions",title_fractions.str().c_str(),10,0,10);  
  
    MRD_t_clusters->GetXaxis()->SetTitle("t_{MRD} [ns]");
    PMT_t_clusters->GetXaxis()->SetTitle("t_{PMT} [ns]");
    PMT_t_clusters_2pe->GetXaxis()->SetTitle("t_{PMT} [ns]");
    PMT_t_clusters_5pe->GetXaxis()->SetTitle("t_{PMT} [ns]");
    PMT_t_clusters_10pe->GetXaxis()->SetTitle("t_{PMT} [ns]");
    PMT_t_clusters_30pe->GetXaxis()->SetTitle("t_{PMT} [ns]");
    PMT_t_clusters_full->GetXaxis()->SetTitle("t_{PMT} [ns]");
    PMT_t_clusters_2pe_full->GetXaxis()->SetTitle("t_{PMT} [ns]");
    PMT_t_clusters_5pe_full->GetXaxis()->SetTitle("t_{PMT} [ns]");
    PMT_t_clusters_10pe_full->GetXaxis()->SetTitle("t_{PMT} [ns]");
    PMT_t_clusters_30pe_full->GetXaxis()->SetTitle("t_{PMT} [ns]");
    MRD_PMT_t->GetXaxis()->SetTitle("t_{MRD} [ns]");
    MRD_PMT_t->GetYaxis()->SetTitle("t_{PMT} [ns]");
    MRD_PMT_t_100pe->GetXaxis()->SetTitle("t_{MRD} [ns]");
    MRD_PMT_t_100pe->GetYaxis()->SetTitle("t_{PMT} [ns]");
    MRD_PMT_Deltat->GetXaxis()->SetTitle("t_{MRD}-t_{PMT} [ns]");
    MRD_PMT_Deltat_100pe->GetXaxis()->SetTitle("t_{MRD}-t_{PMT} [ns]");
    PMT_prompt_charge->GetXaxis()->SetTitle("q_{prompt} [p.e.]");
    PMT_prompt_charge_10hits->GetXaxis()->SetTitle("q_{prompt} [p.e.]");
    PMT_prompt_charge_zoom->GetXaxis()->SetTitle("q_{prompt} [p.e.]");
    PMT_delayed_charge->GetXaxis()->SetTitle("q_{delayed} [p.e.]");
    PMT_delayed_charge_10hits->GetXaxis()->SetTitle("q_{delayed} [p.e.]");
    PMT_delayed_charge_zoom->GetXaxis()->SetTitle("q_{delayed} [p.e.]");
    PMT_chargeperpmt->GetXaxis()->SetTitle("q/n_{pmt} [p.e.]");
    PMT_chargeperpmt_100pe->GetXaxis()->SetTitle("q/n_{pmt} [p.e.]");
    ANNIE_rates->GetYaxis()->SetTitle("Rate [Hz]");
    ANNIE_counts->GetYaxis()->SetTitle("#");
    ANNIE_fractions->GetYaxis()->SetTitle("Fraction of events [%]");
    ANNIE_rates->SetStats(0);
    ANNIE_counts->SetStats(0);
    ANNIE_fractions->SetStats(0);

    gROOT->cd();

    first_entry = false;
  }
  current_time = EventTimeTank;

  bool mrd_hit = false;
  bool facc_hit = false;
  bool tank_hit = false;  
  bool coincident_tank_mrd = false;

  //-------------------------------------------------------------------------
  //----------------Fill cluster time histograms-----------------------------
  //-------------------------------------------------------------------------

  double PMT_prompt_time=0.;
  int n_pmt_hits=0;
  double max_charge=0.;
  double max_charge_pe=0.;
  if (m_all_clusters){
    int clustersize = m_all_clusters->size();
    if (clustersize != 0){
      for(std::pair<double,std::vector<Hit>>&& apair : *m_all_clusters){
        double cluster_charge = apair.first;
        std::vector<Hit>&Hits = apair.second;
        std::vector<unsigned long> detkeys = m_all_clusters_detkey->at(cluster_charge);
        double global_time=0.;
        double global_charge=0.;
        double global_chargeperpmt=0.;
        int nhits=0;
        for (unsigned int i_hit = 0; i_hit < Hits.size(); i_hit++){
          unsigned long detkey = detkeys.at(i_hit);
          double time = Hits.at(i_hit).GetTime();
          double charge = Hits.at(i_hit).GetCharge()/pmt_gains[detkey];
          if (charge > 2) {
            PMT_t_clusters_2pe->Fill(time);
            if (time < 10080 || time > 10400) PMT_t_clusters_2pe_full->Fill(time);
            if (charge > 5) {
              PMT_t_clusters_5pe->Fill(time);
              if (time < 10080 || time > 10400) PMT_t_clusters_5pe_full->Fill(time);
              if (charge > 10) {
                if (time < 10080 || time > 10400) PMT_t_clusters_10pe_full->Fill(time);
                PMT_t_clusters_10pe->Fill(time);
                if (charge > 30) {
                  PMT_t_clusters_30pe->Fill(time);
                  if (time < 10080 || time > 10400) PMT_t_clusters_30pe_full->Fill(time);
                }
              }
            }
	  }
	  global_time+=time;
          global_charge+=charge;
          nhits++;
          if (time < 10080 || time > 10400) PMT_t_clusters_full->Fill(time);
          if (time < 2000.) {
            PMT_t_clusters->Fill(time);
          }
        }
	
        if (nhits>0) {
          global_time/=nhits;
          global_chargeperpmt=global_charge/double(nhits);
        }
        
        if (global_time < 2000.) {
          PMT_prompt_charge->Fill(global_charge);
          PMT_prompt_charge_zoom->Fill(global_charge);
          if (nhits>=10) PMT_prompt_charge_10hits->Fill(global_charge);
          PMT_chargeperpmt->Fill(global_chargeperpmt);
          if (global_charge>100) PMT_chargeperpmt_100pe->Fill(global_chargeperpmt);
          if (cluster_charge > max_charge) {
            max_charge = cluster_charge;
            max_charge_pe = global_charge;
            PMT_prompt_time = global_time;
          }
          n_pmt_hits++;
          tank_hit = true;
        }
        else {
          if (global_time < 10080 || global_time > 10400) {
            PMT_delayed_charge->Fill(global_charge);
            PMT_delayed_charge_zoom->Fill(global_charge);
            if (nhits>=10) PMT_delayed_charge_10hits->Fill(global_charge);
          }
	}
      }
    }
  }

  double global_mrd_time=0;
  int n_mrd_hits=0;
  for (unsigned int i_cluster = 0; i_cluster < MrdTimeClusters.size(); i_cluster++){
    std::vector<int> single_mrdcluster = MrdTimeClusters.at(i_cluster);
    int numdigits = single_mrdcluster.size();
    for (int thisdigit = 0; thisdigit < numdigits; thisdigit++){
      int digit_value = single_mrdcluster.at(thisdigit);
      unsigned long chankey = mrddigitchankeysthisevent.at(digit_value);
      Detector *thedetector = geom->ChannelToDetector(chankey);
      unsigned long detkey = thedetector->GetDetectorID();
      if (thedetector->GetDetectorElement()!="MRD") continue;
      else {
        mrd_hit = true;
        int time = MrdDigitTimes.at(digit_value);
 	if (invert_mrd_times) time = 4000.-time;
        MRD_t_clusters->Fill(time);
        global_mrd_time+=time;
        n_mrd_hits++;
      }
    }
  }

  //-------------------------------------------------------------------------
  //---------------Loop over FACC entries------------------------------------
  //-------------------------------------------------------------------------
  
  if(TDCData){
    if (TDCData->size()==0){
      Log("RunValidation tool: TDC data is empty in this event.",v_message,verbosity);
    } else {
      for (auto&& anmrdpmt : (*TDCData)){
        unsigned long chankey = anmrdpmt.first;
        Detector* thedetector = geom->ChannelToDetector(chankey);
        unsigned long detkey = thedetector->GetDetectorID();
        if (thedetector->GetDetectorElement()=="Veto") facc_hit = true;
      }
    }
  } else {
    Log("RunValidation tool: No TDC data available in this event.",v_message,verbosity);
  }
  
  //-------------------------------------------------------------------------
  //---------------Increment event type counters-----------------------------
  //-------------------------------------------------------------------------
  
  bool pmt_mrd_time_coinc=false;
  if (n_mrd_hits>0) global_mrd_time/=n_mrd_hits;
  if (n_mrd_hits>0 && n_pmt_hits>0){
    MRD_PMT_t->Fill(global_mrd_time,PMT_prompt_time);
    MRD_PMT_Deltat->Fill(global_mrd_time-PMT_prompt_time);
    if ((global_mrd_time-PMT_prompt_time)>700 && (global_mrd_time-PMT_prompt_time)<800) pmt_mrd_time_coinc = true;
    if (max_charge_pe > 100){
      MRD_PMT_t_100pe->Fill(global_mrd_time,PMT_prompt_time);
      MRD_PMT_Deltat_100pe->Fill(global_mrd_time-PMT_prompt_time);
    }
    coincident_tank_mrd = true;
  }

  if (tank_hit) n_pmt_clusters++;
  if (tank_hit && max_charge_pe > 100.) n_pmt_clusters_threshold++;	//100 p.e. threshold arbitrary at this point
  if (mrd_hit) n_mrd_clusters++;
  if (coincident_tank_mrd) n_pmt_mrd_clusters++;
  if (coincident_tank_mrd && !facc_hit) n_pmt_mrd_nofacc++;
  if (pmt_mrd_time_coinc) n_pmt_mrd_time++;
  if (pmt_mrd_time_coinc && facc_hit) n_pmt_mrd_time_facc++;
  if (facc_hit) n_facc++;
  if (tank_hit && facc_hit) n_facc_pmt++;
  if (mrd_hit && facc_hit) n_facc_mrd++;
  if (coincident_tank_mrd && facc_hit) n_facc_pmt_mrd++;
  n_entries++;

  bool facc_mrd = (facc_hit && mrd_hit);
  if (coincident_tank_mrd){
    std::cout <<"Coincident tank + mrd clusters found. facc_hit && mrd_hit: "<<facc_mrd<<", n_pmt_mrd_clusters: "<<n_pmt_mrd_clusters<<", n_facc_mrd: "<<n_facc_mrd<<std::endl;
  }
  if (facc_hit && mrd_hit){
    std::cout <<"FACC hit found. coincident_tank_mrd: "<<coincident_tank_mrd<<", n_pmt_mrd_clusters: "<<n_pmt_mrd_clusters<<", n_facc_mrd: "<<n_facc_mrd<<std::endl;
  }

  return true;
}


bool RunValidation::Finalise(){

  Log("RunValidation tool: Finalise",v_message,verbosity);
 
  //-------------------------------------------------------------------------
  //---------------------------Calculate rates-------------------------------
  //-------------------------------------------------------------------------

  double rate_entries=0;
  double rate_tank=0;
  double rate_tank_threshold=0;
  double rate_mrd=0;
  double rate_coincident=0;
  double rate_coincident_nofacc=0;
  double rate_facc=0;
  double rate_facc_pmt=0;
  double rate_facc_mrd=0;
  double rate_facc_pmt_mrd=0;
  double fraction_entries=0;
  double fraction_tank=0;
  double fraction_tank_threshold=0;
  double fraction_mrd=0;
  double fraction_coincident=0;
  double fraction_coincident_nofacc=0;
  double fraction_facc=0;
  double fraction_facc_pmt=0;
  double fraction_facc_mrd=0;
  double fraction_facc_pmt_mrd=0;
  double fraction_coincident_time = 0;
  double fraction_coincident_time_facc = 0;


  double time_diff = (current_time-start_time)/1000.;
  if (time_diff > 0.){
    rate_entries = n_entries/time_diff;
    rate_tank = n_pmt_clusters/time_diff;
    rate_tank_threshold = n_pmt_clusters_threshold/time_diff;
    rate_mrd = n_mrd_clusters/time_diff;
    rate_coincident = n_pmt_mrd_clusters/time_diff;
    rate_coincident_nofacc = n_pmt_mrd_nofacc/time_diff;
    rate_facc = n_facc/time_diff;
    rate_facc_pmt = n_facc_pmt/time_diff;
    rate_facc_mrd = n_facc_mrd/time_diff;
    rate_facc_pmt_mrd = n_facc_pmt_mrd/time_diff;
  }
  if (n_entries > 0){
    fraction_entries = 100;
    fraction_tank = double(n_pmt_clusters)/n_entries*100;
    fraction_tank_threshold = double(n_pmt_clusters_threshold)/n_entries*100;
    fraction_mrd = double(n_mrd_clusters)/n_entries*100;
    fraction_coincident = double(n_pmt_mrd_clusters)/n_entries*100;
    fraction_coincident_nofacc = double(n_pmt_mrd_nofacc)/n_entries*100;
    fraction_facc = double(n_facc)/n_entries*100;
    fraction_facc_pmt = double(n_facc_pmt)/n_entries*100;
    fraction_facc_mrd = double(n_facc_mrd)/n_entries*100;
    fraction_facc_pmt_mrd = double(n_facc_pmt_mrd)/n_entries*100;
    fraction_coincident_time = double(n_pmt_mrd_time)/n_entries*100;
    fraction_coincident_time_facc = double(n_pmt_mrd_time_facc)/n_entries*100;
  }

  ANNIE_rates->SetBinContent(1,rate_entries);
  ANNIE_rates->SetBinContent(2,rate_tank);
  ANNIE_rates->SetBinContent(3,rate_tank_threshold);
  ANNIE_rates->SetBinContent(4,rate_mrd);
  ANNIE_rates->SetBinContent(5,rate_coincident);
  ANNIE_rates->SetBinContent(6,rate_coincident_nofacc);
  ANNIE_rates->SetBinContent(7,rate_facc);
  ANNIE_rates->SetBinContent(8,rate_facc_pmt);
  ANNIE_rates->SetBinContent(9,rate_facc_mrd);
  ANNIE_rates->SetBinContent(10,rate_facc_pmt_mrd);
  
  ANNIE_counts->SetBinContent(1,n_entries);
  ANNIE_counts->SetBinContent(2,n_pmt_clusters);
  ANNIE_counts->SetBinContent(3,n_pmt_clusters_threshold);
  ANNIE_counts->SetBinContent(4,n_mrd_clusters);
  ANNIE_counts->SetBinContent(5,n_pmt_mrd_clusters);
  ANNIE_counts->SetBinContent(6,n_pmt_mrd_nofacc);
  ANNIE_counts->SetBinContent(7,n_facc);
  ANNIE_counts->SetBinContent(8,n_facc_pmt);
  ANNIE_counts->SetBinContent(9,n_facc_mrd);
  ANNIE_counts->SetBinContent(10,n_facc_pmt_mrd);
  
  ANNIE_fractions->SetBinContent(1,fraction_entries);
  ANNIE_fractions->SetBinContent(2,fraction_tank);
  ANNIE_fractions->SetBinContent(3,fraction_tank_threshold);
  ANNIE_fractions->SetBinContent(4,fraction_mrd);
  ANNIE_fractions->SetBinContent(5,fraction_coincident);
  ANNIE_fractions->SetBinContent(6,fraction_coincident_nofacc);
  ANNIE_fractions->SetBinContent(7,fraction_facc);
  ANNIE_fractions->SetBinContent(8,fraction_facc_pmt);
  ANNIE_fractions->SetBinContent(9,fraction_facc_mrd);
  ANNIE_fractions->SetBinContent(10,fraction_facc_pmt_mrd);

  const char *category_label[10] = {"All Events","PMT Clusters","PMT Clusters > 100p.e.","MRD Clusters","PMT+MRD Clusters","PMT+MRD, No FMV","FMV","FMV+PMT","FMV+MRD","FMV+PMT+MRD"};
  for (int i_label=0;i_label<10;i_label++){
    ANNIE_rates->GetXaxis()->SetBinLabel(i_label+1,category_label[i_label]);
    ANNIE_counts->GetXaxis()->SetBinLabel(i_label+1,category_label[i_label]);
    ANNIE_fractions->GetXaxis()->SetBinLabel(i_label+1,category_label[i_label]);
  }

  Log("RunValidation tool: Finished analysing run. Summary:",v_message,verbosity);
  Log("RunValidation tool: Duration: "+std::to_string(int(time_diff/3600))+"h:"+std::to_string(int(time_diff/60)%60)+"min:"+std::to_string(int(time_diff)%60)+"sec",v_message,verbosity);
  Log("RunValidation tool: Num Entries: "+std::to_string(n_entries)+", PMT Clusters: "+std::to_string(n_pmt_clusters)+", PMT Clusters > 100p.e.: "+std::to_string(n_pmt_clusters_threshold)+", MRD Clusters: "+std::to_string(n_mrd_clusters)+", PMT+MRD clusters: "+std::to_string(n_pmt_mrd_clusters)+", FACC Events: "+std::to_string(n_facc)+", FACC+PMT: "+std::to_string(n_facc_pmt)+", FACC+MRD: "+std::to_string(n_facc_mrd)+", FACC+PMT+MRD: "+std::to_string(n_facc_pmt_mrd),v_message,verbosity); 
  Log("RunValidation tool: Total Rate: "+std::to_string(rate_entries)+", Rate PMT Clusters: "+std::to_string(rate_tank)+", Rate PMT Clusters > 100p.e.: "+std::to_string(rate_tank_threshold)+", Rate MRD Clusters: "+std::to_string(rate_mrd)+", Rate PMT+MRD clusters: "+std::to_string(rate_coincident)+", FACC Events: "+std::to_string(rate_facc)+", Rate FACC+PMT: "+std::to_string(rate_facc_pmt)+", Rate FACC+MRD: "+std::to_string(rate_facc_mrd)+", Rate FACC+PMT+MRD: "+std::to_string(rate_facc_pmt_mrd),v_message,verbosity); 
  Log("RunValidation tool: Fraction PMT Clusters: "+std::to_string(fraction_tank)+", Fraction PMT Clusters > 100p.e.: "+std::to_string(fraction_tank_threshold)+", Fraction MRD Clusters: "+std::to_string(fraction_mrd)+", Fraction PMT+MRD clusters: "+std::to_string(fraction_coincident)+", Fraction FACC: "+std::to_string(fraction_facc)+", Fraction FACC+PMT: "+std::to_string(fraction_facc_pmt)+", Fraction FACC+MRD: "+std::to_string(fraction_facc_mrd)+", Fraction FACC+PMT+MRD: "+std::to_string(fraction_facc_pmt_mrd),v_message,verbosity); 
  Log("RunValidation tool: Fraction Coincident PMT/MRD clusters: "+std::to_string(fraction_coincident_time)+", Fraction Coincident PMT/MRD clusters with FMV hit: "+std::to_string(fraction_coincident_time_facc),v_message,verbosity);
  Log("RunValidation tool: End of Summary",v_message,verbosity);


  //-------------------------------------------------------------------------
  //--------------------Write histograms to file-----------------------------
  //-------------------------------------------------------------------------
  
  outfile->cd();
  MRD_t_clusters->Write();
  PMT_t_clusters->Write();
  PMT_t_clusters_2pe->Write();
  PMT_t_clusters_5pe->Write();
  PMT_t_clusters_10pe->Write();
  PMT_t_clusters_30pe->Write();
  PMT_t_clusters_full->Write();
  PMT_t_clusters_2pe_full->Write();
  PMT_t_clusters_5pe_full->Write();
  PMT_t_clusters_10pe_full->Write();
  PMT_t_clusters_30pe_full->Write();
  MRD_PMT_t->Write();
  MRD_PMT_t_100pe->Write();
  MRD_PMT_Deltat->Write();
  MRD_PMT_Deltat_100pe->Write();
  PMT_prompt_charge->Write();
  PMT_prompt_charge_zoom->Write();
  PMT_prompt_charge_10hits->Write();
  PMT_delayed_charge->Write();
  PMT_delayed_charge_zoom->Write();
  PMT_delayed_charge_10hits->Write();
  PMT_chargeperpmt->Write();
  PMT_chargeperpmt_100pe->Write();
  ANNIE_counts->Write();
  ANNIE_rates->Write();
  ANNIE_fractions->Write();
  outfile->Close();
  delete outfile;

  return true;
}
