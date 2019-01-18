#include "ANNIERecoObjectTable.h"
#include <iostream>
#include <cassert>

//////////////////////////////////////////////////////////////
// This class is adapted from the WCSimRecoObjectTable class
// in Andy Blake's WCSimAnalysis Package
//////////////////////////////////////////////////////////////


static ANNIERecoObjectTable* fgRecoObjectTable = 0;

ANNIERecoObjectTable* ANNIERecoObjectTable::Instance()
{
  if( !fgRecoObjectTable ){
    fgRecoObjectTable = new ANNIERecoObjectTable();
  }

  if( !fgRecoObjectTable ){
    assert(fgRecoObjectTable);
  }

  if( fgRecoObjectTable ){

  }

  return fgRecoObjectTable;
}

ANNIERecoObjectTable::ANNIERecoObjectTable()
{
  this->Reset();
}

ANNIERecoObjectTable::~ANNIERecoObjectTable()
{

}

void ANNIERecoObjectTable::Reset()
{
  numDigits = 0;
  numClusters = 0;
  numClusterDigits = 0;
  numVertices = 0;
  numRings = 0;
  numEvents = 0;
}

void ANNIERecoObjectTable::Print()
{
  std::cout << " *** ANNIERecoObjectTable::Print() *** " << std::endl;
  std::cout << numDigits << "\t Digits " << std::endl;
  std::cout << numClusterDigits << "\t ClusterDigits " << std::endl;
  std::cout << numClusters << "\t Clusters " << std::endl;
  std::cout << numVertices << "\t Vertices " << std::endl;
  std::cout << numRings << "\t Rings " << std::endl;
  std::cout << numEvents << "\t Events " << std::endl;
}
