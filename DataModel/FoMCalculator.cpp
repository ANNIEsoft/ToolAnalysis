#include "FoMCalculator.h"

//Constructor
FoMCalculator::FoMCalculator() {
  fVtxGeo = 0;
  fBaseFOM = 100.0;
  
  // fitting parameters ported from vertexFinder (FIXME)
  fTimeFitWeight = 0.50;  // nominal time weight
  fConeFitWeight = 0.50;  // nominal cone weight
  
  // default Mean time calculator type
  fMeanTimeCalculatorType = 0;
}

//Destructor
FoMCalculator::~FoMCalculator() {
	fVtxGeo = 0;
}
	

void FoMCalculator::LoadVertexGeometry(VertexGeometry* vtxgeo) {
  this->fVtxGeo = vtxgeo;	
}


void FoMCalculator::TimePropertiesLnL(double vtxTime, double& vtxFOM)
{ 
  // internal variables
  // ==================
  double weight = 0.0;
  double delta = 0.0;       // time residual of each hit
  double sigma = 0.0;       // time resolution of each hit
  double A = 0.0;           // normalisation of first Gaussian
  int type;                 // Digit type (LAPPD or PMT)
  
  double Preal = 0.0;       // probability of real hit
  double P = 0.0;           // probability of hit

  double chi2 = 0.0;        // log-likelihood: chi2 = -2.0*log(L)
  double ndof = 0.0;        // total number of hits
  double fom = -9999.;         // figure of merit

  // add noise to model
  // ==================
  double Pnoise;

  //FIXME: We need an implementation of noise models for PMTs and LAPPDs
  //double nFilterDigits = this->fVtxGeo->GetNFilterDigits(); 
  //double fTimeFitNoiseRate = 0.02;  // hits/ns [0.40 for electrons, 0.02 for muons]
  //Pnoise = fTimeFitNoiseRate/nFilterDigits;
  
  // loop over digits
  // ================
  for( int idigit=0; idigit<this->fVtxGeo->GetNDigits(); idigit++ ){    
    	int detType = this->fVtxGeo->GetDigitType(idigit); 
      delta = this->fVtxGeo->GetDelta(idigit) - vtxTime;
      sigma = this->fVtxGeo->GetDeltaSigma(idigit);
      type = this->fVtxGeo->GetDigitType(idigit);
      if (type == RecoDigit::PMT8inch){  //PMT8Inch
        sigma = 1.5*sigma;
        Pnoise = 1e-8; //FIXME; Need implementation of noise model 
      } 
      if (type == RecoDigit::lappd_v0){  //lappd
        sigma = 1.2*sigma;
        Pnoise = 1e-8;  //FIXME; Need implementation of noise model 
      }
      A  = 1.0 / ( 2.0*sigma*sqrt(0.5*TMath::Pi()) ); //normalisation constant
      Preal = A*exp(-(delta*delta)/(2.0*sigma*sigma));
      P = (1.0-Pnoise)*Preal + Pnoise; 
      chi2 += -2.0*log(P);
      ndof += 1.0; 
  }	

  // calculate figure of merit
  if( ndof>0.0 ){
    fom = fBaseFOM - 5.0*chi2/ndof;
  }  

  // return figure of merit
  // ======================
  vtxFOM = fom;
  return;
}




