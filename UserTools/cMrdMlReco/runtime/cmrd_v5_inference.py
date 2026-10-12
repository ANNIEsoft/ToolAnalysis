#!/usr/bin/env python3
"""Persistent gate + reconstruction inference daemon for cMrdMlReco.

Production deployment release ``cMrdMlReco-B2v042`` of the frozen ANNIE cMRD v5
gate, reconstruction, packet, and feature contracts, driven by the cMrdMlReco
ToolAnalysis adapter (output contract v6).  "v5" names the frozen model and
feature interface; it is unrelated to the adapter's v6 output contract.
It accepts one detector-only event packet per line on stdin, constructs the
frozen schema-v5 tensors through :mod:`cmrd_v5_runtime_features`, runs the gate
SavedModel for every structurally valid packet, and conditionally runs the
reconstruction SavedModel.

Production invariants
---------------------
* Load only the final deployment SavedModels with ``tf.saved_model.load``.
* Use only the named ``serving_default`` signatures.
* Pass raw HDF5-style 73-component physics features.  The deployment graphs
  contain their own ``FixedStandardize`` layer.
* Treat the frozen geometry and both model metadata files as one release.
* Do not import the trainer, rebuild a Keras model, load weights, use positional
  output ordering, or denormalize reconstruction outputs in C++/Python.
* Keep stdout protocol-only.  All diagnostics are written to stderr.

Protocol
--------
Startup success writes exactly one line before event processing::

    READY {compact release JSON}

Per event, stdin receives one compact JSON object and stdout returns one compact
JSON object.  ``QUIT`` on its own line ends the daemon without a response.
"""

# ---------------------------------------------------------------------------
# Ownership boundary (kept explicit from the alpha commissioning release)
# ---------------------------------------------------------------------------
# The daemon owns every operation that is part of the learned v5 model
# contract: MRD/Tank selection, the frozen dead mask, geometry lookup, feature
# construction, tensor validation, gate routing, and interpretation of named
# SavedModel outputs.  The C++ adapter deliberately does not reproduce those
# steps.  ToolAnalysis instead owns detector-product collection, data charge
# to PE conversion, process supervision, and publication to ANNIE Stores.
#
# No truth, GENIE, particle identity, SimpleReco, MuonFitter, or FindMrdTracks
# fit is permitted to enter the model tensors.  That detector-only boundary is
# part of the physics validity of the release, not just a software preference.
# ---------------------------------------------------------------------------

from __future__ import annotations

import argparse
import hashlib
import json
import math
import os
import sys
import traceback
from dataclasses import dataclass
from pathlib import Path
from typing import Any, Dict, Iterable, Mapping, MutableMapping, Optional, Sequence, Tuple

# These environment values must be set before TensorFlow is imported in main().
if os.environ.get("CMRD_V5_ALLOW_GPU", "0") != "1":
    os.environ["CUDA_VISIBLE_DEVICES"] = ""
os.environ.setdefault("TF_CPP_MIN_LOG_LEVEL", "3")
os.environ.setdefault("TF_FORCE_GPU_ALLOW_GROWTH", "false")

import numpy as np

from cmrd_v5_features import PHYSICS_FEATURE_NAMES
from cmrd_v5_runtime_features import (
    EXPECTED_INPUT_SHAPES,
    FeatureValidationError,
    PacketValidationError,
    RUNTIME_FEATURE_VERSION,
    RuntimeFeatureBuilder,
    RuntimeFeatureError,
)

DAEMON_VERSION = "cmrd_v5_inference_1.0.0"
PROTOCOL_VERSION = 1
EXPECTED_TF_PREFIX = "2.10."

# ---------------------------------------------------------------------------
# Frozen MRD-only Tank-bypass policy (production; validated on WCSim and data).
#
# When the selected Tank input is empty, the reconstruction SavedModel is still
# run if, and only if, ALL of the following hold:
#   * HasTarget probability >= 0.90  (instead of the normal gate threshold),
#   * >= 4 live unique MRD hits in the selected MRD cluster,
#   * >= 4 physical MRD layers,
#   * a retained MRD TimeCluster was selected (raw-TDC fallback never qualifies),
#   * the category-zero policy, when configured, is satisfied.
# The Reconstructable head is deliberately not consulted on this route.
# Events reconstructed this way are flagged RecoUsedTankBypass=true by the C++
# adapter.  Changing these values changes physics output: treat them as part of
# the frozen release.
# ---------------------------------------------------------------------------
TANK_BYPASS_MIN_HAS_TARGET = 0.90
TANK_BYPASS_MIN_MRD_LIVE_UNIQUE_HITS = 4
TANK_BYPASS_MIN_MRD_LAYERS = 4
TANK_BYPASS_REQUIRE_RETAINED_MRD_TIME_CLUSTER = True

EXPECTED_INPUT_DTYPES: Mapping[str, str] = {
    name: ("int32" if name in {"mrd_hit_idx", "tank_hit_idx"} else "float32")
    for name in EXPECTED_INPUT_SHAPES
}

EXPECTED_GATE_OUTPUT_SHAPES: Mapping[str, Tuple[int, ...]] = {
    "out_event_category": (7,),
    "out_has_target": (1,),
    "out_reconstructable": (1,),
    "out_single_compatibility": (1,),
}

EXPECTED_RECO_OUTPUT_SHAPES: Mapping[str, Tuple[int, ...]] = {
    "out_back_xyz_mean_sigma_physical": (6,),
    "out_cherenkov_mean_sigma_physical": (2,),
    "out_cos_phi": (1,),
    "out_cos_theta_mean_sigma_physical": (2,),
    "out_energy_loss_mean_sigma_physical": (2,),
    "out_exit_face": (7,),
    "out_length_mean_sigma_physical": (2,),
    "out_mrd_entry_xyz_mean_sigma_physical": (6,),
    "out_mrd_exit_xyz_mean_sigma_physical": (6,),
    "out_mrd_scatter_mean_sigma_physical": (6,),
    "out_muon_ke_mean_sigma_physical": (2,),
    "out_penetration_mean_sigma_physical": (2,),
    "out_side_xyz_mean_sigma_physical": (6,),
    "out_sin_phi": (1,),
    "out_stop_xyz_mean_sigma_physical": (6,),
    "out_stopped": (1,),
    "out_tank_exit_xyz_mean_sigma_physical": (6,),
    "out_tank_scatter_mean_sigma_physical": (6,),
    "out_topology": (3,),
    "out_vertex_xyz_mean_sigma_physical": (6,),
}
# Output shapes are part of the frozen SavedModel interface and are checked
# before any output is interpreted. A mismatch is a release-contract failure.
# All four gate outputs must remain within [0,1].
GATE_PROBABILITY_OUTPUTS = frozenset(EXPECTED_GATE_OUTPUT_SHAPES)
RECO_PROBABILITY_OUTPUTS = frozenset({"out_exit_face", "out_stopped", "out_topology"})
RECO_TANH_OUTPUTS = frozenset({"out_sin_phi", "out_cos_phi"})
RECO_GAUSSIAN_OUTPUTS = frozenset(
    name for name in EXPECTED_RECO_OUTPUT_SHAPES if name.endswith("_mean_sigma_physical")
)
RECO_SOFTMAX_OUTPUTS = frozenset({"out_exit_face", "out_topology"})

