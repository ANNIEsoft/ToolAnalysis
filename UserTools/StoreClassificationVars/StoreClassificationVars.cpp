#include "StoreClassificationVars.h"

StoreClassificationVars::StoreClassificationVars():Tool(){}

bool StoreClassificationVars::Initialise(std::string configfile, DataModel &data){

	//---------------------------------------------------------------
	//----------------- Useful header -------------------------------
	//---------------------------------------------------------------

	if(configfile!="")  m_variables.Initialise(configfile); 
	m_data= &data;

	save_csv = 1;
	save_root = 0;
	filename = "classification";

	//---------------------------------------------------------------
	//----------------- Configuration variables ---------------------
	//---------------------------------------------------------------
	
	m_variables.Get("verbosity",verbosity);
	m_variables.Get("Filename",filename);
	m_variables.Get("SaveCSV",save_csv);
	m_variables.Get("SaveROOT",save_root);
	m_variables.Get("VariableConfig",variable_config);
	m_variables.Get("VariableConfigPath",variable_config_path);
	m_variables.Get("IsData",isData);

	std::string logmessage = "StoreClassificationVars Tool: Output Filename is "+filename+".csv/root";
	Log(logmessage,v_message,verbosity);
	Log("StoreClassificationVars Tool: Save variables to a CSV file: "+std::to_string(save_csv),v_message,verbosity);
	Log("StoreClassificationVars Tool: Save variables to a ROOT file: "+std::to_string(save_root),v_message,verbosity);
	Log("StoreClassificationVars Tool: Chosen model: "+variable_config);

	//---------------------------------------------------------------
	//----------------- Load chosen variable config -----------------
	//---------------------------------------------------------------
	
	bool config_loaded;
	config_loaded = this->LoadVariableConfig(variable_config);	
	if (!config_loaded) {
		Log("StoreClassificationVars Tool: Chosen variable configuration >>>"+variable_config+"<<< is not a valid option [Minimal/Full]. Using Minimal configuration.",v_error,verbosity);
		std::string minimal_configuration = "Minimal";
		config_loaded = this->LoadVariableConfig(minimal_configuration);
		if (!config_loaded) {
			Log("StoreClassificationVars Tool: Path to variable configuration ["+variable_config_path+"] seems to be invalid. Abort.",v_error,verbosity);
			return false;
		}
	}

	//Initialise ROOT histograms
	if (save_root) {
		this->InitClassHistograms();
		this->InitClassTree();
	}

	//Create & initialise CSV file
	if (save_csv) this->InitCSV();

	//Populate variable config map
	this->PopulateConfigMap();

	Log("StoreClassificationVars Tool: Initialization complete",v_message,verbosity);

	return true;
}


