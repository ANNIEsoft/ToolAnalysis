#ifndef cMrdMlReco_H
#define cMrdMlReco_H

/*
 * cMrdMlReco
 * ==========
 * ToolAnalysis adapter (output contract v6) for the frozen ANNIE cMRD v5 gate
 * and reconstruction SavedModels, deployed as release cMrdMlReco-B2v042.
 *
 * Naming: "v5" is the frozen model / feature / packet interface (the Python
 * runtime keeps its cmrd_v5_* module names); "v6" is this C++ adapter and its
 * Store / ROOT output contract.  v6 does not imply a new neural network.
 *
 * The C++ tool only:
 *   1. reads detector products from the ToolAnalysis stores,
 *   2. serializes them into a one-line JSON event packet,
 *   3. supervises a persistent Python inference daemon,
 *   4. parses the daemon response and publishes it to Stores["MrdMLReco"].
 * Feature construction, cluster selection, the Tank-bypass decision and
 * TensorFlow inference live in the frozen Python runtime.
 *
 * Design invariants carried forward from the alpha commissioning release
 * ----------------------------------------------------------------------
 *   - Detector-only boundary: truth, GENIE, SimpleReco, MuonFitter fits and
 *     classical FindMrdTracks outputs are never model inputs.
 *   - One source of feature semantics: C++ never reimplements the frozen v5
 *     feature builder.  It forwards detector products; Python owns selection,
 *     masking, geometry lookup and tensor construction.
 *   - Release identity is fail-closed: model / geometry / protocol mismatches
 *     are startup failures, not warnings or fallbacks.
 *   - Event failures are fail-soft: one malformed event receives sentinels and
 *     a structured FailureCode so the rest of the ToolChain can continue.
 *   - Production/debug changes publication breadth only.  It must not change
 *     the packet, gate decision, reconstruction route or physics values.
 *   - Every active per-event Store key is rewritten every event.  No previous
 *     trigger is allowed to leak into a later one.
 *   - The MRD-only Tank bypass is an intentional, frozen reconstruction route
 *     and is explicitly published as RecoUsedTankBypass.
 *
 * Output modes (config key "OutputMode", default production)
 * ---------------------------------------------------------
 *   production : physics products (gate probabilities, topology / exit-face
 *                class, reconstructed quantities with uncertainties), a
 *                minimal status block, compact input provenance (including
 *                RecoUsedTankBypass and the Tank<->MRD association quartet),
 *                and job-level identity keys.  Response-contract
 *                re-validation is left to the daemon, which enforces it.
 *   debug      : full input diagnostics and association provenance,
 *                per-class probabilities, per-expert endpoints, event identity,
 *                and complete C++ response-contract re-validation. Debug*
 *                packet/response dumps are honoured. Legacy smoke-test aliases
 *                map to debug for backward compatibility.
 *
 * Interface to PhaseIITreeMaker
 *   - The Store is always Stores["MrdMLReco"] (not configurable).
 *   - The mode is published as CStore["MrdMLOutputMode"]; PhaseIITreeMaker
 *     refuses to initialise without it, i.e. cMrdMlReco must precede it.
 *   - Job-level keys (identity, hashes, thresholds) are (re)written after
 *     every READY handshake; per-event keys are rewritten every event.
 */

#include <cstdint>
#include <map>
#include <string>
#include <vector>

#include <sys/types.h>

#include "Tool.h"
#include "Geometry.h"
#include "Rtypes.h"

class cMrdMlReco : public Tool {
public:
  cMrdMlReco();
  bool Initialise(std::string configfile, DataModel& data);
  bool Execute();
  bool Finalise();

private:
  // Tool-local logging levels (the Application-branch Tool base does not
  // define v_error/v_warning/v_message/v_debug).
  enum VerbosityLevel { v_error = 0, v_warning = 1, v_message = 2, v_debug = 3 };

  enum OutputMode { kOutputProduction = 0, kOutputDebug = 1 };

  // Frozen interface constants.
  //   Schema 2: mode-dependent key sets (see README.md) plus job-level
  //             identity keys.
  //   Protocol 1: unchanged JSON request/response contract with the daemon.
  static const int kNativeSchemaVersion = 2;
  static const char* const kAdapterVersion;  // e.g. "cMrdMlReco-adapter-v6.0"
  static const char* const kStoreName;       // always "MrdMLReco"
  static const int kProtocolVersion = 1;
  static const int kNumEventCategories = 7;
  static const int kNumTopologyClasses = 3;
  static const int kNumExitFaces = 7;

