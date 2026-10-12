# cMrdMlReco

`cMrdMlReco` runs the frozen ANNIE cMRD machine-learning gate and reconstruction
models on every trigger. It publishes the results to `Stores["MrdMLReco"]`, and
`PhaseIITreeMaker` writes them out as `mrd_ml_*` branches.

| Name | What it is |
|---|---|
| **cMRD v5** | The frozen model, feature and packet interface: the gate and reconstruction SavedModels, the frozen geometry tables, and the Python runtime (`cmrd_v5_*.py`). |
| **cMrdMlReco v6** | This ToolAnalysis adapter and its Store/ROOT output contract (`SchemaVersion 2`, `AdapterVersion cMrdMlReco-adapter-v6.0`). |
| **cMrdMlReco-B2v042** | The deployment release: the repaired B2 v0.4.2 reconstruction tree plus the frozen gate and geometry. |

"v6" does **not** mean a v6 neural network. The models are the v5 models, and v6 changes only the adapter and its output contract.

## How it works

The C++ adapter has four responsibilities:
1. Collects detector-level products from the ToolAnalysis stores.
2. Serializes them into a one-line JSON event packet.
3. Supervises a persistent Python inference daemon.
4. Validates the daemon response and publishes it, once per event.

The frozen Python runtime handles the model-side logic:
- Cluster selection, including the EventSelector-mimic Tank↔MRD association.
- The frozen dead mask and feature construction.
- The decision whether to run reconstruction and by which route, including the Tank bypass.
- TensorFlow inference.

The daemon starts once at `Initialise`. It is accepted only after a `READY` handshake confirms:
- the protocol version,
- the release name,
- the geometry manifest hash,
- the gate and reconstruction SavedModel tree hashes,
- the gate thresholds.


## Maintenance invariants

The source comments intentionally preserve the same ownership boundaries used during the alpha commissioning release:

- **C++ is plumbing, not a second feature builder.** Detector products are collected and serialized in C++; cluster choice, dead masking, geometry lookup and tensor construction stay in the frozen Python feature path.
- **No classical/truth leakage.** GENIE/truth, `SimpleReco`, `MuonFitter` and `FindMrdTracks` fitted quantities are not model inputs.
- **Fail closed on release identity, fail soft on an event.** A model/geometry/protocol mismatch prevents startup; an isolated bad trigger is published with sentinels and a `FailureCode`.
- **One event, one publication.** `Stores["MrdMLReco"]` is rewritten from one result object after processing finishes, preventing stale values from surviving a late failure.
- **Production/debug is publication-only.** Shared outputs must be exactly equal between modes; use `parity_check.py` to enforce this.
- **Do not casually duplicate the Python logic in C++.** If feature semantics must change, treat that as a new model-interface release and validate it explicitly.

These comments are deliberately more explanatory than normal line-by-line comments so a future maintainer can tell which choices are accidental implementation details and which are frozen reconstruction invariants.

## Required ToolChain order

```
LoadGeometry, then LoadANNIEEvent (data) or LoadWCSim (MC)   ->  AnnieGeometry, TDCData
TimeClustering                                               ->  MrdTimeClusters, MrdDigit*
ClusterFinder                                                ->  ClusterMap(MC), ClusterMapDetkey
cMrdMlReco                                                   ->  Stores["MrdMLReco"], CStore["MrdMLOutputMode"]
PhaseIITreeMaker  (MRDRecoSource MrdMLReco | Both)
```

- **Real data** also needs `ChannelNumToTankPMTSPEChargeMap` in the CStore (the same calibration `PhaseIITreeMaker` uses), so the tool can convert tank charge to PE.
- **Order is enforced.** `PhaseIITreeMaker` refuses to initialise with `MRDRecoSource` set to `MrdMLReco` or `Both` unless `cMrdMlReco` ran its `Initialise` first.
- **cMRD does not need FindMrdTracks or MuonFitter.** They are needed only for the legacy branches when `MRDRecoSource Both` is used.
- **Lean chains:** a chain without EventSelector (so no `RecoEvent` store) also needs the optional lean-chain `PhaseIITreeMaker` patch (`0003`) **and** `fillCleanEventsOnly 0`. The clean-event filter still reads `EventFlagApplied` and `EventFlagged` from `RecoEvent` unconditionally.

## Where things live

