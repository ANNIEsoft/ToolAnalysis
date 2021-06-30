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

std::string GetStdoutFromCommand(std::string cmd){
	/*
	  credit: Jeremy Morgan, source:
	  https://www.jeremymorgan.com/tutorials/c-programming/how-to-capture-the-output-of-a-linux-command-in-c/
	*/
	std::string data;
	FILE * stream;
	const int max_buffer = 256;
	char buffer[max_buffer];
	cmd.append(" 2>&1");
	
	stream = popen(cmd.c_str(), "r");
	if(stream){
		while(!feof(stream)){
			if (fgets(buffer, max_buffer, stream) != NULL) data.append(buffer);
		}
		pclose(stream);
	}
	return data;
}
