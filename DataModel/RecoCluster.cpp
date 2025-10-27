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
  for(int i=0;i<j;i++){
    delete fDigitList[i];
  }
  fDigitList.clear();
}

static bool CompareTimes(RecoDigit *rd1, RecoDigit *rd2)
{
  return ( rd1->GetCalTime() > rd2->GetCalTime() );
}

void RecoCluster::SortCluster()
{
  sort(fDigitList.begin(), fDigitList.end(), CompareTimes);
}

void RecoCluster::AddDigit(RecoDigit* digit)
{
  fDigitList.push_back(digit);
}

RecoDigit* RecoCluster::GetDigit(int n)
{
  return (RecoDigit*)(fDigitList.at(n));
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

void RecoCluster::CalcTime() {
	double sum = 0;
	for (RecoDigit* i_digit : fDigitList) {
		sum += i_digit->GetCalTime();
	}
	clusterTime = sum / GetNDigits();

}

void RecoCluster::CalcCharge() {
    double sum=0;
    for (RecoDigit* i_digit : fDigitList) {
        sum += i_digit->GetCalCharge();
    }
    clusterCharge = sum;
}

void RecoCluster::CalcCB() {
    //calculate unfiltered CB
    double total_Q = 0;
    double total_QSquared = 0;
    for (int i = 0; i < fDigitList.size(); i++) {
        //if(unfilteredDigits->at(i)->GetDigitType()==RecoDigit::PMT8inch){
        if (fDigitList.at(i)->GetFilterStatus()) {
            double tube_charge = fDigitList.at(i)->GetCalCharge();
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
        for (int i = 0; i < fDigitList.size(); i++) {
            i_position = fDigitList.at(i)->GetPosition();
            for (int j = 0; j < fDigitList.size(); j++) {
                if (j == i)continue;
                j_position = fDigitList.at(j)->GetPosition();

                angle = j_position.Angle(i_position);
                if (angle > max_angle)max_angle = angle;

            }
        }
        AS0 = max_angle;



        double max_charge = 0;
        double max_angle2 = 0;
        int max_index = 0;
        Position max_position;
        for (int i = 0; i < fDigitList.size(); i++) {
            if (fDigitList.at(i)->GetCalCharge() > max_charge) {
                max_charge = fDigitList.at(i)->GetCalCharge();
                max_index = i;
                max_position = fDigitList.at(i)->GetPosition();
            }
        }


        for (int i = 0; i < fDigitList.size(); i++) {
            if (i == max_index)continue;
            i_position = fDigitList.at(i)->GetPosition();
            angle = i_position.Angle(max_position);
            if (angle > max_angle2)max_angle2 = angle;
        }
        AS1 = max_angle2;


        double max_chargeangle = 0;
        double chargeangle;
        double iq,jq;
        for (int i = 0; i < fDigitList.size(); i++) {
            i_position = fDigitList.at(i)->GetPosition();
            iq=fDigitList.at(i)->GetCalCharge();
            for(int j = 0; j < fDigitList.size(); j++) {
                if(j==i)continue;
                j_position = fDigitList.at(j)->GetPosition();
                jq = fDigitList.at(j)->GetCalCharge();
                chargeangle=iq*jq*j_position.Angle(i_position);
                if(chargeangle>max_chargeangle)max_chargeangle=chargeangle;
            }
        }

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
    this->CalcCA();
    this->CalcCV();
    this->CalcAMD();

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
    std::vector<int> DigiParents;
    int parentCandidate;
    int maxIndex=0;
    double digitCharge;
    double maxCharge=0;
    int tempPDG = -5;
    for (RecoDigit* i_digit : fDigitList) {
        cout<<"Digit parent list size: "<<i_digit->GetParents().size()<<endl;
        if (i_digit->GetParents().size() == 0) {
            cout<<"Digit found with no parents!!!\n";
        }
        for(int i=0; i<i_digit->GetParents().size(); i++){
        parentCandidate=i_digit->GetParents().at(i);
        cout<<"ClusterParent candidate ID: "<<parentCandidate<<endl;
        digitCharge=i_digit->GetCalCharge();
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

    /*for (std::pair<int, int> apair : *fMCParticleIndexMap) {
        if (apair.second == maxIndex) {
            tempPDG=apair.first;
        }
    }*/

    //if(tempPDG==-5) return false;

    cout << "Particle " << maxIndex << " wins with charge " << maxCharge << endl;

    bestParticleID=maxIndex;
    //bestParticlePDG=tempPDG;
    purity=maxCharge/clusterCharge;

    return maxIndex;
}

std::vector<RecoDigit*> RecoCluster::convexHull() {

    double tempX, tempY, tempPhi, tempTheta;
    std::vector<double> effX, effY;
    double sumX=0;
    double sumY=0;
    Position pos;
    //std::vector<RecoDigits> hullDigits;
    std::sort(fDigitList.begin(), fDigitList.end());
    for (RecoDigit* adigit : fDigitList) {
        pos = adigit->GetPosition();
        tempX = sin(pos.GetTheta()) * cos(pos.GetPhi());
        tempY = sin(pos.GetTheta()) * sin(pos.GetPhi());
        effX.push_back(tempX);
        effY.push_back(tempY);
        sumX+=tempX;
        sumY+=tempY;
    }

    TwoDCenter.SetX(sumX/fDigitList.size());
    TwoDCenter.SetY(sumY/fDigitList.size());

    TwoDCenter.SetZ(0); //2D projection center; used for circularity test.

    std::vector<int> lower, upper;
    double diffX, diffY;
    double diffprevX, diffprevY;
    double cross;


    // Lower hull
    for (int i = 0; i < fDigitList.size(); i++) {
        if (lower.size() >= 2) {
            diffprevX = effX[lower[lower.size() - 1]] - effX[lower[lower.size() - 2]];
            diffprevY = effY[lower[lower.size() - 1]] - effY[lower[lower.size() - 2]];
            diffX = effX[i] - effX[lower[lower.size() - 1]];
            diffY = effY[i] - effY[lower[lower.size() - 1]];
            cross = diffprevX * diffY - diffprevY * diffX;
        }
        while (lower.size() >= 2 && cross <= 0) {
            //(lower.size() >= 2 && (lower[lower.size() - 1] - lower[lower.size() - 2]).cross(p - lower[lower.size() - 1]) <= 0) {
            lower.pop_back();
            if (lower.size() >= 2) {
                diffprevX = effX[lower[lower.size() - 1]] - effX[lower[lower.size() - 2]];
                diffprevY = effY[lower[lower.size() - 1]] - effY[lower[lower.size() - 2]];
                diffX = effX[i] - effX[lower[lower.size() - 1]];
                diffY = effY[i] - effY[lower[lower.size() - 1]];
                cross = diffprevX * diffY - diffprevY * diffX;
            }

        }
        lower.push_back(i);
        std::cout<<"lowersize,effX,effY,finalcross: "<<lower.size()<<","<<effX[i] <<","<< effY[i]<<","<<cross<<endl;
    }

    // Upper hull
    for (int i = fDigitList.size() - 1; i >= 0; --i) {
        if (upper.size() >= 2) {
            diffprevX = effX[upper[upper.size() - 1]] - effX[upper[upper.size() - 2]];
            diffprevY = effY[upper[upper.size() - 1]] - effY[upper[upper.size() - 2]];
            diffX = effX[i] - effX[upper[upper.size() - 1]];
            diffY = effY[i] - effY[upper[upper.size() - 1]];
            cross = diffprevX * diffY - diffprevY * diffX;
        }
        while (upper.size() >= 2 && cross <= 0) {
            //(upper[upper.size() - 1] - upper[upper.size() - 2]).cross(points[i] - upper[upper.size() - 1]) <= 0) {
            upper.pop_back();
            if (upper.size() >= 2) {
                diffprevX = effX[upper[upper.size() - 1]] - effX[upper[upper.size() - 2]];
                diffprevY = effY[upper[upper.size() - 1]] - effY[upper[upper.size() - 2]];
                diffX = effX[i] - effX[upper[upper.size() - 1]];
                diffY = effY[i] - effY[upper[upper.size() - 1]];
                cross = diffprevX * diffY - diffprevY * diffX;
            }
        }
        upper.push_back(i);
        std::cout << "uppersize,effX,effY,finalcross: " << upper.size() << "," << effX[i] << "," << effY[i] << "," << cross << endl;
    }

    // Remove the last point of each half because it is repeated at the beginning of the other half
    lower.pop_back();
    upper.pop_back();

    // Concatenate lower and upper hull to get the convex hull
    lower.insert(lower.end(), upper.begin(), upper.end());

    for (int i = 0; i < fDigitList.size(); i++) {
        fDigitList.at(i)->ResetFilter();
    }

    for (int i = 0; i < lower.size(); i++) {
        hullDigits.push_back(fDigitList[lower[i]]);
        fDigitList.at(lower[i])->PassFilter();
        std::cout<<"Adding digit index "<<lower[i]<<endl;
    }

    std::cout<<"found hull of size "<<hullDigits.size()<<" compared to "<<fDigitList.size()<<" total digits and lower size: "<<lower.size()<<endl;

    return hullDigits;
}

void RecoCluster::CalcCV() {
    Position ud;
    ChargeVector.SetX(0);
    ChargeVector.SetY(0);
    ChargeVector.SetZ(0);
    for (int i = 0; i < fDigitList.size(); i++) {
        for (int j = i + 1; j < fDigitList.size(); j++) {
            ud=(fDigitList.at(i)->GetPosition() - fDigitList.at(j)->GetPosition()).Unit();
            ChargeVector+=(fDigitList.at(i)->GetCalCharge()*fDigitList.at(j)->GetCalCharge())*ud;
        }
    }
}

void RecoCluster::CalcAMD() {
    double min_distance = 9999;
    double ave_min_distance=0;
    double distance;
    for (int i = 0; i < fDigitList.size(); i++) {
        Position ipos=fDigitList.at(i)->GetPosition();
        for (int j = 0; j < fDigitList.size(); j++) {
            if(j == i) continue;
            Position jpos=fDigitList.at(j)->GetPosition();
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
       Position ipos=fDigitList.at(i)->GetPosition();
       double iq=fDigitList.at(i)->GetCalCharge();
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
void RecoCluster::CalcCA() {        //AW: Angular Width?  1-sigma angular width?
    std::map<double,double> angle_charge_map;
    double angle,charge;
    double contained=0;
    for (RecoDigit* idigit: fDigitList) {
        angle=SpatialAverage.Angle(idigit->GetPosition());
        charge = idigit->GetCalCharge();
        if(angle_charge_map.count(angle)==0)
            angle_charge_map.emplace(angle,charge);
        else
            angle_charge_map.at(angle)+=charge;
    }
    for (std::pair<double,double> apair: angle_charge_map) {
        ContainingAngle=apair.first;
        contained+=apair.second/clusterCharge;
        if(contained>0.68)break;
    }
    return;
}

//Planarity and Sphericity: measure of how planar or spherical cluster is; currently in-process of translating from reference
/*void ComputePlanaritySphericity_ROOT(const std::vector<Digit>& digits,
    double& P, double& S)
{
    P = 0.0; S = 0.0;

    // 1) Charge-weighted centroid
    TVector3 mean(0, 0, 0);
    double wsum = 0.0;
    for (int i = 0; i<fDigitList.size(); i++) {
        const double w = std::max(1e-12, d.charge);
        mean += w * d.pos;
        wsum += w;
    }
    if (wsum <= 0.0) return;
    mean *= (1.0 / wsum);

    // 2) Charge-weighted covariance (symmetric 3x3)
    //    M = (1/wsum) * sum_i w_i * (r_i)(r_i)^T, with r_i = pos_i - mean
    TMatrixDSym cov(3);
    cov.Zero();
    for (const auto& d : digits) {
        const double w = std::max(1e-12, d.charge);
        const TVector3 r = d.pos - mean;
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
}

// -----------------------
// Example usage
// -----------------------
int main() {
    std::vector<Digit> digits;
    digits.push_back({ TVector3(1.0, 0.0, 0.0), 2.0 });
    digits.push_back({ TVector3(0.0, 1.0, 0.1), 1.0 });
    digits.push_back({ TVector3(0.0,-1.0,-0.1), 1.5 });
    digits.push_back({ TVector3(-1.0, 0.0, 0.0), 1.2 });

    double P = 0, S = 0;
    ComputePlanaritySphericity_ROOT(digits, P, S);

    std::cout << "Planarity P  = " << P << "\n";
    std::cout << "Sphericity S = " << S << "\n";
    return 0;
}*/