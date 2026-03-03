"""
GNN Input Data Analyzer

Loads two GNN Input.bin datasets (baseline + candidate) and supports
various comparison modes.

Data format:
  - Input.bin      : flat float32 array, shape from InputShape.txt
  - InputShape.txt : line 1 = num_samples, line 2 = num_features
  - InputLabels.txt: one line per feature, format "[idx] label_name"
"""

from __future__ import annotations

import struct
from pathlib import Path

import matplotlib.pyplot as plt
import numpy as np

# ── Config ────────────────────────────────────────────────────────────────────

# Unity
BASELINE_DIR  = r"D:\anim-ws\quad-experiments\quadruped-run-7\GNN data"
# AnimHost
CANDIDATE_DIR = r"D:\anim-ws\quad-experiments\quadruped-run-10\e2509_20260225_0\GNN\Data"

MODE = "coordinates"  # "coordinates" | "baseline_coordinates" | "candidate_coordinates" | "speed"

# ── Coordinate mode config ────────────────────────────────────────────────────
# Data index: 0-based index into the dataset's segments.
# The user figures out which baseline segment matches which candidate segment.
BASELINE_DATA_INDEX  = 0
CANDIDATE_DATA_INDEX = 0

# Row slicing (None = no limit). Applied after segment selection.
BASELINE_SLICE  = None # (60, 1060)   # (start, end) or None for all rows
CANDIDATE_SLICE = None # (0, 1000)   # (start, end) or None for all rows

# Labels for the 2D coordinates to plot (X, Y from each dataset)
BASELINE_X_LABEL  = "TrajectoryPosition7X"
BASELINE_Y_LABEL  = "TrajectoryPosition7Z"
CANDIDATE_X_LABEL = "root_pos_x_6"
CANDIDATE_Y_LABEL = "root_pos_y_6"

# ── Speed mode config ──────────────────────────────────────────────────────────
BASELINE_SPEED_LABEL  = "Actions7-3"
CANDIDATE_SPEED_LABEL = "root_speed_6"

# ──────────────────────────────────────────────────────────────────────────────


def _read_shape(directory: Path) -> tuple[int, int]:
    """Read (num_samples, num_features) from InputShape.txt."""
    text = (directory / "InputShape.txt").read_text(encoding="utf-8")
    lines = text.strip().splitlines()
    return int(lines[0]), int(lines[1])


def _read_labels(directory: Path) -> list[str]:
    """Read feature labels from InputLabels.txt.  Format: '[idx] label_name'."""
    labels: list[str] = []
    with open(directory / "InputLabels.txt", "r", encoding="utf-8") as f:
        for line in f:
            line = line.strip()
            if not line:
                continue
            # "[0] RootX" -> "RootX"
            if line.startswith("["):
                label = line.split("]", 1)[1].strip()
            else:
                label = line
            labels.append(label)
    return labels


def _read_binary(path: Path, num_samples: int, num_features: int) -> np.ndarray:
    """Read float32 Input.bin into (num_samples, num_features) array."""
    expected_bytes = num_samples * num_features * 4
    raw = path.read_bytes()
    if len(raw) < expected_bytes:
        raise EOFError(
            f"Input.bin too small: expected {expected_bytes} bytes "
            f"({num_samples}×{num_features}×4), got {len(raw)}"
        )
    return np.frombuffer(raw[:expected_bytes], dtype=np.float32).reshape(
        num_samples, num_features
    )


def _label_to_col(labels: list[str], name: str) -> int:
    """Find column index for a label name. Raises ValueError if not found."""
    for i, lbl in enumerate(labels):
        if lbl == name:
            return i
    raise ValueError(
        f"Label '{name}' not found. Available labels:\n"
        + "\n".join(f"  [{i}] {l}" for i, l in enumerate(labels))
    )


def load_dataset(directory: str) -> tuple[np.ndarray, list[str]]:
    """Load Input.bin + metadata from a GNN data directory.

    Returns (data array of shape (num_samples, num_features), labels list).
    """
    d = Path(directory)
    num_samples, num_features = _read_shape(d)
    print(f"  Shape: {num_samples} samples × {num_features} features")
    data = _read_binary(d / "Input.bin", num_samples, num_features)
    labels = _read_labels(d)
    if len(labels) != num_features:
        print(
            f"  WARNING: InputLabels.txt has {len(labels)} entries "
            f"but InputShape.txt says {num_features} features"
        )
    return data, labels


def _get_segment(data: np.ndarray, segment_index: int) -> np.ndarray:
    """Return rows for the given segment index.

    Currently treats the entire dataset as segment 0.
    Extend this if segment boundary information becomes available.
    """
    if segment_index != 0:
        raise ValueError(
            f"Segment index {segment_index} requested but only segment 0 "
            "is supported (whole dataset). Segment boundary detection not yet implemented."
        )
    return data


# ── Modes ─────────────────────────────────────────────────────────────────────