  // RecoSkipReason values 1-5 come from the Python daemon.  6 is assigned in
  // C++ when the optional MRD-hit prefilter short-circuits the daemon call.
  static const int kSkipReasonPrefilter = 6;

  enum FailureCode {
    kFailureNone = 0,
    kFailureANNIEEventMissing = 1,
    kFailureAnnieGeometryMissing = 2,
    kFailureMRDMappingMissing = 3,
    kFailureMRDInput = 4,
    kFailureTankInput = 5,
    kFailurePacketSerialization = 6,
    kFailureDaemonUnavailable = 10,
    kFailureDaemonTimeout = 11,
    kFailureDaemonProtocol = 12,
    kFailureDaemonRestartExhausted = 13,
    // 20-51 are produced by the Python daemon and passed through unchanged.
    kFailureNativeStoreWrite = 60
  };

  // ---- Request side -------------------------------------------------------
  // These structs mirror detector products only.  They deliberately stop at
  // the ToolAnalysis/Python boundary: no ML feature vectors are constructed
  // here, which keeps the deployed feature definition in one frozen module.

  struct EventIdentity {
    int run_number;
    int subrun_number;
    int event_number;
    uint64_t mc_event_num;
    int mc_trigger_num;
    bool is_data;
    EventIdentity();
  };

  struct MRDPacket {
    std::string source;
    int num_time_clusters_store;
    std::vector<unsigned long long> digit_chankeys;
    std::vector<double> digit_times_ns;
    std::vector<double> digit_charges;
    std::vector<std::vector<int> > time_clusters;
    MRDPacket();
  };

  struct TankClusterPacket {
    double time_ns;
    std::vector<unsigned long long> detkeys;
    std::vector<double> hit_times_ns;
    std::vector<double> hit_charges_pe;
    TankClusterPacket();
  };

  struct TankPacket {
    std::string source;
    std::vector<TankClusterPacket> clusters;
    TankPacket();
  };

  // ---- Response side ------------------------------------------------------
  // ReleaseIdentity belongs to the READY handshake and is job-level state.
  // Diagnostics/Gate/Reconstruction belong to one event.  Keeping those two
  // lifetimes separate is what lets PhaseIITreeMaker write one metadata entry
  // while still rewriting every event-level branch each trigger.

  struct ReleaseIdentity {
    bool valid;
    int protocol_version;
    std::string daemon_version;
    std::string runtime_feature_version;
    std::string model_release;
    std::string gate_model_name;
    std::string reconstruction_model_name;
    std::string gate_model_tree_sha256;
    std::string reconstruction_model_tree_sha256;
    std::string geometry_version;
    std::string geometry_manifest_sha256;
    std::string gate_trainer_version;
    std::string reconstruction_trainer_version;
    std::string tensorflow_version;
    double gate_has_target_threshold;
    double gate_reconstructable_threshold;
    bool require_gate_category_zero;
    ReleaseIdentity();
  };

  // Input-construction audit trail returned by Python.  Production parses
  // only the provenance subset (mrd_source, number_mrd_time_clusters,
  // selected_mrd_cluster_index, selected_tank_cluster_index, input_tank_hits,
  // and the tank_mrd_{association_found, selected_tank_cluster_index,
  // selected_mrd_subevent_id, timing_residual_ns} quartet); debug parses all.
  // Counts/indices use -1 and physical scalars -999 until present=true.
  struct Diagnostics {
    bool present;
    std::string mrd_source;
    int number_mrd_time_clusters;
    int selected_mrd_cluster_index;
    double selected_mrd_cluster_start_time_ns;
    double selected_mrd_cluster_end_time_ns;
    double selected_mrd_cluster_time_span_ns;
    int input_mrd_raw_hits;
    int input_mrd_selected_raw_hits;
    int input_mrd_geometry_valid_hits_before_dead_mask;
    int input_mrd_known_dead_removed;
    int input_mrd_live_hits_before_dedup;
    int input_mrd_live_unique_hits;
    int input_mrd_dedup_removed;
    int input_mrd_unassigned_hits;
    int input_mrd_layers;
    int input_mrd_first_layer;
    int input_mrd_last_layer;
    std::string tank_source;
    int number_tank_clusters;
    int selected_tank_cluster_index;
    double selected_tank_cluster_time_ns;
    double selected_tank_cluster_charge_pe;
    double selected_tank_cluster_time_span_ns;
    int input_tank_hits;
    bool tank_packet_reordered_by_time;
    bool reco_structurally_eligible;
    bool physics_is_raw_unstandardized;
    std::vector<int> additional_dead_mrd_chankeys;
    // EventSelector-mimic Tank<->MRD association provenance.
    bool tank_mrd_association_applied;
    bool tank_mrd_association_found;
    int tank_mrd_compatible_pair_count;
    int tank_mrd_selected_tank_cluster_index;
    int tank_mrd_selected_mrd_subevent_id;
    double tank_mrd_expected_offset_ns;
    double tank_mrd_coincidence_window_ns;
    double tank_mrd_tank_mean_time_ns;
    double tank_mrd_mrd_mean_time_ns;
    double tank_mrd_dt_ns;
    double tank_mrd_timing_residual_ns;
    Diagnostics();
  };

