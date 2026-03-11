"""
GNN Data Distribution Plotter

Loads a GNN Input.bin or Output.bin dataset and plots the value distribution
(histogram) for every feature — one plot per feature.

Data format:
  - Input.bin       : flat float32 array, shape from InputShape.txt
  - InputShape.txt  : line 1 = num_samples, line 2 = num_features
  - InputLabels.txt : one line per feature, format "[idx] label_name"
  - Output.bin / OutputShape.txt / OutputLabels.txt — same layout

Config:
  DATA_DIR      — path to the GNN data directory
  DATA_TYPE     — "input" or "output"
  LABEL_FILTER  — list of label names to plot exclusively; None = plot all
  BINS          — number of histogram bins
"""

from pathlib import Path
from typing import List, Optional, Tuple

import matplotlib.pyplot as plt
import numpy as np

# ── Config ────────────────────────────────────────────────────────────────────

# COMPARE_MODE: when True, overlay baseline + candidate histograms on the same plots.
# When False, plot a single dataset as before.
COMPARE_MODE = True

# Single-dataset config (used when COMPARE_MODE = False)
DATA_DIR = r"D:\anim-ws\survivor-experiments\survivor-1\candidate\GNN Data"
DATA_TYPE = "output"  # "input" | "output"

# Compare-mode config (used when COMPARE_MODE = True)
BASELINE_DIR  = r"D:\anim-ws\quad-experiments\quadruped-run-10\e2509_20260225_0\GNN\Data"
CANDIDATE_DIR = r"D:\anim-ws\MANN Eval Scenes\Inference Data assim scale"
# BASELINE_DIR  = r"D:\anim-ws\survivor-experiments\survivor-1\candidate\GNN Data"
# CANDIDATE_DIR = r"D:\anim-ws\survivor-experiments\survivor-1\inference-data-straight"
COMPARE_DATA_TYPE = "input"  # "input" | "output" (applies to both)

# Format of the CANDIDATE directory:
#   "gnn" — preprocessed GNN data (Input/Output.bin + InputShape/Labels.txt)
#   "raw" — raw AnimHost export   (data_x/data_y.bin + metadata.txt + sequences_mann.txt)
CANDIDATE_FORMAT = "raw"  # "gnn" | "raw"

# If non-empty, only these labels are plotted. In compare mode, only labels
# present in LABEL_FILTER *and* both datasets are shown.
# LABEL_FILTER: Optional[List[str]] = [
#     "delta_x", "delta_y", "delta_angle",
# ]
# LABEL_FILTER: Optional[List[str]] = [
#     "out_root_pos_x_7",  "out_root_pos_y_7",
#     "out_root_pos_x_8",  "out_root_pos_y_8",
#     "out_root_pos_x_9",  "out_root_pos_y_9",
#     "out_root_pos_x_10", "out_root_pos_y_10",
#     "out_root_pos_x_11", "out_root_pos_y_11",
#     "out_root_pos_x_12", "out_root_pos_y_12",
# ]
# LABEL_FILTER: Optional[List[str]] = [
#     "out_jpos_x_Hips", "out_jpos_y_Hips", "out_jpos_z_Hips",
#     "out_jrot_0_Hips", "out_jrot_1_Hips", "out_jrot_2_Hips",
#     "out_jrot_3_Hips", "out_jrot_4_Hips", "out_jrot_5_Hips",
#     "out_jvel_x_Hips", "out_jvel_y_Hips", "out_jvel_z_Hips",
# ]
# LABEL_FILTER: Optional[List[str]] = [
#     "PhaseUpdate-1",  "PhaseUpdate-2",  "PhaseUpdate-3",  "PhaseUpdate-4",
#     "PhaseUpdate-5",  "PhaseUpdate-6",  "PhaseUpdate-7",  "PhaseUpdate-8",
#     "PhaseUpdate-9",  "PhaseUpdate-10", "PhaseUpdate-11", "PhaseUpdate-12",
# ]
# LABEL_FILTER: Optional[List[str]] = [
#     "out_root_speed_7",  "out_root_speed_8",  "out_root_speed_9",
#     "out_root_speed_10", "out_root_speed_11", "out_root_speed_12",
# ]

# LABEL_FILTER: Optional[List[str]] = [
#     "root_pos_x_7", "root_pos_y_7",
#     "root_fwd_x_7", "root_fwd_y_7",
#     "root_vel_x_7", "root_vel_y_7",
#     "root_speed_7",
# ]
# LABEL_FILTER: Optional[List[str]] = [
#     "root_speed_0",  "root_speed_1",  "root_speed_2",  "root_speed_3",
#     "root_speed_4",  "root_speed_5",  "root_speed_6",  "root_speed_7",
#     "root_speed_8",  "root_speed_9",  "root_speed_10", "root_speed_11",
#     "root_speed_12",
# ]
# LABEL_FILTER: Optional[List[str]] = [
#     "jpos_x_Hips", "jpos_y_Hips", "jpos_z_Hips",
#     "jrot_0_Hips", "jrot_1_Hips", "jrot_2_Hips",
#     "jrot_3_Hips", "jrot_4_Hips", "jrot_5_Hips",
#     "jvel_x_Hips", "jvel_y_Hips", "jvel_z_Hips",
# ]

