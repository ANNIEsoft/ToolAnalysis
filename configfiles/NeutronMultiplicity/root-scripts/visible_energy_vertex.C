void visible_energy_vertex(const char* infile, const char* outfile, bool IsMC = true, bool verbose = false){

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

	int beam_ok;
	int num_mrd_tracks;
        int has_tank;
	std::vector<double>* eloss = new std::vector<double>;
	std::vector<double>* tracklength = new std::vector<double>;
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

	int tankmrdcoinc;
	int noveto;
	double true_muone;
	int true_pdg;
	double true_vtxx;
	double true_vtxy;
	double true_vtxz;
	
	int true_neutrons;
	int true_protons;
	int true_mec;
	int true_dis;
	int true_res;
	int true_qel;
	int true_cc;
	double true_q2;
	double true_neutrino_energy;
	std::vector<double> *true_neut_cap_e = new std::vector<double>;
	std::vector<double> *true_neut_cap_time = new std::vector<double>;
	std::vector<double> *true_neut_cap_nucl = new std::vector<double>;
	std::vector<double> *true_neut_cap_vtxx = new std::vector<double>;
	std::vector<double> *true_neut_cap_vtxy = new std::vector<double>;
	std::vector<double> *true_neut_cap_vtxz = new std::vector<double>;
	std::vector<double> *true_neut_cap_gammas = new std::vector<double>;
	std::vector<int> *true_primary_pdgs = new std::vector<int>;
	int trueMultiRing;

	tTrigger->SetBranchAddress("runNumber",&run_trigger);
	tMRD->SetBranchAddress("runNumber",&run_mrd);
	tPMT->SetBranchAddress("runNumber",&run_pmt);
	tTrigger->SetBranchAddress("eventNumber",&ev_trigger);
	tMRD->SetBranchAddress("eventNumber",&ev_mrd);
	tPMT->SetBranchAddress("eventNumber",&ev_pmt);
	tTrigger->SetBranchAddress("beam_ok",&beam_ok);
	tTrigger->SetBranchAddress("numMRDTracks",&num_mrd_tracks);
	tTrigger->SetBranchAddress("MRDTrackLength",&tracklength);
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
	tTrigger->SetBranchAddress("TankMRDCoinc",&tankmrdcoinc);
	tTrigger->SetBranchAddress("NoVeto",&noveto);
	tTrigger->SetBranchAddress("trueMuonEnergy",&true_muone);
	tTrigger->SetBranchAddress("truePrimaryPdg",&true_pdg);
	tTrigger->SetBranchAddress("trueVtxX",&true_vtxx);
	tTrigger->SetBranchAddress("trueVtxY",&true_vtxy);
	tTrigger->SetBranchAddress("trueVtxZ",&true_vtxz);
	tTrigger->SetBranchAddress("truePrimaryPdgs",&true_primary_pdgs);
	tTrigger->SetBranchAddress("trueNeutCapVtxX",&true_neut_cap_vtxx);
	tTrigger->SetBranchAddress("trueNeutCapVtxY",&true_neut_cap_vtxy);
	tTrigger->SetBranchAddress("trueNeutCapVtxZ",&true_neut_cap_vtxz);
	tTrigger->SetBranchAddress("trueNeutCapNucleus",&true_neut_cap_nucl);
	tTrigger->SetBranchAddress("trueNeutCapTime",&true_neut_cap_time);
	tTrigger->SetBranchAddress("trueNeutCapGammas",&true_neut_cap_gammas);
	tTrigger->SetBranchAddress("trueNeutCapE",&true_neut_cap_e);
	tTrigger->SetBranchAddress("trueNeutrinoEnergy",&true_neutrino_energy);
	tTrigger->SetBranchAddress("trueQ2",&true_q2);
	tTrigger->SetBranchAddress("trueCC",&true_cc);
	tTrigger->SetBranchAddress("trueQEL",&true_qel);
	tTrigger->SetBranchAddress("trueRES",&true_res);
	tTrigger->SetBranchAddress("trueDIS",&true_dis);
	tTrigger->SetBranchAddress("trueMEC",&true_mec);
	tTrigger->SetBranchAddress("trueNeutrons",&true_neutrons);
	tTrigger->SetBranchAddress("trueProtons",&true_protons);
	tTrigger->SetBranchAddress("trueMultiRing",&trueMultiRing);

	TH2F *h_visible_rho = new TH2F("h_visible_rho","Visible energy vs. #rho",200,0,4,400,0,4000);
	TH2F *h_visible_zrho = new TH2F("h_visible_zrho","Visible energy vs. |z| #rho",400,-4,4,400,0,4000);
	TH2F *h_visible_y = new TH2F("h_visible_y","Visible energy vs. y",200,-2,2,400,0,4000);
	TH2F *h_visible_y_zrho_0_100 = new TH2F("h_visible_y_zrho_0_100","Vertex y vs |z| #rho (0-100p.e.)",200,0,2,200,-2,2);
	TH2F *h_visible_y_zrho_100_200 = new TH2F("h_visible_y_zrho_100_200","Vertex y vs |z| #rho (100-200p.e.)",200,0,2,200,-2,2);
	TH2F *h_visible_y_zrho_200_300 = new TH2F("h_visible_y_zrho_200_300","Vertex y vs |z| #rho (200-300p.e.)",200,0,2,200,-2,2);
	TH2F *h_visible_y_zrho_300_400 = new TH2F("h_visible_y_zrho_300_400","Vertex y vs |z| #rho (300-400p.e.)",200,0,2,200,-2,2);
	TH2F *h_visible_y_zrho_400_500 = new TH2F("h_visible_y_zrho_400_500","Vertex y vs |z| #rho (400-500p.e.)",200,0,2,200,-2,2);
	TH2F *h_visible_y_zrho_500_600 = new TH2F("h_visible_y_zrho_500_600","Vertex y vs |z| #rho (500-600p.e.)",200,0,2,200,-2,2);
	TH2F *h_visible_y_zrho_600_700 = new TH2F("h_visible_y_zrho_600_700","Vertex y vs |z| #rho (600-700p.e.)",200,0,2,200,-2,2);
	TH2F *h_visible_y_zrho_700_800 = new TH2F("h_visible_y_zrho_700_800","Vertex y vs |z| #rho (700-800p.e.)",200,0,2,200,-2,2);
	TH2F *h_visible_y_zrho_800_900 = new TH2F("h_visible_y_zrho_800_900","Vertex y vs |z| #rho (800-900p.e.)",200,0,2,200,-2,2);
	TH2F *h_visible_y_zrho_900_1000 = new TH2F("h_visible_y_zrho_900_1000","Vertex y vs |z| #rho (900-1000p.e.)",200,0,2,200,-2,2);
	TH2F *h_visible_y_zrho_1000_1500 = new TH2F("h_visible_y_zrho_1000_1500","Vertex y vs |z| #rho (1000-1500p.e.)",200,0,2,200,-2,2);
	TH2F *h_visible_y_zrho_1500_2000 = new TH2F("h_visible_y_zrho_1500_2000","Vertex y vs |z| #rho (1500-2000p.e.)",200,0,2,200,-2,2);
	TH2F *h_visible_y_zrho_2000_2500 = new TH2F("h_visible_y_zrho_2000_2500","Vertex y vs |z| #rho (2000-2500p.e.)",200,0,2,200,-2,2);
	TH2F *h_visible_y_zrho_2500_3000 = new TH2F("h_visible_y_zrho_2500_3000","Vertex y vs |z| #rho (2500-3000p.e.)",200,0,2,200,-2,2);
	TH2F *h_visible_y_zrho_3000_3500 = new TH2F("h_visible_y_zrho_3000_3500","Vertex y vs |z| #rho (3000-3500p.e.)",200,0,2,200,-2,2);
	TH2F *h_visible_y_zrho_3500_4000 = new TH2F("h_visible_y_zrho_3500_4000","Vertex y vs |z| #rho (3500-4000p.e.)",200,0,2,200,-2,2);
	TH2F *h_visible_y_zrho_4000_4500 = new TH2F("h_visible_y_zrho_4000_4500","Vertex y vs |z| #rho (4000-4500p.e.)",200,0,2,200,-2,2);
	TH2F *h_visible_y_zrho_4500_5000 = new TH2F("h_visible_y_zrho_4500_5000","Vertex y vs |z| #rho (4500-5000p.e.)",200,0,2,200,-2,2);
	TH2F *h_visible_y_zrho_5000_plus = new TH2F("h_visible_y_zrho_5000_plus","Vertex y vs |z| #rho (> 5000p.e.)",200,0,2,200,-2,2);

	TH2F *h_Etank_Q = new TH2F("h_Etank_Q","Muon energy (tank) vs. charge",200,0,5000,200,-300,1000);

	int nPrimNeut;
	int nPrimProt;
	int nCaptures;
	int nCapturesPMTVol;
	double Emu;
	double Enu;
	double Q2;
	double vtxX;
	double vtxY;
	double vtxZ;
	std::vector<double> *clusterCB = new std::vector<double>;
	std::vector<double> *clusterTime = new std::vector<double>;
	std::vector<double> *clusterPE = new std::vector<double>;
	std::vector<double> *nCandCB = new std::vector<double>;
	std::vector<double> *nCandTime = new std::vector<double>;
	std::vector<double> *nCandPE = new std::vector<double>;
	double mrdTrackLength;
	double mrdEnergyLoss;
	std::vector<double> *neutVtxX = new std::vector<double>;
	std::vector<double> *neutVtxY = new std::vector<double>;
	std::vector<double> *neutVtxZ = new std::vector<double>;
	std::vector<double> *neutCapNucl = new std::vector<double>;
	std::vector<double> *neutCapTime = new std::vector<double>;
	std::vector<double> *neutCapETotal = new std::vector<double>;
	std::vector<double> *neutCapNGamma = new std::vector<double>;
	int isCC;
	int isQEL;
	int isDIS;
	int isRES;
	int isCOH;
	int isMEC;
	int tankMRDCoinc;
	int multiRing;
	int clusters;
	int nCandidates;
	std::vector<int> *primaryPdgs = new std::vector<int>;
	
	double ChargeTank;
	double TrueVtxX;
	double TrueVtxY;
	double TrueVtxZ;
	double TrueMuonE;
	double TrueNeutrinoE;
	int TrueFV;
	int FV;
	double RecoMuonE;
	double RecoNeutrinoE;
	double RecoVtxX;
	double RecoVtxY;
	double RecoVtxZ;
	double DWater;
	double DAir;
	double DMRD;
	double RecoMuonEMRD;
	double RecoTheta;
	double RecoMuonEWater;
	double MRDStartX;
	double MRDStartY;
	double MRDStartZ;
	double MRDStopX;
	double MRDStopY;
	double MRDStopZ;
	double TankExitX;
	double TankExitY;
	double TankExitZ;
	double PMTVolExitX;
	double PMTVolExitY;
	double PMTVolExitZ;
	double DirX;
	double DirY;
	double DirZ;

	int i_pmt=0;
	for (int i_trig=0; i_trig < nentries_trigger; i_trig++){
		std::cout <<"Load trig entry "<<i_trig<<"/"<<nentries_trigger<<std::endl;
		tTrigger->GetEntry(i_trig);
		if (!noveto) continue;
		if (fabs(true_pdg) != 13) continue;
		bool found_coinc=false;
		bool is_extended = false;
		int n_neutrons=0;
		double max_pe = 0;
		std::vector<double> n_times;
		Emu = 0.;
		Enu = 0.;
		Q2 = 0.;
		nPrimNeut = 0;
		nPrimProt = 0;
		nCaptures = 0;
		nCapturesPMTVol = 0;
		mrdTrackLength = 0;
		mrdEnergyLoss = 0;
		clusterCB->clear();
		clusterTime->clear();
		clusterPE->clear();
		nCandCB->clear();
		nCandTime->clear();
		nCandPE->clear();
		primaryPdgs->clear();
		neutVtxX->clear();
		neutVtxY->clear();
		neutVtxZ->clear();
		neutCapNucl->clear();
		neutCapETotal->clear();
		neutCapNGamma->clear();
		isCC = 0;
		isQEL = 0;
		isDIS = 0;
		isRES = 0;
		isMEC = 0;
		isCOH = 0;
		multiRing = 0;
		clusters = 0;
		nCandidates = 0;
		

		ChargeTank = -999;
        	TrueVtxX = -999;
        	TrueVtxY = -999;
        	TrueVtxZ = -999;
       		TrueMuonE = -999;
        	TrueNeutrinoE = -999;
        	TrueFV = 0;
        	FV = 0;
        	RecoMuonE = -999;
        	RecoNeutrinoE = -999;
        	RecoVtxX = -999;
        	RecoVtxY = -999;
        	RecoVtxZ = -999;
        	DWater = -999;
        	DAir = -999;
        	DMRD = -999;
        	RecoMuonEMRD = -999;
        	RecoTheta = -999;
        	RecoMuonEWater = -999;
        	MRDStartX = -999;
        	MRDStartY = -999;
        	MRDStartZ = -999;
        	MRDStopX = -999;
        	MRDStopY = -999;
		MRDStopZ = -999;
        	TankExitX = -999;
        	TankExitY = -999;
        	TankExitZ = -999;
        	PMTVolExitX = -999;
        	PMTVolExitY = -999;
        	PMTVolExitZ = -999;
		DirX = -999;
		DirY = -999;
		DirZ = -999;

		while (i_pmt < nentries_pmt){
			tPMT->GetEntry(i_pmt);
			if (verbose){
				std::cout <<"i_pmt: "<<i_pmt<<"run (PMT): "<<run_pmt<<", run (Trigger): "<<run_trigger<<std::endl;
				std::cout <<"i_pmt: "<<i_pmt<<"event (PMT): "<<ev_pmt<<", event (Trigger): "<<ev_trigger<<std::endl;
			}
			if (run_trigger == run_pmt){
				if (ev_trigger == ev_pmt){
					if (cluster_time < 2000 && cluster_pe > max_pe) max_pe = cluster_pe;
					if (cluster_time > 10000 && extended > 0 && beam_ok == 1 && cluster_pe < 120 && cluster_cb < 0.4){
						n_times.push_back(cluster_time);
						n_neutrons++;
						nCandCB->push_back(cluster_cb);
						nCandTime->push_back(cluster_time);
						nCandPE->push_back(cluster_pe);
					}
					i_pmt++;
					found_coinc = true;
					if (extended) is_extended = true;
					clusters++;
					clusterCB->push_back(cluster_cb);
					clusterTime->push_back(cluster_time);
					clusterPE->push_back(cluster_pe);
					
				} else if (found_coinc) break;
				else if (ev_pmt < ev_trigger) i_pmt++;
				else if (ev_pmt > ev_trigger) {
					if (IsMC) break;		//MC case
					else i_pmt--;	//Data case
				}
			}
			else if (run_pmt < run_trigger) i_pmt++;
			else if (i_pmt > 0) break;
		}
		if (found_coinc){
				
				double truevtxx = true_vtxx/100.;
				double truevtxy = true_vtxy/100.;
				double truevtxz = true_vtxz/100.;

				double rho = sqrt(truevtxx*truevtxx+truevtxz*truevtxz);
				double zrho = rho;
				if (truevtxz < 0) zrho *= -1;

				//Fill histograms
				h_visible_rho->Fill(rho,max_pe);
				h_visible_zrho->Fill(zrho,max_pe);
				h_visible_y->Fill(truevtxy,max_pe);
				if (max_pe > 5000) h_visible_y_zrho_5000_plus->Fill(zrho,truevtxy);
				else if (max_pe > 4500) h_visible_y_zrho_4500_5000->Fill(zrho,truevtxy);
				else if (max_pe > 4000) h_visible_y_zrho_4000_4500->Fill(zrho,truevtxy);
				else if (max_pe > 3500) h_visible_y_zrho_3500_4000->Fill(zrho,truevtxy);
				else if (max_pe > 3000) h_visible_y_zrho_3000_3500->Fill(zrho,truevtxy);
				else if (max_pe > 2500) h_visible_y_zrho_2500_3000->Fill(zrho,truevtxy);
				else if (max_pe > 2000) h_visible_y_zrho_2000_2500->Fill(zrho,truevtxy);
				else if (max_pe > 1500) h_visible_y_zrho_1500_2000->Fill(zrho,truevtxy);
				else if (max_pe > 1000) h_visible_y_zrho_1000_1500->Fill(zrho,truevtxy);
				else if (max_pe > 900) h_visible_y_zrho_900_1000->Fill(zrho,truevtxy);
				else if (max_pe > 800) h_visible_y_zrho_800_900->Fill(zrho,truevtxy);
				else if (max_pe > 700) h_visible_y_zrho_700_800->Fill(zrho,truevtxy);
				else if (max_pe > 600) h_visible_y_zrho_600_700->Fill(zrho,truevtxy);
				else if (max_pe > 500) h_visible_y_zrho_500_600->Fill(zrho,truevtxy);
				else if (max_pe > 400) h_visible_y_zrho_400_500->Fill(zrho,truevtxy);
				else if (max_pe > 300) h_visible_y_zrho_300_400->Fill(zrho,truevtxy);
				else if (max_pe > 200) h_visible_y_zrho_200_300->Fill(zrho,truevtxy);
				else if (max_pe > 100) h_visible_y_zrho_100_200->Fill(zrho,truevtxy);
				else if (max_pe > 0) h_visible_y_zrho_0_100->Fill(zrho,truevtxy);
		}
	}


	fout->Write();
	fout->Close();

}