| Item | Location |
|---|---|
| C++ adapter, this README, the example config | GitHub, `UserTools/cMrdMlReco/` |
| Python runtime (`cmrd_v5_inference.py`, `cmrd_v5_runtime_features.py`, `cmrd_v5_features.py`) | GitHub, `UserTools/cMrdMlReco/runtime/`. Installed copy at `.../cMrdMlreco/current/runtime/` on PNFS. |
| SavedModels, `model_meta.json`, frozen geometry | PNFS only: `/pnfs/annie/persistent/reconstruction/cMrdMlreco/current/release/` |

The runtime code and the interface contract travel with the tool. The large binary release does not; it is pinned by the hashes below.

## Configuration

The built-in defaults equal the frozen production configuration. A missing key therefore cannot bring back an older policy. Only the paths have no default.

| Key | Default | Meaning |
|---|---|---|
| `OutputMode` | `production` | `production` or `debug`; legacy `smoke-test` aliases map to `debug`. |
| `IsData` | `0` | `1` for data (`Hit`, `ClusterMap`), `0` for MC (`MCHit`, `ClusterMapMC`). |
| `PythonExe` | `python3` | Interpreter for the daemon. |
| `InferenceScript` | — | `cmrd_v5_inference.py` |
| `GateModelDir`, `ReconstructionModelDir` | — | SavedModel directories. |
| `GateModelMeta`, `ReconstructionModelMeta` | — | `model_meta.json` files. |
| `GeometryDir` | — | Frozen geometry package. |
| `ModelRelease` | `cMrdMlReco-B2v042` | Must match the daemon's `READY` handshake. |
| `ExpectedGeometryManifestSHA256` | `8c63f966…fe8ab` | Checked at startup. |
| `ExpectedGateModelTreeSHA256` | `b72962be…4fd240` | Checked at startup. |
| `ExpectedReconstructionModelTreeSHA256` | `27aba803…cbc36` | Checked at startup. |
| `GateHasTargetThreshold` | `0.5` | Loose target gate. |
| `GateReconstructableThreshold` | `0.0` | Kept as a quality score only; never hard-rejects an event. |
| `RequireGateCategoryZero` | `0` | |
| `AllowTFVersionMismatch` | `0` | |
| `TensorFlowIntraOpThreads`, `TensorFlowInterOpThreads` | `1`, `1` | |
| `DaemonStartupTimeoutSec`, `DaemonResponseTimeoutSec`, `DaemonShutdownTimeoutSec` | `240`, `60`, `5` | |
| `RestartDaemonOnFailure`, `MaxDaemonRestarts` | `1`, `3` | |
| `MaxProtocolLineMB` | `8` | |
| `PrefilterMinMRDHits` | `0` (off) | See below. Keep off for the first production release. |
| `MCTDCDataKey` | `TDCData` | |
| `AdditionalDeadMRDChankeys` | empty | Recorded in debug diagnostics only; does not change the frozen mask. |
| `DebugPacketDirectory`, `DebugPacketMaxEvents`, `DebugResponseFile` | unset | Debug mode only. |
| `StoreName` | `MrdMLReco` | Fixed. Any other value is a configuration error. |
| `WriteMRDTracksCompatibility`, `WriteMuonFitterCompatibility` | `0` | Not implemented; must stay `0`. |

## Reconstruction routing (frozen)

The daemon decides whether to run reconstruction. `RecoSkipReason` records why it did not.

| RecoSkipReason | Meaning |
|---|---|
| 0 | Reconstruction ran. |
| 1 | HasTarget is below `GateHasTargetThreshold`. |
| 2 | Reconstructable is below its threshold (Tank-supported route only). |
| 3 | Fewer than 4 live unique MRD hits. |
| 4 | No Tank hits, and the MRD-only bypass conditions were not met. |
| 5 | The category-zero policy is configured and not satisfied. |
| 6 | The C++ prefilter skipped the daemon (only when `PrefilterMinMRDHits > 0`). |

There are two routes, chosen by whether the selected Tank input has any hits:
- **Tank-supported route (≥ 1 selected Tank hit):** the reasons are checked in the order 1, 2, 3, 5.
- **MRD-only Tank-bypass route (0 selected Tank hits):** the reasons are checked in the order 1, 3, 5. Reconstruction then runs only if **all** of these hold:
  - HasTarget ≥ 0.90,
  - ≥ 4 live unique MRD hits,
  - ≥ 4 MRD layers,
  - a retained MRD TimeCluster was selected (raw-TDC fallback never qualifies).

  Otherwise the reason is 4. The Reconstructable head is not consulted on this route.

