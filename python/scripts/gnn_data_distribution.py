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

# Phase-aware modes — set one True to replace histogram plots with phase plots.
# Combine with COMPARE_MODE=True to overlay baseline vs candidate.
#   INPUT_PHASE_MODE  expects Input.bin  with 130 features (13 keys × 5 ch × 2)
#   OUTPUT_PHASE_MODE expects Output.bin with 140 features ( 7 keys × 5 ch × 4)
INPUT_PHASE_MODE  = False
OUTPUT_PHASE_MODE = False

# COMPARE_MODE: when True, overlay baseline + candidate histograms on the same plots.
# When False, plot a single dataset as before.
COMPARE_MODE = True

# Single-dataset config (used when COMPARE_MODE = False)
DATA_DIR = r"D:\anim-ws\survivor-experiments\survivor-1\candidate\GNN Data"
DATA_TYPE = "output"  # "input" | "output"

# Compare-mode config (used when COMPARE_MODE = True)
BASELINE_DIR  = r"D:\anim-ws\quad-experiments\quadruped-run-10\e2509_20260225_0\GNN\Data"   # AnimHost parity model data
CANDIDATE_DIR = r"D:\anim-ws\MANN Eval Scenes\infernece-data-7x1m"                       # AnimHost parity model inference
# CANDIDATE_DIR = r"D:\anim-ws\quad-experiments\quadruped-run-7\GNN data"                     # Unity parity model data
# BASELINE_DIR  = r"D:\anim-ws\survivor-experiments\survivor-1\candidate\GNN Data"          # AnimHost survivor latest data
# CANDIDATE_DIR = r"D:\anim-ws\survivor-experiments\survivor-1\infernece-data-7x1m"     # Animhost survivor inference
COMPARE_DATA_TYPE = "output"  # "input" | "output" (applies to both)

# Format of the CANDIDATE directory:
#   "gnn" — preprocessed GNN data (Input/Output.bin + InputShape/Labels.txt)
#   "raw" — raw AnimHost export   (data_x/data_y.bin + metadata.txt + sequences_mann.txt)
CANDIDATE_FORMAT = "gnn"  # "gnn" | "raw"

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
#     "PhaseUpdate-13",  "PhaseUpdate-14", "PhaseUpdate-15",
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
# LABEL_FILTER: Optional[List[str]] = [
#     "jpos_z_Spine1", "jpos_z_Tail", "jpos_z_Neck", "jpos_z_Head",
#     "root_speed_0", "jpos_z_RightShoulder", "root_speed_1", "jpos_z_LeftShoulder",
# ]
LABEL_FILTER: Optional[List[str]] = [
    "out_root_fwd_y_8", "out_root_fwd_y_9", "out_root_fwd_y_10",
    "out_root_fwd_y_7", "out_root_fwd_y_11", "out_root_fwd_y_12",
]



BINS = 100

# Scale multipliers applied to *candidate* features whose label contains "pos" or "vel".
# Set to 1.0 to disable. Useful for unit conversion (e.g. normalised → metres).
CANDIDATE_POS_MULTIPLIER = 1.0
CANDIDATE_VEL_MULTIPLIER = 1.0

# ──────────────────────────────────────────────────────────────────────────────

_N_CHANNELS   = 5
_MAX_SCATTER  = 20_000   # max points per series for scatter plots


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
            f"but {num_features} expected"
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


# ── Character dimensions ───────────────────────────────────────────────────────