bool StoreClassificationVars::Execute(){

	Log("StoreClassificationVars Tool: Executing",v_debug,verbosity);

	//-----------------------------------------------------------------------
	//-------------- Getting variables from Classification store ------------
	//-----------------------------------------------------------------------
	
	get_ok = m_data->Stores.count("Classification");
	if(!get_ok){
		Log("StoreClassificationVars Tool: No Classification store! Please run CalcClassificationVars before this tool. Exiting...",v_error,verbosity);
		return false;
	};

	bool selection_passed;
	m_data->Stores["Classification"]->Get("SelectionPassed",selection_passed);
	if (!selection_passed) {
		Log("StoreClassificationVars Tool: EventSelection cuts were not passed. Skip this event",v_message,verbosity);
		return true;
	}
	
	
        //General variables
        m_data->Stores["Classification"]->Get("MC-based",use_mctruth);
	m_data->Stores["Classification"]->Get("SelectionPassed",selection_passed);
	m_data->Stores["Classification"]->Get("MLDataPresent",mldata_present);
	if (!selection_passed){
		Log("StoreClassificationVars Tool: Selection cuts were not passed. Abort",v_message,verbosity);
		return true;
	}
	if (!mldata_present){
		Log("StoreClassificationVars Tool: MLData is not present. Abort",v_message,verbosity);
		return true;
	}

	//PMT variables
        m_data->Stores["Classification"]->Get("PMTBaryTheta",pmt_baryTheta);
        m_data->Stores["Classification"]->Get("PMTAvgDist",pmt_avgDist);
        m_data->Stores["Classification"]->Get("PMTAvgT",pmt_avgT);
        m_data->Stores["Classification"]->Get("PMTQtotal",pmt_totalQ);
        m_data->Stores["Classification"]->Get("PMTQtotalClustered",pmt_totalQ_Clustered);
        m_data->Stores["Classification"]->Get("PMTHits",pmt_hits);
        m_data->Stores["Classification"]->Get("PMTFracQmax",pmt_fracHighestQ);
        m_data->Stores["Classification"]->Get("PMTFracQdownstream",pmt_fracDownstream);
        m_data->Stores["Classification"]->Get("PMTFracClustered",pmt_fracClustered);
        m_data->Stores["Classification"]->Get("PMTFracLowQ",pmt_fracLowQ);
        m_data->Stores["Classification"]->Get("PMTFracEarly",pmt_fracEarlyT);
        m_data->Stores["Classification"]->Get("PMTFracLate",pmt_fracLateT);
        m_data->Stores["Classification"]->Get("PMTRMSTheta",pmt_rmsTheta);
        m_data->Stores["Classification"]->Get("PMTVarTheta",pmt_varTheta);
        m_data->Stores["Classification"]->Get("PMTRMSThetaBary",pmt_rmsThetaBary);
        m_data->Stores["Classification"]->Get("PMTVarThetaBary",pmt_varThetaBary);
        m_data->Stores["Classification"]->Get("PMTRMSPhi",pmt_rmsPhi);
        m_data->Stores["Classification"]->Get("PMTVarPhi",pmt_varPhi);
        m_data->Stores["Classification"]->Get("PMTRMSPhiBary",pmt_rmsPhiBary);
        m_data->Stores["Classification"]->Get("PMTVarPhiBary",pmt_varPhiBary);
        m_data->Stores["Classification"]->Get("PMTFracLargeAngleTheta",pmt_fracLargeAngleTheta);
        m_data->Stores["Classification"]->Get("PMTHitsLargeAngleTheta",pmt_hitsLargeAngleTheta);
        m_data->Stores["Classification"]->Get("PMTFracLargeAnglePhi",pmt_fracLargeAnglePhi);
        m_data->Stores["Classification"]->Get("PMTHitsLargeAnglePhi",pmt_hitsLargeAnglePhi);

	//LAPPD variables
        m_data->Stores["Classification"]->Get("LAPPDBaryTheta",lappd_baryTheta);
        m_data->Stores["Classification"]->Get("LAPPDAvgDist",lappd_avgDist);
        m_data->Stores["Classification"]->Get("LAPPDQtotal",lappd_totalQ);
        m_data->Stores["Classification"]->Get("LAPPDAvgT",lappd_avgT);
        m_data->Stores["Classification"]->Get("LAPPDHits",lappd_hits);
        m_data->Stores["Classification"]->Get("LAPPDRMSTheta",lappd_rmsTheta);
        m_data->Stores["Classification"]->Get("LAPPDVarTheta",lappd_varTheta);
        m_data->Stores["Classification"]->Get("LAPPDRMSThetaBary",lappd_rmsThetaBary);
        m_data->Stores["Classification"]->Get("LAPPDVarThetaBary",lappd_varThetaBary);

	//MRD variables
	m_data->Stores["Classification"]->Get("MrdLayers",num_mrd_layers);
        m_data->Stores["Classification"]->Get("MrdPaddles",num_mrd_paddles);
        m_data->Stores["Classification"]->Get("MrdConsLayers",num_mrd_conslayers);
        m_data->Stores["Classification"]->Get("MrdAdjHits",num_mrd_adjacent);
        m_data->Stores["Classification"]->Get("MrdPadPerLayer",mrd_padperlayer);
        m_data->Stores["Classification"]->Get("MrdXSpread",mrd_mean_xspread);
        m_data->Stores["Classification"]->Get("MrdYSpread",mrd_mean_yspread);
        m_data->Stores["Classification"]->Get("MrdClusters",num_mrd_clusters);

	//MC-Truth variables
	if (!isData){
        	m_data->Stores["Classification"]->Get("PMTFracRing",pmt_fracRing);
        	m_data->Stores["Classification"]->Get("PMTFracRingNoWeight",pmt_fracRingNoWeight);
        	m_data->Stores["Classification"]->Get("LAPPDFracRing",lappd_fracRing);
		m_data->Stores["Classification"]->Get("VDistVtxWall",distWallVert);
        	m_data->Stores["Classification"]->Get("HDistVtxWall",distWallHor);
        	m_data->Stores["Classification"]->Get("VDistVtxInner",distInnerStrVert);
        	m_data->Stores["Classification"]->Get("HDistVtxInner",distInnerStrHor);
        	m_data->Stores["Classification"]->Get("VtxTrueTime",true_time);
        	m_data->Stores["Classification"]->Get("TrueEnergy",energy);
        	m_data->Stores["Classification"]->Get("NRings",nrings);
        	m_data->Stores["Classification"]->Get("MultiRing",multiplerings);
        	m_data->Stores["Classification"]->Get("EventNumber",evnum);
		m_data->Stores["Classification"]->Get("PDG",pdg);
	}

	//Detailed variables (for plots)
	pmtQ.clear();
	pmtT.clear();
	pmtDist.clear();
	pmtTheta.clear();
	pmtThetaBary.clear();
	pmtPhi.clear();
	pmtPhiBary.clear();
	pmtY.clear();
	lappdQ.clear();
	lappdT.clear();
	lappdDist.clear();
	lappdTheta.clear();
	lappdThetaBary.clear();

	m_data->Stores["Classification"]->Get("PMTQVector",pmtQ);
	m_data->Stores["Classification"]->Get("PMTTVector",pmtT);
	m_data->Stores["Classification"]->Get("PMTDistVector",pmtDist);
	m_data->Stores["Classification"]->Get("PMTThetaVector",pmtTheta);
	m_data->Stores["Classification"]->Get("PMTThetaBaryVector",pmtThetaBary);
	m_data->Stores["Classification"]->Get("PMTPhiVector",pmtPhi);
	m_data->Stores["Classification"]->Get("PMTPhiBaryVector",pmtPhiBary);
	m_data->Stores["Classification"]->Get("PMTYVector",pmtY);
	m_data->Stores["Classification"]->Get("LAPPDQVector",lappdQ);
	m_data->Stores["Classification"]->Get("LAPPDTVector",lappdT);
	m_data->Stores["Classification"]->Get("LAPPDDistVector",lappdDist);
	m_data->Stores["Classification"]->Get("LAPPDThetaVector",lappdTheta);
	m_data->Stores["Classification"]->Get("LAPPDThetaBaryVector",lappdThetaBary);

	//Fill classification variables in ROOT histograms
	if (save_root) {
		this->FillClassHistograms();
		this->FillClassTree();
	}

	//Update variables in config map
	if (save_csv) this->UpdateConfigMap();

	//Fill classification variables into csv file
	if (save_csv) this->FillCSV();


	return true;

}


bool StoreClassificationVars::Finalise(){

  	Log("StoreClassificationVars tool: Finalisation started",v_message,verbosity);
  	
	if (save_root) WriteClassHistograms();

	if (save_csv) {

		csv_file.close();	
		csv_statusfile.close();
		Log("StoreClassificationVars Tool: Information written to csv-file.",v_message,verbosity);

	}
 
	Log("StoreClassificationVars Tool: Finalisation complete",v_message,verbosity);

	return true;

}

bool StoreClassificationVars::LoadVariableConfig(std::string config_name){
	
	//-----------------------------------------------------------------------
	//-------------------- Load variable configuration----------------------
	//-----------------------------------------------------------------------

	bool config_loaded = false;

	std::stringstream config_filename;
	config_filename << variable_config_path << "/VariableConfig_" << config_name << ".txt";
	std::cout <<"config_filename: "<<config_filename.str()<<std::endl;
	ifstream configfile(config_filename.str().c_str());
	if (!configfile.good()){
		Log("StoreClassificationVars tool: Variable configuration file "+config_filename.str()+" does not seem to exist. Abort",v_error,verbosity);
		return false;
	}

	std::string temp_string;
	while (!configfile.eof()){
		configfile >> temp_string;
		if (configfile.eof()) break;
		variable_names.push_back(temp_string);
	}
	configfile.close();
	config_loaded = true;


	return config_loaded;
}

