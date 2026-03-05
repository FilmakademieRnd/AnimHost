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

import math
import re
import struct
from pathlib import Path

import matplotlib.pyplot as plt
import numpy as np

# ── Config ────────────────────────────────────────────────────────────────────

# Unity
BASELINE_DIR  = r"D:\anim-ws\quad-experiments\quadruped-run-7\GNN data"
# AnimHost
CANDIDATE_DIR = r"D:\anim-ws\quad-experiments\quadruped-run-11\jaspe_20260305_3\GNN\Data"

MODE = "coordinates"  # "coordinates" | "baseline_coordinates" | "candidate_coordinates" | "speed" | "root_output" | "input_skeleton" | "input_skeleton_rot_check" | "input_skeleton_speed_check"

# ── Coordinate mode config ────────────────────────────────────────────────────
# Data index: 0-based index into the dataset's segments.
# The user figures out which baseline segment matches which candidate segment.
BASELINE_DATA_INDEX  = 0
CANDIDATE_DATA_INDEX = 0

# Row slicing (None = no limit). Applied after segment selection.
BASELINE_SLICE  = (60, 1060)   # (start, end) or None for all rows
CANDIDATE_SLICE = (0, 1000)   # (start, end) or None for all rows

# Labels for the 2D coordinates to plot (X, Y from each dataset)
BASELINE_X_LABEL  = "TrajectoryPosition7X"
BASELINE_Y_LABEL  = "TrajectoryPosition7Z"
CANDIDATE_X_LABEL = "root_pos_x_6"
CANDIDATE_Y_LABEL = "root_pos_y_6"

# ── Speed mode config ──────────────────────────────────────────────────────────
BASELINE_SPEED_LABEL  = "Actions7-3"
CANDIDATE_SPEED_LABEL = "root_speed_6"

# ── Root output mode config ────────────────────────────────────────────────────
# Baseline Output.bin lives in BASELINE_DIR alongside Input.bin.
# Each tuple: (baseline_label, candidate_label, row_title, b_scale, c_scale)
#   b_scale / c_scale are multiplied before plotting so both columns share the same unit.
#   Translation: baseline is in normalised units (×100 = metres), candidate already in metres.
#   Rotation:    baseline is in degrees, candidate is in radians → c_scale = 180/π → both degrees.
ROOT_OUTPUT_PAIRS = [
    ("RootUpdateX", "delta_x",     "X translation delta  (m)",   100.0,           1.0),
    ("RootUpdateY", "delta_angle", "Y rotation delta  (deg)",       1.0, 180.0/math.pi),
    ("RootUpdateZ", "delta_y",     "Z translation delta  (m)",   100.0,           1.0),
]

# ── Skeleton mode config ──────────────────────────────────────────────────────
# Each tuple: (baseline_prefix, candidate_bone_name)
#   baseline_prefix     : full prefix in baseline labels, e.g. "Bone1Hips"
#   candidate_bone_name : bone name used in candidate labels, e.g. "Hips"
# Baseline label pattern : {prefix}PositionX/Y/Z, {prefix}ForwardX/Y/Z,
#                          {prefix}UpX/Y/Z, {prefix}VelocityX/Y/Z
# Candidate label pattern: jpos_x/y/z_{bone}, jrot_0..5_{bone}, jvel_x/y/z_{bone}
SKELETON_BONES = [
    ("Bone18Head", "Head"),
]
SKEL_POS_B_SCALE = 100.0   # baseline position: normalised → metres
SKEL_VEL_B_SCALE = 1.0     # baseline velocity: same unit as candidate (both cm/s)

# ── Skeleton check mode config ────────────────────────────────────────────────
# Auto-discovers all bones from baseline labels. No manual SKELETON_BONES needed.
SKEL_CHECK_SPEED_ZERO_THRESH = 0.5   # baseline speed (cm/s) below which a bone is "zero-speed"

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


def _rot6d_to_matrix(a0: np.ndarray, a1: np.ndarray) -> np.ndarray:
    """Gram-Schmidt 6D → rotation matrix.

    a0, a1: (N, 3) — first two COLUMN vectors of the rotation matrix
                      (col0 = Right/X, col1 = Up/Y).
    Returns (N, 3, 3) with columns [Right, Up, Forward].
    Used for candidate (AnimHost/GLM convention).
    """
    b0 = a0 / np.linalg.norm(a0, axis=-1, keepdims=True)
    b1 = a1 - np.sum(a1 * b0, axis=-1, keepdims=True) * b0
    b1 = b1 / np.linalg.norm(b1, axis=-1, keepdims=True)
    b2 = np.cross(b0, b1)
    return np.stack([b0, b1, b2], axis=-1)  # (N, 3, 3)