  struct GateResult {
    bool present;
    // production
    double has_target_probability;
    double reconstructable_probability;
    int event_category;
    bool passed;
    // debug
    double event_category_confidence;
    double event_category_margin;
    std::vector<double> event_category_probabilities;
    double single_muon_compatibility_probability;
    bool single_muon_compatibility_validated;
    GateResult();
  };

  // Topology class order is frozen: 0 = stopped, 1 = back exit, 2 = side exit.
  struct ReconstructionResult {
    bool present;
    // ---- production: topology ----
    int topology_class;
    std::vector<double> topology_probabilities;
    double stopped_probability;
    int exit_face_class;
    // ---- production: reconstructed values and uncertainties ----
    double mrd_track_length_cm, mrd_track_length_sigma_cm;
    double cos_theta, cos_theta_sigma;
    double theta_rad, theta_sigma_rad;
    bool theta_sigma_valid;
    double phi_rad;
    double penetration_cm, penetration_sigma_cm;
    double mrd_energy_loss_mev, mrd_energy_loss_sigma_mev;
    double muon_ke_mev, muon_ke_sigma_mev;
    double cherenkov_angle_rad, cherenkov_angle_sigma_rad;
    std::vector<double> tank_vertex_cm, tank_vertex_sigma_cm;
    std::vector<double> tank_exit_cm, tank_exit_sigma_cm;
    std::vector<double> mrd_entry_cm, mrd_entry_sigma_cm;
    std::vector<double> mrd_exit_cm, mrd_exit_sigma_cm;
    std::vector<double> selected_endpoint_cm, selected_endpoint_sigma_cm;
    std::vector<double> mrd_scatter_rad, mrd_scatter_sigma_rad;
    std::vector<double> tank_scatter_rad, tank_scatter_sigma_rad;
    // ---- debug only ----
    double topology_confidence, topology_margin;
    double topology_p_stopped, topology_p_back_exit, topology_p_side_exit;
    double exit_face_confidence, exit_face_margin;
    std::vector<double> exit_face_probabilities;
    int endpoint_expert_used;
    double sin_phi, cos_phi, phi_vector_norm;
    std::vector<double> stop_endpoint_cm, stop_endpoint_sigma_cm;
    std::vector<double> back_exit_endpoint_cm, back_exit_endpoint_sigma_cm;
    std::vector<double> side_exit_endpoint_cm, side_exit_endpoint_sigma_cm;
    // Reserved placeholders (always false/0 in this release).
    bool quality_baseline, quality_topology, quality_direction, quality_endpoint;
    int quality_tier;
    ReconstructionResult();
  };

  // One per event.  Starts in a deterministic "not processed" sentinel state,
  // is filled by either a technical failure or a validated daemon response,
  // and is published to the Store exactly once at the end of Execute().
  struct DaemonResponse {
    bool parsed;
    int protocol_version;
    bool ok;
    std::string status;
    int failure_code;
    int reco_skip_reason;
    std::string error;
    bool input_build_ok;
    bool gate_attempted;
    bool gate_inference_ok;
    bool gate_passed;
    bool reco_attempted;
    bool reco_inference_ok;
    // Derived in C++ from the same input the daemon routes on:
    // reco_attempted && diagnostics.input_tank_hits < 1, i.e. reconstruction
    // was run through the MRD-only Tank-bypass route of _reco_skip_reason().
    bool reco_used_tank_bypass;
    Diagnostics diagnostics;
    GateResult gate;
    ReconstructionResult reconstruction;
    DaemonResponse();
  };

