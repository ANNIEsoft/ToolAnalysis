#include "cMrdMlReco.h"

// cMrdMlReco intentionally contains very little detector/reconstruction
// physics.  Its job is to make the boundary between ToolAnalysis and the
// frozen Python/SavedModel release explicit and auditable.  Comments below
// therefore emphasize ownership and invariants: what C++ is allowed to do,
// what Python owns, and which failures must stop the job versus only the event.

#include <algorithm>
#include <cerrno>
#include <chrono>
#include <climits>
#include <cmath>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <limits>
#include <locale.h>
#include <sstream>
#include <type_traits>
#include <utility>

#include <dirent.h>
#include <fcntl.h>
#include <poll.h>
#include <pthread.h>
#include <signal.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/wait.h>
#include <unistd.h>

#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ptree.hpp>

namespace {

using boost::property_tree::ptree;

const double kFloatSentinel = -999.0;
const double kProbabilitySentinel = -1.0;

std::vector<double> SentinelVector(size_t n, double value = kFloatSentinel) {
  return std::vector<double>(n, value);
}

// ---------------------------------------------------------------------------
// Locale-independent number I/O
// ---------------------------------------------------------------------------
// JSON requires '.' as the decimal separator. To avoid constructing a stream
// object for every number, reuse a process-local "C" locale. Parsing uses
// strtod_l(); formatting uses snprintf("%.17g") under uselocale().
// "%.17g" matches `ostream << std::setprecision(17)` for finite doubles,
// preserving the event-packet representation used by the Python runtime.
// The same locale is used for strict response parsing.
//

locale_t CLocale() {
  static locale_t c_locale = newlocale(LC_ALL_MASK, "C", (locale_t)0);
  return c_locale;
}

class ScopedCLocale {
public:
  ScopedCLocale() : previous_(uselocale(CLocale())) {}
  ~ScopedCLocale() { uselocale(previous_); }
private:
  locale_t previous_;
  ScopedCLocale(const ScopedCLocale&);
  ScopedCLocale& operator=(const ScopedCLocale&);
};

// Parse a complete finite double.  This is the single finiteness check on the
// response side: the daemon serializes with allow_nan=False, so a nonfinite
// value can only appear through numeric overflow of a malformed literal.
bool ParseDouble(const std::string& text, double& value) {
  if (text.empty()) return false;
  char* end = nullptr;
  const double parsed = strtod_l(text.c_str(), &end, CLocale());
  if (end != text.c_str() + text.size() || !std::isfinite(parsed)) return false;
  value = parsed;
  return true;
}

bool ParseInt(const std::string& text, int& value) {
  if (text.empty()) return false;
  char* end = nullptr;
  errno = 0;
  const long parsed = std::strtol(text.c_str(), &end, 10);
  if (errno == ERANGE || end != text.c_str() + text.size() ||
      parsed < INT_MIN || parsed > INT_MAX)
    return false;
  value = static_cast<int>(parsed);
  return true;
}

bool ParseBool(const std::string& text, bool& value) {
  if (text == "true" || text == "1") { value = true; return true; }
  if (text == "false" || text == "0") { value = false; return true; }
  return false;
}

// Caller must hold a ScopedCLocale.  Returns false for NaN/Inf, which strict
// JSON cannot represent.  This is the single finiteness check on the request
// side; upstream collection no longer repeats it hit by hit.
bool AppendDouble(std::string& out, double value) {
  if (!std::isfinite(value)) return false;
  char buffer[32];
  const int n = std::snprintf(buffer, sizeof(buffer), "%.17g", value);
  if (n <= 0 || n >= static_cast<int>(sizeof(buffer))) return false;
  out.append(buffer, static_cast<size_t>(n));
  return true;
}

bool AppendDoubleArray(std::string& out, const std::vector<double>& values) {
  out.push_back('[');
  for (size_t i = 0; i < values.size(); ++i) {
    if (i) out.push_back(',');
    if (!AppendDouble(out, values[i])) return false;
  }
  out.push_back(']');
  return true;
}

template <typename IntT>
void AppendIntArray(std::string& out, const std::vector<IntT>& values) {
  out.push_back('[');
  for (size_t i = 0; i < values.size(); ++i) {
    if (i) out.push_back(',');
    out += std::to_string(values[i]);
  }
  out.push_back(']');
}

void AppendNestedIntArrays(std::string& out,
                           const std::vector<std::vector<int> >& values) {
  out.push_back('[');
  for (size_t i = 0; i < values.size(); ++i) {
    if (i) out.push_back(',');
    AppendIntArray(out, values[i]);
  }
  out.push_back(']');
}

// ---------------------------------------------------------------------------
// Process helpers
// ---------------------------------------------------------------------------

bool SetCloseOnExec(int fd) {
  const int flags = fcntl(fd, F_GETFD);
  return flags >= 0 && fcntl(fd, F_SETFD, flags | FD_CLOEXEC) == 0;
}

// Descriptors >= 3 open right before fork(); used by the child when the
// close_range syscall is unavailable.
std::vector<int> SnapshotOpenFileDescriptors() {
  std::vector<int> fds;
  DIR* directory = opendir("/proc/self/fd");
  if (!directory) return fds;
  const int directory_fd = dirfd(directory);
  while (struct dirent* entry = readdir(directory)) {
    char* end = nullptr;
    errno = 0;
    const long value = std::strtol(entry->d_name, &end, 10);
    if (errno != 0 || end == entry->d_name || *end != '\0' || value < 3 ||
        value > std::numeric_limits<int>::max())
      continue;
    if (static_cast<int>(value) != directory_fd)
      fds.push_back(static_cast<int>(value));
  }
  closedir(directory);
  std::sort(fds.begin(), fds.end());
  fds.erase(std::unique(fds.begin(), fds.end()), fds.end());
  return fds;
}

// Write all bytes, retrying EINTR.  SIGPIPE is blocked only for this thread
// and only for the duration of the write, so a dead daemon becomes an EPIPE
// return instead of terminating ToolAnalysis, without changing the global
// signal disposition of the ToolChain.  A SIGPIPE generated by this write is
// consumed before the original mask is restored.
bool WriteAllNoSigPipe(int fd, const char* data, size_t size) {
  sigset_t sigpipe_set;
  sigset_t old_set;
  sigemptyset(&sigpipe_set);
  sigaddset(&sigpipe_set, SIGPIPE);
  const int mask_error = pthread_sigmask(SIG_BLOCK, &sigpipe_set, &old_set);
  if (mask_error != 0) {
    errno = mask_error;
    return false;
  }

  bool sigpipe_was_pending = false;
  sigset_t pending_set;
  if (sigpending(&pending_set) == 0)
    sigpipe_was_pending = sigismember(&pending_set, SIGPIPE) == 1;

  bool ok = true;
  int saved_errno = 0;
  const char* ptr = data;
  size_t remaining = size;
  while (remaining > 0) {
    const ssize_t wrote = write(fd, ptr, remaining);
    if (wrote < 0) {
      if (errno == EINTR) continue;
      saved_errno = errno;
      ok = false;
      break;
    }
    if (wrote == 0) {
      saved_errno = EPIPE;
      ok = false;
      break;
    }
    ptr += wrote;
    remaining -= static_cast<size_t>(wrote);
  }

  if (!ok && saved_errno == EPIPE && !sigpipe_was_pending) {
    struct timespec timeout;
    timeout.tv_sec = 0;
    timeout.tv_nsec = 0;
    while (sigtimedwait(&sigpipe_set, nullptr, &timeout) < 0 && errno == EINTR) {
    }
  }

  const int restore_error = pthread_sigmask(SIG_SETMASK, &old_set, nullptr);
  if (ok && restore_error != 0) {
    errno = restore_error;
    return false;
  }
  if (!ok) errno = saved_errno;
  return ok;
}

// ---------------------------------------------------------------------------
// Detector input helpers (shared by data Hit and MC MCHit)
// ---------------------------------------------------------------------------

// Copy raw TDC hits on recognised MRD channels.  Upstream values are
// forwarded untouched; the frozen Python runtime builds the model features.
template <typename HitT>
void AppendRawMRDHits(const std::map<unsigned long, std::vector<HitT> >& tdc,
                      const std::map<unsigned long, int>& channelkey_to_mrdpmtid,
                      std::vector<unsigned long long>& chankeys,
                      std::vector<double>& times_ns,
                      std::vector<double>& charges) {
  for (typename std::map<unsigned long, std::vector<HitT> >::const_iterator it =
           tdc.begin();
       it != tdc.end(); ++it) {
    if (channelkey_to_mrdpmtid.count(it->first) == 0) continue;
    for (size_t hi = 0; hi < it->second.size(); ++hi) {
      chankeys.push_back(static_cast<unsigned long long>(it->first));
      times_ns.push_back(it->second[hi].GetTime());
      charges.push_back(it->second[hi].GetCharge());
    }
  }
}

}  // namespace

// Out-of-class definitions for ODR-used static constants.
const int cMrdMlReco::kNativeSchemaVersion;
const int cMrdMlReco::kProtocolVersion;
const int cMrdMlReco::kNumEventCategories;
const int cMrdMlReco::kNumTopologyClasses;
const int cMrdMlReco::kNumExitFaces;
const int cMrdMlReco::kSkipReasonPrefilter;
const char* const cMrdMlReco::kAdapterVersion = "cMrdMlReco-adapter-v6.0";
const char* const cMrdMlReco::kStoreName = "MrdMLReco";

namespace {

// Copy every ClusterFinder cluster in ascending cluster-time (std::map) order,
// matching the order seen by the training writer.  ClusterMap and
// ClusterMapDetkey must share exact time keys and per-cluster hit counts.
//
// Charge convention:
//   data Hit::GetCharge()  -> calibrated charge; divided by the channel SPE
//                             value (indexed by GetTubeId(), as in
//                             PhaseIITreeMaker) to obtain PE.  Hits without a
//                             usable SPE value are omitted.
//   MC   MCHit::GetCharge() -> already PE (tank_spe_charge_map == nullptr).
template <typename HitT, typename ClusterT>
bool AppendTankClustersImpl(
    const std::map<double, std::vector<HitT> >& clusters,
    const std::map<double, std::vector<unsigned long> >& detkey_map,
    const std::string& source_label,
    const std::map<int, double>* tank_spe_charge_map,
    std::vector<ClusterT>& out, std::string& error) {
  out.reserve(out.size() + clusters.size());
  for (typename std::map<double, std::vector<HitT> >::const_iterator it =
           clusters.begin();
       it != clusters.end(); ++it) {
    const std::map<double, std::vector<unsigned long> >::const_iterator dit =
        detkey_map.find(it->first);
    if (dit == detkey_map.end() || dit->second.size() != it->second.size()) {
      error = source_label + "/ClusterMapDetkey hit-vector mismatch";
      return false;
    }
    ClusterT cluster;
    cluster.time_ns = it->first;
    cluster.detkeys.reserve(it->second.size());
    cluster.hit_times_ns.reserve(it->second.size());
    cluster.hit_charges_pe.reserve(it->second.size());
    for (size_t hi = 0; hi < it->second.size(); ++hi) {
      double charge_pe = it->second[hi].GetCharge();
      if (tank_spe_charge_map != nullptr) {
        const std::map<int, double>::const_iterator spe_it =
            tank_spe_charge_map->find(static_cast<int>(it->second[hi].GetTubeId()));
        if (spe_it == tank_spe_charge_map->end() ||
            !std::isfinite(spe_it->second) || spe_it->second <= 0.0)
          continue;
        charge_pe /= spe_it->second;
      }
      cluster.detkeys.push_back(static_cast<unsigned long long>(dit->second[hi]));
      cluster.hit_times_ns.push_back(it->second[hi].GetTime());
      cluster.hit_charges_pe.push_back(charge_pe);
    }
    out.push_back(std::move(cluster));
  }
  return true;
}

// ---------------------------------------------------------------------------
// Response field access
// ---------------------------------------------------------------------------
// Boost.PropertyTree keeps JSON scalars as text, so every field is converted
// here with the strict parsers above.  Lookups are done relative to an
// already-resolved object node (gate / reconstruction / diagnostics) rather
// than re-walking a dotted path from the root for every key.

const ptree* Child(const ptree& node, const char* key) {
  const ptree::const_assoc_iterator it = node.find(key);
  return it == node.not_found() ? nullptr : &it->second;
}

// The daemon contract guarantees these fields are an object or JSON null.
const ptree* NonNullObject(const ptree& node, const char* key) {
  const ptree* child = Child(node, key);
  if (!child) return nullptr;
  if (child->empty() && child->data() == "null") return nullptr;
  return child;
}

class FieldReader {
public:
  FieldReader(const ptree& node, const char* prefix, std::string& error)
      : node_(node), prefix_(prefix), error_(error) {}