void FoMCalculator::ConePropertiesFoM(double coneEdge, double& coneFOM)
{  
  // calculate figure of merit
  // =========================
  double coneEdgeLow = 21.0;  // cone edge (low side)      
  double coneEdgeHigh = 3.0;  // cone edge (high side)   [muons: 3.0, electrons: 7.0]
  double deltaAngle = 0.0;
  double digitCharge = 0.0;
  double coneCharge = 0.0;
  double allCharge = 0.0;
  double outerCone = -99.9;
  int outhits = 0;
  int inhits = 0;

  double fom = -9999.;

  for( int idigit=0; idigit<this->fVtxGeo->GetNDigits(); idigit++ ){ 	
    if( this->fVtxGeo->IsFiltered(idigit) && this->fVtxGeo->GetDigitType(idigit) == RecoDigit::PMT8inch){
      deltaAngle = this->fVtxGeo->GetAngle(idigit) - coneEdge;
      digitCharge = this->fVtxGeo->GetDigitQ(idigit);

      if( deltaAngle<=0.0 ){
        coneCharge += digitCharge*( 0.75 + 0.25/( 1.0 + (deltaAngle*deltaAngle)/(coneEdgeLow*coneEdgeLow) ) );
	inhits++;	
        //if (deltaAngle > outerCone) outerCone = deltaAngle;
      }
      else{
        coneCharge += digitCharge*( 0.00 + 1.00/( 1.0 + (deltaAngle*deltaAngle)/(coneEdgeHigh*coneEdgeHigh) ) );
	outhits++;
        //outerCone = 0;
      }

      allCharge += digitCharge;
      //outerCone = -outhits/inhits;
    }
  }

  if( allCharge>0.0 ){
    if( outerCone>-42 ){
      fom = fBaseFOM*coneCharge/allCharge/*exp(outerCone)*/;
    }else{
      fom = fBaseFOM*coneCharge/allCharge;
    }
  }

  // return figure of merit
  // ======================
  coneFOM = fom;
  return;
}

void FoMCalculator::ConePropertiesLnL(double vtxX, double vtxY, double vtxZ, double dirX, double dirY, double dirZ, double coneEdge, double& chi2, TH1D angularDist, double& phimax, double& phimin) {
    double coneEdgeLow = 21.0;  // cone edge (low side)      
    double coneEdgeHigh = 3.0;  // cone edge (high side)   [muons: 3.0, electrons: 7.0]
    double deltaAngle = 0.0;
    double digitCharge = 0.0;
    double digitPE = 0.0;
    double coneCharge = 0.0;
    double allCharge = 0.0;
    double outerCone = -99.9;
    double coef = angularDist.Integral(); //1000;
    chi2 = 0;
    cout << "ConePropertiesLnL Position: (" << vtxX << ", " << vtxY << ", " << vtxZ << ")" << endl;
    cout << "And Direction: (" << dirX << ", " << dirY << ", " << dirZ << ")" << endl;

    double digitX, digitY, digitZ;
    double dx, dy, dz, ds;
    double px, py, pz;
    double cosphi, phi, phideg;
    phimax = 0;
    phimin = 10;
    double allPE = 0;
    int refbin;
    double weight;
    double P;
    
    for (int idigit = 0; idigit < this->fVtxGeo->GetNDigits(); idigit++) {
        if (this->fVtxGeo->IsFiltered(idigit) && this->fVtxGeo->GetDigitType(idigit) == RecoDigit::PMT8inch) {
            digitCharge = this->fVtxGeo->GetDigitQ(idigit);
            allCharge += digitCharge;
        }
    }

    for (int idigit = 0; idigit < this->fVtxGeo->GetNDigits(); idigit++) {
        if (this->fVtxGeo->IsFiltered(idigit) && this->fVtxGeo->GetDigitType(idigit) == RecoDigit::PMT8inch) {
            deltaAngle = this->fVtxGeo->GetAngle(idigit) - coneEdge;
            digitCharge = this->fVtxGeo->GetDigitQ(idigit);
            //digitPE = this->fVtxGeo->GetDigitPE(idigit);
            digitX = fVtxGeo->GetDigitX(idigit);
            digitY = fVtxGeo->GetDigitY(idigit);
            digitZ = fVtxGeo->GetDigitZ(idigit);
            dx = digitX - vtxX;
            dy = digitY - vtxY;
            dz = digitZ - vtxZ;
            std::cout << "dx, dy, dz: " << dx << ", " << dy << ", " << dz << endl;
            ds = pow(dx * dx + dy * dy + dz * dz, 0.5);
            std::cout << "ds: " << ds << endl;
            px = dx / ds;
            py = dy / ds;
            pz = dz / ds;
            std::cout << "px, py, pz: " << px << ", " << py << ", " << pz << endl;
            std::cout << "dirX, dirY, DirZ: " << dirX << ", " << dirY << ", " << dirZ << endl;

            cosphi = 1.0;
            phi = 0.0;
            //cout << "angle direction: " << dx << " " << dy << " " << dz << " = " << ds << endl;
            cosphi = px * dirX + py * dirY + pz * dirZ;
            //cout << "cosphi: " << cosphi << endl;
            phi = acos(cosphi);

            if (phi > phimax) phimax = phi;
            if (phi < phimin) phimin = phi;

            phideg = phi / (TMath::Pi() / 180);
            std::cout << "phi, phideg: " << phi << ", " << phideg << endl;
            std::cout << "vs. Zenith: " << fVtxGeo->GetZenith(idigit) << endl;
            refbin = angularDist.FindBin(phideg);
            weight = angularDist.GetBinContent(refbin)/coef;
            P = digitCharge / allCharge;
            //cout << "conefomlnl P: " << P << ", weight: " << weight << endl;
            chi2 += pow(P - weight, 2)/weight;

            //outerCone = -outhits/inhits;
        }
    }
    //chi2 = (100 - chi2) * exp(-pow(pow(0.7330382, 2) - pow(phimax - phimin, 2), 2) / pow(0.7330382, 2));
}




