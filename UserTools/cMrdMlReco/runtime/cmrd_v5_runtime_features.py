#!/usr/bin/env python3
"""Production event-packet -> tensor adapter for ANNIE cMRD v5.

This module is the Checkpoint-02 runtime feature layer.  It deliberately keeps
all detector-feature mathematics in :mod:`cmrd_v5_features`; the code here only
validates the ToolAnalysis event packet, applies an ANNIE EventSelector-mimic
Tank-charge + Tank/MRD timing association for retained subevents, and calls the
authoritative NumPy builders.  Frozen feature mathematics remain in
:mod:`cmrd_v5_features`; this module changes only detector subevent selection.

Important deployment rule
-------------------------
The exported gate and reconstruction SavedModels contain their own fixed
standardization layer for the 73-component ``physics`` input.  This module
therefore returns the *raw HDF5-style physics features*.  Do not z-score them a
second time in the daemon or in C++.

No truth, GENIE, PDG, MC-particle, classical-track, or SimpleReco quantity is
accepted as an input.  The same packet schema is used for data and MC; only the
concrete upstream Hit/MCHit source differs on the C++ side.
"""

from __future__ import annotations

import math
from dataclasses import dataclass
from typing import Any, Dict, List, Mapping, MutableMapping, Optional, Sequence, Tuple

import numpy as np

from cmrd_v5_features import (
    KNOWN_DEAD_MRD_CHANKEYS,
    MAX_MRD_HITS,
    MAX_TANK_CLUSTERS,
    MAX_TANK_HITS,
    MIN_MRD_HITS,
    MIN_TANK_HITS,
    MRD_PAD_IDX,
    N_MRD_LAYERS,
    PHYSICS_FEATURE_NAMES,
    TANK_PAD_IDX,
    GeometryBundle,
    build_mrd_layer_features,
    build_mrd_sequence,
    build_physics_features,
    build_tank_cluster_features,
    build_tank_sequence,
    load_geometry_bundle,
)

PACKET_SCHEMA = "annie_cmrd_v5_event_packet"
PACKET_SCHEMA_VERSION = 1
RUNTIME_FEATURE_VERSION = "cmrd_v5_runtime_features_0.1.4-eventselector-mimic-mrdcore-tankperpmt"

# EventSelector-style Tank-to-MRD association policy.
DATA_PMT_MRD_OFFSET_NS = 755.0
MC_PMT_MRD_OFFSET_NS = 0.0
PMT_MRD_COINCIDENCE_HALF_WINDOW_NS = 50.0
EVENTSELECTOR_TANK_MEAN_TIME_MAX_NS = 2000.0

# ---------------------------------------------------------------------------
# Prompt-core trimming policy (runtime 0.1.4).
#
# R5915 study: in MRD time clusters spanning >50 ns, 55.5% of hits arriving
# more than 20 ns after the cluster's first hit land on a channel that already
# fired in the prompt core (ringing / afterpulse; median re-fire delay 44 ns),
# and 44.5% land on a new channel whose chankey is a median of 40 away from any
# core channel (uncorrelated noise, not neighbour crosstalk).  Only 12% of wide
# clusters contain a second track-capable (>=4 hit) time group, so this is a
# noise-absorption problem and not muon pileup.
#
# Two independent guards, deliberately separated by blast radius:
#
#   TIER 1  MRD_ASSOCIATION_USE_CORE_MEAN
#           Trims only the mean used by the EventSelector-mimic Tank<->MRD
#           coincidence.  Changes *which* cluster pair is selected.  Model
#           input tensors are byte-identical to 0.1.3.  Safe to deploy against
#           the existing frozen SavedModels; no retrain implication.
#
#   TIER 2  MRD_TRIM_SELECTED_CLUSTER
#           Trims the hits that populate the MRD tensors and the layer/line
#           fits.  This CHANGES THE MODEL INPUT DISTRIBUTION and must not be
#           enabled in production until the HDF5 extractor applies the
#           identical filter and the models are retrained on it.  Default OFF.
# ---------------------------------------------------------------------------
MRD_CORE_WINDOW_NS = 20.0          # prompt-core reach after the first MRD hit
MRD_CORE_DEDUP_RINGING = True      # keep earliest hit per channel inside core
MRD_CORE_LAYER_GUARD = False       # disabled: do not delete legitimate one-hit-per-layer tracks
MRD_CORE_MIN_KEPT_HITS = 4         # below this, fall back to the untrimmed set

MRD_ASSOCIATION_USE_CORE_MEAN = True    # TIER 1 - safe, on by default
MRD_TRIM_SELECTED_CLUSTER = False       # TIER 2 - retrain-coupled, off by default

# Tank selected-cluster de-ringing test.  Association is performed on the raw
# ClusterFinder cluster first.  Only after the Tank/MRD pair is chosen do we
# construct a model-facing Tank copy in which each PMT contributes one token:
# earliest hit time + PE summed through 20 ns after that PMT's first hit.
# This intentionally changes the frozen-v5 Tank input distribution and is a
# commissioning/diagnostic A/B, not a training-parity production definition.
TANK_PER_PMT_CORE_WINDOW_NS = 20.0
TANK_PER_PMT_CORE_ENABLED = True

EXPECTED_INPUT_SHAPES: Mapping[str, Tuple[int, ...]] = {
    "mrd_hit_dyn": (MAX_MRD_HITS, 3),
    "mrd_hit_idx": (MAX_MRD_HITS,),
    "mrd_layer_features": (N_MRD_LAYERS, 16),
    "mrd_layer_mask": (N_MRD_LAYERS,),
    "tank_hit_dyn": (MAX_TANK_HITS, 2),
    "tank_hit_idx": (MAX_TANK_HITS,),
    "tank_prompt_mask": (MAX_TANK_HITS,),
    "tank_cluster_features": (MAX_TANK_CLUSTERS, 13),
    "tank_cluster_mask": (MAX_TANK_CLUSTERS,),
    "physics": (len(PHYSICS_FEATURE_NAMES),),
}

INT_INPUTS = frozenset({"mrd_hit_idx", "tank_hit_idx"})
ALLOWED_TOP_LEVEL_KEYS = frozenset(
    {"schema", "schema_version", "event", "mrd", "tank", "runtime_conditions"}
)
FORBIDDEN_INPUT_TOKENS = (
    "truth",
    "true_",
    "pdg",
    "genie",
    "mcparticles",
    "mc_particles",
    "simple_reco",
    "simplereco",
    "classical_track",
    "fittedmuon",
)


class RuntimeFeatureError(RuntimeError):
    """Base class for packet and feature-construction failures."""


class PacketValidationError(RuntimeFeatureError):
    """The ToolAnalysis event packet violates the frozen schema."""


class FeatureValidationError(RuntimeFeatureError):
    """The authoritative feature builders returned an invalid model tensor."""


@dataclass(frozen=True)
class FeatureBuildResult:
    """One event's raw model tensors and detector-only provenance."""

    tensors: Dict[str, np.ndarray]
    diagnostics: Dict[str, Any]

    def model_inputs(self, add_batch_axis: bool = True) -> Dict[str, np.ndarray]:
        """Return exact SavedModel dtypes, optionally with a leading batch axis."""
        out: Dict[str, np.ndarray] = {}
        for name, value in self.tensors.items():
            dtype = np.int32 if name in INT_INPUTS else np.float32
            arr = np.asarray(value, dtype=dtype)
            if add_batch_axis:
                arr = arr[None, ...]
            out[name] = np.ascontiguousarray(arr)
        return out


@dataclass(frozen=True)
class SelectedMRD:
    chankeys: np.ndarray
    times_ns: np.ndarray
    charges: np.ndarray
    selected_index: int
    selected_start_ns: float
    selected_end_ns: float
    selected_span_ns: float
    unassigned_hits: int
    cluster_num_hits: Tuple[int, ...]
    cluster_start_times_ns: Tuple[float, ...]
    cluster_end_times_ns: Tuple[float, ...]
    cluster_mean_times_ns: Tuple[float, ...]
    raw_hit_count: int
    source: str

    # Original schema-v4 choice retained only as provenance/fallback.
    schema_v4_selected_index: int

    # Prompt-core trim provenance (runtime 0.1.4).  When trimming is disabled
    # these mirror the untrimmed values so downstream code is unconditional.
    trim_applied: bool = False
    trim_removed_ringing: int = 0
    trim_removed_noise: int = 0
    trim_removed_total: int = 0
    untrimmed_selected_hits: int = 0
    untrimmed_selected_start_ns: float = 0.0
    untrimmed_selected_end_ns: float = 0.0
    untrimmed_selected_span_ns: float = 0.0


