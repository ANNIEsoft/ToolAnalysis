#ifndef DATAMODEL_H
#define DATAMODEL_H

#include <map>
#include <string>
#include <vector>
#include <stdlib.h>

//#include "TTree.h"

#include "Store.h"
#include "BoostStore.h"
#include "Logging.h"
#include "LAPPD.h"
#include "ANNIEalgorithms.h"
#include "ANNIEconstants.h"
#include "BeamStatusClass.h"
#include "BeamStatus.h"
#include "ChannelKey.h"
#include "Detector.h"
#include "Direction.h"
#include "Geometry.h"
#include "HeftyInfo.h"
#include "Hit.h"
#include "Particle.h"
#include "LAPPDHit.h"
#include "Position.h"
#include "TimeClass.h"
#include "TriggerClass.h"
#include "Waveform.h"
#include "Parameters.h"
#include "ANNIERecoObjectTable.h"
#include "RecoDigit.h"
#include "RecoVertex.h"
#include "RecoRing.h"


#include <zmq.hpp>

class DataModel {


 public:

  DataModel();
  //TTree* GetTTree(std::string name);
  //void AddTTree(std::string name,TTree *tree);
  //void DeleteTTree(std::string name);

  Store vars;
  BoostStore CStore;
  std::map<std::string,BoostStore*> Stores;

  Logging *Log;
  zmq::context_t* context;


 private:



  //std::map<std::string,TTree*> m_trees;



};



#endif
