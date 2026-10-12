#!/usr/bin/env python3
"""Event-by-event parity check for cMrdMlReco PhaseIITreeMaker output.

Two checks:

  modes      Compare a DEBUG file with a PRODUCTION file made from the same input.
             * Every mrd_ml_* branch in the production file must also exist in
               the debug file.
             * Each shared branch must have the same value on every event.
             * mrd_ml_output_mode is the only branch allowed to differ.
             * Each file must contain exactly one mrd_ml_metadata entry, and
               the two must agree apart from output_mode and branch_set.

  reference  Compare a v6 file (normally debug) with a known-good file, such
             as the commissioned B2 output, on the same events.
             * Each physics branch present in both files must agree within
               --atol (default: exact).
             * The event-decision branches must agree exactly: failure code,
               skip reason, gate passed, reconstruction available, topology,
               exit face and event category.
             * Identity and schema branches are reported, not failed.
             * The candidate must contain exactly one mrd_ml_metadata entry.

             --b2-reference  The reference is a commissioned B2-era file.
               Those files declared the 11 mrd_ml_tank_mrd_* association
               branches but never filled them (v6 fixes the publication), so
               they are skipped and listed.  Physics and decision branches are
               still compared in full.
             --audited FILE  Events listed in FILE are excluded from pass/fail
               but still printed with their differences. Use "run subrun event"
               for data, or "run subrun event mcEntry" when both files contain
               mcEntryNumber (for example, audited NaN-input MC events).

Events are matched on (runNumber, subrunNumber, eventNumber[, mcEntryNumber]).
If those branches are missing, events are matched by entry index.

Exit status: 0 = parity, 1 = mismatch, 2 = usage or input error.

    python3 parity_check.py modes     debug.root production.root
    python3 parity_check.py reference v6_debug.root b2_reference.root [--atol 0]
                                      [--b2-reference] [--audited audited.txt]
"""
import argparse
import math
import sys

try:
    import awkward as ak
    import uproot
except ImportError:  # pragma: no cover
    sys.exit("parity_check.py needs uproot and awkward (pip install uproot awkward)")

TRIGGER_TREE = "phaseIITriggerTree"
META_TREE = "mrd_ml_metadata"
KEY_BRANCHES = ("runNumber", "subrunNumber", "eventNumber", "mcEntryNumber")

DECISION_BRANCHES = (
    "mrd_ml_failure_code", "mrd_ml_reco_skip_reason", "mrd_ml_gate_passed",
    "mrd_ml_reconstruction_available", "mrd_ml_reco_used_tank_bypass",
    "mrd_ml_topology_class", "mrd_ml_exit_face_class", "mrd_ml_event_category",
)
# Allowed to differ between a v6 file and an older reference: identity, schema,
# and status text (B2-era daemons used different version strings).
# These are release/publication identity fields rather than event physics.
# A reference build may legitimately carry older strings/schema metadata while
# still being a valid physics-value reference.
REFERENCE_IGNORED = {
    "mrd_ml_schema_version", "mrd_ml_output_mode", "mrd_ml_model_release",
    "mrd_ml_status_text", "mrd_ml_error_text", "mrd_ml_daemon_restart_count",
    "mrd_ml_protocol_version", "mrd_ml_gate_model_name",
    "mrd_ml_reconstruction_model_name", "mrd_ml_geometry_version",
    "mrd_ml_geometry_manifest_sha256", "mrd_ml_runtime_feature_version",
    "mrd_ml_daemon_version", "mrd_ml_tensorflow_version",
    "mrd_ml_gate_model_tree_sha256", "mrd_ml_reconstruction_model_tree_sha256",
    "mrd_ml_gate_trainer_version", "mrd_ml_reconstruction_trainer_version",
}
# Declared but never populated by the commissioned B2 adapter (repaired in v6).
B2_UNPOPULATED = {
    "mrd_ml_tank_mrd_association_applied", "mrd_ml_tank_mrd_association_found",
    "mrd_ml_tank_mrd_compatible_pair_count", "mrd_ml_tank_mrd_selected_tank_cluster_index",
    "mrd_ml_tank_mrd_selected_mrd_subevent_id", "mrd_ml_tank_mrd_expected_offset_ns",
    "mrd_ml_tank_mrd_coincidence_window_ns", "mrd_ml_tank_mrd_tank_mean_time_ns",
    "mrd_ml_tank_mrd_mrd_mean_time_ns", "mrd_ml_tank_mrd_dt_ns",
    "mrd_ml_tank_mrd_timing_residual_ns",
}


