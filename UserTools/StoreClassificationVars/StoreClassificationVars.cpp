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
	variable_config = "VariableConfig_Full.txt";
	variable_config_path = "./configfiles/Classification/PrepareClassification";
	histogram_config = "Variable_HistConfig.txt";

	//---------------------------------------------------------------
	//----------------- Configuration variables ---------------------
	//---------------------------------------------------------------
	
	m_variables.Get("verbosity",verbosity);
	m_variables.Get("Filename",filename);
	m_variables.Get("SaveCSV",save_csv);
	m_variables.Get("SaveROOT",save_root);
	m_variables.Get("VariableConfig",variable_config);
	m_variables.Get("VariableConfigPath",variable_config_path);
	m_variables.Get("HistogramConfig",histogram_config);
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

	// Load histogram configuration
	bool hist_config;
	hist_config = this->LoadHistConfig(histogram_config);
	if (!hist_config){
		Log("StoreClassificationVars Tool: Chosen histogram configuration file >>>"+histogram_config+"<<< does not exist. Use default values.",v_error,verbosity);
	}

	// Set MCTruth variables
	mc_names = {"MCTrueMuonEnergy", "MCTrueNeutrinoEnergy",/* "MCFilename",*/ "EventNumber", "MCNeutrons", "MCVDistVtxWall", "MCHDistVtxWall", "MCVDistVtxInner", "MCHDistVtxInner", "MCPMTFracRing","MCPMTFracRingNoWeight", "MCLAPPDFracRing", "MCPMTVarTheta","MCPMTRMSTheta","MCPMTBaryTheta","MCPMTRMSThetaBary","MCPMTVarThetaBary","MCPMTThetaBaryVector","MCLAPPDVarTheta","MCLAPPDRMSTheta","MCLAPPDBaryTheta","MCLAPPDRMSThetaBary","MCLAPPDVarThetaBary","MCLAPPDThetaBaryVector","MCPMTTVectorTOF","MCLAPPDTVectorTOF","MCNRings", "MCMultiRing", "MCPDG"};

	// Set vector variables
	vec_names = {"PMTQVector","PMTTVector","PMTDistVector","PMTThetaVector","PMTThetaBaryVector","PMTPhiVector","PMTPhiBaryVector","PMTYVector","PMTIDVector","LAPPDQVector","LAPPDTVector","LAPPDDistVector","LAPPDThetaVector","LAPPDThetaBaryVector","LAPPDIDVector"};


	classification_map_map.clear();
	m_data->Stores["Classification"]->Get("ClassificationMapMap",classification_map_map);

	//Initialise ROOT histograms
	if (save_root) {
		this->InitClassHistograms();
	}

	//Create & initialise CSV file
	if (save_csv) this->InitCSV();


	Log("StoreClassificationVars Tool: Initialization complete",v_message,verbosity);

	return true;
}


bool StoreClassificationVars::Execute(){

	Log("StoreClassificationVars Tool: Executing",v_debug,verbosity);

	// Get variables from Classification store
	get_ok = m_data->Stores.count("Classification");
	if(!get_ok){
		Log("StoreClassificationVars Tool: No Classification store! Please run CalcClassificationVars before this tool. Exiting...",v_error,verbosity);
		return false;
	};

	// Was the Event selection passed?
	bool selection_passed;
	m_data->Stores["Classification"]->Get("SelectionPassed",selection_passed);
	
	if (!selection_passed) {
		Log("StoreClassificationVars Tool: EventSelection cuts were not passed. Skip this event",v_message,verbosity);
		return true;
	}
	
	
        // General variables
        m_data->Stores["Classification"]->Get("isData",isData);
	m_data->Stores["Classification"]->Get("MLDataPresent",mldata_present);
	
	if (!mldata_present){
		Log("StoreClassificationVars Tool: MLData is not present. Abort",v_message,verbosity);
		return true;
	}

	// Variable map variables
	m_data->Stores["Classification"]->Get("ClassificationMapInt",classification_map_int);
	m_data->Stores["Classification"]->Get("ClassificationMapDouble",classification_map_double);
	m_data->Stores["Classification"]->Get("ClassificationMapBool",classification_map_bool);
	m_data->Stores["Classification"]->Get("ClassificationMapVector",classification_map_vector);

	if (i_loop == 0){
		this->InitClassTree();
	}

	if (!isData){
		this->EvaluatePionEnergies();
	}

	//Fill classification variables in ROOT histograms/trees/csv-files
	this->FillClassificationVars(variable_names,false);
	
	//Fill vector variables in ROOT histograms/trees
	this->FillClassificationVars(vec_names,false);

	//Fill MC variables in ROOT histograms/trees/csv-files
	if (!isData) this->FillClassificationVars(mc_names,true);

	if (save_root) tree->Fill();

	i_loop++;
	return true;
}