@dataclass(frozen=True)
class SelectedTank:
    cluster_time: np.ndarray
    cluster_start: np.ndarray
    cluster_end: np.ndarray
    cluster_span: np.ndarray
    cluster_mean_time: np.ndarray
    cluster_charge: np.ndarray
    cluster_nhits: np.ndarray
    selected_index: int
    schema_v4_selected_index: int
    earliest_index: int
    all_cid: np.ndarray
    all_det: np.ndarray
    all_ts: np.ndarray
    all_q: np.ndarray
    selected_time: float
    selected_charge: float
    selected_span: float
    selected_det: np.ndarray
    selected_ts: np.ndarray
    selected_q: np.ndarray
    source: str
    packet_reordered: bool


@dataclass(frozen=True)
class TankMrdAssociation:
    """Detector-only Tank ClusterFinder <-> MRD TimeClustering association."""

    applied: bool
    found: bool
    compatible_pair_count: int
    evaluated_pair_count: int
    selected_tank_cluster_index: int
    selected_mrd_subevent_id: int
    expected_offset_ns: float
    coincidence_window_ns: float
    tank_mean_time_ns: float
    mrd_mean_time_ns: float
    dt_ns: float
    timing_residual_ns: float

    # Prompt-core association provenance (runtime 0.1.4).  ``mrd_mean_time_ns``
    # above is the value actually used for the coincidence test; the full
    # untrimmed EventSelector mean is retained here so the mimic can still be
    # shown to reproduce EventSelector accept/reject decisions.
    mrd_mean_used_core: bool = False
    mrd_mean_full_time_ns: float = -999.0
    dt_full_ns: float = -999.0
    mrd_core_hits: int = 0
    mrd_full_hits: int = 0
    mrd_core_shift_ns: float = -999.0

def _is_number(value: Any) -> bool:
    return isinstance(value, (int, float, np.integer, np.floating)) and not isinstance(value, bool)


def _finite_float(value: Any, label: str) -> float:
    if not _is_number(value):
        raise PacketValidationError(f"{label} must be numeric, got {type(value).__name__}")
    out = float(value)
    if not math.isfinite(out):
        raise PacketValidationError(f"{label} is not finite: {value!r}")
    return out


def _integer(value: Any, label: str) -> int:
    if isinstance(value, bool) or not isinstance(value, (int, np.integer)):
        raise PacketValidationError(f"{label} must be an integer, got {type(value).__name__}")
    return int(value)


def _mapping(value: Any, label: str) -> Mapping[str, Any]:
    if not isinstance(value, Mapping):
        raise PacketValidationError(f"{label} must be an object/mapping")
    return value


def _sequence(value: Any, label: str) -> Sequence[Any]:
    if isinstance(value, (str, bytes)) or not isinstance(value, Sequence):
        raise PacketValidationError(f"{label} must be an array/sequence")
    return value


def _numeric_array(value: Any, label: str, dtype: Any) -> np.ndarray:
    seq = _sequence(value, label)
    try:
        arr = np.asarray(seq, dtype=dtype)
    except Exception as exc:
        raise PacketValidationError(f"{label} could not be converted to {np.dtype(dtype)}: {exc}") from exc
    if arr.ndim != 1:
        raise PacketValidationError(f"{label} must be one-dimensional, got shape {arr.shape}")
    if np.issubdtype(arr.dtype, np.floating) and not np.all(np.isfinite(arr)):
        bad = np.flatnonzero(~np.isfinite(arr))[:5].tolist()
        raise PacketValidationError(f"{label} contains non-finite values at indices {bad}")
    return arr


def _reject_forbidden_keys(value: Any, path: str = "packet") -> None:
    """Reject accidental truth/classical leakage anywhere in the packet."""
    if isinstance(value, Mapping):
        for key, child in value.items():
            low = str(key).lower()
            if any(token in low for token in FORBIDDEN_INPUT_TOKENS):
                raise PacketValidationError(f"forbidden detector-input key {path}.{key}")
            _reject_forbidden_keys(child, f"{path}.{key}")
    elif isinstance(value, Sequence) and not isinstance(value, (str, bytes)):
        for index, child in enumerate(value):
            if isinstance(child, (Mapping, list, tuple)):
                _reject_forbidden_keys(child, f"{path}[{index}]")


def _validate_packet_header(packet: Mapping[str, Any]) -> Tuple[Mapping[str, Any], Mapping[str, Any], Mapping[str, Any]]:
    unknown = set(packet) - set(ALLOWED_TOP_LEVEL_KEYS)
    if unknown:
        raise PacketValidationError(f"unknown top-level packet keys: {sorted(unknown)}")
    if packet.get("schema") != PACKET_SCHEMA:
        raise PacketValidationError(
            f"packet.schema must be {PACKET_SCHEMA!r}, got {packet.get('schema')!r}"
        )
    if _integer(packet.get("schema_version"), "packet.schema_version") != PACKET_SCHEMA_VERSION:
        raise PacketValidationError(
            f"unsupported packet schema_version={packet.get('schema_version')!r}; "
            f"expected {PACKET_SCHEMA_VERSION}"
        )

    event = _mapping(packet.get("event", {}), "packet.event")
    mrd = _mapping(packet.get("mrd", {}), "packet.mrd")
    tank = _mapping(packet.get("tank", {}), "packet.tank")

    # Event identity is detector provenance, not a model feature.  Validate when present.
    for name in ("run_number", "subrun_number", "event_number", "mc_event_num", "mc_trigger_num"):
        if name in event:
            _integer(event[name], f"packet.event.{name}")
    if "is_data" in event and not isinstance(event["is_data"], bool):
        raise PacketValidationError("packet.event.is_data must be boolean")

    _reject_forbidden_keys(packet)
    return event, mrd, tank


def _prompt_core_keep(
    indices: Sequence[int],
    times: np.ndarray,
    chankeys: np.ndarray,
    layer_map: Optional[np.ndarray] = None,
    window_ns: float = MRD_CORE_WINDOW_NS,
    dedup_ringing: bool = MRD_CORE_DEDUP_RINGING,
    layer_guard: bool = MRD_CORE_LAYER_GUARD,
) -> Tuple[List[int], int, int]:
    """Reduce one MRD time cluster to its prompt core.

    Returns ``(kept_indices, n_removed_ringing, n_removed_noise)``.

    Removal order is deliberate and is reported separately because the two
    components have different physical origins:

      1. hits later than ``window_ns`` after the cluster's earliest hit are
         dropped -- these are the long noise/ringing tail;
      2. within the surviving core, only the earliest hit per channel is kept
         (ringing suppression, mirroring ``deduplicate_earliest_per_channel``
         in :mod:`cmrd_v5_features` but applied *before* any mean is taken);
      3. optionally, hits occupying an MRD layer that no other core hit
         occupies are dropped -- these are the distant-chankey noise hits that
         act as leverage points on the global x/y line fits.

    The caller decides whether to use the result.  If fewer than
    ``MRD_CORE_MIN_KEPT_HITS`` survive, the caller should fall back to the
    untrimmed set rather than hand a degenerate cluster downstream.
    """
    idx = [int(i) for i in indices]
    if not idx:
        return [], 0, 0

    n = int(times.size)
    idx = [i for i in idx if 0 <= i < n]
    if not idx:
        return [], 0, 0

    idx.sort(key=lambda i: float(times[i]))
    t0 = float(times[idx[0]])

    core = [i for i in idx if float(times[i]) - t0 <= float(window_ns)]
    removed_late = len(idx) - len(core)

    ringing = 0
    if dedup_ringing:
        seen: set = set()
        kept: List[int] = []
        for i in core:                      # already time-sorted -> first is earliest
            ck = int(chankeys[i])
            if ck in seen:
                ringing += 1
                continue
            seen.add(ck)
            kept.append(i)
        core = kept

    noise = 0
    if layer_guard and layer_map is not None and len(core) > 1:
        nmap = int(np.asarray(layer_map).shape[0])
        layers: List[int] = []
        for i in core:
            ck = int(chankeys[i])
            layers.append(int(layer_map[ck]) if 0 <= ck < nmap else -1)
        counts: Dict[int, int] = {}
        for lay in layers:
            counts[lay] = counts.get(lay, 0) + 1
        guarded = [i for i, lay in zip(core, layers) if lay < 0 or counts[lay] > 1]
        # Never let the guard empty the cluster; a genuinely one-hit-per-layer
        # track is legitimate, so only apply it when it leaves a usable core.
        if len(guarded) >= MRD_CORE_MIN_KEPT_HITS:
            noise = len(core) - len(guarded)
            core = guarded

    # Attribute the late-tail removals between the two components using the
    # measured composition of the surviving core's channel set.
    core_ck = {int(chankeys[i]) for i in core}
    late_ringing = 0
    late_noise = 0
    for i in idx:
        if i in core:
            continue
        if float(times[i]) - t0 <= float(window_ns):
            continue
        if int(chankeys[i]) in core_ck:
            late_ringing += 1
        else:
            late_noise += 1
    assert late_ringing + late_noise == removed_late

    return core, ringing + late_ringing, noise + late_noise


