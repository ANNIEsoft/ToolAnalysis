#define AnaTree_cxx
#include "AnaTree.h"
#include <TH2.h>
#include <TStyle.h>
#include <TCanvas.h>

void AnaTree::Loop()
{
//   In a ROOT session, you can do:
//      root> .L AnaTree.C
//      root> AnaTree t
//      root> t.GetEntry(12); // Fill t data members with entry number 12
//      root> t.Show();       // Show values of entry 12
//      root> t.Show(16);     // Read and show values of entry 16
//      root> t.Loop();       // Loop on all entries
//

//     This is the loop skeleton where:
//    jentry is the global entry number in the chain
//    ientry is the entry number in the current Tree
//  Note that the argument to GetEntry must be:
//    jentry for TChain::GetEntry
//    ientry for TTree::GetEntry and TBranch::GetEntry
//
//       To read only selected branches, Insert statements like:
// METHOD1:
//    fChain->SetBranchStatus("*",0);  // disable all branches
//    fChain->SetBranchStatus("branchname",1);  // activate branchname
// METHOD2: replace line
//    fChain->GetEntry(jentry);       //read all branches
//by  b_branchname->GetEntry(ientry); //read only this branch
   cout<<"I'm HERE!!!"<<endl;
   if (fChain == 0) return;

   cout<<"now here"<<endl;
   TH1D* TimeHistTDT = new TH1D("arrivaltimeTDT","arrivaltimeTDT",200,15.,25.);
   TH1D* TimeHistdT = new TH1D("arrivaltimedt","arrivaltimedt",200,15.,25.);
   TH1D* TimeHistdtTDT = new TH1D("arrivaltimedtTDT","arrivaltimedtTDT",200,15.,25.);
   TH1D* deltaT = new TH1D("deltaT","deltaT",200,-3,3);

   TH1D* UnTimeHist = new TH1D("unarrivaltime","unarrivaltime",200,15.,25.);


   Long64_t nentries = fChain->GetEntriesFast();

   Long64_t nbytes = 0, nb = 0;
   for (Long64_t jentry=0; jentry<nentries;jentry++) {
      Long64_t ientry = LoadTree(jentry);
      if (ientry < 0) break;
      nb = fChain->GetEntry(jentry);   nbytes += nb;

      double TdT = TrigDeltaT/1000.;
      cout<<TrigDeltaT<<endl;

      double dT = (xpar[0]*2.0)/(0.53*299.792);
      if(xperp[0]>8.5&&xperp[0]<9.5) deltaT->Fill(dT);

      for(int i=0; i<Npulses_cfd; i++){

        if(pulsestrip_cfd[i]==9 && pulseside_cfd[i]==0){


          double untime = pulsestart_cfd[i];
          double ampl = pulseamp_cfd[i];
          if(ampl>13.5){
            double delT = 0.4*(1.1774-sqrt(2*TMath::Log(ampl/13.0)));
            //cortime = untime + delT - TdT;
            double cortimedt = untime + delT;
            double cortimeTDT = untime - TdT;
            double cortimedtTDT = untime + delT - TdT;
            UnTimeHist->Fill(untime);
            TimeHistdT->Fill(cortimedt);
            TimeHistTDT->Fill(cortimeTDT);
            TimeHistdtTDT->Fill(cortimedtTDT);
          }


        }
      }


      // if (Cut(ientry) < 0) continue;
   }

   TFile* tf = new TFile("out.root","RECREATE");
   UnTimeHist->Write();
   TimeHistdT->Write();
   TimeHistTDT->Write();
   TimeHistdtTDT->Write();
   deltaT->Write();
   tf->Close();
}
