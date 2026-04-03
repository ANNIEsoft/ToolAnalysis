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
#include <fstream>
#include <sstream>
#include <cmath>
#include <algorithm>

void breakCSV(string line, vector<double> &tokens){
	//change line into stream
	std::stringstream line_in(line);
	std::string temp_token;
	double token;
	while (line_in.good()){
		std::getline(line_in, temp_token, ','); //break up by comma
		std::stringstream to_token(temp_token);
		to_token >> token;          //turn into double
		tokens.push_back(token);    //save to array
	}
}

int findBin(double Y, int iter, vector<double> const &bins){
	if(iter == bins.size()) return 0;
	if(Y < bins[iter]) return iter;
	else return findBin(Y, iter+1, bins);
}

void addMRDUnc(){
        //insert variables here
	double mrd_eff;
	double dirt_muon;

        vector<double>* MRDTrackStartY = new vector<double>();
	vector<bool>* MRDStop = new vector<bool>();

	vector<double>* MRDUnc = new vector<double>();
//	vector<double>* DirtUnc = new vector<double>();

	//read in calibration file
	std::vector<double> bins_front;
	std::vector<double> factorY;
	std::vector<double> uncsY;
	string bins_front_str = "";
	string factorY_str = "";
	string uncsY_str = "";

	string mrd_cal_file = "/exp/annie/app/users/jminock/ANNIE_AuxFiles/MRDEffUncYlarge.txt";
	std::ifstream cal_file(mrd_cal_file.c_str(), ios::in);
	cal_file >> bins_front_str;
	cal_file >> factorY_str;
	cal_file >> uncsY_str;
	cal_file.close();

	breakCSV(bins_front_str, bins_front);
	breakCSV(factorY_str, factorY);
	breakCSV(uncsY_str, uncsY);

	TRandom3 rnd;
	rnd.SetSeed(42);
	int n_univ = 60;
	std::vector<std::vector<double>> mrd_reweight_vector;
//	std::vector<double> dirt_rew_vector;
	for(int i = 0; i < uncsY.size(); ++i){
		std::vector<double> universes;
		for(int j = 0; j < n_univ; ++j){
			double mrd_unc = rnd.Gaus(1., uncsY.at(i));
//			double mrd_unc = rnd.Gaus(1., std::abs(factorY.at(i) - 1.));
			universes.push_back(mrd_unc);
		}
		mrd_reweight_vector.push_back(universes);
	}
/*	for(int j = 0; j < n_univ; ++j){
		dirt_rew_vector.push_back(rnd.Gaus(1., 0.0028));
	}
*/

        //Open file and trees
	int runs = 3500;
	int subruns = 1;
	//Loop through runs
	for(int rn = 2500; rn < runs; rn++){
		std::cout << "Looping through run " << std::to_string(rn) << std::endl;
	//Loop through run parts (sub runs)
	for(int srn = 0; srn < subruns; srn++){
//		std::cout << "Looping through subrun " << std::to_string(srn) << std::endl;
        //Open file and trees
        string file_path = "/exp/annie/data/users/jminock/temp_add_branches/PhaseIITree_0." + std::to_string(rn) + "." + std::to_string(srn) + ".root";
//        string file_path = "/pnfs/annie/persistent/users/jminock/v1_3_3_weighted_ntuples/PhaseIITree_0." + std::to_string(rn) + "." + std::to_string(srn) + ".root";

	//check if files exist
	if(gSystem->AccessPathName(file_path.c_str())){
		std::cout << "WARNING: " << file_path << " does not exist. Skipping." << std::endl;
		continue;
	}
 
        TFile *f = new TFile(file_path.c_str(),"update");
//        gSystem->Load("/exp/annie/app/users/jminock/ToolAnalysis/lib/libDataModel.so");
//      gSystem->Load("/exp/annie/app/users/jminock/ToolAnalysis/lib/libDict.so");
//        gInterpreter->GenerateDictionary("map<string,vector<double>>", "map;string;vector");
        TTree *tTrig = (TTree*)f->Get("phaseIITriggerTree");

	//TRandom3 rnd;
	//rnd.SetSeed(rn);

        //Set branch addresses
        tTrig->SetBranchAddress("MRDTrackStartY",&MRDTrackStartY);
        tTrig->SetBranchAddress("MRDStop",&MRDStop);
//        tTrig->SetBranchAddress("DirtMu",&dirt_muon);

        //Branches to add
        //TBranch *MRDE   = tTrig->Branch("MRDEff",&mrd_eff);
        TBranch *MRDU    = tTrig->Branch("weight_MRDUnc",&MRDUnc);
//        TBranch *DU    = tTrig->Branch("weight_DirtUnc",&DirtUnc);

        double muon_m = 105.7;
	double threshold = 0.0001;
        Long64_t nentriesTrig = tTrig->GetEntries();
//        std::cout << "TriggerTree: " << nentriesTrig << std::endl;
        //fill histograms
        for (Long64_t i = 0; i < nentriesTrig; i++) {
                tTrig->GetEntry(i);
//                if(i%1000 == 0) std::cout << i << std::endl;

		//MRD Eff
                //Establish MRD X/Y variable to use if there are multiple tracks
		int ntracks = MRDTrackStartY->size();
		if(ntracks <= 0) {
			mrd_eff = 1.0;
			for(int j = 0; j < n_univ; ++j){
				MRDUnc->push_back(1.0);
			}
		} else {
			double MRD_Y = -9999;
			for(int j = 0; j < ntracks; j++){
				if(MRDStop->at(j)) MRD_Y = MRDTrackStartY->at(j);
			}
			//Establish which bin the event falls into
			int binY = findBin(MRD_Y, 0, bins_front);
			//Assign uncertainty based on bin
			//mrd_eff = factorY[binY];
			//double mrd_uncY = uncsY[binY];
			for(int j = 0; j < n_univ; ++j){
				MRDUnc->push_back(mrd_reweight_vector.at(binY).at(j));
				//MRDUnc->push_back(rnd.Gaus(1.,mrd_uncY));
			}
		}

		//Dirt Muon Correction
/*		if(std::abs(dirt_muon - 1) < threshold){
			for(int j = 0; j<n_univ; ++j) DirtUnc->push_back(1.);
		} else { //is a dirt muon
			//0.0028 is calculated uncertainty from total # of events / POT
			for(int j = 0; j<n_univ; ++j) DirtUnc->push_back(dirt_rew_vector.at(j));
			//for(int j = 0; j<n_univ; ++j) DirtUnc->push_back(rnd.Gaus(1., 0.0028);
		}
*/
		//MRDE->Fill();
		MRDU->Fill();
//		DU->Fill();

		MRDUnc->clear();
//		DirtUnc->clear();
	}


        tTrig->Write("",TObject::kOverwrite);
//      tTrig->ResetBranchAddresses();
        delete f;
	} //end of subrun
	} //end of run
}
