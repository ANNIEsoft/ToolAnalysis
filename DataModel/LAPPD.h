#ifndef LAPPD_H
#define LAPPD_H
#include <vector>

#include <SerialisableObject.h>

class LAPPD{

  friend class boost::serialization::access;

public:

  std::vector<double> waveform;

  bool Print() {return true;};

 private:


  template<class Archive> void serialize(Archive &ar, const unsigned int version){

    ar & waveform;

  }

};
#endif