def _select_mrd(
    mrd_obj: Mapping[str, Any],
    selected_index_override: Optional[int] = None,
    layer_map: Optional[np.ndarray] = None,
    trim_selected: Optional[bool] = None,
) -> SelectedMRD:
    """Parse MRD TimeClustering output and materialize one retained subevent.

    With no override, preserve the schema-v4 fallback:
      largest number of valid digit indices, tie -> earliest start.

    A forced index of -1 produces an empty selected MRD sequence while keeping
    raw/cluster metadata for diagnostics.
    """
    source = str(mrd_obj.get("source", "unknown"))
    allowed_sources = {"CStore", "raw_tdc_data", "raw_tdc_mc", "empty"}
    if source not in allowed_sources:
        raise PacketValidationError(
            f"packet.mrd.source={source!r} is not one of {sorted(allowed_sources)}"
        )

    chankeys = _numeric_array(
        mrd_obj.get("digit_chankeys", []),
        "packet.mrd.digit_chankeys",
        np.int64,
    )
    times = _numeric_array(
        mrd_obj.get("digit_times_ns", []),
        "packet.mrd.digit_times_ns",
        np.float64,
    )
    charges = _numeric_array(
        mrd_obj.get("digit_charges", []),
        "packet.mrd.digit_charges",
        np.float64,
    )
    lengths = {"chankeys": chankeys.size, "times": times.size, "charges": charges.size}
    if len(set(lengths.values())) != 1:
        raise PacketValidationError(f"MRD digit-vector length mismatch: {lengths}")

    raw_clusters = _sequence(
        mrd_obj.get("time_clusters", []), "packet.mrd.time_clusters"
    )
    clusters: List[List[int]] = []
    for ci, raw in enumerate(raw_clusters):
        seq = _sequence(raw, f"packet.mrd.time_clusters[{ci}]")
        clusters.append([
            _integer(v, f"packet.mrd.time_clusters[{ci}][]") for v in seq
        ])

    n_store = _integer(
        mrd_obj.get("num_time_clusters_store", len(clusters)),
        "packet.mrd.num_time_clusters_store",
    )
    if n_store < 0:
        raise PacketValidationError(
            "packet.mrd.num_time_clusters_store must be nonnegative"
        )
    if n_store > 0 and n_store != len(clusters):
        raise PacketValidationError(
            "MRD NumMrdTimeClusters disagrees with MrdTimeClusters length: "
            f"store={n_store}, vector={len(clusters)}"
        )

    n_hits = int(times.size)

    # No retained TimeClustering subevent: preserve inclusive raw-MRD behavior.
    if n_store <= 0 or not clusters:
        start = float(np.min(times)) if n_hits else 0.0
        return SelectedMRD(
            chankeys=chankeys.astype(np.int32, copy=False),
            times_ns=times.astype(np.float32, copy=False),
            charges=charges.astype(np.float32, copy=False),
            selected_index=-1,
            selected_start_ns=start,
            selected_end_ns=start if n_hits else 0.0,
            selected_span_ns=0.0,
            unassigned_hits=n_hits,
            cluster_num_hits=tuple(),
            cluster_start_times_ns=tuple(),
            cluster_end_times_ns=tuple(),
            cluster_mean_times_ns=tuple(),
            raw_hit_count=n_hits,
            source=source,
            schema_v4_selected_index=-1,
        )

    hit_cluster_id = np.full(n_hits, -1, dtype=np.int32)
    cluster_valid_indices: List[List[int]] = []
    cluster_num_hits: List[int] = []
    cluster_start: List[float] = []
    cluster_end: List[float] = []
    cluster_mean: List[float] = []

    schema_v4_selected = -1
    best_n = -1
    best_start = math.inf

    for ci, cluster in enumerate(clusters):
        valid_indices: List[int] = []
        tmin = math.inf
        tmax = -math.inf
        tsum = 0.0

        for idx in cluster:
            if idx < 0 or idx >= n_hits:
                continue
            # Mirrors schema-v4 overlap semantics: last cluster assignment wins.
            hit_cluster_id[idx] = ci
            valid_indices.append(idx)
            t = float(times[idx])
            tsum += t
            tmin = min(tmin, t)
            tmax = max(tmax, t)

        cluster_valid_indices.append(list(valid_indices))
        valid_n = len(valid_indices)
        if valid_n == 0:
            tmin = -999.0
            tmax = -999.0
            tmean = -999.0
        else:
            tmean = tsum / float(valid_n)

        cluster_num_hits.append(valid_n)
        cluster_start.append(float(tmin))
        cluster_end.append(float(tmax))
        cluster_mean.append(float(tmean))

        if valid_n > best_n or (
            valid_n == best_n and float(tmin) < best_start
        ):
            schema_v4_selected = ci
            best_n = valid_n
            best_start = float(tmin)

    if selected_index_override is None:
        selected = int(schema_v4_selected)
        forced_empty = False
    else:
        selected = int(selected_index_override)
        if selected < -1 or selected >= len(clusters):
            raise PacketValidationError(
                "MRD selected_index_override is outside retained cluster range: "
                f"{selected} for {len(clusters)} clusters"
            )
        forced_empty = selected == -1

    do_trim = (
        MRD_TRIM_SELECTED_CLUSTER if trim_selected is None else bool(trim_selected)
    )
    trim_applied = False
    trim_ringing = 0
    trim_noise = 0
    untrimmed_hits = 0
    untrimmed_start = 0.0
    untrimmed_end = 0.0
    untrimmed_span = 0.0

    if selected >= 0:
        use = hit_cluster_id == selected
        selected_times = times[use]
        fallback_start = (
            float(np.min(selected_times)) if selected_times.size else 0.0
        )
        start = cluster_start[selected]
        end = cluster_end[selected]
        if not math.isfinite(start) or start <= -900.0:
            start = fallback_start
        if not math.isfinite(end) or end <= -900.0:
            end = start
        span = max(0.0, float(end) - float(start))

        untrimmed_hits = int(np.sum(use))
        untrimmed_start = float(start)
        untrimmed_end = float(end)
        untrimmed_span = float(span)

        if do_trim and untrimmed_hits > 0:
            kept, n_ring, n_noise = _prompt_core_keep(
                cluster_valid_indices[selected], times, chankeys, layer_map
            )
            if len(kept) >= MRD_CORE_MIN_KEPT_HITS:
                trimmed_use = np.zeros(n_hits, dtype=bool)
                trimmed_use[np.asarray(kept, dtype=np.int64)] = True
                use = trimmed_use
                kept_times = times[use]
                start = float(np.min(kept_times))
                end = float(np.max(kept_times))
                span = max(0.0, end - start)
                trim_applied = True
                trim_ringing = int(n_ring)
                trim_noise = int(n_noise)
    else:
        use = np.zeros(n_hits, dtype=bool)
        start = 0.0
        end = 0.0
        span = 0.0

    # Do not inject a large unassigned-fraction feature for an intentional
    # no-association empty MRD selection.
    unassigned_for_features = (
        0 if forced_empty else int(np.sum(hit_cluster_id < 0))
    )

    return SelectedMRD(
        chankeys=chankeys[use].astype(np.int32, copy=False),
        times_ns=times[use].astype(np.float32, copy=False),
        charges=charges[use].astype(np.float32, copy=False),
        selected_index=int(selected),
        selected_start_ns=float(start),
        selected_end_ns=float(end),
        selected_span_ns=float(span),
        unassigned_hits=int(unassigned_for_features),
        cluster_num_hits=tuple(int(v) for v in cluster_num_hits),
        cluster_start_times_ns=tuple(float(v) for v in cluster_start),
        cluster_end_times_ns=tuple(float(v) for v in cluster_end),
        cluster_mean_times_ns=tuple(float(v) for v in cluster_mean),
        raw_hit_count=n_hits,
        source=source,
        schema_v4_selected_index=int(schema_v4_selected),
        trim_applied=bool(trim_applied),
        trim_removed_ringing=int(trim_ringing),
        trim_removed_noise=int(trim_noise),
        trim_removed_total=int(trim_ringing + trim_noise),
        untrimmed_selected_hits=int(untrimmed_hits),
        untrimmed_selected_start_ns=float(untrimmed_start),
        untrimmed_selected_end_ns=float(untrimmed_end),
        untrimmed_selected_span_ns=float(untrimmed_span),
    )

