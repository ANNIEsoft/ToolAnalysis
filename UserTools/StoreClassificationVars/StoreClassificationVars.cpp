#include "StoreClassificationVars.h"

StoreClassificationVars::StoreClassificationVars():Tool(){}

bool StoreClassificationVars::Initialise(std::string configfile, DataModel &data){

	//---------------------------------------------------------------
	//----------------- Useful header -------------------------------
	//---------------------------------------------------------------

	if(configfile!="")  m_variables.Initialise(configfile); 
	m_data= &data;

	save_csv = 1;
	save_root = 0;
	filename = "classification";

	//---------------------------------------------------------------
	//----------------- Configuration variables ---------------------
	//---------------------------------------------------------------
	
	m_variables.Get("verbosity",verbosity);
	m_variables.Get("Filename",filename);
	m_variables.Get("SaveCSV",save_csv);
	m_variables.Get("SaveROOT",save_root);
	m_variables.Get("ModelConfig",model_config);

	std::string logmessage = "StoreClassificationVars Tool: Output Filename is "+filename+".csv/root";
	Log(logmessage,v_message,verbosity);

	//-----------------------------------------------------------------------
	//------- Histograms showing the variables used in the classification----
	//-----------------------------------------------------------------------

	if (save_root){

		std::stringstream ss_filename_root;
		ss_filename_root << filename << ".root";

		file = new TFile(ss_filename_root.str().c_str(),"RECREATE");
		file->cd();

		Log("StoreClassificationVars tool: Initialising histograms associated to TFile "+ss_filename_root.str(),v_message,verbosity);

		//PMT-related variables

		hist_pmtPE = new TH1F("hist_pmtPE","Single PMT Charges",500,0,500);
		hist_pmtTime = new TH1F("hist_pmtTime","Single PMT Times",2000,0,2000);
		hist_pmtTheta = new TH1F("hist_pmtTheta","Single PMT Theta",100,0,TMath::Pi());
		hist_pmtPhi = new TH1F("hist_pmtPhi","Single PMT Phi",100,0,2*TMath::Pi());
		hist_pmtY = new TH1F("hist_pmtY","Single PMT Y",100,-2.,2.);
		hist_pmtDist = new TH1F("hist_pmtDist","Single PMT Distance",100,0.,4.);
		hist_pmtHits = new TH1F("hist_pmtHits","Hit PMTs",150,0,150);
		hist_pmtPEtotal = new TH1F("hist_pmtPEtotal","PMT Total Charge",500,0,12000);
		hist_pmtAvgTime = new TH1F("hist_pmtAvgTime","PMT Average Time",100,0,50);
		hist_pmtAvgDist = new TH1F("hist_pmtAvgDist","PMT Average Distance",100,0,4.);
		hist_pmtThetaBary = new TH1F("hist_pmtAngleBary","PMT #theta_{Bary}",100,0,TMath::Pi());
		hist_pmtRMSTheta = new TH1F("hist_pmtAngleRMS","PMT RMS of #theta",100,0,TMath::Pi());
		hist_pmtVarTheta = new TH1F("hist_pmtAngleVar","PMT Variance of #theta",100,0,TMath::Pi());
		hist_pmtSkewTheta = new TH1F("hist_pmtAngleSkew","PMT Skewness of #theta",100,-10,2);
		hist_pmtKurtTheta = new TH1F("hist_pmtAngleKurt","PMT Kurtosis of #theta",100,-10,2);
		hist_pmtRMSThetaBary = new TH1F("hist_pmtBaryRMS","PMT RMS of #theta_{Bary}",100,0,TMath::Pi());
		hist_pmtVarThetaBary = new TH1F("hist_pmtBaryVar","PMT Variance of #theta_{Bary}",100,0,TMath::Pi());;
		hist_pmtSkewThetaBary = new TH1F("hist_pmtBarySkew","PMT Skewness of #theta_{Bary}",100,-10,2);
		hist_pmtKurtThetaBary = new TH1F("hist_pmtBaryKurt","PMT Kurtosis of #theta_{Bary}",100,-10,2);
		hist_pmtFracLargeAngleThetaBary= new TH1F("hist_pmtFracLargeAngleThetaBary","#theta_{Bary} fraction large angles",100,0,1.);
		hist_pmtRMSPhiBary = new TH1F("hist_pmtRMSPhiBary","PMT RMS of #phi_{Bary}",100,0,TMath::Pi());
		hist_pmtVarPhiBary = new TH1F("hist_pmtVarPhiBary","PMT Variance of #phi_{Bary}",100,0,2*TMath::Pi());
		hist_pmtFracLargeAnglePhiBary = new TH1F("hist_pmtFracLargeAnglePhiBary","#phi_{Bary} fraction large angles",100,0,1.);
		hist_pmtFracRing = new TH1F("hist_pmtFracRing","Fraction of PMT hits in Ring (weighted)",100,0,1);
		hist_pmtFracDownstream = new TH1F("hist_pmtFracDownstream","Fraction of PMT hits downstream",100,0,1);
		hist_pmtFracRingNoWeight = new TH1F("hist_pmtFracRingNoWeight","Fraction of PMT hits in Ring",100,0,1);
		hist_pmtFracHighestQ = new TH1F("hist_pmtFracHighestQ","Charge fraction highest PMT",100,0,1);
		hist_pmtFracClustered = new TH1F("hist_pmtFracClustered","Fraction of charge (clustered)",100,0,1);
		hist_pmtFracEarlyTime = new TH1F("hist_pmtFracEarlyTime","Fraction of early PMT hits",100,0,1.);	//t (PMT) < 4 ns
		hist_pmtFracLateTime = new TH1F("hist_pmtFracLateTime","Fraction of late PMT hits",100,0,1);		//t (PMT) > 10 ns
		hist_pmtFracLowCharge = new TH1F("hist_pmtFracLowCharge","Fraction of low charge PMT hits",100,0,1);	//Q (PMT) < 30 p.e.
		hist_pmtThetaBary_all = new TH1F("hist_pmtThetaBary_all","#theta_{Bary} distribution",100,0,TMath::Pi());
		hist_pmtThetaBary_all_ChargeWeighted = new TH1F("hist_pmtThetaBary_all_ChargeWeighted","Charge weighted #theta_{Bary} distribution",100,0,TMath::Pi());
		hist_pmtYBary_all = new TH1F("hist_pmtYBary_all","Y_{Bary} distribution",100,-2.,2.);
		hist_pmtYBary_all_ChargeWeighted = new TH1F("hist_pmtYBary_all_ChargeWeighted","Charge weighted Y_{Bary} distribution",100,-2.,2.);
		hist_pmtPhiBary_all = new TH1F("hist_pmtPhiBary_all","#phi_{Bary} distribution",100,-TMath::Pi(),TMath::Pi());
		hist_pmtPhiBary_all_ChargeWeighted = new TH1F("hist_pmtPhiBary_all_ChargeWeighted","Charge weighted #phi_{Bary} distribution",100,-TMath::Pi(),TMath::Pi());

		//LAPPD-related variables
		
		hist_lappdPE = new TH1F("hist_lappdPE","Single LAPPD Hit Charges",100,0,5);
		hist_lappdTime = new TH1F("hist_lappdTime","Single LAPPD Hit Times",1000,0,1000);
		hist_lappdTheta = new TH1F("hist_lappdTheta","Single LAPPD Hit Thetas",100,0,TMath::Pi());
		hist_lappdDist = new TH1F("hist_lappdDist","Single LAPPD Hit Distances",100,0,4.);
		hist_lappdHits = new TH1F("hist_lappdHits","Number LAPPD Hits",1000,0,1000);
		hist_lappdPEtotal = new TH1F("hist_lappdPEtotal","LAPPD Total Charge",1000,0,1000);
		hist_lappdAvgTime = new TH1F("hist_lappdAvgTime","LAPPD Average Time",100,0,50);
		hist_lappdAvgDist = new TH1F("hist_lappdAvgDist","LAPPD Average Distance",100,0,4.);
		hist_lappdThetaBary = new TH1F("hist_lappdAngleBary","LAPPD #theta_{Bary}",100,0,TMath::Pi());
		hist_lappdRMSTheta = new TH1F("hist_lappdAngleRMS","LAPPD RMS of #theta",100,0,TMath::Pi());
		hist_lappdVarTheta = new TH1F("hist_lappdAngleVar","LAPPD Variance of #theta",100,0,TMath::Pi());
		hist_lappdSkewTheta = new TH1F("hist_lappdAngleSkew","LAPPD Skewness of #theta",100,-2,10);
		hist_lappdKurtTheta = new TH1F("hist_lappdAngleKurt","LAPPD Kurtosis of #theta",100,-2,10);
		hist_lappdRMSThetaBary = new TH1F("hist_lappdRMSThetaBary","LAPPD RMS of #theta_{Bary}",100,0,TMath::Pi());
		hist_lappdVarThetaBary = new TH1F("hist_lappdVarTheta","LAPPD Variance of #theta_{Bary}",100,0,TMath::Pi());
		hist_lappdSkewThetaBary = new TH1F("hist_lappdSkewTheta","LAPPD Skewness of #theta_{Bary}",100,-2,10);
		hist_lappdKurtThetaBary = new TH1F("hist_lappdKurtThetaBary","LAPPD Kurtosis of #theta_{Bary}",100,-2,10);
		hist_lappdFracRing = new TH1F("hist_lappdFracRing","Fraction of LAPPD Hits in Ring",100,0,1);
		hist_lappdFracRingNoWeight = new TH1F("hist_lappdFracRingNoWeight","Fraction of LAPPD Hits in Ring",100,0,1);

		//MRD-related variables
		
		hist_mrdPaddles = new TH1F("hist_mrdPaddles","Num MRD Paddles hit",310,0,310);
		hist_mrdLayers = new TH1F("hist_mrdLayers","Num MRD Layers hit",11,0,11);
		hist_mrdconsLayers = new TH1F("hist_mrdconsLayers","Num consecutive MRD Layers hit",11,0,11);
		hist_mrdClusters = new TH1F("hist_mrdClusters","Num MRD Clusters",20,0,20);
		hist_mrdXSpread = new TH1F("hist_mrdXSpread","MRD Layer-wise X spread",100,0,5.);
		hist_mrdYSpread = new TH1F("hist_mrdYSpread","MRD Layer-wise Y spread",100,0,5.);
		hist_mrdAdjacentHits = new TH1F("hist_mrdAdjacentHits","MRD # Adjacent hits",100,0,30);
		hist_mrdPaddlesPerLayer = new TH1F("hist_mrdPaddlesPerLayer","MRD Paddles per Layer",100,0,10.);

		//Truth-related variables

		hist_evnum = new TH1F("hist_evnum","Event Numbers",500,0,10000);
		hist_distWallHor = new TH1F("hist_distWallHor","Horizontal distance Vertex - Wall",100,-3.,3.);
		hist_distWallVert = new TH1F("hist_distWallVert","Vertical distance Vertex - Wall",100,-3.,3.);
		hist_distInnerStrHor = new TH1F("hist_distInnerStrHor","Horizontal distance Vertex - Inner Structure",100,-2.,2.);
		hist_distInnerStrVert = new TH1F("hist_distInnerStrVert","Horizontal distance Vertex - Inner Structure",100,-2.,2.);
		hist_energy = new TH1F("hist_energy","Energy of primary particle",5000,0,5000);
		hist_nrings = new TH1F("hist_nrings","Number of rings",20,0,20);
		hist_multiplerings = new TH1F("hist_multiple_rings","Multiple Rings",2,0,2);
		hist_pdg = new TH1F("hist_pdg","PDG codes",500,-500,500);
	} 

	if (save_csv){

		std::stringstream ss_filename_csv;
		ss_filename_csv << filename << ".csv";
		csv_file.open(ss_filename_csv.str().c_str());
		csv_file << "pmt_hits, pmt_totalQ, pmt_avgT, pmt_baryTheta, pmt_rmsTheta, pmt_varTheta, pmt_skewTheta, pmt_kurtTheta, pmt_rmsThetaBary, pmt_varThetaBary, pmt_skewThetaBary, pmt_kurtThetaBary, pmt_rmsPhi, pmt_varPhi, pmt_rmsPhiBary, pmt_varPhiBary, pmt_fracHighestQ, pmt_fracDownstream, pmt_fracClustered, pmt_fracLowQ, pmt_fracLateT, pmt_fracEarlyT, pmt_fracLargeAngleTheta, pmt_fracLargeAnglePhi, lappd_hits, lappd_avgT, lappd_baryTheta, lappd_rmsTheta, lappd_varTheta, lappd_skewTheta, lappd_kurtTheta, lappd_rmsThetaBary, lappd_varThetaBary, lappd_skewThetaBary, lappd_kurtThetaBary, num_mrd_paddles, num_mrd_layers, num_mrd_conslayers, num_mrd_clusters, mrd_padperlayer, mrd_mean_xspread, mrd_mean_yspread, num_mrd_adjacent " << std::endl;
		
		std::stringstream ss_statusfilename_csv;
		ss_statusfilename_csv << filename << "_status.csv";
		csv_statusfile.open(ss_statusfilename_csv.str().c_str());
		csv_statusfile << "energy, evnum, distWallVert, distWallHor, distInnerStrVert, pmt_fracRing, lappd_fracRing, nrings, multiplerings, pdg"<<std::endl;

		Log("StoreClassificationVars tool: Initialised & opened csv file to write the classification variables to",v_message,verbosity);

	}

	Log("StoreClassificationVars Tool: Initialization complete",v_message,verbosity);

	return true;
}