# Failure codes frozen by the cMrdMlReco interface/store contract.
FAILURE_DAEMON_PROTOCOL = 12  # C++/Python request-response contract failure.
FAILURE_FEATURE_BUILD = 23  # Packet validation or feature construction failed.
FAILURE_TENSOR_VALIDATION = 24  # Built tensors violated the frozen input contract.
FAILURE_GATE_INFERENCE = 30  # TensorFlow failed while executing the gate.
FAILURE_GATE_OUTPUT = 31  # Gate executed but returned malformed output.
FAILURE_RECO_INFERENCE = 50  # TensorFlow failed while executing reconstruction.
FAILURE_RECO_OUTPUT = 51  # Reconstruction executed but returned malformed output.


class StartupError(RuntimeError):
    """Fatal release, metadata, signature, or warm-up error."""


class EventInferenceError(RuntimeError):
    """Per-event failure with a frozen ToolAnalysis failure code."""

    def __init__(
        self,
        failure_code: int,
        status: str,
        message: str,
        diagnostics: Optional[Mapping[str, Any]] = None,
        gate: Optional[Mapping[str, Any]] = None,
    ) -> None:
        super().__init__(message)
        self.failure_code = int(failure_code)
        self.status = str(status)
        self.diagnostics = dict(diagnostics or {})
        self.gate = dict(gate) if gate is not None else None


@dataclass(frozen=True)  # Gate policy is immutable after runtime construction.
class GateThresholds:
    has_target: float = 0.5
    reconstructable: float = 0.0
    require_category_zero: bool = False

    def validate(self) -> None:
        for label, value in (
            ("GateHasTargetThreshold", self.has_target),
            ("GateReconstructableThreshold", self.reconstructable),
        ):
            if not math.isfinite(float(value)) or not (0.0 <= float(value) <= 1.0):
                raise StartupError(f"{label} must be finite and in [0,1], got {value!r}")


@dataclass(frozen=True)  # Release identity is immutable after startup.
class ModelReleaseIdentity:
    model_release: str
    gate_model_name: str
    reconstruction_model_name: str
    gate_model_tree_sha256: str
    reconstruction_model_tree_sha256: str
    geometry_version: str
    geometry_manifest_sha256: str
    gate_trainer_version: str
    reconstruction_trainer_version: str
    tensorflow_version: str

    def as_dict(self, thresholds: GateThresholds) -> Dict[str, Any]:
        return {
            "protocol_version": PROTOCOL_VERSION,
            "daemon_version": DAEMON_VERSION,
            "runtime_feature_version": RUNTIME_FEATURE_VERSION,
            "model_release": self.model_release,
            "gate_model_name": self.gate_model_name,
            "reconstruction_model_name": self.reconstruction_model_name,
            "gate_model_tree_sha256": self.gate_model_tree_sha256,
            "reconstruction_model_tree_sha256": self.reconstruction_model_tree_sha256,
            "geometry_version": self.geometry_version,
            "geometry_manifest_sha256": self.geometry_manifest_sha256,
            "gate_trainer_version": self.gate_trainer_version,
            "reconstruction_trainer_version": self.reconstruction_trainer_version,
            "tensorflow_version": self.tensorflow_version,
            "gate_has_target_threshold": float(thresholds.has_target),
            "gate_reconstructable_threshold": float(thresholds.reconstructable),
            "require_gate_category_zero": bool(thresholds.require_category_zero),
        }


def _stderr(message: str) -> None:
    print(f"[cmrd-v5-daemon] {message}", file=sys.stderr, flush=True)


def _load_json(path: str, label: str) -> Dict[str, Any]:
    p = Path(path).expanduser().resolve()
    if not p.is_file():
        raise StartupError(f"{label} does not exist: {p}")
    try:
        with p.open() as handle:
            obj = json.load(handle)
    except Exception as exc:
        raise StartupError(f"could not parse {label} {p}: {exc}") from exc
    if not isinstance(obj, dict):
        raise StartupError(f"{label} must contain a JSON object: {p}")
    return obj
# Normalize file/path/JSON failures into one startup-error boundary.
# SavedModel identity includes relative paths as well as file contents, so the
# same bytes under different filenames intentionally produce a different digest.
def _sha256_tree(path: str) -> str:
    """Hash all regular files using relative paths and contents in sorted order."""
    root = Path(path).expanduser().resolve()
    if not root.is_dir():
        raise StartupError(f"SavedModel directory does not exist: {root}")
    files = sorted(p for p in root.rglob("*") if p.is_file())
    if not files:
        raise StartupError(f"SavedModel directory contains no files: {root}")
    digest = hashlib.sha256()
    for item in files:
        rel = item.relative_to(root).as_posix().encode("utf-8")
        digest.update(rel)
        digest.update(b"\0")
        with item.open("rb") as handle:
            for block in iter(lambda: handle.read(1024 * 1024), b""):
                digest.update(block)
        digest.update(b"\0")
    return digest.hexdigest()

# Normalize configured hashes before comparison; an empty expected value skips
# that individual check. Production configuration supplies all expected hashes.
def _validate_expected_hash(actual: str, expected: str, label: str) -> None:
    expected = str(expected or "").strip().lower()
    if expected and actual.lower() != expected:
        raise StartupError(f"{label} SHA-256 mismatch: expected {expected}, got {actual}")

# TensorFlow dtypes can appear in several string-like representations.
def _dtype_name(dtype: Any) -> str:
    name = getattr(dtype, "name", None)
    if name:
        return str(name)
    text = str(dtype)
    return text.replace("<dtype: '", "").replace("'>", "")

# Normalize TensorFlow shapes into ordinary Python tuples for strict checks.
def _shape_tuple(spec_or_tensor: Any) -> Tuple[Optional[int], ...]:
    shape = getattr(spec_or_tensor, "shape", None)
    if shape is None:
        raise StartupError(f"object has no shape: {spec_or_tensor!r}")
    try:
        values = shape.as_list()
    except AttributeError:
        values = list(shape)
    out = []
    for value in values:
        out.append(None if value is None else int(value))
    return tuple(out)
# TensorShape normally provides as_list(); list(shape) supports simple iterable
# shape objects. Other conversion errors remain startup failures rather than
# being silently coerced.
#

def _signature_inputs(signature: Any) -> Mapping[str, Any]:
    structured = getattr(signature, "structured_input_signature", None)
    if structured is None or len(structured) != 2:
        raise StartupError("SavedModel serving_default has no structured_input_signature")
    positional, keyword = structured
    if positional not in ((), [], None):
        raise StartupError("serving_default must use named keyword inputs only")
    if not isinstance(keyword, Mapping):
        raise StartupError("serving_default keyword input signature is not a mapping")
    return keyword
# Signatures must use named inputs so dictionary construction order cannot
# silently connect a tensor to the wrong model branch.

def _signature_outputs(signature: Any) -> Mapping[str, Any]:
    outputs = getattr(signature, "structured_outputs", None)
    if not isinstance(outputs, Mapping):
        raise StartupError("SavedModel serving_default structured_outputs is not a mapping")
    return outputs