def _fwd_up_to_matrix(fwd: np.ndarray, up: np.ndarray) -> np.ndarray:
    """Build rotation matrix from Forward (col2) + Up (col1) vectors.

    Baseline (Unity/AI4Animation) convention: Forward = local-Z (col2),
    Up = local-Y (col1).  Unity is LEFT-handed, so Right = Forward × Up
    (left-hand cross product rule, same as standard math formula but yields
    +X when Forward=+Z and Up=+Y in a left-handed frame).
    Returns (N, 3, 3) with columns [Right, Up, Forward] — same layout as
    _rot6d_to_matrix — so geodesic error is meaningful.
    """
    fwd_n = fwd / np.linalg.norm(fwd, axis=-1, keepdims=True)
    up_n  = up  / np.linalg.norm(up,  axis=-1, keepdims=True)
    right = np.cross(up_n, fwd_n)                              # col0 = Up × Forward → +X (right-hand)
    right = right / np.linalg.norm(right, axis=-1, keepdims=True)
    up_ortho = np.cross(fwd_n, right)                          # re-orthogonalise col1
    return np.stack([right, up_ortho, fwd_n], axis=-1)         # (N, 3, 3) = [Right, Up, Forward]


def _rotation_error_deg(Ra: np.ndarray, Rb: np.ndarray) -> np.ndarray:
    """Geodesic angular error in degrees between two batches of rotation matrices.

    Ra, Rb: (N, 3, 3). Returns (N,) error in degrees.
    """
    R_err = Ra.swapaxes(-2, -1) @ Rb  # R_a^T @ R_b  (N, 3, 3)
    trace = R_err[..., 0, 0] + R_err[..., 1, 1] + R_err[..., 2, 2]
    cos_angle = np.clip((trace - 1.0) / 2.0, -1.0, 1.0)
    return np.degrees(np.arccos(cos_angle))


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


def load_output_dataset(directory: str) -> tuple[np.ndarray, list[str]]:
    """Load Output.bin + metadata (OutputShape.txt, OutputLabels.txt)."""
    d = Path(directory)
    lines = (d / "OutputShape.txt").read_text(encoding="utf-8").strip().splitlines()
    num_samples, num_features = int(lines[0]), int(lines[1])
    print(f"  Shape: {num_samples} samples × {num_features} features")
    data = _read_binary(d / "Output.bin", num_samples, num_features)
    labels: list[str] = []
    with open(d / "OutputLabels.txt", encoding="utf-8") as f:
        for line in f:
            line = line.strip()
            if not line:
                continue
            label = line.split("]", 1)[1].strip() if line.startswith("[") else line
            labels.append(label)
    return data, labels


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