def log_character_dimensions(
    data: np.ndarray,
    labels: List[str],
    tag: str = "",
) -> None:
    """Print median bounding-box extent (X/Y/Z) across all frames.

    Finds all jpos_x/y/z columns (input: jpos_*; output: out_jpos_*).
    Per frame: extent = max(joint) - min(joint) per axis.
    Reports median + p10/p90 across frames, with a unit hint.
    """
    label_idx = {lbl: i for i, lbl in enumerate(labels)}

    axes = {"x": [], "y": [], "z": []}
    for axis in axes:
        axes[axis] = [i for lbl, i in label_idx.items() if f"jpos_{axis}_" in lbl]

    if not any(axes.values()):
        return  # no jpos features in this dataset

    n_joints = len(axes["x"])
    result = {}
    for axis, cols in axes.items():
        if not cols:
            result[axis] = (float("nan"),) * 3
            continue
        vals = data[:, cols]                              # (N, J)
        extent = vals.max(axis=1) - vals.min(axis=1)     # (N,)
        result[axis] = (
            float(np.median(extent)),
            float(np.percentile(extent, 10)),
            float(np.percentile(extent, 90)),
        )

    unit_hint = " (~cm)" if result["y"][0] > 10 else " (~m)"
    prefix = f" [{tag}]" if tag else ""
    print(
        f"\n── Character dimensions{prefix} "
        f"(median bbox, N={data.shape[0]} frames, {n_joints} joints){unit_hint} ──"
    )
    for name, axis in [("X (lateral) ", "x"), ("Y (vertical)", "y"), ("Z (depth)   ", "z")]:
        med, p10, p90 = result[axis]
        print(f"  {name}: {med:6.2f}   [p10={p10:.2f}  p90={p90:.2f}]")
    print()


# ── Phase decode ───────────────────────────────────────────────────────────────

def decode_input_phase(data: np.ndarray) -> np.ndarray:
    """(N, 130) → (N, 13, 5, 2)   last dim = [x, y]

    Layout: key*10 + channel*2 + {0=x, 1=y}
    Amplitude is baked into the vector radius.
    """
    return data.reshape(data.shape[0], 13, _N_CHANNELS, 2)


def decode_output_phase(data: np.ndarray) -> np.ndarray:
    """(N, 140) → (N, 7, 5, 4)   last dim = [x, y, amp, freq]

    Per-key block of 20: [ch0_x ch0_y … ch4_y (10)] | [ch0…ch4 amp (5)] | [ch0…ch4 freq (5)]
    """
    N = data.shape[0]
    blocks   = data.reshape(N, 7, 20)
    phase2d  = blocks[:, :, :10].reshape(N, 7, _N_CHANNELS, 2)   # (N,7,5,2)
    amp      = blocks[:, :, 10:15][..., np.newaxis]               # (N,7,5,1)
    freq     = blocks[:, :, 15:20][..., np.newaxis]               # (N,7,5,1)
    return np.concatenate([phase2d, amp, freq], axis=-1)          # (N,7,5,4)


# ── Phase plot helpers ─────────────────────────────────────────────────────────

def _flat_ch(arr: np.ndarray, ch: int) -> np.ndarray:
    """Flatten (N, K, 5[, ...]) → 1-D array for channel ch."""
    return arr[:, :, ch].ravel()


def _subsample_xy(x: np.ndarray, y: np.ndarray) -> Tuple[np.ndarray, np.ndarray]:
    if len(x) > _MAX_SCATTER:
        idx = np.random.default_rng(42).choice(len(x), _MAX_SCATTER, replace=False)
        return x[idx], y[idx]
    return x, y


# ── Phase plots ────────────────────────────────────────────────────────────────

def plot_phase_scatter(
    b_xy: np.ndarray,
    c_xy: np.ndarray,
    title: str,
    compare: bool,
) -> None:
    """2D scatter / density per channel.

    b_xy, c_xy: (N, K, 5, 2)
    Single mode: hexbin density map.
    Compare mode: two subsampled scatter series (baseline blue, candidate orange).
    """
    fig, axes = plt.subplots(1, _N_CHANNELS, figsize=(20, 4))
    fig.suptitle(title)

    for ch, ax in enumerate(axes):
        bx = b_xy[:, :, ch, 0].ravel()
        by = b_xy[:, :, ch, 1].ravel()

        if compare:
            cx = c_xy[:, :, ch, 0].ravel()
            cy = c_xy[:, :, ch, 1].ravel()
            bx2, by2 = _subsample_xy(bx, by)
            cx2, cy2 = _subsample_xy(cx, cy)
            ax.scatter(bx2, by2, s=1, alpha=0.2, color="tab:blue",   label="baseline",  rasterized=True)
            ax.scatter(cx2, cy2, s=1, alpha=0.2, color="tab:orange", label="candidate", rasterized=True)
            if ch == 0:
                ax.legend(fontsize=7, markerscale=5)
        else:
            ax.hexbin(bx, by, gridsize=60, cmap="Blues", mincnt=1)

        ax.set_aspect("equal")
        ax.axhline(0, color="k", lw=0.4, alpha=0.4)
        ax.axvline(0, color="k", lw=0.4, alpha=0.4)
        ax.set_title(f"Ch {ch + 1}", fontsize=9)
        ax.set_xlabel("x", fontsize=8)
        ax.set_ylabel("y", fontsize=8)
        ax.grid(True, alpha=0.2)

    plt.tight_layout()