//Given the position of the point vertex (x, y, z) and n digits, calculate the mean expected vertex time
double FoMCalculator::FindSimpleTimeProperties(double myConeEdge) {
	double meanTime = 0.0;
	// weighted average
	if(fMeanTimeCalculatorType == 0) {
    // calculate mean and rms of hits inside cone
    // ==========================================
    double Swx = 0.0;
    double Sw = 0.0;
    
    double delta = 0.0;
    double sigma = 0.0;
    double weight = 0.0;
    double deweight = 0.0;
    double deltaAngle = 0.0;
    
    double myConeEdgeSigma = 7.0;  // [degrees]
    
    for( int idigit=0; idigit<this->fVtxGeo->GetNDigits(); idigit++ ){
      int detType = this->fVtxGeo->GetDigitType(idigit); 
      if( this->fVtxGeo->IsFiltered(idigit) ){
        delta = this->fVtxGeo->GetDelta(idigit);    
        sigma = this->fVtxGeo->GetDeltaSigma(idigit);
        weight = 1.0/(sigma*sigma); 
        // profile in angle
        deltaAngle = this->fVtxGeo->GetAngle(idigit) - myConeEdge;      
        // deweight hits outside cone
        if( deltaAngle<=0.0 ){
          deweight = 1.0;
        }
        else{
          deweight = 1.0/(1.0+(deltaAngle*deltaAngle)/(myConeEdgeSigma*myConeEdgeSigma));
        }
        Swx += deweight*weight*delta; //delta is expected vertex time 
        Sw  += deweight*weight;
      }
    }
    if( Sw>0.0 ){
      meanTime = Swx*1.0/Sw;
    }
	}
	
	// most probable time
	else if(fMeanTimeCalculatorType == 1) {
		double sigma = 0.0;
		double deltaAngle = 0.0;
		double weight = 0.0;
		double deweight = 0.0;
    double myConeEdgeSigma = 7.0;  // [degrees]
		vector<double> deltaTime1;
		vector<double> deltaTime2;
		vector<double> TimeWeight;
		
		for( int idigit=0; idigit<fVtxGeo->GetNDigits(); idigit++ ){
      if(fVtxGeo->IsFiltered(idigit)){
        deltaTime1.push_back(fVtxGeo->GetDelta(idigit));  
        deltaTime2.push_back(fVtxGeo->GetDelta(idigit));   
        sigma = fVtxGeo->GetDeltaSigma(idigit);
        weight = 1.0/(sigma*sigma); 
        deltaAngle = fVtxGeo->GetAngle(idigit) - myConeEdge;
        if( deltaAngle<=0.0 ){
          deweight = 1.0;
        }
        else{
          deweight = 1.0/(1.0+(deltaAngle*deltaAngle)/(myConeEdgeSigma*myConeEdgeSigma));
        }
        TimeWeight.push_back(deweight*weight);
      }
    }
    int n = deltaTime1.size();
    std::sort(deltaTime1.begin(),deltaTime1.end());
    double timeMin = deltaTime1.at(int((n-1)*0.05)); // 5% of the total entries
    double timeMax = deltaTime1.at(int((n-1)*0.90)); // 90% of the total entries
    int nbins = int(n/5);
    TH1D *hDeltaTime = new TH1D("hDeltaTime", "hDeltaTime", nbins, timeMin, timeMax);
    for(int i=0; i<n; i++) {
      //hDeltaTime->Fill(deltaTime2.at(i), TimeWeight.at(i));	
      hDeltaTime->Fill(deltaTime2.at(i));	
    }
    meanTime = hDeltaTime->GetBinCenter(hDeltaTime->GetMaximumBin());
    delete hDeltaTime; hDeltaTime = 0;
	}
	
	else std::cout<<"FoMCalculator Error: Wrong type of Mean time calculator! "<<std::endl;
  
  return meanTime; //return expected vertex time
}