  // Required fields: on failure record a path-specific error and return false.
  bool Double(const char* key, double& v) {
    const ptree* c = Child(node_, key);
    return (c && ParseDouble(c->data(), v)) || Fail("number", key);
  }
  bool Int(const char* key, int& v) {
    const ptree* c = Child(node_, key);
    return (c && ParseInt(c->data(), v)) || Fail("integer", key);
  }
  bool Bool(const char* key, bool& v) {
    const ptree* c = Child(node_, key);
    return (c && ParseBool(c->data(), v)) || Fail("boolean", key);
  }
  bool String(const char* key, std::string& v) {
    const ptree* c = Child(node_, key);
    if (!c || !c->empty()) return Fail("string", key);
    v = c->data();
    return true;
  }
  bool Array(const char* key, std::vector<double>& v, size_t expected_size) {
    const ptree* c = Child(node_, key);
    if (!c) return Fail("array", key);
    std::vector<double> values;
    values.reserve(c->size());
    for (ptree::const_iterator it = c->begin(); it != c->end(); ++it) {
      double x = 0.0;
      if (!ParseDouble(it->second.data(), x)) return Fail("array number", key);
      values.push_back(x);
    }
    if (expected_size > 0 && values.size() != expected_size) {
      error_ = Path(key) + " has length " + std::to_string(values.size()) +
               ", expected " + std::to_string(expected_size);
      return false;
    }
    v.swap(values);
    return true;
  }

  // Optional fields: leave the sentinel untouched when missing/null/invalid.
  void OptDouble(const char* key, double& v) const {
    const ptree* c = Child(node_, key);
    double x;
    if (c && ParseDouble(c->data(), x)) v = x;
  }
  void OptInt(const char* key, int& v) const {
    const ptree* c = Child(node_, key);
    int x;
    if (c && ParseInt(c->data(), x)) v = x;
  }
  void OptBool(const char* key, bool& v) const {
    const ptree* c = Child(node_, key);
    bool x;
    if (c && ParseBool(c->data(), x)) v = x;
  }
  void OptString(const char* key, std::string& v) const {
    const ptree* c = Child(node_, key);
    if (c && c->empty() && c->data() != "null") v = c->data();
  }
  void OptIntArray(const char* key, std::vector<int>& v) const {
    const ptree* c = Child(node_, key);
    if (!c) return;
    std::vector<int> values;
    for (ptree::const_iterator it = c->begin(); it != c->end(); ++it) {
      int x;
      if (!ParseInt(it->second.data(), x)) return;
      values.push_back(x);
    }
    v.swap(values);
  }

private:
  std::string Path(const char* key) const {
    return prefix_[0] ? std::string(prefix_) + "." + key : std::string(key);
  }
  bool Fail(const char* type, const char* key) {
    error_ = std::string("missing or invalid JSON ") + type + ": " + Path(key);
    return false;
  }
  const ptree& node_;
  const char* prefix_;
  std::string& error_;
};

bool IsProbability(double v) { return v >= 0.0 && v <= 1.0; }

// Match the Python daemon softmax tolerance (2e-3) so C++ accepts the same
// numerically valid probability vectors as the daemon.
// This avoids spurious protocol failures and daemon restarts.
bool IsProbabilityVector(const std::vector<double>& values) {
  if (values.empty()) return false;
  double sum = 0.0;
  for (size_t i = 0; i < values.size(); ++i) {
    if (!IsProbability(values[i])) return false;
    sum += values[i];
  }
  return std::fabs(sum - 1.0) <= 2.0e-3;
}

bool IsNonNegativeVector(const std::vector<double>& values) {
  for (size_t i = 0; i < values.size(); ++i)
    if (values[i] < 0.0) return false;
  return true;
}

}  // namespace

// ===========================================================================
// Struct defaults: every field starts in a deterministic sentinel state.
//   counts / indices / classes : -1
//   probabilities              : -1.0
//   physical scalars / vectors : -999.0
// ===========================================================================

cMrdMlReco::EventIdentity::EventIdentity()
    : run_number(0), subrun_number(0), event_number(-1), mc_event_num(0),
      mc_trigger_num(-1), is_data(false) {}

cMrdMlReco::MRDPacket::MRDPacket() : source("empty"), num_time_clusters_store(0) {}

cMrdMlReco::TankClusterPacket::TankClusterPacket() : time_ns(0.0) {}

cMrdMlReco::TankPacket::TankPacket() : source("empty") {}

cMrdMlReco::ReleaseIdentity::ReleaseIdentity()
    : valid(false), protocol_version(-1), gate_has_target_threshold(-1.0),
      gate_reconstructable_threshold(-1.0), require_gate_category_zero(false) {}

cMrdMlReco::Diagnostics::Diagnostics()
    : present(false), number_mrd_time_clusters(-1), selected_mrd_cluster_index(-1),
      selected_mrd_cluster_start_time_ns(kFloatSentinel),
      selected_mrd_cluster_end_time_ns(kFloatSentinel),
      selected_mrd_cluster_time_span_ns(kFloatSentinel), input_mrd_raw_hits(-1),
      input_mrd_selected_raw_hits(-1),
      input_mrd_geometry_valid_hits_before_dead_mask(-1),
      input_mrd_known_dead_removed(-1), input_mrd_live_hits_before_dedup(-1),
      input_mrd_live_unique_hits(-1), input_mrd_dedup_removed(-1),
      input_mrd_unassigned_hits(-1), input_mrd_layers(-1),
      input_mrd_first_layer(-1), input_mrd_last_layer(-1),
      number_tank_clusters(-1), selected_tank_cluster_index(-1),
      selected_tank_cluster_time_ns(kFloatSentinel),
      selected_tank_cluster_charge_pe(kFloatSentinel),
      selected_tank_cluster_time_span_ns(kFloatSentinel), input_tank_hits(-1),
      tank_packet_reordered_by_time(false), reco_structurally_eligible(false),
      physics_is_raw_unstandardized(false),
      tank_mrd_association_applied(false), tank_mrd_association_found(false),
      tank_mrd_compatible_pair_count(0), tank_mrd_selected_tank_cluster_index(-1),
      tank_mrd_selected_mrd_subevent_id(-1),
      tank_mrd_expected_offset_ns(kFloatSentinel),
      tank_mrd_coincidence_window_ns(kFloatSentinel),
      tank_mrd_tank_mean_time_ns(kFloatSentinel),
      tank_mrd_mrd_mean_time_ns(kFloatSentinel), tank_mrd_dt_ns(kFloatSentinel),
      tank_mrd_timing_residual_ns(kFloatSentinel) {}

cMrdMlReco::GateResult::GateResult()
    : present(false), has_target_probability(kProbabilitySentinel),
      reconstructable_probability(kProbabilitySentinel), event_category(-1),
      passed(false), event_category_confidence(kProbabilitySentinel),
      event_category_margin(kProbabilitySentinel),
      event_category_probabilities(kNumEventCategories, kProbabilitySentinel),
      single_muon_compatibility_probability(kProbabilitySentinel),
      single_muon_compatibility_validated(false) {}

cMrdMlReco::ReconstructionResult::ReconstructionResult()
    : present(false), topology_class(-1),
      topology_probabilities(kNumTopologyClasses, kProbabilitySentinel),
      stopped_probability(kProbabilitySentinel), exit_face_class(-1),
      mrd_track_length_cm(kFloatSentinel), mrd_track_length_sigma_cm(kFloatSentinel),
      cos_theta(kFloatSentinel), cos_theta_sigma(kFloatSentinel),
      theta_rad(kFloatSentinel), theta_sigma_rad(kFloatSentinel),
      theta_sigma_valid(false), phi_rad(kFloatSentinel),
      penetration_cm(kFloatSentinel), penetration_sigma_cm(kFloatSentinel),
      mrd_energy_loss_mev(kFloatSentinel), mrd_energy_loss_sigma_mev(kFloatSentinel),
      muon_ke_mev(kFloatSentinel), muon_ke_sigma_mev(kFloatSentinel),
      cherenkov_angle_rad(kFloatSentinel), cherenkov_angle_sigma_rad(kFloatSentinel),
      tank_vertex_cm(SentinelVector(3)), tank_vertex_sigma_cm(SentinelVector(3)),
      tank_exit_cm(SentinelVector(3)), tank_exit_sigma_cm(SentinelVector(3)),
      mrd_entry_cm(SentinelVector(3)), mrd_entry_sigma_cm(SentinelVector(3)),
      mrd_exit_cm(SentinelVector(3)), mrd_exit_sigma_cm(SentinelVector(3)),
      selected_endpoint_cm(SentinelVector(3)),
      selected_endpoint_sigma_cm(SentinelVector(3)),
      mrd_scatter_rad(SentinelVector(3)), mrd_scatter_sigma_rad(SentinelVector(3)),
      tank_scatter_rad(SentinelVector(3)), tank_scatter_sigma_rad(SentinelVector(3)),
      topology_confidence(kProbabilitySentinel), topology_margin(kProbabilitySentinel),
      topology_p_stopped(kProbabilitySentinel),
      topology_p_back_exit(kProbabilitySentinel),
      topology_p_side_exit(kProbabilitySentinel),
      exit_face_confidence(kProbabilitySentinel),
      exit_face_margin(kProbabilitySentinel),
      exit_face_probabilities(kNumExitFaces, kProbabilitySentinel),
      endpoint_expert_used(-1), sin_phi(kFloatSentinel), cos_phi(kFloatSentinel),
      phi_vector_norm(kFloatSentinel),
      stop_endpoint_cm(SentinelVector(3)), stop_endpoint_sigma_cm(SentinelVector(3)),
      back_exit_endpoint_cm(SentinelVector(3)),
      back_exit_endpoint_sigma_cm(SentinelVector(3)),
      side_exit_endpoint_cm(SentinelVector(3)),
      side_exit_endpoint_sigma_cm(SentinelVector(3)),
      quality_baseline(false), quality_topology(false), quality_direction(false),
      quality_endpoint(false), quality_tier(0) {}

cMrdMlReco::DaemonResponse::DaemonResponse()
    : parsed(false), protocol_version(-1), ok(false), status("not_processed"),
      failure_code(-1), reco_skip_reason(0), input_build_ok(false),
      gate_attempted(false), gate_inference_ok(false), gate_passed(false),
      reco_attempted(false), reco_inference_ok(false), reco_used_tank_bypass(false) {}

// ===========================================================================
// Lifecycle and configuration
// ===========================================================================
// The alpha release taught us that configuration drift is as dangerous as a
// code change.  The constructor therefore mirrors the frozen production
// policy, while ValidateConfiguration() turns unsupported combinations into
// explicit startup failures.  Only environment-specific release paths must be
// supplied by the config file.
//
// Initialise() also establishes the producer/consumer handshake used by
// PhaseIITreeMaker.  If the ToolChain order is wrong, the job should fail
// before processing events rather than silently emit a partial tree.

// Defaults equal the frozen production configuration (cMrdMlRecoConfig), so a
// missing config key can never silently resurrect an older policy.  Paths to
// the release on PNFS have no default and must be configured.
cMrdMlReco::cMrdMlReco()
    : Tool(), verbosity_(1), is_data_(false), output_mode_(kOutputProduction),
      output_mode_name_("production"),
      python_exe_("python3"), model_release_("cMrdMlReco-B2v042"),
      expected_geometry_manifest_sha256_(
          "8c63f966f8a3948d43f2ee3f71c41610adc0d72e3cb9cbbdcb84383f878fe8ab"),
      expected_gate_model_tree_sha256_(
          "b72962be6eaf03b501c996aa858b933e9104c264ca556e37dfb28105714fd240"),
      expected_reconstruction_model_tree_sha256_(
          "27aba803f3bc5f5364281fda98b85f4ba2016fc98f67c29605616cf8744cbc36"),
      gate_has_target_threshold_(0.5), gate_reconstructable_threshold_(0.0),
      require_gate_category_zero_(false), allow_tf_version_mismatch_(false),
      tensorflow_intra_op_threads_(1), tensorflow_inter_op_threads_(1),
      daemon_startup_timeout_sec_(240), daemon_response_timeout_sec_(60),
      daemon_shutdown_timeout_sec_(5), restart_daemon_on_failure_(true),
      max_daemon_restarts_(3), prefilter_min_mrd_hits_(0),
      mc_tdc_data_key_("TDCData"), debug_packet_max_events_(0),
      write_mrdtracks_compatibility_(false),
      write_muonfitter_compatibility_(false), max_protocol_line_bytes_(8u * 1024u * 1024u),
      geom_(nullptr), native_store_(nullptr), event_counter_(0),
      daemon_request_counter_(0), daemon_failure_counter_(0),
      prefilter_skip_counter_(0), daemon_restart_count_(0), daemon_pid_(-1),
      daemon_stdin_fd_(-1), daemon_stdout_fd_(-1), daemon_alive_(false) {}

