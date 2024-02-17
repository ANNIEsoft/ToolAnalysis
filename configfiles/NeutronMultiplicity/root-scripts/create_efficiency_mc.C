void create_efficiency_mc_new(int port, int height){

	std::stringstream file_out;
	file_out <<"ambe_mc_eff_port"<<port<<"_z"<<height<<".root";
	
	std::string global_path = "/pnfs/annie/persistent/users/mnieslon/ntuples/mc/AmBe/";
	std::stringstream ntuple_name;
	ntuple_name <<  global_path << "MC_AmBe_ANNRI_PMTwiseQE_Port"<<port<<"_z"<<height<<"_0-24.ntuple.root";

	TFile *fin = new TFile(ntuple_name.str().c_str(),"READ");

	TTree *t = (TTree*) fin->Get("phaseIITankClusterTree");
	TTree *tTrigger = (TTree*) fin->Get("phaseIITriggerTree");	

	int sipm1pulses;
	int sipm2pulses;
	std::vector<double>* sipmhitT = new std::vector<double>;
	std::vector<double>* hitT = new std::vector<double>;
	std::vector<double>* hitPE = new std::vector<double>;
	std::vector<double>* hitChankey = new std::vector<double>;
	double clusterPE;
	double clusterT;
	double clusterCB;
	double clusterMaxPE;
	uint32_t clusterHits;
	int evnumber;
        int clusternumber;

	//t->SetBranchAddress("SiPM1NPulses",&sipm1pulses);
	//t->SetBranchAddress("SiPM2NPulses",&sipm2pulses);
	//t->SetBranchAddress("SiPMhitT",&sipmhitT);
	t->SetBranchAddress("hitT",&hitT);
	t->SetBranchAddress("hitPE",&hitPE);
	t->SetBranchAddress("hitChankey",&hitChankey);
	t->SetBranchAddress("clusterPE",&clusterPE);
	t->SetBranchAddress("clusterMaxPE",&clusterMaxPE);
	t->SetBranchAddress("clusterTime",&clusterT);
	t->SetBranchAddress("clusterChargeBalance",&clusterCB);	
	t->SetBranchAddress("eventNumber",&evnumber);
        t->SetBranchAddress("clusterNumber",&clusternumber);
	t->SetBranchAddress("clusterHits",&clusterHits);

	int entries = t->GetEntries();
	int entriesTrigger = tTrigger->GetEntries();

	int entries_time_range=0;
	std::vector<double> *nT = new std::vector<double>;
	tTrigger->SetBranchAddress("trueNeutCapTime",&nT);

	bool prompt = false;
	bool last_event = -1;
	std::vector<double> pmt_charges;
	std::vector<double> pmt_charges_corrected;

	//std::cout <<"Tree entries: "<<entries<<std::endl;

	int n_candidates_NHits10 = 0;
	int n_candidates_CB = 0;
	int n_candidates_CBStrict = 0;
	int n_events = 0;
	int n_entries = entries;

	double tmin = 10000;
	double tmax = 67000;

	for (int i_trigger=0; i_trigger < entriesTrigger; i_trigger++){
		tTrigger->GetEntry(i_trigger);
		for (int i_n=0; i_n< nT->size(); i_n++){
			if (nT->at(i_n) >= tmin && nT->at(i_n) <= tmax) entries_time_range++;
		}
	} 

	for (int i_entry = 0; i_entry < entries; i_entry++){

		t->GetEntry(i_entry);

		//std::cout <<i_entry<<"/"<<entries<<std::endl;

		if (evnumber != last_event){
			last_event = evnumber;
			prompt = false;
		}

		if (prompt) continue;

		n_events++;

		//std::cout <<"sipm1pulses: "<<sipm1pulses<<", sipm2pulses: "<<sipm2pulses<<std::endl;

		//1 pulse in each SiPM
		//if (!(sipm1pulses==1 && sipm2pulses==1)) continue;

		//SiPM peaks within 200ns
		/*int sipm_size = (int) sipmhitT->size();
		std::cout <<"sipm hits size: "<<sipm_size;
		if (sipm_size != 2) continue;
		
		double delta_t = sipmhitT->at(1)-sipmhitT->at(0);
		if (fabs(delta_t)>200) {
			std::cout <<"Time difference between sipm peaks > 200ns! ("<<delta_t<<")"<<std::endl;
			continue;
		}*/

		//No burst above 150pe
		if (clusterMaxPE > 150) continue;

		//Timing cut (standard > 10us and <67us)
		//10us due to afterpulsing -> search for better discrimination in the future
		//67us due to readout window
		if (clusterT < tmin || clusterT > tmax) continue;

		//CB < 0.4
		if (clusterCB <= 0.4) n_candidates_CB++;
		
		if (clusterHits >= 10) n_candidates_NHits10++;

		if ((clusterCB <= 0.4) && (clusterCB <= (1.-clusterPE/150.)*0.5)) n_candidates_CBStrict++;
	}

	fin->Close();
	delete fin;

	ofstream eff_file_nhits("ambe_eff_mc_NHits10.txt",ios_base::app);
	eff_file_nhits <<port<<","<<height<<","<<n_candidates<<","<<entriesTrigger<<","<<double(n_candidates_Nhits10)/entries_time_range<<","<<double(entries_time_range)/entriesTrigger<<std::endl;
	eff_file_nhits.close();
	ofstream eff_file_cb("ambe_eff_mc_CB.txt",ios_base::app);
	eff_file_cb <<port<<","<<height<<","<<n_candidates<<","<<entriesTrigger<<","<<double(n_candidates_CB)/entries_time_range<<","<<double(entries_time_range)/entriesTrigger<<std::endl;
	eff_file_cb.close();
	ofstream eff_file_mc("ambe_eff_mc_CBStrict.txt",ios_base::app);
	eff_file_cbstrict <<port<<","<<height<<","<<n_candidates<<","<<entriesTrigger<<","<<double(n_candidates_CBStrict)/entries_time_range<<","<<double(entries_time_range)/entriesTrigger<<std::endl;
	eff_file_cbstrict.close();

}