bool StoreClassificationVars::Finalise(){

	if (save_root){
		this->WriteClassHistograms();
	}

	if (save_csv){
		csv_file.close();
		csv_pion_energies.close();
		if (!isData) csv_statusfile.close();
	}

	return true;

}

bool StoreClassificationVars::LoadVariableConfig(std::string config_name){

	bool config_loaded = false;
	std::stringstream config_filename;
	config_filename << variable_config_path << "/VariableConfig_" <<config_name<<".txt";

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

bool StoreClassificationVars::LoadHistConfig(std::string histogram_config){

	bool hist_setting_loaded = false;
	std::stringstream ss_histfile;
	ss_histfile << variable_config_path+"/"+histogram_config;

	ifstream histfile(ss_histfile.str().c_str());
	if (!histfile.good()){
		Log("StoreClassificationVars tool: Histogram config file "+histogram_config+" does not seem to exist. Abort",v_error,verbosity);
		return false;
	}

	std::string temp_string;
	int temp_nbins;
	double temp_min, temp_max;

	while (!histfile.eof()){
		histfile >> temp_string >> temp_nbins >> temp_min >> temp_max;
		if (histfile.eof()) break;
		n_bins.emplace(temp_string,temp_nbins);
		min_bins.emplace(temp_string,temp_min);
		max_bins.emplace(temp_string,temp_max);
	}
	histfile.close();
	hist_setting_loaded = true;

	return hist_setting_loaded;

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
	std::vector<std::vector<std::string>> vec_variables;
	if (isData) vec_variables = {variable_names, vec_names};
	else vec_variables = {variable_names,vec_names,mc_names};

	for (unsigned int i_set = 0; i_set < vec_variables.size(); i_set++){
		for (unsigned int i_var = 0; i_var < vec_variables[i_set].size(); i_var++){
			std::string varname = vec_variables[i_set].at(i_var);
			int nbins = n_bins.at(varname);
			double min_bin = min_bins.at(varname);
			double max_bin = max_bins.at(varname);
			std::stringstream ss_histname, ss_histtitle;
			ss_histname << "hist_"<<varname;
			ss_histtitle << varname;
			TH1F *h_temp = new TH1F(ss_histname.str().c_str(),ss_histtitle.str().c_str(),nbins,min_bin,max_bin);
			vector_hist.emplace(varname,h_temp);
		}
	}

}

void StoreClassificationVars::InitClassTree(){

	//-----------------------------------------------------------------------
	//-------------------- Initialise classification tree--------------------
	//-----------------------------------------------------------------------

	Log("StoreClassificationVars tool: Initialising classification tree.",v_message,verbosity);

	file->cd();
	tree = new TTree("classification_tree","Classification tree");

	std::vector<std::vector<std::string>> vec_variables;
	if (isData) vec_variables = {variable_names, vec_names};
	else vec_variables = {variable_names,vec_names,mc_names};

	for (unsigned int i_set = 0; i_set < vec_variables.size(); i_set++){
		for (unsigned int i_var=0; i_var < vec_variables[i_set].size(); i_var++){

			std::string variable = vec_variables[i_set].at(i_var);
			if (classification_map_map[variable] == 1){
				classification_map_int_copy.emplace(variable,-1);
				tree->Branch(variable.c_str(),&classification_map_int_copy[variable]);
			}
			else if (classification_map_map[variable] == 2){
				classification_map_double_copy.emplace(variable,-1.);
				tree->Branch(variable.c_str(),&classification_map_double_copy[variable]);
			}
			else if (classification_map_map[variable] == 3){
				classification_map_bool_copy.emplace(variable,0);
				tree->Branch(variable.c_str(),&classification_map_bool_copy[variable]);
			}
			else if (classification_map_map[variable] == 4){
				std::vector<double> *temp_vector = new std::vector<double>;
				classification_map_vector_pointer.emplace(variable,temp_vector);
				tree->Branch(variable.c_str(),&classification_map_vector_pointer[variable]);
			}
		}
	}

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
		
	if (!isData){

		std::stringstream ss_statusfilename_csv;
		ss_statusfilename_csv << filename << "_status.csv";
		csv_statusfile.open(ss_statusfilename_csv.str().c_str());
		for (unsigned i_mc=0; i_mc < mc_names.size(); i_mc++){
			if (mc_names.at(i_mc) == "MCPMTThetaBaryVector" || mc_names.at(i_mc) == "MCLAPPDThetaBaryVector" || mc_names.at(i_mc) == "MCPMTTVectorTOF" || mc_names.at(i_mc) == "MCLAPPDTVectorTOF") continue;
			if (i_mc != mc_names.size()-1) csv_statusfile << mc_names.at(i_mc)<<",";
			else csv_statusfile << mc_names.at(i_mc)<<std::endl;
		}

		std::stringstream ss_filename_pion;
        	ss_filename_pion << filename << "_pion_energies.csv";
		csv_pion_energies.open(ss_filename_pion.str().c_str());
		csv_pion_energies << "Pi+, Pi-, Pi0, K+, K-"<<std::endl;
 	}

	Log("StoreClassificationVars tool: Initialised & opened csv file to write the classification variables to",v_message,verbosity);

}

void StoreClassificationVars::FillClassificationVars(std::vector<std::string> variable_vector,bool isMC){

	for (unsigned int i_var = 0; i_var < variable_vector.size(); i_var++){
		//Check the type of the variable
		std::string variable = variable_vector.at(i_var);

		if (classification_map_map[variable] == 1){
			int value = classification_map_int[variable];
			classification_map_int_copy[variable] = value;
			if (save_root) vector_hist[variable]->Fill(value);
			if (save_csv){
				if (isMC){
					if (i_var != variable_vector.size()-1) csv_statusfile << value << ",";
					else csv_statusfile << value << std::endl;

				}
				else {
					if (i_var != variable_vector.size()-1) csv_file << value << ",";
					else csv_file << value << std::endl;
				}
			}
		}
		if (classification_map_map[variable] == 2){
			double value = classification_map_double[variable];
			classification_map_double_copy[variable] = value;
			if (save_root) vector_hist[variable]->Fill(value);
			if (save_csv){
				if (isMC){
					if (i_var != variable_vector.size()-1) csv_statusfile << value << ",";
					else csv_statusfile << value << std::endl;
				}
				else {
					if (i_var != variable_vector.size()-1) csv_file << value << ",";
					else csv_file << value << std::endl;
				}
			}
		}
		if (classification_map_map[variable] == 3){
			bool value = classification_map_bool[variable];
			classification_map_bool_copy[variable] = value;
			if (save_root) vector_hist[variable]->Fill(value);
			if (save_csv){
				if (isMC){
					if (i_var != variable_vector.size()-1) csv_statusfile << value << ",";
					else csv_statusfile << value << std::endl;
				}
				else {
					if (i_var != variable_vector.size()-1) csv_file << value << ",";
					else csv_file << value << std::endl;
				}
			}
		}
		if (classification_map_map[variable] == 4){
			std::vector<double> value_vector = classification_map_vector[variable];
			if (save_root){
				classification_map_vector_pointer[variable]->clear();
				for (unsigned int i_vector=0; i_vector < value_vector.size(); i_vector++){
					double value = value_vector.at(i_vector);
					vector_hist[variable]->Fill(value);
					classification_map_vector_pointer[variable]->push_back(value);
				}
			}
		}
	}

}


void StoreClassificationVars::WriteClassHistograms(){

	//-----------------------------------------------------------------------
        //-------------- Write overview histograms to file ----------------------
        //-----------------------------------------------------------------------	
		
	Log("StoreClassificationVars tool: Writing histograms to root-file.",v_message,verbosity);
		
  	file->cd();
 	file->Write();

	file->Close();
	delete file;

	Log("StoreClassificationVars Tool: Histograms written to root-file.",v_message,verbosity);

}

void StoreClassificationVars::EvaluatePionEnergies(){

        m_data->Stores["Classification"]->Get("MCPionEnergies",map_pion_energies);
	m_data->Stores["Classification"]->Get("MCNeutrons",n_neutrons);

	int n_pip=0;
	int n_pim=0;
	int n_pi0=0;
	int n_kp=0;
	int n_km=0;
	std::vector<double> vec_pip;
	std::vector<double> vec_pim;
	std::vector<double> vec_pi0;
	std::vector<double> vec_kp;
	std::vector<double> vec_km;

	std::map<int,std::vector<double>>::iterator it;
	std::cout <<"Storevars: map_pion_energies size: "<<map_pion_energies.size()<<std::endl;
	for (it = map_pion_energies.begin(); it != map_pion_energies.end(); it++){

		int pdg_code = it->first;
		std::vector<double> particle_energy = it->second;
		std::cout <<"pdg_code: "<<pdg_code<<std::endl;

		if (pdg_code == 111){
			n_pi0+=particle_energy.size();
			for (int i=0; i<int(particle_energy.size()); i++){
				vec_pi0.push_back(particle_energy.at(i));
			}			
		}	
		if (pdg_code == 211){
			n_pip+=particle_energy.size();
			for (int i=0; i<int(particle_energy.size()); i++){
				vec_pip.push_back(particle_energy.at(i));
			}
		}	
		if (pdg_code == -211){
			n_pim+=particle_energy.size();
			for (int i=0; i<int(particle_energy.size()); i++){
				vec_pim.push_back(particle_energy.at(i));
			}
		}	
		if (pdg_code == 321){
			n_kp+=particle_energy.size();
			for (int i=0; i<int(particle_energy.size()); i++){
				vec_kp.push_back(particle_energy.at(i));
			}
		}	
		if (pdg_code == -321){
			n_km+=particle_energy.size();
			for (int i=0; i<int(particle_energy.size()); i++){
				vec_km.push_back(particle_energy.at(i));
			}
		}	
	}

	// Fill to csv-file
	csv_pion_energies << n_pip <<",";
	for (int i=0; i<int(vec_pip.size()); i++){
		csv_pion_energies << vec_pip.at(i) <<",";
	}
	csv_pion_energies << n_pim <<",";
	for (int i=0; i<int(vec_pim.size()); i++){
		csv_pion_energies << vec_pim.at(i) <<",";
	}
	csv_pion_energies << n_pi0 <<",";
	for (int i=0; i<int(vec_pi0.size()); i++){
		csv_pion_energies << vec_pi0.at(i) <<",";
	}
	csv_pion_energies << n_kp <<",";
	for (int i=0; i<int(vec_kp.size()); i++){
		csv_pion_energies << vec_kp.at(i) <<",";
	}
	csv_pion_energies << n_km;
	if (int(vec_km.size())!=0) csv_pion_energies << ",";
	for (int i=0; i<int(vec_km.size()); i++){
		csv_pion_energies << vec_km.at(i);
		if (i != int(vec_km.size())-1) csv_pion_energies <<",";
	}
	csv_pion_energies<<std::endl;

}