# Named outputs prevent export ordering from changing the physical interpretation.
# Name sets are compared because all SavedModel inputs and outputs are keyed.
def validate_signature_contract(
    signature: Any,
    expected_outputs: Mapping[str, Tuple[int, ...]],
    label: str,
) -> None:
    inputs = _signature_inputs(signature)
    if set(inputs) != set(EXPECTED_INPUT_SHAPES):
        raise StartupError(
            f"{label} input-name mismatch: expected={sorted(EXPECTED_INPUT_SHAPES)}, "
            f"got={sorted(inputs)}"
        )
    for name, tail in EXPECTED_INPUT_SHAPES.items():
        got_shape = _shape_tuple(inputs[name])
        expected_shape = (None,) + tuple(tail)
        if got_shape != expected_shape:
            raise StartupError(
                f"{label} input {name} shape mismatch: expected {expected_shape}, got {got_shape}"
            )
        got_dtype = _dtype_name(getattr(inputs[name], "dtype", None))
        if got_dtype != EXPECTED_INPUT_DTYPES[name]:
            raise StartupError(
                f"{label} input {name} dtype mismatch: expected "
                f"{EXPECTED_INPUT_DTYPES[name]}, got {got_dtype}"
            )

    outputs = _signature_outputs(signature)
    if set(outputs) != set(expected_outputs):
        raise StartupError(
            f"{label} output-name mismatch: expected={sorted(expected_outputs)}, "
            f"got={sorted(outputs)}"
        )
    for name, tail in expected_outputs.items():
        got_shape = _shape_tuple(outputs[name])
        expected_shape = (None,) + tuple(tail)
        if got_shape != expected_shape:
            raise StartupError(
                f"{label} output {name} shape mismatch: expected {expected_shape}, got {got_shape}"
            )
        got_dtype = _dtype_name(getattr(outputs[name], "dtype", None))
        if got_dtype != "float32":
            raise StartupError(
                f"{label} output {name} dtype mismatch: expected float32, got {got_dtype}"
            )

# Metadata validation checks release compatibility only; it does not build
# features or execute either model.
def _validate_metadata(
    gate_meta: Mapping[str, Any],
    reco_meta: Mapping[str, Any],
    geometry_hash: str,
) -> None:
    if gate_meta.get("mode") != "gate":
        raise StartupError(f"gate metadata mode is {gate_meta.get('mode')!r}, expected 'gate'")
    if reco_meta.get("mode") != "reconstruction":
        raise StartupError(
            f"reconstruction metadata mode is {reco_meta.get('mode')!r}, expected 'reconstruction'"
        )  # Prevent swapped metadata files or metadata from another export mode.
    gate_hash = str(gate_meta.get("geometry_manifest_sha256", ""))
    reco_hash = str(reco_meta.get("geometry_manifest_sha256", ""))
    if not gate_hash or not reco_hash:
        raise StartupError("both model metadata files must record geometry_manifest_sha256")
    if gate_hash != reco_hash:
        raise StartupError(
            f"gate/reconstruction geometry hashes differ: gate={gate_hash}, reconstruction={reco_hash}"
        )
    if gate_hash != geometry_hash:
        raise StartupError(
            f"model/geometry hash mismatch: metadata={gate_hash}, frozen geometry={geometry_hash}"
        )  # Geometry identity is part of the frozen model contract.