def _select_tank(
    tank_obj: Mapping[str, Any],
    selected_index_override: Optional[int] = None,
) -> SelectedTank:
    """Parse ClusterFinder output and materialize one Tank cluster.

    Without an override, the schema-v4 highest-total-PE selection is retained as
    provenance/Tank-only fallback.  EventSelector-mimic association supplies an
    explicit selected index when both Tank and MRD retained subevents exist.
    """
    source = str(tank_obj.get("source", "unknown"))
    allowed_sources = {"ClusterMap", "ClusterMapMC", "empty"}
    if source not in allowed_sources:
        raise PacketValidationError(
            f"packet.tank.source={source!r} is not one of {sorted(allowed_sources)}"
        )

    raw_clusters = _sequence(tank_obj.get("clusters", []), "packet.tank.clusters")
    parsed: List[Tuple[float, int, np.ndarray, np.ndarray, np.ndarray]] = []
    for original_index, raw in enumerate(raw_clusters):
        obj = _mapping(raw, f"packet.tank.clusters[{original_index}]")
        t = _finite_float(
            obj.get("time_ns"),
            f"packet.tank.clusters[{original_index}].time_ns",
        )
        det = _numeric_array(
            obj.get("detkeys", []),
            f"packet.tank.clusters[{original_index}].detkeys",
            np.int64,
        )
        ts = _numeric_array(
            obj.get("hit_times_ns", []),
            f"packet.tank.clusters[{original_index}].hit_times_ns",
            np.float64,
        )
        q = _numeric_array(
            obj.get("hit_charges_pe", []),
            f"packet.tank.clusters[{original_index}].hit_charges_pe",
            np.float64,
        )
        lengths = {"detkeys": det.size, "times": ts.size, "charges": q.size}
        if len(set(lengths.values())) != 1:
            raise PacketValidationError(
                f"tank cluster {original_index} hit-vector length mismatch: {lengths}"
            )
        parsed.append((t, original_index, det, ts, q))

    # EventSelector traverses ClusterMap in std::map key order.
    parsed_sorted = sorted(parsed, key=lambda row: (row[0], row[1]))
    reordered = [row[1] for row in parsed_sorted] != list(range(len(parsed)))

    cluster_time: List[float] = []
    cluster_start: List[float] = []
    cluster_end: List[float] = []
    cluster_span: List[float] = []
    cluster_mean: List[float] = []
    cluster_charge: List[float] = []
    cluster_nhits: List[int] = []
    all_cid: List[int] = []
    all_det: List[int] = []
    all_ts: List[float] = []
    all_q: List[float] = []

    schema_v4_selected = -1
    earliest = -1
    max_q = -math.inf
    earliest_time = math.inf

    for ci, (t, _original, det, ts, q) in enumerate(parsed_sorted):
        qsum = float(np.sum(q, dtype=np.float64))
        if ts.size:
            tmin = float(np.min(ts))
            tmax = float(np.max(ts))
            tmean = float(np.mean(ts, dtype=np.float64))
            span = tmax - tmin
        else:
            tmin = -999.0
            tmax = -999.0
            tmean = -999.0
            span = -999.0

        cluster_time.append(t)
        cluster_start.append(tmin)
        cluster_end.append(tmax)
        cluster_span.append(span)
        cluster_mean.append(tmean)
        cluster_charge.append(qsum)
        cluster_nhits.append(int(ts.size))
        all_cid.extend([ci] * int(ts.size))
        all_det.extend(int(v) for v in det)
        all_ts.extend(float(v) for v in ts)
        all_q.extend(float(v) for v in q)

        if t < earliest_time:
            earliest_time = t
            earliest = ci

        # Schema-v4 provenance: strict > means equal-charge ties retain earlier map entry.
        if qsum > max_q:
            max_q = qsum
            schema_v4_selected = ci

    if selected_index_override is None:
        selected = int(schema_v4_selected)
    else:
        selected = int(selected_index_override)
        if selected < -1 or selected >= len(parsed_sorted):
            raise PacketValidationError(
                "Tank selected_index_override is outside cluster range: "
                f"{selected} for {len(parsed_sorted)} clusters"
            )

    all_cid_arr = np.asarray(all_cid, dtype=np.int32)
    all_det_arr = np.asarray(all_det, dtype=np.int32)
    all_ts_arr = np.asarray(all_ts, dtype=np.float32)
    all_q_arr = np.asarray(all_q, dtype=np.float32)

    if selected >= 0:
        use = all_cid_arr == selected
        selected_det = all_det_arr[use].astype(np.int32, copy=True)
        selected_ts = all_ts_arr[use].astype(np.float32, copy=True)
        selected_q = all_q_arr[use].astype(np.float32, copy=True)
        selected_time = float(cluster_time[selected])
        selected_charge = float(cluster_charge[selected])
        selected_span = float(cluster_span[selected])
    else:
        selected_det = np.zeros(0, dtype=np.int32)
        selected_ts = np.zeros(0, dtype=np.float32)
        selected_q = np.zeros(0, dtype=np.float32)
        selected_time = 0.0
        selected_charge = 0.0
        selected_span = 0.0

    return SelectedTank(
        cluster_time=np.asarray(cluster_time, dtype=np.float32),
        cluster_start=np.asarray(cluster_start, dtype=np.float32),
        cluster_end=np.asarray(cluster_end, dtype=np.float32),
        cluster_span=np.asarray(cluster_span, dtype=np.float32),
        cluster_mean_time=np.asarray(cluster_mean, dtype=np.float32),
        cluster_charge=np.asarray(cluster_charge, dtype=np.float32),
        cluster_nhits=np.asarray(cluster_nhits, dtype=np.int32),
        selected_index=int(selected),
        schema_v4_selected_index=int(schema_v4_selected),
        earliest_index=int(earliest),
        all_cid=all_cid_arr,
        all_det=all_det_arr,
        all_ts=all_ts_arr,
        all_q=all_q_arr,
        selected_time=float(selected_time),
        selected_charge=float(selected_charge),
        selected_span=float(selected_span),
        selected_det=selected_det,
        selected_ts=selected_ts,
        selected_q=selected_q,
        source=source,
        packet_reordered=bool(reordered),
    )



