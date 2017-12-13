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
