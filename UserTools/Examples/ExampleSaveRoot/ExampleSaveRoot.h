#ifndef ExampleSaveRoot_H
#define ExampleSaveRoot_H

#include <string>
#include <iostream>
#include <TFile.h>
#include <TTree.h>

#include "Tool.h"

class ExampleSaveRoot: public Tool {


 public:

  ExampleSaveRoot();
  bool Initialise(std::string configfile,DataModel &data);
  bool Execute();
  bool Finalise();


 private:

  TFile* file;
  TTree* tree;

  int a;
  double b;
  std::string c;

  int verbose;
  std::string outfile;



};


#endif
