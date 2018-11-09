#ifndef ANNIERECOOBJECTTABLE_H
#define ANNIERECOOBJECTTABLE_H

#include "TObject.h"

class ANNIERecoObjectTable{

 public:
  static ANNIERecoObjectTable* Instance();

  void NewDigit(){ numDigits++; }
  void DeleteDigit(){ numDigits--; }
  Int_t NumberOfDigits(){ return numDigits; }

  void NewCluster() { numClusters++; }
  void DeleteCluster(){ numClusters--; }
  Int_t NumberOfClusters(){ return numClusters; }

  void NewClusterDigit(){ numClusterDigits++; }
  void DeleteClusterDigit(){ numClusterDigits--; }
  Int_t NumberOfClusterDigits(){ return numClusterDigits; }

  void NewVertex(){ numVertices++; }
  void DeleteVertex(){ numVertices--; }
  Int_t NumberOfVertices(){ return numVertices; }

  void NewRing(){ numRings++; }
  void DeleteRing(){ numRings--; }
  Int_t NumberOfRings(){ return numRings; }

  void NewEvent(){ numEvents++; }
  void DeleteEvent(){ numEvents--; }
  Int_t NumberOfEvents(){ return numEvents; }

  void Reset();
  void Print();

 private:
  ANNIERecoObjectTable();
  ~ANNIERecoObjectTable();

  Int_t numDigits;
  Int_t numClusters;
  Int_t numClusterDigits;
  Int_t numVertices;
  Int_t numRings;
  Int_t numEvents;

};

#endif







