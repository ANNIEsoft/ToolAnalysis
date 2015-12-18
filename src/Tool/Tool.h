#ifndef TOOL_H
#define TOOL_H

#include <string>

#include "Store.h"
#include "DataModel.h"

class Tool{
  
 public:
  
  virtual bool Initialise(std::string configfile,DataModel &data)=0;
  virtual bool Execute()=0;
  virtual bool Finalise()=0;
  
 protected:
  
  Store m_variables;
  DataModel *m_data;
  
 private:
  
  
  
  
  
};

#endif
