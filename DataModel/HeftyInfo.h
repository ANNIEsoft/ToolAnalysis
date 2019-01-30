// Class to store Phase I Hefty timestamps, trigger masks, etc. generated using
// Jonathan Eisch's annietools scripts
//
// Steven Gardiner <sjgardiner@ucdavis.edu>
#pragma once

// standard library includes
#include <iostream>

// ToolAnalysis includes
#include "SerialisableObject.h"

class HeftyInfo : public SerialisableObject {

  friend class boost::serialization::access;

  public:
    inline HeftyInfo() : sequence_id_(0), more_(false) { serialise = true; }

    inline HeftyInfo(int sequence_id,
      const std::vector<unsigned long long>& time,
      const std::vector<int>& label, const std::vector<long long>& t_since_beam,
      const std::vector<int>& more) : sequence_id_(sequence_id), time_(time),
      label_(label), t_since_beam_(t_since_beam), more_( more.back() == 1 )
    {
      serialise = true;
    }

    inline size_t num_minibuffers() const { return time_.size(); }

    inline int sequence_id() const { return sequence_id_; }
    inline unsigned long long time(size_t mb) const { return time_.at(mb); }
    inline int label(size_t mb) const { return label_.at(mb); }
    inline long long t_since_beam(size_t mb) const
      { return t_since_beam_.at(mb); }
    inline bool more() const { return more_; }
    inline std::vector<unsigned long long> all_times() const { return time_; }

    inline virtual bool Print() override {
      std::cout << "SequenceID = " << sequence_id_ << '\n';
      std::cout << "Number of minibuffers = " << num_minibuffers() << '\n';
      return true;
    }

  protected:
    int sequence_id_;

    // ns since Unix epoch for each minibuffer
    std::vector<unsigned long long> time_;

    std::vector<int> label_; // trigger masks for each minibuffer

    // Time since the last beam (or source) trigger for the start of each
    // minibuffer
    std::vector<long long> t_since_beam_;

    bool more_; // true if last element of More[] == 1, false otherwise

    template<class Archive> void serialize(Archive & ar,
      const unsigned int version)
    {
      if (serialise) {
        ar & sequence_id_;
        ar & time_;
        ar & label_;
        ar & t_since_beam_;
        ar & more_;
      }
    }
};