def plot_phase_polar(
    b_xy: np.ndarray,
    c_xy: np.ndarray,
    title: str,
    compare: bool,
) -> None:
    """Rose (polar histogram) of phase angle per channel.

    b_xy, c_xy: (N, K, 5, 2)
    """
    N_BINS  = 36
    edges   = np.linspace(0, 2 * np.pi, N_BINS + 1)
    width   = 2 * np.pi / N_BINS
    centers = (edges[:-1] + edges[1:]) / 2

    fig, axes = plt.subplots(1, _N_CHANNELS, figsize=(20, 4),
                             subplot_kw={"projection": "polar"})
    fig.suptitle(title)

    for ch, ax in enumerate(axes):
        bx = b_xy[:, :, ch, 0].ravel()
        by = b_xy[:, :, ch, 1].ravel()
        b_theta  = np.arctan2(by, bx) % (2 * np.pi)
        b_counts, _ = np.histogram(b_theta, bins=edges)
        b_counts = b_counts / b_counts.max()

        if compare:
            cx = c_xy[:, :, ch, 0].ravel()
            cy = c_xy[:, :, ch, 1].ravel()
            c_theta  = np.arctan2(cy, cx) % (2 * np.pi)
            c_counts, _ = np.histogram(c_theta, bins=edges)
            c_counts = c_counts / c_counts.max()
            ax.bar(centers,                b_counts, width=width * 0.45, color="tab:blue",   alpha=0.65, align="center", label="baseline")
            ax.bar(centers + width * 0.45, c_counts, width=width * 0.45, color="tab:orange", alpha=0.65, align="center", label="candidate")
        else:
            ax.bar(centers, b_counts, width=width, color="tab:blue", alpha=0.8, align="center")

        ax.set_title(f"Ch {ch + 1}", fontsize=9, pad=10)
        ax.set_yticks([])

    plt.tight_layout()


def plot_phase_amplitude(
    b_r: np.ndarray,
    c_r: np.ndarray,
    title: str,
    compare: bool,
) -> None:
    """Amplitude histogram per channel.

    b_r, c_r: (N, K, 5)  — pre-computed radii or explicit amp field.
    """
    fig, axes = plt.subplots(1, _N_CHANNELS, figsize=(20, 4))
    fig.suptitle(title)

    for ch, ax in enumerate(axes):
        br = _flat_ch(b_r, ch)
        if compare:
            cr = _flat_ch(c_r, ch)
            lo = min(br.min(), cr.min())
            hi = max(br.max(), cr.max())
            bins = np.linspace(lo, hi, BINS + 1)
            ax.hist(br, bins=bins, color="tab:blue",   alpha=0.55, density=True, label="baseline")
            ax.hist(cr, bins=bins, color="tab:orange", alpha=0.55, density=True, label="candidate")
            if ch == 0:
                ax.legend(fontsize=7)
        else:
            ax.hist(br, bins=BINS, color="steelblue", alpha=0.85)

        ax.set_title(f"Ch {ch + 1}", fontsize=9)
        ax.set_xlabel("amplitude", fontsize=8)
        ax.set_ylabel("density" if compare else "count", fontsize=8)
        ax.grid(True, alpha=0.25)

    plt.tight_layout()