def _apply_tank_per_pmt_core(
    tank: SelectedTank,
    window_ns: float = TANK_PER_PMT_CORE_WINDOW_NS,
    enabled: bool = TANK_PER_PMT_CORE_ENABLED,
) -> Tuple[SelectedTank, Dict[str, Any]]:
    """Build a model-facing de-rung copy of the already-selected Tank cluster.

    The Tank/MRD association has already been performed when this helper is
    called, so this function cannot change which Tank cluster was selected.

    For each PMT in the selected cluster:
      * find that PMT's earliest hit t0;
      * retain raw hits with t <= t0 + ``window_ns``;
      * sum retained calibrated PE;
      * emit one model token at t0 with the retained PE sum.

    Other Tank clusters are left untouched.  Selected-cluster aggregate arrays
    are rebuilt so the sequence, cluster-context and physics inputs all describe
    the same cleaned representation.
    """
    raw_det = np.asarray(tank.selected_det, dtype=np.int32)
    raw_ts = np.asarray(tank.selected_ts, dtype=np.float64)
    raw_q = np.asarray(tank.selected_q, dtype=np.float64)

    raw_hits = int(min(raw_det.size, raw_ts.size, raw_q.size))
    raw_det = raw_det[:raw_hits]
    raw_ts = raw_ts[:raw_hits]
    raw_q = raw_q[:raw_hits]

    unique_pmts = int(np.unique(raw_det).size) if raw_hits else 0
    raw_total_pe = float(np.sum(raw_q, dtype=np.float64)) if raw_hits else 0.0
    raw_mean_ns = float(np.mean(raw_ts, dtype=np.float64)) if raw_hits else -999.0

    diag: Dict[str, Any] = {
        "tank_per_pmt_core_enabled": bool(enabled),
        "tank_per_pmt_core_window_ns": float(window_ns),
        "tank_per_pmt_raw_hits": raw_hits,
        "tank_per_pmt_raw_unique_pmts": unique_pmts,
        "tank_per_pmt_raw_duplicate_hits": max(0, raw_hits - unique_pmts),
        "tank_per_pmt_information_ratio": (
            float(unique_pmts) / float(raw_hits) if raw_hits > 0 else 0.0
        ),
        "tank_per_pmt_kept_raw_hits": raw_hits,
        "tank_per_pmt_model_tokens": raw_hits,
        "tank_per_pmt_early_merged_hits": 0,
        "tank_per_pmt_late_removed_hits": 0,
        "tank_per_pmt_raw_total_pe": raw_total_pe,
        "tank_per_pmt_core_total_pe": raw_total_pe,
        "tank_per_pmt_late_removed_pe": 0.0,
        "tank_per_pmt_raw_mean_hit_ns": raw_mean_ns,
        "tank_per_pmt_core_mean_first_hit_ns": raw_mean_ns,
        "tank_per_pmt_full_minus_core_mean_ns": 0.0 if raw_hits else -999.0,
    }

    if not enabled or tank.selected_index < 0 or raw_hits <= 0:
        return tank, diag

    tokens: List[Tuple[int, float, float]] = []
    kept_raw_hits = 0
    late_removed_hits = 0
    late_removed_pe = 0.0

    # Per-PMT local timing protects legitimate propagation/TOF differences
    # between distinct PMTs while rejecting only late activity on the same PMT.
    for det in sorted(int(v) for v in np.unique(raw_det)):
        slots = np.flatnonzero(raw_det == det)
        if slots.size == 0:
            continue
        order = slots[np.argsort(raw_ts[slots], kind="stable")]
        t0 = float(raw_ts[order[0]])
        keep = order[raw_ts[order] <= t0 + float(window_ns)]
        late = order[raw_ts[order] > t0 + float(window_ns)]

        if keep.size == 0:
            # Defensive only: the earliest hit must satisfy its own window.
            keep = order[:1]
            late = order[1:]

        qsum = float(np.sum(raw_q[keep], dtype=np.float64))
        tokens.append((det, t0, qsum))
        kept_raw_hits += int(keep.size)
        late_removed_hits += int(late.size)
        if late.size:
            late_removed_pe += float(np.sum(raw_q[late], dtype=np.float64))

    tokens.sort(key=lambda row: (row[1], row[0]))
    model_det = np.asarray([r[0] for r in tokens], dtype=np.int32)
    model_ts = np.asarray([r[1] for r in tokens], dtype=np.float32)
    model_q = np.asarray([r[2] for r in tokens], dtype=np.float32)

    model_tokens = int(model_det.size)
    core_total_pe = float(np.sum(model_q, dtype=np.float64)) if model_tokens else 0.0
    core_mean_ns = float(np.mean(model_ts, dtype=np.float64)) if model_tokens else -999.0

    # Rebuild the selected cluster inside the all-cluster hit arrays so
    # build_tank_cluster_features sees exactly the same cleaned representation
    # as build_tank_sequence.  Other clusters are preserved byte-for-byte.
    ids = np.asarray(tank.all_cid, dtype=np.int32)
    keep_other = ids != int(tank.selected_index)
    other_cid = ids[keep_other]
    other_det = np.asarray(tank.all_det, dtype=np.int32)[keep_other]
    other_ts = np.asarray(tank.all_ts, dtype=np.float32)[keep_other]
    other_q = np.asarray(tank.all_q, dtype=np.float32)[keep_other]

    new_cid = np.concatenate([
        other_cid,
        np.full(model_tokens, int(tank.selected_index), dtype=np.int32),
    ])
    new_det = np.concatenate([other_det, model_det])
    new_ts = np.concatenate([other_ts, model_ts])
    new_q = np.concatenate([other_q, model_q])

    cluster_start = np.asarray(tank.cluster_start, dtype=np.float32).copy()
    cluster_end = np.asarray(tank.cluster_end, dtype=np.float32).copy()
    cluster_span = np.asarray(tank.cluster_span, dtype=np.float32).copy()
    cluster_mean = np.asarray(tank.cluster_mean_time, dtype=np.float32).copy()
    cluster_charge = np.asarray(tank.cluster_charge, dtype=np.float32).copy()
    cluster_nhits = np.asarray(tank.cluster_nhits, dtype=np.int32).copy()

    si = int(tank.selected_index)
    if model_tokens > 0:
        cstart = float(np.min(model_ts))
        cend = float(np.max(model_ts))
        cspan = max(0.0, cend - cstart)
        cmean = core_mean_ns
    else:
        cstart = cend = cspan = cmean = 0.0

    cluster_start[si] = cstart
    cluster_end[si] = cend
    cluster_span[si] = cspan
    cluster_mean[si] = cmean
    cluster_charge[si] = core_total_pe
    cluster_nhits[si] = model_tokens

    cleaned = SelectedTank(
        cluster_time=np.asarray(tank.cluster_time, dtype=np.float32).copy(),
        cluster_start=cluster_start,
        cluster_end=cluster_end,
        cluster_span=cluster_span,
        cluster_mean_time=cluster_mean,
        cluster_charge=cluster_charge,
        cluster_nhits=cluster_nhits,
        selected_index=si,
        schema_v4_selected_index=int(tank.schema_v4_selected_index),
        earliest_index=int(tank.earliest_index),
        all_cid=new_cid,
        all_det=new_det,
        all_ts=new_ts,
        all_q=new_q,
        # Preserve the original ClusterMap cluster time as the sequence timing
        # reference.  Association has already used the raw mean and is unchanged.
        selected_time=float(tank.selected_time),
        selected_charge=core_total_pe,
        selected_span=cspan,
        selected_det=model_det,
        selected_ts=model_ts,
        selected_q=model_q,
        source=tank.source,
        packet_reordered=bool(tank.packet_reordered),
    )

    diag.update({
        "tank_per_pmt_kept_raw_hits": int(kept_raw_hits),
        "tank_per_pmt_model_tokens": model_tokens,
        "tank_per_pmt_early_merged_hits": max(0, int(kept_raw_hits) - model_tokens),
        "tank_per_pmt_late_removed_hits": int(late_removed_hits),
        "tank_per_pmt_core_total_pe": core_total_pe,
        "tank_per_pmt_late_removed_pe": float(late_removed_pe),
        "tank_per_pmt_core_mean_first_hit_ns": core_mean_ns,
        "tank_per_pmt_full_minus_core_mean_ns": (
            float(raw_mean_ns - core_mean_ns)
            if raw_hits > 0 and model_tokens > 0 else -999.0
        ),
    })
    return cleaned, diag