void StoreClassificationVars::InitClassHistograms(){

	//-----------------------------------------------------------------------
	//------- Histograms showing the variables used in the classification----
	//-----------------------------------------------------------------------

	std::stringstream ss_filename_root;
	ss_filename_root << filename << ".root";

	file = new TFile(ss_filename_root.str().c_str(),"RECREATE");
	file->cd();

	Log("StoreClassificationVars tool: Initialising histograms associated to TFile "+ss_filename_root.str(),v_message,verbosity);

	//PMT-related variables

	hist_pmtPE_single = new TH1F("hist_pmtPE_single","Single PMT Charges",500,0,500);
	hist_pmtTime_single = new TH1F("hist_pmtTime_single","Single PMT Times",2000,0,2000);
	hist_pmtTheta_single = new TH1F("hist_pmtTheta_single","Single PMT Theta",100,0,TMath::Pi());
	hist_pmtPhi_single = new TH1F("hist_pmtPhi_single","Single PMT Phi",100,0,2*TMath::Pi());
	hist_pmtY_single = new TH1F("hist_pmtY_single","Single PMT Y",100,-2.,2.);
	hist_pmtDist_single = new TH1F("hist_pmtDist_single","Single PMT Distance",100,0.,4.);
	hist_pmtThetaBary_single = new TH1F("hist_pmtThetaBary_single","#theta_{Bary} distribution",100,-TMath::Pi(),TMath::Pi());
	hist_pmtPhiBary_single = new TH1F("hist_pmtPhiBary_single","#phi_{Bary} distribution",100,-TMath::Pi(),TMath::Pi());

	hist_pmtHits = new TH1F("hist_pmtHits","Hit PMTs",150,0,150);
	hist_pmtPEtotal = new TH1F("hist_pmtPEtotal","PMT Total Charge",500,0,12000);
	hist_pmtPEtotalClustered = new TH1F("hist_pmtPEtotalClustered","PMT Total Charge (Clustered)",500,0,12000);
	hist_pmtAvgTime = new TH1F("hist_pmtAvgTime","PMT Average Time",100,0,50);
	hist_pmtAvgDist = new TH1F("hist_pmtAvgDist","PMT Average Distance",100,0,4.);
	hist_pmtThetaBary = new TH1F("hist_pmtThetaBary","PMT #theta_{Bary}",100,0,TMath::Pi());
	hist_pmtThetaRMS = new TH1F("hist_pmtThetaRMS","PMT RMS of #theta",100,0,TMath::Pi());
	hist_pmtThetaVar = new TH1F("hist_pmtThetaVar","PMT Variance of #theta",100,0,TMath::Pi());
	hist_pmtThetaBaryRMS = new TH1F("hist_pmtThetaBaryRMS","PMT RMS of #theta_{Bary}",100,0,TMath::Pi());
	hist_pmtThetaBaryVar = new TH1F("hist_pmtThetaBaryVar","PMT Variance of #theta_{Bary}",100,0,TMath::Pi());;
	hist_pmtFracLargeAngleThetaBary= new TH1F("hist_pmtFracLargeAngleThetaBary","#theta_{Bary} fraction large angles",100,0,1.);
	hist_pmtPhiRMS = new TH1F("hist_pmtPhiRMS","PMT RMS of #phi",100,0,2*TMath::Pi());
	hist_pmtPhiVar = new TH1F("hist_pmtPhiVar","PMT Variance of #phi",100,0,2*TMath::Pi());
	hist_pmtPhiBaryRMS = new TH1F("hist_pmtPhiBaryRMS","PMT RMS of #theta_{Bary}",100,0,TMath::Pi());
	hist_pmtPhiBaryVar = new TH1F("hist_pmtPhiBaryVar","PMT Variance of #theta_{Bary}",100,0,TMath::Pi());;
	hist_pmtFracLargeAnglePhiBary = new TH1F("hist_pmtFracLargeAnglePhiBary","#phi_{Bary} fraction large angles",100,0,1.);
	hist_pmtFracDownstream = new TH1F("hist_pmtFracDownstream","Fraction of PMT hits downstream",100,0,1);
	hist_pmtFracHighestQ = new TH1F("hist_pmtFracHighestQ","Charge fraction highest PMT",100,0,1);
	hist_pmtFracClustered = new TH1F("hist_pmtFracClustered","Fraction of charge (clustered)",100,0,1);
	hist_pmtFracEarlyTime = new TH1F("hist_pmtFracEarlyTime","Fraction of early PMT hits",100,0,1.);	//t (PMT) < 4 ns
	hist_pmtFracLateTime = new TH1F("hist_pmtFracLateTime","Fraction of late PMT hits",100,0,1);		//t (PMT) > 10 ns
	hist_pmtFracLowCharge = new TH1F("hist_pmtFracLowCharge","Fraction of low charge PMT hits",100,0,1);	//Q (PMT) < 30 p.e.

	//LAPPD-related variables
		
	hist_lappdPE_single = new TH1F("hist_lappdPE_single","Single LAPPD Hit Charges",100,0,5);
	hist_lappdTime_single = new TH1F("hist_lappdTime_single","Single LAPPD Hit Times",1000,0,1000);
	hist_lappdTheta_single = new TH1F("hist_lappdTheta_single","Single LAPPD Hit Thetas",100,0,TMath::Pi());
	hist_lappdThetaBary_single = new TH1F("hist_lappdThetaBary_single","Single LAPPD Hit #Theta_{Bary}",100,0,TMath::Pi());
	hist_lappdDist_single = new TH1F("hist_lappdDist_single","Single LAPPD Hit Distances",100,0,4.);

	hist_lappdHits = new TH1F("hist_lappdHits","Number LAPPD Hits",1000,0,1000);
	hist_lappdPEtotal = new TH1F("hist_lappdPEtotal","LAPPD Total Charge",1000,0,1000);
	hist_lappdAvgTime = new TH1F("hist_lappdAvgTime","LAPPD Average Time",100,0,50);
	hist_lappdAvgDist = new TH1F("hist_lappdAvgDist","LAPPD Average Distance",100,0,4.);
	hist_lappdThetaBary = new TH1F("hist_lappdThetaBary","LAPPD #theta_{Bary}",100,0,TMath::Pi());
	hist_lappdThetaRMS = new TH1F("hist_lappdThetaRMS","LAPPD RMS of #theta",100,0,TMath::Pi());
	hist_lappdThetaVar = new TH1F("hist_lappdThetaVar","LAPPD Variance of #theta",100,0,TMath::Pi());
	hist_lappdThetaBaryRMS = new TH1F("hist_lappdThetaBaryRMS","LAPPD RMS of #theta_{Bary}",100,0,TMath::Pi());
	hist_lappdThetaBaryVar = new TH1F("hist_lappdThetaBaryVar","LAPPD Variance of #theta_{Bary}",100,0,TMath::Pi());

	//MRD-related variables
		
	hist_mrdPaddles = new TH1F("hist_mrdPaddles","Num MRD Paddles hit",310,0,310);
	hist_mrdLayers = new TH1F("hist_mrdLayers","Num MRD Layers hit",11,0,11);
	hist_mrdconsLayers = new TH1F("hist_mrdconsLayers","Num consecutive MRD Layers hit",11,0,11);
	hist_mrdClusters = new TH1F("hist_mrdClusters","Num MRD Clusters",20,0,20);
	hist_mrdXSpread = new TH1F("hist_mrdXSpread","MRD Layer-wise X spread",100,0,5.);
	hist_mrdYSpread = new TH1F("hist_mrdYSpread","MRD Layer-wise Y spread",100,0,5.);
	hist_mrdAdjacentHits = new TH1F("hist_mrdAdjacentHits","MRD # Adjacent hits",100,0,30);
	hist_mrdPaddlesPerLayer = new TH1F("hist_mrdPaddlesPerLayer","MRD Paddles per Layer",100,0,10.);

	//Truth-related variables

	hist_pmtFracRing = new TH1F("hist_pmtFracRing","Fraction of PMT hits in Ring (weighted)",100,0,1);
	hist_pmtFracRingNoWeight = new TH1F("hist_pmtFracRingNoWeight","Fraction of PMT hits in Ring",100,0,1);
	hist_lappdFracRing = new TH1F("hist_lappdFracRing","Fraction of LAPPD Hits in Ring",100,0,1);
	hist_evnum = new TH1F("hist_evnum","Event Numbers",500,0,10000);
	hist_distWallHor = new TH1F("hist_distWallHor","Horizontal distance Vertex - Wall",100,-3.,3.);
	hist_distWallVert = new TH1F("hist_distWallVert","Vertical distance Vertex - Wall",100,-3.,3.);
	hist_distInnerStrHor = new TH1F("hist_distInnerStrHor","Horizontal distance Vertex - Inner Structure",100,-2.,2.);
	hist_distInnerStrVert = new TH1F("hist_distInnerStrVert","Horizontal distance Vertex - Inner Structure",100,-2.,2.);
	hist_energy = new TH1F("hist_energy","Energy of primary particle",5000,0,5000);
	hist_nrings = new TH1F("hist_nrings","Number of rings",20,0,20);
	hist_multiplerings = new TH1F("hist_multiple_rings","Multiple Rings",2,0,2);
	hist_pdg = new TH1F("hist_pdg","PDG codes",500,-500,500);
	hist_truetime = new TH1F("hist_truetime","True vtx time",500,-100,200);

}