  // ---- Configuration ------------------------------------------------------
  int verbosity_;
  bool is_data_;
  OutputMode output_mode_;
  std::string output_mode_name_;
  std::string python_exe_;
  std::string inference_script_;
  std::string gate_model_dir_;
  std::string reconstruction_model_dir_;
  std::string gate_model_meta_;
  std::string reconstruction_model_meta_;
  std::string geometry_dir_;
  std::string model_release_;
  std::string expected_geometry_manifest_sha256_;
  std::string expected_gate_model_tree_sha256_;
  std::string expected_reconstruction_model_tree_sha256_;
  double gate_has_target_threshold_;
  double gate_reconstructable_threshold_;
  bool require_gate_category_zero_;
  bool allow_tf_version_mismatch_;
  int tensorflow_intra_op_threads_;
  int tensorflow_inter_op_threads_;
  int daemon_startup_timeout_sec_;
  int daemon_response_timeout_sec_;
  int daemon_shutdown_timeout_sec_;
  bool restart_daemon_on_failure_;
  int max_daemon_restarts_;
  int prefilter_min_mrd_hits_;
  std::string mc_tdc_data_key_;
  std::vector<int> additional_dead_mrd_chankeys_;
  std::vector<std::string> configuration_errors_;
  std::string debug_packet_directory_;
  int debug_packet_max_events_;
  std::string debug_response_file_;
  bool write_mrdtracks_compatibility_;
  bool write_muonfitter_compatibility_;
  size_t max_protocol_line_bytes_;

  // ---- ToolAnalysis / runtime state ---------------------------------------
  Geometry* geom_;
  BoostStore* native_store_;
  std::map<unsigned long, int> channelkey_to_mrdpmtid_;
  long long event_counter_;
  long long daemon_request_counter_;
  long long daemon_failure_counter_;
  long long prefilter_skip_counter_;
  int daemon_restart_count_;
  ReleaseIdentity release_identity_;

  // ---- Daemon process state -----------------------------------------------
  pid_t daemon_pid_;
  int daemon_stdin_fd_;
  int daemon_stdout_fd_;
  bool daemon_alive_;
  std::string daemon_read_buffer_;

  bool DebugMode() const { return output_mode_ == kOutputDebug; }

  // Startup and configuration
  void LoadConfiguration(const std::string& configfile);
  bool ValidateConfiguration();
  bool EnsureNativeStore();
  bool RefreshRuntimeGeometryAndMapping();
  static bool FileExists(const std::string& path);
  static bool DirectoryExists(const std::string& path);
  static bool ParseIntList(const std::string& text, std::vector<int>& values,
                           std::string& error);

  // Per-event processing
  void ProcessEvent(EventIdentity& identity, DaemonResponse& result);
  void MarkTechnicalFailure(DaemonResponse& result, int failure_code,
                            const std::string& status,
                            const std::string& error);

  // Input collection and packet serialization
  bool ReadEventIdentity(EventIdentity& identity, std::string& error) const;
  bool CollectMRDInput(MRDPacket& out, int& failure_code, std::string& error);
  bool CollectRawMRDInput(MRDPacket& out, bool& store_present, std::string& error);
  bool CollectTankInput(TankPacket& out, int& failure_code, std::string& error);
  bool BuildEventPacketJSON(const EventIdentity& identity, const MRDPacket& mrd,
                            const TankPacket& tank, std::string& json,
                            std::string& error) const;
  void MaybeWriteDebugPacket(const EventIdentity& identity,
                             const std::string& json) const;
  void MaybeWriteDebugResponse(const EventIdentity& identity,
                               const std::string& json) const;

  // Daemon process and protocol
  bool StartDaemon();
  void StopDaemon();
  bool EnsureDaemonAvailable();
  bool CheckDaemonAlive();
  bool WriteProtocolLine(std::string& line);
  bool ReadProtocolLine(std::string& line, int timeout_sec, bool& timed_out);
  bool ParseReadyLine(const std::string& line, std::string& error);
  bool ParseDaemonResponse(const std::string& line, DaemonResponse& response,
                           std::string& error) const;
  bool SendPacketAndReceive(std::string& packet, std::string& response_line,
                            int& failure_code, std::string& error);

  // Store publication (exactly once per event)
  void Publish(const EventIdentity& identity, const DaemonResponse& result);
  void PublishJobIdentity();  // after every READY handshake (both modes)

  // JSON helpers
  static std::string EscapeJSON(const std::string& value);
};

#endif
