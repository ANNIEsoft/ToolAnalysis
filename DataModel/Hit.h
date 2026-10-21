/* vim:set noexpandtab tabstop=4 wrap */
#ifndef HITCLASS_H
#define HITCLASS_H

#include<SerialisableObject.h>

#include <iostream>

class Hit : public SerialisableObject {
  
  friend class boost::serialization::access;
  
public:
  Hit()
    : TubeId(0)
	, Time(0)
	, Charge(0)
  {
	serialise=true;
  }

  Hit(int thetubeid, double thetime, double thecharge)
    : TubeId(thetubeid)
	, Time(thetime)
	, Charge(thecharge)
  {
	serialise=true;
  }

  virtual ~Hit(){};
	
  inline int GetTubeId() const {return TubeId;}
  inline double GetTime() const {return Time;}
  inline double GetCharge() const {return Charge;}
	
  inline void SetTubeId(int tubeid){TubeId=tubeid;}
  inline void SetTime(double tc){Time=tc;}
  inline void SetCharge(double chg){Charge=chg;}
	
  bool Print()
  {
	std::cout<< "TubeId : " <<TubeId << std::endl;
	std::cout<< "Time : "   <<Time   << std::endl;
	std::cout<< "Charge : " <<Charge << std::endl;
	return true;
  }
	
protected:
  int TubeId;
  double Time;
  double Charge;
	
  template<class Archive> void serialize(Archive & ar, const unsigned int version)
  {
	if (serialise) {
	  ar & TubeId;
	  ar & Time;
	  ar & Charge;
	}
  }
};

//  Derived classes

class MCHit : public Hit {
  // XXX ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ XXX
  // XXX ~~~~~~~~~~~~~~~~~~~~~~~~ UPDATING THIS CLASS? ~~~~~~~~~~~~~~~~~~~~~~~~~~~~ XXX
  // XXX ~~~~~ Everything added in this class must be duplicated in MCLAPPDHit!~~~~ XXX
  // XXX ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ XXX
	

  friend class boost::serialization::access;
	
public:
  MCHit()
	: Hit()
	, Parents(std::vector<int>{})
	, DirectParents(std::vector<int>{})
	, StartTick(-5)
	, EndTick(-5)
	, IsDarknoise(false)
  {
	serialise=true;
  }

  // Start and End ticks are only ever to be set after initialization
  MCHit(int tubeid, double thetime, double thecharge, std::vector<int> theparents, std::vector<int> thedirectparents)
    : Hit(tubeid, thetime, thecharge)
	, Parents(theparents)
  , DirectParents(thedirectparents)
	, StartTick(-5)
	, EndTick(-5)
	, IsDarknoise(false)
  {
	serialise=true;
  }

  virtual ~MCHit(){};
	
  const std::vector<int>* GetParents() { return &Parents; }
  const std::vector<int>* GetDirectParents() { return &DirectParents; }
  int GetStartTick() { return StartTick; }
  int GetEndTick() { return EndTick; }
  bool GetIsDarknoise() const { return IsDarknoise; }

  void SetParents(std::vector<int> parentsin) { Parents = parentsin; }
  void SetDirectParents(std::vector<int> directparentsin) { DirectParents = directparentsin; }
  void SetStartTick(int tick) { StartTick = tick; }
  void SetEndTick(int tick) { EndTick = tick; }
  void SetIsDarknoise(bool v) { IsDarknoise = v; }

	
  bool Print()
  {
	std::cout << "TubeId : " << TubeId << std::endl;
	std::cout << "Time : "   << Time   << std::endl;
	std::cout << "Charge : " << Charge << std::endl;

	if (Parents.size()) {
	  std::cout << "Parent MCPartice TrackIDs: {";
	  for (uint idx = 0; idx < Parents.size(); ++idx) {
		std::cout << Parents.at(idx);

		if ((idx+1) < Parents.size()) std::cout << ", ";
	  }
	  std::cout << "}" << std::endl;
	} else {
	  std::cout << "No recorded parents" << std::endl;
	}

	std::cout << "IsDarknoise : " << (IsDarknoise ? "true" : "false") << std::endl;

	return true;
  }

protected:
  std::vector<int> Parents;
  std::vector<int> DirectParents;
  int StartTick;
  int EndTick;
  bool IsDarknoise;

  template<class Archive> void serialize(Archive & ar, const unsigned int version)
  {
	if (serialise) {
	  ar & TubeId;
	  ar & Time;
	  ar & Charge;

	  if (version > 0) {
		ar & Parents; // Parents is now track IDs rather than index within vector
    ar & DirectParents;
		ar & StartTick;
		ar & EndTick;
	  }

	  if (version > 1) {
		ar & IsDarknoise;
	  }
	}
  }
};

BOOST_CLASS_VERSION(MCHit, 2)

#endif