def _eventselector_mrd_cluster_means(
    mrd_obj: Mapping[str, Any],
    geometry: GeometryBundle,
) -> Tuple[List[float], List[int], List[float], List[int]]:
    """Build EventSelector-style MRD TimeClustering mean times.

    EventSelector averages only digits belonging to detector element "MRD".
    The frozen cMRD geometry's chankey_to_layer >= 0 is the runtime MRD identity;
    veto/unknown channels are excluded from this coincidence mean.
    """
    chankeys = _numeric_array(
        mrd_obj.get("digit_chankeys", []),
        "packet.mrd.digit_chankeys",
        np.int64,
    )
    times = _numeric_array(
        mrd_obj.get("digit_times_ns", []),
        "packet.mrd.digit_times_ns",
        np.float64,
    )
    charges = _numeric_array(
        mrd_obj.get("digit_charges", []),
        "packet.mrd.digit_charges",
        np.float64,
    )
    lengths = {"chankeys": chankeys.size, "times": times.size, "charges": charges.size}
    if len(set(lengths.values())) != 1:
        raise PacketValidationError(f"MRD digit-vector length mismatch: {lengths}")

    raw_clusters = _sequence(
        mrd_obj.get("time_clusters", []), "packet.mrd.time_clusters"
    )

    layer_map = np.asarray(geometry.chankey_to_layer)
    nmap = int(layer_map.shape[0])

    means: List[float] = []
    nhits_mrd_only: List[int] = []
    core_means: List[float] = []
    core_counts: List[int] = []

    for ci, raw in enumerate(raw_clusters):
        seq = _sequence(raw, f"packet.mrd.time_clusters[{ci}]")
        tsum = 0.0
        count = 0
        mrd_indices: List[int] = []

        for raw_idx in seq:
            idx = _integer(raw_idx, f"packet.mrd.time_clusters[{ci}][]")
            if idx < 0 or idx >= int(times.size):
                continue

            ck = int(chankeys[idx])
            if ck < 0 or ck >= nmap:
                continue
            if int(layer_map[ck]) < 0:
                continue

            tsum += float(times[idx])
            count += 1
            mrd_indices.append(idx)

        # EventSelector initializes the per-cluster mean accumulator to zero.
        # This full-cluster mean is preserved verbatim so the mimic can still be
        # shown to reproduce EventSelector's accept/reject decisions.
        means.append(float(tsum / count) if count > 0 else 0.0)
        nhits_mrd_only.append(int(count))

        # Prompt-core mean: same MRD-identity digit set, minus the ringing and
        # noise tail that drags the full mean late in proportion to the
        # cluster's time span.
        kept, _ring, _noise = _prompt_core_keep(
            mrd_indices, times, chankeys, layer_map
        )
        if len(kept) >= MRD_CORE_MIN_KEPT_HITS:
            core_means.append(float(np.mean(times[np.asarray(kept, dtype=np.int64)])))
            core_counts.append(int(len(kept)))
        else:
            # Degenerate core: fall back to the untrimmed mean rather than
            # invent a coincidence time from one or zero digits.
            core_means.append(means[-1])
            core_counts.append(int(count))

    return means, nhits_mrd_only, core_means, core_counts


def _associate_tank_mrd_eventselector(
    tank: SelectedTank,
    mrd_obj: Mapping[str, Any],
    geometry: GeometryBundle,
    is_data: bool,
) -> TankMrdAssociation:
    """Mimic EventSelector's Tank-charge + Tank/MRD timing selection.

    Data behavior:
      * iterate Tank clusters in ClusterMap/std::map chronological order;
      * use each cluster's mean hit time and total calibrated PE;
      * skip Tank clusters with mean hit time > 2000 ns;
      * starting from max_charge=0, choose the largest charge using strict >;
      * compare only that Tank cluster against MRD TimeClustering mean times;
      * accept strict 705 ns < (MRDmean - Tankmean) < 805 ns.

    For MC the same logic uses a 0 ns expected offset with the same +/-50 ns
    strict window.

    EventSelector stores all compatible MRD indices.  cMRD requires one MRD
    tensor, so the first compatible index in TimeClustering vector order is the
    minimal deterministic adaptation; compatible_pair_count records all matches.
    """
    expected = DATA_PMT_MRD_OFFSET_NS if bool(is_data) else MC_PMT_MRD_OFFSET_NS
    window = float(PMT_MRD_COINCIDENCE_HALF_WINDOW_NS)

    tank_means = np.asarray(tank.cluster_mean_time, dtype=np.float64)
    tank_charges = np.asarray(tank.cluster_charge, dtype=np.float64)
    tank_nhits = np.asarray(tank.cluster_nhits, dtype=np.int64)

    mrd_means_full, mrd_counts_full, mrd_means_core, mrd_counts_core = (
        _eventselector_mrd_cluster_means(mrd_obj, geometry)
    )
    use_core = bool(MRD_ASSOCIATION_USE_CORE_MEAN)
    mrd_means = mrd_means_core if use_core else mrd_means_full

    valid_tank_count = int(np.sum(
        (tank_nhits > 0)
        & np.isfinite(tank_means)
        & np.isfinite(tank_charges)
    ))

    # No retained timing-defined subevent on one side: preserve fallback semantics.
    if tank_means.size == 0 or len(mrd_means) == 0:
        return TankMrdAssociation(
            applied=False,
            found=False,
            compatible_pair_count=0,
            evaluated_pair_count=0,
            selected_tank_cluster_index=-1,
            selected_mrd_subevent_id=-1,
            expected_offset_ns=float(expected),
            coincidence_window_ns=window,
            tank_mean_time_ns=-999.0,
            mrd_mean_time_ns=-999.0,
            dt_ns=-999.0,
            timing_residual_ns=-999.0,
        )

    selected_tank = -1
    selected_tank_mean = -999.0
    max_charge = 0.0

    for ti in range(int(tank_means.size)):
        if int(tank_nhits[ti]) <= 0:
            continue
        tmean = float(tank_means[ti])
        qsum = float(tank_charges[ti])
        if not math.isfinite(tmean) or not math.isfinite(qsum):
            continue
        if tmean > EVENTSELECTOR_TANK_MEAN_TIME_MAX_NS:
            continue

        # Strict > exactly reproduces EventSelector's charge update.
        if qsum > max_charge:
            max_charge = qsum
            selected_tank = int(ti)
            selected_tank_mean = tmean

    if selected_tank < 0:
        return TankMrdAssociation(
            applied=True,
            found=False,
            compatible_pair_count=0,
            evaluated_pair_count=len(mrd_means),
            selected_tank_cluster_index=-1,
            selected_mrd_subevent_id=-1,
            expected_offset_ns=float(expected),
            coincidence_window_ns=window,
            tank_mean_time_ns=-999.0,
            mrd_mean_time_ns=-999.0,
            dt_ns=-999.0,
            timing_residual_ns=-999.0,
            mrd_mean_used_core=use_core,
        )

    compatible: List[Tuple[int, float, float, float]] = []
    for mj, raw_mmean in enumerate(mrd_means):
        mmean = float(raw_mmean)
        if not math.isfinite(mmean):
            continue
        dt = mmean - selected_tank_mean

        # EventSelector uses strict lower and upper inequalities.
        if dt > float(expected) - window and dt < float(expected) + window:
            residual = abs(dt - float(expected))
            compatible.append((int(mj), mmean, float(dt), float(residual)))

    if not compatible:
        return TankMrdAssociation(
            applied=True,
            found=False,
            compatible_pair_count=0,
            evaluated_pair_count=len(mrd_means),
            selected_tank_cluster_index=int(selected_tank),
            selected_mrd_subevent_id=-1,
            expected_offset_ns=float(expected),
            coincidence_window_ns=window,
            tank_mean_time_ns=float(selected_tank_mean),
            mrd_mean_time_ns=-999.0,
            dt_ns=-999.0,
            timing_residual_ns=-999.0,
            mrd_mean_used_core=use_core,
        )

    # EventSelector fills MRDCoincidenceCluster in MRD vector order.
    mj, mmean, dt, residual = compatible[0]
    mmean_full = float(mrd_means_full[mj])
    return TankMrdAssociation(
        applied=True,
        found=True,
        compatible_pair_count=len(compatible),
        evaluated_pair_count=len(mrd_means),
        selected_tank_cluster_index=int(selected_tank),
        selected_mrd_subevent_id=int(mj),
        expected_offset_ns=float(expected),
        coincidence_window_ns=window,
        tank_mean_time_ns=float(selected_tank_mean),
        mrd_mean_time_ns=float(mmean),
        dt_ns=float(dt),
        timing_residual_ns=float(residual),
        mrd_mean_used_core=use_core,
        mrd_mean_full_time_ns=mmean_full,
        dt_full_ns=float(mmean_full - selected_tank_mean),
        mrd_core_hits=int(mrd_counts_core[mj]),
        mrd_full_hits=int(mrd_counts_full[mj]),
        mrd_core_shift_ns=float(mmean_full - mmean),
    )