def mode_root_output(
    baseline_dir: str,
    candidate_dir: str,
    baseline_data_index: int,
    candidate_data_index: int,
    pairs: list[tuple[str, str, str, float, float]],
    baseline_slice: tuple[int, int] | None = None,
    candidate_slice: tuple[int, int] | None = None,
) -> None:
    """Overlay matched root-delta features from baseline and candidate outputs.

    Each pair (baseline_label, candidate_label, title, b_scale, c_scale) describes
    the same physical quantity.  b_scale / c_scale normalise both columns to the
    same unit before plotting so the curves can be compared directly.
    """
    print(f"Loading baseline output : {baseline_dir}")
    b_data, b_labels = load_output_dataset(baseline_dir)
    b_seg = _get_segment(b_data, baseline_data_index)
    if baseline_slice is not None:
        b_seg = b_seg[baseline_slice[0]:baseline_slice[1]]
        print(f"  baseline: {len(b_seg)} frames (sliced [{baseline_slice[0]}:{baseline_slice[1]}])")
    else:
        print(f"  baseline: {len(b_seg)} frames")

    print(f"Loading candidate output: {candidate_dir}")
    c_data, c_labels = load_output_dataset(candidate_dir)
    c_seg = _get_segment(c_data, candidate_data_index)
    if candidate_slice is not None:
        c_seg = c_seg[candidate_slice[0]:candidate_slice[1]]
        print(f"  candidate: {len(c_seg)} frames (sliced [{candidate_slice[0]}:{candidate_slice[1]}])")
    else:
        print(f"  candidate: {len(c_seg)} frames")

    print(f"\n── Root Output Stats {'─' * 40}")
    b_cols = [(_label_to_col(b_labels, b_lbl), b_lbl) for b_lbl, _, _, _, _ in pairs]
    c_cols = [(_label_to_col(c_labels, c_lbl), c_lbl) for _, c_lbl, _, _, _ in pairs]
    for (b_col, b_lbl), (c_col, c_lbl), (_, _, title, b_sc, c_sc) in zip(b_cols, c_cols, pairs):
        b_vals = b_seg[:, b_col] * b_sc
        c_vals = c_seg[:, c_col] * c_sc
        print(f"  {title}")
        print(f"    baseline  [{b_vals.min():+.4f}, {b_vals.max():+.4f}]  mean {b_vals.mean():+.4f}  ({b_lbl} ×{b_sc})")
        print(f"    candidate [{c_vals.min():+.4f}, {c_vals.max():+.4f}]  mean {c_vals.mean():+.4f}  ({c_lbl} ×{c_sc})")

    fig, axes = plt.subplots(len(pairs), 1, figsize=(14, 4 * len(pairs)), sharex=True)
    if len(pairs) == 1:
        axes = [axes]
    fig.suptitle("GNN Output — Root Delta: Baseline vs Candidate (unit-normalised)", fontsize=12)

    for ax, (b_col, b_lbl), (c_col, c_lbl), (_, _, title, b_sc, c_sc) in zip(axes, b_cols, c_cols, pairs):
        b_vals = b_seg[:, b_col] * b_sc
        c_vals = c_seg[:, c_col] * c_sc
        ax.plot(b_vals, color="tab:blue",   lw=0.8, alpha=0.85, label=f"baseline  ({b_lbl} ×{b_sc})")
        ax.plot(c_vals, color="tab:orange", lw=0.8, alpha=0.85, label=f"candidate ({c_lbl} ×{c_sc:.4g})")
        ax.set_title(title, fontsize=9, loc="left")
        ax.grid(True, alpha=0.25)
        ax.legend(fontsize=8)

    axes[-1].set_xlabel("Sample index")
    plt.tight_layout()
    plt.show()


