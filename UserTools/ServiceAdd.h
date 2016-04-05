#ifndef ServiceAdd_H
#define ServiceAdd_H

#include <string>
#include <iostream>

#include "Tool.h"

#include <boost/uuid/uuid.hpp>            // uuid class                                                                     
#include <boost/uuid/uuid_generators.hpp> // generators                                                                     
#include <boost/uuid/uuid_io.hpp>         // streaming operators etc.        

class ServiceAdd: public Tool {


 public:

  ServiceAdd();
  bool Initialise(std::string configfile,DataModel &data);
  bool Execute();
  bool Finalise();


 private:





};


#endif