void FoMCalculator::PointPositionChi2(double vtxX, double vtxY, double vtxZ, double vtxTime, double& fom)
{  
  // figure of merit
  // ===============
  double vtxFOM = -9999.;
  
  // calculate residuals
  // ===================
  this->fVtxGeo->CalcPointResiduals(vtxX, vtxY, vtxZ, 0.0, 0.0, 0.0, 0.0); //calculate expected point vertex time for each digit

  // calculate figure of merit
  // =========================
  this->TimePropertiesLnL(vtxTime, vtxFOM);

  // calculate overall figure of merit
  // =================================
  fom = vtxFOM;
  // truncate
  if( fom<-9999. ) fom = -9999.;

  return;
}





void FoMCalculator::PointDirectionChi2(double vtxX, double vtxY, 
	                                       double vtxZ, double dirX, double dirY, double dirZ, 
	                                       double coneAngle, double& fom)
{  
  // figure of merit
  // ===============
  double coneFOM = -9999.;
  
  // calculate residuals
  // ===================
  this->fVtxGeo->CalcPointResiduals(vtxX, vtxY, vtxZ, 0.0, 
                                 dirX, dirY, dirZ); //load expected vertex time for each digit

  // calculate figure of merit
  // =========================
  this->ConePropertiesFoM(coneAngle, coneFOM);

  // calculate overall figure of merit
  // =================================
  fom = coneFOM;

  // truncate
  if( fom<-9999. ) fom = -9999.;

  return;
}

void FoMCalculator::PointVertexChi2(double vtxX, double vtxY, double vtxZ, 
	                                    double dirX, double dirY, double dirZ,
	                                    double coneAngle, double vtxTime, double& fom)
{  
  // figure of merit
  // ===============
  double vtxFOM = -9999.;

  // calculate residuals
  // ===================
  this->fVtxGeo->CalcPointResiduals(vtxX, vtxY, vtxZ, 0.0, 
                                 dirX, dirY, dirZ); //calculate expected vertex time for each digit
  // calculate figure of merit
  // =========================
  double timeFOM = -9999.;
  double coneFOM = -9999.;
  
  this->ConePropertiesFoM(coneAngle,coneFOM);
  this->TimePropertiesLnL(vtxTime, timeFOM);
  
  double fTimeFitWeight = this->fTimeFitWeight;
  double fConeFitWeight = this->fConeFitWeight;
  vtxFOM = (fTimeFitWeight*timeFOM+fConeFitWeight*coneFOM)/(fTimeFitWeight+fConeFitWeight);

  // calculate overall figure of merit
  // =================================
  fom = vtxFOM;
  // truncate
  if( fom<-9999. ) fom = -9999.;

  return;
}

