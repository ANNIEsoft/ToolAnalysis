#!/usr/bin/env python3
"""Pure NumPy feature construction for ANNIE cMrdML schema v5.

This module deliberately has no ROOT or TensorFlow dependency.  The ROOT ->
HDF5 extractor from my training server, validation utilities, and future overlay builders can therefore
share exactly the same detector-derived feature definitions.

Coordinate convention
---------------------
All positions passed to these functions are ANNIE global coordinates in cm.
The schema-v4 writer stores translated coordinates; the extractor is responsible
for converting those truth targets back to global cm before calling this module.
"""

from __future__ import annotations

import hashlib
import json
import math
from dataclasses import dataclass
from pathlib import Path
from typing import Any, Dict, Iterable, List, Mapping, Optional, Sequence, Tuple

import numpy as np
# Frozen dimensions and selection settings used when constructing the v5 model
# inputs. Real MRD indices are 0-331 and real tank indices are 0-499; indices
# 332 and 500 are reserved all-zero padding entries for fixed-length sequences really could do the
#specific numbers but eh
N_MRD_CHANNELS = 332
N_MRD_LAYERS = 11
N_TANK_DETKEYS = 500
MRD_PAD_IDX = 332
TANK_PAD_IDX = 500
MAX_MRD_HITS = 100
MAX_TANK_HITS = 200
MAX_TANK_CLUSTERS = 16
MIN_MRD_HITS = 4
MIN_TANK_HITS = 1
PROMPT_LSTM_FRAC = 0.30
PROMPT_LSTM_MIN_HITS = 3
PROMPT_SCALAR_FRAC = 0.30
PROMPT_SCALAR_MIN_HITS = 5
MAX_TANK_CHARGE_PE = 200.0

# Kept in one place so extractor and trainer cannot silently diverge. these are what this version was trained one
# if you look at slides versions before v2q were trained on 50 missing channels so we can recover worse detectors but need training for setups
KNOWN_DEAD_MRD_CHANKEYS = frozenset([
    51, 123, 124, 133, 140, 141,
    195, 203, 206, 215, 217, 218,
    220, 224, 233, 251, 254, 260,
    271, 275, 325,
])# from models training backed by effiency studies and known dead of most epochs
#really guys we should of caught these back in 2022
# The LSTM prompt mask and prompt centroid scalars use separate frozen small-N
# policies: minimum 3 hits for the sequence mask, but minimum 5 for the scalar
# centroid calculation.


# Frozen feature names contracts for the five detector input branches. The list
# order is the tensor column order used by extraction, training, metadata, and
# deployment validation so these names must not be reordered, inserted, or renamed
# without rebuilding the HDF5 files and retraining/exporting the models luckily it just goes off 
# of the start wcsim or loadannie then timeclustering and clusterfinder.
MRD_DYN_FEATURE_NAMES = [
    "time_rel_cluster_start_50ns",
    "time_within_layer_30ns",
    "sequence_position",
]

TANK_DYN_FEATURE_NAMES = [
    "time_rel_selected_cluster_50ns",
    "log_charge_norm",
]

MRD_LAYER_FEATURE_NAMES = [
    "valid",
    "axis_is_x",
    "z_from_front_norm",
    "n_hits_norm",
    "live_occupancy_fraction",
    "earliest_time_rel_50ns",
    "mean_time_rel_50ns",
    "time_rms_30ns",
    "coord_mean_norm",
    "coord_rms_norm",
    "coord_min_norm",
    "coord_max_norm",
    "coord_width_norm",
    "max_gap_norm",
    "two_band_score",
    "global_line_residual_norm",
]

TANK_CLUSTER_FEATURE_NAMES = [
    "valid",
    "time_from_first_5000ns",
    "time_span_200ns",
    "log_charge_norm",
    "n_hits_norm",
    "charge_fraction",
    "selected_highest_charge",
    "earliest_cluster",
    "max_hit_charge_fraction",
    "charge_balance_max_over_total",
    "centroid_x_norm",
    "centroid_y_norm",
    "centroid_z_norm",
]

PHYSICS_FEATURE_NAMES = [
    # MRD occupancy/timing
    "mrd_sequence_valid",
    "mrd_n_hits_norm",
    "mrd_n_layers_norm",
    "mrd_first_layer_norm",
    "mrd_last_layer_norm",
    "mrd_layer_span_norm",
    "mrd_selected_time_span_30ns",
    "mrd_unassigned_fraction",
    "mrd_dedup_removed_fraction",
    "mrd_mean_live_occupancy",
    "mrd_max_layer_width_norm",
    "mrd_mean_two_band_score",
    "mrd_max_two_band_score",
    # global x/y fits
    "mrd_x_fit_valid",
    "mrd_x_slope",
    "mrd_x_intercept_norm",
    "mrd_x_residual_rms_norm",
    "mrd_y_fit_valid",
    "mrd_y_slope",
    "mrd_y_intercept_norm",
    "mrd_y_residual_rms_norm",
    # early/late scattering proxies
    "mrd_early_x_fit_valid",
    "mrd_late_x_fit_valid",
    "mrd_early_x_slope",
    "mrd_late_x_slope",
    "mrd_delta_x_slope",
    "mrd_early_y_fit_valid",
    "mrd_late_y_fit_valid",
    "mrd_early_y_slope",
    "mrd_late_y_slope",
    "mrd_delta_y_slope",
    "mrd_early_late_scatter_angle_rad",
    "mrd_max_local_kink_rad",
    "mrd_mean_local_kink_rad",
    "mrd_path_excess_fraction",
    "mrd_residual_growth",
    "mrd_two_line_improvement_x",
    "mrd_two_line_improvement_y",
    "mrd_entry_x_norm",
    "mrd_entry_y_norm",
    "mrd_exit_x_norm",
    "mrd_exit_y_norm",
    # Detector only effective-iron/range proxies inspired by Julie's MRD range
    # treatment. They use reconstructed hit geometry and fitted slopes only no
    # truth energy, truth direction, or particle information enters these values. 
    # pulled logic from her thesis chapter 6
    "mrd_julie_proxy_valid",
    "mrd_detector_cos_theta",
    "mrd_straight_chord_length_200cm",
    "mrd_julie_effective_iron_length_200cm",
    "mrd_julie_effective_to_chord_ratio",
    # tank selected-cluster summaries
    "tank_selected_valid",
    "tank_n_hits_norm",
    "tank_log_charge_norm",
    "tank_time_span_200ns",
    "tank_n_clusters_norm",
    "tank_selected_charge_fraction",
    "tank_second_over_first_charge",
    "tank_cluster_charge_entropy",
    "tank_max_hit_charge_fraction",
    "tank_prompt_lstm_fraction",
    "tank_prompt_scalar_valid",
    # tank centroids / spreads
    "tank_full_centroid_x_norm",
    "tank_full_centroid_y_norm",
    "tank_full_centroid_z_norm",
    "tank_full_rms_radius_norm",
    "tank_prompt_centroid_x_norm",
    "tank_prompt_centroid_y_norm",
    "tank_prompt_centroid_z_norm",
    "tank_prompt_rms_radius_norm",
    "tank_prompt_time_rms_50ns",
    "tank_prompt_phi_sin",
    "tank_prompt_phi_cos",
    # detector-only rough consistency/ambiguity
    "tank_prompt_to_mrd_phi_sin_delta",
    "tank_prompt_to_mrd_phi_cos_delta",
    "single_line_quality",
    "multi_track_ambiguity_proxy",
]

