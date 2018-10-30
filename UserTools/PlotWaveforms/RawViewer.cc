// ROOT includes
#include "TAxis.h"
#include "TCanvas.h"
#include "TF1.h"
#include "TGButton.h"
#include "TGClient.h"
#include "TGFrame.h"
#include "TGraph.h"
#include "TGResourcePool.h"
#include "TRootEmbeddedCanvas.h"

#include "TGClient.h"
#include "TGLabel.h"
#include "TGListBox.h"
#include "TGTextView.h"

// recoANNIE includes
#include "Constants.h"

// viewer includes
#include "RawViewer.h"

annie::RawViewer::RawViewer()
{
  prepare_gui();
}

void annie::RawViewer::next_readout(annie::RawReadout* nextreadout, std::promise<int> finishedin) {

  finished = std::promise<int>(std::move(finishedin));
  raw_readout_ = nextreadout;
  if (!raw_readout_) return;

  update_channel_selector();
  update_text_view();
  update_plot();
  
}

void annie::RawViewer::handle_next_button() {
  finished.set_value(1);
}

//void annie::RawViewer::handle_previous_button() {
//  auto rr = reader_.previous();
//  if (!rr) return;

//  raw_readout_ = std::move(rr);

//  update_channel_selector();
//  update_text_view();
//  update_plot();
//}

void annie::RawViewer::handle_close_button() {
  finished.set_value(0);
  //delete this;
}

// Updates the waveform plot based on the currently selected channel
void annie::RawViewer::handle_channel_selection() {

  selected_channel_index_ = channel_selector_->GetSelected();

  update_text_view();
  update_plot();
}

void annie::RawViewer::update_plot() {

  TCanvas* can = embedded_canvas_->GetCanvas();
  can->cd();

  auto triple = channel_indices_.at(selected_channel_index_);
  int card_id = std::get<0>(triple);
  int channel_id = std::get<1>(triple);
  int minibuffer_id = std::get<2>(triple);

  const std::vector<unsigned short>& mb_data = raw_readout_->card(card_id)
    .channel(channel_id).minibuffer_data(minibuffer_id);
  
  size_t num_points = mb_data.size();
  graph_.reset( new TGraph(num_points) );
  for (size_t i = 0; i < num_points; ++i) {
    graph_->SetPoint(i, i * NS_PER_SAMPLE, mb_data.at(i));
  }

  graph_->SetLineColor(kBlack);
  graph_->SetLineWidth(2);

  TString temp_title;
  temp_title.Form("SequenceID %d Card %d Channel %d; time (ns); ADC counts",
    raw_readout_->sequence_id(), card_id, channel_id);

  graph_->GetXaxis()->SetRangeUser(0., num_points * NS_PER_SAMPLE);
  graph_->SetTitle(temp_title);

  graph_->Draw("al");

  can->Update();
}

annie::RawViewer::~RawViewer() {
  // Clean up used widgets: frames, buttons, layout hints
  main_frame_->Cleanup();
}