void cMrdMlReco::LoadConfiguration(const std::string& configfile) {
  configuration_errors_.clear();
  if (!configfile.empty()) m_variables.Initialise(configfile);

  m_variables.Get("verbosity", verbosity_);
  m_variables.Get("IsData", is_data_);
  // The output Store name is part of the interface with PhaseIITreeMaker and
  // is fixed.  StoreName is still accepted, but only with the fixed value.
  std::string store_name = kStoreName;
  m_variables.Get("StoreName", store_name);
  if (store_name != kStoreName)
    configuration_errors_.push_back("StoreName is fixed to '" + std::string(kStoreName) +
                                    "'; got '" + store_name + "' (remove the key)");
  m_variables.Get("PythonExe", python_exe_);
  m_variables.Get("InferenceScript", inference_script_);
  m_variables.Get("GateModelDir", gate_model_dir_);
  m_variables.Get("ReconstructionModelDir", reconstruction_model_dir_);
  m_variables.Get("GateModelMeta", gate_model_meta_);
  m_variables.Get("ReconstructionModelMeta", reconstruction_model_meta_);
  m_variables.Get("GeometryDir", geometry_dir_);
  m_variables.Get("ModelRelease", model_release_);
  m_variables.Get("ExpectedGeometryManifestSHA256", expected_geometry_manifest_sha256_);
  m_variables.Get("ExpectedGateModelTreeSHA256", expected_gate_model_tree_sha256_);
  m_variables.Get("ExpectedReconstructionModelTreeSHA256",
                  expected_reconstruction_model_tree_sha256_);
  m_variables.Get("GateHasTargetThreshold", gate_has_target_threshold_);
  m_variables.Get("GateReconstructableThreshold", gate_reconstructable_threshold_);
  m_variables.Get("RequireGateCategoryZero", require_gate_category_zero_);
  m_variables.Get("AllowTFVersionMismatch", allow_tf_version_mismatch_);
  m_variables.Get("TensorFlowIntraOpThreads", tensorflow_intra_op_threads_);
  m_variables.Get("TensorFlowInterOpThreads", tensorflow_inter_op_threads_);
  m_variables.Get("DaemonStartupTimeoutSec", daemon_startup_timeout_sec_);
  m_variables.Get("DaemonResponseTimeoutSec", daemon_response_timeout_sec_);
  m_variables.Get("DaemonShutdownTimeoutSec", daemon_shutdown_timeout_sec_);
  m_variables.Get("RestartDaemonOnFailure", restart_daemon_on_failure_);
  m_variables.Get("MaxDaemonRestarts", max_daemon_restarts_);
  m_variables.Get("PrefilterMinMRDHits", prefilter_min_mrd_hits_);
  m_variables.Get("MCTDCDataKey", mc_tdc_data_key_);
  m_variables.Get("DebugPacketDirectory", debug_packet_directory_);
  m_variables.Get("DebugPacketMaxEvents", debug_packet_max_events_);
  m_variables.Get("DebugResponseFile", debug_response_file_);
  m_variables.Get("WriteMRDTracksCompatibility", write_mrdtracks_compatibility_);
  m_variables.Get("WriteMuonFitterCompatibility", write_muonfitter_compatibility_);

  // OutputMode: production | debug. Legacy smoke-test aliases map to debug.
  std::string mode = output_mode_name_;
  m_variables.Get("OutputMode", mode);
  std::string lowered = mode;
  std::transform(lowered.begin(), lowered.end(), lowered.begin(), ::tolower);
  if (lowered == "production") {
    output_mode_ = kOutputProduction;
    output_mode_name_ = "production";
  } else if (lowered == "debug" || lowered == "smoke-test" ||
             lowered == "smoke_test" || lowered == "smoketest") {
    output_mode_ = kOutputDebug;
    output_mode_name_ = "debug";
  } else {
    configuration_errors_.push_back(
        "OutputMode must be 'production' or 'debug' (alias 'smoke-test'); got '" +
        mode + "'");
  }

  // MaxProtocolLineMB is in MiB.
  int max_line_mb = 8;
  m_variables.Get("MaxProtocolLineMB", max_line_mb);
  const size_t bytes_per_mb = 1024u * 1024u;
  if (max_line_mb <= 0) {
    configuration_errors_.push_back("MaxProtocolLineMB must be positive");
  } else if (static_cast<size_t>(max_line_mb) >
             std::numeric_limits<size_t>::max() / bytes_per_mb) {
    configuration_errors_.push_back("MaxProtocolLineMB is too large");
  } else {
    max_protocol_line_bytes_ = static_cast<size_t>(max_line_mb) * bytes_per_mb;
  }

  // Optional current-condition dead channels.  Recorded for diagnostics only;
  // the frozen model dead mask is not modified.
  std::string additional_dead;
  m_variables.Get("AdditionalDeadMRDChankeys", additional_dead);
  std::string parse_error;
  if (!ParseIntList(additional_dead, additional_dead_mrd_chankeys_, parse_error))
    configuration_errors_.push_back("AdditionalDeadMRDChankeys: " + parse_error);
}

bool cMrdMlReco::ValidateConfiguration() {
  std::vector<std::string> errors = configuration_errors_;
  if (python_exe_.empty()) errors.push_back("PythonExe is empty");
  if (!FileExists(inference_script_))
    errors.push_back("InferenceScript missing: " + inference_script_);
  if (!DirectoryExists(gate_model_dir_))
    errors.push_back("GateModelDir missing: " + gate_model_dir_);
  if (!DirectoryExists(reconstruction_model_dir_))
    errors.push_back("ReconstructionModelDir missing: " + reconstruction_model_dir_);
  if (!FileExists(gate_model_meta_))
    errors.push_back("GateModelMeta missing: " + gate_model_meta_);
  if (!FileExists(reconstruction_model_meta_))
    errors.push_back("ReconstructionModelMeta missing: " + reconstruction_model_meta_);
  if (!DirectoryExists(geometry_dir_))
    errors.push_back("GeometryDir missing: " + geometry_dir_);
  if (!std::isfinite(gate_has_target_threshold_) || gate_has_target_threshold_ < 0.0 ||
      gate_has_target_threshold_ > 1.0)
    errors.push_back("GateHasTargetThreshold must be in [0,1]");
  if (!std::isfinite(gate_reconstructable_threshold_) ||
      gate_reconstructable_threshold_ < 0.0 || gate_reconstructable_threshold_ > 1.0)
    errors.push_back("GateReconstructableThreshold must be in [0,1]");
  if (daemon_startup_timeout_sec_ <= 0)
    errors.push_back("DaemonStartupTimeoutSec must be positive");
  if (daemon_response_timeout_sec_ <= 0)
    errors.push_back("DaemonResponseTimeoutSec must be positive");
  if (max_daemon_restarts_ < 0)
    errors.push_back("MaxDaemonRestarts must be nonnegative");
  if (prefilter_min_mrd_hits_ < 0)
    errors.push_back("PrefilterMinMRDHits must be nonnegative");
  if (write_mrdtracks_compatibility_ || write_muonfitter_compatibility_)
    errors.push_back("legacy compatibility publication is not implemented; set both "
                     "WriteMRDTracksCompatibility and WriteMuonFitterCompatibility to 0");
  if (CLocale() == (locale_t)0)
    errors.push_back("could not create the C locale used for JSON number I/O");

  if (!errors.empty()) {
    for (size_t i = 0; i < errors.size(); ++i)
      Log("cMrdMlReco config error: " + errors[i], v_error, verbosity_);
    return false;
  }

  // Non-fatal notes.
  if (prefilter_min_mrd_hits_ > 4)
    Log("cMrdMlReco: PrefilterMinMRDHits > 4 can skip events the daemon would "
        "have reconstructed (reconstruction needs >= 4 live MRD hits)",
        v_warning, verbosity_);
  if (!DebugMode() && (!debug_packet_directory_.empty() || !debug_response_file_.empty()))
    Log("cMrdMlReco: DebugPacketDirectory/DebugResponseFile are ignored in "
        "OutputMode production",
        v_warning, verbosity_);
  return true;
}

bool cMrdMlReco::EnsureNativeStore() {
  if (!m_data) return false;
  if (m_data->Stores.count(kStoreName) == 0 || m_data->Stores[kStoreName] == nullptr)
    m_data->Stores[kStoreName] = new BoostStore(false, 2);
  native_store_ = m_data->Stores[kStoreName];
  return native_store_ != nullptr;
}

// The AnnieGeometry pointer only confirms the ToolChain detector context; the
// frozen geometry tables used for features live in the Python release.
// channelkey_to_mrdpmtid is assumed fixed for the job and cached here.
bool cMrdMlReco::RefreshRuntimeGeometryAndMapping() {
  geom_ = nullptr;
  if (!m_data || m_data->Stores.count("ANNIEEvent") == 0 ||
      !m_data->Stores.at("ANNIEEvent") || !m_data->Stores.at("ANNIEEvent")->Header)
    return false;
  if (!m_data->Stores.at("ANNIEEvent")->Header->Get("AnnieGeometry", geom_) || !geom_)
    return false;
  channelkey_to_mrdpmtid_.clear();
  return m_data->CStore.Get("channelkey_to_mrdpmtid", channelkey_to_mrdpmtid_) &&
         !channelkey_to_mrdpmtid_.empty();
}

bool cMrdMlReco::Initialise(std::string configfile, DataModel& data) {
  m_data = &data;
  LoadConfiguration(configfile);
  Log(std::string("cMrdMlReco: initialising ") + kAdapterVersion + " for release " +
          model_release_ + ", OutputMode=" + output_mode_name_,
      v_message, verbosity_);

  if (!ValidateConfiguration()) return false;
  if (!EnsureNativeStore()) {
    Log("cMrdMlReco: could not create native MrdMLReco store", v_error, verbosity_);
    return false;
  }
  if (!RefreshRuntimeGeometryAndMapping()) {
    Log("cMrdMlReco: missing AnnieGeometry or channelkey_to_mrdpmtid at "
        "Initialise. Check LoadGeometry/LoadWCSim/TimeClustering order.",
        v_error, verbosity_);
    return false;
  }

  // Producer-mode handshake read by PhaseIITreeMaker::Initialise().
  m_data->CStore.Set("MrdMLOutputMode", output_mode_name_);

  if (!StartDaemon()) {
    Log("cMrdMlReco: fatal daemon startup failure", v_error, verbosity_);
    return false;
  }

  Log(std::string("cMrdMlReco: initialised; outputs in Stores[\"") + kStoreName + "\"]",
      v_message, verbosity_);
  return true;
}

// ===========================================================================
// Per-event execution
// ===========================================================================
// Event flow is intentionally linear and has a single publication point:
//
//   read identity -> check geometry/mapping -> collect MRD + Tank input
//   -> (optional prefilter) -> serialize -> daemon round trip -> parse
//   -> Publish() exactly once.
//
// This is an important invariant.  Partial results are accumulated only in the
// local DaemonResponse object; the BoostStore is not incrementally mutated as
// the event progresses.  That prevents a late failure from leaving a mixture
// of current-event and previous-event values in Stores["MrdMLReco"].
//
// Event-level failures are recorded in the Store and Execute() returns true so
// the ToolChain continues.  Startup/release-contract failures make Initialise()
// return false.

bool cMrdMlReco::Execute() {
  ++event_counter_;
  EventIdentity identity;
  DaemonResponse result;
  ProcessEvent(identity, result);
  Publish(identity, result);
  return true;
}

void cMrdMlReco::MarkTechnicalFailure(DaemonResponse& result, int failure_code,
                                      const std::string& status,
                                      const std::string& error) {
  result.protocol_version = kProtocolVersion;
  result.ok = false;
  result.status = status;
  result.error = error;
  result.failure_code = failure_code;
  result.reco_skip_reason = 0;
  Log("cMrdMlReco event failure code=" + std::to_string(failure_code) +
          " status=" + status + " error=" + error,
      v_warning, verbosity_);
}

void cMrdMlReco::ProcessEvent(EventIdentity& identity, DaemonResponse& result) {
  std::string error;
  if (!ReadEventIdentity(identity, error)) {
    MarkTechnicalFailure(result, kFailureANNIEEventMissing, "annieevent_missing", error);
    return;
  }

  // Processed-data entries may replace the ANNIEEvent Header without the
  // runtime AnnieGeometry pointer; refresh it when present, else keep the
  // pointer validated at Initialise().
  Geometry* event_geometry = nullptr;
  BoostStore* header = m_data->Stores.at("ANNIEEvent")->Header;
  if (header && header->Get("AnnieGeometry", event_geometry) && event_geometry)
    geom_ = event_geometry;
  if (!geom_) {
    MarkTechnicalFailure(result, kFailureAnnieGeometryMissing, "annie_geometry_missing",
                         "no cached or event-level AnnieGeometry pointer");
    return;
  }
  if (channelkey_to_mrdpmtid_.empty()) {
    MarkTechnicalFailure(result, kFailureMRDMappingMissing, "mrd_mapping_missing",
                         "cached channelkey_to_mrdpmtid mapping is empty");
    return;
  }

  int failure_code = kFailureNone;
  MRDPacket mrd;
  if (!CollectMRDInput(mrd, failure_code, error)) {
    MarkTechnicalFailure(result, failure_code, "event_packet_input_error", error);
    return;
  }

  // Optional prefilter.  Reconstruction requires >= 4 live unique MRD hits in
  // the selected cluster, which is a subset of all MRD digits, so with a
  // threshold <= 4 no reconstructable event is lost.  Only the gate
  // probabilities of these events are not computed.
  if (prefilter_min_mrd_hits_ > 0 &&
      mrd.digit_times_ns.size() < static_cast<size_t>(prefilter_min_mrd_hits_)) {
    ++prefilter_skip_counter_;
    result.protocol_version = kProtocolVersion;
    result.ok = true;
    result.status = "prefilter_insufficient_mrd_hits";
    result.failure_code = kFailureNone;
    result.reco_skip_reason = kSkipReasonPrefilter;
    return;
  }

  TankPacket tank;
  if (!CollectTankInput(tank, failure_code, error)) {
    MarkTechnicalFailure(result, failure_code, "event_packet_input_error", error);
    return;
  }

  std::string packet;
  if (!BuildEventPacketJSON(identity, mrd, tank, packet, error)) {
    MarkTechnicalFailure(result, kFailurePacketSerialization, "event_packet_input_error",
                         error);
    return;
  }
  MaybeWriteDebugPacket(identity, packet);

  if (!EnsureDaemonAvailable()) {
    const int code = (restart_daemon_on_failure_ &&
                      daemon_restart_count_ >= max_daemon_restarts_)
                         ? kFailureDaemonRestartExhausted
                         : kFailureDaemonUnavailable;
    MarkTechnicalFailure(result, code, "daemon_unavailable",
                         "persistent Python daemon is unavailable");
    return;
  }

  // After any transport or protocol error the C++ side cannot prove the
  // daemon is still in sync, so it is stopped and restarted on a later event.
  std::string response_line;
  if (!SendPacketAndReceive(packet, response_line, failure_code, error)) {
    ++daemon_failure_counter_;
    MarkTechnicalFailure(result, failure_code, "daemon_transport_error", error);
    StopDaemon();
    return;
  }
  MaybeWriteDebugResponse(identity, response_line);

  DaemonResponse parsed;
  if (!ParseDaemonResponse(response_line, parsed, error)) {
    ++daemon_failure_counter_;
    MarkTechnicalFailure(result, kFailureDaemonProtocol, "daemon_response_parse_error",
                         error);
    StopDaemon();
    return;
  }
  if (!parsed.ok)
    Log("cMrdMlReco daemon event failure code=" + std::to_string(parsed.failure_code) +
            " status=" + parsed.status + " error=" + parsed.error,
        v_warning, verbosity_);
  std::swap(result, parsed);
}

