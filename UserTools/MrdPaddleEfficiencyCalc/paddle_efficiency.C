// Small macro to calculate efficiency values for the MRD paddle histograms
// Steps
// 1) Get the efficiency by dividing the number of observed hits in a paddle by the number of expected hits in a paddle
// 2) Calculate the uncertainty accordingly
// 3) Fill the result in histograms and save them
// 4) Save histograms in a ROOT-file
// Macro is going to be implemented as a separate tool in the ToolAnalysis framework later

void paddle_efficiency(){


	int num_slots = 24;
	int num_channels = 32;

	TFile *f = new TFile("R1415_R1459_mrdobserved.root","READ");
	TFile *out_file = new TFile("R1415_R1459_mrdefficiency.root","RECREATE");
	TH1D *eff_chankey = new TH1D("eff_chankey","Efficiency vs. channelkey",310,26,336);
	TH2Poly *eff_top = new TH2Poly("eff_top","Efficiency - Top View",1.6,3.,-2,2.);
	TH2Poly *eff_side = new TH2Poly("eff_side","Efficiency - Side View",1.6,3.,-2.,2.);
	TH2D *eff_crate1 = new TH2D("eff_crate1","Efficiency - Rack 7",num_slots,0,num_slots,num_channels,0,num_channels);
	TH2D *eff_crate2 = new TH2D("eff_crate2","Efficiency - Rack 8",num_slots,0,num_slots,num_channels,0,num_channels);
	std::vector<TBox*> vector_box_inactive;
	eff_chankey->GetXaxis()->SetTitle("chankey");
	eff_chankey->GetYaxis()->SetTitle("efficiency #epsilon");	
	eff_chankey->SetStats(0);
	eff_top->GetXaxis()->SetTitle("z [m]");
	eff_top->GetYaxis()->SetTitle("x [m]");
	eff_side->GetXaxis()->SetTitle("z [m]");
	eff_side->GetYaxis()->SetTitle("y [m]");
	eff_top->GetZaxis()->SetTitle("#epsilon");
	eff_side->GetZaxis()->SetTitle("#epsilon");
	eff_top->SetStats(0);
	eff_side->SetStats(0);
	eff_top->GetZaxis()->SetRangeUser(0.000,1);
	eff_side->GetZaxis()->SetRangeUser(0.000,1);
	eff_crate1->SetStats(0);
	eff_crate1->GetXaxis()->SetNdivisions(num_slots);
	eff_crate1->GetYaxis()->SetNdivisions(num_channels);
	for (int i_label=0;i_label<int(num_channels);i_label++){
		std::stringstream ss_slot, ss_ch;
		ss_slot<<(i_label+1);
		ss_ch<<(i_label);
		std::string str_ch = "ch "+ss_ch.str();
		eff_crate1->GetYaxis()->SetBinLabel(i_label+1,str_ch.c_str());
		if (i_label<num_slots){
			std::string str_slot = "slot "+ss_slot.str();
			eff_crate1->GetXaxis()->SetBinLabel(i_label+1,str_slot.c_str());
		}
	}
	eff_crate1->LabelsOption("v");
	eff_crate2->SetStats(0);
	eff_crate2->GetXaxis()->SetNdivisions(num_slots);
	eff_crate2->GetYaxis()->SetNdivisions(num_channels);
	for (int i_label=0;i_label<int(num_channels);i_label++){
		std::stringstream ss_slot, ss_ch;
		ss_slot<<(i_label+1);
		ss_ch<<(i_label);
		std::string str_ch = "ch "+ss_ch.str();
		eff_crate2->GetYaxis()->SetBinLabel(i_label+1,str_ch.c_str());
		if (i_label<num_slots){
			std::string str_slot = "slot "+ss_slot.str();
			eff_crate2->GetXaxis()->SetBinLabel(i_label+1,str_slot.c_str());
    		}
  	}
	eff_crate2->LabelsOption("v");	
	TCanvas *canvas_elec = new TCanvas("canvas_elec","Canvas Electronics Space",900,600);

	int chankey_offset = 26;
	int channels_per_layer[11] = {26,30,26,34,26,26,26,30,26,30,26};
	int channels_start[11] = {26,52,82,108,142,168,194,220,250,276,306};
	int bins_start[11] = {1,27,57,83,117,143,169,195,225,251,281};
	double extents[11] = {1.318,1.146,1.318,1.299,1.318,1.318,1.318,1.521,1.318,1.521,1.318};
	double zmin[11] = {1.6798,1.8009,1.929,2.0501,2.1782,2.2993,2.4204,2.5415,2.6626,2.7837,2.9048};
	double zmax[11] = {1.6858,1.8139,1.935,2.0631,2.1842,2.3053,2.4264,2.5475,2.6686,2.7897,2.9108};

	double dummy;
	int temp_channel, temp_orientation, temp_half, temp_side, temp_elec_crate, temp_elec_slot, temp_elec_ch;
	double temp_xmin, temp_xmax, temp_ymin, temp_ymax, temp_zmin, temp_zmax;
	std::map<int,int> map_ch_orientation, map_ch_half, map_ch_side;
	std::map<int,double> map_ch_xmin, map_ch_xmax, map_ch_ymin, map_ch_ymax, map_ch_zmin, map_ch_zmax, map_ch_Crate, map_ch_Slot, map_ch_Channel;

	std::vector<int> active_slots_cr1, active_slots_cr2;
	std::vector<int> inactive_ch_crate1, inactive_slot_crate1, inactive_ch_crate2, inactive_slot_crate2;
	int active_channel[2][24]={{0}};

	ifstream geometry_file("mrdgeometry_noheader.dat");
	while(!geometry_file.eof()){
		geometry_file >> temp_channel >> temp_orientation >> temp_half >> temp_side >> temp_xmin >> temp_xmax >> temp_ymin >> temp_ymax >> temp_zmin >> temp_zmax >> temp_elec_crate >> temp_elec_slot >> temp_elec_ch; 
		if (geometry_file.eof()) break;
		std::cout <<"Reading in channel "<<temp_channel<<std::endl;
		map_ch_orientation.emplace(temp_channel,temp_orientation);
		map_ch_half.emplace(temp_channel,temp_half);
		map_ch_side.emplace(temp_channel,temp_side);
		map_ch_xmin.emplace(temp_channel,temp_xmin);
		map_ch_xmax.emplace(temp_channel,temp_xmax);
		map_ch_ymin.emplace(temp_channel,temp_ymin);
		map_ch_ymax.emplace(temp_channel,temp_ymax);
		map_ch_zmin.emplace(temp_channel,temp_zmin);
		map_ch_zmax.emplace(temp_channel,temp_zmax);
		map_ch_Crate.emplace(temp_channel,temp_elec_crate);
		map_ch_Slot.emplace(temp_channel,temp_elec_slot);
		map_ch_Channel.emplace(temp_channel,temp_elec_ch);
		if (temp_elec_crate == 7) active_slots_cr1.push_back(temp_elec_slot);
		else active_slots_cr2.push_back(temp_elec_slot);
		active_channel[temp_elec_crate-7][temp_elec_slot-1]=1;
	}
	geometry_file.close();

	ifstream inactive_file("inactive_channels.dat");
	while(!inactive_file.eof()){
		inactive_file >> temp_elec_crate >> temp_elec_slot >> temp_elec_ch;
		if (inactive_file.eof()) break;
		if (temp_elec_crate == 7){
			inactive_ch_crate1.push_back(temp_elec_ch);
			inactive_slot_crate1.push_back(temp_elec_slot);
		} else {
			inactive_ch_crate2.push_back(temp_elec_ch);
			inactive_slot_crate2.push_back(temp_elec_slot);
		}
	}
	inactive_file.close();

	double enlargeBoxes = 0.01;
	double shiftSecRow = 0.03;

	for (int i_layer = 0; i_layer < 11; i_layer++){

		for (int i_ch = 0; i_ch < channels_per_layer[i_layer]; i_ch++){

			if (i_layer%2 == 0) {
				if (i_ch < channels_per_layer[i_layer]/2.){
					eff_side->AddBin(zmin[i_layer]-enlargeBoxes,map_ch_ymin[channels_start[i_layer]+i_ch],zmax[i_layer]+enlargeBoxes,map_ch_ymax[channels_start[i_layer]+i_ch]);
				}else {
					eff_side->AddBin(zmin[i_layer]-enlargeBoxes+shiftSecRow,map_ch_ymin[channels_start[i_layer]+i_ch],zmax[i_layer]+shiftSecRow+enlargeBoxes,map_ch_ymax[channels_start[i_layer]+i_ch]);
				}	
			} else {
				if (i_ch < channels_per_layer[i_layer]/2.){
					eff_top->AddBin(zmin[i_layer]-enlargeBoxes,map_ch_xmin[channels_start[i_layer]+i_ch],zmax[i_layer]+enlargeBoxes,map_ch_xmax[channels_start[i_layer]+i_ch]);
				} else {
					eff_top->AddBin(zmin[i_layer]-enlargeBoxes+shiftSecRow,map_ch_xmin[channels_start[i_layer]+i_ch],zmax[i_layer]+shiftSecRow+enlargeBoxes,map_ch_xmax[channels_start[i_layer]+i_ch]);
				}
			
			}
		}
	}


	std::string str_expected_hist = "expectedhits_layer";
	std::string str_observed_hist = "observedhits_layer";
	std::string str_eff_hist = "efficiency_layer";
	std::string str_chkey = "_chkey";
	std::string str_avg = "_avg";
	std::string str_title_eff_hist = "Efficiency Layer ";
	std::string str_title_chkey = " Chankey ";

	//Loop over MRD layers & create the histograms
	for (int i_layer = 0; i_layer < 11; i_layer++){

		//If first or last layer, don't calculate efficiencies
		if (i_layer == 0 || i_layer == 10) continue;

		std::stringstream name_layer_hist1, name_layer_hist2, title_layer_hist1, title_layer_hist2;
		name_layer_hist1 << "efficiency_layer"<<i_layer<<"_1";
		name_layer_hist2 << "efficiency_layer"<<i_layer<<"_2";
		title_layer_hist1 << "Efficiency MRD Layer "<<i_layer<<", Channels "<<channels_start[i_layer]<<"-"<<channels_start[i_layer]+channels_per_layer[i_layer]/2-1;
		title_layer_hist2 << "Efficiency MRD Layer "<<i_layer<<", Channels "<<channels_start[i_layer]+channels_per_layer[i_layer]/2<<"-"<<channels_start[i_layer]+channels_per_layer[i_layer]-1;
		TH1D *layer_hist1 = new TH1D(name_layer_hist1.str().c_str(),title_layer_hist1.str().c_str(),channels_per_layer[i_layer]/2,-extents[i_layer],extents[i_layer]);
		TH1D *layer_hist2 = new TH1D(name_layer_hist2.str().c_str(),title_layer_hist2.str().c_str(),channels_per_layer[i_layer]/2,-extents[i_layer],extents[i_layer]);
		if (i_layer%2 == 0) {
			layer_hist1->GetXaxis()->SetTitle("y [m]");
			layer_hist2->GetXaxis()->SetTitle("y [m]");
		} else {
			layer_hist1->GetXaxis()->SetTitle("x [m]");
			layer_hist2->GetXaxis()->SetTitle("x [m]");
		}
		layer_hist1->GetYaxis()->SetTitle("Efficiency #epsilon");
		layer_hist2->GetYaxis()->SetTitle("Efficiency #epsilon");
		layer_hist1->SetStats(0);
		layer_hist2->SetStats(0);
		layer_hist1->GetYaxis()->SetRangeUser(-0.05,1.1);
		layer_hist2->GetYaxis()->SetRangeUser(-0.01,1.1);


		//Loop over channels within that layer
		for (int i_ch = 0; i_ch < channels_per_layer[i_layer]; i_ch++){
			std::stringstream name_obs_hist, name_exp_hist, name_eff_hist, name_eff_hist_avg, title_eff_hist;
			
			name_obs_hist << str_observed_hist << i_layer << str_chkey << channels_start[i_layer] + i_ch;
			name_exp_hist << str_expected_hist << i_layer << str_chkey << channels_start[i_layer] + i_ch;
			name_eff_hist << str_eff_hist << i_layer << str_chkey << channels_start[i_layer] + i_ch;
			name_eff_hist_avg << str_eff_hist << i_layer << str_chkey << channels_start[i_layer] + i_ch + str_avg;
			title_eff_hist << str_title_eff_hist << i_layer << str_title_chkey << channels_start[i_layer] + i_ch;
			
			//Get observed/expected histograms
			TH1D *temp_exp = (TH1D*) f->Get(name_exp_hist.str().c_str());
			TH1D *temp_obs = (TH1D*) f->Get(name_obs_hist.str().c_str());

			//Scale down from 50 bins to 10
			temp_exp->Rebin(5);
			temp_obs->Rebin(5);

			//Define and calculate efficiency histogram with 10 bins
			TH1D *efficiency_hist = (TH1D*) temp_obs->Clone(name_eff_hist.str().c_str());
			efficiency_hist->SetStats(0);
			efficiency_hist->SetTitle(title_eff_hist.str().c_str());
			if (i_layer%2==0) efficiency_hist->GetXaxis()->SetTitle("y [m]");
			else efficiency_hist->GetXaxis()->SetTitle("x [m]");
			efficiency_hist->GetYaxis()->SetTitle("efficiency #epsilon");
			efficiency_hist->Divide(temp_exp);

			//Scale down from 10 bins to 1
			temp_exp->Rebin(10);
			temp_obs->Rebin(10);
			
			//Define and calculate efficiency histogram with 1 bin
			TH1D *efficiency_hist_avg = (TH1D*) temp_obs->Clone(name_eff_hist_avg.str().c_str());
			efficiency_hist_avg->SetStats(0);
			efficiency_hist_avg->SetTitle(title_eff_hist.str().c_str());
			efficiency_hist_avg->Divide(temp_exp);

			double efficiency = efficiency_hist_avg->GetBinContent(1);
			double exp = temp_exp->GetBinContent(1);
			double obs = temp_exp->GetBinContent(1);
			double err_eff = 1./exp * sqrt(obs + obs*obs/exp);
			eff_chankey->SetBinContent(bins_start[i_layer]+i_ch,efficiency);
			eff_chankey->SetBinError(bins_start[i_layer]+i_ch,err_eff);

			if (i_ch < channels_per_layer[i_layer]/2){
				layer_hist1->SetBinContent(i_ch+1,efficiency);
				layer_hist1->SetBinError(i_ch+1,err_eff);
			} else {
				layer_hist2->SetBinContent((i_ch-channels_per_layer[i_layer]/2)+1,efficiency);
				layer_hist2->SetBinError((i_ch-channels_per_layer[i_layer]/2)+1,err_eff);
			}

			double xcenter = 0.5*(map_ch_xmin[channels_start[i_layer]+i_ch]+map_ch_xmax[channels_start[i_layer]+i_ch]);
			double ycenter = 0.5*(map_ch_ymin[channels_start[i_layer]+i_ch]+map_ch_ymax[channels_start[i_layer]+i_ch]);
			double zcenter = 0.5*(map_ch_zmin[channels_start[i_layer]+i_ch]+map_ch_zmax[channels_start[i_layer]+i_ch]);
			if (map_ch_half[channels_start[i_layer]+i_ch]==1) zcenter+=shiftSecRow;

			if (map_ch_orientation[channels_start[i_layer]+i_ch]==0) eff_side->Fill(zcenter,ycenter,efficiency);
			else eff_top->Fill(zcenter,xcenter,efficiency);


			if (map_ch_Crate[channels_start[i_layer]+i_ch]==7) eff_crate1->SetBinContent(map_ch_Slot[channels_start[i_layer]+i_ch],map_ch_Channel[channels_start[i_layer]+i_ch]+1,efficiency);
			else eff_crate2->SetBinContent(map_ch_Slot[channels_start[i_layer]+i_ch],map_ch_Channel[channels_start[i_layer]+i_ch]+1,efficiency);

			//Write efficiency files to the file
			out_file->cd();
			efficiency_hist->Write();
			efficiency_hist_avg->Write();
		}
		
		out_file->cd();
		layer_hist1->Write();
		layer_hist2->Write();
	}

out_file->cd();
eff_chankey->Write();
eff_top->Write();
eff_side->Write();

canvas_elec->cd();
canvas_elec->Divide(2,1);
TPad *p1 = (TPad*) canvas_elec->cd(1);
p1->SetGrid();
eff_crate1->Draw("colz");
int inactive_crate1 = 0;
int inactive_crate2 = 0;
for (int i_slot=0;i_slot<num_slots;i_slot++){
	if (active_channel[0][i_slot]==0){
		TBox *box_inactive = new TBox(i_slot,0,i_slot+1,num_channels);
		vector_box_inactive.push_back(box_inactive);
		box_inactive->SetFillStyle(3004);
		box_inactive->SetFillColor(1);
		inactive_crate1++;
	}
}
for (int i_ch = 0; i_ch < inactive_ch_crate1.size(); i_ch++){
	std::cout <<"Inactive box (crate 1): Slot "<<inactive_slot_crate1.at(i_ch)<<", Channel "<<inactive_ch_crate1.at(i_ch)<<std::endl;
	TBox *box_inactive = new TBox(inactive_slot_crate1.at(i_ch)-1,inactive_ch_crate1.at(i_ch),inactive_slot_crate1.at(i_ch),inactive_ch_crate1.at(i_ch)+1);
	vector_box_inactive.push_back(box_inactive);
	box_inactive->SetFillStyle(3004);
	box_inactive->SetFillColor(1);
	inactive_crate1++;
}

for (int i_slot=0;i_slot<num_slots;i_slot++){
	if (active_channel[1][i_slot]==0){
		TBox *box_inactive = new TBox(i_slot,0,i_slot+1,num_channels);
		vector_box_inactive.push_back(box_inactive);
		box_inactive->SetFillColor(1);
		box_inactive->SetFillStyle(3004);
		inactive_crate2++;
	}
}
for (int i_ch = 0; i_ch < inactive_ch_crate2.size(); i_ch++){
	std::cout <<"Inactive box (crate 2): Slot "<<inactive_slot_crate2.at(i_ch)<<", Channel "<<inactive_ch_crate2.at(i_ch)<<std::endl;
	TBox *box_inactive = new TBox(inactive_slot_crate2.at(i_ch)-1,inactive_ch_crate2.at(i_ch),inactive_slot_crate2.at(i_ch),inactive_ch_crate2.at(i_ch)+1);
	vector_box_inactive.push_back(box_inactive);
	box_inactive->SetFillStyle(3004);
	box_inactive->SetFillColor(1);
	inactive_crate2++;
}
for (int i_ch=0; i_ch < inactive_crate1; i_ch++){
	vector_box_inactive.at(i_ch)->Draw("same");
}
p1->Update();
TPad *p2 = (TPad*) canvas_elec->cd(2);
p2->SetGrid();
eff_crate2->Draw("colz");
for (int i_ch = 0; i_ch < inactive_crate2; i_ch++){
	vector_box_inactive.at(inactive_crate1+i_ch)->Draw("same");
}
p2->Update();
canvas_elec->Write();

out_file->Close();
delete out_file;





}