def mode_input_skeleton(
    baseline_dir: str,
    candidate_dir: str,
    baseline_data_index: int,
    candidate_data_index: int,
    bones: list[tuple[str, str]],
    pos_b_scale: float = 100.0,
    vel_b_scale: float = 100.0,
    baseline_slice: tuple[int, int] | None = None,
    candidate_slice: tuple[int, int] | None = None,
) -> None:
    """Compare skeleton bone data (position, rotation, velocity) per bone.

    For each bone computes:
      - Position: Euclidean distance after scaling baseline to metres.
      - Rotation: geodesic angular error in degrees (Gram-Schmidt on both
                  baseline Forward+Up and candidate col0+col1).
      - Velocity: Euclidean distance after scaling baseline to m/s.

    Produces 3 figures (one per channel type), each with one subplot per bone.
    """
    print(f"Loading baseline : {baseline_dir}")
    b_data, b_labels = load_dataset(baseline_dir)
    b_seg = _get_segment(b_data, baseline_data_index)
    if baseline_slice is not None:
        b_seg = b_seg[baseline_slice[0]:baseline_slice[1]]
        print(f"  baseline: {len(b_seg)} frames (sliced [{baseline_slice[0]}:{baseline_slice[1]}])")
    else:
        print(f"  baseline: {len(b_seg)} frames")

    print(f"Loading candidate: {candidate_dir}")
    c_data, c_labels = load_dataset(candidate_dir)
    c_seg = _get_segment(c_data, candidate_data_index)
    if candidate_slice is not None:
        c_seg = c_seg[candidate_slice[0]:candidate_slice[1]]
        print(f"  candidate: {len(c_seg)} frames (sliced [{candidate_slice[0]}:{candidate_slice[1]}])")
    else:
        print(f"  candidate: {len(c_seg)} frames")

    n = min(len(b_seg), len(c_seg))
    if n < len(b_seg) or n < len(c_seg):
        print(f"  Trimming to {n} frames (min of both)")
    b_seg = b_seg[:n]
    c_seg = c_seg[:n]

    pos_errors: list[np.ndarray] = []
    rot_errors: list[np.ndarray] = []
    vel_errors: list[tuple[np.ndarray, np.ndarray]] = []
    b_pos_list: list[np.ndarray] = []
    c_pos_list: list[np.ndarray] = []
    bone_names: list[str] = []

    print(f"\n── Skeleton Stats (pos/vel baseline ×{pos_b_scale}) {'─' * 30}")
    for b_prefix, c_bone in bones:
        bone_names.append(c_bone)

        # ── Position ──────────────────────────────────────────────────────────
        b_pos = np.stack([
            b_seg[:, _label_to_col(b_labels, f"{b_prefix}PositionX")] * pos_b_scale,
            b_seg[:, _label_to_col(b_labels, f"{b_prefix}PositionY")] * pos_b_scale,
            b_seg[:, _label_to_col(b_labels, f"{b_prefix}PositionZ")] * pos_b_scale,
        ], axis=-1)
        c_pos = np.stack([
            c_seg[:, _label_to_col(c_labels, f"jpos_x_{c_bone}")],
            c_seg[:, _label_to_col(c_labels, f"jpos_y_{c_bone}")],
            c_seg[:, _label_to_col(c_labels, f"jpos_z_{c_bone}")],
        ], axis=-1)
        pos_err = np.linalg.norm(b_pos - c_pos, axis=-1)
        pos_errors.append(pos_err)
        b_pos_list.append(b_pos)
        c_pos_list.append(c_pos)

        # ── Rotation ──────────────────────────────────────────────────────────
        # Baseline: Forward (a0) + Up (a1)
        b_fwd = np.stack([
            b_seg[:, _label_to_col(b_labels, f"{b_prefix}ForwardX")],
            b_seg[:, _label_to_col(b_labels, f"{b_prefix}ForwardY")],
            b_seg[:, _label_to_col(b_labels, f"{b_prefix}ForwardZ")],
        ], axis=-1)
        b_up = np.stack([
            b_seg[:, _label_to_col(b_labels, f"{b_prefix}UpX")],
            b_seg[:, _label_to_col(b_labels, f"{b_prefix}UpY")],
            b_seg[:, _label_to_col(b_labels, f"{b_prefix}UpZ")],
        ], axis=-1)
        # Candidate: col0/right (a0) + col1/up (a1)
        c_col0 = np.stack([
            c_seg[:, _label_to_col(c_labels, f"jrot_0_{c_bone}")],
            c_seg[:, _label_to_col(c_labels, f"jrot_1_{c_bone}")],
            c_seg[:, _label_to_col(c_labels, f"jrot_2_{c_bone}")],
        ], axis=-1)
        c_col1 = np.stack([
            c_seg[:, _label_to_col(c_labels, f"jrot_3_{c_bone}")],
            c_seg[:, _label_to_col(c_labels, f"jrot_4_{c_bone}")],
            c_seg[:, _label_to_col(c_labels, f"jrot_5_{c_bone}")],
        ], axis=-1)
        Ra = _fwd_up_to_matrix(b_fwd, b_up)   # Forward=col2, Up=col1 → [Right, Up, Forward]
        Rc = _rot6d_to_matrix(c_col0, c_col1)  # col0=Right, col1=Up  → [Right, Up, Forward]
        rot_err = _rotation_error_deg(Ra, Rc)
        rot_errors.append(rot_err)

        # ── Velocity ──────────────────────────────────────────────────────────
        b_vel = np.stack([
            b_seg[:, _label_to_col(b_labels, f"{b_prefix}VelocityX")] * vel_b_scale,
            b_seg[:, _label_to_col(b_labels, f"{b_prefix}VelocityY")] * vel_b_scale,
            b_seg[:, _label_to_col(b_labels, f"{b_prefix}VelocityZ")] * vel_b_scale,
        ], axis=-1)
        c_vel = np.stack([
            c_seg[:, _label_to_col(c_labels, f"jvel_x_{c_bone}")],
            c_seg[:, _label_to_col(c_labels, f"jvel_y_{c_bone}")],
            c_seg[:, _label_to_col(c_labels, f"jvel_z_{c_bone}")],
        ], axis=-1)
        b_speed = np.linalg.norm(b_vel, axis=-1)
        c_speed = np.linalg.norm(c_vel, axis=-1)
        vel_errors.append((b_speed, c_speed))

        print(f"  {c_bone} ({b_prefix}):")
        print(f"    pos  (m)     mean {pos_err.mean():.4f}  max {pos_err.max():.4f}")
        print(f"    rot  (°)     mean {rot_err.mean():.4f}  max {rot_err.max():.4f}")
        print(f"    speed(B)     mean {b_speed.mean():.4f}  max {b_speed.max():.4f}")
        print(f"    speed(C)     mean {c_speed.mean():.4f}  max {c_speed.max():.4f}")

    # ── 1 figure: N_bones rows × 5 cols ──────────────────────────────────────
    # Cols 0-2: position X / Y / Z (baseline vs candidate)
    # Col 3:    rotation geodesic error
    # Col 4:    velocity Euclidean error
    n_bones = len(bones)
    fig, axes = plt.subplots(
        n_bones, 5,
        figsize=(22, 3 * n_bones),
        sharex=True,
        squeeze=False,
    )
    fig.suptitle("GNN Input — Skeleton Comparison: Baseline vs Candidate", fontsize=12)

    _POS_LABELS = ("X", "Y", "Z")
    _POS_COLORS = ("tab:red", "tab:green", "tab:blue")

    for row, ((b_prefix, c_bone), b_pos_arr, c_pos_arr, rot_err, (b_speed, c_speed)) in enumerate(
        zip(bones, b_pos_list, c_pos_list, rot_errors, vel_errors)
    ):
        name = c_bone

        # ── Cols 0-2: position X / Y / Z ─────────────────────────────────────
        for axis_i, (axis_name, color) in enumerate(zip(_POS_LABELS, _POS_COLORS)):
            ax = axes[row, axis_i]
            ax.plot(b_pos_arr[:, axis_i], color=color, lw=0.9, alpha=0.9, label="baseline")
            ax.plot(c_pos_arr[:, axis_i], color=color, lw=0.9, alpha=0.45,
                    linestyle="--", label="candidate")
            ax.set_title(f"{name} — pos {axis_name} (m)", fontsize=8, loc="left")
            ax.set_ylabel("m", fontsize=8)
            if axis_i == 0:
                ax.legend(fontsize=7)
            ax.grid(True, alpha=0.25)

        # ── Col 3: rotation error ─────────────────────────────────────────────
        ax = axes[row, 3]
        ax.plot(rot_err, color="tab:purple", lw=0.8, alpha=0.9)
        ax.set_title(
            f"{name} — Rotation Error\nmean={rot_err.mean():.2f}°  max={rot_err.max():.2f}°",
            fontsize=8, loc="left",
        )
        ax.set_ylabel("Geodesic angle (°)", fontsize=8)
        ax.grid(True, alpha=0.25)

        # ── Col 4: speed overlay ──────────────────────────────────────────────
        ax = axes[row, 4]
        ax.plot(b_speed, color="tab:blue",   lw=0.9, alpha=0.9, label="baseline")
        ax.plot(c_speed, color="tab:orange", lw=0.9, alpha=0.75, label="candidate")
        ax.set_title(
            f"{name} — Speed\nB mean={b_speed.mean():.3f}  C mean={c_speed.mean():.3f}",
            fontsize=8, loc="left",
        )
        ax.set_ylabel("Speed (cm/s)", fontsize=8)
        ax.legend(fontsize=7)
        ax.grid(True, alpha=0.25)

    for col in range(5):
        axes[-1, col].set_xlabel("Sample index")

    plt.tight_layout()
    plt.show()