bool cMrdMlReco::Finalise() {
  StopDaemon();
  Log("cMrdMlReco: finalised; events=" + std::to_string(event_counter_) +
          " daemon_requests=" + std::to_string(daemon_request_counter_) +
          " daemon_failures=" + std::to_string(daemon_failure_counter_) +
          " daemon_restarts=" + std::to_string(daemon_restart_count_) +
          " prefilter_skips=" + std::to_string(prefilter_skip_counter_),
      v_message, verbosity_);
  return true;
}

// ===========================================================================
// Input collection
// ===========================================================================
// C++ forwards detector-level products only.  It does not select the final
// MRD/Tank cluster, apply the frozen dead mask, build geometry embeddings, or
// assemble model tensors.  Those operations are part of the trained feature
// contract and stay in cmrd_v5_runtime_features.py / cmrd_v5_features.py.
//
// Preferred MRD source: TimeClustering CStore products, because they retain the
// subevent structure used during training.  Raw TDCData is only the inclusive
// fallback used when those products are absent/inconsistent; Python records the
// source and will not allow the MRD-only Tank bypass on raw-TDC fallback input.
//
// Tank input is similarly forwarded cluster-by-cluster.  C++ converts data
// charge to PE because that calibration is a ToolAnalysis runtime product, but
// it does not choose the winning Tank cluster.

// Provenance fields are optional and may be stored with different integral
// types; missing fields keep their defaults.
bool cMrdMlReco::ReadEventIdentity(EventIdentity& identity, std::string& error) const {
  identity = EventIdentity();
  identity.is_data = is_data_;
  if (!m_data || m_data->Stores.count("ANNIEEvent") == 0 ||
      !m_data->Stores.at("ANNIEEvent")) {
    error = "ANNIEEvent store is missing";
    return false;
  }
  BoostStore* annie = m_data->Stores.at("ANNIEEvent");

  uint32_t u32 = 0;
  int value_i = 0;
  if (annie->Get("RunNumber", u32)) identity.run_number = static_cast<int>(u32);
  else if (annie->Get("RunNumber", value_i)) identity.run_number = value_i;

  u32 = 0;
  value_i = 0;
  if (annie->Get("SubrunNumber", u32)) identity.subrun_number = static_cast<int>(u32);
  else if (annie->Get("SubrunNumber", value_i)) identity.subrun_number = value_i;

  u32 = 0;
  value_i = -1;
  if (annie->Get("EventNumber", u32)) identity.event_number = static_cast<int>(u32);
  else if (annie->Get("EventNumber", value_i)) identity.event_number = value_i;

  uint64_t event_u64 = 0;
  if (annie->Get("MCEventNum", event_u64)) identity.mc_event_num = event_u64;

  uint16_t trigger_u16 = 0;
  value_i = -1;
  if (annie->Get("MCTriggernum", trigger_u16))
    identity.mc_trigger_num = static_cast<int>(trigger_u16);
  else if (annie->Get("MCTriggernum", value_i))
    identity.mc_trigger_num = value_i;
  return true;
}

// Raw-TDC fallback.  store_present reports whether a usable TDC map existed
// (distinct from whether collection succeeded).  Raw hits carry no
// TimeCluster assignment, so Python can use them for the inclusive gate but
// never forms a reconstruction candidate from them.
bool cMrdMlReco::CollectRawMRDInput(MRDPacket& out, bool& store_present,
                                    std::string& error) {
  store_present = false;
  out = MRDPacket();
  if (!m_data || m_data->Stores.count("ANNIEEvent") == 0 ||
      !m_data->Stores.at("ANNIEEvent")) {
    error = "ANNIEEvent unavailable for raw TDC fallback";
    return false;
  }
  BoostStore* annie = m_data->Stores.at("ANNIEEvent");

  if (is_data_) {
    std::map<unsigned long, std::vector<Hit> >* tdc = nullptr;
    if (!annie->Get("TDCData", tdc) || !tdc) return true;
    store_present = true;
    AppendRawMRDHits(*tdc, channelkey_to_mrdpmtid_, out.digit_chankeys,
                     out.digit_times_ns, out.digit_charges);
    out.source = out.digit_times_ns.empty() ? "empty" : "raw_tdc_data";
    return true;
  }

  std::map<unsigned long, std::vector<MCHit> >* tdc = nullptr;
  bool got = false;
  if (!mc_tdc_data_key_.empty()) got = annie->Get(mc_tdc_data_key_, tdc);
  if ((!got || !tdc) && mc_tdc_data_key_ != "TDCData") got = annie->Get("TDCData", tdc);
  if (!got || !tdc) return true;
  store_present = true;
  AppendRawMRDHits(*tdc, channelkey_to_mrdpmtid_, out.digit_chankeys,
                   out.digit_times_ns, out.digit_charges);
  out.source = out.digit_times_ns.empty() ? "empty" : "raw_tdc_mc";
  return true;
}

// Prefer the retained TimeClustering products in CStore when they are
// present and mutually consistent; otherwise fall back to fresh raw TDC hits
// for the inclusive gate.  No retained cluster and no TDCData is a valid,
// empty event.
bool cMrdMlReco::CollectMRDInput(MRDPacket& out, int& failure_code, std::string& error) {
  failure_code = kFailureNone;
  out = MRDPacket();

  std::vector<std::vector<int> > clusters;
  int nclusters_store = 0;
  const bool got_clusters = m_data->CStore.Get("MrdTimeClusters", clusters);
  const bool got_nclusters = m_data->CStore.Get("NumMrdTimeClusters", nclusters_store);

  if (got_nclusters && nclusters_store < 0) {
    failure_code = kFailureMRDInput;
    error = "NumMrdTimeClusters is negative";
    return false;
  }
  if (got_nclusters && got_clusters &&
      nclusters_store != static_cast<int>(clusters.size())) {
    failure_code = kFailureMRDInput;
    error = "NumMrdTimeClusters=" + std::to_string(nclusters_store) +
            " disagrees with MrdTimeClusters.size()=" + std::to_string(clusters.size());
    return false;
  }
  if (got_nclusters && !got_clusters && nclusters_store != 0) {
    failure_code = kFailureMRDInput;
    error = "NumMrdTimeClusters is nonzero but MrdTimeClusters is missing";
    return false;
  }

  bool clustered_vectors_inconsistent = false;
  if (got_clusters && !clusters.empty()) {
    std::vector<double> times;
    std::vector<int> pmts;
    std::vector<double> charges;
    std::vector<ULong64_t> chankeys64;
    bool ok = m_data->CStore.Get("MrdDigitTimes", times) &&
              m_data->CStore.Get("MrdDigitPmts", pmts) &&
              m_data->CStore.Get("MrdDigitCharges", charges);
    // Channel keys may be stored as ULong64_t or unsigned long.
    if (ok && !m_data->CStore.Get("MrdDigitChankeys", chankeys64)) {
      std::vector<unsigned long> chankeys_ul;
      if (m_data->CStore.Get("MrdDigitChankeys", chankeys_ul))
        chankeys64.assign(chankeys_ul.begin(), chankeys_ul.end());
      else
        ok = false;
    }
    if (ok && times.size() == pmts.size() && times.size() == charges.size() &&
        times.size() == chankeys64.size()) {
      out.source = "CStore";
      out.num_time_clusters_store =
          got_nclusters ? nclusters_store : static_cast<int>(clusters.size());
      out.time_clusters.swap(clusters);
      out.digit_times_ns.swap(times);
      out.digit_charges.swap(charges);
      out.digit_chankeys.assign(chankeys64.begin(), chankeys64.end());
      return true;
    }
    clustered_vectors_inconsistent = true;
  }

  bool raw_store_present = false;
  if (!CollectRawMRDInput(out, raw_store_present, error)) {
    failure_code = kFailureMRDInput;
    return false;
  }
  if (clustered_vectors_inconsistent) {
    if (!raw_store_present) {
      failure_code = kFailureMRDInput;
      error = "MrdDigit vectors were inconsistent and raw TDC fallback was unavailable";
      return false;
    }
    Log("cMrdMlReco: inconsistent clustered MRD digit vectors; using fresh "
        "raw-TDC input for the inclusive gate",
        v_warning, verbosity_);
  }
  out.num_time_clusters_store = 0;
  out.time_clusters.clear();
  return true;
}

// Tank input has no raw fallback.  A populated hit map without detector keys
// cannot be used (Python needs detkeys for the frozen geometry lookup).
// A missing/null ClusterMap is treated like an empty one.
bool cMrdMlReco::CollectTankInput(TankPacket& out, int& failure_code, std::string& error) {
  failure_code = kFailureNone;
  out = TankPacket();

  std::map<double, std::vector<unsigned long> >* detkey_map = nullptr;
  m_data->CStore.Get("ClusterMapDetkey", detkey_map);

  if (is_data_) {
    std::map<double, std::vector<Hit> >* clusters = nullptr;
    if (!m_data->CStore.Get("ClusterMap", clusters) || !clusters || clusters->empty())
      return true;
    if (!detkey_map) {
      failure_code = kFailureTankInput;
      error = "ClusterMap is populated but ClusterMapDetkey is missing";
      return false;
    }
    std::map<int, double> tank_spe_charge_map;
    if (!m_data->CStore.Get("ChannelNumToTankPMTSPEChargeMap", tank_spe_charge_map) ||
        tank_spe_charge_map.empty()) {
      failure_code = kFailureTankInput;
      error = "ClusterMap data requires ChannelNumToTankPMTSPEChargeMap for "
              "charge-to-PE conversion";
      return false;
    }
    out.source = "ClusterMap";
    if (!AppendTankClustersImpl(*clusters, *detkey_map, out.source,
                                &tank_spe_charge_map, out.clusters, error)) {
      failure_code = kFailureTankInput;
      return false;
    }
    return true;
  }

  std::map<double, std::vector<MCHit> >* clusters = nullptr;
  if (!m_data->CStore.Get("ClusterMapMC", clusters) || !clusters || clusters->empty())
    return true;
  if (!detkey_map) {
    failure_code = kFailureTankInput;
    error = "ClusterMapMC is populated but ClusterMapDetkey is missing";
    return false;
  }
  out.source = "ClusterMapMC";
  if (!AppendTankClustersImpl(*clusters, *detkey_map, out.source, nullptr,
                              out.clusters, error)) {
    failure_code = kFailureTankInput;
    return false;
  }
  return true;
}

// ===========================================================================
// JSON packet encoding (schema annie_cmrd_v5_event_packet v1, unchanged)
// ===========================================================================
// Packet compatibility is deliberately stricter than the v6 Store contract:
// the detector packet remains the frozen v5 schema so a new output/publication
// layer cannot accidentally retrain or reinterpret the model input.  Values and
// structure are serialized exactly as collected.  Any NaN/Inf in detector input
// is rejected here with the offending array named.
//
// Keep this function boring.  Changing field names, nesting, ordering-sensitive
// numeric formatting, or units is a model-interface change and requires a new
// packet contract plus parity validation.

