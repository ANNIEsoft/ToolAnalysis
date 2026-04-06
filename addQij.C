#include "TROOT.h"
#include "TFile.h"
#include "TTree.h"
#include "THStack.h"
#include "TLegend.h"
#include "TF1.h"
#include "TF2.h"
#include "TLine.h"
#include "TMath.h"
#include <vector>
#include <map>
#include <string>
#include <iostream>
#include <cmath>

double LoC(double vtxx, double vtxy, double vtxz, double pmt1x, double pmt1y, double pmt1z, double pmt2x, double pmt2y, double pmt2z){
	double b = (vtxx-pmt1x)*(vtxx-pmt1x) + (vtxy-pmt1y)*(vtxy-pmt1y) + (vtxz-pmt1z)*(vtxz-pmt1z);
	double c = (vtxx-pmt2x)*(vtxx-pmt2x) + (vtxy-pmt2y)*(vtxy-pmt2y) + (vtxz-pmt2z)*(vtxz-pmt2z);
	double a = (pmt2x-pmt1x)*(pmt2x-pmt1x) + (pmt2y-pmt1y)*(pmt2y-pmt1y) + (pmt2z-pmt1z)*(pmt2z-pmt1z);
	return std::acos((b + c - a)/(2*std::sqrt(b*c)));
}

void addQij(){

	int runs = 1;//4000;
	int subruns = 1;
	//Loop through runs
	for(int rn = 0; rn < runs; rn++){
		std::cout << "Looping through run " << std::to_string(rn) << std::endl;
	//Loop through run parts (sub runs)
	for(int srn = 0; srn < subruns; srn++){
//		std::cout << "Looping through subrun " << std::to_string(srn) << std::endl;
        //Open file and trees
        string file_path = "PhaseIITree_0." + std::to_string(rn) + "." + std::to_string(srn) + ".root";

	//check if files exist
	if(gSystem->AccessPathName(file_path.c_str())){
		std::cout << "WARNING: " << file_path << " does not exist. Skipping." << std::endl;
		continue;
	}
        //Open file and trees
        TFile *f = new TFile(file_path.c_str(),"update");
        gSystem->Load("/exp/annie/app/users/jminock/ToolAnalysis/lib/libDataModel.so");
//      gSystem->Load("/exp/annie/app/users/jminock/ToolAnalysis/lib/libDict.so");
        gInterpreter->GenerateDictionary("map<string,vector<double>>", "map;string;vector");
        TTree *tTrig = (TTree*)f->Get("phaseIITriggerTree");

        //insert variables here
	vector<double>* hitT = new vector<double>();
	vector<double>* hitX = new vector<double>();
	vector<double>* hitY = new vector<double>();
	vector<double>* hitZ = new vector<double>();
	vector<double>* hitQ = new vector<double>();

	double Qij;

        double simplevtxx, simplevtxy, simplevtxz; 

        //Set branch addresses
        tTrig->SetBranchAddress("hitT",&hitT);
        tTrig->SetBranchAddress("hitX",&hitX);
        tTrig->SetBranchAddress("hitY",&hitY);
        tTrig->SetBranchAddress("hitZ",&hitZ);
        tTrig->SetBranchAddress("hitQ",&hitQ);
        tTrig->SetBranchAddress("simpleRecoVtxX",&simplevtxx);
        tTrig->SetBranchAddress("simpleRecoVtxY",&simplevtxy);
        tTrig->SetBranchAddress("simpleRecoVtxZ",&simplevtxz);

        //TBranch *Qij_branch = tTrig->Branch("Qij",&Qij);

        double muon_m = 105.66;
	double time_window = 20.0;
        Long64_t nentriesTrig = tTrig->GetEntries();
//        std::cout << "TriggerTree: " << nentriesTrig << std::endl;
        //fill histograms
        for (Long64_t i = 0; i < nentriesTrig; i++) {
//      for (Long64_t i = 0; i < 2; i++) {
                tTrig->GetEntry(i);
//                if(i%1000 == 0) std::cout << i << std::endl;
		double qval = 0.0;
		double num = 0.0;
		double minT = 99999.0;
		for(int j = 0; j < hitT->size(); ++j){
			if(hitT->at(j) <= 0) continue;
			if(minT > hitT->at(j)) minT = hitT->at(j);
		}
		std::cout << "minT: " << minT << std::endl;
		for(int j = 0; j < hitT->size(); ++j){
			if(hitT->at(j) > (minT+time_window)) continue; //"prompt" cluster cut
			qval += hitQ->at(j);
			double qi = hitQ->at(j);
			for(int k = 0; k < hitT->size(); ++k){
				if(j == k) continue;
				if(hitT->at(k) > (minT+time_window)) continue; //"prompt" cluster cut
				double qj = hitQ->at(k);
				num += qi*qj*LoC(simplevtxx,simplevtxy+0.1446,simplevtxz-1.681,hitX->at(j),hitY->at(j),hitZ->at(j),hitX->at(k),hitY->at(k),hitZ->at(k));
			}
		}

		Qij = num/qval;
		std::cout << "Qij: " << Qij << std::endl;
		std::cout << "qval: " << qval << std::endl;
	}


//        tTrig->Write("",TObject::kOverwrite);
//      tTrig->ResetBranchAddresses();
        delete f;
	}//end subrun loop
	}//end run loop
}