void StoreClassificationVars::InitClassTree(){

	//-----------------------------------------------------------------------
	//-------------------- Initialise classification tree--------------------
	//-----------------------------------------------------------------------

	Log("StoreClassificationVars tool: Initialising classification tree.",v_message,verbosity);

	file->cd();
	tree = new TTree("classification_tree","Classification tree");

	tree->Branch("pmt_baryTheta",&pmt_baryTheta);
	tree->Branch("pmt_avgDist",&pmt_avgDist);
	tree->Branch("pmt_avgT",&pmt_avgT);
	tree->Branch("pmt_totalQ",&pmt_totalQ);
	tree->Branch("pmt_totalQ_Clustered",&pmt_totalQ_Clustered);
	tree->Branch("pmt_hits",&pmt_hits);
	tree->Branch("pmt_fracHighestQ",&pmt_fracHighestQ);
	tree->Branch("pmt_fracDownstream",&pmt_fracDownstream);
	tree->Branch("pmt_fracClustered",&pmt_fracClustered);
	tree->Branch("pmt_fracLowQ",&pmt_fracLowQ);
	tree->Branch("pmt_fracEarlyT",&pmt_fracEarlyT);
	tree->Branch("pmt_fracLateT",&pmt_fracLateT);
	tree->Branch("pmt_rmsTheta",&pmt_rmsTheta);
	tree->Branch("pmt_varTheta",&pmt_varTheta);
	tree->Branch("pmt_rmsThetaBary",&pmt_rmsThetaBary);
	tree->Branch("pmt_varThetaBary",&pmt_varThetaBary);
	tree->Branch("pmt_rmsPhi",&pmt_rmsPhi);
	tree->Branch("pmt_varPhi",&pmt_varPhi);
	tree->Branch("pmt_rmsPhiBary",&pmt_rmsPhiBary);
	tree->Branch("pmt_varPhiBary",&pmt_varPhiBary);
	tree->Branch("pmt_fracLargeAngleTheta",&pmt_fracLargeAngleTheta);
	tree->Branch("pmt_hitsLargeAngleTheta",&pmt_hitsLargeAngleTheta);
	tree->Branch("pmt_fracLargeAnglePhi",&pmt_fracLargeAnglePhi);
	tree->Branch("pmt_hitsLargeAnglePhi",&pmt_hitsLargeAnglePhi);

	tree->Branch("lappd_baryTheta",&lappd_baryTheta);
	tree->Branch("lappd_avgDist",&lappd_avgDist);
	tree->Branch("lappd_totalQ",&lappd_totalQ);
	tree->Branch("lappd_avgT",&lappd_avgT);
	tree->Branch("lappd_hits",&lappd_hits);
	tree->Branch("lappd_rmsTheta",&lappd_rmsTheta);
	tree->Branch("lappd_varTheta",&lappd_varTheta);
	tree->Branch("lappd_rmsThetaBary",&lappd_rmsThetaBary);
	tree->Branch("lappd_varThetaBary",&lappd_varThetaBary);

	tree->Branch("num_mrd_layers",&num_mrd_layers);
	tree->Branch("num_mrd_paddles",&num_mrd_paddles);
	tree->Branch("num_mrd_conslayers",&num_mrd_conslayers);
	tree->Branch("mrd_padperlayer",&mrd_padperlayer);
	tree->Branch("mrd_mean_xspread",&mrd_mean_xspread);
	tree->Branch("mrd_mean_yspread",&mrd_mean_yspread);
	tree->Branch("num_mrd_clusters",&num_mrd_clusters);

	if (!isData){
		tree->Branch("pmt_fracRing",&pmt_fracRing);
		tree->Branch("pmt_fracRingNoWeight",&pmt_fracRingNoWeight);
		tree->Branch("lappd_fracRing",&lappd_fracRing);
		tree->Branch("distWallVert",&distWallVert);
		tree->Branch("distWallHor",&distWallHor);
		tree->Branch("distInnerStrVert",&distInnerStrVert);
		tree->Branch("distInnerStrHor",&distInnerStrHor);
		tree->Branch("true_time",&true_time);
		tree->Branch("energy",&energy);
		tree->Branch("nrings",&nrings);
		tree->Branch("multiplerings",&multiplerings);
		tree->Branch("evnum",&evnum);
		tree->Branch("pdg",&pdg);
        }

	pmtQ_vec = new std::vector<double>;
	pmtT_vec = new std::vector<double>;
	pmtDist_vec = new std::vector<double>;
	pmtTheta_vec = new std::vector<double>;
	pmtThetaBary_vec = new std::vector<double>;
	pmtPhi_vec = new std::vector<double>;
	pmtPhiBary_vec = new std::vector<double>;
	pmtY_vec = new std::vector<double>;
	lappdQ_vec = new std::vector<double>;
	lappdT_vec = new std::vector<double>;
	lappdDist_vec = new std::vector<double>;
	lappdTheta_vec = new std::vector<double>;
	lappdThetaBary_vec = new std::vector<double>;

	tree->Branch("pmtQ_vec",&pmtQ_vec);
	tree->Branch("pmtT_vec",&pmtT_vec);
	tree->Branch("pmtDist_vec",&pmtDist_vec);
	tree->Branch("pmtTheta_vec",&pmtTheta_vec);
	tree->Branch("pmtThetaBary_vec",&pmtThetaBary_vec);
	tree->Branch("pmtPhi_vec",&pmtPhi_vec);
	tree->Branch("pmtPhiBary_vec",&pmtPhiBary_vec);
	tree->Branch("pmtY_vec",&pmtY_vec);
	tree->Branch("lappdQ_vec",&lappdQ_vec);
	tree->Branch("lappdT_vec",&lappdT_vec);
	tree->Branch("lappdDist_vec",&lappdDist_vec);
	tree->Branch("lappdTheta_vec",&lappdTheta_vec);
	tree->Branch("lappdThetaBary_vec",&lappdThetaBary_vec);

}