# Utility helpers for the frozen geometry package these compute file hashes, load an
# optional json manifest, and append the all zero padding row required by the
# MRD and tank geometry embedding tables.
def sha256_file(path: Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as f:
        for block in iter(lambda: f.read(1024 * 1024), b""):#reads in 1 Mib chunks, gets verified via hash
            h.update(block)
    return h.hexdigest()


def _safe_json(path: Path) -> Dict[str, Any]:
    if not path.exists():
        return {}
    with path.open() as f:
        return json.load(f)


def _append_zero_row(arr: np.ndarray) -> np.ndarray:
    return np.concatenate(
        [arr.astype(np.float32, copy=False), np.zeros((1, arr.shape[1]), np.float32)],
        axis=0,
    )

#for below container for the checksum and verify of frozen ml geometry
#this holds the lookup tables, physical detector coordinates, channel masks, manifest
#normalization scales used for the feature builders
@dataclass
class GeometryBundle:
    root: Path
    mrd_features: np.ndarray
    tank_features: np.ndarray
    chankey_to_layer: np.ndarray
    chankey_to_pos_global_cm: np.ndarray
    chankey_to_size_cm: np.ndarray
    chankey_to_bounds_global_cm: np.ndarray
    mrd_layer_z_center_cm: np.ndarray
    mrd_active_box_global_cm: np.ndarray
    mrd_detector_envelope_global_cm: np.ndarray
    tank_pos_global_cm: np.ndarray
    tank_center_global_cm: np.ndarray
    tank_active: np.ndarray
    manifest: Dict[str, Any]
    manifest_sha256: str
    mrd_feature_names: List[str]
    tank_feature_names: List[str]

    @property
    def mrd_x_half_cm(self) -> float:
        env = self.mrd_detector_envelope_global_cm
        return max(abs(float(env[0])), abs(float(env[1])))
# used later as x_normalized = x_cm / geo.mrd_x_half_cm
# coordinate scales are the biggest absolute global contianing coordinates,
# matching the normalization used during making training data set they are not recomputed
# as half of the min-to-max span.
    @property
    def mrd_y_half_cm(self) -> float:
        env = self.mrd_detector_envelope_global_cm
        return max(abs(float(env[2])), abs(float(env[3])))

    @property
    def mrd_z_front_cm(self) -> float:
        return float(self.mrd_detector_envelope_global_cm[4])

    @property
    def mrd_z_back_cm(self) -> float:
        return float(self.mrd_detector_envelope_global_cm[5])

    @property
    def mrd_z_span_cm(self) -> float:
        return max(1.0, self.mrd_z_back_cm - self.mrd_z_front_cm)

    @property
    def tank_scales_cm(self) -> np.ndarray:
        # Inactive detkey rows are zero-filled in the geometry table and are not
        # physical PMT positions.  Excluding them avoids falsely using the tank
        # centre offset itself as the z scale.
        active = np.asarray(self.tank_active, dtype=bool)
        pos = self.tank_pos_global_cm[active] if np.any(active) else self.tank_pos_global_cm
        rel = pos - self.tank_center_global_cm[None, :]
        s = np.nanmax(np.abs(rel), axis=0)
        s[s < 1.0] = 1.0
        return s.astype(np.float32)
# above derives per acis tank coords from active pmt positions 
# this is compared to the frozen tank center
# forces a 1cm minimum so the centroid cant be divided by zero

#for load_geometry_bundle, requries every model lookup array to enforce a fixed mrd tank embedding, and then append the zero rows
#made from padding and returns the complete geom and relase as a single bundle GeometryBundle
def load_geometry_bundle(geo_dir: str, verify_checksums: bool = True) -> GeometryBundle:
    root = Path(geo_dir).expanduser().resolve()#missing or mistyped geometry dics are immediately fatal
    if not root.is_dir():
        raise FileNotFoundError("Geometry directory does not exist: %s" % root)

    manifest_path = root / "geometry_manifest_v3.json"
    if not manifest_path.exists():
        alt = root / "geometry_manifest_v3_tilt_approx.json"
        manifest_path = alt if alt.exists() else manifest_path
    manifest = _safe_json(manifest_path)

    required = [
        "mrd_chankey_features.npy",
        "tank_detkey_features.npy",
        "chankey_to_layer.npy",
        "chankey_to_pos_global_cm.npy",
        "chankey_to_size_cm.npy",
        "chankey_to_bounds_global_cm.npy",
        "mrd_layer_z_center_cm.npy",
        "mrd_box_global_cm.npy",
        "mrd_detector_envelope_global_cm.npy",
        "detkey_to_pos_global_cm.npy",
        "tank_center_global_cm.npy",
        "tank_detkey_active.npy",
    ]
    missing = [name for name in required if not (root / name).exists()]
    if missing:
        raise FileNotFoundError("Geometry bundle is missing: %s" % ", ".join(missing))

    if verify_checksums and manifest.get("arrays"):
        failures = []
        for name in required:
            rec = manifest["arrays"].get(name)
            if rec and rec.get("sha256"):
                got = sha256_file(root / name)
                if got != rec["sha256"]:
                    failures.append("%s expected %s got %s" % (name, rec["sha256"], got))
        if failures:
            raise ValueError("Geometry checksum failure:\n" + "\n".join(failures))

    mrd_raw = np.load(root / "mrd_chankey_features.npy")
    tank_raw = np.load(root / "tank_detkey_features.npy")
    if mrd_raw.shape != (N_MRD_CHANNELS, 12):
        raise ValueError("Expected MRD geometry (332,12), got %r" % (mrd_raw.shape,))
    if tank_raw.shape != (N_TANK_DETKEYS, 11):
        raise ValueError("Expected tank geometry (500,11), got %r" % (tank_raw.shape,))

    tank_pos_raw = np.load(root / "detkey_to_pos_global_cm.npy").astype(np.float32)
    if tank_pos_raw.ndim != 2 or tank_pos_raw.shape[0] != N_TANK_DETKEYS or tank_pos_raw.shape[1] < 3:
        raise ValueError("detkey_to_pos_global_cm.npy has unexpected shape %r" % (tank_pos_raw.shape,))

    feature_tables = manifest.get("feature_tables", {})
    mrd_names = list(feature_tables.get("mrd_feature_names", ["mrd_f%d" % i for i in range(12)]))
    tank_names = list(feature_tables.get("tank_feature_names", ["tank_f%d" % i for i in range(11)]))

    return GeometryBundle(
        root=root,
        mrd_features=_append_zero_row(mrd_raw),
        tank_features=_append_zero_row(tank_raw),
        chankey_to_layer=np.load(root / "chankey_to_layer.npy").astype(np.int32),
        chankey_to_pos_global_cm=np.load(root / "chankey_to_pos_global_cm.npy").astype(np.float32),
        chankey_to_size_cm=np.load(root / "chankey_to_size_cm.npy").astype(np.float32),
        chankey_to_bounds_global_cm=np.load(root / "chankey_to_bounds_global_cm.npy").astype(np.float32),
        mrd_layer_z_center_cm=np.load(root / "mrd_layer_z_center_cm.npy").astype(np.float32),
        mrd_active_box_global_cm=np.load(root / "mrd_box_global_cm.npy").astype(np.float32),
        mrd_detector_envelope_global_cm=np.load(root / "mrd_detector_envelope_global_cm.npy").astype(np.float32),
        tank_pos_global_cm=tank_pos_raw[:, :3],# only first columns x,y,z are used, the forzen file contains 6 but the others are not needed
        tank_center_global_cm=np.load(root / "tank_center_global_cm.npy").astype(np.float32),
        tank_active=np.load(root / "tank_detkey_active.npy").astype(np.uint8),
        manifest=manifest,
        manifest_sha256=sha256_file(manifest_path) if manifest_path.exists() else "",
        mrd_feature_names=mrd_names,
        tank_feature_names=tank_names,
    )

# next portions sanitize and filer mrd hits and remove the dead channels giving only the 
# earliest hit per paddle and order hits by detector later then time
def finite_or(value: Any, default: float = 0.0) -> float:
    try:
        out = float(value)
    except Exception:
        return float(default)
    return out if math.isfinite(out) else float(default)
# attempts to covnert to python float
#if it gets a nan or fails it will return default

def valid_truth(value: Any) -> bool:
    try:
        v = float(value)
    except Exception:
        return False
    return math.isfinite(v) and v > -900.0
#used by helpers only not the production line

def translated_to_global_cm(value: Any, shift_cm: float) -> float:
    if not valid_truth(value):
        return -999.0
    return float(value) - float(shift_cm)


def vector_translated_to_global_cm(values: Sequence[Any], shifts_cm: Sequence[float]) -> np.ndarray:
    out = np.full(3, -999.0, dtype=np.float32)
    for i in range(3):
        out[i] = translated_to_global_cm(values[i], shifts_cm[i])
    return out

#mrd geom helpers
def is_mrd_channel(ck: int, geo: GeometryBundle) -> bool:
    return 0 <= int(ck) < N_MRD_CHANNELS and int(geo.chankey_to_layer[int(ck)]) >= 0
#defensive channels only in our mrd list are used

def mrd_measure_axis_is_x(ck: int, geo: GeometryBundle) -> bool:
    """True when the paddle is narrow in x and therefore measures x."""
    size = geo.chankey_to_size_cm[int(ck)]
    return bool(float(size[0]) < float(size[1]))

# this is to help with noise or ringing that we get from the discriminators 
def deduplicate_earliest_per_channel(
    chankeys: Sequence[int], times_ns: Sequence[float], charges: Optional[Sequence[float]] = None
) -> Tuple[np.ndarray, np.ndarray, np.ndarray, int]:
    best: Dict[int, Tuple[float, float]] = {}
    if charges is None:
        charges = [0.0] * len(chankeys)
    for ck, t, q in zip(chankeys, times_ns, charges):
        ck_i = int(ck)
        t_f = finite_or(t, 0.0)
        q_f = finite_or(q, 0.0)
        old = best.get(ck_i)
        if old is None or t_f < old[0]:
            best[ck_i] = (t_f, q_f)
    ordered = sorted(best.items(), key=lambda item: (item[1][0], item[0]))
    cks = np.array([item[0] for item in ordered], np.int32)
    ts = np.array([item[1][0] for item in ordered], np.float32)
    qs = np.array([item[1][1] for item in ordered], np.float32)
    return cks, ts, qs, max(0, len(chankeys) - len(cks))

#  model order is layer first, then hit time, then channel key.
# np.lexsort uses the final key as the primary sort key.
def build_mrd_sequence(
    chankeys: Sequence[int],
    times_ns: Sequence[float],
    charges: Sequence[float],
    geo: GeometryBundle,
    cluster_start_ns: Optional[float] = None,
    max_hits: int = MAX_MRD_HITS,
    remove_known_dead: bool = True,
) -> Dict[str, Any]:
    raw_n = min(len(chankeys), len(times_ns), len(charges))
    rows: List[Tuple[int, float, float]] = []
    for ck, t, q in zip(chankeys[:raw_n], times_ns[:raw_n], charges[:raw_n]):
        ck_i = int(ck)
        if not is_mrd_channel(ck_i, geo):
            continue
        if remove_known_dead and ck_i in KNOWN_DEAD_MRD_CHANKEYS:
            continue
        rows.append((ck_i, finite_or(t, 0.0), finite_or(q, 0.0)))

    cks, ts, qs, n_removed = deduplicate_earliest_per_channel(
        [r[0] for r in rows], [r[1] for r in rows], [r[2] for r in rows]
    )
    if cks.size:
        layers = geo.chankey_to_layer[cks]
        order = np.lexsort((cks, ts, layers))
        cks, ts, qs, layers = cks[order], ts[order], qs[order], layers[order]
    else:
        layers = np.zeros(0, np.int32)

    n = min(int(cks.size), int(max_hits))
    cks, ts, qs, layers = cks[:n], ts[:n], qs[:n], layers[:n]
    idx = np.full(max_hits, MRD_PAD_IDX, np.int16)
    dyn = np.zeros((max_hits, len(MRD_DYN_FEATURE_NAMES)), np.float32)

    if n:
        start = finite_or(cluster_start_ns, float(np.min(ts))) if cluster_start_ns is not None else float(np.min(ts))
        t_rel = np.clip((ts - start) / 50.0, -4.0, 4.0)#Negative values are possible when the upstream cluster-map start is later than an individual hit time.
        t_within = np.zeros(n, np.float32)
        for layer in np.unique(layers):
            slots = np.where(layers == layer)[0]
            layer_t0 = float(np.min(ts[slots]))
            t_within[slots] = np.clip((ts[slots] - layer_t0) / 30.0, 0.0, 4.0)
        seq = np.arange(n, dtype=np.float32) / float(max(1, max_hits - 1))
        idx[:n] = cks.astype(np.int16)
        dyn[:n, 0] = t_rel
        dyn[:n, 1] = t_within
        dyn[:n, 2] = seq
# biggest thing between mc and data will be the 750 ns difference from the 50 between mrd and tank
    return {
        "dyn": dyn,
        "idx": idx,
        "length": np.int16(n),
        "raw_valid_count": int(len(rows)),
        "dedup_removed": int(n_removed),
        "chankeys": cks,
        "times_ns": ts,
        "charges": qs,
        "layers": layers,
    }

# requires at least a zero gaurd total hits, ifnot returns no propmt
# when good selects larger of min_hits and capps at all N hits
def prompt_indices(times_ns: np.ndarray, fraction: float, min_hits: int, zero_guard: int) -> np.ndarray:
    n = int(times_ns.size)
    if n < zero_guard:
        return np.zeros(0, np.int32)
    n_prompt = max(int(min_hits), int(math.ceil(float(fraction) * n)))
    n_prompt = min(n, n_prompt)
    return np.argsort(times_ns, kind="stable")[:n_prompt].astype(np.int32)

#checks time order hits from tank hits, clamps negative charge to zero, uses the earliest 200 hits
def build_tank_sequence(
    detkeys: Sequence[int],
    times_ns: Sequence[float],
    charges: Sequence[float],
    selected_cluster_time_ns: float,
    max_hits: int = MAX_TANK_HITS,
) -> Dict[str, Any]:
    n_raw = min(len(detkeys), len(times_ns), len(charges))
    rows = []
    for det, t, q in zip(detkeys[:n_raw], times_ns[:n_raw], charges[:n_raw]):
        det_i = int(det)
        if not (0 <= det_i < N_TANK_DETKEYS):
            continue
        rows.append((det_i, finite_or(t, 0.0), max(0.0, finite_or(q, 0.0))))
    rows.sort(key=lambda x: (x[1], x[0]))
    rows = rows[:max_hits]
    n = len(rows)

    idx = np.full(max_hits, TANK_PAD_IDX, np.int16)
    dyn = np.zeros((max_hits, len(TANK_DYN_FEATURE_NAMES)), np.float32)
    prompt_mask = np.zeros(max_hits, np.uint8)
    if n:
        det = np.array([r[0] for r in rows], np.int32)
        ts = np.array([r[1] for r in rows], np.float32)
        qs = np.array([r[2] for r in rows], np.float32)
        idx[:n] = det.astype(np.int16)
        dyn[:n, 0] = np.clip((ts - float(selected_cluster_time_ns)) / 50.0, -20.0, 20.0)
        dyn[:n, 1] = np.log1p(np.clip(qs, 0.0, MAX_TANK_CHARGE_PE)) / math.log1p(MAX_TANK_CHARGE_PE)# clips charge at 200 PE
        pidx = prompt_indices(ts, PROMPT_LSTM_FRAC, PROMPT_LSTM_MIN_HITS, PROMPT_LSTM_MIN_HITS)# masks earliest hits
        prompt_mask[pidx] = 1
    else:
        det = np.zeros(0, np.int32)
        ts = np.zeros(0, np.float32)
        qs = np.zeros(0, np.float32)

    return {
        "dyn": dyn,
        "idx": idx,
        "prompt_mask": prompt_mask,
        "length": np.int16(n),
        "detkeys": det,
        "times_ns": ts,
        "charges": qs,
    }

# computes a nonnegative wieght 3d centroid and radial rms spread, if empty we get zero
# if all are zero ro negative will fall back to equal wieghting instead of invalid
# assumes weights have the same number of elements as pos hs rows if this is wrong np will spit an error
def _weighted_centroid(pos: np.ndarray, weights: np.ndarray) -> Tuple[np.ndarray, float]:
    if pos.shape[0] == 0:
        return np.zeros(3, np.float32), 0.0
    w = np.maximum(np.asarray(weights, np.float64), 0.0)
    if not np.any(w > 0.0):
        w = np.ones(pos.shape[0], np.float64)
    wsum = float(np.sum(w))
    c = np.sum(pos.astype(np.float64) * w[:, None], axis=0) / max(wsum, 1e-12)
    dr = pos.astype(np.float64) - c[None, :]
    rms = math.sqrt(float(np.sum(w * np.sum(dr * dr, axis=1)) / max(wsum, 1e-12)))
    return c.astype(np.float32), float(rms)

# estimates if a layer coordinates form two speated hit bands
# pretty much checks and scores multiple paths kinda like cell fitter
def _two_band_score(coords: np.ndarray, scale_cm: float) -> Tuple[float, float]:
    x = np.sort(np.asarray(coords, np.float64))
    n = x.size# if number of hits is below 4 it wont try it, confidence was to low in testing for 3 or under need a better mrd to lower this
    if n < 4 or float(np.ptp(x)) < 1e-6:
        return 0.0, 0.0
    mean = float(np.mean(x))
    sse1 = float(np.sum((x - mean) ** 2))
    best_sse = sse1
    best_gap = 0.0
    for split in range(2, n - 1):
        a, b = x[:split], x[split:]
        sse = float(np.sum((a - np.mean(a)) ** 2) + np.sum((b - np.mean(b)) ** 2))
        gap = float(b[0] - a[-1])
        if sse < best_sse:
            best_sse, best_gap = sse, gap
    improvement = max(0.0, (sse1 - best_sse) / max(sse1, 1e-9))
    gap_norm = max(0.0, best_gap / max(scale_cm, 1.0))
    score = improvement * min(1.0, 4.0 * gap_norm)
    return float(np.clip(gap_norm, 0.0, 2.0)), float(np.clip(score, 0.0, 1.0))

# fit a straight line with wieght to the mrd
# ignores nonfinite pairs reejcts fits with fewer then two z positions 
# residual is a sum of squares
def _weighted_line_fit(z: np.ndarray, coord: np.ndarray, weights: Optional[np.ndarray] = None) -> Dict[str, float]:
    z = np.asarray(z, np.float64)
    x = np.asarray(coord, np.float64)
    ok = np.isfinite(z) & np.isfinite(x)
    z, x = z[ok], x[ok]
    if weights is None:
        w = np.ones(z.size, np.float64)
    else:
        w = np.asarray(weights, np.float64)[ok]
        w = np.maximum(w, 1e-6)
    if z.size < 2 or float(np.ptp(z)) < 1e-6:
        return {"valid": 0.0, "slope": 0.0, "intercept": 0.0, "rms": 0.0, "rss": 0.0}
    z0 = float(np.average(z, weights=w))
    x0 = float(np.average(x, weights=w))
    denom = float(np.sum(w * (z - z0) ** 2))
    if denom < 1e-9:
        return {"valid": 0.0, "slope": 0.0, "intercept": x0, "rms": 0.0, "rss": 0.0}
    slope = float(np.sum(w * (z - z0) * (x - x0)) / denom)
    intercept = x0 - slope * z0
    resid = x - (slope * z + intercept)
    rss = float(np.sum(w * resid * resid))
    rms = math.sqrt(rss / max(float(np.sum(w)), 1e-9))
    return {"valid": 1.0, "slope": slope, "intercept": intercept, "rms": rms, "rss": rss}

# measures for a change in trajectory by comparing one global line 
# with every valid pair of line fits in z, returns a residual error bounded 0 to 1
def _two_line_improvement(z: np.ndarray, x: np.ndarray) -> float:
    z = np.asarray(z, np.float64)
    x = np.asarray(x, np.float64)
    order = np.argsort(z)# each candidate split reps a an early trajectory
    z, x = z[order], x[order]
    if z.size < 5:
        return 0.0
    one = _weighted_line_fit(z, x)
    if not one["valid"] or one["rss"] <= 1e-9:
        return 0.0
    best = one["rss"]
    for split in range(2, z.size - 2 + 1):
        a = _weighted_line_fit(z[:split], x[:split])
        b = _weighted_line_fit(z[split:], x[split:])
        if a["valid"] and b["valid"]:
            best = min(best, a["rss"] + b["rss"])
    return float(np.clip((one["rss"] - best) / max(one["rss"], 1e-9), 0.0, 1.0))

# builds one fixed row for each mrd layer, properties are summarized and then fitted to occipied layer centers
# each layer gets  a risdual in x and y limited by paddle dimnesions 
def build_mrd_layer_features(mrd: Mapping[str, Any], geo: GeometryBundle) -> Tuple[np.ndarray, np.ndarray, Dict[str, Any]]:
    feats = np.zeros((N_MRD_LAYERS, len(MRD_LAYER_FEATURE_NAMES)), np.float32)
    mask = np.zeros(N_MRD_LAYERS, np.uint8)
    cks = np.asarray(mrd["chankeys"], np.int32)
    ts = np.asarray(mrd["times_ns"], np.float32)
    layers = np.asarray(mrd["layers"], np.int32)
    if cks.size == 0:
        return feats, mask, _fit_summary_from_layers(feats, mask, geo)

    start = float(np.min(ts))
    x_half, y_half = geo.mrd_x_half_cm, geo.mrd_y_half_cm
    layer_points: Dict[int, Dict[str, Any]] = {}

    for layer in range(N_MRD_LAYERS):
        slots = np.where(layers == layer)[0]
        z = float(geo.mrd_layer_z_center_cm[layer])
        z_norm = (z - geo.mrd_z_front_cm) / geo.mrd_z_span_cm
        feats[layer, 2] = z_norm
        if slots.size == 0:
            # Axis is deterministic even when the layer is empty: infer from all paddles.
            layer_cks = np.where(geo.chankey_to_layer == layer)[0]
            if layer_cks.size:
                feats[layer, 1] = 1.0 if mrd_measure_axis_is_x(int(layer_cks[0]), geo) else 0.0
            continue

        layer_cks = cks[slots]
        axis_x = mrd_measure_axis_is_x(int(layer_cks[0]), geo)
        coords = geo.chankey_to_pos_global_cm[layer_cks, 0 if axis_x else 1].astype(np.float64)
        layer_ts = ts[slots].astype(np.float64)
        scale = x_half if axis_x else y_half
        live_layer = np.where(geo.chankey_to_layer == layer)[0]
        n_live = sum(int(ck) not in KNOWN_DEAD_MRD_CHANKEYS for ck in live_layer)
        max_gap, two_score = _two_band_score(coords, scale)

        mask[layer] = 1
        feats[layer, 0] = 1.0
        feats[layer, 1] = 1.0 if axis_x else 0.0
        feats[layer, 3] = min(2.0, slots.size / 16.0)
        feats[layer, 4] = min(2.0, slots.size / max(1.0, float(n_live)))
        feats[layer, 5] = np.clip((np.min(layer_ts) - start) / 50.0, -4.0, 4.0)
        feats[layer, 6] = np.clip((np.mean(layer_ts) - start) / 50.0, -4.0, 4.0)
        feats[layer, 7] = np.clip(np.std(layer_ts) / 30.0, 0.0, 4.0)
        feats[layer, 8] = np.clip(np.mean(coords) / scale, -2.0, 2.0)
        feats[layer, 9] = np.clip(np.std(coords) / scale, 0.0, 2.0)
        feats[layer, 10] = np.clip(np.min(coords) / scale, -2.0, 2.0)
        feats[layer, 11] = np.clip(np.max(coords) / scale, -2.0, 2.0)
        feats[layer, 12] = np.clip(np.ptp(coords) / scale, 0.0, 2.0)
        feats[layer, 13] = max_gap
        feats[layer, 14] = two_score
        layer_points[layer] = {
            "z": z,
            "axis_x": axis_x,
            "coord": float(np.mean(coords)),
            "spread": float(np.std(coords)),
            "n": int(slots.size),
        }

    summary = _fit_summary_from_layers(feats, mask, geo, layer_points)
    for layer, point in layer_points.items():
        fit = summary["x_fit"] if point["axis_x"] else summary["y_fit"]
        scale = x_half if point["axis_x"] else y_half
        if fit["valid"]:
            pred = fit["slope"] * point["z"] + fit["intercept"]
            feats[layer, 15] = np.clip((point["coord"] - pred) / scale, -2.0, 2.0)
    return feats, mask, summary

# one measured coordinate projection onto the full layer-z grid.
# leaves points outside the measured z range as NaN so the caller can fill those
# endpoints using the global line fit rather than flat interpolation.
def _interp_or_fit(z_grid: np.ndarray, z: np.ndarray, x: np.ndarray, fit: Mapping[str, float]) -> np.ndarray:
    if z.size >= 2:
        return np.interp(z_grid, z, x, left=np.nan, right=np.nan)
    if fit.get("valid", 0.0):
        return fit["slope"] * z_grid + fit["intercept"]
    return np.zeros_like(z_grid)

# fits the x or y orientation by layer center and reconstrcutiosn bot projections on a common 11 layer grid in z
# will test early and late slope changes kinks and path excesses and attempts a 1 line versus 2 line improvement
# as of now tho it does not fully attempt if an event could be multimuon we would need to salt alot of events for training
# for future bsm could be doable but we need to simulate a decent amount of events
def _fit_summary_from_layers(
    feats: np.ndarray,
    mask: np.ndarray,
    geo: GeometryBundle,
    layer_points: Optional[Mapping[int, Mapping[str, Any]]] = None,
) -> Dict[str, Any]:
    if layer_points is None:
        layer_points = {}
    xz, xx, yz, yy = [], [], [], []
    for layer, point in layer_points.items():
        if point["axis_x"]:
            xz.append(point["z"]); xx.append(point["coord"])
        else:
            yz.append(point["z"]); yy.append(point["coord"])
    xz_a, xx_a = np.asarray(xz, np.float64), np.asarray(xx, np.float64)
    yz_a, yy_a = np.asarray(yz, np.float64), np.asarray(yy, np.float64)
    xfit = _weighted_line_fit(xz_a, xx_a)
    yfit = _weighted_line_fit(yz_a, yy_a)

    z_mid = 0.5 * (geo.mrd_z_front_cm + geo.mrd_z_back_cm)
    ex = _weighted_line_fit(xz_a[xz_a <= z_mid], xx_a[xz_a <= z_mid])
    lx = _weighted_line_fit(xz_a[xz_a >= z_mid], xx_a[xz_a >= z_mid])
    ey = _weighted_line_fit(yz_a[yz_a <= z_mid], yy_a[yz_a <= z_mid])
    ly = _weighted_line_fit(yz_a[yz_a >= z_mid], yy_a[yz_a >= z_mid])

    dx = (lx["slope"] - ex["slope"]) if ex["valid"] and lx["valid"] else 0.0
    dy = (ly["slope"] - ey["slope"]) if ey["valid"] and ly["valid"] else 0.0
    scatter = 0.0
    if ex["valid"] and lx["valid"] and ey["valid"] and ly["valid"]:
        a = np.array([ex["slope"], ey["slope"], 1.0], np.float64)
        b = np.array([lx["slope"], ly["slope"], 1.0], np.float64)
        a /= max(np.linalg.norm(a), 1e-12); b /= max(np.linalg.norm(b), 1e-12)
        scatter = math.acos(float(np.clip(np.dot(a, b), -1.0, 1.0)))

    # Interpolate alternating x/y measurements to a common 11-layer trajectory.
    z_grid = geo.mrd_layer_z_center_cm.astype(np.float64)
    x_ord = np.argsort(xz_a) if xz_a.size else np.zeros(0, np.int64)
    y_ord = np.argsort(yz_a) if yz_a.size else np.zeros(0, np.int64)
    x_path = _interp_or_fit(z_grid, xz_a[x_ord], xx_a[x_ord], xfit)
    y_path = _interp_or_fit(z_grid, yz_a[y_ord], yy_a[y_ord], yfit)
    # Fill endpoint NaNs from the global fit where interpolation cannot extrapolate.
    if xfit["valid"]:
        x_path = np.where(np.isfinite(x_path), x_path, xfit["slope"] * z_grid + xfit["intercept"])
    else:
        x_path = np.nan_to_num(x_path)
    if yfit["valid"]:
        y_path = np.where(np.isfinite(y_path), y_path, yfit["slope"] * z_grid + yfit["intercept"])
    else:
        y_path = np.nan_to_num(y_path)

    pts_all = np.stack([x_path, y_path, z_grid], axis=1)
    # Compute path/kink quantities only across the observed first-to-last layer
    # span.  Extrapolating a stopped track through unobserved downstream layers
    # would manufacture a falsely precise path and dilute the scattering signal.
    observed_layers = sorted(int(v) for v in layer_points.keys())
    if len(observed_layers) >= 2:
        lo, hi = observed_layers[0], observed_layers[-1]
        pts = pts_all[lo:hi + 1]
    else:
        pts = pts_all[:0]
    seg = np.diff(pts, axis=0) if pts.shape[0] >= 2 else np.zeros((0, 3), np.float64)
    seg_norm = np.linalg.norm(seg, axis=1) if seg.size else np.zeros(0, np.float64)
    good_seg = seg_norm > 1e-6
    dirs = np.zeros_like(seg)
    dirs[good_seg] = seg[good_seg] / seg_norm[good_seg, None]
    kinks = []
    for i in range(max(0, dirs.shape[0] - 1)):
        if good_seg[i] and good_seg[i + 1]:
            kinks.append(math.acos(float(np.clip(np.dot(dirs[i], dirs[i + 1]), -1.0, 1.0))))
    path = float(np.sum(seg_norm[good_seg]))
    chord = float(np.linalg.norm(pts[-1] - pts[0])) if pts.shape[0] >= 2 else 0.0
    path_excess = max(0.0, (path - chord) / max(chord, 1e-6)) if chord > 0.0 else 0.0

    residual_growth = 0.0
    residuals, depths = [], []
    for point in layer_points.values():
        fit = xfit if point["axis_x"] else yfit
        if fit["valid"]:
            residuals.append(abs(point["coord"] - (fit["slope"] * point["z"] + fit["intercept"])))
            depths.append((point["z"] - geo.mrd_z_front_cm) / geo.mrd_z_span_cm)
    if len(residuals) >= 3 and np.std(depths) > 1e-8 and np.std(residuals) > 1e-8:
        residual_growth = float(np.corrcoef(depths, residuals)[0, 1])
        if not math.isfinite(residual_growth): residual_growth = 0.0

    return {
        "x_fit": xfit,
        "y_fit": yfit,
        "early_x_fit": ex,
        "late_x_fit": lx,
        "early_y_fit": ey,
        "late_y_fit": ly,
        "delta_x_slope": float(dx),
        "delta_y_slope": float(dy),
        "scatter_angle": float(scatter),
        "max_local_kink": float(max(kinks) if kinks else 0.0),
        "mean_local_kink": float(np.mean(kinks) if kinks else 0.0),
        "path_excess_fraction": float(path_excess),
        "residual_growth": float(np.clip(residual_growth, -1.0, 1.0)),
        "two_line_improvement_x": _two_line_improvement(xz_a, xx_a),
        "two_line_improvement_y": _two_line_improvement(yz_a, yy_a),
        "x_path": x_path.astype(np.float32),
        "y_path": y_path.astype(np.float32),
    }

# summarizes the first 16 tank clusters by timing, span, charge, multiplicy, status, charge concentration and cahrge weights
# also returns event level cluster count and slection fraction, charge balance and normalized charge entropy- see readme 
def build_tank_cluster_features(
    cluster_times: Sequence[float],
    cluster_start_times: Sequence[float],
    cluster_end_times: Sequence[float],
    cluster_spans: Sequence[float],
    cluster_charges: Sequence[float],
    cluster_nhits: Sequence[int],
    all_hit_cluster_ids: Sequence[int],
    all_hit_detkeys: Sequence[int],
    all_hit_charges: Sequence[float],
    selected_index: int,
    earliest_index: int,
    geo: GeometryBundle,
    max_clusters: int = MAX_TANK_CLUSTERS,
) -> Tuple[np.ndarray, np.ndarray, Dict[str, float]]:
    feats = np.zeros((max_clusters, len(TANK_CLUSTER_FEATURE_NAMES)), np.float32)
    mask = np.zeros(max_clusters, np.uint8)
    n = min(len(cluster_times), len(cluster_charges), len(cluster_nhits), max_clusters)
    if n <= 0:
        return feats, mask, {"n_clusters": 0.0, "selected_fraction": 0.0, "second_over_first": 0.0, "entropy": 0.0}

    times = np.asarray(cluster_times[:n], np.float64)
    charges = np.maximum(np.asarray(cluster_charges[:n], np.float64), 0.0)
    nhits = np.asarray(cluster_nhits[:n], np.float64)
    spans = np.asarray(cluster_spans[:n], np.float64) if len(cluster_spans) >= n else np.zeros(n)
    first_t = float(np.min(times))
    total_q = float(np.sum(charges))
    qprob = charges / max(total_q, 1e-12)
    entropy = -float(np.sum(qprob[qprob > 0] * np.log(qprob[qprob > 0]))) / max(math.log(max(2, n)), 1e-12)
    order_q = np.sort(charges)[::-1]
    second_over_first = float(order_q[1] / max(order_q[0], 1e-12)) if n >= 2 else 0.0
    tank_scale = geo.tank_scales_cm

    ids = np.asarray(all_hit_cluster_ids, np.int32)
    det = np.asarray(all_hit_detkeys, np.int32)
    hq = np.maximum(np.asarray(all_hit_charges, np.float64), 0.0)

    for ci in range(n):
        slots = np.where(ids == ci)[0]
        valid_slots = slots[(det[slots] >= 0) & (det[slots] < N_TANK_DETKEYS)] if slots.size else slots
        if valid_slots.size:
            positions = geo.tank_pos_global_cm[det[valid_slots]]
            centroid, _ = _weighted_centroid(positions, hq[valid_slots])
            max_hit = float(np.max(hq[valid_slots]))
            cb = max_hit / max(float(np.sum(hq[valid_slots])), 1e-12)
        else:
            centroid = geo.tank_center_global_cm.copy()
            max_hit = 0.0
            cb = 0.0

        mask[ci] = 1
        feats[ci, 0] = 1.0
        feats[ci, 1] = np.clip((times[ci] - first_t) / 5000.0, -2.0, 8.0)
        feats[ci, 2] = np.clip(finite_or(spans[ci], 0.0) / 200.0, 0.0, 10.0)
        feats[ci, 3] = math.log1p(charges[ci]) / math.log1p(2000.0)
        feats[ci, 4] = min(5.0, nhits[ci] / 200.0)
        feats[ci, 5] = charges[ci] / max(total_q, 1e-12)
        feats[ci, 6] = 1.0 if ci == int(selected_index) else 0.0
        feats[ci, 7] = 1.0 if ci == int(earliest_index) else 0.0
        feats[ci, 8] = max_hit / max(charges[ci], 1e-12)
        feats[ci, 9] = cb
        rel = centroid - geo.tank_center_global_cm
        feats[ci, 10:13] = np.clip(rel / tank_scale, -2.0, 2.0)

    selected_fraction = charges[int(selected_index)] / max(total_q, 1e-12) if 0 <= int(selected_index) < n else 0.0
    return feats, mask, {
        "n_clusters": float(n),
        "selected_fraction": float(selected_fraction),
        "second_over_first": second_over_first,
        "entropy": float(np.clip(entropy, 0.0, 1.0)),
    }

# final feature assembler for the 73 component physics branches
# this is the final ordered input contract shared by all steps of the model ie wcsim extraction,training,runtime inference
def build_physics_features(
    mrd: Mapping[str, Any],
    mrd_layer_features: np.ndarray,
    mrd_layer_mask: np.ndarray,
    mrd_fit: Mapping[str, Any],
    tank: Mapping[str, Any],
    tank_cluster_features: np.ndarray,
    tank_cluster_mask: np.ndarray,
    tank_cluster_summary: Mapping[str, float],
    mrd_selected_time_span_ns: float,
    mrd_unassigned_hits: int,
    tank_selected_charge: float,
    tank_selected_time_span_ns: float,
    geo: GeometryBundle,
) -> np.ndarray:
    out: List[float] = []
    n_mrd = int(mrd["length"])
    valid_layers = np.where(mrd_layer_mask > 0)[0]
    n_layers = int(valid_layers.size)
    first_layer = int(valid_layers[0]) if n_layers else -1
    last_layer = int(valid_layers[-1]) if n_layers else -1
    live_occ = mrd_layer_features[valid_layers, 4] if n_layers else np.zeros(0)
    widths = mrd_layer_features[valid_layers, 12] if n_layers else np.zeros(0)
    twos = mrd_layer_features[valid_layers, 14] if n_layers else np.zeros(0)

    out.extend([
        1.0 if n_mrd > 0 else 0.0,
        n_mrd / float(MAX_MRD_HITS),
        n_layers / float(N_MRD_LAYERS),
        first_layer / 10.0 if first_layer >= 0 else 0.0,
        last_layer / 10.0 if last_layer >= 0 else 0.0,
        (last_layer - first_layer) / 10.0 if n_layers else 0.0,
        np.clip(finite_or(mrd_selected_time_span_ns, 0.0) / 30.0, 0.0, 10.0),
        float(mrd_unassigned_hits) / max(1.0, float(mrd["raw_valid_count"])),
        float(mrd["dedup_removed"]) / max(1.0, float(mrd["raw_valid_count"])),
        float(np.mean(live_occ)) if live_occ.size else 0.0,
        float(np.max(widths)) if widths.size else 0.0,
        float(np.mean(twos)) if twos.size else 0.0,
        float(np.max(twos)) if twos.size else 0.0,
    ])

    xfit, yfit = mrd_fit["x_fit"], mrd_fit["y_fit"]
    ex, lx = mrd_fit["early_x_fit"], mrd_fit["late_x_fit"]
    ey, ly = mrd_fit["early_y_fit"], mrd_fit["late_y_fit"]
    z0, z1 = geo.mrd_z_front_cm, geo.mrd_z_back_cm
    x_scale, y_scale = geo.mrd_x_half_cm, geo.mrd_y_half_cm
    x_entry = (xfit["slope"] * z0 + xfit["intercept"]) if xfit["valid"] else 0.0
    x_exit = (xfit["slope"] * z1 + xfit["intercept"]) if xfit["valid"] else 0.0
    y_entry = (yfit["slope"] * z0 + yfit["intercept"]) if yfit["valid"] else 0.0
    y_exit = (yfit["slope"] * z1 + yfit["intercept"]) if yfit["valid"] else 0.0
    out.extend([
        xfit["valid"], np.clip(xfit["slope"], -2.0, 2.0), np.clip(xfit["intercept"] / x_scale, -10.0, 10.0), np.clip(xfit["rms"] / x_scale, 0.0, 4.0),
        yfit["valid"], np.clip(yfit["slope"], -2.0, 2.0), np.clip(yfit["intercept"] / y_scale, -10.0, 10.0), np.clip(yfit["rms"] / y_scale, 0.0, 4.0),
        ex["valid"], lx["valid"], np.clip(ex["slope"], -2.0, 2.0), np.clip(lx["slope"], -2.0, 2.0), np.clip(mrd_fit["delta_x_slope"], -4.0, 4.0),
        ey["valid"], ly["valid"], np.clip(ey["slope"], -2.0, 2.0), np.clip(ly["slope"], -2.0, 2.0), np.clip(mrd_fit["delta_y_slope"], -4.0, 4.0),
        np.clip(mrd_fit["scatter_angle"], 0.0, math.pi),
        np.clip(mrd_fit["max_local_kink"], 0.0, math.pi),
        np.clip(mrd_fit["mean_local_kink"], 0.0, math.pi),
        np.clip(mrd_fit["path_excess_fraction"], 0.0, 4.0),
        mrd_fit["residual_growth"],
        mrd_fit["two_line_improvement_x"],
        mrd_fit["two_line_improvement_y"],
        np.clip(x_entry / x_scale, -2.0, 2.0),
        np.clip(y_entry / y_scale, -2.0, 2.0),
        np.clip(x_exit / x_scale, -2.0, 2.0),
        np.clip(y_exit / y_scale, -2.0, 2.0),
    ])

    # Julie Back's thesis, Sec. 6.3.1, estimates effective iron traversed
    # from the number of crossed scintillator layers, 5 cm iron per layer,
    # divided by cos(track angle), with an added half iron slab for the
    # uncertain stopping point.  We encode the same detector-side concept as
    # a proxy, not as a truth correction: current schema-v4 ROOT files do not
    # contain the Geant4 stepwise path length required for a direct
    # straight/actual-path training target.
    julie_valid = bool(n_layers >= 2 and xfit["valid"] and yfit["valid"] and last_layer > first_layer)
    if julie_valid:
        sx, sy = float(xfit["slope"]), float(yfit["slope"])
        detector_cos_theta = 1.0 / math.sqrt(1.0 + sx * sx + sy * sy)
        z_first = float(geo.mrd_layer_z_center_cm[first_layer])
        z_last = float(geo.mrd_layer_z_center_cm[last_layer])
        p_first = np.asarray([
            sx * z_first + float(xfit["intercept"]),
            sy * z_first + float(yfit["intercept"]),
            z_first,
        ], dtype=np.float64)
        p_last = np.asarray([
            sx * z_last + float(xfit["intercept"]),
            sy * z_last + float(yfit["intercept"]),
            z_last,
        ], dtype=np.float64)
        straight_chord_cm = float(np.linalg.norm(p_last - p_first))
        crossed_layer_span = int(last_layer - first_layer + 1)
        effective_iron_cm = (5.0 * crossed_layer_span + 2.5) / max(detector_cos_theta, 0.20)
        effective_to_chord = effective_iron_cm / max(straight_chord_cm, 1.0)
    else:
        detector_cos_theta = 0.0
        straight_chord_cm = 0.0
        effective_iron_cm = 0.0
        effective_to_chord = 0.0
    out.extend([
        1.0 if julie_valid else 0.0,
        np.clip(detector_cos_theta, 0.0, 1.0),
        np.clip(straight_chord_cm / 200.0, 0.0, 4.0),
        np.clip(effective_iron_cm / 200.0, 0.0, 4.0),
        np.clip(effective_to_chord, 0.0, 4.0),
    ])

    n_tank = int(tank["length"])
    det = np.asarray(tank["detkeys"], np.int32)
    ts = np.asarray(tank["times_ns"], np.float32)
    qs = np.asarray(tank["charges"], np.float32)
    tank_scale = geo.tank_scales_cm
    if n_tank:
        pos = geo.tank_pos_global_cm[det]
        full_c, full_rms = _weighted_centroid(pos, qs)
        scalar_prompt_idx = prompt_indices(ts, PROMPT_SCALAR_FRAC, PROMPT_SCALAR_MIN_HITS, PROMPT_SCALAR_MIN_HITS)
        if scalar_prompt_idx.size:
            prompt_c, prompt_rms = _weighted_centroid(pos[scalar_prompt_idx], qs[scalar_prompt_idx])
            prompt_time_rms = float(np.std(ts[scalar_prompt_idx]))
            prompt_valid = 1.0
        else:
            prompt_c = geo.tank_center_global_cm.copy()
            prompt_rms = 0.0
            prompt_time_rms = 0.0
            prompt_valid = 0.0
        max_hit_frac = float(np.max(qs) / max(float(np.sum(qs)), 1e-12))
        prompt_lstm_frac = float(np.sum(tank["prompt_mask"][:n_tank])) / max(1.0, float(n_tank))
    else:
        full_c = prompt_c = geo.tank_center_global_cm.copy()
        full_rms = prompt_rms = prompt_time_rms = max_hit_frac = prompt_lstm_frac = prompt_valid = 0.0

    rel_full = (full_c - geo.tank_center_global_cm) / tank_scale
    rel_prompt = (prompt_c - geo.tank_center_global_cm) / tank_scale
    prompt_phi = math.atan2(float(rel_prompt[2]), float(rel_prompt[0])) if prompt_valid else 0.0
    mrd_phi = math.atan2(yfit["slope"], xfit["slope"]) if xfit["valid"] and yfit["valid"] else 0.0
    delta_phi = prompt_phi - mrd_phi
    while delta_phi > math.pi: delta_phi -= 2.0 * math.pi
    while delta_phi < -math.pi: delta_phi += 2.0 * math.pi

    n_clusters = int(np.sum(tank_cluster_mask > 0))
    one_line_quality = 1.0 / (1.0 + xfit["rms"] / 10.0 + yfit["rms"] / 10.0 + float(np.max(twos) if twos.size else 0.0))
    ambiguity = max(
        float(np.max(twos) if twos.size else 0.0),
        float(mrd_fit["two_line_improvement_x"]),
        float(mrd_fit["two_line_improvement_y"]),
        min(1.0, max(0.0, tank_cluster_summary.get("second_over_first", 0.0))),
    )

    out.extend([
        1.0 if n_tank > 0 else 0.0,
        n_tank / float(MAX_TANK_HITS),
        math.log1p(max(0.0, finite_or(tank_selected_charge, 0.0))) / math.log1p(2000.0),
        np.clip(finite_or(tank_selected_time_span_ns, 0.0) / 200.0, 0.0, 10.0),
        n_clusters / float(MAX_TANK_CLUSTERS),
        tank_cluster_summary.get("selected_fraction", 0.0),
        tank_cluster_summary.get("second_over_first", 0.0),
        tank_cluster_summary.get("entropy", 0.0),
        max_hit_frac,
        prompt_lstm_frac,
        prompt_valid,
        *np.clip(rel_full, -2.0, 2.0).tolist(),
        np.clip(full_rms / max(float(np.mean(tank_scale)), 1.0), 0.0, 4.0),
        *np.clip(rel_prompt, -2.0, 2.0).tolist(),
        np.clip(prompt_rms / max(float(np.mean(tank_scale)), 1.0), 0.0, 4.0),
        np.clip(prompt_time_rms / 50.0, 0.0, 10.0),
        math.sin(prompt_phi),
        math.cos(prompt_phi),
        math.sin(delta_phi),
        math.cos(delta_phi),
        np.clip(one_line_quality, 0.0, 1.0),
        np.clip(ambiguity, 0.0, 1.0),
    ])

    arr = np.asarray(out, np.float32)
    if arr.size != len(PHYSICS_FEATURE_NAMES):
        raise RuntimeError("Physics feature schema mismatch: built %d, expected %d" % (arr.size, len(PHYSICS_FEATURE_NAMES)))
    arr[~np.isfinite(arr)] = 0.0
    return arr

# Calculate the expected Cherenkov angle in water from total particle energy.
# Return the -999 sentinel when the energy is unphysical, the particle is below
# Cherenkov threshold, or no real angle exists.
def cherenkov_angle_from_total_energy(total_energy_mev: float, mass_mev: float = 105.6583755, n_water: float = 1.333) -> float:
    e = finite_or(total_energy_mev, -999.0)
    if e <= mass_mev:
        return -999.0
    p2 = e * e - mass_mev * mass_mev
    if p2 <= 0.0:
        return -999.0
    beta = math.sqrt(p2) / e
    c = 1.0 / (n_water * beta)
    if c < -1.0 or c > 1.0:
        return -999.0
    return float(math.acos(c))

# Test whether a valid xyz point lies inside an axis checked detector box,
# allowing a small tolerance around each boundary. Invalid -999 style points
# are always treated as outside.
def point_inside_box(point: np.ndarray, box: np.ndarray, tolerance_cm: float = 1.0) -> bool:
    if point.shape != (3,) or np.any(point <= -900.0):
        return False
    return bool(
        box[0] - tolerance_cm <= point[0] <= box[1] + tolerance_cm and
        box[2] - tolerance_cm <= point[1] <= box[3] + tolerance_cm and
        box[4] - tolerance_cm <= point[2] <= box[5] + tolerance_cm
    )