def _load_and_trim(
    baseline_dir: str,
    candidate_dir: str,
    baseline_data_index: int,
    candidate_data_index: int,
    baseline_slice: tuple[int, int] | None,
    candidate_slice: tuple[int, int] | None,
) -> tuple[np.ndarray, list[str], np.ndarray, list[str], list[tuple[str, str]]]:
    """Shared setup for skeleton check modes: load, slice, trim, discover bones."""
    print(f"Loading baseline : {baseline_dir}")
    b_data, b_labels = load_dataset(baseline_dir)
    b_seg = _get_segment(b_data, baseline_data_index)
    if baseline_slice is not None:
        b_seg = b_seg[baseline_slice[0]:baseline_slice[1]]
        print(f"  baseline: {len(b_seg)} frames (sliced [{baseline_slice[0]}:{baseline_slice[1]}])")
    else:
        print(f"  baseline: {len(b_seg)} frames")

    print(f"Loading candidate: {candidate_dir}")
    c_data, c_labels = load_dataset(candidate_dir)
    c_seg = _get_segment(c_data, candidate_data_index)
    if candidate_slice is not None:
        c_seg = c_seg[candidate_slice[0]:candidate_slice[1]]
        print(f"  candidate: {len(c_seg)} frames (sliced [{candidate_slice[0]}:{candidate_slice[1]}])")
    else:
        print(f"  candidate: {len(c_seg)} frames")

    n = min(len(b_seg), len(c_seg))
    if n < len(b_seg) or n < len(c_seg):
        print(f"  Trimming to {n} frames (min of both)")
    b_seg = b_seg[:n]
    c_seg = c_seg[:n]

    bones: list[tuple[str, str]] = []
    for lbl in b_labels:
        m = re.match(r'(Bone\d+(\w+))PositionX$', lbl)
        if m:
            bones.append((m.group(1), m.group(2)))
    print(f"\n  Auto-discovered {len(bones)} bones from baseline.\n")

    return b_seg, b_labels, c_seg, c_labels, bones