void StoreClassificationVars::InitCSV(){

	//-----------------------------------------------------------------------
	//-------------------- Initialise CSV file ------------------------------
	//-----------------------------------------------------------------------

	std::stringstream ss_filename_csv;
	ss_filename_csv << filename << "_"<<variable_config<<".csv";
	csv_file.open(ss_filename_csv.str().c_str());

	for (unsigned int i_var = 0; i_var < variable_names.size(); i_var++){
		if (i_var != variable_names.size()-1) csv_file << variable_names.at(i_var)<<",";
		else csv_file << variable_names.at(i_var)<<std::endl;
	}
		
	std::stringstream ss_statusfilename_csv;
	ss_statusfilename_csv << filename << "_status.csv";
	csv_statusfile.open(ss_statusfilename_csv.str().c_str());
	csv_statusfile << "energy, evnum, distWallVert, distWallHor, distInnerStrVert, pmt_fracRing, lappd_fracRing, nrings, multiplerings, pdg"<<std::endl;

	Log("StoreClassificationVars tool: Initialised & opened csv file to write the classification variables to",v_message,verbosity);

}

void StoreClassificationVars::PopulateConfigMap(){
	
	//-----------------------------------------------------------------------
        //------------------ Populate variable config map -----------------------
        //-----------------------------------------------------------------------	

	for (unsigned int i_var=0; i_var < variable_names.size(); i_var++){
		std::string var_name = variable_names.at(i_var);
		if (var_name == "PMTBaryTheta") variable_map.emplace(var_name,pmt_baryTheta);
		else if (var_name == "PMTAvgDist") variable_map.emplace(var_name,pmt_avgDist);
		else if (var_name == "PMTAvgT") variable_map.emplace(var_name,pmt_avgT);
		else if (var_name == "PMTQtotal") variable_map.emplace(var_name,pmt_totalQ);
		else if (var_name == "PMTQtotalClustered") variable_map.emplace(var_name,pmt_totalQ_Clustered);
		else if (var_name == "PMTHits") variable_map.emplace(var_name,pmt_hits);
		else if (var_name == "PMTFracQmax") variable_map.emplace(var_name,pmt_fracHighestQ);
		else if (var_name == "PMTFracQdownstream") variable_map.emplace(var_name,pmt_fracDownstream);
		else if (var_name == "PMTFracClustered") variable_map.emplace(var_name,pmt_fracClustered);
		else if (var_name == "PMTFracLowQ") variable_map.emplace(var_name,pmt_fracLowQ);
		else if (var_name == "PMTFracEarly") variable_map.emplace(var_name,pmt_fracEarlyT);
		else if (var_name == "PMTFracLate") variable_map.emplace(var_name,pmt_fracLateT);
		else if (var_name == "PMTRMSTheta") variable_map.emplace(var_name,pmt_rmsTheta);
		else if (var_name == "PMTVarTheta") variable_map.emplace(var_name,pmt_varTheta);
		else if (var_name == "PMTRMSThetaBary") variable_map.emplace(var_name,pmt_rmsThetaBary);
		else if (var_name == "PMTVarThetaBary") variable_map.emplace(var_name,pmt_varThetaBary);
		else if (var_name == "PMTRMSPhi") variable_map.emplace(var_name,pmt_rmsPhi);
		else if (var_name == "PMTVarPhi") variable_map.emplace(var_name,pmt_varPhi);
		else if (var_name == "PMTRMSPhiBary") variable_map.emplace(var_name,pmt_rmsPhiBary);
		else if (var_name == "PMTVarPhiBary") variable_map.emplace(var_name,pmt_varPhiBary);
		else if (var_name == "PMTFracLargeAnglePhi") variable_map.emplace(var_name,pmt_fracLargeAnglePhi);
		else if (var_name == "PMTFracLargeAngleTheta") variable_map.emplace(var_name,pmt_fracLargeAngleTheta);
		else if (var_name == "PMTHitsLargeAnglePhi") variable_map.emplace(var_name,pmt_hitsLargeAnglePhi);
		else if (var_name == "PMTHitsLargeAngleTheta") variable_map.emplace(var_name,pmt_hitsLargeAngleTheta);
		else if (var_name == "LAPPDBaryTheta") variable_map.emplace(var_name,lappd_baryTheta);
		else if (var_name == "LAPPDAvgDist") variable_map.emplace(var_name,lappd_avgDist);
		else if (var_name == "LAPPDQtotal") variable_map.emplace(var_name,lappd_totalQ);
		else if (var_name == "LAPPDAvgT") variable_map.emplace(var_name,lappd_avgT);
		else if (var_name == "LAPPDHits") variable_map.emplace(var_name,lappd_hits);
		else if (var_name == "LAPPDRMSTheta") variable_map.emplace(var_name,lappd_rmsTheta);
		else if (var_name == "LAPPDVarTheta") variable_map.emplace(var_name,lappd_varTheta);
		else if (var_name == "LAPPDRMSThetaBary") variable_map.emplace(var_name,lappd_rmsThetaBary);
		else if (var_name == "LAPPDVarThetaBary") variable_map.emplace(var_name,lappd_varThetaBary);
		else if (var_name == "MrdLayers") variable_map.emplace(var_name,num_mrd_layers);
		else if (var_name == "MrdPaddles") variable_map.emplace(var_name,num_mrd_paddles);
		else if (var_name == "MrdConsLayers") variable_map.emplace(var_name,num_mrd_conslayers);
		else if (var_name == "MrdAdjHits") variable_map.emplace(var_name,num_mrd_adjacent);
		else if (var_name == "MrdXSpread") variable_map.emplace(var_name,mrd_mean_xspread);
		else if (var_name == "MrdYSpread") variable_map.emplace(var_name,mrd_mean_yspread);
		else if (var_name == "MrdPadPerLayer") variable_map.emplace(var_name,mrd_padperlayer);
		else if (var_name == "MrdClusters") variable_map.emplace(var_name,num_mrd_clusters);
		else if (var_name == "PMTFracRing") variable_map.emplace(var_name,pmt_fracRing);
		else if (var_name == "PMTFracRingNoWeight") variable_map.emplace(var_name,pmt_fracRingNoWeight);
		else if (var_name == "LAPPDFracRing") variable_map.emplace(var_name,lappd_fracRing);
		else if (var_name == "VDistVtxWall") variable_map.emplace(var_name,distWallVert);
		else if (var_name == "HDistVtxWall") variable_map.emplace(var_name,distWallHor);
		else if (var_name == "VDistVtxInner") variable_map.emplace(var_name,distInnerStrVert);
		else if (var_name == "HDistVtxInner") variable_map.emplace(var_name,distInnerStrHor);
		else if (var_name == "VtxTrueTime") variable_map.emplace(var_name,true_time);
		else if (var_name == "TrueEnergy") variable_map.emplace(var_name,energy);
		else if (var_name == "NRings") variable_map.emplace(var_name,nrings);
		else if (var_name == "MultiRing") variable_map.emplace(var_name,multiplerings);
		else if (var_name == "EventNumber") variable_map.emplace(var_name,evnum);
		else if (var_name == "PDG") variable_map.emplace(var_name,pdg);

	}

}

