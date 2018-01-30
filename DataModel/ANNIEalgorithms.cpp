#include "ANNIEalgorithms.h"

double FindPulseMax(std::vector<double> *theWav, double &themax, int &maxbin, double &themin, int &minbin){

  std::cout<<"Finding Pulse Max"<<std::endl;

  themax=0;
  maxbin=0;

  int tracesize = theWav->size();
  for(int i=0; i<tracesize; i++){

    if(themax<theWav->at(i)){
      themax = theWav->at(i);
      maxbin=i;
    }

    if(themin>theWav->at(i)){
      themin = theWav->at(i);
      minbin=i;
    }


  }

  return themax;

}