def load(path):
    f = uproot.open(path)
    if TRIGGER_TREE not in f:
        sys.exit(f"{path}: no {TRIGGER_TREE}")
    t = f[TRIGGER_TREE]
    names = [n for n in t.keys() if n.startswith("mrd_ml_")]
    keys = [k for k in KEY_BRANCHES if k in t.keys()]
    arrays = t.arrays(names + keys, library="ak")
    meta, meta_entries = None, 0
    if META_TREE in f:
        m = f[META_TREE].arrays(library="ak")
        meta_entries = len(m)
        meta = {k: _py(m[k][0]) for k in m.fields} if len(m) else {}
    return arrays, set(names), keys, meta, meta_entries


def require_one_metadata(label, entries):
    if entries != 1:
        print(f"FAIL {label}: expected exactly one {META_TREE} entry, found {entries}")
        return False
    return True


def read_audited(path):
    if not path:
        return set()
    out = set()
    for line in open(path):
        line = line.split("#", 1)[0].split()
        if line:
            out.add(tuple(int(x) for x in line))
    return out


def _py(v):
    v = ak.to_list(v) if isinstance(v, (ak.Array, ak.Record)) else v
    if isinstance(v, bytes):
        return v.decode()
    return v


def index(arrays, keys):
    if not keys:
        return {i: i for i in range(len(arrays))}
    cols = [ak.to_list(arrays[k]) for k in keys]
    out = {}
    for i, key in enumerate(zip(*cols)):
        if key in out:
            sys.exit(f"duplicate event key {dict(zip(keys, key))}; cannot align")
        out[key] = i
    return out


def equal(a, b, atol):
    if isinstance(a, list) or isinstance(b, list):
        return (isinstance(a, list) and isinstance(b, list) and len(a) == len(b)
                and all(equal(x, y, atol) for x, y in zip(a, b)))
    if isinstance(a, float) or isinstance(b, float):
        if isinstance(a, (int, float)) and isinstance(b, (int, float)):
            if math.isnan(a) and math.isnan(b):
                return True
            return abs(a - b) <= atol
        return False
    return a == b


def compare(a, b, branches, atol, ia, ib, label_a, label_b, max_report=5):
    shared_events = sorted(set(ia) & set(ib), key=str)
    failures = {}
    for br in sorted(branches):
        col_a, col_b = ak.to_list(a[br]), ak.to_list(b[br])
        bad = [ev for ev in shared_events
               if not equal(_py(col_a[ia[ev]]), _py(col_b[ib[ev]]), atol)]
        if bad:
            failures[br] = bad
    print(f"  events: {label_a}={len(ia)} {label_b}={len(ib)} matched={len(shared_events)}")
    if set(ia) ^ set(ib):
        only_a = sorted(set(ia) - set(ib), key=str)[:max_report]
        only_b = sorted(set(ib) - set(ia), key=str)[:max_report]
        print(f"  unmatched events: only-{label_a}={only_a} only-{label_b}={only_b}")
    for br, bad in failures.items():
        ev = bad[0]
        print(f"  MISMATCH {br}: {len(bad)} events, e.g. {ev}: "
              f"{label_a}={_py(ak.to_list(a[br])[ia[ev]])} {label_b}={_py(ak.to_list(b[br])[ib[ev]])}")
    return failures, set(ia) ^ set(ib)


# Same input, same release, two publication modes.  Every production branch
# must exist in debug and agree exactly; metadata must identify the same release
# apart from the intentional mode/branch-set labels.
def cmd_modes(args):
    dbg, dbg_names, dbg_keys, dbg_meta, dbg_n = load(args.debug)
    prod, prod_names, prod_keys, prod_meta, prod_n = load(args.production)
    ok = require_one_metadata("debug", dbg_n) & require_one_metadata("production", prod_n)
    missing = prod_names - dbg_names
    if missing:
        print(f"FAIL production branches absent from debug: {sorted(missing)}")
        ok = False
    print(f"branches: debug={len(dbg_names)} production={len(prod_names)} "
          f"shared={len(prod_names & dbg_names)}")
    keys = [k for k in dbg_keys if k in prod_keys]
    failures, unmatched = compare(dbg, prod, (prod_names & dbg_names) - {"mrd_ml_output_mode"},
                                  0.0, index(dbg, keys), index(prod, keys), "debug", "production")
    ok = ok and not failures and not unmatched
    if dbg_n == 1 and prod_n == 1:
        diff = {k for k in set(dbg_meta) | set(prod_meta)
                if k not in ("output_mode", "branch_set") and dbg_meta.get(k) != prod_meta.get(k)}
        if diff:
            print(f"FAIL metadata differs: { {k: (dbg_meta.get(k), prod_meta.get(k)) for k in diff} }")
            ok = False
        else:
            print(f"  metadata: identical apart from mode ({dbg_meta.get('output_mode')} vs "
                  f"{prod_meta.get('output_mode')}); release={prod_meta.get('model_release')}")
    print("PARITY OK" if ok else "PARITY FAILED")
    return 0 if ok else 1


