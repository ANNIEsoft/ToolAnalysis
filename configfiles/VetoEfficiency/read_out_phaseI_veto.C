void read_out_phaseI_veto(const char *filename, const char *outname){

	TFile *f = new TFile(filename);
	TTree *t = (TTree*) f->Get("veto_efficiency");
	
	TFile *out = new TFile(outname,"RECREATE");
	TH1F *eff_veto1_mrd = new TH1F("eff_veto1_mrd","Veto Layer 1 (MRD coinc)",13,0,13);
	TH1F *eff_veto2_mrd = new TH1F("eff_veto2_mrd","Veto Layer 2 (MRD coinc)",13,13,26);
	eff_veto1_mrd->GetXaxis()->SetTitle("chankey");
	eff_veto1_mrd->GetYaxis()->SetTitle("efficiency");
	eff_veto2_mrd->GetXaxis()->SetTitle("chankey");
	eff_veto2_mrd->GetYaxis()->SetTitle("efficiency");
	TH1F *eff_veto1_tank = new TH1F("eff_veto1_tank","Veto Layer 1 (Tank coinc)",13,0,13);
	TH1F *eff_veto2_tank = new TH1F("eff_veto2_tank","Veto Layer 2 (Tank coinc)",13,13,26);
	eff_veto1_tank->GetXaxis()->SetTitle("chankey");
	eff_veto1_tank->GetYaxis()->SetTitle("efficiency");
	eff_veto2_tank->GetXaxis()->SetTitle("chankey");
	eff_veto2_tank->GetYaxis()->SetTitle("efficiency");

	std::vector<unsigned long>* vetol1 = new std::vector<unsigned long>;
	std::vector<unsigned long>* vetol2 = new std::vector<unsigned long>;
	int num_mrdl1, num_mrdl2;
	int coincidence_layer;
	int num_vetol1, num_vetol2;
	double tank_charge;
	int num_unique_water_pmts;
	int expected_veto_l2[13]={0};
	int observed_veto_l2[13]={0};
	int expected_veto_l1[13]={0};
	int observed_veto_l1[13]={0};
	int expected_veto_tank_l2[13]={0};
	int observed_veto_tank_l2[13]={0};
	int expected_veto_tank_l1[13]={0};
	int observed_veto_tank_l1[13]={0};

	t->SetBranchAddress("veto_l1_ids",&vetol1);
	t->SetBranchAddress("veto_l2_ids",&vetol2);
	t->SetBranchAddress("num_mrdl1_hits",&num_mrdl1);
	t->SetBranchAddress("num_mrdl2_hits",&num_mrdl2);
	t->SetBranchAddress("num_veto_l1_hits",&num_vetol1);
	t->SetBranchAddress("num_veto_l2_hits",&num_vetol2);
	t->SetBranchAddress("coincidence_layer",&coincidence_layer);
	t->SetBranchAddress("tank_charge",&tank_charge);
	t->SetBranchAddress("num_unique_water_pmts",&num_unique_water_pmts);

	int nentries = t->GetEntries();
	for (int i_entry=0;i_entry<nentries; i_entry++){
		t->GetEntry(i_entry);
		if (num_mrdl1>=0 && num_mrdl2 >=0){
			if (coincidence_layer==1){
				for (int il1=0; il1 < vetol1->size(); il1++){
					int in_layer_chankey = (vetol1->at(il1)-1000000)/100;
					expected_veto_l2[in_layer_chankey]++;
					for (int il2=0; il2 < vetol2->size(); il2++){
						int in_layer_chankey_L2 = (vetol2->at(il2)-1010000)/100;
						if (in_layer_chankey != in_layer_chankey_L2) continue;
						observed_veto_l2[in_layer_chankey_L2]++;
					}
				}
			} else {
				for (int il2=0; il2 < vetol2->size(); il2++){
					int in_layer_chankey = (vetol2->at(il2)-1010000)/100;
					expected_veto_l1[in_layer_chankey]++;
					for (int il1=0; il1 < vetol1->size(); il1++){
						int in_layer_chankey_L1 = (vetol1->at(il1)-1000000)/100;
						if (in_layer_chankey != in_layer_chankey_L1) continue;
						observed_veto_l1[in_layer_chankey_L1]++;
					}
				}
			}
		}
		if (num_unique_water_pmts>=10 && tank_charge >=0.5){
			if (coincidence_layer==1){
				for (int il1=0; il1 < vetol1->size(); il1++){
					int in_layer_chankey = (vetol1->at(il1)-1000000)/100;
					expected_veto_tank_l2[in_layer_chankey]++;
					for (int il2=0; il2 < vetol2->size(); il2++){
						int in_layer_chankey_L2 = (vetol2->at(il2)-1010000)/100;
						if (in_layer_chankey != in_layer_chankey_L2) continue;
						observed_veto_tank_l2[in_layer_chankey_L2]++;
					}
				}
			} else {
				for (int il2=0; il2 < vetol2->size(); il2++){
					int in_layer_chankey = (vetol2->at(il2)-1010000)/100;
					expected_veto_tank_l1[in_layer_chankey]++;
					for (int il1=0; il1 < vetol1->size(); il1++){
						int in_layer_chankey_L1 = (vetol1->at(il1)-1000000)/100;
						if (in_layer_chankey != in_layer_chankey_L1) continue;
						observed_veto_tank_l1[in_layer_chankey_L1]++;
					}
				}
			}
		}
	}

std::cout << std::endl;
std::cout <<"--------------------------------------"<<std::endl;
std::cout <<"Summary MRD coincidences: "<<std::endl;
std::cout <<"--------------------------------------"<<std::endl;
std::cout << std::endl;

for (int i=0; i<13; i++){
	double efficiency = (expected_veto_l2[i] == 0)? 0 : (observed_veto_l2[i]/double(expected_veto_l2[i]));
	double error = (expected_veto_l2[i] == 0)? 0 : sqrt(observed_veto_l2[i]/double(expected_veto_l2[i])/expected_veto_l2[i]+observed_veto_l2[i]*observed_veto_l2[i]/double(expected_veto_l2[i])/expected_veto_l2[i]/expected_veto_l2[i]);
	std::cout <<"Efficiency Layer 2, Channel "<<i<<": "<<efficiency<<" ["<<observed_veto_l2[i]<<" / "<<expected_veto_l2[i]<<"]"<<std::endl;
	eff_veto2_mrd->SetBinContent(i+1,efficiency);
	eff_veto2_mrd->SetBinError(i+1,error);
}
for (int i=0; i<13; i++){
	double efficiency = (expected_veto_l1[i] == 0)? 0 : (observed_veto_l1[i]/double(expected_veto_l1[i]));
	double error = (expected_veto_l1[i] == 0)? 0 : sqrt(observed_veto_l1[i]/double(expected_veto_l1[i])/expected_veto_l1[i]+observed_veto_l1[i]*observed_veto_l1[i]/double(expected_veto_l1[i])/expected_veto_l1[i]/expected_veto_l1[i]);
	std::cout <<"Efficiency Layer 1, Channel "<<i<<": "<<efficiency<<" ["<<observed_veto_l1[i]<<" / "<<expected_veto_l1[i]<<"]"<<std::endl;
	eff_veto1_mrd->SetBinContent(i+1,efficiency);
	eff_veto1_mrd->SetBinError(i+1,error);
}

std::cout << std::endl;
std::cout <<"--------------------------------------"<<std::endl;
std::cout <<"Summary Tank coincidences: "<<std::endl;
std::cout <<"--------------------------------------"<<std::endl;
std::cout << std::endl;

for (int i=0; i<13; i++){
	double efficiency = (expected_veto_tank_l2[i] == 0)? 0 : (observed_veto_tank_l2[i]/double(expected_veto_tank_l2[i]));
	double error = (expected_veto_l2[i] == 0)? 0 : sqrt(observed_veto_l2[i]/double(expected_veto_l2[i])/expected_veto_l2[i]+observed_veto_l2[i]*observed_veto_l2[i]/double(expected_veto_l2[i])/expected_veto_l2[i]/expected_veto_l2[i]);
	std::cout <<"Efficiency Layer 2, Channel "<<i<<": "<<efficiency<<" ["<<observed_veto_tank_l2[i]<<" / "<<expected_veto_tank_l2[i]<<"]"<<std::endl;
	eff_veto2_tank->SetBinContent(i+1,efficiency);
	eff_veto2_tank->SetBinError(i+1,error);
}
for (int i=0; i<13; i++){
	double efficiency = (expected_veto_tank_l1[i] == 0)? 0 : (observed_veto_tank_l1[i]/double(expected_veto_tank_l1[i]));
	double error = (expected_veto_l1[i] == 0)? 0 : sqrt(observed_veto_l1[i]/double(expected_veto_l1[i])/expected_veto_l1[i]+observed_veto_l1[i]*observed_veto_l1[i]/double(expected_veto_l1[i])/expected_veto_l1[i]/expected_veto_l1[i]);
	std::cout <<"Efficiency Layer 1, Channel "<<i<<": "<<efficiency<<" ["<<observed_veto_tank_l1[i]<<" / "<<expected_veto_tank_l1[i]<<"]"<<std::endl;
	eff_veto1_tank->SetBinContent(i+1,efficiency);
	eff_veto1_tank->SetBinError(i+1,error);
}

out->cd();
eff_veto1_mrd->Write();
eff_veto2_mrd->Write();
eff_veto1_tank->Write();
eff_veto2_tank->Write();
out->Close();
delete out;
delete f;


}
