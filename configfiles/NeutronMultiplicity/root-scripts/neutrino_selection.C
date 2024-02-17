void neutrino_selection(const char* infile, const char* outfile, bool verbose = false){

	TFile *f = new TFile(infile,"READ");
	TTree *tTrigger = (TTree*) f->Get("phaseIITriggerTree");
	TTree *tMRD = (TTree*) f->Get("phaseIIMRDClusterTree");
	TTree *tPMT = (TTree*) f->Get("phaseIITankClusterTree");

	TFile *fout = new TFile(outfile,"RECREATE");

	int nentries_trigger = tTrigger->GetEntries();
	int nentries_mrd = tMRD->GetEntries();
	int nentries_pmt = tPMT->GetEntries();

	std::cout << "Entries Trigger/MRD/PMT: "<<nentries_trigger<<","<<nentries_mrd<<","<<nentries_pmt<<std::endl;

	int run_trigger;
	int run_mrd;
	int run_pmt;

	int ev_trigger;
	int ev_mrd;
	int ev_pmt;

	ULong64_t eventTimeTank;

	int beam_ok;
	int num_mrd_tracks;
        int has_tank;
	std::vector<double>* eloss = new std::vector<double>;
	std::vector<bool>* mrdstop = new std::vector<bool>;
	std::vector<double>* mrdstartx = new std::vector<double>;
	std::vector<double>* mrdstarty = new std::vector<double>;
	std::vector<double>* mrdstartz = new std::vector<double>;
	std::vector<double>* mrdstopx = new std::vector<double>;
	std::vector<double>* mrdstopy = new std::vector<double>;
	std::vector<double>* mrdstopz = new std::vector<double>;

	double cluster_time;
	double cluster_cb;
	double cluster_pe;
	int extended;
	int trigword;
	int extended_trigger;

	int tankmrdcoinc;
	int noveto;
	double true_muone;
	int true_pdg;
	double true_vtxx;
	double true_vtxy;
	double true_vtxz;

	tTrigger->SetBranchAddress("runNumber",&run_trigger);
	tMRD->SetBranchAddress("runNumber",&run_mrd);
	tPMT->SetBranchAddress("runNumber",&run_pmt);
	tTrigger->SetBranchAddress("eventNumber",&ev_trigger);
	tMRD->SetBranchAddress("eventNumber",&ev_mrd);
	tPMT->SetBranchAddress("eventNumber",&ev_pmt);
	tTrigger->SetBranchAddress("eventTimeTank",&eventTimeTank);
	tTrigger->SetBranchAddress("beam_ok",&beam_ok);
	tTrigger->SetBranchAddress("trigword",&trigword);
	tTrigger->SetBranchAddress("numMRDTracks",&num_mrd_tracks);
	tTrigger->SetBranchAddress("MRDEnergyLoss",&eloss);
        tTrigger->SetBranchAddress("MRDTrackStartX",&mrdstartx);
        tTrigger->SetBranchAddress("MRDTrackStartY",&mrdstarty);
        tTrigger->SetBranchAddress("MRDTrackStartZ",&mrdstartz);
        tTrigger->SetBranchAddress("MRDTrackStopX",&mrdstopx);
        tTrigger->SetBranchAddress("MRDTrackStopY",&mrdstopy);
        tTrigger->SetBranchAddress("MRDTrackStopZ",&mrdstopz);
	tTrigger->SetBranchAddress("MRDStop",&mrdstop);
        tTrigger->SetBranchAddress("HasTank",&has_tank);
	tPMT->SetBranchAddress("clusterTime",&cluster_time);
	tPMT->SetBranchAddress("clusterPE",&cluster_pe);
	tPMT->SetBranchAddress("clusterChargeBalance",&cluster_cb);
	tPMT->SetBranchAddress("Extended",&extended);
	tTrigger->SetBranchAddress("Extended",&extended_trigger);
	tTrigger->SetBranchAddress("TankMRDCoinc",&tankmrdcoinc);
	tTrigger->SetBranchAddress("NoVeto",&noveto);
	tTrigger->SetBranchAddress("trueMuonEnergy",&true_muone);
	tTrigger->SetBranchAddress("truePrimaryPdg",&true_pdg);
	tTrigger->SetBranchAddress("trueVtxX",&true_vtxx);
	tTrigger->SetBranchAddress("trueVtxY",&true_vtxy);
	tTrigger->SetBranchAddress("trueVtxZ",&true_vtxz);

	TH1F *h_time_beam = new TH1F("h_time_beam","PMT Cluster time distribution (beam events)",200,0,2000);
	TH1F *h_time_beam_mrdcoinc = new TH1F("h_time_beam_mrdcoinc","PMT Cluster time distribution (beam events + MRD coincidence)",200,0,2000);
	TH1F *h_time_beam_mrdcoinc_noveto = new TH1F("h_time_beam_mrdcoinc_noveto","PMT Cluster time distribution (beam events + MRD coincidence + no veto)",200,0,2000);
	TH1F *h_time_beam_mrdcoinc_noveto_beamok = new TH1F("h_time_beam_mrdcoinc_noveto_beamok","PMT Cluster time distribution (beam events + MRD coincidence + no veto + beam ok)",200,0,2000);
	TH1F *h_time_beam_mrdcoinc_noveto_beamok_extended = new TH1F("h_time_beam_mrdcoinc_noveto_beamok_extended","PMT Cluster time distribution (beam events + MRD coincidence + no veto + beam ok + extended)",200,0,2000);
	TH1F *h_charge_beam = new TH1F("h_charge_beam","PMT Cluster charge distribution (beam events)",500,0,5000);
	TH1F *h_charge_beam_mrdcoinc = new TH1F("h_charge_beam_mrdcoinc","PMT Cluster charge distribution (beam events + MRD coincidence)",500,0,5000);
	TH1F *h_charge_beam_mrdcoinc_noveto = new TH1F("h_charge_beam_mrdcoinc_noveto","PMT Cluster charge distribution (beam events + MRD coincidence + no veto)",500,0,5000);
	TH1F *h_charge_beam_mrdcoinc_noveto_beamok = new TH1F("h_charge_beam_mrdcoinc_noveto_beamok","PMT Cluster charge distribution (beam events + MRD coincidence + no veto + beam ok)",500,0,5000);
	TH1F *h_charge_beam_mrdcoinc_noveto_beamok_extended = new TH1F("h_charge_beam_mrdcoinc_noveto_beamok_extended","PMT Cluster charge distribution (beam events + MRD coincidence + no veto + beam ok + extended)",500,0,5000);

	TTree *tout = new TTree("selection_tree","Data selection tree");
	int mrdcoinc;
	int no_veto;
	int beamok;
	int is_extended;
	double clusterT;
	double clusterQ;
	int run_nr;
	int ev_nr;
	ULong64_t ev_timestamp;
	tout->Branch("mrdcoinc",&mrdcoinc);
	tout->Branch("no_veto",&no_veto);
	tout->Branch("beamok",&beamok);
	tout->Branch("is_extended",&is_extended);
	tout->Branch("clusterT",&clusterT);
	tout->Branch("clusterQ",&clusterQ);
	tout->Branch("run_nr",&run_nr);
	tout->Branch("ev_nr",&ev_nr);
	tout->Branch("ev_timestamp",&ev_timestamp);

	int i_pmt=0;
	for (int i_trig=0; i_trig < nentries_trigger; i_trig++){
		std::cout <<"Load trig entry "<<i_trig<<"/"<<nentries_trigger<<std::endl;
		tTrigger->GetEntry(i_trig);
		bool found_coinc=false;
		double max_pe = 0;
		mrdcoinc = 0;
		no_veto = 0;
		beamok = 0;
		is_extended = 0;
		clusterT = -999;
		clusterQ = -999;
		run_nr = -999;
		ev_nr = -999;
		ev_timestamp = 0;
		if (trigword != 5) continue;		

		bool check_above=false;
                bool check_below = false;
                bool run_switch=false;

		while (i_pmt < nentries_pmt){
			tPMT->GetEntry(i_pmt);
			if (verbose){
				std::cout <<"i_pmt: "<<i_pmt<<"run (PMT): "<<run_pmt<<", run (Trigger): "<<run_trigger<<std::endl;
				std::cout <<"i_pmt: "<<i_pmt<<"event (PMT): "<<ev_pmt<<", event (Trigger): "<<ev_trigger<<std::endl;
			}
			if (run_trigger == run_pmt){
				if (ev_trigger == ev_pmt){
					if (cluster_time < 2000 && cluster_pe > max_pe) {
						max_pe = cluster_pe;
						clusterT = cluster_time;
						clusterQ = cluster_pe;
					}
					i_pmt++;
					found_coinc = true;
				
				} else if (found_coinc) break;

 				else if (ev_pmt < ev_trigger) {i_pmt++; check_below=true;}
                                else if (ev_pmt > ev_trigger && i_pmt > 0 && !run_switch) {i_pmt--; check_above=true;}
                                if (run_switch && ev_pmt > ev_trigger) break;
                                if (i_pmt ==0) break;
                                if (check_below && check_above) break;
                        }
                        else if (run_pmt < run_trigger) {i_pmt++; run_switch=true;}
                        else if (i_pmt > 0) break;
		}
		if (extended_trigger) is_extended = 1;
		if (tankmrdcoinc) mrdcoinc = 1;
		if (noveto) no_veto = 1;
		if (beam_ok) beamok = 1;
		if (tankmrdcoinc && noveto && beam_ok) {
			std::cout <<"Found event that passed selection, run_trigger = "<<run_trigger <<", ev_trigger = "<<ev_trigger<<", run_pmt = "<<run_pmt << ", ev_pmt = "<<ev_pmt<<std::endl;
			std::cout <<"Extended = "<<extended<<", is_extended = "<<is_extended<<", extended_trigger = "<<extended_trigger << std::endl;
		}
		run_nr = run_trigger;
		ev_nr = ev_trigger;
		ev_timestamp = eventTimeTank;
		tout->Fill();		

		h_time_beam->Fill(clusterT);
		h_charge_beam->Fill(clusterQ);
		if (mrdcoinc){
			h_time_beam_mrdcoinc->Fill(clusterT);
			h_charge_beam_mrdcoinc->Fill(clusterQ);
			if (noveto){
				h_time_beam_mrdcoinc_noveto->Fill(clusterT);
				h_charge_beam_mrdcoinc_noveto->Fill(clusterQ);
				if (beam_ok){
					h_time_beam_mrdcoinc_noveto_beamok->Fill(clusterT);
					h_charge_beam_mrdcoinc_noveto_beamok->Fill(clusterQ);
					if (is_extended){
						h_time_beam_mrdcoinc_noveto_beamok_extended->Fill(clusterT);
						h_charge_beam_mrdcoinc_noveto_beamok_extended->Fill(clusterQ);
					}
				}
			}
		}
	}


	fout->Write();

	fout->Close();

}