# Compare the candidate against a known-good reconstruction file.  This is
# stricter than histogram closure: events are aligned by identity and compared
# value-by-value so a small but systematic software change cannot average away.
def cmd_reference(args):
    new, new_names, new_keys, new_meta, new_n = load(args.candidate)
    ref, ref_names, ref_keys, _, _ = load(args.reference)
    meta_ok = require_one_metadata("candidate", new_n)
    keys = [k for k in new_keys if k in ref_keys]
    ignored = set(REFERENCE_IGNORED)
    if args.b2_reference:
        skipped = sorted(B2_UNPOPULATED & new_names & ref_names)
        print(f"  --b2-reference: skipping {len(skipped)} association branches never filled in B2: {skipped}")
        ignored |= B2_UNPOPULATED
    shared = (new_names & ref_names) - ignored
    print(f"branches: candidate={len(new_names)} reference={len(ref_names)} compared={len(shared)}")
    only_ref = sorted((ref_names - new_names) - REFERENCE_IGNORED)
    if only_ref:
        print(f"  note: reference-only branches (not compared): {only_ref}")
    missing_decisions = [b for b in DECISION_BRANCHES if b in ref_names and b not in new_names]
    ia, ib = index(new, keys), index(ref, keys)
    audited = read_audited(args.audited)
    if audited:
        found = sorted(audited & set(ia) & set(ib))
        print(f"  audited events excluded from comparison ({len(found)} of {len(audited)} found):")
        for ev in found:
            diffs = {br: (_py(ak.to_list(new[br])[ia[ev]]), _py(ak.to_list(ref[br])[ib[ev]]))
                     for br in sorted(shared)
                     if not equal(_py(ak.to_list(new[br])[ia[ev]]), _py(ak.to_list(ref[br])[ib[ev]]), args.atol)}
            print(f"    {ev}: {diffs if diffs else 'no differences'}")
        missing = sorted(audited - (set(ia) & set(ib)))
        if missing:
            print(f"  note: audited events not present in both files: {missing}")
        ia = {k: v for k, v in ia.items() if k not in audited}
        ib = {k: v for k, v in ib.items() if k not in audited}
    failures, unmatched = compare(new, ref, shared, args.atol, ia, ib, "candidate", "reference")
    decision_fail = sorted(set(failures) & set(DECISION_BRANCHES))
    if decision_fail:
        print(f"FAIL event decisions differ: {decision_fail}")
    if missing_decisions:
        print(f"FAIL candidate lacks decision branches: {missing_decisions}")
    if new_meta:
        print(f"  candidate release: {new_meta.get('model_release')} "
              f"reco_tree={str(new_meta.get('reconstruction_model_tree_sha256'))[:12]}... "
              f"mode={new_meta.get('output_mode')}")
    ok = meta_ok and not failures and not unmatched and not missing_decisions
    print("PARITY OK" if ok else "PARITY FAILED")
    return 0 if ok else 1


def main():
    p = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    sub = p.add_subparsers(dest="cmd", required=True)
    m = sub.add_parser("modes", help="debug vs production from the same input")
    m.add_argument("debug")
    m.add_argument("production")
    r = sub.add_parser("reference", help="v6 candidate vs known-good reference")
    r.add_argument("candidate")
    r.add_argument("reference")
    r.add_argument("--atol", type=float, default=0.0,
                   help="absolute tolerance for floating-point physics branches (default exact)")
    r.add_argument("--b2-reference", action="store_true",
                   help="reference is a commissioned B2 file: skip the 11 association branches it never filled")
    r.add_argument("--audited", metavar="FILE",
                   help="events (run subrun event [mcEntry]) excluded from comparison but reported")
    args = p.parse_args()
    sys.exit(cmd_modes(args) if args.cmd == "modes" else cmd_reference(args))


if __name__ == "__main__":
    main()