BINS = 100

# Scale multipliers applied to *candidate* features whose label contains "pos" or "vel".
# Set to 1.0 to disable. Useful for unit conversion (e.g. normalised → metres).
CANDIDATE_POS_MULTIPLIER = 1.0
CANDIDATE_VEL_MULTIPLIER = 1.0

# ──────────────────────────────────────────────────────────────────────────────


def _candidate_multiplier(label: str) -> float:
    """Return CANDIDATE_POS_MULTIPLIER or CANDIDATE_VEL_MULTIPLIER if the label contains 'pos' or 'vel'."""
    ll = label.lower()
    if "pos" in ll:
        return CANDIDATE_POS_MULTIPLIER
    if "vel" in ll:
        return CANDIDATE_VEL_MULTIPLIER
    return 1.0


def _read_shape(directory: Path, prefix: str) -> Tuple[int, int]:
    text = (directory / f"{prefix}Shape.txt").read_text(encoding="utf-8")
    lines = text.strip().splitlines()
    return int(lines[0]), int(lines[1])


def _read_labels(directory: Path, prefix: str) -> List[str]:
    labels: List[str] = []
    with open(directory / f"{prefix}Labels.txt", "r", encoding="utf-8") as f:
        for line in f:
            line = line.strip()
            if not line:
                continue
            label = line.split("]", 1)[1].strip() if line.startswith("[") else line
            labels.append(label)
    return labels


def _read_binary(path: Path, num_samples: int, num_features: int) -> np.ndarray:
    expected_bytes = num_samples * num_features * 4
    raw = path.read_bytes()
    if len(raw) < expected_bytes:
        raise EOFError(
            f"{path.name} too small: expected {expected_bytes} bytes "
            f"({num_samples}×{num_features}×4), got {len(raw)}"
        )
    return np.frombuffer(raw[:expected_bytes], dtype=np.float32).reshape(
        num_samples, num_features
    )


def _count_lines(path: Path) -> int:
    """Return the number of non-empty lines in a text file."""
    with open(path, "r", encoding="utf-8") as f:
        return sum(1 for line in f if line.strip())


def load_raw_dataset(directory: str, data_type: str = "input") -> Tuple[np.ndarray, List[str]]:
    """Load data_x.bin or data_y.bin with labels from metadata.txt.

    metadata.txt format (comma-separated):
      row 0: num_input_features, label_0, label_1, ...
      row 1: num_output_features, label_0, label_1, ...

    Sample count is derived from sequences_mann.txt (one non-empty line per sample).
    """
    d = Path(directory)
    with open(d / "metadata.txt", "r", encoding="utf-8") as f:
        lines = [l.strip() for l in f if l.strip()]

    row_idx = 0 if data_type == "input" else 1
    parts = lines[row_idx].split(",")
    num_features = int(parts[0])
    labels = [p.strip() for p in parts[1 : num_features + 1]]

    num_samples = _count_lines(d / "sequences_mann.txt")
    print(f"  [{data_type}] Shape: {num_samples} samples × {num_features} features")

    bin_name = "data_x.bin" if data_type == "input" else "data_y.bin"
    data = _read_binary(d / bin_name, num_samples, num_features)

    if len(labels) != num_features:
        print(
            f"  WARNING: metadata.txt has {len(labels)} labels "
            f"but {num_features} features expected"
        )
    return data, labels


def load_dataset(directory: str, data_type: str = "input") -> Tuple[np.ndarray, List[str]]:
    """Load Input.bin or Output.bin depending on data_type ('input'|'output')."""
    d = Path(directory)
    prefix = "Input" if data_type == "input" else "Output"
    num_samples, num_features = _read_shape(d, prefix)
    print(f"  [{data_type}] Shape: {num_samples} samples × {num_features} features")
    data = _read_binary(d / f"{prefix}.bin", num_samples, num_features)
    labels = _read_labels(d, prefix)
    if len(labels) != num_features:
        print(
            f"  WARNING: {prefix}Labels.txt has {len(labels)} entries "
            f"but {prefix}Shape.txt says {num_features} features"
        )
    return data, labels