void FoMCalculator::ExtendedVertexChi2(double vtxX, double vtxY, double vtxZ, double dirX, double dirY, double dirZ, double coneAngle, double vtxTime, double& fom)
{  
  // figure of merit
  // ===============
  double vtxFOM = -9999.;
  double timeFOM = -9999.;
  double coneFOM = -9999.;

  // calculate residuals
  // ===================
  this->fVtxGeo->CalcExtendedResiduals(vtxX,vtxY,vtxZ,0.0,dirX,dirY,dirZ);
  
  // calculate figure of merit
  // =========================

  this->ConePropertiesFoM(coneAngle,coneFOM);
  this->TimePropertiesLnL(vtxTime, timeFOM);
  
  double fTimeFitWeight = this->fTimeFitWeight;
  double fConeFitWeight = this->fConeFitWeight;
  vtxFOM = (fTimeFitWeight*timeFOM+fConeFitWeight*coneFOM)/(fTimeFitWeight+fConeFitWeight);

  // calculate overall figure of merit
  // =================================
  fom = vtxFOM;

  // truncate
  if( fom<-9999. ) fom = -9999.;

  return;
}

void FoMCalculator::ExtendedVertexChi2(double vtxX, double vtxY, double vtxZ, double dirX, double dirY, double dirZ, double coneAngle, double vtxTime, double& fom, TH1D pdf)
{
	// figure of merit
	// ===============
	double vtxFOM = -9999.;
	double timeFOM = -9999.;
	double coneFOM = -9999.;
    double phimax, phimin;

	// calculate residuals
	// ===================
	this->fVtxGeo->CalcExtendedResiduals(vtxX, vtxY, vtxZ, 0.0, dirX, dirY, dirZ);

	// calculate figure of merit
	// =========================

    this->ConePropertiesLnL(vtxX, vtxY, vtxZ, dirX, dirY, dirZ, coneAngle, coneFOM, pdf, phimax, phimin);
	this->TimePropertiesLnL(vtxTime, timeFOM);

	double fTimeFitWeight = this->fTimeFitWeight;
	double fConeFitWeight = this->fConeFitWeight;
	vtxFOM = (fTimeFitWeight*timeFOM + fConeFitWeight * coneFOM) / (fTimeFitWeight + fConeFitWeight);

	// calculate overall figure of merit
	// =================================
	fom = vtxFOM;

	// truncate
	if (fom < -9999.) fom = -9999.;

	return;
}