def plot_phase_temporal(
    b_ph: np.ndarray,
    c_ph: np.ndarray,
    title: str,
    compare: bool,
) -> None:
    """Heatmap of mean amplitude over (keys × channels). Input phase only.

    b_ph, c_ph: (N, 13, 5, 2)
    """
    b_r = np.sqrt((b_ph ** 2).sum(-1)).mean(0)   # (13, 5)

    if compare:
        c_r   = np.sqrt((c_ph ** 2).sum(-1)).mean(0)
        vmin  = min(b_r.min(), c_r.min())
        vmax  = max(b_r.max(), c_r.max())
        fig, (ax_b, ax_c) = plt.subplots(1, 2, figsize=(14, 4))
        fig.suptitle(title)
        for ax, mat, lbl in [(ax_b, b_r, "baseline"), (ax_c, c_r, "candidate")]:
            im = ax.imshow(mat.T, aspect="auto", origin="lower", vmin=vmin, vmax=vmax, cmap="viridis")
            ax.set_title(lbl, fontsize=9)
            ax.set_xlabel("key offset  (0 = far past … 12 = far future)", fontsize=8)
            ax.set_ylabel("channel", fontsize=8)
            ax.set_yticks(range(_N_CHANNELS))
            ax.set_yticklabels([f"Ch {c + 1}" for c in range(_N_CHANNELS)], fontsize=8)
            fig.colorbar(im, ax=ax, label="mean amplitude")
    else:
        fig, ax = plt.subplots(figsize=(10, 3))
        fig.suptitle(title)
        im = ax.imshow(b_r.T, aspect="auto", origin="lower", cmap="viridis")
        ax.set_xlabel("key offset  (0 = far past … 12 = far future)", fontsize=8)
        ax.set_ylabel("channel", fontsize=8)
        ax.set_yticks(range(_N_CHANNELS))
        ax.set_yticklabels([f"Ch {c + 1}" for c in range(_N_CHANNELS)], fontsize=8)
        fig.colorbar(im, ax=ax, label="mean amplitude")

    plt.tight_layout()


def plot_phase_frequency(
    b_freq: np.ndarray,
    c_freq: np.ndarray,
    title: str,
    compare: bool,
) -> None:
    """Frequency histogram per channel.

    b_freq, c_freq: (N, K, 5)
    """
    fig, axes = plt.subplots(1, _N_CHANNELS, figsize=(20, 4))
    fig.suptitle(title)

    for ch, ax in enumerate(axes):
        bf = _flat_ch(b_freq, ch)
        if compare:
            cf = _flat_ch(c_freq, ch)
            lo = min(bf.min(), cf.min())
            hi = max(bf.max(), cf.max())
            bins = np.linspace(lo, hi, BINS + 1)
            ax.hist(bf, bins=bins, color="tab:blue",   alpha=0.55, density=True, label="baseline")
            ax.hist(cf, bins=bins, color="tab:orange", alpha=0.55, density=True, label="candidate")
            if ch == 0:
                ax.legend(fontsize=7)
        else:
            ax.hist(bf, bins=BINS, color="steelblue", alpha=0.85)

        ax.set_title(f"Ch {ch + 1}", fontsize=9)
        ax.set_xlabel("frequency", fontsize=8)
        ax.set_ylabel("density" if compare else "count", fontsize=8)
        ax.grid(True, alpha=0.25)

    plt.tight_layout()