bool cMrdMlReco::BuildEventPacketJSON(const EventIdentity& identity,
                                      const MRDPacket& mrd, const TankPacket& tank,
                                      std::string& json, std::string& error) const {
  ScopedCLocale c_locale;
  std::string out;
  size_t estimate = 512 + 64 * mrd.digit_times_ns.size();
  for (size_t ci = 0; ci < tank.clusters.size(); ++ci)
    estimate += 96 + 64 * tank.clusters[ci].hit_times_ns.size();
  out.reserve(estimate);

  out += "{\"schema\":\"annie_cmrd_v5_event_packet\",\"schema_version\":1,";
  out += "\"event\":{";
  out += "\"run_number\":" + std::to_string(identity.run_number) + ",";
  out += "\"subrun_number\":" + std::to_string(identity.subrun_number) + ",";
  out += "\"event_number\":" + std::to_string(identity.event_number) + ",";
  out += "\"mc_event_num\":" + std::to_string(identity.mc_event_num) + ",";
  out += "\"mc_trigger_num\":" + std::to_string(identity.mc_trigger_num) + ",";
  out += std::string("\"is_data\":") + (identity.is_data ? "true" : "false") + "},";

  out += "\"mrd\":{\"source\":\"" + EscapeJSON(mrd.source) + "\",";
  out += "\"num_time_clusters_store\":" + std::to_string(mrd.num_time_clusters_store) + ",";
  out += "\"digit_chankeys\":";
  AppendIntArray(out, mrd.digit_chankeys);
  out += ",\"digit_times_ns\":";
  if (!AppendDoubleArray(out, mrd.digit_times_ns)) {
    error = "nonfinite value in mrd.digit_times_ns (" + mrd.source + ")";
    return false;
  }
  out += ",\"digit_charges\":";
  if (!AppendDoubleArray(out, mrd.digit_charges)) {
    error = "nonfinite value in mrd.digit_charges (" + mrd.source + ")";
    return false;
  }
  out += ",\"time_clusters\":";
  AppendNestedIntArrays(out, mrd.time_clusters);
  out += "},";

  out += "\"tank\":{\"source\":\"" + EscapeJSON(tank.source) + "\",\"clusters\":[";
  for (size_t ci = 0; ci < tank.clusters.size(); ++ci) {
    const TankClusterPacket& cluster = tank.clusters[ci];
    if (ci) out += ',';
    out += "{\"time_ns\":";
    if (!AppendDouble(out, cluster.time_ns)) {
      error = "nonfinite tank cluster time (" + tank.source + ")";
      return false;
    }
    out += ",\"detkeys\":";
    AppendIntArray(out, cluster.detkeys);
    out += ",\"hit_times_ns\":";
    if (!AppendDoubleArray(out, cluster.hit_times_ns)) {
      error = "nonfinite tank hit time (" + tank.source + ")";
      return false;
    }
    out += ",\"hit_charges_pe\":";
    if (!AppendDoubleArray(out, cluster.hit_charges_pe)) {
      error = "nonfinite tank hit charge (" + tank.source + ")";
      return false;
    }
    out += '}';
  }
  out += "]},";
  out += "\"runtime_conditions\":{\"additional_dead_mrd_chankeys\":";
  AppendIntArray(out, additional_dead_mrd_chankeys_);
  out += "}}";

  if (out.size() > max_protocol_line_bytes_) {
    error = "event packet exceeds MaxProtocolLineMB";
    return false;
  }
  json.swap(out);
  return true;
}

// Debug dumps (OutputMode debug only).  File names include the tool's event
// counter so MC events sharing event_number=-1 no longer overwrite each other.
void cMrdMlReco::MaybeWriteDebugPacket(const EventIdentity& identity,
                                       const std::string& json) const {
  if (!DebugMode() || debug_packet_directory_.empty() || debug_packet_max_events_ <= 0 ||
      event_counter_ > debug_packet_max_events_)
    return;
  std::ostringstream path;
  path << debug_packet_directory_ << "/cmrd_v5_packet_run" << identity.run_number
       << "_subrun" << identity.subrun_number << "_event" << identity.event_number
       << "_n" << event_counter_ << ".json";
  std::ofstream out(path.str().c_str());
  if (out) out << json << '\n';
}

void cMrdMlReco::MaybeWriteDebugResponse(const EventIdentity& identity,
                                         const std::string& json) const {
  if (!DebugMode() || debug_response_file_.empty()) return;
  std::ofstream out(debug_response_file_.c_str(), std::ios::app);
  if (!out) return;
  out << identity.run_number << ',' << identity.subrun_number << ','
      << identity.event_number << '\t' << json << '\n';
}

// ===========================================================================
// Daemon process management
// ===========================================================================
// The Python process is persistent by design: TensorFlow/model loading happens
// once per job rather than once per trigger.  Stdout is reserved exclusively
// for the line protocol (READY, then one response per request); stderr remains
// attached for human-readable diagnostics.
//
// The daemon is not considered usable merely because execvp() succeeded.  The
// READY handshake must prove that C++ and Python agree on protocol version,
// release label, model hashes, geometry hash and gate policy.  Any transport or
// protocol ambiguity tears the process down; a later event may restart it from
// a fresh READY handshake.

bool cMrdMlReco::StartDaemon() {
  StopDaemon();

  // Build argv completely before fork(); the child only manipulates
  // descriptors and calls execvp().
  std::vector<std::string> args;
  {
    ScopedCLocale c_locale;
    char threshold_a[32], threshold_b[32];
    std::snprintf(threshold_a, sizeof(threshold_a), "%.17g", gate_has_target_threshold_);
    std::snprintf(threshold_b, sizeof(threshold_b), "%.17g",
                  gate_reconstructable_threshold_);
    args.push_back(python_exe_);
    args.push_back(inference_script_);
    args.push_back("--gate-model-dir");              args.push_back(gate_model_dir_);
    args.push_back("--reconstruction-model-dir");    args.push_back(reconstruction_model_dir_);
    args.push_back("--gate-model-meta");             args.push_back(gate_model_meta_);
    args.push_back("--reconstruction-model-meta");   args.push_back(reconstruction_model_meta_);
    args.push_back("--geometry-dir");                args.push_back(geometry_dir_);
    args.push_back("--model-release");               args.push_back(model_release_);
    args.push_back("--gate-has-target-threshold");   args.push_back(threshold_a);
    args.push_back("--gate-reconstructable-threshold"); args.push_back(threshold_b);
    args.push_back("--require-gate-category-zero");
    args.push_back(require_gate_category_zero_ ? "1" : "0");
    args.push_back("--tensorflow-intra-op-threads");
    args.push_back(std::to_string(tensorflow_intra_op_threads_));
    args.push_back("--tensorflow-inter-op-threads");
    args.push_back(std::to_string(tensorflow_inter_op_threads_));
    if (!expected_gate_model_tree_sha256_.empty()) {
      args.push_back("--expected-gate-model-tree-sha256");
      args.push_back(expected_gate_model_tree_sha256_);
    }
    if (!expected_reconstruction_model_tree_sha256_.empty()) {
      args.push_back("--expected-reconstruction-model-tree-sha256");
      args.push_back(expected_reconstruction_model_tree_sha256_);
    }
    if (allow_tf_version_mismatch_) args.push_back("--allow-tf-version-mismatch");
  }

  std::vector<char*> argv;
  argv.reserve(args.size() + 1);
  for (size_t i = 0; i < args.size(); ++i) argv.push_back(const_cast<char*>(args[i].c_str()));
  argv.push_back(nullptr);

  int to_child[2] = {-1, -1};
  int from_child[2] = {-1, -1};
  const auto close_pipes = [&]() {
    if (to_child[0] >= 0) close(to_child[0]);
    if (to_child[1] >= 0) close(to_child[1]);
    if (from_child[0] >= 0) close(from_child[0]);
    if (from_child[1] >= 0) close(from_child[1]);
  };
  if (pipe(to_child) != 0 || pipe(from_child) != 0) {
    Log("cMrdMlReco: pipe() failed: " + std::string(std::strerror(errno)), v_error,
        verbosity_);
    close_pipes();
    return false;
  }
  if (!SetCloseOnExec(to_child[0]) || !SetCloseOnExec(to_child[1]) ||
      !SetCloseOnExec(from_child[0]) || !SetCloseOnExec(from_child[1])) {
    Log("cMrdMlReco: could not set FD_CLOEXEC on daemon pipes: " +
            std::string(std::strerror(errno)),
        v_error, verbosity_);
    close_pipes();
    return false;
  }

  // The child must not inherit ROOT files, BoostStore files, logs, or other
  // tools' descriptors.  It prefers close_range(); otherwise it closes the
  // snapshot taken here; as a last resort every fd up to OPEN_MAX.
  const std::vector<int> child_fds_to_close = SnapshotOpenFileDescriptors();
  long open_max = sysconf(_SC_OPEN_MAX);
  if (open_max < 0) open_max = 65536;
  if (open_max > static_cast<long>(std::numeric_limits<int>::max()))
    open_max = static_cast<long>(std::numeric_limits<int>::max());
  const int child_close_limit_fallback = static_cast<int>(open_max);

  const pid_t pid = fork();
  if (pid < 0) {
    Log("cMrdMlReco: fork() failed: " + std::string(std::strerror(errno)), v_error,
        verbosity_);
    close_pipes();
    return false;
  }

  if (pid == 0) {
    // Child.  Exit codes: 126 = could not prepare exec, 127 = exec failed.
    if (dup2(to_child[0], STDIN_FILENO) < 0 || dup2(from_child[1], STDOUT_FILENO) < 0) {
      static const char message[] = "cMrdMlReco child: dup2 failed before execvp\n";
      ssize_t ignored = write(STDERR_FILENO, message, sizeof(message) - 1);
      (void)ignored;
      _exit(126);
    }
    bool closed_descriptor_range = false;
#ifdef SYS_close_range
    if (syscall(SYS_close_range, 3u, ~0u, 0u) == 0) closed_descriptor_range = true;
#endif
    if (!closed_descriptor_range && !child_fds_to_close.empty()) {
      for (size_t i = 0; i < child_fds_to_close.size(); ++i) close(child_fds_to_close[i]);
    } else if (!closed_descriptor_range) {
      for (int fd = 3; fd < child_close_limit_fallback; ++fd) close(fd);
    }
    execvp(argv[0], &argv[0]);
    static const char message[] = "cMrdMlReco child: execvp failed\n";
    ssize_t ignored = write(STDERR_FILENO, message, sizeof(message) - 1);
    (void)ignored;
    _exit(127);
  }

  daemon_pid_ = pid;
  daemon_stdin_fd_ = to_child[1];
  daemon_stdout_fd_ = from_child[0];
  close(to_child[0]);
  close(from_child[1]);
  daemon_alive_ = true;
  daemon_read_buffer_.clear();

  std::string ready_line;
  bool timed_out = false;
  if (!ReadProtocolLine(ready_line, daemon_startup_timeout_sec_, timed_out)) {
    Log("cMrdMlReco: daemon startup " +
            std::string(timed_out ? "timed out" : "failed before READY"),
        v_error, verbosity_);
    StopDaemon();
    return false;
  }
  std::string error;
  if (!ParseReadyLine(ready_line, error)) {
    Log("cMrdMlReco: invalid READY handshake: " + error, v_error, verbosity_);
    StopDaemon();
    return false;
  }

  PublishJobIdentity();
  Log("cMrdMlReco: Python daemon READY pid=" +
          std::to_string(static_cast<long long>(daemon_pid_)) +
          " geometry=" + release_identity_.geometry_version,
      v_message, verbosity_);
  return true;
}

// Staged shutdown: QUIT + close stdin, wait DaemonShutdownTimeoutSec, then
// SIGTERM (2 s), then SIGKILL.
void cMrdMlReco::StopDaemon() {
  const pid_t pid = daemon_pid_;
  if (daemon_stdin_fd_ >= 0) {
    static const char quit[] = "QUIT\n";
    WriteAllNoSigPipe(daemon_stdin_fd_, quit, sizeof(quit) - 1);
    close(daemon_stdin_fd_);
  }
  daemon_stdin_fd_ = -1;
  if (daemon_stdout_fd_ >= 0) close(daemon_stdout_fd_);
  daemon_stdout_fd_ = -1;
  daemon_alive_ = false;
  daemon_read_buffer_.clear();

  if (pid > 0) {
    const auto reaped = [pid]() -> bool {
      int status = 0;
      pid_t result = -1;
      do {
        result = waitpid(pid, &status, WNOHANG);
      } while (result < 0 && errno == EINTR);
      return result == pid || (result < 0 && errno == ECHILD);
    };
    bool exited = false;
    const int checks = std::max(1, daemon_shutdown_timeout_sec_ * 10);
    for (int i = 0; i < checks && !exited; ++i) {
      exited = reaped();
      if (!exited) usleep(100000);
    }
    if (!exited) {
      kill(pid, SIGTERM);
      for (int i = 0; i < 20 && !exited; ++i) {
        exited = reaped();
        if (!exited) usleep(100000);
      }
    }
    if (!exited) {
      int status = 0;
      kill(pid, SIGKILL);
      waitpid(pid, &status, 0);
    }
  }
  daemon_pid_ = -1;
}

bool cMrdMlReco::CheckDaemonAlive() {
  if (!daemon_alive_ || daemon_pid_ <= 0 || daemon_stdin_fd_ < 0 || daemon_stdout_fd_ < 0)
    return false;
  int status = 0;
  pid_t result = -1;
  do {
    result = waitpid(daemon_pid_, &status, WNOHANG);
  } while (result < 0 && errno == EINTR);
  if (result == 0) return true;
  daemon_alive_ = false;
  // Already reaped: forget the PID so StopDaemon never signals a reused PID.
  if (result == daemon_pid_ || (result < 0 && errno == ECHILD)) daemon_pid_ = -1;
  return false;
}