void StoreClassificationVars::UpdateConfigMap(){
	
	//-----------------------------------------------------------------------
        //------------- Update variable config map with new values --------------
        //-----------------------------------------------------------------------	
	
	for (unsigned int i_var=0; i_var < variable_names.size(); i_var++){
		std::string var_name = variable_names.at(i_var);
		if (var_name == "PMTBaryTheta") variable_map[var_name] = pmt_baryTheta;
		else if (var_name == "PMTAvgDist") variable_map[var_name] = pmt_avgDist;
		else if (var_name == "PMTAvgT") variable_map[var_name] = pmt_avgT;
		else if (var_name == "PMTQtotal") variable_map[var_name] = pmt_totalQ;
		else if (var_name == "PMTQtotalClustered") variable_map[var_name] = pmt_totalQ_Clustered;
		else if (var_name == "PMTHits") variable_map[var_name] = pmt_hits;
		else if (var_name == "PMTFracQmax") variable_map[var_name] = pmt_fracHighestQ;
		else if (var_name == "PMTFracQdownstream") variable_map[var_name] = pmt_fracDownstream;
		else if (var_name == "PMTFracClustered") variable_map[var_name] = pmt_fracClustered;
		else if (var_name == "PMTFracLowQ") variable_map[var_name] = pmt_fracLowQ;
		else if (var_name == "PMTFracEarly") variable_map[var_name] = pmt_fracEarlyT;
		else if (var_name == "PMTFracLate") variable_map[var_name] = pmt_fracLateT;
		else if (var_name == "PMTRMSTheta") variable_map[var_name] = pmt_rmsTheta;
		else if (var_name == "PMTVarTheta") variable_map[var_name] = pmt_varTheta;
		else if (var_name == "PMTRMSThetaBary") variable_map[var_name] = pmt_rmsThetaBary;
		else if (var_name == "PMTVarThetaBary") variable_map[var_name] = pmt_varThetaBary;
		else if (var_name == "PMTRMSPhi") variable_map[var_name] = pmt_rmsPhi;
		else if (var_name == "PMTVarPhi") variable_map[var_name] = pmt_varPhi;
		else if (var_name == "PMTRMSPhiBary") variable_map[var_name] = pmt_rmsPhiBary;
		else if (var_name == "PMTVarPhiBary") variable_map[var_name] = pmt_varPhiBary;
		else if (var_name == "PMTFracLargeAnglePhi") variable_map[var_name] = pmt_fracLargeAnglePhi;
		else if (var_name == "PMTFracLargeAngleTheta") variable_map[var_name] = pmt_fracLargeAngleTheta;
		else if (var_name == "PMTHitsLargeAnglePhi") variable_map[var_name] = pmt_hitsLargeAnglePhi;
		else if (var_name == "PMTHitsLargeAngleTheta") variable_map[var_name] = pmt_hitsLargeAngleTheta;
		else if (var_name == "LAPPDBaryTheta") variable_map[var_name] = lappd_baryTheta;
		else if (var_name == "LAPPDAvgDist") variable_map[var_name] = lappd_avgDist;
		else if (var_name == "LAPPDQtotal") variable_map[var_name] = lappd_totalQ;
		else if (var_name == "LAPPDAvgT") variable_map[var_name] = lappd_avgT;
		else if (var_name == "LAPPDHits") variable_map[var_name] = lappd_hits;
		else if (var_name == "LAPPDRMSTheta") variable_map[var_name] = lappd_rmsTheta;
		else if (var_name == "LAPPDVarTheta") variable_map[var_name] = lappd_varTheta;
		else if (var_name == "LAPPDRMSThetaBary") variable_map[var_name] = lappd_rmsThetaBary;
		else if (var_name == "LAPPDVarThetaBary") variable_map[var_name] = lappd_varThetaBary;
		else if (var_name == "MrdLayers") variable_map[var_name] = num_mrd_layers;
		else if (var_name == "MrdPaddles") variable_map[var_name] = num_mrd_paddles;
		else if (var_name == "MrdConsLayers") variable_map[var_name] = num_mrd_conslayers;
		else if (var_name == "MrdAdjHits") variable_map[var_name] = num_mrd_adjacent;
		else if (var_name == "MrdXSpread") variable_map[var_name] = mrd_mean_xspread;
		else if (var_name == "MrdYSpread") variable_map[var_name] = mrd_mean_yspread;
		else if (var_name == "MrdPadPerLayer") variable_map[var_name] = mrd_padperlayer;
		else if (var_name == "MrdClusters") variable_map[var_name] = num_mrd_clusters;
		else if (var_name == "PMTFracRing") variable_map[var_name] = pmt_fracRing;
		else if (var_name == "PMTFracRingNoWeight") variable_map[var_name] = pmt_fracRingNoWeight;
		else if (var_name == "LAPPDFracRing") variable_map[var_name] = lappd_fracRing;
		else if (var_name == "VDistVtxWall") variable_map[var_name] = distWallVert;
		else if (var_name == "HDistVtxWall") variable_map[var_name] = distWallHor;
		else if (var_name == "VDistVtxInner") variable_map[var_name] = distInnerStrVert;
		else if (var_name == "VtxTrueTime") variable_map[var_name] = true_time;
		else if (var_name == "TrueEnergy") variable_map[var_name] = energy;
		else if (var_name == "NRings") variable_map[var_name] = nrings;
		else if (var_name == "MultiRing") variable_map[var_name] = multiplerings;
		else if (var_name == "EventNumber") variable_map[var_name] = evnum;
		else if (var_name == "PDG") variable_map[var_name] = pdg;

	}


}

