#ifndef DATAMODEL_H
#define DATAMODEL_H

#include <map>
#include <string>
#include <vector>

//#include "TTree.h"

#include "Store.h"
#include "Logging.h"

#include <zmq.hpp>

class DataModel {


 public:
  
  DataModel();
  //TTree* GetTTree(std::string name);
  //void AddTTree(std::string name,TTree *tree);

  Store vars;
  Logging *Log;

  zmq::context_t* context;


  //  bool (*Log)(std::string, int);

  /*  
  template<Type T>
    struct Log {
      typedef bool (*type)(T message,int verboselevel);
    };
  */
 private:


  
  //std::map<std::string,TTree*> m_trees; 
  
  
  
};



#endif
