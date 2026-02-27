#include "PlotsTrackLengthAndEnergy.h"
#include "TCanvas.h"
#include "TMath.h"
#include "TH2F.h"
#include "TLegend.h"
#include "TAxis.h"
#include "TLine.h"

PlotsTrackLengthAndEnergy::PlotsTrackLengthAndEnergy():Tool(){}


bool PlotsTrackLengthAndEnergy::Initialise(std::string configfile, DataModel &data){

  /////////////////// Useful header ///////////////////////
  if(configfile!="") m_variables.Initialise(configfile); // loading config file
  //m_variables.Print();

  m_data= &data; //assigning transient data pointer
  /////////////////////////////////////////////////////////////////
  
  return true;
}


bool PlotsTrackLengthAndEnergy::Execute(){

    BoostStore EnergyReco(true,2);

    EnergyReco.Initialise("EnergyRecoStore.bs");

    unsigned long n_entries;
    bool get_ok = EnergyReco.Header->Get("TotalEntries", n_entries);
    if(not get_ok) {
        Log("PlotsTrackLengthAndEnergy Tool: EnergyRecoStore file does not exist, run the EnergyRecoPredict toolchain first!",v_error,verbosity);
        return false;
    }
    std::cout<<"got total entries; "<<get_ok<<", n_entries: "<<n_entries<<std::endl;

    TCanvas c1("c1","c1",1280,1024);
    TCanvas c2("c2","c2",1280,1024);
    TCanvas c3("c3","c3",1280,1024);
    TCanvas c4("c4","c4",1280,1024);
    TCanvas c5("c5","c5",1280,1024);
    
    TH2D lengthhist("True_RecoLength", "; MC Track Length [cm]; Reconstructed Track Length [cm]", 50, 0, 400., 50, 0., 400.);
    TH2D energyhist("True_Reco_Energy", ";  E_{MC} [MeV]; E_{reco} [MeV]", 100, 0, 2000., 100, 0., 2000.);
    TH1D lengthresol1("wDNNRecolength", "Length Resolution", 80, 0, 400);
    TH1D lengthresol2("wlambda_max", "Length Resolution", 80, 0, 400);
    TH1D energyresol1("MC Energy", "Energy Resolution", 100, 0, 2000);
    TH1D energyresol2("BDT Energy", "Energy Resolution", 100, 0, 2000);
    TH1D deltaenergy("Relative Error", "Energy Relative Error %;#DeltaE/E (%)", 100, 0, 0); 

    for(int i=0; i<n_entries; i++){
      double DNNRecoLength, trueMuonEnergy, BDTMuonEnergy, lambda_max, deltaE;
      float TrueTrackLengthInWater;
      
      EnergyReco.GetEntry(i);
      
      EnergyReco.Get("TrueTrackLengthInWater",TrueTrackLengthInWater);
      EnergyReco.Get("DNNRecoLength",DNNRecoLength);
      EnergyReco.Get("trueMuonEnergy",trueMuonEnergy);
      EnergyReco.Get("BDTMuonEnergy",BDTMuonEnergy);
      EnergyReco.Get("lambda_max",lambda_max);

      deltaE = 100*(trueMuonEnergy-BDTMuonEnergy)/trueMuonEnergy;
      
      lengthhist.Fill(TrueTrackLengthInWater,DNNRecoLength);
      energyhist.Fill(trueMuonEnergy,BDTMuonEnergy);
      lengthresol1.Fill(TMath::Abs(DNNRecoLength-TrueTrackLengthInWater));
      lengthresol2.Fill(TMath::Abs(lambda_max-TrueTrackLengthInWater));
      energyresol1.Fill(trueMuonEnergy);
      energyresol2.Fill(BDTMuonEnergy);
      deltaenergy.Fill(deltaE);
    }
    
    c1.cd();
    TLine line(0,0,400,400);
    line.SetLineColor(2);
    lengthhist.SetStats(0);
    lengthhist.Draw("ColZ");
    line.Draw("Same");
    c1.Draw();
    c1.SaveAs("MC_recolength.png");
    
    c2.cd();
    TLine line1(0,0, 2000,2000);
    line1.SetLineColor(2);
    energyhist.SetStats(0);
    energyhist.Draw("ColZ");
    line1.Draw("Same");
    c2.Draw();
    c2.SaveAs("MC_recoE.png");
    
    c3.cd();
    energyresol1.Draw();
    energyresol1.SetStats(0);
    energyresol2.SetLineColor(kRed);
    energyresol2.SetStats(0);
    energyresol2.Draw("Same");
    TLegend legend(0.7,0.7,0.9,0.9);
    legend.AddEntry(&energyresol1,"MC Energy [MeV]","l");
    legend.AddEntry(&energyresol2,"Reco Energy [MeV]","l");
    legend.Draw();
    c3.SaveAs("resol_energy.png");
    
    c4.cd();
    lengthresol1.Draw();
    lengthresol1.SetStats(0);
    lengthresol2.SetLineColor(kRed);
    lengthresol2.SetStats(0);
    lengthresol2.Draw("Same");
    TLegend legend1(0.3,0.7,0.9,0.9);
    legend1.AddEntry(&lengthresol1,"#DeltaR = |L_{Reco}-L_{MC}| [cm]","l");
    legend1.AddEntry(&lengthresol2,"#DeltaR = |lambda_{max}-L_{MC}| [cm]","l");
    legend1.AddEntry((TObject*)0, TString::Format("mean = %.2f, std = %.2f, Prev: mean = %.2f, std = %.2f ", lengthresol1.GetMean(),lengthresol1.GetStdDev(),lengthresol2.GetMean(),lengthresol2.GetStdDev()), "");
    legend1.Draw("Same");
    c4.SaveAs("resol_length.png");

    double meanDeltaE = deltaenergy.GetMean();
    std::stringstream meandeltaE;
    meandeltaE << std::fixed << std::setprecision(2) << meanDeltaE;
    std::string mean = meandeltaE.str();
    double stdDeltaE = deltaenergy.GetStdDev();
    std::stringstream stddeltaE;
    stddeltaE << std::fixed << std::setprecision(2) << stdDeltaE;
    std::string stddev = stddeltaE.str();
    std::string str = "Energy Relative Deviation % | mean =" + mean + ", std =" + stddev + ";#DeltaE/E (%)";
    const char *title = str.c_str();

    c5.cd();
    deltaenergy.Draw();
    deltaenergy.SetStats(0);
    deltaenergy.SetTitle(title);
    deltaenergy.SetTitleSize(0.5,"t");
    TLegend legend2(0.7,0.7,0.9,0.9);
    legend2.AddEntry(&deltaenergy, "#DeltaE/E=(E_{MC}-E_{Reco})/E_{True}","l");
    legend2.Draw("Same");
    deltaenergy.SetFillColorAlpha(kBlue-4, 0.35);
    c5.SaveAs("deltaenergy.png");
    
  return true;
}


bool PlotsTrackLengthAndEnergy::Finalise(){
  
  return true;
}
