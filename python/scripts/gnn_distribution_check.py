"""
GNN Distribution Check

Programmatically checks whether candidate (inference) data falls within the
training (baseline) data distribution for every input and output feature.

3-tier classification per feature (percentile-based):
  WITHIN     — candidate value in baseline [p1, p99]
  BORDERLINE — candidate value in baseline [p0.1, p99.9] but outside [p1, p99]
  OUTSIDE    — candidate value outside baseline [p0.1, p99.9]

Reports per-feature pass/borderline/fail rates and an overall verdict.
Exit code: 0 = pass, 1 = any feature failing.
"""

import sys
from dataclasses import dataclass
from pathlib import Path
from typing import List, Optional, Tuple

import numpy as np

from gnn_data_distribution import (
    load_dataset,
    load_raw_dataset,
    _candidate_multiplier,
)

# -- Config -------------------------------------------------------------------

BASELINE_DIR  = r"D:\anim-ws\quad-experiments\quadruped-run-10\e2509_20260225_0\GNN\Data"
CANDIDATE_DIR = r"D:\anim-ws\MANN Eval Scenes\infernece-data-7x1m"
CANDIDATE_FORMAT = "gnn"  # "gnn" | "raw"

CHECK_TYPES: List[str] = ["input", "output"]  # which data types to check

INNER_PERCENTILES = (1, 99)      # comfortable range
OUTER_PERCENTILES = (0.1, 99.9)  # plausible range
FLAG_THRESHOLD    = 5.0           # % outside to flag a feature as FAILING

# Scale multipliers for candidate pos/vel features (unit conversion).
CANDIDATE_POS_MULTIPLIER = 1.0
CANDIDATE_VEL_MULTIPLIER = 1.0

# If non-empty, only check these labels. None = check all shared labels.
LABEL_FILTER: Optional[List[str]] = None

# -- Implementation -----------------------------------------------------------


@dataclass
class FeatureResult:
    index: int
    label: str
    pct_within: float
    pct_borderline: float
    pct_outside: float


def compute_bounds(
    data: np.ndarray,
    inner_pct: Tuple[float, float],
    outer_pct: Tuple[float, float],
) -> Tuple[np.ndarray, np.ndarray, np.ndarray, np.ndarray]:
    """Compute per-feature percentile bounds from baseline data.

    Returns (lo_inner, hi_inner, lo_outer, hi_outer), each shape (num_features,).
    """
    lo_inner = np.percentile(data, inner_pct[0], axis=0)
    hi_inner = np.percentile(data, inner_pct[1], axis=0)
    lo_outer = np.percentile(data, outer_pct[0], axis=0)
    hi_outer = np.percentile(data, outer_pct[1], axis=0)
    return lo_inner, hi_inner, lo_outer, hi_outer


def classify_features(
    baseline: np.ndarray,
    candidate: np.ndarray,
    labels: List[str],
    label_indices_b: dict,
    label_indices_c: dict,
) -> List[FeatureResult]:
    """Classify every shared feature into 3 tiers.

    Returns a list of FeatureResult, one per shared label.
    """
    lo_in, hi_in, lo_out, hi_out = compute_bounds(
        baseline, INNER_PERCENTILES, OUTER_PERCENTILES
    )

    results: List[FeatureResult] = []
    n_samples = candidate.shape[0]

    for lbl in labels:
        bi = label_indices_b[lbl]
        ci = label_indices_c[lbl]

        m = _candidate_multiplier(lbl)
        c_vals = candidate[:, ci] * m

        within = np.sum((c_vals >= lo_in[bi]) & (c_vals <= hi_in[bi]))
        in_outer = np.sum(
            ((c_vals >= lo_out[bi]) & (c_vals < lo_in[bi]))
            | ((c_vals > hi_in[bi]) & (c_vals <= hi_out[bi]))
        )
        outside = n_samples - within - in_outer

        results.append(FeatureResult(
            index=bi,
            label=lbl,
            pct_within=100.0 * within / n_samples,
            pct_borderline=100.0 * in_outer / n_samples,
            pct_outside=100.0 * outside / n_samples,
        ))

    return results


def print_report(results: List[FeatureResult], data_type: str) -> int:
    """Print a formatted report. Returns count of failing features."""
    failing = [r for r in results if r.pct_outside >= FLAG_THRESHOLD]
    borderline = [r for r in results if 0 < r.pct_outside < FLAG_THRESHOLD]
    passing = [r for r in results if r.pct_outside == 0]

    header = f"{data_type.upper()} CHECK ({len(results)} features)"
    print(f"\n{'=' * 60}")
    print(f"  {header}")
    print(f"{'=' * 60}")
    print(f"  + {len(passing)} features fully within distribution")
    print(f"  ~ {len(borderline)} features borderline (< {FLAG_THRESHOLD}% outside)")
    print(f"  X {len(failing)} features FAILING (>= {FLAG_THRESHOLD}% outside)")

    if failing:
        print(f"\n  FAILING features:")
        for r in sorted(failing, key=lambda r: -r.pct_outside):
            print(
                f"    [{r.index:>4}] {r.label:<30s} "
                f"-- {r.pct_outside:5.1f}% outside, {r.pct_borderline:5.1f}% borderline"
            )

    if borderline:
        print(f"\n  BORDERLINE features:")
        for r in sorted(borderline, key=lambda r: -r.pct_outside):
            print(
                f"    [{r.index:>4}] {r.label:<30s} "
                f"-- {r.pct_outside:5.1f}% outside, {r.pct_borderline:5.1f}% borderline"
            )

    return len(failing)


def resolve_shared_labels(
    b_labels: List[str],
    c_labels: List[str],
    label_filter: Optional[List[str]],
) -> Tuple[List[str], dict, dict]:
    """Find shared labels, apply filter, return (labels, b_idx_map, c_idx_map)."""
    b_idx = {lbl: i for i, lbl in enumerate(b_labels)}
    c_idx = {lbl: i for i, lbl in enumerate(c_labels)}
    shared = [lbl for lbl in b_labels if lbl in c_idx]

    if label_filter:
        missing = [lbl for lbl in label_filter if lbl not in b_idx or lbl not in c_idx]
        if missing:
            print(f"WARNING: labels not in both datasets: {missing}")
        shared = [lbl for lbl in label_filter if lbl in b_idx and lbl in c_idx]

    return shared, b_idx, c_idx


# -- Main --------------------------------------------------------------------

if __name__ == "__main__":
    total_failing = 0

    for dtype in CHECK_TYPES:
        print(f"\nLoading baseline ({dtype}): {BASELINE_DIR}")
        b_data, b_labels = load_dataset(BASELINE_DIR, dtype)

        print(f"Loading candidate ({dtype}) [{CANDIDATE_FORMAT}]: {CANDIDATE_DIR}")
        if CANDIDATE_FORMAT == "raw":
            c_data, c_labels = load_raw_dataset(CANDIDATE_DIR, dtype)
        else:
            c_data, c_labels = load_dataset(CANDIDATE_DIR, dtype)

        shared, b_idx, c_idx = resolve_shared_labels(b_labels, c_labels, LABEL_FILTER)
        if not shared:
            print(f"ERROR: no shared labels for {dtype}. Skipping.")
            continue

        results = classify_features(b_data, c_data, shared, b_idx, c_idx)
        total_failing += print_report(results, dtype)

    print(f"\n{'=' * 60}")
    if total_failing:
        print(f"  OVERALL: FAIL ({total_failing} feature(s) outside distribution)")
    else:
        print(f"  OVERALL: PASS")
    print(f"{'=' * 60}\n")

    sys.exit(1 if total_failing else 0)