void StoreClassificationVars::FillClassHistograms(){

	//-----------------------------------------------------------------------
	//--------------- Filling properties into histograms---------------------
	//-----------------------------------------------------------------------
	
	Log("StoreClassificationVars tool: Filling classification variables into histograms.",v_debug,verbosity);

	file->cd();

	hist_pmtHits->Fill(pmt_hits);
	hist_pmtPEtotal->Fill(pmt_totalQ);
	hist_pmtPEtotalClustered->Fill(pmt_totalQ_Clustered);
	hist_pmtAvgTime->Fill(pmt_avgT);
	hist_pmtAvgDist->Fill(pmt_avgDist);
	hist_pmtThetaBary->Fill(pmt_baryTheta);
	hist_pmtThetaRMS->Fill(pmt_rmsTheta);
	hist_pmtThetaVar->Fill(pmt_varTheta);
	hist_pmtThetaBaryRMS->Fill(pmt_rmsThetaBary);
	hist_pmtThetaBaryVar->Fill(pmt_varThetaBary);
	hist_pmtPhiRMS->Fill(pmt_rmsPhi);
	hist_pmtPhiVar->Fill(pmt_varPhi);
	hist_pmtPhiBaryRMS->Fill(pmt_rmsPhiBary);
	hist_pmtPhiBaryVar->Fill(pmt_varPhiBary);
	hist_pmtFracDownstream->Fill(pmt_fracDownstream);
	hist_pmtFracHighestQ->Fill(pmt_fracHighestQ);
	hist_pmtFracClustered->Fill(pmt_fracClustered);
	hist_pmtFracLowCharge->Fill(pmt_fracLowQ);
	hist_pmtFracLateTime->Fill(pmt_fracLateT);
	hist_pmtFracEarlyTime->Fill(pmt_fracEarlyT);
	hist_pmtFracLargeAngleThetaBary->Fill(pmt_fracLargeAngleTheta);
	hist_pmtFracLargeAnglePhiBary->Fill(pmt_fracLargeAnglePhi);
	
	hist_lappdHits->Fill(lappd_hits);
	hist_lappdPEtotal->Fill(lappd_totalQ);
	hist_lappdAvgTime->Fill(lappd_avgT);
	hist_lappdAvgDist->Fill(lappd_avgDist);
	hist_lappdThetaBary->Fill(lappd_baryTheta);
	hist_lappdThetaRMS->Fill(lappd_rmsTheta);
	hist_lappdThetaVar->Fill(lappd_varTheta);
	hist_lappdThetaBaryRMS->Fill(lappd_rmsThetaBary);
	hist_lappdThetaBaryVar->Fill(lappd_varThetaBary);

	hist_mrdPaddles->Fill(num_mrd_paddles);
	hist_mrdLayers->Fill(num_mrd_layers);
	hist_mrdconsLayers->Fill(num_mrd_conslayers);
	hist_mrdClusters->Fill(num_mrd_clusters);
	hist_mrdXSpread->Fill(mrd_mean_xspread);
	hist_mrdYSpread->Fill(mrd_mean_yspread);
	hist_mrdAdjacentHits->Fill(num_mrd_adjacent);
	hist_mrdPaddlesPerLayer->Fill(mrd_padperlayer);
	
	hist_pmtFracRing->Fill(pmt_fracRing);
	hist_pmtFracRingNoWeight->Fill(pmt_fracRingNoWeight);
	hist_lappdFracRing->Fill(lappd_fracRing);
	hist_evnum->Fill(evnum);
	hist_distWallHor->Fill(distWallHor);
	hist_distWallVert->Fill(distWallVert);
	hist_distInnerStrHor->Fill(distInnerStrHor);
	hist_distInnerStrVert->Fill(distInnerStrVert);
	hist_energy->Fill(energy);
	hist_nrings->Fill(nrings);
	hist_multiplerings->Fill(multiplerings);
	hist_pdg->Fill(pdg);
	hist_truetime->Fill(true_time);

	//Detailed (single PMT variables)
	for (unsigned int i_pmt = 0; i_pmt < pmtQ.size(); i_pmt++){
		hist_pmtPE_single->Fill(pmtQ.at(i_pmt));
		hist_pmtTime_single->Fill(pmtT.at(i_pmt));
		hist_pmtTheta_single->Fill(pmtTheta.at(i_pmt));
		hist_pmtPhi_single->Fill(pmtPhi.at(i_pmt));
		hist_pmtY_single->Fill(pmtY.at(i_pmt));
		hist_pmtDist_single->Fill(pmtDist.at(i_pmt));
		hist_pmtThetaBary_single->Fill(pmtThetaBary.at(i_pmt));
		hist_pmtPhiBary_single->Fill(pmtPhiBary.at(i_pmt));
	}

	//Detailed (single LAPPD variables)
	for (unsigned int i_lappd = 0; i_lappd < lappdQ.size(); i_lappd++){
		hist_lappdPE_single->Fill(lappdQ.at(i_lappd));
		hist_lappdTime_single->Fill(lappdT.at(i_lappd));
		hist_lappdDist_single->Fill(lappdDist.at(i_lappd));
		hist_lappdTheta_single->Fill(lappdTheta.at(i_lappd));
		hist_lappdThetaBary_single->Fill(lappdThetaBary.at(i_lappd));
	}


}