# Even small coordinate-convention changes can alter topology and endpoint heads.
    expected_features = list(PHYSICS_FEATURE_NAMES)
    for label, meta in (("gate", gate_meta), ("reconstruction", reco_meta)):
        normalization = meta.get("normalization", {})
        if not isinstance(normalization, Mapping):
            raise StartupError(f"{label} metadata normalization is missing or invalid")
        names = list(normalization.get("physics_feature_names", []))  # Feature order is frozen.
        if names != expected_features:
            raise StartupError(
                f"{label} metadata physics_feature_names do not match the runtime feature order"
            )
        physics = normalization.get("physics", {})
        if not isinstance(physics, Mapping):
            raise StartupError(f"{label} metadata physics normalization is missing")
        mean = np.asarray(physics.get("mean", []), dtype=np.float64)
        std = np.asarray(physics.get("std", []), dtype=np.float64)
        if mean.shape != (len(expected_features),) or std.shape != (len(expected_features),):
            raise StartupError(
                f"{label} physics normalization shape mismatch: mean={mean.shape}, std={std.shape}"
            )
        if not np.all(np.isfinite(mean)) or not np.all(np.isfinite(std)) or np.any(std <= 0):
            raise StartupError(f"{label} physics normalization contains invalid values")
        recorded_tf = str(meta.get("tensorflow_version", ""))  # Release is pinned to TensorFlow 2.10.x.
        if recorded_tf and not recorded_tf.startswith(EXPECTED_TF_PREFIX):
            raise StartupError(
                f"{label} metadata was produced with TensorFlow {recorded_tf!r}, "
                f"expected {EXPECTED_TF_PREFIX}x"
            )

    expected_gate_raw = set(EXPECTED_GATE_OUTPUT_SHAPES)  # Guard against export drift.
    gate_recorded = set(gate_meta.get("model_output_names", []))
    if gate_recorded != expected_gate_raw:
        raise StartupError(
            f"gate metadata model_output_names mismatch: expected={sorted(expected_gate_raw)}, "
            f"got={sorted(gate_recorded)}"
        )

    expected_reco_raw = {
        (name[:-len("_mean_sigma_physical")] if name.endswith("_mean_sigma_physical") else name)
        for name in EXPECTED_RECO_OUTPUT_SHAPES
    }
    reco_recorded = set(reco_meta.get("model_output_names", []))
    if reco_recorded != expected_reco_raw:
        raise StartupError(
            f"reconstruction metadata model_output_names mismatch: "
            f"expected={sorted(expected_reco_raw)}, got={sorted(reco_recorded)}"
        )

    reco_targets = reco_meta.get("normalization", {}).get("targets", {})
    if not isinstance(reco_targets, Mapping):
        raise StartupError("reconstruction target normalization is missing or invalid")
    for deployed_name, tail in EXPECTED_RECO_OUTPUT_SHAPES.items():
        if not deployed_name.endswith("_mean_sigma_physical"):
            continue
        raw_name = deployed_name[:-len("_mean_sigma_physical")]
        dimension = int(tail[0] // 2)
        record = reco_targets.get(raw_name)
        if not isinstance(record, Mapping):
            raise StartupError(f"missing reconstruction normalization for {raw_name}")
        mean = np.asarray(record.get("mean", []), dtype=np.float64)
        std = np.asarray(record.get("std", []), dtype=np.float64)
        if mean.shape != (dimension,) or std.shape != (dimension,):
            raise StartupError(
                f"{raw_name} target normalization shape mismatch: mean={mean.shape}, std={std.shape}"
            )
        if not np.all(np.isfinite(mean)) or not np.all(np.isfinite(std)) or np.any(std <= 0):
            raise StartupError(f"{raw_name} target normalization contains invalid values")
# Target normalization is not applied here; these records verify that the
# deployment wrapper carries the expected training-time provenance.

def _as_numpy_dict(raw: Mapping[str, Any]) -> Dict[str, np.ndarray]:
    out: Dict[str, np.ndarray] = {}
    for name, value in raw.items():
        if hasattr(value, "numpy"):
            value = value.numpy()
        out[str(name)] = np.asarray(value)
    return out


def _range_checked(arr: np.ndarray, low: float, high: float, label: str) -> np.ndarray:
    tolerance = 2.0e-5  # Absorb only tiny floating-point excursions.
    if np.any(arr < low - tolerance) or np.any(arr > high + tolerance):
        amin = float(np.min(arr)); amax = float(np.max(arr))
        raise ValueError(f"{label} outside [{low},{high}]: min={amin}, max={amax}")
    return np.clip(arr, low, high)


def validate_model_outputs(
    raw: Mapping[str, Any],
    expected_shapes: Mapping[str, Tuple[int, ...]],
    model_kind: str,
) -> Dict[str, np.ndarray]:
    values = _as_numpy_dict(raw)
    if set(values) != set(expected_shapes):
        raise ValueError(
            f"{model_kind} output-key mismatch: expected={sorted(expected_shapes)}, "
            f"got={sorted(values)}"
        )
    out: Dict[str, np.ndarray] = {}
    for name, tail in expected_shapes.items():
        arr = np.asarray(values[name], dtype=np.float32)
        expected = (1,) + tuple(tail)
        if arr.shape != expected:
            raise ValueError(
                f"{model_kind} output {name} shape mismatch: expected {expected}, got {arr.shape}"
            )
        if not np.all(np.isfinite(arr)):
            bad = np.argwhere(~np.isfinite(arr))[:5].tolist()  # Report the first failing elements.
            raise ValueError(f"{model_kind} output {name} contains non-finite values at {bad}")
        out[name] = arr

    if model_kind == "gate":
        for name in GATE_PROBABILITY_OUTPUTS:
            out[name] = _range_checked(out[name], 0.0, 1.0, name)
        total = float(np.sum(out["out_event_category"][0]))  # Seven-class softmax.
        if abs(total - 1.0) > 2.0e-3:  # Scalar gate heads are independent probabilities.
            raise ValueError(f"out_event_category probabilities sum to {total}, expected 1")
    elif model_kind == "reconstruction":
        for name in RECO_PROBABILITY_OUTPUTS:
            out[name] = _range_checked(out[name], 0.0, 1.0, name)  # Probability outputs.
        for name in RECO_TANH_OUTPUTS:
            out[name] = _range_checked(out[name], -1.0, 1.0, name)  # sin/cos heads.
        for name in RECO_SOFTMAX_OUTPUTS:
            total = float(np.sum(out[name][0]))
            if abs(total - 1.0) > 2.0e-3:
                raise ValueError(f"{name} probabilities sum to {total}, expected 1")
        for name in RECO_GAUSSIAN_OUTPUTS:
            vector = out[name][0]
            half = vector.size // 2
            if vector.size % 2 != 0 or half == 0:
                raise ValueError(f"{name} has invalid mean/sigma width {vector.size}")
            if np.any(vector[half:] < 0.0):
                raise ValueError(f"{name} contains a negative physical sigma")
    else:
        raise ValueError(f"unknown model_kind={model_kind!r}")
    return out
# Bounded outputs within 2e-5 of their legal range are clipped to
# absorb harmless floating-point overshoot; larger violations fail the event.
# Convert a probability vector to class, confidence, margin, and raw probabilities.
def _classification(probabilities: Sequence[float]) -> Tuple[int, float, float, list]:
    probs = np.asarray(probabilities, dtype=np.float64).reshape(-1)
    if probs.size == 0:
        raise ValueError("empty probability vector")  # Defensive if this helper is reused.
    order = np.argsort(probs)
    best = int(order[-1])
    confidence = float(probs[best])
    second = float(probs[order[-2]]) if probs.size > 1 else 0.0
    return best, confidence, confidence - second, [float(v) for v in probs]
# Exact ties yield zero margin; either tied class has the same confidence.
def _mean_sigma(values: np.ndarray, dimension: int, label: str) -> Tuple[list, list]:
    vector = np.asarray(values, dtype=np.float64).reshape(-1)
    if vector.size != 2 * dimension:
        raise ValueError(f"{label} expected width {2 * dimension}, got {vector.size}")
    mean = [float(v) for v in vector[:dimension]]
    sigma = [float(v) for v in vector[dimension:]]
    return mean, sigma
# Width is checked again here so interpretation remains safe if the helper is reused.
# The model-output validator normally catches this earlier.

def _scalar_mean_sigma(values: np.ndarray, label: str) -> Tuple[float, float]:
    mean, sigma = _mean_sigma(values, 1, label)
    return mean[0], sigma[0]
# Convenience wrapper for one-dimensional Gaussian heads.

def _empty_packet() -> Dict[str, Any]:
    return {
        "schema": "annie_cmrd_v5_event_packet",
        "schema_version": 1,
        "event": {
            "run_number": 0,
            "subrun_number": 0,
            "event_number": -1,
            "mc_event_num": 0,
            "mc_trigger_num": -1,
            "is_data": False,
        },
        "mrd": {
            "source": "empty",
            "num_time_clusters_store": 0,
            "digit_chankeys": [],
            "digit_times_ns": [],
            "digit_charges": [],
            "time_clusters": [],
        },
        "tank": {"source": "empty", "clusters": []},
        "runtime_conditions": {"additional_dead_mrd_chankeys": []},
    }  # Valid detector-empty packet used for warm-up.

# Persistent runtime: validates one release and keeps both SavedModels warm.
# Model, metadata, geometry, and thresholds are bound during startup.
class CmrdV5InferenceRuntime:
    """Persistent, validated gate/reconstruction runtime."""

    def __init__(
        self,
        feature_builder: RuntimeFeatureBuilder,
        tf_module: Any,
        gate_signature: Any,
        reconstruction_signature: Any,
        gate_meta: Mapping[str, Any],
        reconstruction_meta: Mapping[str, Any],
        identity: ModelReleaseIdentity,
        thresholds: GateThresholds,
        warm_up: bool = True,
    ) -> None:
        self.feature_builder = feature_builder
        self.tf = tf_module
        self.gate_signature = gate_signature
        self.reconstruction_signature = reconstruction_signature
        self.gate_meta = dict(gate_meta)  # Detach the top-level mapping supplied by the caller.
        self.reconstruction_meta = dict(reconstruction_meta)
        self.identity = identity
        self.thresholds = thresholds
        self.thresholds.validate()

        validate_signature_contract(
            self.gate_signature, EXPECTED_GATE_OUTPUT_SHAPES, "gate SavedModel"
        )
        validate_signature_contract(
            self.reconstruction_signature,
            EXPECTED_RECO_OUTPUT_SHAPES,
            "reconstruction SavedModel",
        )
        if warm_up:
            self._warm_up()  # Validate the loaded release with a batch-one execution.

    @classmethod
    # ------------------------------------------------------------------
    # Release construction / startup validation
    # ------------------------------------------------------------------
    # Startup is intentionally fail-closed.  Metadata, frozen geometry,
    # SavedModel signatures and tree hashes are checked before READY is ever
    # printed.  A fallback model, regenerated geometry, positional output map,
    # or partially matching signature would make the release unreproducible.
    def load_from_paths(
        cls,
        tf_module: Any,
        gate_model_dir: str,
        reconstruction_model_dir: str,
        gate_model_meta: str,
        reconstruction_model_meta: str,
        geometry_dir: str,
        thresholds: GateThresholds,
        model_release: str = "cMrdMlReco-B2v042",
        expected_gate_tree_sha256: str = "",
        expected_reconstruction_tree_sha256: str = "",
        allow_tf_version_mismatch: bool = False,
        # TensorFlow is release-pinned unless an explicit compatibility override
        # is requested.
    ) -> "CmrdV5InferenceRuntime":
        tf_version = str(getattr(tf_module, "__version__", ""))
        if not allow_tf_version_mismatch and not tf_version.startswith(EXPECTED_TF_PREFIX):
            raise StartupError(
                f"TensorFlow {tf_version!r} does not match required {EXPECTED_TF_PREFIX}x"
            )

        gate_meta = _load_json(gate_model_meta, "gate model metadata")
        reco_meta = _load_json(reconstruction_model_meta, "reconstruction model metadata")
        gate_hash_from_meta = str(gate_meta.get("geometry_manifest_sha256", ""))
        reco_hash_from_meta = str(reco_meta.get("geometry_manifest_sha256", ""))
        if not gate_hash_from_meta or gate_hash_from_meta != reco_hash_from_meta:
            raise StartupError(
                "model metadata must contain one identical nonempty geometry manifest SHA-256"
            )

        feature_builder = RuntimeFeatureBuilder(
            geometry_dir,
            expected_geometry_sha256=gate_hash_from_meta,
            verify_geometry_checksums=True,
        )
        _validate_metadata(
            gate_meta,
            reco_meta,
            feature_builder.geometry.manifest_sha256,
        )

        gate_tree_hash = _sha256_tree(gate_model_dir)
        reco_tree_hash = _sha256_tree(reconstruction_model_dir)
        _validate_expected_hash(gate_tree_hash, expected_gate_tree_sha256, "gate SavedModel tree")
        _validate_expected_hash(
            reco_tree_hash,
            expected_reconstruction_tree_sha256,
            "reconstruction SavedModel tree",
        )

        _stderr(f"loading gate SavedModel from {Path(gate_model_dir).resolve()}")
        gate_loaded = tf_module.saved_model.load(str(Path(gate_model_dir).expanduser().resolve()))
        _stderr(
            f"loading reconstruction SavedModel from "
            f"{Path(reconstruction_model_dir).resolve()}"
        )
        reco_loaded = tf_module.saved_model.load(
            str(Path(reconstruction_model_dir).expanduser().resolve())
        )
        gate_signature = getattr(gate_loaded, "signatures", {}).get("serving_default")
        reco_signature = getattr(reco_loaded, "signatures", {}).get("serving_default")
        if gate_signature is None:
            raise StartupError("gate SavedModel has no serving_default signature")
        if reco_signature is None:
            raise StartupError("reconstruction SavedModel has no serving_default signature")

        identity = ModelReleaseIdentity(
            model_release=str(model_release),
            gate_model_name=str(gate_meta.get("model_name", "")),
            reconstruction_model_name=str(reco_meta.get("model_name", "")),
            gate_model_tree_sha256=gate_tree_hash,
            reconstruction_model_tree_sha256=reco_tree_hash,
            geometry_version=str(feature_builder.geometry.manifest.get("version", "")),
            geometry_manifest_sha256=feature_builder.geometry.manifest_sha256,
            gate_trainer_version=str(gate_meta.get("trainer_version", "")),
            reconstruction_trainer_version=str(reco_meta.get("trainer_version", "")),
            tensorflow_version=tf_version,
        )
        return cls(
            feature_builder=feature_builder,
            tf_module=tf_module,
            gate_signature=gate_signature,
            reconstruction_signature=reco_signature,
            gate_meta=gate_meta,
            reconstruction_meta=reco_meta,
            identity=identity,
            thresholds=thresholds,
            warm_up=True,
        )

    def release_identity(self) -> Dict[str, Any]:
        return self.identity.as_dict(self.thresholds)
    # Build the ten named model inputs through the authoritative runtime feature layer.
    # ------------------------------------------------------------------
    # Detector packet -> frozen v5 tensors
    # ------------------------------------------------------------------
    # RuntimeFeatureBuilder is the only production implementation of cluster
    # selection and feature assembly.  Keep this method as an adapter/validator
    # around that builder rather than duplicating feature mathematics here.
    def _tensor_inputs(self, packet: Mapping[str, Any]) -> Tuple[Dict[str, Any], Dict[str, Any]]:
        try:
            built = self.feature_builder.build(packet)
        except PacketValidationError as exc:
            raise EventInferenceError(FAILURE_FEATURE_BUILD, "packet_validation_error", str(exc)) from exc
        except FeatureValidationError as exc:
            raise EventInferenceError(
                FAILURE_TENSOR_VALIDATION, "tensor_validation_error", str(exc)
            ) from exc
        except RuntimeFeatureError as exc:
            raise EventInferenceError(FAILURE_FEATURE_BUILD, "feature_build_error", str(exc)) from exc
        except Exception as exc:
            raise EventInferenceError(
                FAILURE_FEATURE_BUILD, "feature_build_exception", str(exc)
            ) from exc

        model_inputs_np = built.model_inputs(add_batch_axis=True)
        try:
            tensor_inputs = {
                name: self.tf.convert_to_tensor(value) for name, value in model_inputs_np.items()
            }
        except Exception as exc:
            raise EventInferenceError(
                FAILURE_TENSOR_VALIDATION, "tensor_conversion_error", str(exc)
            ) from exc
        return tensor_inputs, dict(built.diagnostics)

    def _run_gate(self, tensor_inputs: Mapping[str, Any]) -> Dict[str, np.ndarray]:
        try:
            raw = self.gate_signature(**dict(tensor_inputs))
        except Exception as exc:
            raise EventInferenceError(FAILURE_GATE_INFERENCE, "gate_inference_error", str(exc)) from exc
        try:
            return validate_model_outputs(raw, EXPECTED_GATE_OUTPUT_SHAPES, "gate")
        except Exception as exc:
            raise EventInferenceError(FAILURE_GATE_OUTPUT, "gate_output_error", str(exc)) from exc

    def _run_reconstruction(self, tensor_inputs: Mapping[str, Any]) -> Dict[str, np.ndarray]:
        try:
            raw = self.reconstruction_signature(**dict(tensor_inputs))
        except Exception as exc:
            raise EventInferenceError(
                FAILURE_RECO_INFERENCE, "reconstruction_inference_error", str(exc)
            ) from exc
        try:
            return validate_model_outputs(raw, EXPECTED_RECO_OUTPUT_SHAPES, "reconstruction")
        except Exception as exc:
            raise EventInferenceError(
                FAILURE_RECO_OUTPUT, "reconstruction_output_error", str(exc)
            ) from exc

    def _warm_up(self) -> None:
        _stderr("validating signatures and warming both deployment SavedModels")
        tensor_inputs, _ = self._tensor_inputs(_empty_packet())
        self._run_gate(tensor_inputs)
        self._run_reconstruction(tensor_inputs)
        _stderr("SavedModel signature validation and batch-1 warm-up passed")

    def _gate_object(self, outputs: Mapping[str, np.ndarray]) -> Dict[str, Any]:
        category, confidence, margin, probabilities = _classification(
            outputs["out_event_category"][0]
        )
        has_target = float(outputs["out_has_target"][0, 0])
        reconstructable = float(outputs["out_reconstructable"][0, 0])
        compatibility = float(outputs["out_single_compatibility"][0, 0])
        passed = (
            has_target >= self.thresholds.has_target
            and reconstructable >= self.thresholds.reconstructable
            and (
                not self.thresholds.require_category_zero
                or category == 0
            )
        )
        return {
            "has_target_probability": has_target,
            "reconstructable_probability": reconstructable,
            "event_category": category,
            "event_category_confidence": confidence,
            "event_category_margin": margin,
            "event_category_probabilities": probabilities,
            "single_muon_compatibility_probability": compatibility,
            "single_muon_compatibility_validated": False,
            "passed": bool(passed),
        }

    # Reconstruction routing is physics policy, not an optimization.  Keep
    # the normal Tank-supported route and the validated MRD-only bypass visibly
    # separate so later changes cannot accidentally make one inherit the other
    # route's cuts (notably the Reconstructable head).
    def _reco_skip_reason(self, gate: Mapping[str, Any], diagnostics: Mapping[str, Any]) -> int:
        """Return the primary reason detailed reconstruction should be skipped.

        Skip reasons (0 = run reconstruction):
          1  HasTarget below the gate threshold (both routes)
          2  Reconstructable below threshold (Tank-supported route only)
          3  fewer than 4 live unique MRD hits (both routes)
          4  no Tank hits and the MRD-only bypass conditions are not met
          5  category-zero policy configured and not satisfied

        Tank-supported route (>= 1 selected Tank hit): reasons 1, 2, 3, 5 in
        that priority.

        Tank-absent route: reasons 1, 3, 5, then the frozen MRD-only bypass
        (HasTarget >= 0.90, >= 4 live unique MRD hits, >= 4 MRD layers, a
        retained MRD TimeCluster selected); otherwise reason 4.  The
        Reconstructable head is intentionally ignored on this route because an
        empty Tank input is itself part of the non-reconstructable training
        population.  The C++ adapter derives RecoUsedTankBypass from the same
        input_tank_hits value used here.
        """
        has_target = float(gate["has_target_probability"])
        reconstructable = float(gate["reconstructable_probability"])
        live_hits = int(diagnostics.get("input_mrd_live_unique_hits", 0))
        layers = int(diagnostics.get("input_mrd_layers", 0))
        tank_hits = int(diagnostics.get("input_tank_hits", 0))
        n_mrd_clusters = int(diagnostics.get("number_mrd_time_clusters", 0))
        selected_mrd_cluster = int(diagnostics.get("selected_mrd_cluster_index", -1))

        # HasTarget remains mandatory for both normal and bypass routes.
        if has_target < self.thresholds.has_target:
            return 1

        # Normal Tank-supported route: frozen reason priority.
        if tank_hits >= 1:
            if reconstructable < self.thresholds.reconstructable:
                return 2
            if live_hits < 4:
                return 3
            if self.thresholds.require_category_zero and int(gate["event_category"]) != 0:
                return 5
            return 0

        # Tank-absent route.  Even a bypass attempt must satisfy the original
        # minimum MRD support and any configured category-zero policy.
        if live_hits < 4:
            return 3
        if self.thresholds.require_category_zero and int(gate["event_category"]) != 0:
            return 5

        # Conservative MRD-only Tank bypass.  Reconstructable is intentionally
        # not consulted here.
        retained_cluster_ok = (
            n_mrd_clusters >= 1
            and selected_mrd_cluster >= 0
        )
        tank_bypass = (
            has_target >= TANK_BYPASS_MIN_HAS_TARGET
            and live_hits >= TANK_BYPASS_MIN_MRD_LIVE_UNIQUE_HITS
            and layers >= TANK_BYPASS_MIN_MRD_LAYERS
            and (
                retained_cluster_ok
                if TANK_BYPASS_REQUIRE_RETAINED_MRD_TIME_CLUSTER
                else True
            )
        )
        if tank_bypass:
            return 0

        # No selected Tank hits and the MRD pattern was not strong enough for
        # the bypass.  Keep existing skip-reason code 4 so the
        # C++/Store contract remains unchanged.
        return 4

    # Reconstruction interpretation uses the frozen topology convention.
    # Topology classes: 0 = stopped, 1 = back-exiting, 2 = side-exiting.
    # Theta uncertainty is propagated from sigma(cos theta).
    # The derivative diverges near theta = 0 or pi, so theta sigma is invalid there.
    # Selected endpoint experts follow the predicted topology class.
    # Convert named SavedModel outputs into the physical response contract.
    # The deployment graph already returns physical means/sigmas; applying an
    # external denormalization here would be a double transform.  Derived theta
    # and phi are bookkeeping only and retain the underlying head values too.
    def _reconstruction_object(self, outputs: Mapping[str, np.ndarray]) -> Dict[str, Any]:
        topology_class, topology_confidence, topology_margin, topology_probs = _classification(
            outputs["out_topology"][0]
        )
        exit_class, exit_confidence, exit_margin, exit_probs = _classification(
            outputs["out_exit_face"][0]
        )

        length, length_sigma = _scalar_mean_sigma(
            outputs["out_length_mean_sigma_physical"][0], "MRD track length"
        )
        cos_theta, cos_theta_sigma = _scalar_mean_sigma(
            outputs["out_cos_theta_mean_sigma_physical"][0], "cos theta"
        )
        penetration, penetration_sigma = _scalar_mean_sigma(
            outputs["out_penetration_mean_sigma_physical"][0], "penetration"
        )
        energy_loss, energy_loss_sigma = _scalar_mean_sigma(
            outputs["out_energy_loss_mean_sigma_physical"][0], "energy loss"
        )
        muon_ke, muon_ke_sigma = _scalar_mean_sigma(
            outputs["out_muon_ke_mean_sigma_physical"][0], "muon KE"
        )
        cherenkov, cherenkov_sigma = _scalar_mean_sigma(
            outputs["out_cherenkov_mean_sigma_physical"][0], "Cherenkov angle"
        )

        tank_vertex, tank_vertex_sigma = _mean_sigma(
            outputs["out_vertex_xyz_mean_sigma_physical"][0], 3, "tank vertex"
        )
        tank_exit, tank_exit_sigma = _mean_sigma(
            outputs["out_tank_exit_xyz_mean_sigma_physical"][0], 3, "tank exit"
        )
        mrd_entry, mrd_entry_sigma = _mean_sigma(
            outputs["out_mrd_entry_xyz_mean_sigma_physical"][0], 3, "MRD entry"
        )
        mrd_exit, mrd_exit_sigma = _mean_sigma(
            outputs["out_mrd_exit_xyz_mean_sigma_physical"][0], 3, "MRD exit"
        )
        stop_endpoint, stop_endpoint_sigma = _mean_sigma(
            outputs["out_stop_xyz_mean_sigma_physical"][0], 3, "stop endpoint"
        )
        back_endpoint, back_endpoint_sigma = _mean_sigma(
            outputs["out_back_xyz_mean_sigma_physical"][0], 3, "back endpoint"
        )
        side_endpoint, side_endpoint_sigma = _mean_sigma(
            outputs["out_side_xyz_mean_sigma_physical"][0], 3, "side endpoint"
        )
        mrd_scatter, mrd_scatter_sigma = _mean_sigma(
            outputs["out_mrd_scatter_mean_sigma_physical"][0], 3, "MRD scatter"
        )
        tank_scatter, tank_scatter_sigma = _mean_sigma(
            outputs["out_tank_scatter_mean_sigma_physical"][0], 3, "tank scatter"
        )

        expert_means = [stop_endpoint, back_endpoint, side_endpoint]
        expert_sigmas = [stop_endpoint_sigma, back_endpoint_sigma, side_endpoint_sigma]
        selected_endpoint = list(expert_means[topology_class])
        selected_endpoint_sigma = list(expert_sigmas[topology_class])

        sin_phi = float(outputs["out_sin_phi"][0, 0])
        cos_phi = float(outputs["out_cos_phi"][0, 0])
        phi = math.atan2(sin_phi, cos_phi)
        phi_norm = math.hypot(sin_phi, cos_phi)

        clipped_cos_theta = min(1.0, max(-1.0, cos_theta))
        theta = math.acos(clipped_cos_theta)
        sin_theta = math.sin(theta)
        theta_sigma_valid = bool(
            math.isfinite(cos_theta_sigma)
            and cos_theta_sigma >= 0.0
            and sin_theta > 1.0e-6
            and abs(cos_theta) <= 1.0
        )
        theta_sigma = cos_theta_sigma / sin_theta if theta_sigma_valid else -999.0

        return {
            "topology_class": topology_class,
            "topology_confidence": topology_confidence,
            "topology_margin": topology_margin,
            "topology_probabilities": topology_probs,
            "topology_p_stopped": topology_probs[0],
            "topology_p_back_exit": topology_probs[1],
            "topology_p_side_exit": topology_probs[2],
            "stopped_probability": float(outputs["out_stopped"][0, 0]),
            "exit_face_class": exit_class,
            "exit_face_confidence": exit_confidence,
            "exit_face_margin": exit_margin,
            "exit_face_probabilities": exit_probs,
            "endpoint_expert_used": topology_class,
            "mrd_track_length_cm": length,
            "mrd_track_length_sigma_cm": length_sigma,
            "cos_theta": cos_theta,
            "cos_theta_sigma": cos_theta_sigma,
            "theta_rad": theta,
            "theta_sigma_rad": theta_sigma,
            "theta_sigma_valid": theta_sigma_valid,
            "sin_phi": sin_phi,
            "cos_phi": cos_phi,
            "phi_rad": phi,
            "phi_vector_norm": phi_norm,
            "penetration_cm": penetration,
            "penetration_sigma_cm": penetration_sigma,
            "mrd_energy_loss_mev": energy_loss,
            "mrd_energy_loss_sigma_mev": energy_loss_sigma,
            "muon_ke_mev": muon_ke,
            "muon_ke_sigma_mev": muon_ke_sigma,
            "cherenkov_angle_rad": cherenkov,
            "cherenkov_angle_sigma_rad": cherenkov_sigma,
            "tank_vertex_cm": tank_vertex,
            "tank_vertex_sigma_cm": tank_vertex_sigma,
            "tank_exit_cm": tank_exit,
            "tank_exit_sigma_cm": tank_exit_sigma,
            "mrd_entry_cm": mrd_entry,
            "mrd_entry_sigma_cm": mrd_entry_sigma,
            "mrd_exit_cm": mrd_exit,
            "mrd_exit_sigma_cm": mrd_exit_sigma,
            "stop_endpoint_cm": stop_endpoint,
            "stop_endpoint_sigma_cm": stop_endpoint_sigma,
            "back_exit_endpoint_cm": back_endpoint,
            "back_exit_endpoint_sigma_cm": back_endpoint_sigma,
            "side_exit_endpoint_cm": side_endpoint,
            "side_exit_endpoint_sigma_cm": side_endpoint_sigma,
            "selected_endpoint_cm": selected_endpoint,
            "selected_endpoint_sigma_cm": selected_endpoint_sigma,
            "mrd_scatter_rad": mrd_scatter,
            "mrd_scatter_sigma_rad": mrd_scatter_sigma,
            "tank_scatter_rad": tank_scatter,
            "tank_scatter_sigma_rad": tank_scatter_sigma,
            # Reserved until ToolAnalysis-side quality working points are frozen.
            "quality_baseline": False,
            "quality_topology": False,
            "quality_direction": False,
            "quality_endpoint": False,
            "quality_tier": 0,
        }

    # ------------------------------------------------------------------
    # One event: build -> gate -> route -> optional reconstruction
    # ------------------------------------------------------------------
    # A successful gate does not imply reconstruction ran.  reco_skip_reason
    # records the first frozen policy reason that stopped reconstruction, while
    # the gate object and diagnostics are still returned for commissioning and
    # downstream selections.
    def process(self, packet: Mapping[str, Any]) -> Dict[str, Any]:
        diagnostics: Dict[str, Any] = {}
        gate: Optional[Dict[str, Any]] = None
        try:
            tensor_inputs, diagnostics = self._tensor_inputs(packet)
            gate_outputs = self._run_gate(tensor_inputs)
            gate = self._gate_object(gate_outputs)
            skip_reason = self._reco_skip_reason(gate, diagnostics)

            if skip_reason != 0:
                return {
                    "protocol_version": PROTOCOL_VERSION,
                    "ok": True,
                    "status": "gate_ok_reconstruction_skipped",
                    "failure_code": 0,
                    "reco_skip_reason": int(skip_reason),
                    "error": "",
                    "input_build_ok": True,
                    "gate_attempted": True,
                    "gate_inference_ok": True,
                    "gate_passed": bool(gate["passed"]),
                    "reco_attempted": False,
                    "reco_inference_ok": False,
                    "diagnostics": diagnostics,
                    "gate": gate,
                    "reconstruction": None,
                }

            reco_outputs = self._run_reconstruction(tensor_inputs)
            reconstruction = self._reconstruction_object(reco_outputs)
            return {
                "protocol_version": PROTOCOL_VERSION,
                "ok": True,
                "status": "reconstruction_ok",
                "failure_code": 0,
                "reco_skip_reason": 0,
                "error": "",
                "input_build_ok": True,
                "gate_attempted": True,
                "gate_inference_ok": True,
                "gate_passed": bool(gate["passed"]),
                "reco_attempted": True,
                "reco_inference_ok": True,
                "diagnostics": diagnostics,
                "gate": gate,
                "reconstruction": reconstruction,
            }
        except EventInferenceError as exc:
            if diagnostics and not exc.diagnostics:
                exc.diagnostics = dict(diagnostics)
            if gate is not None and exc.gate is None:
                exc.gate = dict(gate)
            raise

    # Convert known per-event exceptions into structured responses.  This is
    # deliberately different from startup errors: a bad trigger should not kill
    # a long ANNIE job, but the response must still say exactly how far the event
    # progressed so C++ cannot mistake a technical failure for a physics skip.
    def process_safe(self, packet: Mapping[str, Any]) -> Dict[str, Any]:
        try:
            return self.process(packet)
        except EventInferenceError as exc:
            input_ok = bool(exc.diagnostics)
            gate_attempted = exc.failure_code in {
                FAILURE_GATE_INFERENCE,
                FAILURE_GATE_OUTPUT,
                FAILURE_RECO_INFERENCE,
                FAILURE_RECO_OUTPUT,
            }
            gate_ok = exc.gate is not None or exc.failure_code in {
                FAILURE_RECO_INFERENCE,
                FAILURE_RECO_OUTPUT,
            }
            reco_attempted = exc.failure_code in {
                FAILURE_RECO_INFERENCE,
                FAILURE_RECO_OUTPUT,
            }
            return {
                "protocol_version": PROTOCOL_VERSION,
                "ok": False,
                "status": exc.status,
                "failure_code": exc.failure_code,
                "reco_skip_reason": 0,
                "error": str(exc),
                "input_build_ok": input_ok,
                "gate_attempted": gate_attempted,
                "gate_inference_ok": gate_ok,
                "gate_passed": bool(exc.gate.get("passed", False)) if exc.gate else False,
                "reco_attempted": reco_attempted,
                "reco_inference_ok": False,
                "diagnostics": dict(exc.diagnostics),
                "gate": dict(exc.gate) if exc.gate is not None else None,
                "reconstruction": None,
            }
        except Exception as exc:
            return {
                "protocol_version": PROTOCOL_VERSION,
                "ok": False,
                "status": "unclassified_daemon_error",
                "failure_code": FAILURE_DAEMON_PROTOCOL,
                "reco_skip_reason": 0,
                "error": str(exc),
                "input_build_ok": False,
                "gate_attempted": False,
                "gate_inference_ok": False,
                "gate_passed": False,
                "reco_attempted": False,
                "reco_inference_ok": False,
                "diagnostics": {},
                "gate": None,
                "reconstruction": None,
            }
# Event processing always returns the same response layout for success, skip, or failure.
# The full workflow builds inputs, runs the gate, and conditionally reconstructs.
# Successful, skipped, and failed events share one response schema so the C++
# adapter can publish deterministic status, diagnostics, and output keys.
# process_safe converts known inference failures and unexpected exceptions into
# complete error responses instead of terminating the persistent daemon.
#

def _json_loads_strict(text: str) -> Any:
    def reject_constant(value: str) -> None:
        raise ValueError(f"non-standard JSON constant {value!r} is forbidden")
    return json.loads(text, parse_constant=reject_constant)


def _json_dumps_strict(value: Any) -> str:
    return json.dumps(
        value,
        allow_nan=False,
        sort_keys=True,
        separators=(",", ":"),
    )


def parse_args(argv: Optional[Sequence[str]] = None) -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="cMrdMlReco persistent inference daemon for the frozen ANNIE cMRD v5 models")
    parser.add_argument("--gate-model-dir", required=True,
                        help="Final gate deployment SavedModel directory")
    parser.add_argument("--reconstruction-model-dir", required=True,
                        help="Final reconstruction deployment SavedModel directory")
    parser.add_argument("--gate-model-meta", required=True)
    parser.add_argument("--reconstruction-model-meta", required=True)
    parser.add_argument("--geometry-dir", required=True)
    parser.add_argument("--model-release", default="cMrdMlReco-B2v042")
    parser.add_argument("--gate-has-target-threshold", type=float, default=0.5)
    parser.add_argument("--gate-reconstructable-threshold", type=float, default=0.0)
    parser.add_argument("--require-gate-category-zero", type=int, choices=(0, 1), default=0)
    parser.add_argument("--expected-gate-model-tree-sha256", default="")
    parser.add_argument("--expected-reconstruction-model-tree-sha256", default="")
    parser.add_argument("--allow-tf-version-mismatch", action="store_true")
    parser.add_argument("--tensorflow-intra-op-threads", type=int, default=1)
    parser.add_argument("--tensorflow-inter-op-threads", type=int, default=1)
    parser.add_argument("--traceback-on-event-error", action="store_true")
    return parser.parse_args(argv)