def mode_input_skeleton_rot_check(
    baseline_dir: str,
    candidate_dir: str,
    baseline_data_index: int,
    candidate_data_index: int,
    baseline_slice: tuple[int, int] | None = None,
    candidate_slice: tuple[int, int] | None = None,
) -> None:
    """Auto-discover all bones and report geodesic rotation error per bone.

    Columns: Bone | Rot mean° | Rot min° | Rot max°
    Bones missing from the candidate are reported as errors and skipped.
    """
    b_seg, b_labels, c_seg, c_labels, bones = _load_and_trim(
        baseline_dir, candidate_dir,
        baseline_data_index, candidate_data_index,
        baseline_slice, candidate_slice,
    )

    W = 18
    header = (
        f"{'Bone':<{W}} "
        f"{'Rot mean°':>10} {'Rot min°':>9} {'Rot max°':>9}"
    )
    sep = "─" * len(header)
    print(header)
    print(sep)

    missing: list[str] = []
    for b_prefix, c_bone in bones:
        try:
            b_fwd = np.stack([
                b_seg[:, _label_to_col(b_labels, f"{b_prefix}ForwardX")],
                b_seg[:, _label_to_col(b_labels, f"{b_prefix}ForwardY")],
                b_seg[:, _label_to_col(b_labels, f"{b_prefix}ForwardZ")],
            ], axis=-1)
            b_up = np.stack([
                b_seg[:, _label_to_col(b_labels, f"{b_prefix}UpX")],
                b_seg[:, _label_to_col(b_labels, f"{b_prefix}UpY")],
                b_seg[:, _label_to_col(b_labels, f"{b_prefix}UpZ")],
            ], axis=-1)
            c_col0 = np.stack([
                c_seg[:, _label_to_col(c_labels, f"jrot_0_{c_bone}")],
                c_seg[:, _label_to_col(c_labels, f"jrot_1_{c_bone}")],
                c_seg[:, _label_to_col(c_labels, f"jrot_2_{c_bone}")],
            ], axis=-1)
            c_col1 = np.stack([
                c_seg[:, _label_to_col(c_labels, f"jrot_3_{c_bone}")],
                c_seg[:, _label_to_col(c_labels, f"jrot_4_{c_bone}")],
                c_seg[:, _label_to_col(c_labels, f"jrot_5_{c_bone}")],
            ], axis=-1)
        except ValueError:
            missing.append(c_bone)
            print(f"{'  ' + c_bone:<{W}} {'MISSING IN CANDIDATE':>10}")
            continue

        Ra = _fwd_up_to_matrix(b_fwd, b_up)
        Rc = _rot6d_to_matrix(c_col0, c_col1)
        rot_err = _rotation_error_deg(Ra, Rc)
        print(f"{c_bone:<{W}} {rot_err.mean():>10.2f} {rot_err.min():>9.2f} {rot_err.max():>9.2f}")

    print(sep)
    if missing:
        print(f"\n  Bones missing in candidate ({len(missing)}): {', '.join(missing)}")