def _validate_tensors(tensors: MutableMapping[str, np.ndarray]) -> Dict[str, np.ndarray]:
    missing = set(EXPECTED_INPUT_SHAPES) - set(tensors)
    extra = set(tensors) - set(EXPECTED_INPUT_SHAPES)
    if missing or extra:
        raise FeatureValidationError(
            f"model-input key mismatch: missing={sorted(missing)}, extra={sorted(extra)}"
        )

    validated: Dict[str, np.ndarray] = {}
    for name, expected_shape in EXPECTED_INPUT_SHAPES.items():
        dtype = np.int32 if name in INT_INPUTS else np.float32
        arr = np.asarray(tensors[name], dtype=dtype)
        if tuple(arr.shape) != tuple(expected_shape):
            raise FeatureValidationError(
                f"{name} has shape {arr.shape}, expected {expected_shape}"
            )
        if np.issubdtype(arr.dtype, np.floating) and not np.all(np.isfinite(arr)):
            bad = np.argwhere(~np.isfinite(arr))[:5].tolist()
            raise FeatureValidationError(f"{name} contains non-finite values at {bad}")
        validated[name] = np.ascontiguousarray(arr)

    mrd_idx = validated["mrd_hit_idx"]
    tank_idx = validated["tank_hit_idx"]
    if np.any((mrd_idx < 0) | (mrd_idx > MRD_PAD_IDX)):
        raise FeatureValidationError("mrd_hit_idx contains values outside [0,332]")
    if np.any((tank_idx < 0) | (tank_idx > TANK_PAD_IDX)):
        raise FeatureValidationError("tank_hit_idx contains values outside [0,500]")
    return validated