def plot_distributions(
    data: np.ndarray,
    labels: List[str],
    label_filter: Optional[List[str]],
    bins: int,
) -> None:
    if label_filter:
        missing = [l for l in label_filter if l not in labels]
        if missing:
            print(f"WARNING: labels not found in dataset: {missing}")
        indices = [(i, lbl) for i, lbl in enumerate(labels) if lbl in label_filter]
    else:
        indices = list(enumerate(labels))

    print(f"Plotting {len(indices)} feature(s)...")

    def _fill_ax(ax: plt.Axes, col_idx: int, label: str) -> None:
        values = data[:, col_idx]
        ax.hist(values, bins=bins, color="steelblue", edgecolor="none", alpha=0.85)
        ax.set_title(f"[{col_idx}] {label}", fontsize=9)
        stats = (
            f"min={values.min():.4g}  max={values.max():.4g}  "
            f"median={np.median(values):.4g}  std={values.std():.4g}"
        )
        ax.set_xlabel(stats, fontsize=8)
        ax.set_ylabel("Count", fontsize=8)
        ax.grid(True, alpha=0.25)

    if len(indices) <= 12:
        COLS = 4
        ROWS = 3
        fig, axes = plt.subplots(ROWS, COLS, figsize=(16, 9))
        axes_flat = axes.flatten()
        for plot_i, (col_idx, label) in enumerate(indices):
            _fill_ax(axes_flat[plot_i], col_idx, label)
        # hide unused subplots
        for ax in axes_flat[len(indices):]:
            ax.set_visible(False)
        plt.tight_layout()
        plt.show()
    else:
        for col_idx, label in indices:
            _, ax = plt.subplots(figsize=(8, 4))
            _fill_ax(ax, col_idx, label)
            plt.tight_layout()
            plt.show()


def plot_compare_distributions(
    b_data: np.ndarray,
    b_labels: List[str],
    c_data: np.ndarray,
    c_labels: List[str],
    label_filter: Optional[List[str]],
    bins: int,
) -> None:
    """Overlay baseline + candidate histograms on the same subplots.

    Labels must exist in both datasets. If label_filter is None, all shared
    labels are plotted.
    """
    b_label_idx = {lbl: i for i, lbl in enumerate(b_labels)}
    c_label_idx = {lbl: i for i, lbl in enumerate(c_labels)}
    shared = [lbl for lbl in b_labels if lbl in c_label_idx]

    if label_filter:
        missing = [lbl for lbl in label_filter if lbl not in b_label_idx or lbl not in c_label_idx]
        if missing:
            print(f"WARNING: labels not in both datasets: {missing}")
        plot_labels = [lbl for lbl in label_filter if lbl in b_label_idx and lbl in c_label_idx]
    else:
        plot_labels = shared

    if not plot_labels:
        print("ERROR: no shared labels to plot.")
        return

    print(f"Plotting {len(plot_labels)} feature(s)...")

    COLS = min(4, len(plot_labels))
    ROWS = (len(plot_labels) + COLS - 1) // COLS
    fig, axes = plt.subplots(ROWS, COLS, figsize=(5 * COLS, 4 * ROWS), squeeze=False)
    axes_flat = axes.flatten()

    for plot_i, lbl in enumerate(plot_labels):
        ax = axes_flat[plot_i]
        m = _candidate_multiplier(lbl)
        b_vals = b_data[:, b_label_idx[lbl]]
        c_vals = c_data[:, c_label_idx[lbl]] * m

        lo = min(b_vals.min(), c_vals.min())
        hi = max(b_vals.max(), c_vals.max())
        bin_edges = np.linspace(lo, hi, bins + 1)

        ax.hist(b_vals, bins=bin_edges, color="tab:blue",   edgecolor="none", alpha=0.55, label="baseline",  density=True)
        ax.hist(c_vals, bins=bin_edges, color="tab:orange", edgecolor="none", alpha=0.55, label="candidate", density=True)

        suffix = f"  (cand ×{m:g})" if m != 1.0 else ""
        ax.set_title(f"{lbl}{suffix}", fontsize=9)
        stats = (
            f"B: [{b_vals.min():.3g}, {b_vals.max():.3g}] μ={b_vals.mean():.3g}\n"
            f"C: [{c_vals.min():.3g}, {c_vals.max():.3g}] μ={c_vals.mean():.3g}"
        )
        ax.set_xlabel(stats, fontsize=7)
        ax.set_ylabel("Density", fontsize=8)
        ax.legend(fontsize=7)
        ax.grid(True, alpha=0.25)

    for ax in axes_flat[len(plot_labels):]:
        ax.set_visible(False)

    fig.suptitle("Distribution Comparison: Baseline vs Candidate", fontsize=12)
    plt.tight_layout()
    plt.show()


if __name__ == "__main__":
    if COMPARE_MODE:
        print(f"Loading baseline  ({COMPARE_DATA_TYPE}): {BASELINE_DIR}")
        b_data, b_labels = load_dataset(BASELINE_DIR, COMPARE_DATA_TYPE)
        print(f"Loading candidate ({COMPARE_DATA_TYPE}) [{CANDIDATE_FORMAT}]: {CANDIDATE_DIR}")
        if CANDIDATE_FORMAT == "raw":
            c_data, c_labels = load_raw_dataset(CANDIDATE_DIR, COMPARE_DATA_TYPE)
        else:
            c_data, c_labels = load_dataset(CANDIDATE_DIR, COMPARE_DATA_TYPE)
        plot_compare_distributions(b_data, b_labels, c_data, c_labels, LABEL_FILTER, BINS)
    else:
        if DATA_TYPE not in ("input", "output"):
            raise ValueError(f"DATA_TYPE must be 'input' or 'output', got '{DATA_TYPE}'")
        print(f"Loading ({DATA_TYPE}): {DATA_DIR}")
        data, labels = load_dataset(DATA_DIR, DATA_TYPE)
        plot_distributions(data, labels, LABEL_FILTER, BINS)
