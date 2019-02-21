#include "RecoClusterDigit.h"

#include "ANNIERecoObjectTable.h"

RecoClusterDigit::RecoClusterDigit(RecoDigit* recoDigit)
{
  serialise=true;
  fIsClustered = 0;
  fIsAllClustered = 0;

  fRecoDigit = recoDigit;

  fClusterDigitList = new std::vector<RecoClusterDigit*>;

  ANNIERecoObjectTable::Instance()->NewClusterDigit();
}

RecoClusterDigit::~RecoClusterDigit()
{
  delete fClusterDigitList;

  ANNIERecoObjectTable::Instance()->DeleteClusterDigit();
}

void RecoClusterDigit::AddClusterDigit(RecoClusterDigit* clusterDigit)
{ 
  fClusterDigitList->push_back(clusterDigit); 
}

int RecoClusterDigit::GetNClusterDigits() 
{ 
  return fClusterDigitList->size(); 
}
  
RecoClusterDigit* RecoClusterDigit::GetClusterDigit(int idigit) 
{ 
  return (RecoClusterDigit*)(fClusterDigitList->at(idigit)); 
}
  
std::vector<RecoClusterDigit*>* RecoClusterDigit::GetClusterDigitList()
{ 
  return fClusterDigitList; 
}

bool RecoClusterDigit::IsAllClustered()
{
  if( fIsAllClustered==0 ){
    fIsAllClustered = 1;
    for( int n=0; n<fClusterDigitList->size(); n++ ){
      RecoClusterDigit* clusterDigit = (RecoClusterDigit*)(fClusterDigitList->at(n));
      if( clusterDigit->IsClustered()==0 ) fIsAllClustered = 0;
    }
  }

  return fIsAllClustered;
}