class RuntimeFeatureBuilder:
    """Frozen-geometry, detector-only cMRD v5 event feature builder."""

    def __init__(
        self,
        geometry_dir: str,
        expected_geometry_sha256: Optional[str] = None,
        verify_geometry_checksums: bool = True,
    ) -> None:
        self.geometry: GeometryBundle = load_geometry_bundle(
            geometry_dir, verify_checksums=verify_geometry_checksums
        )
        self.expected_geometry_sha256 = expected_geometry_sha256 or ""
        if self.expected_geometry_sha256 and (
            self.geometry.manifest_sha256 != self.expected_geometry_sha256
        ):
            raise RuntimeFeatureError(
                "geometry manifest SHA-256 mismatch: "
                f"expected {self.expected_geometry_sha256}, "
                f"got {self.geometry.manifest_sha256}"
            )

    def build(self, packet: Mapping[str, Any]) -> FeatureBuildResult:
        if not isinstance(packet, Mapping):
            raise PacketValidationError("event packet must be a mapping")
        event_obj, mrd_obj, tank_obj = _validate_packet_header(packet)

        # Parse both detector cluster collections.  These default selections are
        # preserved as schema-v4 provenance and one-detector fallbacks.
        layer_map = np.asarray(self.geometry.chankey_to_layer)
        default_tank = _select_tank(tank_obj)
        default_mrd = _select_mrd(mrd_obj, layer_map=layer_map)

        association = _associate_tank_mrd_eventselector(
            default_tank,
            mrd_obj,
            self.geometry,
            is_data=bool(event_obj.get("is_data", False)),
        )

        if association.applied and association.found:
            # Retarget both detectors to EventSelector's charge+timing pair.
            selected_tank = _select_tank(
                tank_obj,
                selected_index_override=association.selected_tank_cluster_index,
            )
            selected_mrd = _select_mrd(
                mrd_obj,
                selected_index_override=association.selected_mrd_subevent_id,
                layer_map=layer_map,
            )
        elif association.applied:
            # Both sides had retained clusters but EventSelector found no
            # coincidence.  Do not feed an unrelated Tank cluster.  Retain the
            # schema-v4 MRD fallback for the inclusive MRD-only gate path.
            selected_tank = _select_tank(tank_obj, selected_index_override=-1)
            selected_mrd = default_mrd
        else:
            selected_tank = default_tank
            selected_mrd = default_mrd

        mrd = build_mrd_sequence(
            selected_mrd.chankeys,
            selected_mrd.times_ns,
            selected_mrd.charges,
            self.geometry,
            selected_mrd.selected_start_ns,
            remove_known_dead=True,
        )
        layer_features, layer_mask, mrd_fit = build_mrd_layer_features(
            mrd, self.geometry
        )

        model_tank, tank_core_diag = _apply_tank_per_pmt_core(selected_tank)

        tank = build_tank_sequence(
            model_tank.selected_det,
            model_tank.selected_ts,
            model_tank.selected_q,
            model_tank.selected_time,
        )
        cluster_features, cluster_mask, cluster_summary = build_tank_cluster_features(
            model_tank.cluster_time,
            model_tank.cluster_start,
            model_tank.cluster_end,
            model_tank.cluster_span,
            model_tank.cluster_charge,
            model_tank.cluster_nhits,
            model_tank.all_cid,
            model_tank.all_det,
            model_tank.all_q,
            model_tank.selected_index,
            model_tank.earliest_index,
            self.geometry,
        )
        physics = build_physics_features(
            mrd,
            layer_features,
            layer_mask,
            mrd_fit,
            tank,
            cluster_features,
            cluster_mask,
            cluster_summary,
            selected_mrd.selected_span_ns,
            selected_mrd.unassigned_hits,
            model_tank.selected_charge,
            max(0.0, model_tank.selected_span),
            self.geometry,
        )

        tensors = _validate_tensors(
            {
                "mrd_hit_dyn": mrd["dyn"],
                "mrd_hit_idx": mrd["idx"],
                "mrd_layer_features": layer_features,
                "mrd_layer_mask": layer_mask,
                "tank_hit_dyn": tank["dyn"],
                "tank_hit_idx": tank["idx"],
                "tank_prompt_mask": tank["prompt_mask"],
                "tank_cluster_features": cluster_features,
                "tank_cluster_mask": cluster_mask,
                # Raw detector-derived physics vector. Deployment SavedModels
                # perform fixed standardization inside the graph.
                "physics": physics,
            }
        )

        valid_layers = np.flatnonzero(layer_mask > 0)
        runtime_conditions = _mapping(
            packet.get("runtime_conditions", {}), "packet.runtime_conditions"
        )
        additional_dead = runtime_conditions.get("additional_dead_mrd_chankeys", [])
        additional_dead_arr = _numeric_array(
            additional_dead,
            "packet.runtime_conditions.additional_dead_mrd_chankeys",
            np.int64,
        )

        selected_ck = np.asarray(selected_mrd.chankeys, dtype=np.int64)
        in_range = (
            (selected_ck >= 0)
            & (selected_ck < self.geometry.chankey_to_layer.shape[0])
        )
        geometry_valid_hits = int(np.sum(
            in_range
            & (
                self.geometry.chankey_to_layer[
                    np.clip(
                        selected_ck,
                        0,
                        self.geometry.chankey_to_layer.shape[0] - 1,
                    )
                ] >= 0
            )
        )) if selected_ck.size else 0
        live_before_dedup = int(mrd["raw_valid_count"])
        known_dead_removed = max(0, geometry_valid_hits - live_before_dedup)

        diagnostics: Dict[str, Any] = {
            "runtime_feature_version": RUNTIME_FEATURE_VERSION,
            "geometry_manifest_sha256": self.geometry.manifest_sha256,
            "geometry_version": str(self.geometry.manifest.get("version", "")),
            "run_number": int(event_obj.get("run_number", 0)),
            "subrun_number": int(event_obj.get("subrun_number", 0)),
            "event_number": int(event_obj.get("event_number", -1)),
            "mc_event_num": int(event_obj.get("mc_event_num", 0)),
            "mc_trigger_num": int(event_obj.get("mc_trigger_num", -1)),
            "is_data": bool(event_obj.get("is_data", False)),

            "mrd_source": selected_mrd.source,
            "number_mrd_time_clusters": len(selected_mrd.cluster_num_hits),
            "selected_mrd_cluster_index": selected_mrd.selected_index,
            "selected_mrd_cluster_start_time_ns": selected_mrd.selected_start_ns,
            "selected_mrd_cluster_end_time_ns": selected_mrd.selected_end_ns,
            "selected_mrd_cluster_time_span_ns": selected_mrd.selected_span_ns,
            "mrd_schema_v4_selected_cluster_index": default_mrd.schema_v4_selected_index,

            # Canonical EventSelector-mimic Tank<->MRD diagnostics.
            "tank_mrd_association_applied": association.applied,
            "tank_mrd_association_found": association.found,
            "tank_mrd_compatible_pair_count": association.compatible_pair_count,
            "tank_mrd_evaluated_pair_count": association.evaluated_pair_count,
            "tank_mrd_selected_tank_cluster_index": association.selected_tank_cluster_index,
            "tank_mrd_selected_mrd_subevent_id": association.selected_mrd_subevent_id,
            "tank_mrd_expected_offset_ns": association.expected_offset_ns,
            "tank_mrd_coincidence_window_ns": association.coincidence_window_ns,
            "tank_mrd_tank_mean_time_ns": association.tank_mean_time_ns,
            "tank_mrd_mrd_mean_time_ns": association.mrd_mean_time_ns,
            "tank_mrd_dt_ns": association.dt_ns,
            "tank_mrd_timing_residual_ns": association.timing_residual_ns,

            # Prompt-core trim diagnostics (runtime 0.1.4).
            "tank_mrd_mrd_mean_used_core": association.mrd_mean_used_core,
            "tank_mrd_mrd_mean_full_time_ns": association.mrd_mean_full_time_ns,
            "tank_mrd_dt_full_ns": association.dt_full_ns,
            "tank_mrd_mrd_core_hits": association.mrd_core_hits,
            "tank_mrd_mrd_full_hits": association.mrd_full_hits,
            "tank_mrd_mrd_core_shift_ns": association.mrd_core_shift_ns,
            "mrd_core_window_ns": float(MRD_CORE_WINDOW_NS),
            "mrd_trim_selected_cluster_enabled": bool(MRD_TRIM_SELECTED_CLUSTER),
            "mrd_selected_trim_applied": selected_mrd.trim_applied,
            "mrd_selected_trim_removed_ringing": selected_mrd.trim_removed_ringing,
            "mrd_selected_trim_removed_noise": selected_mrd.trim_removed_noise,
            "mrd_selected_trim_removed_total": selected_mrd.trim_removed_total,
            "mrd_selected_untrimmed_hits": selected_mrd.untrimmed_selected_hits,
            "mrd_selected_untrimmed_time_span_ns": selected_mrd.untrimmed_selected_span_ns,
            "tank_mrd_changed_from_schema_v4": bool(
                association.found
                and (
                    association.selected_tank_cluster_index
                    != default_tank.schema_v4_selected_index
                    or association.selected_mrd_subevent_id
                    != default_mrd.schema_v4_selected_index
                )
            ),
            "tank_mrd_rejected_no_match": bool(
                association.applied and not association.found
            ),

            # Backward aliases retained so existing C++/PhaseII diagnostics do
            # not need to change for this A/B runtime test.
            "mrd_tank_association_applied": association.applied,
            "mrd_tank_association_found": association.found,
            "mrd_tank_association_candidate_count": association.compatible_pair_count,
            "mrd_tank_association_expected_offset_ns": association.expected_offset_ns,
            "mrd_tank_association_window_ns": association.coincidence_window_ns,
            "mrd_tank_association_tank_mean_time_ns": association.tank_mean_time_ns,
            "mrd_tank_association_selected_mrd_mean_time_ns": association.mrd_mean_time_ns,
            "mrd_tank_association_selected_dt_ns": association.dt_ns,
            "mrd_tank_association_residual_ns": association.timing_residual_ns,
            "mrd_tank_association_changed_selection": bool(
                association.found
                and association.selected_mrd_subevent_id
                != default_mrd.schema_v4_selected_index
            ),
            "mrd_tank_association_rejected_no_match": bool(
                association.applied and not association.found
            ),

            "input_mrd_raw_hits": selected_mrd.raw_hit_count,
            "input_mrd_selected_raw_hits": int(selected_mrd.chankeys.size),
            "input_mrd_geometry_valid_hits_before_dead_mask": geometry_valid_hits,
            "input_mrd_known_dead_removed": known_dead_removed,
            "input_mrd_live_hits_before_dedup": live_before_dedup,
            "input_mrd_live_unique_hits": int(mrd["length"]),
            "input_mrd_dedup_removed": int(mrd["dedup_removed"]),
            "input_mrd_unassigned_hits": int(selected_mrd.unassigned_hits),
            "input_mrd_layers": int(valid_layers.size),
            "input_mrd_first_layer": int(valid_layers[0]) if valid_layers.size else -1,
            "input_mrd_last_layer": int(valid_layers[-1]) if valid_layers.size else -1,

            "tank_source": selected_tank.source,
            "number_tank_clusters": int(selected_tank.cluster_time.size),
            "selected_tank_cluster_index": selected_tank.selected_index,
            "tank_schema_v4_selected_cluster_index": default_tank.schema_v4_selected_index,
            "selected_tank_cluster_time_ns": selected_tank.selected_time,
            "selected_tank_cluster_charge_pe": selected_tank.selected_charge,
            "selected_tank_cluster_time_span_ns": max(0.0, selected_tank.selected_span),
            "input_tank_hits": int(tank["length"]),
            "tank_packet_reordered_by_time": selected_tank.packet_reordered,

            # Model-facing Tank per-PMT de-ringing diagnostics.
            **tank_core_diag,
            "tank_model_selected_cluster_charge_pe": model_tank.selected_charge,
            "tank_model_selected_cluster_time_span_ns": max(0.0, model_tank.selected_span),

            "reco_structurally_eligible": bool(
                int(mrd["length"]) >= MIN_MRD_HITS
                and int(tank["length"]) >= MIN_TANK_HITS
            ),
            "known_dead_mrd_chankeys": sorted(
                int(v) for v in KNOWN_DEAD_MRD_CHANKEYS
            ),
            # Diagnostic only. Runtime conditions do not mutate frozen model mask.
            "additional_dead_mrd_chankeys": [
                int(v) for v in additional_dead_arr
            ],
            "physics_is_raw_unstandardized": True,
        }
        return FeatureBuildResult(tensors=tensors, diagnostics=diagnostics)

def build_runtime_features(
    packet: Mapping[str, Any],
    geometry_dir: str,
    expected_geometry_sha256: Optional[str] = None,
) -> FeatureBuildResult:
    """Convenience one-shot API; persistent daemons should reuse the class."""
    return RuntimeFeatureBuilder(
        geometry_dir,
        expected_geometry_sha256=expected_geometry_sha256,
        verify_geometry_checksums=True,
    ).build(packet)


__all__ = [
    "PACKET_SCHEMA",
    "PACKET_SCHEMA_VERSION",
    "RUNTIME_FEATURE_VERSION",
    "MRD_CORE_WINDOW_NS",
    "MRD_CORE_DEDUP_RINGING",
    "MRD_CORE_LAYER_GUARD",
    "MRD_CORE_MIN_KEPT_HITS",
    "MRD_ASSOCIATION_USE_CORE_MEAN",
    "MRD_TRIM_SELECTED_CLUSTER",
    "TANK_PER_PMT_CORE_WINDOW_NS",
    "TANK_PER_PMT_CORE_ENABLED",
    "DATA_PMT_MRD_OFFSET_NS",
    "MC_PMT_MRD_OFFSET_NS",
    "PMT_MRD_COINCIDENCE_HALF_WINDOW_NS",
    "EVENTSELECTOR_TANK_MEAN_TIME_MAX_NS",
    "EXPECTED_INPUT_SHAPES",
    "RuntimeFeatureError",
    "PacketValidationError",
    "FeatureValidationError",
    "FeatureBuildResult",
    "RuntimeFeatureBuilder",
    "build_runtime_features",
]