// daemon_restart_count_ counts restart attempts after the initial start,
// including attempts whose StartDaemon() fails.
bool cMrdMlReco::EnsureDaemonAvailable() {
  if (CheckDaemonAlive()) return true;
  StopDaemon();
  if (!restart_daemon_on_failure_) return false;
  if (daemon_restart_count_ >= max_daemon_restarts_) return false;
  ++daemon_restart_count_;
  return StartDaemon();
}

// Sends `line` plus a newline in a single write.  The newline is appended to
// and removed from the caller's buffer to avoid a copy and a second syscall.
// Liveness was just checked by EnsureDaemonAvailable(); a daemon that died
// since then surfaces as EPIPE here.
bool cMrdMlReco::WriteProtocolLine(std::string& line) {
  if (!daemon_alive_ || daemon_stdin_fd_ < 0 || line.size() > max_protocol_line_bytes_)
    return false;
  line.push_back('\n');
  const bool ok = WriteAllNoSigPipe(daemon_stdin_fd_, line.data(), line.size());
  line.pop_back();
  if (!ok) daemon_alive_ = false;
  return ok;
}

bool cMrdMlReco::ReadProtocolLine(std::string& line, int timeout_sec, bool& timed_out) {
  timed_out = false;
  line.clear();
  const std::chrono::steady_clock::time_point deadline =
      std::chrono::steady_clock::now() + std::chrono::seconds(timeout_sec);
  size_t scan_from = 0;

  while (true) {
    const size_t newline = daemon_read_buffer_.find('\n', scan_from);
    if (newline != std::string::npos) {
      // Enforce the size limit before returning any line (READY included).
      if (newline > max_protocol_line_bytes_) {
        daemon_alive_ = false;
        return false;
      }
      line.assign(daemon_read_buffer_, 0, newline);
      daemon_read_buffer_.erase(0, newline + 1);
      if (!line.empty() && line[line.size() - 1] == '\r') line.resize(line.size() - 1);
      return true;
    }
    scan_from = daemon_read_buffer_.size();
    if (daemon_read_buffer_.size() > max_protocol_line_bytes_) {
      daemon_alive_ = false;  // daemon keeps writing without a newline
      return false;
    }

    const std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
    if (now >= deadline) {
      timed_out = true;
      return false;
    }
    const long long remaining_ms =
        std::chrono::duration_cast<std::chrono::milliseconds>(deadline - now).count();
    struct pollfd pfd;
    pfd.fd = daemon_stdout_fd_;
    pfd.events = POLLIN | POLLHUP;
    pfd.revents = 0;
    const int result = poll(&pfd, 1, static_cast<int>(std::max<long long>(1, remaining_ms)));
    if (result < 0) {
      if (errno == EINTR) continue;
      daemon_alive_ = false;
      return false;
    }
    if (result == 0) {
      timed_out = true;
      return false;
    }
    if (pfd.revents & (POLLERR | POLLNVAL)) {
      daemon_alive_ = false;
      return false;
    }

    char buffer[16384];
    const ssize_t nread = read(daemon_stdout_fd_, buffer, sizeof(buffer));
    if (nread < 0) {
      if (errno == EINTR) continue;
      daemon_alive_ = false;
      return false;
    }
    if (nread == 0) {
      daemon_alive_ = false;
      return false;
    }
    daemon_read_buffer_.append(buffer, static_cast<size_t>(nread));
  }
}

// READY {json}: the daemon reports which release it loaded.  Protocol,
// release name, geometry and model hashes, and gate policy must all match
// the C++ configuration before any event is sent.
// READY is the release-integrity boundary.  Do not weaken these checks into
// warnings: accepting a daemon with the wrong SavedModel/geometry/thresholds
// would produce plausible-looking but irreproducible physics output.
bool cMrdMlReco::ParseReadyLine(const std::string& line, std::string& error) {
  const std::string prefix = "READY ";
  if (line.compare(0, prefix.size(), prefix) != 0) {
    error = "expected exact 'READY {json}' line, got: " + line;
    return false;
  }
  ptree tree;
  try {
    std::istringstream input(line.substr(prefix.size()));
    boost::property_tree::read_json(input, tree);
  } catch (const std::exception& exc) {
    error = std::string("READY JSON parse failure: ") + exc.what();
    return false;
  }

  ReleaseIdentity identity;
  FieldReader f(tree, "", error);
  if (!f.Int("protocol_version", identity.protocol_version) ||
      !f.String("daemon_version", identity.daemon_version) ||
      !f.String("runtime_feature_version", identity.runtime_feature_version) ||
      !f.String("model_release", identity.model_release) ||
      !f.String("gate_model_name", identity.gate_model_name) ||
      !f.String("reconstruction_model_name", identity.reconstruction_model_name) ||
      !f.String("gate_model_tree_sha256", identity.gate_model_tree_sha256) ||
      !f.String("reconstruction_model_tree_sha256",
                identity.reconstruction_model_tree_sha256) ||
      !f.String("geometry_version", identity.geometry_version) ||
      !f.String("geometry_manifest_sha256", identity.geometry_manifest_sha256) ||
      !f.String("gate_trainer_version", identity.gate_trainer_version) ||
      !f.String("reconstruction_trainer_version", identity.reconstruction_trainer_version) ||
      !f.String("tensorflow_version", identity.tensorflow_version) ||
      !f.Double("gate_has_target_threshold", identity.gate_has_target_threshold) ||
      !f.Double("gate_reconstructable_threshold",
                identity.gate_reconstructable_threshold) ||
      !f.Bool("require_gate_category_zero", identity.require_gate_category_zero))
    return false;

  if (identity.protocol_version != kProtocolVersion) {
    error = "daemon protocol version mismatch";
    return false;
  }
  if (identity.model_release != model_release_) {
    error = "READY model release mismatch: expected '" + model_release_ + "', got '" +
            identity.model_release + "'";
    return false;
  }
  if (!expected_geometry_manifest_sha256_.empty() &&
      identity.geometry_manifest_sha256 != expected_geometry_manifest_sha256_) {
    error = "READY geometry manifest hash mismatch";
    return false;
  }
  if (!expected_gate_model_tree_sha256_.empty() &&
      identity.gate_model_tree_sha256 != expected_gate_model_tree_sha256_) {
    error = "READY gate model tree hash mismatch";
    return false;
  }
  if (!expected_reconstruction_model_tree_sha256_.empty() &&
      identity.reconstruction_model_tree_sha256 !=
          expected_reconstruction_model_tree_sha256_) {
    error = "READY reconstruction model tree hash mismatch";
    return false;
  }
  if (std::fabs(identity.gate_has_target_threshold - gate_has_target_threshold_) > 1.0e-9 ||
      std::fabs(identity.gate_reconstructable_threshold - gate_reconstructable_threshold_) >
          1.0e-9 ||
      identity.require_gate_category_zero != require_gate_category_zero_) {
    error = "READY threshold policy does not match C++ configuration";
    return false;
  }
  identity.valid = true;
  release_identity_ = identity;
  return true;
}

bool cMrdMlReco::SendPacketAndReceive(std::string& packet, std::string& response_line,
                                      int& failure_code, std::string& error) {
  failure_code = kFailureNone;
  if (!WriteProtocolLine(packet)) {
    failure_code = kFailureDaemonUnavailable;
    error = "failed to write event packet to daemon";
    return false;
  }
  ++daemon_request_counter_;
  bool timed_out = false;
  if (!ReadProtocolLine(response_line, daemon_response_timeout_sec_, timed_out)) {
    failure_code = timed_out ? kFailureDaemonTimeout : kFailureDaemonUnavailable;
    error = timed_out ? "timed out waiting for daemon response"
                      : "daemon closed or failed while reading response";
    return false;
  }
  return true;
}

// ===========================================================================
// Daemon response parsing -- last boundary before the Store
// ===========================================================================
// Treat the daemon response as untrusted protocol data even though both sides
// live in the same release.  A malformed response is more dangerous than an
// ordinary reconstruction failure because C++ can no longer know which fields
// are safe to publish or whether request/response synchronization was retained.

// Both modes:
//   * strict JSON parse; required status fields; protocol version;
//   * object presence consistent with the inference flags;
//   * ok <=> failure_code == 0.
//   These keep C++ and the daemon in lock-step and are cheap.
// Production: parse only the published physics fields.
// Debug:      parse every field, the diagnostics block, and re-check the
//             probability / class / sigma contract that the daemon already
//             enforces (an end-to-end deployment-contract check).
// A response that fails any check never reaches the Store.

