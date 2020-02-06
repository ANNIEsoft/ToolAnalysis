// Class that represents a full readout from all of the DAQ VME
// cards. Includes data for a single trigger in non-Hefty mode
// or multiple triggers in Hefty mode.
//
// Steven Gardiner <sjgardiner@ucdavis.edu>
#ifndef RAWREADOUT_H
#define RAWREADOUT_H

// standard library includes
#include <map>
#include <string>

// reco-annie includes
#include "Constants.h"
#include "RawCard.h"
#include "RawTrigData.h"
#include "ANNIEconstants.h"

namespace annie {

  class RawReadout {

    public:

      RawReadout(int SequenceID = BOGUS_INT) : sequence_id_(SequenceID) {}

      inline void set_sequence_id(int seq_id) { sequence_id_ = seq_id; }
      inline int sequence_id() const { return sequence_id_; }

      void add_card(int CardID, unsigned long long LastSync, int StartTimeSec,
        int StartTimeNSec, unsigned long long StartCount, int Channels,
        int BufferSize, int MiniBufferSize,
        const std::vector<unsigned short>& FullBufferData,
        const std::vector<unsigned long long>& TriggerCounts,
        const std::vector<unsigned int>& Rates, bool overwrite_ok = false);

      inline const std::map<int, annie::RawCard>& cards() const
        { return cards_; }

      inline const annie::RawCard& card(int index) const
        { return cards_.at(index); }

      inline const annie::RawChannel& channel(int card_index,
        int channel_index)
      {
        return cards_.at(card_index).channel(channel_index);
      }

      inline const annie::RawTrigData& trig_data() const { return trig_data_; }

      inline void set_trig_data(const annie::RawTrigData& TrigData)
        { trig_data_ = TrigData; }

      inline void set_run_information(
        const std::map<std::string, std::string>& RunInfo)
      {
        run_info_ = RunInfo;
      }

      inline const std::map<std::string, std::string>& run_information() const
        { return run_info_; }

    protected:

      /// @brief Integer index identifying this DAQ readout (unique within
      /// a run)
      int sequence_id_;

      /// @brief Raw data for each of the VME cards included in the readout
      /// @details Keys are VME card IDs, values are RawCard objects storing
      /// the associated data from the PMTData tree.
      std::map<int, annie::RawCard> cards_;

      /// @brief Container holding the contents of the TrigData TTree for this
      /// readout's SequenceID
      annie::RawTrigData trig_data_;

      /// @brief Map representing the RunInformation TTree. Keys are InfoTitle
      /// entries, values are the corresponding JSON InfoMessage strings
      std::map<std::string, std::string> run_info_;

    public:   // just to keep the Store happy, so we can put pointers in the BoostStore.
    template<class Archive> void serialize(Archive & ar, const unsigned int version){}

  };
}

#endif