def mode_input_skeleton_speed_check(
    baseline_dir: str,
    candidate_dir: str,
    baseline_data_index: int,
    candidate_data_index: int,
    vel_b_scale: float = 1.0,
    speed_zero_thresh: float = 0.5,
    baseline_slice: tuple[int, int] | None = None,
    candidate_slice: tuple[int, int] | None = None,
) -> None:
    """Auto-discover all bones and report speed stats per bone.

    Columns: Bone | B spd mean | B spd max | B zero? | C spd mean | C spd max
    B zero? is flagged YES when baseline max speed < speed_zero_thresh.
    Bones missing from the candidate are reported as errors and skipped.
    """
    b_seg, b_labels, c_seg, c_labels, bones = _load_and_trim(
        baseline_dir, candidate_dir,
        baseline_data_index, candidate_data_index,
        baseline_slice, candidate_slice,
    )

    W = 18
    header = (
        f"{'Bone':<{W}} "
        f"{'B spd mean':>10} {'B spd max':>9} {'B zero?':>7}  "
        f"{'C spd mean':>10} {'C spd max':>9}"
    )
    sep = "─" * len(header)
    print(header)
    print(sep)

    missing: list[str] = []
    for b_prefix, c_bone in bones:
        try:
            b_vel = np.stack([
                b_seg[:, _label_to_col(b_labels, f"{b_prefix}VelocityX")] * vel_b_scale,
                b_seg[:, _label_to_col(b_labels, f"{b_prefix}VelocityY")] * vel_b_scale,
                b_seg[:, _label_to_col(b_labels, f"{b_prefix}VelocityZ")] * vel_b_scale,
            ], axis=-1)
            c_vel = np.stack([
                c_seg[:, _label_to_col(c_labels, f"jvel_x_{c_bone}")],
                c_seg[:, _label_to_col(c_labels, f"jvel_y_{c_bone}")],
                c_seg[:, _label_to_col(c_labels, f"jvel_z_{c_bone}")],
            ], axis=-1)
        except ValueError:
            missing.append(c_bone)
            print(f"{'  ' + c_bone:<{W}} {'MISSING IN CANDIDATE':>10}")
            continue

        b_speed = np.linalg.norm(b_vel, axis=-1)
        c_speed = np.linalg.norm(c_vel, axis=-1)
        b_zero = b_speed.max() < speed_zero_thresh
        print(
            f"{c_bone:<{W}} "
            f"{b_speed.mean():>10.3f} {b_speed.max():>9.3f} {'YES':>7}  "
            f"{c_speed.mean():>10.3f} {c_speed.max():>9.3f}"
            if b_zero else
            f"{c_bone:<{W}} "
            f"{b_speed.mean():>10.3f} {b_speed.max():>9.3f} {'---':>7}  "
            f"{c_speed.mean():>10.3f} {c_speed.max():>9.3f}"
        )

    print(sep)
    if missing:
        print(f"\n  Bones missing in candidate ({len(missing)}): {', '.join(missing)}")


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
    elif MODE == "root_output":
        mode_root_output(
            BASELINE_DIR,
            CANDIDATE_DIR,
            BASELINE_DATA_INDEX,
            CANDIDATE_DATA_INDEX,
            ROOT_OUTPUT_PAIRS,
            BASELINE_SLICE,
            CANDIDATE_SLICE,
        )
    elif MODE == "input_skeleton":
        mode_input_skeleton(
            BASELINE_DIR,
            CANDIDATE_DIR,
            BASELINE_DATA_INDEX,
            CANDIDATE_DATA_INDEX,
            SKELETON_BONES,
            SKEL_POS_B_SCALE,
            SKEL_VEL_B_SCALE,
            BASELINE_SLICE,
            CANDIDATE_SLICE,
        )
    elif MODE == "input_skeleton_rot_check":
        mode_input_skeleton_rot_check(
            BASELINE_DIR,
            CANDIDATE_DIR,
            BASELINE_DATA_INDEX,
            CANDIDATE_DATA_INDEX,
            BASELINE_SLICE,
            CANDIDATE_SLICE,
        )
    elif MODE == "input_skeleton_speed_check":
        mode_input_skeleton_speed_check(
            BASELINE_DIR,
            CANDIDATE_DIR,
            BASELINE_DATA_INDEX,
            CANDIDATE_DATA_INDEX,
            SKEL_VEL_B_SCALE,
            SKEL_CHECK_SPEED_ZERO_THRESH,
            BASELINE_SLICE,
            CANDIDATE_SLICE,
        )
    else:
        print(f"Unknown mode: {MODE}")