def _plot_trajectory(
    ax: plt.Axes,
    x: np.ndarray,
    y: np.ndarray,
    tag: str,
    color: str,
) -> None:
    """Plot a single 2D trajectory with start/end markers."""
    ax.plot(x, y, "o-", color=color, markersize=2, lw=0.8, alpha=0.8, label=tag)
    ax.plot(x[0], y[0], "s", color=color, markersize=8, label=f"{tag} start")
    ax.plot(x[-1], y[-1], "^", color=color, markersize=8, label=f"{tag} end")


def _print_coord_stats(tag: str, xs: np.ndarray, ys: np.ndarray) -> None:
    print(f"  {tag}:")
    print(f"    X  range [{xs.min():.4f}, {xs.max():.4f}]  mean {xs.mean():.4f}")
    print(f"    Y  range [{ys.min():.4f}, {ys.max():.4f}]  mean {ys.mean():.4f}")


def _extract_coords(
    directory: str,
    data_index: int,
    x_label: str,
    y_label: str,
    tag: str,
    row_slice: tuple[int, int] | None = None,
) -> tuple[np.ndarray, np.ndarray]:
    """Load dataset, resolve labels, extract segment, optionally slice rows, return (x, y) arrays."""
    print(f"Loading {tag:10}: {directory}")
    data, labels = load_dataset(directory)
    col_x = _label_to_col(labels, x_label)
    col_y = _label_to_col(labels, y_label)
    print(f"  {tag} X=[{col_x}] {x_label}  Y=[{col_y}] {y_label}")
    seg = _get_segment(data, data_index)
    if row_slice is not None:
        seg = seg[row_slice[0]:row_slice[1]]
        print(f"  {tag} : {len(seg)} frames (sliced [{row_slice[0]}:{row_slice[1]}])")
    else:
        print(f"  {tag} : {len(seg)} frames")
    return seg[:, col_x], seg[:, col_y]


def mode_coordinates(
    baseline_dir: str,
    candidate_dir: str,
    baseline_data_index: int,
    candidate_data_index: int,
    baseline_x_label: str,
    baseline_y_label: str,
    candidate_x_label: str,
    candidate_y_label: str,
    baseline_slice: tuple[int, int] | None = None,
    candidate_slice: tuple[int, int] | None = None,
) -> None:
    """Plot 2D coordinates from baseline and candidate for a given segment."""
    b_x, b_y = _extract_coords(baseline_dir, baseline_data_index, baseline_x_label, baseline_y_label, "baseline", baseline_slice)
    c_x, c_y = _extract_coords(candidate_dir, candidate_data_index, candidate_x_label, candidate_y_label, "candidate", candidate_slice)

    # Scale baseline coords to match candidate coordinate system
    b_x = b_x * 100
    b_y = b_y * 100

    print(f"\n── Coordinate Stats (baseline ×100) {'─' * 40}")
    _print_coord_stats("Baseline", b_x, b_y)
    _print_coord_stats("Candidate", c_x, c_y)

    fig, ax = plt.subplots(figsize=(10, 10))
    fig.suptitle("GNN Input — 2D Coordinate Comparison (baseline ×100)", fontsize=12)

    _plot_trajectory(ax, b_x, b_y, "baseline", "tab:blue")
    _plot_trajectory(ax, c_x, c_y, "candidate", "tab:orange")

    ax.set_xlabel(f"X  (B: {baseline_x_label} / C: {candidate_x_label})")
    ax.set_ylabel(f"Y  (B: {baseline_y_label} / C: {candidate_y_label})")
    ax.set_aspect("equal")
    ax.grid(True, alpha=0.25)
    ax.legend(fontsize=9)

    plt.tight_layout()
    plt.show()


def mode_baseline_coordinates(
    baseline_dir: str,
    baseline_data_index: int,
    baseline_x_label: str,
    baseline_y_label: str,
    baseline_slice: tuple[int, int] | None = None,
) -> None:
    """Plot 2D coordinates from baseline only."""
    b_x, b_y = _extract_coords(baseline_dir, baseline_data_index, baseline_x_label, baseline_y_label, "baseline", baseline_slice)

    print(f"\n── Coordinate Stats {'─' * 50}")
    _print_coord_stats("Baseline", b_x, b_y)

    fig, ax = plt.subplots(figsize=(10, 10))
    fig.suptitle("GNN Input — Baseline 2D Coordinates", fontsize=12)

    _plot_trajectory(ax, b_x, b_y, "baseline", "tab:blue")

    ax.set_xlabel(baseline_x_label)
    ax.set_ylabel(baseline_y_label)
    ax.set_aspect("equal")
    ax.grid(True, alpha=0.25)
    ax.legend(fontsize=9)

    plt.tight_layout()
    plt.show()