# Build a complete failure response for malformed protocol input.
# No feature construction or model inference is attempted, so all stage-status
# flags remain false and diagnostics are empty.
# The response still uses the normal schema so C++ remains synchronized.
def _protocol_error_response(message: str) -> Dict[str, Any]:
    return {
        "protocol_version": PROTOCOL_VERSION,
        "ok": False,
        "status": "daemon_protocol_error",
        "failure_code": FAILURE_DAEMON_PROTOCOL,
        "reco_skip_reason": 0,
        "error": str(message),
        "input_build_ok": False,
        "gate_attempted": False,
        "gate_inference_ok": False,
        "gate_passed": False,
        "reco_attempted": False,
        "reco_inference_ok": False,
        "diagnostics": {},
        "gate": None,
        "reconstruction": None,
    }

# Start the persistent daemon from the validated command-line configuration.
# Startup loads and warms both SavedModels before the sole READY handshake is
# written to stdout. The main loop then consumes one JSON packet per input line
# and writes one JSON response per event.
# QUIT or end-of-input exits cleanly; stdout remains protocol-only throughout.
#
def main(argv: Optional[Sequence[str]] = None) -> int:
    args = parse_args(argv)
    thresholds = GateThresholds(
        has_target=args.gate_has_target_threshold,
        reconstructable=args.gate_reconstructable_threshold,
        require_category_zero=bool(args.require_gate_category_zero),
    )
    # Import TensorFlow only after the daemon environment variables have been
    # configured, then apply any requested CPU thread limits. Load and validate
    # the complete model release; any failure here prevents the READY handshake.
    try:
        import tensorflow as tf

        if args.tensorflow_intra_op_threads > 0:
            tf.config.threading.set_intra_op_parallelism_threads(
                args.tensorflow_intra_op_threads
            )
        if args.tensorflow_inter_op_threads > 0:
            tf.config.threading.set_inter_op_parallelism_threads(
                args.tensorflow_inter_op_threads
            )

        runtime = CmrdV5InferenceRuntime.load_from_paths(
            tf_module=tf,
            gate_model_dir=args.gate_model_dir,
            reconstruction_model_dir=args.reconstruction_model_dir,
            gate_model_meta=args.gate_model_meta,
            reconstruction_model_meta=args.reconstruction_model_meta,
            geometry_dir=args.geometry_dir,
            thresholds=thresholds,
            model_release=args.model_release,
            expected_gate_tree_sha256=args.expected_gate_model_tree_sha256,
            expected_reconstruction_tree_sha256=args.expected_reconstruction_model_tree_sha256,
            allow_tf_version_mismatch=args.allow_tf_version_mismatch,
        )
    except Exception as exc:
        _stderr(f"FATAL startup failure: {exc}")
        traceback.print_exc(file=sys.stderr)
        return 2
    # Startup has now verified the geometry, metadata, model hashes, signatures,
    # and batch-one execution. Stdout is reserved for the READY handshake and
    # subsequent one-line JSON responses consumed by the C++ Tool.
    # No stdout content is permitted before this verified READY line.
    print("READY " + _json_dumps_strict(runtime.release_identity()), flush=True)

    for raw_line in sys.stdin:
        line = raw_line.strip()
        if not line:
            continue
        if line == "QUIT":
            break
        try:
            packet = _json_loads_strict(line)
            if not isinstance(packet, Mapping):
                raise ValueError("top-level event packet must be a JSON object")
            response = runtime.process_safe(packet)
            if not response.get("ok", False):
                _stderr(
                    f"event failure code={response.get('failure_code')} "
                    f"status={response.get('status')}: {response.get('error')}"
                )
                if args.traceback_on_event_error:
                    traceback.print_exc(file=sys.stderr)
        except Exception as exc:
            response = _protocol_error_response(str(exc))
            _stderr(f"protocol input error: {exc}")
            if args.traceback_on_event_error:
                traceback.print_exc(file=sys.stderr)
        # Serialize and flush exactly one response for the current input event.
        # A serialization failure is fatal because it would break synchronization
        # between the Python daemon and the C++ line-based protocol.
        try:
            print(_json_dumps_strict(response), flush=True)
        except Exception as exc:
            # This should be unreachable because all model outputs are validated
            # as finite and JSON-native before response construction.
            _stderr(f"FATAL response serialization failure: {exc}")
            return 3

    _stderr("clean shutdown")
    return 0

# Run main only when this file is launched as the daemon executable, not when it
# is imported by tests or supporting Python code. Convert main's integer return
# value into the process exit status seen by the C++ Tool.
if __name__ == "__main__":
    raise SystemExit(main())