`RecoUsedTankBypass` is true exactly when reconstruction was attempted on the bypass route:
`RecoAttempted && InputTankHits < 1`. This is derived in C++ from the same `input_tank_hits` value the daemon routes on.

That derivation can never silently guess. Whenever `input_build_ok` is true, the response must carry the diagnostics object with all nine production provenance fields. Also, `reco_attempted` must imply `input_build_ok`. A response that breaks either rule is a protocol error (FailureCode 12), not an event wrongly classified as bypass.

On bypass events, the tank-side outputs (vertex, tank exit, Cherenkov angle) come from a model that saw no tank hits. Treat those events as a separate sample in physics analyses.

### Optional prefilter

`PrefilterMinMRDHits N` skips the daemon for events with fewer than N MRD digits in total.
- **What is lost:** those events get no gate probabilities, and `RecoSkipReason` is 6.
- **What is kept:** with N ≤ 4 no event the daemon would have reconstructed is lost, because both routes need ≥ 4 live hits in the selected cluster.

It is off for the first production release until a production-vs-prefilter parity sample has been validated.

## Output: `Stores["MrdMLReco"]`

**Sentinel values:**
- Counts, indices and classes: `-1`.
- Probabilities: `-1.0`.
- Physical scalars and `[x,y,z]` vectors: `-999.0`.
- Strings: `""`.

Every per-event key of the active mode is rewritten on every event, so a value from a previous event can never survive. Job-level keys are written after each `READY` handshake.

### Production per-event keys (all also written in debug)

| Group | Keys |
|---|---|
| Status | `FailureCode` `RecoSkipReason` `GatePassed` `ReconstructionAvailable` |
| Provenance | `MRDInputSource` `NumberMrdTimeClusters` `SelectedMrdClusterIndex` `SelectedTankClusterIndex` `InputTankHits` `RecoUsedTankBypass` `TankMrdAssociationFound` `TankMrdSelectedTankClusterIndex` `TankMrdSelectedMrdSubEventID` `TankMrdTimingResidualNs` |
| Gate | `HasTargetMuonProbability` `ReconstructableProbability` `EventCategory` |
| Topology | `TopologyClass` (0 stopped, 1 back exit, 2 side exit) `TopologyProbabilities[3]` `StoppedProbability` `ExitFaceClass` |
| Reco ± σ | `MRDTrackLengthCm` `CosTheta` `ThetaRad` `PenetrationCm` `MRDEnergyLossMeV` `MuonKEMeV` `CherenkovAngleRad`, each with its `…Sigma…` key; also `ThetaSigmaValid` and `PhiRad` (the model gives no σ for φ) |
| Positions ± σ | `TankVertexCm` `TankExitCm` `MRDEntryCm` `MRDExitCm` `SelectedEndpointCm` `MRDScatterRad` `TankScatterRad`, each with its `…SigmaCm`/`…SigmaRad` key |

### Debug-only per-event keys

| Group | Keys |
|---|---|
| Status | `ProtocolVersion` `StatusText` `ErrorText` `InputBuildOK` `GateAttempted` `GateInferenceOK` `RecoAttempted` `RecoInferenceOK` `DaemonRestartCount` |
| Identity | `RunNumber` `SubrunNumber` `EventNumber` `MCEventNum` `MCTriggerNum` `IsData` |
| Input diagnostics | `SelectedMrdCluster{Start,End}TimeNs` `SelectedMrdClusterTimeSpanNs` `InputMRD*` `TankInputSource` `NumberTankClusters` `SelectedTankCluster{TimeNs,ChargePE,TimeSpanNs}` `TankPacketReorderedByTime` `AdditionalDeadMRDChankeys` `RecoStructurallyEligible` `PhysicsIsRawUnstandardized` |
| Association | `TankMrdAssociationApplied` `TankMrdCompatiblePairCount` `TankMrdExpectedOffsetNs` `TankMrdCoincidenceWindowNs` `TankMrdTankMeanTimeNs` `TankMrdMrdMeanTimeNs` `TankMrdDtNs` |
| Full gate and topology | `EventCategoryConfidence` `EventCategoryMargin` `EventCategoryProbabilities[7]` `SingleMuonCompatibility{Probability,Validated}` `TopologyConfidence` `TopologyMargin` `TopologyP{Stopped,BackExit,SideExit}` `ExitFaceConfidence` `ExitFaceMargin` `ExitFaceProbabilities[7]` `EndpointExpertUsed` `SinPhi` `CosPhi` `PhiVectorNorm` |
| Per-expert endpoints | `{Stop,BackExit,SideExit}EndpointCm` with their `…SigmaCm` keys |
| Placeholders | `Quality{Baseline,Topology,Direction,Endpoint,Tier}` (always false/0 in this release) |