bool cMrdMlReco::ParseDaemonResponse(const std::string& line, DaemonResponse& response,
                                     std::string& error) const {
  response = DaemonResponse();
  ptree tree;
  try {
    std::istringstream input(line);
    boost::property_tree::read_json(input, tree);
  } catch (const std::exception& exc) {
    error = std::string("response JSON parse failure: ") + exc.what();
    return false;
  }

  FieldReader top(tree, "", error);
  if (!top.Int("protocol_version", response.protocol_version) ||
      !top.Bool("ok", response.ok) || !top.String("status", response.status) ||
      !top.Int("failure_code", response.failure_code) ||
      !top.Int("reco_skip_reason", response.reco_skip_reason) ||
      !top.String("error", response.error) ||
      !top.Bool("input_build_ok", response.input_build_ok) ||
      !top.Bool("gate_attempted", response.gate_attempted) ||
      !top.Bool("gate_inference_ok", response.gate_inference_ok) ||
      !top.Bool("gate_passed", response.gate_passed) ||
      !top.Bool("reco_attempted", response.reco_attempted) ||
      !top.Bool("reco_inference_ok", response.reco_inference_ok))
    return false;
  if (response.protocol_version != kProtocolVersion) {
    error = "event response protocol version mismatch";
    return false;
  }

  const bool debug = DebugMode();

  // Diagnostics.  The daemon sends the full diagnostics object whenever input
  // construction succeeded (input_build_ok), and the bypass flag below is
  // derived from it, so in that case the object and the nine production
  // provenance fields are required; a response missing them is a protocol
  // error, never a silently mis-classified event.  The rest is debug-only and
  // optional.
  const ptree* diagnostics_node = NonNullObject(tree, "diagnostics");
  if (response.input_build_ok && !diagnostics_node) {
    error = "input_build_ok=true but diagnostics object is null/missing";
    return false;
  }
  if (response.reco_attempted && !response.input_build_ok) {
    error = "reco_attempted=true but input_build_ok=false";
    return false;
  }
  if (diagnostics_node) {
    const ptree& node = *diagnostics_node;
    Diagnostics& d = response.diagnostics;
    d.present = true;
    FieldReader f(node, "diagnostics", error);
    if (response.input_build_ok) {
      if (!f.String("mrd_source", d.mrd_source) ||
          !f.Int("number_mrd_time_clusters", d.number_mrd_time_clusters) ||
          !f.Int("selected_mrd_cluster_index", d.selected_mrd_cluster_index) ||
          !f.Int("selected_tank_cluster_index", d.selected_tank_cluster_index) ||
          !f.Int("input_tank_hits", d.input_tank_hits) ||
          !f.Bool("tank_mrd_association_found", d.tank_mrd_association_found) ||
          !f.Int("tank_mrd_selected_tank_cluster_index",
                 d.tank_mrd_selected_tank_cluster_index) ||
          !f.Int("tank_mrd_selected_mrd_subevent_id", d.tank_mrd_selected_mrd_subevent_id) ||
          !f.Double("tank_mrd_timing_residual_ns", d.tank_mrd_timing_residual_ns))
        return false;
    }
    // Failure responses with input_build_ok=false carry an empty diagnostics
    // object ({}); nothing is read from it and the sentinels stay in place.
    if (debug) {
      f.OptDouble("selected_mrd_cluster_start_time_ns", d.selected_mrd_cluster_start_time_ns);
      f.OptDouble("selected_mrd_cluster_end_time_ns", d.selected_mrd_cluster_end_time_ns);
      f.OptDouble("selected_mrd_cluster_time_span_ns", d.selected_mrd_cluster_time_span_ns);
      f.OptInt("input_mrd_raw_hits", d.input_mrd_raw_hits);
      f.OptInt("input_mrd_selected_raw_hits", d.input_mrd_selected_raw_hits);
      f.OptInt("input_mrd_geometry_valid_hits_before_dead_mask",
               d.input_mrd_geometry_valid_hits_before_dead_mask);
      f.OptInt("input_mrd_known_dead_removed", d.input_mrd_known_dead_removed);
      f.OptInt("input_mrd_live_hits_before_dedup", d.input_mrd_live_hits_before_dedup);
      f.OptInt("input_mrd_live_unique_hits", d.input_mrd_live_unique_hits);
      f.OptInt("input_mrd_dedup_removed", d.input_mrd_dedup_removed);
      f.OptInt("input_mrd_unassigned_hits", d.input_mrd_unassigned_hits);
      f.OptInt("input_mrd_layers", d.input_mrd_layers);
      f.OptInt("input_mrd_first_layer", d.input_mrd_first_layer);
      f.OptInt("input_mrd_last_layer", d.input_mrd_last_layer);
      f.OptString("tank_source", d.tank_source);
      f.OptInt("number_tank_clusters", d.number_tank_clusters);
      f.OptDouble("selected_tank_cluster_time_ns", d.selected_tank_cluster_time_ns);
      f.OptDouble("selected_tank_cluster_charge_pe", d.selected_tank_cluster_charge_pe);
      f.OptDouble("selected_tank_cluster_time_span_ns", d.selected_tank_cluster_time_span_ns);
      f.OptBool("tank_packet_reordered_by_time", d.tank_packet_reordered_by_time);
      f.OptBool("reco_structurally_eligible", d.reco_structurally_eligible);
      f.OptBool("physics_is_raw_unstandardized", d.physics_is_raw_unstandardized);
      f.OptIntArray("additional_dead_mrd_chankeys", d.additional_dead_mrd_chankeys);
      f.OptBool("tank_mrd_association_applied", d.tank_mrd_association_applied);
      f.OptInt("tank_mrd_compatible_pair_count", d.tank_mrd_compatible_pair_count);
      f.OptDouble("tank_mrd_expected_offset_ns", d.tank_mrd_expected_offset_ns);
      f.OptDouble("tank_mrd_coincidence_window_ns", d.tank_mrd_coincidence_window_ns);
      f.OptDouble("tank_mrd_tank_mean_time_ns", d.tank_mrd_tank_mean_time_ns);
      f.OptDouble("tank_mrd_mrd_mean_time_ns", d.tank_mrd_mrd_mean_time_ns);
      f.OptDouble("tank_mrd_dt_ns", d.tank_mrd_dt_ns);
    }
  }

  if (const ptree* node = NonNullObject(tree, "gate")) {
    GateResult& g = response.gate;
    g.present = true;
    FieldReader f(*node, "gate", error);
    if (!f.Double("has_target_probability", g.has_target_probability) ||
        !f.Double("reconstructable_probability", g.reconstructable_probability) ||
        !f.Int("event_category", g.event_category) || !f.Bool("passed", g.passed))
      return false;
    if (debug &&
        (!f.Double("event_category_confidence", g.event_category_confidence) ||
         !f.Double("event_category_margin", g.event_category_margin) ||
         !f.Array("event_category_probabilities", g.event_category_probabilities,
                  kNumEventCategories) ||
         !f.Double("single_muon_compatibility_probability",
                   g.single_muon_compatibility_probability) ||
         !f.Bool("single_muon_compatibility_validated",
                 g.single_muon_compatibility_validated)))
      return false;
  }

  if (const ptree* node = NonNullObject(tree, "reconstruction")) {
    ReconstructionResult& r = response.reconstruction;
    r.present = true;
    FieldReader f(*node, "reconstruction", error);
    if (!f.Int("topology_class", r.topology_class) ||
        !f.Array("topology_probabilities", r.topology_probabilities, kNumTopologyClasses) ||
        !f.Double("stopped_probability", r.stopped_probability) ||
        !f.Int("exit_face_class", r.exit_face_class) ||
        !f.Double("mrd_track_length_cm", r.mrd_track_length_cm) ||
        !f.Double("mrd_track_length_sigma_cm", r.mrd_track_length_sigma_cm) ||
        !f.Double("cos_theta", r.cos_theta) ||
        !f.Double("cos_theta_sigma", r.cos_theta_sigma) ||
        !f.Double("theta_rad", r.theta_rad) ||
        !f.Double("theta_sigma_rad", r.theta_sigma_rad) ||
        !f.Bool("theta_sigma_valid", r.theta_sigma_valid) ||
        !f.Double("phi_rad", r.phi_rad) ||
        !f.Double("penetration_cm", r.penetration_cm) ||
        !f.Double("penetration_sigma_cm", r.penetration_sigma_cm) ||
        !f.Double("mrd_energy_loss_mev", r.mrd_energy_loss_mev) ||
        !f.Double("mrd_energy_loss_sigma_mev", r.mrd_energy_loss_sigma_mev) ||
        !f.Double("muon_ke_mev", r.muon_ke_mev) ||
        !f.Double("muon_ke_sigma_mev", r.muon_ke_sigma_mev) ||
        !f.Double("cherenkov_angle_rad", r.cherenkov_angle_rad) ||
        !f.Double("cherenkov_angle_sigma_rad", r.cherenkov_angle_sigma_rad) ||
        !f.Array("tank_vertex_cm", r.tank_vertex_cm, 3) ||
        !f.Array("tank_vertex_sigma_cm", r.tank_vertex_sigma_cm, 3) ||
        !f.Array("tank_exit_cm", r.tank_exit_cm, 3) ||
        !f.Array("tank_exit_sigma_cm", r.tank_exit_sigma_cm, 3) ||
        !f.Array("mrd_entry_cm", r.mrd_entry_cm, 3) ||
        !f.Array("mrd_entry_sigma_cm", r.mrd_entry_sigma_cm, 3) ||
        !f.Array("mrd_exit_cm", r.mrd_exit_cm, 3) ||
        !f.Array("mrd_exit_sigma_cm", r.mrd_exit_sigma_cm, 3) ||
        !f.Array("selected_endpoint_cm", r.selected_endpoint_cm, 3) ||
        !f.Array("selected_endpoint_sigma_cm", r.selected_endpoint_sigma_cm, 3) ||
        !f.Array("mrd_scatter_rad", r.mrd_scatter_rad, 3) ||
        !f.Array("mrd_scatter_sigma_rad", r.mrd_scatter_sigma_rad, 3) ||
        !f.Array("tank_scatter_rad", r.tank_scatter_rad, 3) ||
        !f.Array("tank_scatter_sigma_rad", r.tank_scatter_sigma_rad, 3))
      return false;
    if (debug &&
        (!f.Double("topology_confidence", r.topology_confidence) ||
         !f.Double("topology_margin", r.topology_margin) ||
         !f.Double("topology_p_stopped", r.topology_p_stopped) ||
         !f.Double("topology_p_back_exit", r.topology_p_back_exit) ||
         !f.Double("topology_p_side_exit", r.topology_p_side_exit) ||
         !f.Double("exit_face_confidence", r.exit_face_confidence) ||
         !f.Double("exit_face_margin", r.exit_face_margin) ||
         !f.Array("exit_face_probabilities", r.exit_face_probabilities, kNumExitFaces) ||
         !f.Int("endpoint_expert_used", r.endpoint_expert_used) ||
         !f.Double("sin_phi", r.sin_phi) || !f.Double("cos_phi", r.cos_phi) ||
         !f.Double("phi_vector_norm", r.phi_vector_norm) ||
         !f.Array("stop_endpoint_cm", r.stop_endpoint_cm, 3) ||
         !f.Array("stop_endpoint_sigma_cm", r.stop_endpoint_sigma_cm, 3) ||
         !f.Array("back_exit_endpoint_cm", r.back_exit_endpoint_cm, 3) ||
         !f.Array("back_exit_endpoint_sigma_cm", r.back_exit_endpoint_sigma_cm, 3) ||
         !f.Array("side_exit_endpoint_cm", r.side_exit_endpoint_cm, 3) ||
         !f.Array("side_exit_endpoint_sigma_cm", r.side_exit_endpoint_sigma_cm, 3) ||
         !f.Bool("quality_baseline", r.quality_baseline) ||
         !f.Bool("quality_topology", r.quality_topology) ||
         !f.Bool("quality_direction", r.quality_direction) ||
         !f.Bool("quality_endpoint", r.quality_endpoint) ||
         !f.Int("quality_tier", r.quality_tier)))
      return false;
  }

  // Structural consistency (both modes).
  if (response.gate_inference_ok && !response.gate.present) {
    error = "gate_inference_ok=true but gate object is null/missing";
    return false;
  }
  if (response.reco_inference_ok && !response.reconstruction.present) {
    error = "reco_inference_ok=true but reconstruction object is null/missing";
    return false;
  }
  if (response.reco_inference_ok && !response.reco_attempted) {
    error = "reco_inference_ok=true but reco_attempted=false";
    return false;
  }
  if (response.ok != (response.failure_code == kFailureNone)) {
    error = response.ok ? "ok=true with nonzero failure_code" : "ok=false with failure_code=0";
    return false;
  }

  // Same routing input as CmrdV5InferenceRuntime._reco_skip_reason(): Tank
  // hits < 1 sends the event down the MRD-only bypass route.  reco_attempted
  // implies input_build_ok, which implies a parsed input_tank_hits (above).
  response.reco_used_tank_bypass =
      response.reco_attempted && response.diagnostics.input_tank_hits < 1;

  // Output-contract re-validation (debug mode only).
  if (debug && response.gate.present) {
    const GateResult& g = response.gate;
    if (!IsProbability(g.has_target_probability) ||
        !IsProbability(g.reconstructable_probability) ||
        !IsProbability(g.event_category_confidence) ||
        !IsProbability(g.event_category_margin) ||
        !IsProbabilityVector(g.event_category_probabilities) ||
        !IsProbability(g.single_muon_compatibility_probability) ||
        g.event_category < 0 || g.event_category >= kNumEventCategories) {
      error = "gate object violates probability/class contract";
      return false;
    }
  }
  if (debug && response.reconstruction.present) {
    const ReconstructionResult& r = response.reconstruction;
    if (r.topology_class < 0 || r.topology_class >= kNumTopologyClasses ||
        r.exit_face_class < 0 || r.exit_face_class >= kNumExitFaces ||
        r.endpoint_expert_used != r.topology_class ||
        !IsProbability(r.topology_confidence) || !IsProbability(r.topology_margin) ||
        !IsProbabilityVector(r.topology_probabilities) ||
        !IsProbability(r.topology_p_stopped) || !IsProbability(r.topology_p_back_exit) ||
        !IsProbability(r.topology_p_side_exit) || !IsProbability(r.stopped_probability) ||
        !IsProbability(r.exit_face_confidence) || !IsProbability(r.exit_face_margin) ||
        !IsProbabilityVector(r.exit_face_probabilities) ||
        r.mrd_track_length_sigma_cm < 0.0 || r.cos_theta_sigma < 0.0 ||
        (r.theta_sigma_valid && r.theta_sigma_rad < 0.0) ||
        r.penetration_sigma_cm < 0.0 || r.mrd_energy_loss_sigma_mev < 0.0 ||
        r.muon_ke_sigma_mev < 0.0 || r.cherenkov_angle_sigma_rad < 0.0 ||
        !IsNonNegativeVector(r.tank_vertex_sigma_cm) ||
        !IsNonNegativeVector(r.tank_exit_sigma_cm) ||
        !IsNonNegativeVector(r.mrd_entry_sigma_cm) ||
        !IsNonNegativeVector(r.mrd_exit_sigma_cm) ||
        !IsNonNegativeVector(r.stop_endpoint_sigma_cm) ||
        !IsNonNegativeVector(r.back_exit_endpoint_sigma_cm) ||
        !IsNonNegativeVector(r.side_exit_endpoint_sigma_cm) ||
        !IsNonNegativeVector(r.selected_endpoint_sigma_cm) ||
        !IsNonNegativeVector(r.mrd_scatter_sigma_rad) ||
        !IsNonNegativeVector(r.tank_scatter_sigma_rad)) {
      error = "reconstruction object violates class/probability/sigma contract";
      return false;
    }
  }

  response.parsed = true;
  return true;
}

// ===========================================================================
// Store publication
// ===========================================================================
// Publication is intentionally separated from inference.  Production and
// debug use the same DaemonResponse; OutputMode only decides how much of that
// already-computed state is exposed downstream.  This is the core reason a
// production/debug parity run can require exact agreement on every shared key.
//
// Per-event keys are written exactly once per event from the result struct
// (sentinels unless filled), so no value from a previous event can survive.
// Job-level keys are written by PublishJobIdentity() after every READY
// handshake and are never deleted.  README.md lists every key.
//
// Production per-event keys
//   status     : FailureCode RecoSkipReason GatePassed ReconstructionAvailable
//   provenance : MRDInputSource NumberMrdTimeClusters SelectedMrdClusterIndex
//                SelectedTankClusterIndex InputTankHits RecoUsedTankBypass
//                TankMrdAssociationFound TankMrdSelectedTankClusterIndex
//                TankMrdSelectedMrdSubEventID TankMrdTimingResidualNs
//   gate       : HasTargetMuonProbability ReconstructableProbability EventCategory
//   topology   : TopologyClass TopologyProbabilities StoppedProbability ExitFaceClass
//   reco       : MRDTrackLengthCm CosTheta ThetaRad PhiRad PenetrationCm
//                MRDEnergyLossMeV MuonKEMeV CherenkovAngleRad (+ each *Sigma*),
//                ThetaSigmaValid, TankVertexCm TankExitCm MRDEntryCm MRDExitCm
//                SelectedEndpointCm MRDScatterRad TankScatterRad (+ each *Sigma*)
// Debug adds the remaining per-event keys (see below).