//KEPT FOR HISTORY, BUT FITTER IS CURRENTLY NOT WORKING
//void FoMCalculator::CorrectedVertexChi2(double vtxX, double vtxY, double vtxZ, double dirX, double dirY, double dirZ, double& vtxAngle, double& vtxTime, double& fom)
//{  
//  // figure of merit
//  // ===============
//  double vtxFOM = 0.0;
//  double timeFOM = 0.0;
//  double coneFOM = 0.0;
//  double penaltyFOM = 0.0;
//  double fixPositionFOM = 0.0;
//  double fixDirectionFOM = 0.0;
//
//  // calculate residuals
//  // ===================
//  this->fVtxGeo->CalcExtendedResiduals(vtxX,vtxY,vtxZ,0.0,dirX,dirY,dirZ);
//  
//  // calculate figure of merit
//  // =========================
//
//  this->ConePropertiesFoM(vtxAngle,coneFOM);
//  fom = coneFOM;
//
//  // truncate
//  if( fom<-999.999*fBaseFOM ) fom = -999.999*fBaseFOM;
//
//  return;
//}
//
//void FoMCalculator::ConePropertiesLnL(double coneParam0, double coneParam1, double coneParam2, double& coneAngle, double& coneFOM)
//{  
//  
//  // nuisance parameters
//  // ===================
//  double alpha  = coneParam0; // track length parameter = 0.25
//  double alpha0 = coneParam1; // track length parameter = 0.5
//  double beta   = coneParam2; // particle ID:  0.0[electron]->1.0[muon] = 0.75
//
//  // internal variables
//  // ==================
//  double deltaAngle = 0.0; //
//  double sigmaAngle = 7.0; //Cherenkov Angle resolution
//  double Angle0 = Parameters::CherenkovAngle(); //Cherenkov Angle: 42 degree
//  double deltaAngle0 = Angle0*alpha0; //?
//  
//  double digitQ = 0.0;
//  double sigmaQmin = 1.0;
//  double sigmaQmax = 10.0;
//  double sigmaQ = 0.0;
//
//  double A = 0.0;
//  
//  double PconeA = 0.0;
//  double PconeB = 0.0;
//  double Pmu = 0.0;
//  double Pel = 0.0;
//
//  double Pcharge = 0.0;
//  double Pangle = 0.0;
//  double P = 0.0;
//
//  double chi2 = 0.0;
//  double ndof = 0.0;
//
//  double angle = 46.0;
//  double fom = 0.0;
//
//  // hard-coded parameters: 200 kton (100 kton)
//  // ==========================================
//  double lambdaMuShort = 0.5; //  0.5;
//  double lambdaMuLong  = 5.0; // 15.0;
//  double alphaMu =       1.0; //  4.5;
//
//  double lambdaElShort = 1.0; //  2.5;
//  double lambdaElLong =  7.5; // 15.0;
//  double alphaEl =       6.0; //  3.5;
//
//  // numerical integrals
//  // ===================
//  fSconeA = 21.9938;  
//  fSconeB =  0.0000;
//  
//  // Number of P.E. angular distribution
//  // inside cone
//  int nbinsInside = 420; //divide the angle range by 420 bins
//  for( int n=0; n<nbinsInside; n++ ){
//    deltaAngle = -Angle0 + (n+0.5)*(Angle0/(double)nbinsInside); // angle axis
//    fSconeB += 1.4944765*sin( (Angle0+deltaAngle)*(TMath::Pi()/180.0) )
//                           *( 1.0/(1.0+(deltaAngle*deltaAngle)/(deltaAngle0*deltaAngle0)) )
//                           *( Angle0/(double)nbinsInside );
//  }
//
//  // outside cone
//  if( fIntegralsDone == 0 ){ // default 0
//    fSmu = 0.0;
//    fSel = 0.0;
//
//    int nbinsOutside = 1380;
//    for( int n=0; n<nbinsOutside; n++ ){
//      deltaAngle = 0.0 + (n+0.5)*(138.0/(double)nbinsOutside);
//
//      fSmu += 1.4944765*sin( (Angle0+deltaAngle)*(TMath::Pi()/180.0) )
//                          *( 1.0/(1.0+alphaMu*(lambdaMuShort/lambdaMuLong)) )*( 1.0/(1.0+(deltaAngle*deltaAngle)/(lambdaMuShort*lambdaMuShort)) 
//	  			            + alphaMu*(lambdaMuShort/lambdaMuLong)/(1.0+(deltaAngle*deltaAngle)/(lambdaMuLong*lambdaMuLong)) )
//                          *( 138.0/(double)nbinsOutside );
//
//      fSel += 1.4944765*sin( (Angle0+deltaAngle)*(TMath::Pi()/180.0) )
//                          *( 1.0/(1.0+alphaEl*(lambdaElShort/lambdaElLong)) )*( 1.0/(1.0+(deltaAngle*deltaAngle)/(lambdaElShort*lambdaElShort)) 
//				          + alphaEl*(lambdaElShort/lambdaElLong)/(1.0+(deltaAngle*deltaAngle)/(lambdaElLong*lambdaElLong)) )
//                          *( 138.0/(double)nbinsOutside );
//    }
//
//    fIntegralsDone = 1;
//  }
//
//  // loop over digits
//  // ================
//  for( int idigit=0; idigit<this->fVtxGeo->GetNDigits(); idigit++ ){
//
//    if( this->fVtxGeo->IsFiltered(idigit) ){
//      digitQ = this->fVtxGeo->GetDigitQ(idigit);
//      deltaAngle = this->fVtxGeo->GetAngle(idigit) - Angle0;
//
//      // pulse height distribution
//      // =========================
//      if( deltaAngle<=0 ){ //inside Cone
//	      sigmaQ = sigmaQmax;
//      }
//      else{ //outside Cone
//        sigmaQ = sigmaQmin + (sigmaQmax-sigmaQmin)/(1.0+(deltaAngle*deltaAngle)/(sigmaAngle*sigmaAngle));
//      }
//
//      A = 1.0/(log(2.0)+0.5*TMath::Pi()*sigmaQ);
//
//      if( digitQ<1.0 ){
//        Pcharge = 2.0*A*digitQ/(1.0+digitQ*digitQ);
//      }
//      else{
//        Pcharge = A/(1.0+(digitQ-1.0)*(digitQ-1.0)/(sigmaQ*sigmaQ));
//      }
//
//      // angular distribution
//      // ====================
//      A = 1.0/( alpha*fSconeA + (1.0-alpha)*fSconeB
//               + beta*fSmu + (1.0-beta)*fSel ); // numerical integrals
//
//      if( deltaAngle<=0 ){
//
//        // pdfs inside cone:
//        PconeA = 1.4944765*sin( (Angle0+deltaAngle)*(TMath::Pi()/180.0) );
//        PconeB = 1.4944765*sin( (Angle0+deltaAngle)*(TMath::Pi()/180.0) )
//                          *( 1.0/(1.0+(deltaAngle*deltaAngle)/(deltaAngle0*deltaAngle0)) );
//
//        Pangle = A*( alpha*PconeA+(1.0-alpha)*PconeB );
//      }		
//      else{
//
//        // pdfs outside cone
//        Pmu = 1.4944765*sin( (Angle0+deltaAngle)*(TMath::Pi()/180.0) )
//                       *( 1.0/(1.0+alphaMu*(lambdaMuShort/lambdaMuLong)) )*( 1.0/(1.0+(deltaAngle*deltaAngle)/(lambdaMuShort*lambdaMuShort)) 
//                                          + alphaMu*(lambdaMuShort/lambdaMuLong)/(1.0+(deltaAngle*deltaAngle)/(lambdaMuLong*lambdaMuLong)) );
//
//        Pel = 1.4944765*sin( (Angle0+deltaAngle)*(TMath::Pi()/180.0) )
//                       *( 1.0/(1.0+alphaEl*(lambdaElShort/lambdaElLong)) )*( 1.0/(1.0+(deltaAngle*deltaAngle)/(lambdaElShort*lambdaElShort)) 
//                                          + alphaEl*(lambdaElShort/lambdaElLong)/(1.0+(deltaAngle*deltaAngle)/(lambdaElLong*lambdaElLong)) );
//
//        Pangle = A*( beta*Pmu+(1.0-beta)*Pel );
//      }
//
//      // overall probability
//      // ===================
//      P = Pcharge*Pangle;
//      
//      chi2 += -2.0*log(P);
//      ndof += 1.0;
//    }
//  }
//
//  // calculate figure of merit
//  // =========================   
//  if( ndof>0.0 ){
//    fom = fBaseFOM - 5.0*chi2/ndof;
//    angle = beta*43.0 + (1.0-beta)*49.0;
//  }
//
//  // return figure of merit
//  // ======================
//  coneAngle = angle;
//  coneFOM = fom;
//
//  return;
//}