### Job-level keys (both modes)

- `SchemaVersion` `OutputMode` `AdapterVersion`
- `ModelRelease` `DaemonVersion` `RuntimeFeatureVersion` `TensorFlowVersion`
- `GateModelName` `ReconstructionModelName` `GateModelTreeSHA256` `ReconstructionModelTreeSHA256` `GateTrainerVersion` `ReconstructionTrainerVersion`
- `GeometryVersion` `GeometryManifestSHA256`
- `GateHasTargetThreshold` `GateReconstructableThreshold` `RequireGateCategoryZero` `PrefilterMinMRDHits`

## ROOT output (PhaseIITreeMaker)

**`phaseIITriggerTree`** gets one `mrd_ml_<snake_case>` branch per per-event key of the active branch set. Examples: `mrd_ml_muon_ke_mev`, `mrd_ml_reco_used_tank_bypass`.
- **Production set:** 51 per-event branches, plus `mrd_ml_schema_version`, `mrd_ml_output_mode` and `mrd_ml_model_release` on every entry.
- **Debug set:** adds 67 more branches.

**`mrd_ml_metadata`** is a one-entry tree holding every job-level key, plus `mrd_reco_source` and `branch_set`. It makes the file self-describing without repeating 64-character hashes on every entry.

The metadata tree is part of the release contract. If any job-level key is missing, the incomplete tree is still written for inspection, but `PhaseIITreeMaker::Finalise()` returns false.

## Failure handling

| Kind of failure | What happens |
|---|---|
| Startup, configuration, or model contract | `Initialise` returns false and the job does not start. |
| One bad event | The event gets sentinels and a structured `FailureCode`, and the ToolChain continues. |

`FailureCode` values:

| Code | Meaning |
|---|---|
| 0 | OK |
| 1 | `ANNIEEvent` missing |
| 2 | `AnnieGeometry` missing |
| 3 | MRD mapping missing |
| 4 | Inconsistent MRD input |
| 5 | Inconsistent tank input |
| 6 | NaN/Inf in detector input, or packet too large |
| 10 | Daemon unavailable |
| 11 | Daemon timeout |
| 12 | Daemon protocol or response error |
| 13 | Daemon restarts exhausted |
| 20–51 | Passed through from the daemon (feature build, gate or reconstruction inference) |

After a transport or protocol error the daemon is stopped. It is restarted on a later event, up to `MaxDaemonRestarts` times.

## Validation before a release

1. Data: debug vs production.
2. WCSim: debug vs production.
3. Data: v6-debug vs the known-good B2 output on the same events.
4. WCSim: v6-debug vs the known-good B2 output on the same events.

Physics quantities and event decisions must agree event by event. Only the breadth of publication may differ, and no branch present in both outputs may differ in value. `parity_check.py` performs these comparisons on `PhaseIITreeMaker` ROOT files:

```
python3 parity_check.py modes debug.root production.root
python3 parity_check.py reference v6_debug.root b2.root --b2-reference [--audited audited.txt]
```

- **`modes`** requires exactly one `mrd_ml_metadata` entry in each file.
- **`reference`** requires exactly one entry in the candidate file.
- **`--b2-reference`** skips only the 11 `mrd_ml_tank_mrd_*` association branches, which B2 declared but never filled. Physics and decision branches are still compared in full.
- **`--audited`** excludes listed events but prints every difference. Use `run subrun event` for data and add `mcEntry` when both MC files contain `mcEntryNumber`. This is intended for the deliberate NaN-input change; audit those events by hand rather than suppressing them globally.

Then build the exact candidate from a clean checkout using the collaboration build path.
