void calculate_fmv_efficiency(std::string inputfile, std::string outputfile, bool has_tank){

  TFile *f_out = new TFile(outputfile.c_str(),"RECREATE");

  //Define output histograms
  TEfficiency *fmv_eff_layer1 = new TEfficiency("fmv_eff_layer1","FMV Efficiencies Layer 1;chankey;efficiency #varepsilon",13,0,13);
  TEfficiency *fmv_eff_strict_layer1 = new TEfficiency("fmv_eff_strict_layer1","FMV Efficiencies Layer 1 (Strict MRD Track);chankey;efficiency #varepsilon",13,0,13);
  TEfficiency *fmv_eff_strict_avg_layer1 = new TEfficiency("fmv_eff_strict_avg_layer1","FMV Average Efficiencies Layer 1 (Strict MRD Track);chankey;efficiency #varepsilon",13,0,13);
  TEfficiency *fmv_eff_loose_layer1 = new TEfficiency("fmv_eff_loose_layer1","FMV Efficiencies Layer 1 (MRD Track);chankey; efficiency #varepsilon",13,0,13);
  TEfficiency *fmv_eff_loose_avg_layer1 = new TEfficiency("fmv_eff_loose_avg_layer1","FMV Average Efficiencies Layer 1 (MRD Track);chankey; efficiency #varepsilon",13,0,13);
  TEfficiency *fmv_eff_layer2 = new TEfficiency("fmv_eff_layer2","FMV Efficiencies Layer 2; chankey; efficiency #varepsilon",13,13,26);
  TEfficiency *fmv_eff_strict_layer2 = new TEfficiency("fmv_eff_strict_layer2","FMV Efficiencies Layer 2 (Strict MRD Track); chankey; efficiency #varepsilon",13,13,26);
  TEfficiency *fmv_eff_strict_avg_layer2 = new TEfficiency("fmv_eff_strict_avg_layer2","FMV Average Efficiencies Layer 2 (Strict MRD Track); chankey; efficiency #varepsilon",13,13,26);
  TEfficiency *fmv_eff_loose_layer2 = new TEfficiency("fmv_eff_loose_layer2","FMV Efficiencies Layer 2 (MRD Track); chankey; efficiency #varepsilon",13,13,26);
  TEfficiency *fmv_eff_loose_avg_layer2 = new TEfficiency("fmv_eff_loose_avg_layer2","FMV Average Efficiencies Layer 2 (MRD Track); chankey; efficiency #varepsilon",13,13,26);
  TEfficiency *fmv_eff_tank_layer1 = nullptr;
  TEfficiency *fmv_eff_tank_layer2 = nullptr;
  if (has_tank){
    fmv_eff_tank_layer1 = new TEfficiency("fmv_eff_tank_layer1","FMV Efficiencies (Tank coincidence) Layer 1;chankey; efficiency #varepsilon",13,0,13);
    fmv_eff_tank_layer2 = new TEfficiency("fmv_eff_tank_layer2","FMV Efficiencies (Tank coincidence) Layer 2; chankey; efficiency #varepsilon",13,13,26);
  }

  std::vector<TEfficiency*> eff_hists_strict, eff_hists_loose;

  TFile *f = new TFile(inputfile.c_str(),"READ");

  TH1D *obs_layer1 = (TH1D*) f->Get("fmv_observed_layer1");
  TH1D *exp_layer1 = (TH1D*) f->Get("fmv_expected_layer1");
  TH1D *obs_layer2 = (TH1D*) f->Get("fmv_observed_layer2");
  TH1D *exp_layer2 = (TH1D*) f->Get("fmv_expected_layer2");
  TH1D *obs_track_strict_layer1 = (TH1D*) f->Get("fmv_observed_track_strict_layer1");
  TH1D *exp_track_strict_layer1 = (TH1D*) f->Get("fmv_expected_track_strict_layer1");
  TH1D *obs_track_strict_layer2 = (TH1D*) f->Get("fmv_observed_track_strict_layer2");
  TH1D *exp_track_strict_layer2 = (TH1D*) f->Get("fmv_expected_track_strict_layer2");
  TH1D *obs_track_loose_layer1 = (TH1D*) f->Get("fmv_observed_track_loose_layer1");
  TH1D *exp_track_loose_layer1 = (TH1D*) f->Get("fmv_expected_track_loose_layer1");
  TH1D *obs_track_loose_layer2 = (TH1D*) f->Get("fmv_observed_track_loose_layer2");
  TH1D *exp_track_loose_layer2 = (TH1D*) f->Get("fmv_expected_track_loose_layer2");
 
  TH1D *obs_tank_layer1 = nullptr;
  TH1D *exp_tank_layer1 = nullptr;
  TH1D *obs_tank_layer2 = nullptr;
  TH1D *exp_tank_layer2 = nullptr;
 
  if (has_tank){
    obs_tank_layer1 = (TH1D*) f->Get("fmv_tank_observed_layer1");
    exp_tank_layer1 = (TH1D*) f->Get("fmv_tank_expected_layer1");
    obs_tank_layer2 = (TH1D*) f->Get("fmv_tank_observed_layer2");
    exp_tank_layer2 = (TH1D*) f->Get("fmv_tank_expected_layer2");
  }

  //Layer 1 efficiencies
  for (int i=0; i< 13; i++){
    double obs_counts = obs_layer1->GetBinContent(i+1);
    double exp_counts = exp_layer1->GetBinContent(i+1);
    fmv_eff_layer1->SetTotalEvents(i+1,exp_counts);
    fmv_eff_layer1->SetPassedEvents(i+1,obs_counts);
    double obs_counts_strict = obs_track_strict_layer1->GetBinContent(i+1);
    double exp_counts_strict = exp_track_strict_layer1->GetBinContent(i+1);
    fmv_eff_strict_layer1->SetTotalEvents(i+1,exp_counts_strict);
    fmv_eff_strict_layer1->SetPassedEvents(i+1,obs_counts_strict);
    double obs_counts_loose = obs_track_loose_layer1->GetBinContent(i+1);
    double exp_counts_loose = exp_track_loose_layer1->GetBinContent(i+1);
    fmv_eff_loose_layer1->SetTotalEvents(i+1,exp_counts_loose);
    fmv_eff_loose_layer1->SetPassedEvents(i+1,obs_counts_loose);
    if (has_tank){
      double obs_counts_tank = obs_tank_layer1->GetBinContent(i+1);
      double exp_counts_tank = exp_tank_layer1->GetBinContent(i+1);
      //if (exp_counts_tank == 0) exp_counts_tank = 1;
      fmv_eff_tank_layer1->SetTotalEvents(i+1,exp_counts_tank);
      fmv_eff_tank_layer1->SetPassedEvents(i+1,obs_counts_tank);
    }
  }
  
  //Layer 2 efficiencies
  for (int i=0; i< 13; i++){
    double obs_counts = obs_layer2->GetBinContent(i+1);
    double exp_counts = exp_layer2->GetBinContent(i+1);
    fmv_eff_layer2->SetTotalEvents(i+1,exp_counts);
    fmv_eff_layer2->SetPassedEvents(i+1,obs_counts);
    double obs_counts_strict = obs_track_strict_layer2->GetBinContent(i+1);
    double exp_counts_strict = exp_track_strict_layer2->GetBinContent(i+1);
    fmv_eff_strict_layer2->SetTotalEvents(i+1,exp_counts_strict);
    fmv_eff_strict_layer2->SetPassedEvents(i+1,obs_counts_strict);
    double obs_counts_loose = obs_track_loose_layer2->GetBinContent(i+1);
    double exp_counts_loose = exp_track_loose_layer2->GetBinContent(i+1);
    fmv_eff_loose_layer2->SetTotalEvents(i+1,exp_counts_loose);
    fmv_eff_loose_layer2->SetPassedEvents(i+1,obs_counts_loose);
    if (has_tank){
      double obs_counts_tank = obs_tank_layer2->GetBinContent(i+1);
      double exp_counts_tank = exp_tank_layer2->GetBinContent(i+1);
      fmv_eff_tank_layer2->SetTotalEvents(i+1,exp_counts_tank);
      fmv_eff_tank_layer2->SetPassedEvents(i+1,obs_counts_tank);
    }
  }

  //Channel-per-channel efficiencies
  for (int i=0; i< 26; i++){

    //Get observed/expected histograms
    std::stringstream hist_obs_strict_name, hist_exp_strict_name, hist_obs_loose_name, hist_exp_loose_name, hist_eff_strict_name, hist_eff_strict_title, hist_eff_loose_name, hist_eff_loose_title;
    hist_obs_strict_name <<"hist_observed_strict_chankey"<<i;
    hist_exp_strict_name <<"hist_expected_strict_chankey"<<i;
    hist_obs_loose_name <<"hist_observed_loose_chankey"<<i;
    hist_exp_loose_name <<"hist_expected_loose_chankey"<<i;
    hist_eff_strict_name <<"hist_efficiency_strict_chankey"<<i;
    hist_eff_strict_title << "FMV Efficiency (Strict MRD track) Chankey "<<i<<";x [m]; efficiency #varepsilon";
    hist_eff_loose_name << "hist_efficiency_loose_chankey"<<i;
    hist_eff_loose_title << "FMV Efficiency (MRD track) Chankey "<<i<<";x [m]; efficiency #varepsilon";
    TH1F* hist_obs_strict_temp = (TH1F*) f->Get(hist_obs_strict_name.str().c_str());
    TH1F* hist_exp_strict_temp = (TH1F*) f->Get(hist_exp_strict_name.str().c_str());
    TH1F* hist_obs_loose_temp = (TH1F*) f->Get(hist_obs_loose_name.str().c_str());
    TH1F* hist_exp_loose_temp = (TH1F*) f->Get(hist_exp_loose_name.str().c_str());

    //Create efficiency histograms
    f_out->cd();
    TEfficiency* hist_eff_strict_temp = new TEfficiency(hist_eff_strict_name.str().c_str(),hist_eff_strict_title.str().c_str(),hist_obs_strict_temp->GetXaxis()->GetNbins()/2,hist_obs_strict_temp->GetXaxis()->GetXmin(),hist_obs_strict_temp->GetXaxis()->GetXmax());
    TEfficiency* hist_eff_loose_temp = new TEfficiency(hist_eff_loose_name.str().c_str(),hist_eff_loose_title.str().c_str(),hist_obs_loose_temp->GetXaxis()->GetNbins()/2,hist_obs_loose_temp->GetXaxis()->GetXmin(),hist_obs_loose_temp->GetXaxis()->GetXmax());
   
    TH1F *hist_obs_strict_rebin = (TH1F*) hist_obs_strict_temp->Rebin(2);
    TH1F *hist_exp_strict_rebin = (TH1F*) hist_exp_strict_temp->Rebin(2);
    TH1F *hist_obs_loose_rebin = (TH1F*) hist_obs_loose_temp->Rebin(2);
    TH1F *hist_exp_loose_rebin = (TH1F*) hist_exp_loose_temp->Rebin(2);

    double avg_efficiency_strict=0;
    double error_avg_efficiency_strict=0;
    double avg_efficiency_loose=0;
    double error_avg_efficiency_loose=0;
    int tot_exp_strict=0;
    int tot_obs_strict=0;
    int tot_exp_loose=0;
    int tot_obs_loose=0;

    int non_zero_bins_strict=0;
    int non_zero_bins_loose=0;
    for (int bin=0; bin<hist_obs_strict_rebin->GetXaxis()->GetNbins(); bin++){
      double obs_counts = hist_obs_strict_rebin->GetBinContent(bin+1);
      double exp_counts = hist_exp_strict_rebin->GetBinContent(bin+1);
      hist_eff_strict_temp->SetTotalEvents(bin+1,exp_counts);
      hist_eff_strict_temp->SetPassedEvents(bin+1,obs_counts);
      double obs_counts_loose = hist_obs_loose_rebin->GetBinContent(bin+1);
      double exp_counts_loose = hist_exp_loose_rebin->GetBinContent(bin+1);
      hist_eff_loose_temp->SetTotalEvents(bin+1,exp_counts_loose);
      hist_eff_loose_temp->SetPassedEvents(bin+1,obs_counts_loose);
      if (exp_counts >0){
        tot_exp_strict+=exp_counts;
        tot_obs_strict+=obs_counts;
        avg_efficiency_strict+=(obs_counts/exp_counts);
      }
      if (exp_counts_loose > 0){
        tot_exp_loose+=exp_counts_loose;
        tot_obs_loose+=obs_counts_loose;
        avg_efficiency_loose+=(obs_counts_loose/exp_counts_loose);
      }
    }
    eff_hists_strict.push_back(hist_eff_strict_temp);
    eff_hists_loose.push_back(hist_eff_loose_temp);

    avg_efficiency_strict/=non_zero_bins_strict;
    avg_efficiency_loose/=non_zero_bins_loose;
    error_avg_efficiency_strict=sqrt(error_avg_efficiency_strict);
    error_avg_efficiency_strict/=non_zero_bins_strict;
    error_avg_efficiency_loose=sqrt(error_avg_efficiency_loose);
    error_avg_efficiency_loose/=non_zero_bins_loose;
    if (i < 13) {
      fmv_eff_strict_avg_layer1->SetTotalEvents(i+1,tot_exp_strict);
      fmv_eff_strict_avg_layer1->SetPassedEvents(i+1,tot_obs_strict);
      fmv_eff_loose_avg_layer1->SetTotalEvents(i+1,tot_exp_loose);
      fmv_eff_loose_avg_layer1->SetPassedEvents(i+1,tot_obs_loose);
    }
    else {
      fmv_eff_strict_avg_layer2->SetTotalEvents(i-13+1,tot_exp_strict);
      fmv_eff_strict_avg_layer2->SetPassedEvents(i-13+1,tot_obs_strict);
      fmv_eff_loose_avg_layer2->SetTotalEvents(i-13+1,tot_exp_loose);
      fmv_eff_loose_avg_layer2->SetPassedEvents(i-13+1,tot_obs_loose);
    }
  }

  f_out->cd();
  fmv_eff_layer1->Write();
  fmv_eff_strict_layer1->Write();
  fmv_eff_strict_avg_layer1->Write();
  fmv_eff_loose_layer1->Write();
  fmv_eff_loose_avg_layer1->Write();
  fmv_eff_layer2->Write();
  fmv_eff_strict_layer2->Write();
  fmv_eff_strict_avg_layer2->Write();
  fmv_eff_loose_layer2->Write();
  fmv_eff_loose_avg_layer2->Write();
  if (has_tank){
    fmv_eff_tank_layer1->Write();
    fmv_eff_tank_layer2->Write();
  }

  for (int i=0; i< int(eff_hists_strict.size()); i++){
    eff_hists_strict.at(i)->Write();
    eff_hists_loose.at(i)->Write();
  }

  f_out->Close();
  f->Close();

  delete f_out;
  delete f;

}