bool StoreClassificationVars::Execute(){

	Log("StoreClassificationVars Tool: Executing",v_debug,verbosity);

	//-----------------------------------------------------------------------
	//-------------- Getting variables from Classification store ------------
	//-----------------------------------------------------------------------
	
	get_ok = m_data->Stores.count("Classification");
	if(!get_ok){
		Log("StoreClassificationVars Tool: No Classification store! Please run CalcClassificationVars before this tool. Exiting...",v_error,verbosity);
		return false;
	};

	bool selection_passed;
	m_data->Stores["Classification"]->Get("SelectionPassed",selection_passed);
	if (!selection_passed) {
		Log("StoreClassificationVars Tool: EventSelection cuts were not passed. Skip this event",v_message,verbosity);
		return true;
	}
	
	
        //General variables
        m_data->Stores["Classification"]->Get("MC-based",use_mctruth);

	//PMT variables
        m_data->Stores["Classification"]->Get("PMTAvgDist",pmt_avgDist);
        m_data->Stores["Classification"]->Get("PMTAvgT",pmt_avgT);
        m_data->Stores["Classification"]->Get("PMTQtotal",pmt_totalQ);
        m_data->Stores["Classification"]->Get("PMTHits",pmt_hits);
        m_data->Stores["Classification"]->Get("PMTFracQmax",pmt_fracHighestQ);
        m_data->Stores["Classification"]->Get("PMTFracQdownstream",pmt_fracDownstream);
        m_data->Stores["Classification"]->Get("PMTFracClustered",pmt_fracClustered);
        m_data->Stores["Classification"]->Get("PMTFracLowQ",pmt_fracLowQ);
        m_data->Stores["Classification"]->Get("PMTFracEarly",pmt_fracEarlyT);
        m_data->Stores["Classification"]->Get("PMTFracLate",pmt_fracLateT);
        m_data->Stores["Classification"]->Get("PMTRMSTheta",pmt_rmsTheta);
        m_data->Stores["Classification"]->Get("PMTVarTheta",pmt_varTheta);
        m_data->Stores["Classification"]->Get("PMTSkewTheta",pmt_skewTheta);
        m_data->Stores["Classification"]->Get("PMTKurtTheta",pmt_kurtTheta);
        m_data->Stores["Classification"]->Get("PMTRMSThetaBary",pmt_rmsThetaBary);
        m_data->Stores["Classification"]->Get("PMTVarThetaBary",pmt_varThetaBary);
        m_data->Stores["Classification"]->Get("PMTSkewThetaBary",pmt_skewThetaBary);
        m_data->Stores["Classification"]->Get("PMTKurtThetaBary",pmt_kurtThetaBary);
        m_data->Stores["Classification"]->Get("PMTRMSPhiBary",pmt_rmsPhiBary);
        m_data->Stores["Classification"]->Get("PMTVarPhiBary",pmt_varPhiBary);
        m_data->Stores["Classification"]->Get("PMTFracLargeAngleTheta",pmt_fracLargeAngleTheta);
        m_data->Stores["Classification"]->Get("PMTHitsLargeAngleTheta",pmt_hitsLargeAngleTheta);
        m_data->Stores["Classification"]->Get("PMTFracLargeAnglePhi",pmt_fracLargeAnglePhi);
        m_data->Stores["Classification"]->Get("PMTHitsLargeAnglePhi",pmt_hitsLargeAnglePhi);


	//LAPPD variables
        m_data->Stores["Classification"]->Get("LAPPDAvgDistance",lappd_avgDist);
        m_data->Stores["Classification"]->Get("LAPPDQtotal",lappd_totalQ);
        m_data->Stores["Classification"]->Get("LAPPDAvgT",lappd_avgT);
        m_data->Stores["Classification"]->Get("LAPPDHits",lappd_hits);
        m_data->Stores["Classification"]->Get("LAPPDRMSTheta",lappd_rmsTheta);
        m_data->Stores["Classification"]->Get("LAPPDVarTheta",lappd_varTheta);
        m_data->Stores["Classification"]->Get("LAPPDSkewTheta",lappd_skewTheta);
        m_data->Stores["Classification"]->Get("LAPPDKurtTheta",lappd_kurtTheta);
        m_data->Stores["Classification"]->Get("LAPPDRMSThetaBary",lappd_rmsThetaBary);
        m_data->Stores["Classification"]->Get("LAPPDVarThetaBary",lappd_varThetaBary);
        m_data->Stores["Classification"]->Get("LAPPDSkewThetaBary",lappd_skewThetaBary);
        m_data->Stores["Classification"]->Get("LAPPDKurtThetaBary",lappd_kurtThetaBary);


	//MRD variables
	m_data->Stores["Classification"]->Get("MrdLayers",num_mrd_layers);
        m_data->Stores["Classification"]->Get("MrdPaddles",num_mrd_paddles);
        m_data->Stores["Classification"]->Get("MrdConsLayers",num_mrd_conslayers);
        m_data->Stores["Classification"]->Get("MrdAdjHits",num_mrd_adjacent);
        m_data->Stores["Classification"]->Get("MrdPadPerLayer",mrd_padperlayer);
        m_data->Stores["Classification"]->Get("MrdXSpread",mrd_mean_xspread);
        m_data->Stores["Classification"]->Get("MrdYSpread",mrd_mean_yspread);
        m_data->Stores["Classification"]->Get("MrdClusters",MrdClusters);


	//MC-Truth variables
        m_data->Stores["Classification"]->Get("PMTFracRing",pmt_fracRing);
        m_data->Stores["Classification"]->Get("PMTFracRingNoWeight",pmt_fracRingNoWeight);
        m_data->Stores["Classification"]->Get("LAPPDFracRing",lappd_fracRing);
        m_data->Stores["Classification"]->Get("LAPPDFracRingNoWeight",lappd_fracRingNoWeight);
	m_data->Stores["Classification"]->Get("VDistVtxWall",distWallVert);
        m_data->Stores["Classification"]->Get("HDistVtxWall",distWallHor);
        m_data->Stores["Classification"]->Get("VDistVtxInner",distInnerStrVert);
        m_data->Stores["Classificaiton"]->Get("HDistVtxInner",distInnerStrHor);
        m_data->Stores["Classification"]->Get("VtxTrueTime",true_time);
        m_data->Stores["Classification"]->Get("TrueEnergy",energy);
        m_data->Stores["Classificaiton"]->Get("NRings",nrings);
        m_data->Stores["Classification"]->Get("MultiRing",multiplering);
        m_data->Stores["Classification"]->Get("EventNumber",evnum);
	m_data->Stores["Classification"]->Get("PDG",pdg);


	if (save_root){

		//-----------------------------------------------------------------------
		//--------------- Filling properties into histograms---------------------
		//-----------------------------------------------------------------------
	
		Log("StoreClassificationVars tool: Filling classification variables into histograms.",v_debug,verbosity);

		file->cd();

		hist_pmtHits->Fill(pmt_hits);
		hist_pmtPEtotal->Fill(pmt_totalQ);
		hist_pmtAvgTime->Fill(pmt_avgT);
		hist_pmtAvgDist->Fill(pmt_avgDist);
		hist_pmtAngleBary->Fill(pmtBaryAngle);
		hist_pmtAngleRMS->Fill(pmt_rmsAngle);
		hist_pmtAngleVar->Fill(pmt_varAngle);
		hist_pmtAngleSkew->Fill(pmt_skewAngle);
		hist_pmtAngleKurt->Fill(pmt_kurtAngle);
		hist_pmtBaryRMS->Fill(pmt_rmsAngleBary);
		hist_pmtBaryVar->Fill(pmt_varAngleBary);
		hist_pmtBarySkew->Fill(pmt_skewAngleBary);
		hist_pmtBaryKurt->Fill(pmt_kurtAngleBary);
		hist_pmtPhiBaryRMS->Fill(pmt_rmsPhiBary);
		hist_pmtPhiBaryVar->Fill(pmt_varPhiBary);
		hist_pmtFracRing->Fill(pmt_fracRing);
		hist_pmtFracDownstream->Fill(pmt_fracQDownstream);
		hist_pmtFracRingNoWeight->Fill(pmt_fracRingNoWeight);
		hist_pmtFracHighestQ->Fill(pmt_frachighestQ);
		hist_pmtFracClustered->Fill(pmt_fracClustered);
		hist_pmtFracLateTime->Fill(pmt_fracLate);
		hist_pmtFracLowCharge->Fill(pmt_fracLowQ);
		hist_pmtFracEarlyTime->Fill(pmt_fracEarly);
		hist_pmtBaryFracLargeAngle->Fill(pmt_fracLargeAngle);
		hist_pmtPhiBaryFracLargeAngle->Fill(pmt_fracLargeAnglePhi);

		hist_lappdHits->Fill(lappd_hits);
		hist_lappdPEtotal->Fill(lappd_totalQ);
		hist_lappdAvgTime->Fill(lappd_avgT);
		hist_lappdAvgDist->Fill(lappd_avgDist);
		hist_lappdAngleBary->Fill(lappdBaryAngle);
		hist_lappdAngleRMS->Fill(lappd_rmsAngle);
		hist_lappdAngleVar->Fill(lappd_varAngle);
		hist_lappdAngleSkew->Fill(lappd_skewAngle);
		hist_lappdAngleKurt->Fill(lappd_kurtAngle);
		hist_lappdBaryRMS->Fill(lappd_rmsAngleBary);
		hist_lappdBaryVar->Fill(lappd_varAngleBary);
		hist_lappdBarySkew->Fill(lappd_skewAngleBary);
		hist_lappdBaryKurt->Fill(lappd_kurtAngleBary);
		hist_lappdFracRing->Fill(lappd_fracRing);
		hist_lappdFracRingNoWeight->Fill(lappd_fracRingNoWeight);

		hist_evnum->Fill(evnum);
		hist_distWallHor->Fill(distWallHor);
		hist_distWallVert->Fill(distWallVert);
		hist_distInnerStrHor->Fill(distInnerStrHor);
		hist_distInnerStrVert->Fill(distInnerStrVert);
		hist_energy->Fill(TrueMuonEnergy);
		hist_nrings->Fill(nrings);
		hist_multiplerings->Fill(multiplering);
		hist_pdg->Fill(pdg);
	}

	if (save_csv){

		//-----------------------------------------------------------------------
		//--------------- Store properties in csv-file --------------------------
		//-----------------------------------------------------------------------
		
		Log("StoreClassificationVars tool: Storing properties in csv-file: ",v_debug,verbosity);


		csv_file << pmt_hits
		<<","<<pmt_totalQ
                <<","<<pmt_avgT
                <<","<<pmt_baryTheta
		<<","<<pmt_rmsTheta
		<<","<<pmt_varTheta
		<<","<<pmt_skewTheta
		<<","<<pmt_kurtTheta
		<<","<<pmt_rmsThetaBary
		<<","<<pmt_varThetaBary
		<<","<<pmt_skewThetaBary
		<<","<<pmt_kurtThetaBary
		<<","<<pmt_rmsPhi
		<<","<<pmt_varPhi
		<<","<<pmt_rmsPhiBary
		<<","<<pmt_varPhiBary
		<<","<<pmt_fracHighestQ
		<<","<<pmt_fracDownstream
		<<","<<pmt_fracClustered
		<<","<<pmt_fracLowQ
		<<","<<pmt_fracLateT
		<<","<<pmt_fracEarlyT
		<<","<<pmt_fracLargeAngleTheta
		<<","<<pmt_fracLargeAnglePhi
		<<","<<lappd_hits
		<<","<<lappd_avgT
		<<","<<lappd_baryTheta
		<<","<<lappd_rmsTheta
		<<","<<lappd_varTheta
		<<","<<lappd_skewTheta
		<<","<<lappd_kurtTheta
		<<","<<lappd_rmsThetaBary
		<<","<<lappd_varThetaBary
		<<","<<lappd_skewThetaBary
		<<","<<lappd_kurtThetaBary
		<<","<<num_mrd_paddles
		<<","<<num_mrd_layers
		<<","<<num_mrd_conslayers
		<<","<<num_mrd_clusters
		<<","<<mrd_padperlayer
		<<","<<mrd_mean_xspread
		<<","<<mrd_mean_yspread
		<<","<<num_mrd_adjacent
		std::endl;

		csv_statusfile << energy
		<<","<<evnum
		<<","<<distWallVert
		<<","<<distWallHor
		<<","<<distInnerStrVert
		<<","<<distInnerStrHor
		<<","<<pmt_fracRing
		<<","<<lappd_fracRing
		<<","<<nrings
		<<","<<multiplerings
		<<","<<pdg
		<<std::endl;
		


	}

	return true;

}