void cMrdMlReco::Publish(const EventIdentity& identity, const DaemonResponse& result) {
  if (!native_store_) return;
  BoostStore& s = *native_store_;
  const Diagnostics& d = result.diagnostics;
  const GateResult& g = result.gate;
  const ReconstructionResult& r = result.reconstruction;

  s.Set("FailureCode", result.failure_code);
  s.Set("RecoSkipReason", result.reco_skip_reason);
  s.Set("GatePassed", result.gate_passed);
  s.Set("ReconstructionAvailable", result.reco_inference_ok && r.present);

  s.Set("MRDInputSource", d.mrd_source);
  s.Set("NumberMrdTimeClusters", d.number_mrd_time_clusters);
  s.Set("SelectedMrdClusterIndex", d.selected_mrd_cluster_index);
  s.Set("SelectedTankClusterIndex", d.selected_tank_cluster_index);
  s.Set("InputTankHits", d.input_tank_hits);
  s.Set("RecoUsedTankBypass", result.reco_used_tank_bypass);
  s.Set("TankMrdAssociationFound", d.tank_mrd_association_found);
  s.Set("TankMrdSelectedTankClusterIndex", d.tank_mrd_selected_tank_cluster_index);
  s.Set("TankMrdSelectedMrdSubEventID", d.tank_mrd_selected_mrd_subevent_id);
  s.Set("TankMrdTimingResidualNs", d.tank_mrd_timing_residual_ns);

  s.Set("HasTargetMuonProbability", g.has_target_probability);
  s.Set("ReconstructableProbability", g.reconstructable_probability);
  s.Set("EventCategory", g.event_category);

  s.Set("TopologyClass", r.topology_class);
  s.Set("TopologyProbabilities", r.topology_probabilities);
  s.Set("StoppedProbability", r.stopped_probability);
  s.Set("ExitFaceClass", r.exit_face_class);

  s.Set("MRDTrackLengthCm", r.mrd_track_length_cm);
  s.Set("MRDTrackLengthSigmaCm", r.mrd_track_length_sigma_cm);
  s.Set("CosTheta", r.cos_theta);
  s.Set("CosThetaSigma", r.cos_theta_sigma);
  s.Set("ThetaRad", r.theta_rad);
  s.Set("ThetaSigmaRad", r.theta_sigma_rad);
  s.Set("ThetaSigmaValid", r.theta_sigma_valid);
  s.Set("PhiRad", r.phi_rad);
  s.Set("PenetrationCm", r.penetration_cm);
  s.Set("PenetrationSigmaCm", r.penetration_sigma_cm);
  s.Set("MRDEnergyLossMeV", r.mrd_energy_loss_mev);
  s.Set("MRDEnergyLossSigmaMeV", r.mrd_energy_loss_sigma_mev);
  s.Set("MuonKEMeV", r.muon_ke_mev);
  s.Set("MuonKESigmaMeV", r.muon_ke_sigma_mev);
  s.Set("CherenkovAngleRad", r.cherenkov_angle_rad);
  s.Set("CherenkovAngleSigmaRad", r.cherenkov_angle_sigma_rad);
  s.Set("TankVertexCm", r.tank_vertex_cm);
  s.Set("TankVertexSigmaCm", r.tank_vertex_sigma_cm);
  s.Set("TankExitCm", r.tank_exit_cm);
  s.Set("TankExitSigmaCm", r.tank_exit_sigma_cm);
  s.Set("MRDEntryCm", r.mrd_entry_cm);
  s.Set("MRDEntrySigmaCm", r.mrd_entry_sigma_cm);
  s.Set("MRDExitCm", r.mrd_exit_cm);
  s.Set("MRDExitSigmaCm", r.mrd_exit_sigma_cm);
  s.Set("SelectedEndpointCm", r.selected_endpoint_cm);
  s.Set("SelectedEndpointSigmaCm", r.selected_endpoint_sigma_cm);
  s.Set("MRDScatterRad", r.mrd_scatter_rad);
  s.Set("MRDScatterSigmaRad", r.mrd_scatter_sigma_rad);
  s.Set("TankScatterRad", r.tank_scatter_rad);
  s.Set("TankScatterSigmaRad", r.tank_scatter_sigma_rad);

  if (!DebugMode()) return;

  // ---------------------------- debug only ----------------------------------
  s.Set("ProtocolVersion", result.protocol_version);
  s.Set("StatusText", result.status);
  s.Set("ErrorText", result.error);
  s.Set("InputBuildOK", result.input_build_ok);
  s.Set("GateAttempted", result.gate_attempted);
  s.Set("GateInferenceOK", result.gate_inference_ok);
  s.Set("RecoAttempted", result.reco_attempted);
  s.Set("RecoInferenceOK", result.reco_inference_ok);
  s.Set("DaemonRestartCount", daemon_restart_count_);

  s.Set("RunNumber", identity.run_number);
  s.Set("SubrunNumber", identity.subrun_number);
  s.Set("EventNumber", identity.event_number);
  s.Set("MCEventNum", identity.mc_event_num);
  s.Set("MCTriggerNum", identity.mc_trigger_num);
  s.Set("IsData", identity.is_data);

  s.Set("SelectedMrdClusterStartTimeNs", d.selected_mrd_cluster_start_time_ns);
  s.Set("SelectedMrdClusterEndTimeNs", d.selected_mrd_cluster_end_time_ns);
  s.Set("SelectedMrdClusterTimeSpanNs", d.selected_mrd_cluster_time_span_ns);
  s.Set("InputMRDRawHits", d.input_mrd_raw_hits);
  s.Set("InputMRDSelectedRawHits", d.input_mrd_selected_raw_hits);
  s.Set("InputMRDGeometryValidHitsBeforeDeadMask",
        d.input_mrd_geometry_valid_hits_before_dead_mask);
  s.Set("InputMRDKnownDeadRemoved", d.input_mrd_known_dead_removed);
  s.Set("InputMRDLiveHitsBeforeDedup", d.input_mrd_live_hits_before_dedup);
  s.Set("InputMRDLiveUniqueHits", d.input_mrd_live_unique_hits);
  s.Set("InputMRDDedupRemoved", d.input_mrd_dedup_removed);
  s.Set("InputMRDUnassignedHits", d.input_mrd_unassigned_hits);
  s.Set("InputMRDLayers", d.input_mrd_layers);
  s.Set("InputMRDFirstLayer", d.input_mrd_first_layer);
  s.Set("InputMRDLastLayer", d.input_mrd_last_layer);
  s.Set("TankInputSource", d.tank_source);
  s.Set("NumberTankClusters", d.number_tank_clusters);
  s.Set("SelectedTankClusterTimeNs", d.selected_tank_cluster_time_ns);
  s.Set("SelectedTankClusterChargePE", d.selected_tank_cluster_charge_pe);
  s.Set("SelectedTankClusterTimeSpanNs", d.selected_tank_cluster_time_span_ns);
  s.Set("TankPacketReorderedByTime", d.tank_packet_reordered_by_time);
  s.Set("AdditionalDeadMRDChankeys",
        d.present ? d.additional_dead_mrd_chankeys : additional_dead_mrd_chankeys_);
  s.Set("RecoStructurallyEligible", d.reco_structurally_eligible);
  s.Set("PhysicsIsRawUnstandardized", d.physics_is_raw_unstandardized);
  s.Set("TankMrdAssociationApplied", d.tank_mrd_association_applied);
  s.Set("TankMrdCompatiblePairCount", d.tank_mrd_compatible_pair_count);
  s.Set("TankMrdExpectedOffsetNs", d.tank_mrd_expected_offset_ns);
  s.Set("TankMrdCoincidenceWindowNs", d.tank_mrd_coincidence_window_ns);
  s.Set("TankMrdTankMeanTimeNs", d.tank_mrd_tank_mean_time_ns);
  s.Set("TankMrdMrdMeanTimeNs", d.tank_mrd_mrd_mean_time_ns);
  s.Set("TankMrdDtNs", d.tank_mrd_dt_ns);

  s.Set("EventCategoryConfidence", g.event_category_confidence);
  s.Set("EventCategoryMargin", g.event_category_margin);
  s.Set("EventCategoryProbabilities", g.event_category_probabilities);
  s.Set("SingleMuonCompatibilityProbability", g.single_muon_compatibility_probability);
  s.Set("SingleMuonCompatibilityValidated", g.single_muon_compatibility_validated);

  s.Set("TopologyConfidence", r.topology_confidence);
  s.Set("TopologyMargin", r.topology_margin);
  s.Set("TopologyPStopped", r.topology_p_stopped);
  s.Set("TopologyPBackExit", r.topology_p_back_exit);
  s.Set("TopologyPSideExit", r.topology_p_side_exit);
  s.Set("ExitFaceConfidence", r.exit_face_confidence);
  s.Set("ExitFaceMargin", r.exit_face_margin);
  s.Set("ExitFaceProbabilities", r.exit_face_probabilities);
  s.Set("EndpointExpertUsed", r.endpoint_expert_used);
  s.Set("SinPhi", r.sin_phi);
  s.Set("CosPhi", r.cos_phi);
  s.Set("PhiVectorNorm", r.phi_vector_norm);
  s.Set("StopEndpointCm", r.stop_endpoint_cm);
  s.Set("StopEndpointSigmaCm", r.stop_endpoint_sigma_cm);
  s.Set("BackExitEndpointCm", r.back_exit_endpoint_cm);
  s.Set("BackExitEndpointSigmaCm", r.back_exit_endpoint_sigma_cm);
  s.Set("SideExitEndpointCm", r.side_exit_endpoint_cm);
  s.Set("SideExitEndpointSigmaCm", r.side_exit_endpoint_sigma_cm);
  s.Set("QualityBaseline", r.quality_baseline);
  s.Set("QualityTopology", r.quality_topology);
  s.Set("QualityDirection", r.quality_direction);
  s.Set("QualityEndpoint", r.quality_endpoint);
  s.Set("QualityTier", r.quality_tier);
}

// Job-level identity: schema/mode, adapter, the release confirmed by the READY
// handshake (names, hashes, versions), and the event-selection policy in
// force.  Everything needed to make an output file self-describing.
// Job identity is republished after every successful READY handshake so a
// daemon restart cannot leave stale provenance.  PhaseIITreeMaker copies these
// values once into mrd_ml_metadata instead of repeating long hashes per event.
void cMrdMlReco::PublishJobIdentity() {
  if (!native_store_) return;
  BoostStore& s = *native_store_;
  const ReleaseIdentity& id = release_identity_;
  s.Set("SchemaVersion", kNativeSchemaVersion);
  s.Set("OutputMode", output_mode_name_);
  s.Set("AdapterVersion", std::string(kAdapterVersion));
  s.Set("ModelRelease", id.valid ? id.model_release : model_release_);
  s.Set("DaemonVersion", id.daemon_version);
  s.Set("RuntimeFeatureVersion", id.runtime_feature_version);
  s.Set("TensorFlowVersion", id.tensorflow_version);
  s.Set("GateModelName", id.gate_model_name);
  s.Set("ReconstructionModelName", id.reconstruction_model_name);
  s.Set("GateModelTreeSHA256", id.gate_model_tree_sha256);
  s.Set("ReconstructionModelTreeSHA256", id.reconstruction_model_tree_sha256);
  s.Set("GateTrainerVersion", id.gate_trainer_version);
  s.Set("ReconstructionTrainerVersion", id.reconstruction_trainer_version);
  s.Set("GeometryVersion", id.geometry_version);
  s.Set("GeometryManifestSHA256", id.geometry_manifest_sha256);
  s.Set("GateHasTargetThreshold", gate_has_target_threshold_);
  s.Set("GateReconstructableThreshold", gate_reconstructable_threshold_);
  s.Set("RequireGateCategoryZero", require_gate_category_zero_);
  s.Set("PrefilterMinMRDHits", prefilter_min_mrd_hits_);
}

// ===========================================================================
// General helpers
// ===========================================================================

bool cMrdMlReco::FileExists(const std::string& path) {
  struct stat info;
  return !path.empty() && stat(path.c_str(), &info) == 0 && S_ISREG(info.st_mode);
}

bool cMrdMlReco::DirectoryExists(const std::string& path) {
  struct stat info;
  return !path.empty() && stat(path.c_str(), &info) == 0 && S_ISDIR(info.st_mode);
}

// Parse a set of nonnegative integer channel keys.  Separators: whitespace,
// ',', ';', and surrounding '[' ']'.  The result is sorted and deduplicated.
bool cMrdMlReco::ParseIntList(const std::string& text, std::vector<int>& values,
                              std::string& error) {
  values.clear();
  error.clear();
  std::string normalized = text;
  for (size_t i = 0; i < normalized.size(); ++i) {
    const char c = normalized[i];
    if (c == ',' || c == ';' || c == '[' || c == ']') normalized[i] = ' ';
  }
  std::istringstream input(normalized);
  std::string token;
  while (input >> token) {
    int parsed = 0;
    if (!ParseInt(token, parsed) || parsed < 0) {
      error = "invalid nonnegative integer token '" + token + "'";
      values.clear();
      return false;
    }
    values.push_back(parsed);
  }
  std::sort(values.begin(), values.end());
  values.erase(std::unique(values.begin(), values.end()), values.end());
  return true;
}

std::string cMrdMlReco::EscapeJSON(const std::string& value) {
  static const char hex[] = "0123456789abcdef";
  std::string out;
  out.reserve(value.size() + 2);
  for (size_t i = 0; i < value.size(); ++i) {
    const unsigned char c = static_cast<unsigned char>(value[i]);
    switch (c) {
      case '"': out += "\\\""; break;
      case '\\': out += "\\\\"; break;
      case '\b': out += "\\b"; break;
      case '\f': out += "\\f"; break;
      case '\n': out += "\\n"; break;
      case '\r': out += "\\r"; break;
      case '\t': out += "\\t"; break;
      default:
        if (c < 0x20) {
          out += "\\u00";
          out.push_back(hex[c >> 4]);
          out.push_back(hex[c & 0xF]);
        } else {
          out.push_back(static_cast<char>(c));
        }
    }
  }
  return out;
}