# ── Histogram plots ────────────────────────────────────────────────────────────

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
    if INPUT_PHASE_MODE or OUTPUT_PHASE_MODE:
        dtype  = "input" if INPUT_PHASE_MODE else "output"
        decode = decode_input_phase if INPUT_PHASE_MODE else decode_output_phase
        mode   = "Input Phase" if INPUT_PHASE_MODE else "Output Phase"

        label_prefix = "PhaseSpace-" if INPUT_PHASE_MODE else "PhaseUpdate-"

        def _extract_phase(data: np.ndarray, labels: List[str]) -> np.ndarray:
            indices = [i for i, lbl in enumerate(labels) if lbl.startswith(label_prefix)]
            if not indices:
                raise ValueError(f"No labels starting with '{label_prefix}' found in dataset.")
            print(f"  Extracted {len(indices)} phase columns ({label_prefix}*)")
            return data[:, indices]

        if COMPARE_MODE:
            print(f"Loading baseline  ({dtype}): {BASELINE_DIR}")
            b_data, b_labels = load_dataset(BASELINE_DIR, dtype)
            log_character_dimensions(b_data, b_labels, "baseline")
            print(f"Loading candidate ({dtype}): {CANDIDATE_DIR}")
            c_data, c_labels = load_dataset(CANDIDATE_DIR, dtype)
            log_character_dimensions(c_data, c_labels, "candidate")
            b_ph = decode(_extract_phase(b_data, b_labels))
            c_ph = decode(_extract_phase(c_data, c_labels))
            suffix = "Baseline vs Candidate"
        else:
            print(f"Loading ({dtype}): {DATA_DIR}")
            data, labels = load_dataset(DATA_DIR, dtype)
            log_character_dimensions(data, labels)
            b_ph = c_ph = decode(_extract_phase(data, labels))
            suffix = DATA_DIR

        compare = COMPARE_MODE

        if INPUT_PHASE_MODE:
            b_r = np.sqrt((b_ph ** 2).sum(-1))   # (N, 13, 5)
            c_r = np.sqrt((c_ph ** 2).sum(-1))
            plot_phase_scatter(b_ph, c_ph,   f"{mode} Scatter — {suffix}", compare)
            plot_phase_polar(b_ph, c_ph,     f"{mode} Angle Distribution — {suffix}", compare)
            plot_phase_amplitude(b_r, c_r,   f"{mode} Amplitude — {suffix}", compare)
            plot_phase_temporal(b_ph, c_ph,  f"{mode} Temporal Amplitude — {suffix}", compare)
        else:
            b_xy   = b_ph[..., :2]          # (N, 7, 5, 2)
            c_xy   = c_ph[..., :2]
            b_amp  = b_ph[..., 2]           # (N, 7, 5)
            c_amp  = c_ph[..., 2]
            b_freq = b_ph[..., 3]           # (N, 7, 5)
            c_freq = c_ph[..., 3]
            plot_phase_scatter(b_xy, c_xy,     f"{mode} Scatter — {suffix}", compare)
            plot_phase_polar(b_xy, c_xy,       f"{mode} Angle Distribution — {suffix}", compare)
            plot_phase_amplitude(b_amp, c_amp, f"{mode} Amplitude — {suffix}", compare)
            plot_phase_frequency(b_freq, c_freq, f"{mode} Frequency — {suffix}", compare)

        plt.show()

    elif COMPARE_MODE:
        print(f"Loading baseline  ({COMPARE_DATA_TYPE}): {BASELINE_DIR}")
        b_data, b_labels = load_dataset(BASELINE_DIR, COMPARE_DATA_TYPE)
        log_character_dimensions(b_data, b_labels, "baseline")
        print(f"Loading candidate ({COMPARE_DATA_TYPE}) [{CANDIDATE_FORMAT}]: {CANDIDATE_DIR}")
        if CANDIDATE_FORMAT == "raw":
            c_data, c_labels = load_raw_dataset(CANDIDATE_DIR, COMPARE_DATA_TYPE)
        else:
            c_data, c_labels = load_dataset(CANDIDATE_DIR, COMPARE_DATA_TYPE)
        log_character_dimensions(c_data, c_labels, "candidate")
        plot_compare_distributions(b_data, b_labels, c_data, c_labels, LABEL_FILTER, BINS)
    else:
        if DATA_TYPE not in ("input", "output"):
            raise ValueError(f"DATA_TYPE must be 'input' or 'output', got '{DATA_TYPE}'")
        print(f"Loading ({DATA_TYPE}): {DATA_DIR}")
        data, labels = load_dataset(DATA_DIR, DATA_TYPE)
        log_character_dimensions(data, labels)
        plot_distributions(data, labels, LABEL_FILTER, BINS)