def mode_candidate_coordinates(
    candidate_dir: str,
    candidate_data_index: int,
    candidate_x_label: str,
    candidate_y_label: str,
    candidate_slice: tuple[int, int] | None = None,
) -> None:
    """Plot 2D coordinates from candidate only."""
    c_x, c_y = _extract_coords(candidate_dir, candidate_data_index, candidate_x_label, candidate_y_label, "candidate", candidate_slice)

    print(f"\n── Coordinate Stats {'─' * 50}")
    _print_coord_stats("Candidate", c_x, c_y)

    fig, ax = plt.subplots(figsize=(10, 10))
    fig.suptitle("GNN Input — Candidate 2D Coordinates", fontsize=12)

    _plot_trajectory(ax, c_x, c_y, "candidate", "tab:orange")

    ax.set_xlabel(candidate_x_label)
    ax.set_ylabel(candidate_y_label)
    ax.set_aspect("equal")
    ax.grid(True, alpha=0.25)
    ax.legend(fontsize=9)

    plt.tight_layout()
    plt.show()


def mode_speed(
    baseline_dir: str,
    candidate_dir: str,
    baseline_data_index: int,
    candidate_data_index: int,
    baseline_speed_label: str,
    candidate_speed_label: str,
    baseline_slice: tuple[int, int] | None = None,
    candidate_slice: tuple[int, int] | None = None,
) -> None:
    """Plot speed over sample index for baseline and candidate."""
    print(f"Loading baseline : {baseline_dir}")
    b_data, b_labels = load_dataset(baseline_dir)
    b_col = _label_to_col(b_labels, baseline_speed_label)
    print(f"  baseline speed=[{b_col}] {baseline_speed_label}")
    b_seg = _get_segment(b_data, baseline_data_index)
    if baseline_slice is not None:
        b_seg = b_seg[baseline_slice[0]:baseline_slice[1]]
        print(f"  baseline: {len(b_seg)} frames (sliced [{baseline_slice[0]}:{baseline_slice[1]}])")
    else:
        print(f"  baseline: {len(b_seg)} frames")
    b_speed = b_seg[:, b_col]

    print(f"Loading candidate: {candidate_dir}")
    c_data, c_labels = load_dataset(candidate_dir)
    c_col = _label_to_col(c_labels, candidate_speed_label)
    print(f"  candidate speed=[{c_col}] {candidate_speed_label}")
    c_seg = _get_segment(c_data, candidate_data_index)
    if candidate_slice is not None:
        c_seg = c_seg[candidate_slice[0]:candidate_slice[1]]
        print(f"  candidate: {len(c_seg)} frames (sliced [{candidate_slice[0]}:{candidate_slice[1]}])")
    else:
        print(f"  candidate: {len(c_seg)} frames")
    c_speed = c_seg[:, c_col]

    print(f"\n── Speed Stats {'─' * 50}")
    print(f"  Baseline  range [{b_speed.min():.4f}, {b_speed.max():.4f}]  mean {b_speed.mean():.4f}")
    print(f"  Candidate range [{c_speed.min():.4f}, {c_speed.max():.4f}]  mean {c_speed.mean():.4f}")

    fig, ax = plt.subplots(figsize=(14, 5))
    fig.suptitle("GNN Input — Speed Comparison", fontsize=12)

    ax.plot(b_speed, color="tab:blue",   lw=0.9, alpha=0.85, label=f"baseline  ({baseline_speed_label})")
    ax.plot(c_speed, color="tab:orange", lw=0.9, alpha=0.85, label=f"candidate ({candidate_speed_label})")

    ax.set_xlabel("Sample index")
    ax.set_ylabel("Speed")
    ax.grid(True, alpha=0.25)
    ax.legend(fontsize=9)

    plt.tight_layout()
    plt.show()


# ── Main ──────────────────────────────────────────────────────────────────────

if __name__ == "__main__":
    if MODE == "coordinates":
        mode_coordinates(
            BASELINE_DIR,
            CANDIDATE_DIR,
            BASELINE_DATA_INDEX,
            CANDIDATE_DATA_INDEX,
            BASELINE_X_LABEL,
            BASELINE_Y_LABEL,
            CANDIDATE_X_LABEL,
            CANDIDATE_Y_LABEL,
            BASELINE_SLICE,
            CANDIDATE_SLICE,
        )
    elif MODE == "baseline_coordinates":
        mode_baseline_coordinates(
            BASELINE_DIR,
            BASELINE_DATA_INDEX,
            BASELINE_X_LABEL,
            BASELINE_Y_LABEL,
            BASELINE_SLICE,
        )
    elif MODE == "candidate_coordinates":
        mode_candidate_coordinates(
            CANDIDATE_DIR,
            CANDIDATE_DATA_INDEX,
            CANDIDATE_X_LABEL,
            CANDIDATE_Y_LABEL,
            CANDIDATE_SLICE,
        )
    elif MODE == "speed":
        mode_speed(
            BASELINE_DIR,
            CANDIDATE_DIR,
            BASELINE_DATA_INDEX,
            CANDIDATE_DATA_INDEX,
            BASELINE_SPEED_LABEL,
            CANDIDATE_SPEED_LABEL,
            BASELINE_SLICE,
            CANDIDATE_SLICE,
        )
    else:
        print(f"Unknown mode: {MODE}")
