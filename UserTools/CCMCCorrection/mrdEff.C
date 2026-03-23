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
	if(Y >= bins[iter]) return findBin(Y, iter+1, bins);
	else return iter;
}
void mrdEff(){
        //insert variables here
	double mrd_eff; //weight for MRD Efficiency correction
	double dirt_mu; //weight for dirt muon scaling

	double nuvtxy;
	double simpletracklength;

        vector<double>* MRDTrackStartX = new vector<double>();
        vector<double>* MRDTrackStartY = new vector<double>();
        vector<double>* MRDTrackStartZ = new vector<double>();
        vector<bool>* MRDStop = new vector<bool>();
        
        //Open calibration file
	string mrd_cal_file = "/exp/annie/app/users/jminock/ANNIE_AuxFiles/MRDEffCal.txt";
	if(gSystem->AccessPathName(mrd_cal_file.c_str())){
		std::cout << "WARNING: " << mrd_cal_file << " does not exist. Stopping." << std::endl;
		return false;
	}

	//read in calibration file
	std::vector<double> bins_front;
	std::vector<double> bins_TL;
	std::vector<double> factorX;
	std::vector<double> factorY;
	std::vector<double> factorTL;
	string bins_front_str = "";
	string bins_TL_str = "";
	string factorX_str = "";
	string factorY_str = "";
	string factorTL_str = "";

	std::ifstream cal_file(mrd_cal_file.c_str(), ios::in);
	cal_file >> bins_front_str;
	cal_file >> bins_TL_str;
	cal_file >> factorX_str;
	cal_file >> factorY_str;
	cal_file >> factorTL_str;
	cal_file.close();

	//break into appropriate vectors
	breakCSV(bins_front_str, bins_front);
	breakCSV(bins_TL_str, bins_TL);
	breakCSV(factorX_str, factorX);
	breakCSV(factorY_str, factorY);
	breakCSV(factorTL_str, factorTL);

	int runs = 5000;
	//Loop through runs
	for(int rn = 0; rn < runs; rn++){
		std::cout << "Looping through run " << std::to_string(rn) << std::endl;
        //Open file and trees
        string file_path = "/exp/annie/data/users/jminock/temp_add_branches/PhaseIITree_0." + std::to_string(rn) + ".0.root";

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

        //Set branch addresses
        tTrig->SetBranchAddress("MRDTrackStartX",&MRDTrackStartX);
        tTrig->SetBranchAddress("MRDTrackStartY",&MRDTrackStartY);
        tTrig->SetBranchAddress("MRDTrackStartZ",&MRDTrackStartZ);
        tTrig->SetBranchAddress("MRDStop",&MRDStop);
	tTrig->SetBranchAddress("simpleRecoTrackLengthInMRD",&simpletracklength);
	tTrig->SetBranchAddress("trueNuIntxVtx_Y",&nuvtxy);

	TBranch *MRDEff = tTrig->Branch("MRDEff",&mrd_eff);
	TBranch *DirtMu = tTrig->Branch("DirtMu",&dirt_muon);

        double muon_m = 105.7;
        Long64_t nentriesTrig = tTrig->GetEntries();
//        std::cout << "TriggerTree: " << nentriesTrig << std::endl;
        //fill histograms
        for(Long64_t i = 0; i < nentriesTrig; i++) {
                tTrig->GetEntry(i);
//                if(i%1000 == 0) std::cout << i << std::endl;
		//MRD Calibration
                //Establish MRD X/Y variable to use if there are multiple tracks
		int ntracks = MRDTrackStartY->size();
		if(ntracks <= 0){ //assign 1 for no MRD tracks
			mrd_eff = 1.0;
		} else {
			double MRD_X = -9999;
			double MRD_Y = -9999;
			for(int j = 0; j < ntracks; j++){
//				if(MRDStop->at(j)) MRD_X = MRDTrackStartX->at(j); //confirm its stopping track
				if(MRDStop->at(j)) MRD_Y = MRDTrackStartY->at(j);
			}
			//Establish which bin the event falls into
//			int binX = findBin(MRD_X, 0, bins_front);
			int binY = findBin(MRD_Y, 0, bins_front);
//			int binTL = findBin(simpletracklength*100, 0, bins_TL);
			//Assign weight based on bin
//			mrd_eff = factorX[binX]*factorY[binY]*factorTL[binTL];
			mrd_eff = factorY[binY];
		}
		//Dirt Muon Correction
		if(nuvtxz < 0.){ dirt_muon = 0.0697;
		} else { dirt_muon = 1.; }


		MRDEff->Fill();
	}


        tTrig->Write("",TObject::kOverwrite);
//      tTrig->ResetBranchAddresses();
        delete f;
	} //end of loop run
}
