#include "RecoCluster.h"
#include "ANNIERecoObjectTable.h"
#include <algorithm>

RecoCluster::RecoCluster()
{
  serialise=true;
  ANNIERecoObjectTable::Instance()->NewCluster();
}

RecoCluster::~RecoCluster()
{
  ANNIERecoObjectTable::Instance()->DeleteCluster();
  Reset();
}

void RecoCluster::Reset()
{
  int j=fDigitList.size();
  /*for (int i = 0; i<j; i++) {
    delete fDigitList[i];
  }*/
  fDigitList.clear();
}

static bool CompareTimes(RecoDigit rd1, RecoDigit rd2)
{
  return ( rd1.GetCalTime() > rd2.GetCalTime() );
}

void RecoCluster::SortCluster()
{
  sort(fDigitList.begin(), fDigitList.end(), CompareTimes);
}

void RecoCluster::AddDigit(RecoDigit digit)
{
  //digit.AddCluster(fClusterMode);
    fDigitList.push_back(digit);
}

RecoDigit RecoCluster::GetDigit(int n)
{
  return (RecoDigit)(fDigitList.at(n));
}

int RecoCluster::GetNDigits()
{
  return fDigitList.size();
}

void RecoCluster::SetClusterMode(int cmode)
{
  fClusterMode = cmode;
}

int RecoCluster::GetClusterMode()
{
  return fClusterMode;
}

bool RecoCluster::CheckFilter() {
    map<int,double> ModeDigitMap;
    vector<int> DigitModes;
    double baseline;

    fIsFiltered=true;
    for (int i=0;i<fDigitList.size();i++) {
        RecoDigit adigit=fDigitList.at(i);
        DigitModes=adigit.GetClusteredModes();
        for (int j = 0; j < DigitModes.size(); j++) {
            if(!ModeDigitMap.count(DigitModes.at(j))) ModeDigitMap.emplace(DigitModes.at(j),adigit.GetCalCharge());
            else ModeDigitMap.at(DigitModes.at(j))+=adigit.GetCalCharge();
        }
        
    }
    if(ModeDigitMap.size()==0) return fIsFiltered;
    baseline=ModeDigitMap.at(fClusterMode);
    for (pair<int, double> apair : ModeDigitMap) {
        if (apair.first>=fClusterMode) continue;
        if(apair.second/baseline>0.6) fIsFiltered=false;
    }
    return fIsFiltered;
}

void RecoCluster::CalcTime() {
	double sum = 0;
	for (int i=0; i<fDigitList.size(); i++) {
		sum += fDigitList.at(i).GetCalTime();
	}
	clusterTime = sum / GetNDigits();

}

void RecoCluster::CalcCharge() {
    double sum=0;
    for (int i = 0; i < fDigitList.size(); i++) {
        sum += fDigitList.at(i).GetCalCharge();
    }
    clusterCharge = sum;
}

void RecoCluster::CalcCB() {
    //calculate unfiltered CB
    double total_Q = 0;
    double total_QSquared = 0;
    for (int i = 0; i < fDigitList.size(); i++) {
        //if(unfilteredDigits->at(i)->GetDigitType()==RecoDigit::PMT8inch){
        if (fDigitList.at(i).GetFilterStatus()) {
            double tube_charge = fDigitList.at(i).GetCalCharge();
            total_Q += tube_charge;
            total_QSquared += (tube_charge * tube_charge);
            //}
        }
    }
    //FIXME: Need a method to have the 123 be equal to the number of operating detectors
    double charge_balance = sqrt((total_QSquared) / (total_Q * total_Q) - (1. / 123.));

    clusterCB=charge_balance;
}

//Calculate Angular span
// - AS0 is the maximum angle between any two hit PMTs
// - AS1 is the largest angle between the greatest hit by charge and any of the other hits
// - ASC is a charge-weighted version of AS0
void RecoCluster::CalcAS() {

        double max_angle = 0;
        double angle;
        Position i_position, j_position;
        double max_charge = 0;
        double max_angle2 = 0;
        int max_index = 0;
        Position max_position;
        double max_chargeangle = 0;
        double chargeangle;
        double iq, jq;

        for (int i = 0; i < fDigitList.size(); i++) {
            i_position = fDigitList.at(i).GetPosition();
            for (int j = 0; j < fDigitList.size(); j++) {
                if (j == i)continue;
                j_position = fDigitList.at(j).GetPosition();

                angle = j_position.Angle(i_position);
                if (angle > max_angle)max_angle = angle;

            }
            
            if (fDigitList.at(i).GetCalCharge() > max_charge) {
                max_charge = fDigitList.at(i).GetCalCharge();
                max_index = i;
                max_position = fDigitList.at(i).GetPosition();
            }

            iq = fDigitList.at(i).GetCalCharge();
            for (int j = 0; j < fDigitList.size(); j++) {
                if (j == i)continue;
                j_position = fDigitList.at(j).GetPosition();
                jq = fDigitList.at(j).GetCalCharge();
                chargeangle = iq * jq * j_position.Angle(i_position);
                if (chargeangle > max_chargeangle)max_chargeangle = chargeangle;
            }
        }
        AS0 = max_angle;

        for (int i = 0; i < fDigitList.size(); i++) {
            if (i == max_index)continue;
            i_position = fDigitList.at(i).GetPosition();
            angle = i_position.Angle(max_position);
            if (angle > max_angle2)max_angle2 = angle;
        }
        AS1 = max_angle2;

        ASC=max_chargeangle;

}