bool StoreClassificationVars::Finalise(){

  	Log("StoreClassificationVars tool: Finalisation started",v_message,verbosity);
  	
	if (save_root){
        
		//-----------------------------------------------------------------------
        	//-------------- Write overview histograms to file ----------------------
        	//-----------------------------------------------------------------------	
		
		Log("StoreClassificationVars tool: Writing histograms to root-file.",v_message,verbosity);
		
  		file->cd();
 
		//PMT histograms
 		        
        	hist_pmtPE->Write();
        	hist_pmtTime->Write();
		hist_pmtTheta->Write();
		hist_pmtPhi->Write();
		hist_pmtY->Write();
		hist_pmtDist->Write();
        	hist_pmtHits->Write();
        	hist_pmtPEtotal->Write();
        	hist_pmtAvgTime->Write();
        	hist_pmtThetaBary->Write();
        	hist_pmtRMSTheta->Write();
        	hist_pmtVarTheta->Write();
        	hist_pmtSkewTheta->Write();
        	hist_pmtKurtTheta->Write();
       		hist_pmtRMSThetaBary->Write();
        	hist_pmtVarThetaBary->Write();
        	hist_pmtSkewThetaBary->Write();
        	hist_pmtKurtThetaBary->Write();
		hist_pmtRMSPhiBary->Write();
		hist_pmtVarPhiBary->Write();
        	hist_pmtFracDownstream->Write();
        	hist_pmtFracRing->Write();
        	hist_pmtFracRingNoWeight->Write();
        	hist_pmtFracHighestQ->Write();
		hist_pmtFracClustered->Write();
		hist_pmtFracLowCharge->Write();
		hist_pmtFracLateTime->Write();
		hist_pmtFracEarlyTime->Write();
		hist_pmtThetaBary_all->Write();
		hist_pmtThetaBary_QWeighted_all->Write();
		hist_pmtPhiBary_all->Write();
		hist_pmtPhiBary_QWeighted_all->Write();
		hist_pmtYBary_all->Write();
		hist_pmtYBary_QWeighted_all->Write();
		hist_pmtFracLargeAngleThetaBary->Write();
		hist_pmtFracLargeAnglePhiBary->Write();

		//LAPPD histograms
	
        	hist_lappdPE->Write();
        	hist_lappdTime->Write();
        	hist_lappdTheta->Write();
        	hist_lappdDist->Write();
        	hist_lappdHits->Write();
        	hist_lappdPEtotal->Write();
       		hist_lappdAvgTime->Write();
        	hist_lappdThetaBary->Write();
        	hist_lappdRMSTheta->Write();
        	hist_lappdVarTheta->Write();
        	hist_lappdSkewTheta->Write();
        	hist_lappdKurtTheta->Write();
        	hist_lappdRMSThetaBary->Write();
        	hist_lappdVarThetaBary->Write();
        	hist_lappdSkewThetaBary->Write();
        	hist_lappdKurtThetaBary->Write();
        	hist_lappdFracRing->Write();
        	hist_lappdFracRingNoWeight->Write();
	
		//MRD histograms

		hist_mrdPaddles->Write();
		hist_mrdLayers->Write();
		hist_mrdconsLayers->Write();
		hist_mrdClusters->Write();	
		hist_mrdXSpread->Write();
		hist_mrdYSpread->Write();
		hist_mrdAdjacentHits->Write();
		hist_mrdPaddlesPerLayer->Write();
	
		//MC-Truth histograms

		hist_evnum->Write();
		hist_distWallHor->Write();
		hist_distWallVert->Write();
		hist_distInnerStrHor->Write();
		hist_distInnerStrVert->Write();
		hist_energy->Write();
		hist_nrings->Write();
		hist_multiplerings->Write();

		file->Close();
		delete file;

		Log("StoreClassificationVars Tool: Histograms written to root-file.",v_message,verbosity);
	} 

	if (save_csv) {

		csv_file.close();	
		csv_statusfile.close();
		Log("StoreClassificationVars Tool: Information written to csv-file.",v_message,verbosity);

	}
 
	Log("StoreClassificationVars Tool: Finalisation complete",v_message,verbosity);

	return true;

}