void annie::RawViewer::prepare_gui() {

  // Create a main frame that we'll use to run the GUI
  if(not gClient){ std::cerr<<"no gClient!"<<std::endl; return; }
  //main_frame_ = std::make_unique<TGMainFrame>(gClient->GetRoot(), 1240, 794,
  //  kMainFrame | kVerticalFrame);
  main_frame_ = std::unique_ptr<TGMainFrame>(
    new TGMainFrame(gClient->GetRoot(), 1240, 794, kMainFrame | kVerticalFrame));
    
  main_frame_->SetName("main_frame_");
  main_frame_->SetWindowName("recoANNIE Raw Data Viewer");
  main_frame_->SetLayoutBroken(true);

  // Create a composite frame to hold everything
  composite_frame_ = new TGCompositeFrame(main_frame_.get(), 1240, 648,
    kVerticalFrame);
  composite_frame_->SetName("composite_frame_");
  composite_frame_->SetLayoutBroken(true);

  // Text label for the PMT selection list box
  channel_selector_label_ = new TGLabel(composite_frame_, "Channels");
  channel_selector_label_->SetTextJustify(36);
  channel_selector_label_->SetMargins(0, 0, 0, 0);
  channel_selector_label_->SetWrapLength(-1);
  composite_frame_->AddFrame(channel_selector_label_,
    new TGLayoutHints(kLHintsLeft | kLHintsTop, 2, 2, 2, 2));
  channel_selector_label_->MoveResize(920, 8, 232, 20);

  // List box used to select PMTs
  channel_selector_ = new TGListBox(composite_frame_, -1, kSunkenFrame);
  channel_selector_->SetName("channel_selector_");
  composite_frame_->AddFrame(channel_selector_,
    new TGLayoutHints(kLHintsLeft | kLHintsTop | kLHintsExpandX
    | kLHintsExpandY, 2, 2, 2, 2));
  channel_selector_->MoveResize(912, 32, 240, 498);

  // Set up PMT selection actions
  // Handle keyboard movements in the list box
  TGLBContainer* channel_selector_container
    = dynamic_cast<TGLBContainer*>(channel_selector_->GetContainer());
  if (!channel_selector_container)
    throw "Unable to access channel selector container!";
  else channel_selector_container->Connect("CurrentChanged(TGFrame*)",
    "annie::RawViewer", this, "handle_channel_selection()");
  // Handle mouse clicks in the list box
  channel_selector_->Connect("Selected(Int_t)", "annie::RawViewer", this,
    "handle_channel_selection()");

  // Embedded canvas to use when plotting raw waveforms
  embedded_canvas_ = new TRootEmbeddedCanvas(0, composite_frame_, 880, 520,
    kSunkenFrame);
  embedded_canvas_->SetName("embedded_canvas_");
  Int_t embedded_canvas_window_id = embedded_canvas_->GetCanvasWindowId();
  TCanvas* dummy_canvas = new TCanvas("dummy_canvas", 10, 10,
    embedded_canvas_window_id);
  embedded_canvas_->AdoptCanvas(dummy_canvas);
  composite_frame_->AddFrame(embedded_canvas_,
    new TGLayoutHints(kLHintsLeft | kLHintsTop, 2, 2, 2, 2));
  embedded_canvas_->MoveResize(16, 32, 880, 520);

  // Text view box that shows information about the selected channel
  text_view_ = new TGTextView(composite_frame_, 888, 80);
  text_view_->SetName("text_view_");
  composite_frame_->AddFrame(text_view_,
    new TGLayoutHints(kLHintsBottom | kLHintsExpandX));
  text_view_->MoveResize(8, 560, 888, 80);

  // Set the font in the text view box to something more readable (default
  // is too small)
  const TGFont* font = gClient
    ->GetFontPool()->GetFont("helvetica", 20, kFontWeightNormal,
    kFontSlantRoman);
  if (!font) font = gClient->GetResourcePool()->GetDefaultFont();
  text_view_->SetFont(font->GetFontStruct());

  // Label for the raw waveform canvas
  canvas_label_ = new TGLabel(composite_frame_, "Raw Waveform");
  canvas_label_->SetTextJustify(36);
  canvas_label_->SetMargins(0, 0, 0, 0);
  canvas_label_->SetWrapLength(-1);
  composite_frame_->AddFrame(canvas_label_,
    new TGLayoutHints(kLHintsLeft | kLHintsTop, 2, 2, 2, 2));
  canvas_label_->MoveResize(288,8,296,24);

  // "Next trigger" button
  next_button_ = new TGTextButton(composite_frame_, "next readout", -1,
    TGTextButton::GetDefaultGC()(), TGTextButton::GetDefaultFontStruct(),
    kRaisedFrame);
  next_button_->SetTextJustify(36);
  next_button_->SetMargins(0, 0, 0, 0);
  next_button_->SetWrapLength(-1);
  next_button_->Resize(99, 48);
  composite_frame_->AddFrame(next_button_,
    new TGLayoutHints(kLHintsLeft | kLHintsTop, 2, 2, 2, 2));
  next_button_->MoveResize(1080, 568, 99, 48);

  // Set up next readout action
  next_button_->Connect("Clicked()", "annie::RawViewer", this,
    "handle_next_button()");

//  // "Previous trigger" button
//  previous_button_ = new TGTextButton(composite_frame_, "previous readout",
//    -1, TGTextButton::GetDefaultGC()(), TGTextButton::GetDefaultFontStruct(),
//    kRaisedFrame);
//  previous_button_->SetTextJustify(36);
//  previous_button_->SetMargins(0, 0, 0, 0);
//  previous_button_->SetWrapLength(-1);
//  previous_button_->Resize(99, 48);
//  composite_frame_->AddFrame(previous_button_,
//    new TGLayoutHints(kLHintsLeft | kLHintsTop, 2, 2, 2, 2));
//  previous_button_->MoveResize(920, 568, 99, 48);

//  // Set up previous readout action
//  previous_button_->Connect("Clicked()", "annie::RawViewer", this,
//    "handle_previous_button()");

  // "Close Viewer" button
  close_button_ = new TGTextButton(composite_frame_, "close viewer",
    -1, TGTextButton::GetDefaultGC()(), TGTextButton::GetDefaultFontStruct(),
    kRaisedFrame);
  close_button_->SetTextJustify(36);
  close_button_->SetMargins(0, 0, 0, 0);
  close_button_->SetWrapLength(-1);
  close_button_->Resize(99, 48);
  composite_frame_->AddFrame(previous_button_,
    new TGLayoutHints(kLHintsLeft | kLHintsTop, 2, 2, 2, 2));
  close_button_->MoveResize(920, 568, 99, 48);

  // Set up close readout action
  close_button_->Connect("Clicked()", "annie::RawViewer", this,
    "handle_close_button()");


  // Add the completed composite frame to the main frame.
  // Resize everything as needed.
  main_frame_->AddFrame(composite_frame_, new TGLayoutHints(kLHintsNormal));
  main_frame_->MoveResize(0, 0, 1184, 647);
  main_frame_->SetMWMHints(kMWMDecorAll, kMWMFuncAll, kMWMInputModeless);
  main_frame_->MapSubwindows();
  main_frame_->MapWindow();
}

