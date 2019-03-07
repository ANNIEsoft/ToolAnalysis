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