//Retrieve Angular Span value
double RecoCluster::GetAS(int mode) {
    if(mode==0)return AS0;
    else if(mode==1)return AS1;
    else return 0;
}

void RecoCluster::CalcParameters() {
    this->CalcTime();
    this->CalcCharge();
    this->CalcCB();
    this->CalcAS();
    this->CalcSA();
    this->CalcAW();
    this->CalcCV();
    this->CalcAMD();
    this->CalcPlanaritySphericity();
    this->CalcTR();

    //Other needed parameters here
}

void RecoCluster::SetParticle(int pID, int pPDG, double eff, double pur) {
    bestParticleID=pID;
    bestParticlePDG=pPDG;
    efficiency=eff;
    purity=pur;
}

int RecoCluster::calcBestParent() {
    std::map<int,double> ParticletoClusterCharge;
    int parentCandidate;
    int maxIndex=0;
    double digitCharge;
    double maxCharge=0;
    int tempPDG = -5;
    for (int i = 0; i < fDigitList.size(); i++) {
        RecoDigit i_digit=fDigitList.at(i);
        for(int j=0; j<i_digit.GetParents().size(); j++){
        parentCandidate=i_digit.GetParents().at(j);
        digitCharge=i_digit.GetCalCharge();
        if (ParticletoClusterCharge.count(parentCandidate) == 0) {
            ParticletoClusterCharge.emplace(parentCandidate,digitCharge);
        }
        else {
            ParticletoClusterCharge.at(parentCandidate)+=digitCharge;
        }
        }
    }
    for (std::pair<int, double>apair : ParticletoClusterCharge) {
        if (apair.second > maxCharge) {
            maxCharge=apair.second;
            maxIndex=apair.first;
            
        }
    }

    //cout << "Particle " << maxIndex << " wins with charge " << maxCharge << endl;

    bestParticleID=maxIndex;
    purity=maxCharge/clusterCharge;

    return maxIndex;
}

void RecoCluster::CalcCV() {
    Position ud;
    ChargeVector.SetX(0);
    ChargeVector.SetY(0);
    ChargeVector.SetZ(0);
    for (int i = 0; i < fDigitList.size(); i++) {
        for (int j = i + 1; j < fDigitList.size(); j++) {
            ud=(fDigitList.at(i).GetPosition() - fDigitList.at(j).GetPosition()).Unit();
            ChargeVector+=(fDigitList.at(i).GetCalCharge()*fDigitList.at(j).GetCalCharge())*ud;
        }
    }
}

void RecoCluster::CalcAMD() {
    double min_distance = 9999;
    double ave_min_distance=0;
    double distance;
    for (int i = 0; i < fDigitList.size(); i++) {
        Position ipos=fDigitList.at(i).GetPosition();
        for (int j = 0; j < fDigitList.size(); j++) {
            if(j == i) continue;
            Position jpos=fDigitList.at(j).GetPosition();
            distance = (jpos-ipos).Mag();
            if(distance<min_distance) min_distance=distance;

        }
        ave_min_distance+=min_distance;
        min_distance=9999;
    }
    ave_min_distance/=fDigitList.size();
    AMD=ave_min_distance;
}

//spatial average: Charge-averaged location of all 
void RecoCluster::CalcSA() {
   double aveX=0;
   double aveY=0;
   double aveZ=0;
   for (int i = 0; i < fDigitList.size(); i++) {
       Position ipos=fDigitList.at(i).GetPosition();
       double iq=fDigitList.at(i).GetCalCharge();
       aveX += ipos.X() * iq;
       aveY += ipos.Y() * iq;
       aveZ += ipos.Z() * iq;
   }
   aveX/=clusterCharge;
   aveY/=clusterCharge;
   aveZ/=clusterCharge;
   SpatialAverage=Position(aveX,aveY,aveZ);
}

//Containing Angle: angle from SpatialAverage unit vector which contains 68% of the total charge of the cluster
void RecoCluster::CalcAW() {        //AW: Angular Width?  1-sigma angular width?
    std::map<double,double> angle_charge_map;
    double angle,charge;
    double contained=0;
    for (int i = 0; i < fDigitList.size(); i++) {
        RecoDigit idigit=fDigitList.at(i);
        angle=SpatialAverage.Angle(idigit.GetPosition());
        charge = idigit.GetCalCharge();
        if(angle_charge_map.count(angle)==0)
            angle_charge_map.emplace(angle,charge);
        else
            angle_charge_map.at(angle)+=charge;
    }
    for (std::pair<double,double> apair: angle_charge_map) {
        AngularWidth=apair.first;
        contained+=apair.second/clusterCharge;
        if(contained>0.68)break;
    }
    return;
}