// Updates the list box of channels based on those present in the current
// readout
void annie::RawViewer::update_channel_selector() {

  channel_indices_.clear();

  // Remove all of the old PMT entries from the list box
  channel_selector_->RemoveAll();

  // Check that we actually have PMTs for this trigger
  if (raw_readout_->cards().empty()) throw "Empty DAQ card map!";

  // Create new entries for all of the PMTs in the current trigger. Use
  // their indices in the vector of PMTs as their IDs for the list box.
  // This has the benefit that signal handlers can immediately access
  // the correct PMT simply by knowing its list box item ID.
  TString dummy_str;
  int num_listbox_entries = 0;
  for ( const auto& card_pair : raw_readout_->cards() ) {
    int card_id = card_pair.first;
    for ( const auto& channel_pair : card_pair.second.channels() ) {
      int channel_id = channel_pair.first;
      for (unsigned int m = 0; m < channel_pair.second.num_minibuffers(); ++m)
      {
        dummy_str.Form("Card %d Channel %d Minibuffer %d", card_id,
          channel_id, m);
        channel_selector_->AddEntry(dummy_str.Data(), num_listbox_entries);
        channel_indices_[num_listbox_entries] = std::make_tuple(card_id,
          channel_id, m);
        ++num_listbox_entries;
      }
    }
  }

  // Keep the index of the selected channel the same if possible.
  // If we get an invalid channel index, make it something reasonable.
  if (selected_channel_index_ >= num_listbox_entries)
    selected_channel_index_ = num_listbox_entries - 1;
  if (selected_channel_index_ < 0) selected_channel_index_ = 0;
  channel_selector_->Select(selected_channel_index_);

  // Update the displayed list box
  channel_selector_->Layout();
}

// Update the channel information in the text view panel
void annie::RawViewer::update_text_view() {
  text_view_->Clear();

  auto triple = channel_indices_.at(selected_channel_index_);
  int card_id = std::get<0>(triple);
  int channel_id = std::get<1>(triple);
  int minibuffer_id = std::get<2>(triple);

  TString message;
  message.Form("\nShowing card ID = %d, channel = %d, minibuffer = %d\n",
    card_id, channel_id, minibuffer_id);

  text_view_->LoadBuffer(message.Data());
}