void StoreClassificationVars::FillClassTree(){

	//-----------------------------------------------------------------------
        //-------------------- Fill classification tree -------------------------
        //-----------------------------------------------------------------------	

	pmtQ_vec->clear();
        pmtT_vec->clear();
        pmtDist_vec->clear();
        pmtTheta_vec->clear();
        pmtThetaBary_vec->clear();
        pmtPhi_vec->clear();
        pmtPhiBary_vec->clear();
        pmtY_vec->clear();
        lappdQ_vec->clear();
        lappdT_vec->clear();
        lappdDist_vec->clear();
        lappdTheta_vec->clear();
        lappdThetaBary_vec->clear();
	
	for (unsigned int i_pmt = 0; i_pmt < pmtQ.size(); i_pmt++){
	
		pmtQ_vec->push_back(pmtQ.at(i_pmt));
		pmtT_vec->push_back(pmtT.at(i_pmt));
		pmtDist_vec->push_back(pmtDist.at(i_pmt));
		pmtTheta_vec->push_back(pmtTheta.at(i_pmt));
		pmtThetaBary_vec->push_back(pmtThetaBary.at(i_pmt));
		pmtPhi_vec->push_back(pmtPhi.at(i_pmt));
		pmtPhiBary_vec->push_back(pmtPhiBary.at(i_pmt));
		pmtY_vec->push_back(pmtY.at(i_pmt));
	}

	for (unsigned int i_lappd = 0; i_lappd < lappdQ.size(); i_lappd++){
		
		lappdQ_vec->push_back(lappdQ.at(i_lappd));
		lappdT_vec->push_back(lappdT.at(i_lappd));
		lappdDist_vec->push_back(lappdDist.at(i_lappd));
		lappdTheta_vec->push_back(lappdTheta.at(i_lappd));
		lappdThetaBary_vec->push_back(lappdThetaBary.at(i_lappd));
	}

	tree->Fill();

}

void StoreClassificationVars::FillCSV(){

	//-----------------------------------------------------------------------
        //-------------------- Fill variables in csv-file -----------------------
        //-----------------------------------------------------------------------	
		
	Log("StoreClassificationVars tool: Storing properties in csv-file: ",v_debug,verbosity);

	for (unsigned int i_var = 0; i_var < variable_names.size(); i_var++){
		if (i_var != variable_names.size()-1) csv_file << variable_map[variable_names.at(i_var)] <<",";
		else csv_file << variable_map[variable_names.at(i_var)] << std::endl;
	}

	//TODO: put extraction of status varibles into a separate tool
	csv_statusfile << energy
	<<","<<evnum
	<<","<<distWallVert
	<<","<<distWallHor
	<<","<<distInnerStrVert
	<<","<<distInnerStrHor
	<<","<<pmt_fracRing
	<<","<<lappd_fracRing
	<<","<<nrings
	<<","<<multiplerings
	<<","<<pdg
	<<std::endl;
	//ODOT

}

void StoreClassificationVars::WriteClassHistograms(){

	//-----------------------------------------------------------------------
        //-------------- Write overview histograms to file ----------------------
        //-----------------------------------------------------------------------	
		
	Log("StoreClassificationVars tool: Writing histograms to root-file.",v_message,verbosity);
		
  	file->cd();
 
	//PMT-single histograms
 		        
        hist_pmtPE_single->Write();
        hist_pmtTime_single->Write();
	hist_pmtTheta_single->Write();
	hist_pmtPhi_single->Write();
	hist_pmtY_single->Write();
	hist_pmtDist_single->Write();
	hist_pmtThetaBary_single->Write();
	hist_pmtPhiBary_single->Write();
        
	//PMT-average histograms

	hist_pmtHits->Write();
        hist_pmtPEtotal->Write();
        hist_pmtAvgTime->Write();
        hist_pmtThetaBary->Write();
        hist_pmtThetaRMS->Write();
        hist_pmtThetaVar->Write();
       	hist_pmtThetaBaryRMS->Write();
        hist_pmtThetaBaryVar->Write();
	hist_pmtPhiRMS->Write();
	hist_pmtPhiVar->Write();
	hist_pmtPhiBaryRMS->Write();
	hist_pmtPhiBaryVar->Write();
        hist_pmtFracDownstream->Write();
        hist_pmtFracRing->Write();
        hist_pmtFracRingNoWeight->Write();
        hist_pmtFracHighestQ->Write();
	hist_pmtFracClustered->Write();
	hist_pmtFracLowCharge->Write();
	hist_pmtFracLateTime->Write();
	hist_pmtFracEarlyTime->Write();
	hist_pmtFracLargeAngleThetaBary->Write();
	hist_pmtFracLargeAnglePhiBary->Write();

	//LAPPD histograms
	
        hist_lappdPE_single->Write();
        hist_lappdTime_single->Write();
        hist_lappdTheta_single->Write();
        hist_lappdDist_single->Write();
        hist_lappdThetaBary_single->Write();
	hist_lappdHits->Write();
        hist_lappdPEtotal->Write();
       	hist_lappdAvgTime->Write();
        hist_lappdThetaBary->Write();
        hist_lappdThetaRMS->Write();
        hist_lappdThetaVar->Write();
        hist_lappdThetaBaryRMS->Write();
        hist_lappdThetaBaryVar->Write();
        hist_lappdFracRing->Write();

	//MRD histograms

	hist_mrdPaddles->Write();
	hist_mrdLayers->Write();
	hist_mrdconsLayers->Write();
	hist_mrdClusters->Write();	
	hist_mrdXSpread->Write();
	hist_mrdYSpread->Write();
	hist_mrdAdjacentHits->Write();
	hist_mrdPaddlesPerLayer->Write();

	//MC-Truth histograms

	hist_evnum->Write();
	hist_distWallHor->Write();
	hist_distWallVert->Write();
	hist_distInnerStrHor->Write();
	hist_distInnerStrVert->Write();
	hist_energy->Write();
	hist_nrings->Write();

	hist_multiplerings->Write();

	//Write tree
	tree->Write();

	file->Close();
	delete file;

	Log("StoreClassificationVars Tool: Histograms written to root-file.",v_message,verbosity);

}
