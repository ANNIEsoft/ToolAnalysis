double calculate_neutrino(double Emu, double cosT){

	double Ev = 0;
	double Mp = 938.272;
	double Mn = 939.565;
	double Eb = 26;
	double Mmu = 105.658;
	double pmu = sqrt(Emu*Emu-Mmu*Mmu);
	Ev = ((Mp*Mp-pow((Mn-Eb),2)-Mmu*Mmu+2*(Mn-Eb)*Emu)/(2*(Mn-Eb-Emu+pmu*cosT)));
	std::cout <<"Emu: "<<Emu<<", cosT: "<<cosT<<", Ev: "<<Ev<<std::endl;
	return Ev;

}

void reconstruction_migration(const char* infile, const char* outfile, bool verbose = false){

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
	double true_dirx;
	double true_diry;
	double true_dirz;
	
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
	tTrigger->SetBranchAddress("trueDirX",&true_dirx);
	tTrigger->SetBranchAddress("trueDirY",&true_diry);
	tTrigger->SetBranchAddress("trueDirZ",&true_dirz);

	TH1F *h_muon_energy = new TH1F("h_muon_energy","Muon energy distribution",100,0,2000);
	TH1F *h_muon_energy_fv = new TH1F("h_muon_energy_fv","Muon energy distribution (FV)",100,0,2000);
	TH1F *h_neutrino_energy = new TH1F("h_neutrino_energy","Neutrino energy distribution",100,0,2000);
	TH1F *h_neutrino_energy_fv = new TH1F("h_neutrino_energy_fv","Neutrino energy distribution (FV)",100,0,2000);
	TH1F *h_muon_vtx_x = new TH1F("h_muon_vtx_x","Muon vertex (x)",200,-3,3);
	TH1F *h_muon_vtx_y = new TH1F("h_muon_vtx_y","Muon vertex (y)",200,-3,3);
	TH1F *h_muon_vtx_z = new TH1F("h_muon_vtx_z","Muon vertex (z)",200,-3,3);
	TH2F *h_muon_vtx_yz = new TH2F("h_muon_vtx_yz","Muon vertex (tank) Y-Z",200,-3,3,200,-3,3);
	TH2F *h_muon_vtx_xz = new TH2F("h_muon_vtx_xz","Muon vertex (tank) X-Z",200,-2.5,2.5,200,-2.5,2.5);
	
	TH1F *h_true_muon_energy = new TH1F("h_true_muon_energy","True Muon energy distribution",100,0,2000);
	TH1F *h_true_muon_energy_fv = new TH1F("h_true_muon_energy_fv","True Muon energy distribution (FV)",100,0,2000);
	TH1F *h_true_neutrino_energy = new TH1F("h_true_neutrino_energy","True Neutrino energy distribution",100,0,2000);
	TH1F *h_true_neutrino_energy_fv = new TH1F("h_true_neutrino_energy_fv","True Neutrino energy distribution (FV)",100,0,2000);
	TH1F *h_true_muon_vtx_x = new TH1F("h_true_muon_vtx_x","True Muon vertex (x)",200,-3,3);
	TH1F *h_true_muon_vtx_y = new TH1F("h_true_muon_vtx_y","True Muon vertex (y)",200,-3,3);
	TH1F *h_true_muon_vtx_z = new TH1F("h_true_muon_vtx_z","True Muon vertex (z)",200,-3,3);
	TH2F *h_true_muon_vtx_yz = new TH2F("h_true_muon_vtx_yz","True Muon vertex (tank) Y-Z",200,-3,3,200,-3,3);
	TH2F *h_true_muon_vtx_xz = new TH2F("h_true_muon_vtx_xz","True Muon vertex (tank) X-Z",200,-2.5,2.5,200,-2.5,2.5);

	TH1F *h_diff_muon_energy = new TH1F("h_diff_muon_energy","Difference Muon energy distribution",100,-1000,1000);
	TH1F *h_diff_muon_energy_fv = new TH1F("h_diff_muon_energy_fv","Difference Muon energy distribution (FV)",100,-1000,1000);
	TH1F *h_diff_neutrino_energy = new TH1F("h_diff_neutrino_energy","Difference Neutrino energy distribution",100,-1000,1000);
	TH1F *h_diff_neutrino_energy_fv = new TH1F("h_diff_neutrino_energy_fv","Difference Neutrino energy distribution (FV)",100,-1000,1000);
	TH1F *h_diff_muon_vtx_x = new TH1F("h_diff_muon_vtx_x","Difference Muon vertex (x)",200,-3,3);
	TH1F *h_diff_muon_vtx_y = new TH1F("h_diff_muon_vtx_y","Difference Muon vertex (y)",200,-3,3);
	TH1F *h_diff_muon_vtx_z = new TH1F("h_diff_muon_vtx_z","Difference Muon vertex (z)",200,-3,3);
	TH2F *h_diff_muon_vtx_yz = new TH2F("h_diff_muon_vtx_yz","Difference Muon vertex (tank) Y-Z",200,-3,3,200,-3,3);
	TH2F *h_diff_muon_vtx_xz = new TH2F("h_diff_muon_vtx_xz","Difference Muon vertex (tank) X-Z",200,-2.5,2.5,200,-2.5,2.5);

	TH2F *h_Etank_Q = new TH2F("h_Etank_Q","Muon energy (tank) vs. charge",200,0,5000,200,-300,1000);

	TH1F *h_diff_dirz = new TH1F("h_diff_dirz","Difference muon direction (z)",100,-2,2);
	
	TH2F *h_migration_energy = new TH2F("h_migration_energy","Migration matrix muon energy",8,400,1200,8,400,1200);
	TH2F *h_migration_angle = new TH2F("h_migration_angle","Migration matrix cos(#theta)",6,0.7,1.,6,0.7,1.);

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

        TTree *tout = new TTree("neutron_tree","Neutron tree");
        tout->Branch("TankMRDCoinc",&tankMRDCoinc);
        tout->Branch("nPrimNeut",&nPrimNeut);
        tout->Branch("nPrimProt",&nPrimProt);
        tout->Branch("nCaptures",&nCaptures);
        tout->Branch("nCapturesPMTVol",&nCapturesPMTVol);
        tout->Branch("nCandidates",&nCandidates);
        tout->Branch("Emu",&Emu);
        tout->Branch("Enu",&Enu);
        tout->Branch("Q2",&Q2);
        tout->Branch("vtxX",&vtxX);
        tout->Branch("vtxY",&vtxY);
        tout->Branch("vtxZ",&vtxZ);
        tout->Branch("Clusters",&clusters);
        tout->Branch("ClusterCB",&clusterCB);
        tout->Branch("ClusterTime",&clusterTime);
        tout->Branch("ClusterPE",&clusterPE);
        tout->Branch("nCandCB",&nCandCB);
        tout->Branch("nCandTime",&nCandTime);
        tout->Branch("nCandPE",&nCandPE);
        tout->Branch("mrdTrackLength",&mrdTrackLength);
        tout->Branch("mrdEnergyLoss",&mrdEnergyLoss);
        tout->Branch("neutVtxX",&neutVtxX);
        tout->Branch("neutVtxY",&neutVtxY);
        tout->Branch("neutVtxZ",&neutVtxZ);
        tout->Branch("neutCapNucl",&neutCapNucl);
        tout->Branch("neutCapTime",&neutCapTime);
        tout->Branch("neutCapETotal",&neutCapETotal);
        tout->Branch("neutCapNGamma",&neutCapNGamma);
        tout->Branch("isCC",&isCC);
        tout->Branch("isQEL",&isQEL);
        tout->Branch("isDIS",&isDIS);
        tout->Branch("isRES",&isRES);
        tout->Branch("isMEC",&isMEC);
        tout->Branch("isCOH",&isCOH);
        tout->Branch("multiRing",&multiRing);
        tout->Branch("primaryPdgs",&primaryPdgs);

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

	TTree *tout_reco = new TTree("tout_reco","Reco tree");
	tout_reco->Branch("ChargeTank",&ChargeTank);
	tout_reco->Branch("TrueVtxX",&TrueVtxX);
	tout_reco->Branch("TrueVtxY",&TrueVtxY);
	tout_reco->Branch("TrueVtxZ",&TrueVtxZ);
	tout_reco->Branch("TrueMuonE",&TrueMuonE);
	tout_reco->Branch("TrueNeutrinoE",&TrueNeutrinoE);
	tout_reco->Branch("TrueFV",&TrueFV);
	
	tout_reco->Branch("RecoMuonE",&RecoMuonE);
	tout_reco->Branch("RecoNeutrinoE",&RecoNeutrinoE);
	tout_reco->Branch("RecoVtxX",&RecoVtxX);
	tout_reco->Branch("RecoVtxY",&RecoVtxY);
	tout_reco->Branch("RecoVtxZ",&RecoVtxZ);
	tout_reco->Branch("FV",&FV);
	
	tout_reco->Branch("DWater",&DWater);
	tout_reco->Branch("DAir",&DAir);
	tout_reco->Branch("DMRD",&DMRD);
	tout_reco->Branch("RecoMuonEMRD",&RecoMuonEMRD);
	tout_reco->Branch("RecoTheta",&RecoTheta);
	tout_reco->Branch("RecoMuonEWater",&RecoMuonEWater);
	tout_reco->Branch("MRDStartX",&MRDStartX);
	tout_reco->Branch("MRDStartY",&MRDStartY);
	tout_reco->Branch("MRDStartZ",&MRDStartZ);
	tout_reco->Branch("MRDStopX",&MRDStopX);
	tout_reco->Branch("MRDStopY",&MRDStopY);
	tout_reco->Branch("MRDStopZ",&MRDStopZ);
	tout_reco->Branch("TankExitX",&TankExitX);
	tout_reco->Branch("TankExitY",&TankExitY);
	tout_reco->Branch("TankExitZ",&TankExitZ);
	tout_reco->Branch("PMTVolExitX",&PMTVolExitX);
	tout_reco->Branch("PMTVolExitY",&PMTVolExitY);
	tout_reco->Branch("PMTVolExitZ",&PMTVolExitZ);
	tout_reco->Branch("DirX",&DirX);
	tout_reco->Branch("DirY",&DirY);
	tout_reco->Branch("DirZ",&DirZ);

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
					//i_pmt--;	//Data case
					break;		//MC case
				}
			}
			else if (run_pmt < run_trigger) i_pmt++;
			else if (i_pmt > 0) break;
		}
		if (found_coinc){
		if (is_extended && num_mrd_tracks == 1) {
			if (mrdstop->at(0)){
				std::cout <<"cuts passed!"<<std::endl;

				//Energy reconstruction
				double muon_eloss = eloss->at(0);
				mrdTrackLength = tracklength->at(0);
				//double reco_muon_energy = muon_eloss + max_pe/12;	//Data version
				double reco_muon_energy = muon_eloss + max_pe*0.08534;	//Data version
				double true_muon_energy = true_muone-105.66;
				
				//Direction information (from MRD)
				double startx = mrdstartx->at(0);
				double starty = mrdstarty->at(0);
				double startz = mrdstartz->at(0);
				double stopx = mrdstopx->at(0);
				double stopy = mrdstopy->at(0);
				double stopz = mrdstopz->at(0);
				double diffx = stopx-startx;
				double diffy = stopy-starty;
				double diffz = stopz-startz;
				double startz_c = startz-1.681;
				bool hit_tank = false;
				double a = pow(diffx,2)+pow(diffz,2);
				double b = -2*diffx*startx-2*diffz*startz_c;
				double c = pow(startx,2)+pow(startz_c,2)-1.0*1.0;
				double t1 = (-b+sqrt(b*b-4*a*c))/(2*a);
				double t2 = (-b-sqrt(b*b-4*a*c))/(2*a);
				double t = 0;
				if (t1 < 0) t = t2;
				else if (t2 < 0) t = t1;
				else t = (t1 < t2)? t1 : t2;
				double exitx = startx - t*diffx;
				double exity = starty - t*diffy;
				double exitz = startz - t*diffz;
				double dirx = diffx/(sqrt(diffx*diffx+diffy*diffy+diffz*diffz));
				double diry = diffy/(sqrt(diffx*diffx+diffy*diffy+diffz*diffz));
				double dirz = diffz/(sqrt(diffx*diffx+diffy*diffy+diffz*diffz));

				double dist_mrd = sqrt(pow(startx-stopx,2)+pow(starty-stopy,2)+pow(startz-stopz,2));
				double dist_air = sqrt(pow(startx-exitx,2)+pow(starty-exity,2)+pow(startz-exitz,2));
				double dist_air_x = sqrt(pow(startx-exitx,2));
				double dist_air_y = sqrt(pow(starty-exity,2));
				double dist_air_z = sqrt(pow(startz-exitz,2));

				double ap = pow(diffx,2)+pow(diffz,2);
                                double bp = -2*diffx*startx-2*diffz*startz_c;
                                double cp = pow(startx,2)+pow(startz_c,2)-1.5*1.5;
                                double t1p = (-bp+sqrt(bp*bp-4*ap*cp))/(2*ap);
                                double t2p = (-bp-sqrt(bp*bp-4*ap*cp))/(2*ap);
                                std::cout <<"t1, t2: "<<t1p<<","<<t2p<<std::endl;
                                double tp = 0;
                                if (t1p < 0) tp = t2p;
                                else if (t2p < 0) tp = t1p;
                                else tp = (t1p < t2p)? t1p : t2p;
                                std::cout <<"t: "<<tp<<std::endl;

				double exitxp = startx - tp*diffx;
				double exityp = starty - tp*diffy;
				double exitzp = startz - tp*diffz;

				double diff_exit_x = exitxp - exitx;
				double diff_exit_y = exityp - exity;
				double diff_exit_z = exitzp - exitz;
				double diff_pmtvol_tank = sqrt(pow(diff_exit_x,2)+pow(diff_exit_y,2)+pow(diff_exit_z,2));
				std::cout <<"old energy: "<<reco_muon_energy<<std::endl;
				if (!std::isnan(diff_pmtvol_tank)){
					reco_muon_energy += 200*(diff_pmtvol_tank);
				}
				std::cout <<"new energy: "<<reco_muon_energy<<std::endl;
				//Add offset (potentially from slight bias in MRD energy estimate)
				reco_muon_energy += 87.3;

				//Neutrino energy reconstruction (with direction from MRD)
				double reco_neutrino_energy = calculate_neutrino(reco_muon_energy,diffz);
				
				//Vertex reconstruction
				//double reco_vtxx = exitx - max_pe/12/2./100.*dirx; 
				//double reco_vtxy = exity - max_pe/12/2./100.*diry; 
				//double reco_vtxz = exitz - max_pe/12/2./100.*dirz;
				double reco_vtxx = exitx - max_pe/9/2./100.*dirx; 	//9 MeV/p.e. seems to yield the most unbiased results for the z-reconstruction
				double reco_vtxy = exity - max_pe/9/2./100.*diry; 
				double reco_vtxz = exitz - max_pe/9/2./100.*dirz;
				double truevtxx = true_vtxx/100.;
				double truevtxy = true_vtxy/100.-0.1445;
				double truevtxz = true_vtxz/100.+1.681;

				//Fill tree variables
				vtxX = truevtxx;
				vtxY = truevtxy;
				vtxZ = truevtxz;
				mrdEnergyLoss = muon_eloss;
				Emu = true_muon_energy;
				Enu = true_neutrino_energy;
				tankMRDCoinc = tankmrdcoinc;
				nPrimNeut = true_neutrons;
				isMEC = true_mec;
				isDIS = true_dis;
				isRES = true_res;
				isQEL = true_qel;
				isCC = true_cc;
				Q2 = -true_q2;	//For some reason Q2 in Genie is saved with negative sign
				nPrimProt = true_protons;
				multiRing = trueMultiRing;
				nCaptures = true_neut_cap_vtxx->size();
				nCandidates = n_neutrons;
				nCapturesPMTVol = 0;
				for (int i_neut= 0; i_neut < (int) true_neut_cap_vtxx->size(); i_neut++){
					neutVtxX->push_back(true_neut_cap_vtxx->at(i_neut));
					neutVtxY->push_back(true_neut_cap_vtxy->at(i_neut));
					neutVtxZ->push_back(true_neut_cap_vtxz->at(i_neut));
					neutCapNucl->push_back(true_neut_cap_nucl->at(i_neut));
					neutCapTime->push_back(true_neut_cap_time->at(i_neut));
					neutCapETotal->push_back(true_neut_cap_e->at(i_neut));
					neutCapNGamma->push_back(true_neut_cap_gammas->at(i_neut));
					//PMT volume cut
					double zcorr = true_neut_cap_vtxz->at(i_neut)/100.-1.681;
					double ycorr = true_neut_cap_vtxy->at(i_neut)/100.+0.144;
					double xcorr = true_neut_cap_vtxx->at(i_neut)/100.;
					double rad = sqrt(xcorr*xcorr+zcorr*zcorr);
					if (rad < 1.0 && fabs(ycorr)<1.5) nCapturesPMTVol++;
				}
				for (int i_part = 0; i_part < (int) true_primary_pdgs->size(); i_part++){
					primaryPdgs->push_back(true_primary_pdgs->at(i_part));
				}
				tout->Fill();

				if (tankmrdcoinc){

					//Fill reconstruction parameters to tree
					ChargeTank = max_pe;
					TrueVtxX = truevtxx;
					TrueVtxY = truevtxy;
					TrueVtxZ = truevtxz;
					TrueMuonE = true_muon_energy;
					TrueNeutrinoE = true_neutrino_energy*1000;
					
					RecoMuonE = reco_muon_energy;
					RecoNeutrinoE = reco_neutrino_energy;
					RecoVtxX = reco_vtxx;
					RecoVtxY = reco_vtxy;
					RecoVtxZ = reco_vtxz;
					DWater = diff_pmtvol_tank;
					DAir = dist_air;
					DMRD = dist_mrd;				
					RecoMuonEMRD = muon_eloss;
					RecoTheta = diffz;	//this is actually cos(theta), not theta
					RecoMuonEWater = true_muon_energy - muon_eloss - 200.*DWater;
					MRDStartX = startx;
					MRDStartY = starty;
					MRDStartZ = startz;
					MRDStopX = stopx;
					MRDStopY = stopy;
					MRDStopZ = stopz;
					TankExitX = exitx;
					TankExitY = exity;
					TankExitZ = exitz;
					PMTVolExitX = exitxp;
					PMTVolExitY = exityp;
					PMTVolExitZ = exitzp;
					DirX = dirx;
					DirY = diry;
					DirZ = dirz;

					h_Etank_Q->Fill(ChargeTank,RecoMuonEWater);

					//Fill reconstruction-specific histograms

					h_neutrino_energy->Fill(reco_neutrino_energy);
					h_true_neutrino_energy->Fill(1000.*true_neutrino_energy);
					h_diff_neutrino_energy->Fill(reco_neutrino_energy-1000.*true_neutrino_energy);

					h_muon_energy->Fill(reco_muon_energy);
					h_true_muon_energy->Fill(true_muon_energy);
					h_diff_muon_energy->Fill(reco_muon_energy-true_muon_energy);

					h_diff_dirz->Fill(dirz - true_dirz);
					h_migration_angle->Fill(true_dirz,dirz);
					h_migration_energy->Fill(true_muon_energy,reco_muon_energy);

					h_muon_vtx_x->Fill(reco_vtxx);
					h_muon_vtx_y->Fill(reco_vtxy);
					h_muon_vtx_z->Fill(reco_vtxz);
					h_true_muon_vtx_x->Fill(truevtxx);	
					h_true_muon_vtx_y->Fill(truevtxy);	
					h_true_muon_vtx_z->Fill(truevtxz);	
					h_diff_muon_vtx_x->Fill(reco_vtxx-truevtxx);					
					h_diff_muon_vtx_y->Fill(reco_vtxy-truevtxy);					
					h_diff_muon_vtx_z->Fill(reco_vtxz-truevtxz);					

					h_muon_vtx_yz->Fill(reco_vtxz-1.681,reco_vtxy+0.144);
					h_muon_vtx_xz->Fill(reco_vtxz-1.681,reco_vtxx);
					h_true_muon_vtx_yz->Fill(truevtxz-1.681,truevtxy+0.144);
					h_true_muon_vtx_xz->Fill(truevtxz-1.681,truevtxx);
					h_diff_muon_vtx_yz->Fill(reco_vtxz-truevtxz,reco_vtxy-truevtxy);
					h_diff_muon_vtx_xz->Fill(reco_vtxz-truevtxz,reco_vtxx-truevtxx);

					if (sqrt(reco_vtxx*reco_vtxx+(reco_vtxz-1.681)*(reco_vtxz-1.681))<1.0 && fabs(reco_vtxy+0.144)<0.5 && ((reco_vtxz-1.681) < 0.)) {
						h_muon_energy_fv->Fill(reco_muon_energy);
						h_neutrino_energy_fv->Fill(reco_neutrino_energy);
						FV = 1;
					}
					if (sqrt(truevtxx*truevtxx+(truevtxz-1.681)*(truevtxz-1.681))<1.0 && fabs(truevtxy+0.144)<0.5 && ((truevtxz-1.681) < 0.)) {
						h_true_muon_energy_fv->Fill(true_muon_energy);
						h_true_neutrino_energy_fv->Fill(1000.*true_neutrino_energy);
						TrueFV = 1;
					}
					tout_reco->Fill();
				}
			}
			}
		}
	}


	fout->Write();
	fout->Close();

}