//Planarity and Sphericity: measure of how planar or spherical cluster is; currently in-process of translating from reference
void RecoCluster::CalcPlanaritySphericity()
{
    double P = 0.0; double S = 0.0;

    // 1) Charge-weighted centroid
    Position mean(0, 0, 0);
    double wsum = 0.0;
    for (int i = 0; i<fDigitList.size(); i++) {
        double w = fDigitList.at(i).GetCalCharge();
        mean += w * fDigitList.at(i).GetPosition();
        wsum += w;
    }
    if (wsum <= 0.0) return;
    mean *= (1.0 / wsum);

    // 2) Charge-weighted covariance (symmetric 3x3)
    //    M = (1/wsum) * sum_i w_i * (r_i)(r_i)^T, with r_i = pos_i - mean
    TMatrixDSym cov(3);
    cov.Zero();
    for (int i = 0; i < fDigitList.size(); i++) {
        RecoDigit d = fDigitList.at(i);
        double w = d.GetCalCharge();
        Position r = d.GetPosition() - mean;
        // Outer product r*r^T scaled by w
        cov(0, 0) += w * r.X() * r.X();
        cov(0, 1) += w * r.X() * r.Y();
        cov(0, 2) += w * r.X() * r.Z();
        cov(1, 1) += w * r.Y() * r.Y();
        cov(1, 2) += w * r.Y() * r.Z();
        cov(2, 2) += w * r.Z() * r.Z();
    }
    cov(1, 0) = cov(0, 1);
    cov(2, 0) = cov(0, 2);
    cov(2, 1) = cov(1, 2);
    cov *= (1.0 / wsum);

    // 3) Eigen-decomposition (symmetric → real eigen system)
    TMatrixDSymEigen eig(cov);
    TVectorD evals = eig.GetEigenValues(); // typically ascending, but we’ll sort to be safe

    // Copy to std::vector and sort descending
    std::vector<double> lambda = { evals[0], evals[1], evals[2] };
    std::sort(lambda.begin(), lambda.end(), std::greater<double>());

    const double lambda1 = lambda[0]; // largest
    const double lambda2 = lambda[1];
    const double lambda3 = lambda[2]; // smallest
    const double sum = lambda1 + lambda2 + lambda3;

    if (sum <= 0.0) return;

    // 4) Planarity and Sphericity
    //    P = (lambda2 - lambda3) / (lambda1 + lambda2 + lambda3)
    //    S = (3/2) * lambda3 / (lambda1 + lambda2 + lambda3)
    P = (lambda2 - lambda3) / sum;
    S = 1.5 * (lambda3 / sum);

    Planarity=P;
    Sphericity=S;
}

double RecoCluster::CalcBeta(int order) {
    double Beta=0;
    Position ipos,jpos;
    double angle;
    int NDigits=fDigitList.size();
    TF1 LegendreFunction("legendrefunction","ROOT::Math::legendre([0],x)",-1,1);
    LegendreFunction.SetParameter(0,order);
    for (int i = 0; i < fDigitList.size(); i++) {
        for (int j = i + 1; j < fDigitList.size(); j++) {
            ipos=fDigitList.at(i).GetPosition();
            jpos=fDigitList.at(j).GetPosition();
            angle=ipos.Angle(jpos);
            Beta+=2 * LegendreFunction.Eval(cos(angle)) / (NDigits*(NDigits-1));
        }
    }

    if(BetaParameters.count(order)) BetaParameters.at(order)=Beta;
    else BetaParameters.emplace(order, Beta);

    return Beta;
}

double RecoCluster::GetBeta(int order) {
    if(BetaParameters.count(order))return BetaParameters.at(order);
    else return -999;
}

void RecoCluster::CalcTR() {
    double firsttime=fDigitList.at(0).GetCalTime();
    double lasttime=fDigitList.at(fDigitList.size()-1).GetCalTime();
    double maxtime=fDigitList.at(0).GetCalTime();
    double maxcharge=fDigitList.at(0).GetCalCharge();
    
    for (int i = 0; i < fDigitList.size(); i++) {
        RecoDigit adigit=fDigitList.at(i);
        if(adigit.GetCalTime()<firsttime)firsttime=adigit.GetCalTime();
        if(adigit.GetCalTime()>lasttime)lasttime=adigit.GetCalTime();
        if (adigit.GetCalCharge() > maxcharge) {
            maxtime=adigit.GetCalTime();
            maxcharge=adigit.GetCalCharge();
        }
    }

    TRTotal=lasttime-firsttime;
    if(clusterTime-firsttime>lasttime-clusterTime)TRCluster=clusterTime-firsttime;
    else TRCluster=lasttime-clusterTime;
    if(maxtime-firsttime>lasttime-maxtime)TRQ=maxtime-firsttime;
    else TRQ=lasttime-maxtime;

    TRRatioTQ=TRTotal/TRQ;
    TRRatioTC=TRTotal/TRCluster;
    TRRatioQC=TRQ/TRCluster;
}

/*void RecoCluster::CleanDigits() {
    for (int i = 0; i < fDigitList.size(); i++) {
        if (fDigitList.at(i) != nullptr) {
            delete fDigitList.at(i);
            fDigitList.at(i)=nullptr;
        }
    }
}*/