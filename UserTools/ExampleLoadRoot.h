////////////////////////////////////////////////////////////////
/*
Interestingly there are many differnt ways to load root data into ToolDAQ 
You could:

1: Create a new BoostStore and load every variable into the store one by one and save each entry 
2: Create a new BoostStore and load a class contianing all the variables in one and save for every entry 
3: Create a new BoostStore that holds a Root Makeclass and loop through the entries using that class or even load it into the common stor
4: Same as 1 but sticking the variables in the DataModle
5: same as 3 but sticking the Root MakeClass into the Data model
6: Maually linking branches in your ttree to store variable pointers
7: Using the inbuilt tree store
8: ..... I can think of at least 3 more but you get the point.....

ToolDAQ is all about doing things easily so ill pick what i think is about the easiest which is no.3

*/
///////////////////////////////////////////////////////////

#ifndef ExampleLoadRoot_H
#define ExampleLoadRoot_H

#include <string>
#include <iostream>

#include "Tool.h"
#include "TFile.h"
#include "TTree.h"
#include "ExampleRoot.h"

class ExampleLoadRoot: public Tool {


 public:

  ExampleLoadRoot();
  bool Initialise(std::string configfile,DataModel &data);
  bool Execute();
  bool Finalise();


 private:

  int verbose;
  std::string infile;
  TFile* file;
  TTree* tree;
  ExampleRoot* Data;

  long currententry;
  long NumEvents;

};


#endif

