#pragma once

// standard library includes
#include <map>
#include <memory>
#include <string>
#include <tuple>
#include <vector>
#include <future>

// ROOT includes
#include "RQ_OBJECT.h"
#include "TGClient.h"

// recoANNIE includes
#include "RawReader.h"
#include "RawReadout.h"

class TGMainFrame;
class TGraph;
class TGWindow;
class TGCompositeFrame;
class TGLabel;
class TGListBox;
class TGTextButton;
class TGTextView;
class TRootEmbeddedCanvas;

namespace annie {
  class RawViewer {

    public:
      RawViewer();
      virtual ~RawViewer();

      void handle_next_button();
      //void handle_previous_button();  // cannot go to last event in ToolChain
      void handle_close_button();  // close the viewer and allow ToolChain to complete
      void handle_channel_selection();

      void prepare_gui();

      void next_readout(annie::RawReadout* nextreadout, std::promise<int> finishedin);
      void update_plot();
      void update_channel_selector();
      void update_text_view();

    protected:

      std::promise<int> finished;   // use to hold the ToolChain until we're done looking at the plots

      annie::RawReadout* raw_readout_ = nullptr;

      std::unique_ptr<TGraph> graph_;

      int selected_channel_index_ = 0;

      // Keys are TGListBox entry IDs, values are (card, channel, minibuffer)
      // index tuples
      std::map<int, std::tuple<int, int, int> > channel_indices_;

      // GUI elements
      std::unique_ptr<TGMainFrame> main_frame_;
      TRootEmbeddedCanvas* embedded_canvas_;
      TGCompositeFrame* composite_frame_;
      TGLabel* channel_selector_label_;
      TGListBox* channel_selector_;
      TGTextView* text_view_;
      TGTextButton* next_button_;
      TGTextButton* previous_button_;
      TGTextButton* close_button_;
      TGLabel* canvas_label_;

    RQ_OBJECT("annie::RawViewer")
  };
}
