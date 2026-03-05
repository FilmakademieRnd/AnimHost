"""
GNN ONNX Inference Script

Loads an ONNX-exported GNN model and runs inference on GNN Input.bin data,
then compares model predictions against ground-truth Output.bin.

Key facts:
  - Normalization is baked into the ONNX model (Xnorm/Ynorm as nn.Parameter,
    exported with export_params=True).  Pass raw Input.bin floats directly.
  - gating_indices / main_indices are also baked in as constants.
  - Separate ONNX files are required for baseline (559 features) and candidate
    (530 features) because the input dimensions differ.
  - "Target"    = Output.bin ground truth
  - "Inference" = ONNX model prediction for the same input row

Output label names (from OutputLabels.txt) are used for all config labels —
NOT the input labels used in analyze_gnn_data.py.

Modes:
  "coordinates"           : 3 plots — baseline target vs inference, candidate
                            target vs inference, per-frame position error.
  "baseline_coordinates"  : 2 plots — baseline target vs inference + error.
  "candidate_coordinates" : 2 plots — candidate target vs inference + error.
  "speed"                 : 3 plots — baseline target vs inference speed,
                            candidate target vs inference speed, speed error.

Dependency: pip install onnxruntime
"""

from __future__ import annotations

from pathlib import Path

import matplotlib.pyplot as plt
import numpy as np
import onnxruntime as ort

# ── Config ────────────────────────────────────────────────────────────────────

# Data directories (same as analyze_gnn_data.py)
BASELINE_DIR  = r"D:\anim-ws\quad-experiments\quadruped-run-7\GNN data"
CANDIDATE_DIR = r"D:\anim-ws\quad-experiments\quadruped-run-10\e2509_20260225_0\GNN\Data"
# CANDIDATE_DIR = r"D:\anim-ws\survivor-experiments\PR50-30pae-150gnn-nomirror-survivor25\GNN Data"

# ONNX model paths — separate files because input dims differ (559 vs 530)
BASELINE_ONNX_PATH  = r"D:\anim-ws\quad-experiments\quadruped-run-7\GNN training\r7-unity-nomirror-150.onnx"
CANDIDATE_ONNX_PATH = r"D:\anim-ws\quad-experiments\quadruped-run-10\r10-e2509_20260225_0-nomirror-150.onnx"
# CANDIDATE_ONNX_PATH = r"D:\anim-ws\survivor-experiments\PR50-30pae-150gnn-nomirror-survivor25\GNN Training\150.onnx"

MODE = "candidate_speed"  # "coordinates" | "baseline_coordinates" | "candidate_coordinates" | "speed" | "candidate_speed"

BASELINE_SLICE  = (60, 1060)   # (start, end) rows from Input/Output.bin, or None
CANDIDATE_SLICE = (5000, 5500)

# Scale applied to baseline OUTPUT positions before plotting (normalised → metres)
BASELINE_COORD_SCALE = 100.0

# ── Coordinate mode config ────────────────────────────────────────────────────
# Labels must be OUTPUT label names (from OutputLabels.txt).
# Baseline output has trajectory frames 8-13;  "TrajectoryPosition10X/Z" is
# the midpoint.  Candidate output prefixes positions with "out_".
BASELINE_X_LABEL  = "TrajectoryPosition8X"
BASELINE_Y_LABEL  = "TrajectoryPosition8Z"
CANDIDATE_X_LABEL = "out_root_pos_x_7"
CANDIDATE_Y_LABEL = "out_root_pos_y_7"

# ── Speed mode config ──────────────────────────────────────────────────────────
# Use single-column output labels that represent a speed-like scalar.
# Baseline: RootVelocityX is the only single-axis root speed in the output.
# Candidate: out_root_speed_7 is an explicit scalar speed prediction.
BASELINE_SPEED_LABEL  = "Actions8-3"
CANDIDATE_SPEED_LABEL = "out_root_speed_7"

# ── I/O helpers (mirrors analyze_gnn_data.py) ─────────────────────────────────


def _read_shape(directory: Path) -> tuple[int, int]:
    text = (directory / "InputShape.txt").read_text(encoding="utf-8")
    lines = text.strip().splitlines()
    return int(lines[0]), int(lines[1])


def _read_output_shape(directory: Path) -> tuple[int, int]:
    text = (directory / "OutputShape.txt").read_text(encoding="utf-8")
    lines = text.strip().splitlines()
    return int(lines[0]), int(lines[1])


def _read_labels(path: Path) -> list[str]:
    labels: list[str] = []
    with open(path, "r", encoding="utf-8") as f:
        for line in f:
            line = line.strip()
            if not line:
                continue
            label = line.split("]", 1)[1].strip() if line.startswith("[") else line
            labels.append(label)
    return labels


def _read_binary(path: Path, num_samples: int, num_features: int) -> np.ndarray:
    expected = num_samples * num_features * 4
    raw = path.read_bytes()
    if len(raw) < expected:
        raise EOFError(
            f"{path.name} too small: expected {expected} bytes "
            f"({num_samples}×{num_features}×4), got {len(raw)}"
        )
    return np.frombuffer(raw[:expected], dtype=np.float32).reshape(num_samples, num_features)


def _label_to_col(labels: list[str], name: str) -> int:
    for i, lbl in enumerate(labels):
        if lbl == name:
            return i
    raise ValueError(
        f"Label '{name}' not found. Available labels:\n"
        + "\n".join(f"  [{i}] {l}" for i, l in enumerate(labels))
    )


def load_input(directory: str) -> tuple[np.ndarray, list[str]]:
    d = Path(directory)
    n, f = _read_shape(d)
    print(f"  Input  shape: {n} samples × {f} features")
    data = _read_binary(d / "Input.bin", n, f)
    labels = _read_labels(d / "InputLabels.txt")
    return data, labels


def load_output(directory: str) -> tuple[np.ndarray, list[str]]:
    d = Path(directory)
    n, f = _read_output_shape(d)
    print(f"  Output shape: {n} samples × {f} features")
    data = _read_binary(d / "Output.bin", n, f)
    labels = _read_labels(d / "OutputLabels.txt")
    return data, labels


# ── ONNX inference ─────────────────────────────────────────────────────────────


def run_inference(onnx_path: str, input_data: np.ndarray) -> np.ndarray:
    """Run ONNX model on input_data (N, in_features) → (N, out_features).

    Normalization is baked into the model — pass raw Input.bin floats.
    Output name "Y" contains the motion predictions; "W" (gating weights)
    is ignored here.
    """
    sess = ort.InferenceSession(onnx_path, providers=["CPUExecutionProvider"])
    in_name  = sess.get_inputs()[0].name   # "X"
    out_name = "Y"

    # Verify input feature count
    expected_in = sess.get_inputs()[0].shape[1]
    if expected_in is not None and expected_in != input_data.shape[1]:
        raise ValueError(
            f"ONNX model expects {expected_in} input features but "
            f"data has {input_data.shape[1]}.  Wrong ONNX file for this dataset?"
        )

    print(f"  Running ONNX inference on {len(input_data)} frames …", end="", flush=True)
    x = input_data.astype(np.float32)
    try:
        # Attempt batch inference first (faster)
        result = sess.run([out_name], {in_name: x})[0]
    except Exception:
        # Fall back to row-by-row if batch dim is fixed at 1
        rows = [
            sess.run([out_name], {in_name: x[i : i + 1]})[0][0]
            for i in range(len(x))
        ]
        result = np.array(rows, dtype=np.float32)
    print(f" done → shape {result.shape}")
    return result


# ── Shared dataset pair loader ─────────────────────────────────────────────────


def _load_pair(
    directory: str,
    onnx_path: str,
    row_slice: tuple[int, int] | None,
    tag: str,
) -> tuple[np.ndarray, np.ndarray, list[str]]:
    """Load input+output, slice, run inference.

    Returns (target_data, inference_data, output_labels).
    """
    print(f"\nLoading {tag}: {directory}")
    in_data,  _       = load_input(directory)
    out_data, out_lbl = load_output(directory)

    if row_slice is not None:
        s, e = row_slice
        in_data  = in_data[s:e]
        out_data = out_data[s:e]
        print(f"  Sliced [{s}:{e}] → {len(in_data)} frames")
    else:
        print(f"  {len(in_data)} frames (no slice)")

    inference = run_inference(onnx_path, in_data)

    if len(out_lbl) != inference.shape[1]:
        print(
            f"  WARNING: OutputLabels.txt has {len(out_lbl)} entries "
            f"but inference produced {inference.shape[1]} features"
        )

    return out_data, inference, out_lbl


# ── Plot helpers ───────────────────────────────────────────────────────────────


def _plot_traj(
    ax: plt.Axes,
    x: np.ndarray,
    y: np.ndarray,
    label: str,
    color: str,
    linestyle: str = "-",
) -> None:
    ax.plot(x, y, "o", color=color, markersize=2, lw=0, alpha=0.5)
    ax.plot(x, y, color=color, lw=0.8, alpha=0.8, linestyle=linestyle, label=label)
    ax.plot(x[0], y[0], "s", color=color, markersize=7)
    ax.plot(x[-1], y[-1], "^", color=color, markersize=7)


def _pos_error(
    tx: np.ndarray,
    ty: np.ndarray,
    ix: np.ndarray,
    iy: np.ndarray,
) -> np.ndarray:
    return np.sqrt((tx - ix) ** 2 + (ty - iy) ** 2)


# ── Modes ─────────────────────────────────────────────────────────────────────


def mode_coordinates(
    baseline_dir: str,
    candidate_dir: str,
    baseline_onnx: str,
    candidate_onnx: str,
    baseline_x_label: str,
    baseline_y_label: str,
    candidate_x_label: str,
    candidate_y_label: str,
    baseline_coord_scale: float = 100.0,
    baseline_slice: tuple[int, int] | None = None,
    candidate_slice: tuple[int, int] | None = None,
) -> None:
    """3-subplot figure: baseline traj, candidate traj, per-frame position error."""
    b_tgt, b_inf, b_lbl = _load_pair(baseline_dir,  baseline_onnx,  baseline_slice,  "baseline")
    c_tgt, c_inf, c_lbl = _load_pair(candidate_dir, candidate_onnx, candidate_slice, "candidate")

    bx_col = _label_to_col(b_lbl, baseline_x_label)
    by_col = _label_to_col(b_lbl, baseline_y_label)
    cx_col = _label_to_col(c_lbl, candidate_x_label)
    cy_col = _label_to_col(c_lbl, candidate_y_label)

    btx = b_tgt[:, bx_col] * baseline_coord_scale
    bty = b_tgt[:, by_col] * baseline_coord_scale
    bix = b_inf[:, bx_col] * baseline_coord_scale
    biy = b_inf[:, by_col] * baseline_coord_scale

    ctx = c_tgt[:, cx_col]
    cty = c_tgt[:, cy_col]
    cix = c_inf[:, cx_col]
    ciy = c_inf[:, cy_col]

    b_err = _pos_error(btx, bty, bix, biy)
    c_err = _pos_error(ctx, cty, cix, ciy)

    print(f"\n── Coordinate Stats ─────────────────────────────────────────")
    print(f"  Baseline  pos error: mean {b_err.mean():.4f}  max {b_err.max():.4f}")
    print(f"  Candidate pos error: mean {c_err.mean():.4f}  max {c_err.max():.4f}")

    fig, axes = plt.subplots(1, 3, figsize=(21, 7))
    fig.suptitle("GNN Inference — 2D Trajectory: Target vs Inference", fontsize=12)

    ax = axes[0]
    _plot_traj(ax, btx, bty, "target",    "tab:blue",   "-")
    _plot_traj(ax, bix, biy, "inference", "tab:orange", "--")
    ax.set_title(f"Baseline  ({baseline_x_label})", fontsize=9, loc="left")
    ax.set_xlabel("X (m)"); ax.set_ylabel("Y (m)")
    ax.set_aspect("equal"); ax.grid(True, alpha=0.25); ax.legend(fontsize=8)

    ax = axes[1]
    _plot_traj(ax, ctx, cty, "target",    "tab:blue",   "-")
    _plot_traj(ax, cix, ciy, "inference", "tab:orange", "--")
    ax.set_title(f"Candidate  ({candidate_x_label})", fontsize=9, loc="left")
    ax.set_xlabel("X (m)"); ax.set_ylabel("Y (m)")
    ax.set_aspect("equal"); ax.grid(True, alpha=0.25); ax.legend(fontsize=8)

    ax = axes[2]
    ax.plot(b_err, color="tab:blue",   lw=0.8, alpha=0.85,
            label=f"baseline  (mean={b_err.mean():.3f})")
    ax.plot(c_err, color="tab:orange", lw=0.8, alpha=0.85,
            label=f"candidate (mean={c_err.mean():.3f})")
    ax.set_title("Position Error (Euclidean)", fontsize=9, loc="left")
    ax.set_xlabel("Frame"); ax.set_ylabel("Error (m)")
    ax.grid(True, alpha=0.25); ax.legend(fontsize=8)

    plt.tight_layout()
    plt.show()


def mode_baseline_coordinates(
    baseline_dir: str,
    baseline_onnx: str,
    baseline_x_label: str,
    baseline_y_label: str,
    baseline_coord_scale: float = 100.0,
    baseline_slice: tuple[int, int] | None = None,
) -> None:
    """2-subplot figure: baseline trajectory + per-frame error."""
    b_tgt, b_inf, b_lbl = _load_pair(baseline_dir, baseline_onnx, baseline_slice, "baseline")

    bx_col = _label_to_col(b_lbl, baseline_x_label)
    by_col = _label_to_col(b_lbl, baseline_y_label)

    btx = b_tgt[:, bx_col] * baseline_coord_scale
    bty = b_tgt[:, by_col] * baseline_coord_scale
    bix = b_inf[:, bx_col] * baseline_coord_scale
    biy = b_inf[:, by_col] * baseline_coord_scale
    b_err = _pos_error(btx, bty, bix, biy)

    print(f"\n  Baseline pos error: mean {b_err.mean():.4f}  max {b_err.max():.4f}")

    fig, axes = plt.subplots(1, 2, figsize=(16, 7))
    fig.suptitle("GNN Inference — Baseline Trajectory: Target vs Inference", fontsize=12)

    ax = axes[0]
    _plot_traj(ax, btx, bty, "target",    "tab:blue",   "-")
    _plot_traj(ax, bix, biy, "inference", "tab:orange", "--")
    ax.set_title(f"Baseline  ({baseline_x_label})", fontsize=9, loc="left")
    ax.set_xlabel("X (m)"); ax.set_ylabel("Y (m)")
    ax.set_aspect("equal"); ax.grid(True, alpha=0.25); ax.legend(fontsize=8)

    ax = axes[1]
    ax.plot(b_err, color="tab:blue", lw=0.8, alpha=0.85,
            label=f"mean={b_err.mean():.3f}")
    ax.set_title("Position Error (Euclidean)", fontsize=9, loc="left")
    ax.set_xlabel("Frame"); ax.set_ylabel("Error (m)")
    ax.grid(True, alpha=0.25); ax.legend(fontsize=8)

    plt.tight_layout()
    plt.show()


def mode_candidate_coordinates(
    candidate_dir: str,
    candidate_onnx: str,
    candidate_x_label: str,
    candidate_y_label: str,
    candidate_slice: tuple[int, int] | None = None,
) -> None:
    """2-subplot figure: candidate trajectory + per-frame error."""
    c_tgt, c_inf, c_lbl = _load_pair(candidate_dir, candidate_onnx, candidate_slice, "candidate")

    cx_col = _label_to_col(c_lbl, candidate_x_label)
    cy_col = _label_to_col(c_lbl, candidate_y_label)

    ctx = c_tgt[:, cx_col]
    cty = c_tgt[:, cy_col]
    cix = c_inf[:, cx_col]
    ciy = c_inf[:, cy_col]
    c_err = _pos_error(ctx, cty, cix, ciy)

    print(f"\n  Candidate pos error: mean {c_err.mean():.4f}  max {c_err.max():.4f}")

    fig, axes = plt.subplots(1, 2, figsize=(16, 7))
    fig.suptitle("GNN Inference — Candidate Trajectory: Target vs Inference", fontsize=12)

    ax = axes[0]
    _plot_traj(ax, ctx, cty, "target",    "tab:blue",   "-")
    _plot_traj(ax, cix, ciy, "inference", "tab:orange", "--")
    ax.set_title(f"Candidate  ({candidate_x_label})", fontsize=9, loc="left")
    ax.set_xlabel("X (m)"); ax.set_ylabel("Y (m)")
    ax.set_aspect("equal"); ax.grid(True, alpha=0.25); ax.legend(fontsize=8)

    ax = axes[1]
    ax.plot(c_err, color="tab:orange", lw=0.8, alpha=0.85,
            label=f"mean={c_err.mean():.3f}")
    ax.set_title("Position Error (Euclidean)", fontsize=9, loc="left")
    ax.set_xlabel("Frame"); ax.set_ylabel("Error (m)")
    ax.grid(True, alpha=0.25); ax.legend(fontsize=8)

    plt.tight_layout()
    plt.show()


def mode_candidate_speed(
    candidate_dir: str,
    candidate_onnx: str,
    candidate_speed_label: str,
    candidate_slice: tuple[int, int] | None = None,
) -> None:
    """2-subplot figure: candidate speed target vs inference + error."""
    c_tgt, c_inf, c_lbl = _load_pair(candidate_dir, candidate_onnx, candidate_slice, "candidate")

    cs_col = _label_to_col(c_lbl, candidate_speed_label)
    cts = c_tgt[:, cs_col]
    cis = c_inf[:, cs_col]
    c_err = np.abs(cts - cis)

    print(f"\n── Candidate Speed Stats ────────────────────────────────────")
    print(f"  Target  range [{cts.min():.4f}, {cts.max():.4f}]  mean {cts.mean():.4f}")
    print(f"  Infer   range [{cis.min():.4f}, {cis.max():.4f}]  mean {cis.mean():.4f}")
    print(f"  Error   mean {c_err.mean():.4f}  max {c_err.max():.4f}")

    fig, axes = plt.subplots(1, 2, figsize=(14, 5))
    fig.suptitle("GNN Inference — Candidate Speed: Target vs Inference", fontsize=12)

    ax = axes[0]
    ax.plot(cts, color="tab:blue",   lw=0.8, alpha=0.85, label="target")
    ax.plot(cis, color="tab:orange", lw=0.8, alpha=0.85, label="inference", linestyle="--")
    ax.set_title(f"Candidate  ({candidate_speed_label})", fontsize=9, loc="left")
    ax.set_xlabel("Frame"); ax.set_ylabel("Speed")
    ax.grid(True, alpha=0.25); ax.legend(fontsize=8)

    ax = axes[1]
    ax.plot(c_err, color="tab:orange", lw=0.8, alpha=0.85,
            label=f"mean={c_err.mean():.4f}")
    ax.set_title("Speed Error (|target − inference|)", fontsize=9, loc="left")
    ax.set_xlabel("Frame"); ax.set_ylabel("|Error|")
    ax.grid(True, alpha=0.25); ax.legend(fontsize=8)

    plt.tight_layout()
    plt.show()


def mode_speed(
    baseline_dir: str,
    candidate_dir: str,
    baseline_onnx: str,
    candidate_onnx: str,
    baseline_speed_label: str,
    candidate_speed_label: str,
    baseline_slice: tuple[int, int] | None = None,
    candidate_slice: tuple[int, int] | None = None,
) -> None:
    """3-subplot figure: baseline speed, candidate speed, speed error."""
    b_tgt, b_inf, b_lbl = _load_pair(baseline_dir,  baseline_onnx,  baseline_slice,  "baseline")
    c_tgt, c_inf, c_lbl = _load_pair(candidate_dir, candidate_onnx, candidate_slice, "candidate")

    bs_col = _label_to_col(b_lbl, baseline_speed_label)
    cs_col = _label_to_col(c_lbl, candidate_speed_label)

    bts = b_tgt[:, bs_col]
    bis = b_inf[:, bs_col]
    cts = c_tgt[:, cs_col]
    cis = c_inf[:, cs_col]

    b_err = np.abs(bts - bis)
    c_err = np.abs(cts - cis)

    print(f"\n── Speed Stats ──────────────────────────────────────────────")
    print(f"  Baseline  target  range [{bts.min():.4f}, {bts.max():.4f}]  mean {bts.mean():.4f}")
    print(f"  Baseline  infer   range [{bis.min():.4f}, {bis.max():.4f}]  mean {bis.mean():.4f}")
    print(f"  Candidate target  range [{cts.min():.4f}, {cts.max():.4f}]  mean {cts.mean():.4f}")
    print(f"  Candidate infer   range [{cis.min():.4f}, {cis.max():.4f}]  mean {cis.mean():.4f}")

    fig, axes = plt.subplots(1, 3, figsize=(21, 5))
    fig.suptitle("GNN Inference — Speed: Target vs Inference", fontsize=12)

    ax = axes[0]
    ax.plot(bts, color="tab:blue",   lw=0.8, alpha=0.85, label="target")
    ax.plot(bis, color="tab:orange", lw=0.8, alpha=0.85, label="inference", linestyle="--")
    ax.set_title(f"Baseline  ({baseline_speed_label})", fontsize=9, loc="left")
    ax.set_xlabel("Frame"); ax.set_ylabel("Speed")
    ax.grid(True, alpha=0.25); ax.legend(fontsize=8)

    ax = axes[1]
    ax.plot(cts, color="tab:blue",   lw=0.8, alpha=0.85, label="target")
    ax.plot(cis, color="tab:orange", lw=0.8, alpha=0.85, label="inference", linestyle="--")
    ax.set_title(f"Candidate  ({candidate_speed_label})", fontsize=9, loc="left")
    ax.set_xlabel("Frame"); ax.set_ylabel("Speed")
    ax.grid(True, alpha=0.25); ax.legend(fontsize=8)

    ax = axes[2]
    ax.plot(b_err, color="tab:blue",   lw=0.8, alpha=0.85,
            label=f"baseline  (mean={b_err.mean():.4f})")
    ax.plot(c_err, color="tab:orange", lw=0.8, alpha=0.85,
            label=f"candidate (mean={c_err.mean():.4f})")
    ax.set_title("Speed Error (|target − inference|)", fontsize=9, loc="left")
    ax.set_xlabel("Frame"); ax.set_ylabel("|Error|")
    ax.grid(True, alpha=0.25); ax.legend(fontsize=8)

    plt.tight_layout()
    plt.show()


# ── Main ──────────────────────────────────────────────────────────────────────

if __name__ == "__main__":
    if MODE == "coordinates":
        mode_coordinates(
            BASELINE_DIR, CANDIDATE_DIR,
            BASELINE_ONNX_PATH, CANDIDATE_ONNX_PATH,
            BASELINE_X_LABEL, BASELINE_Y_LABEL,
            CANDIDATE_X_LABEL, CANDIDATE_Y_LABEL,
            BASELINE_COORD_SCALE,
            BASELINE_SLICE, CANDIDATE_SLICE,
        )
    elif MODE == "baseline_coordinates":
        mode_baseline_coordinates(
            BASELINE_DIR, BASELINE_ONNX_PATH,
            BASELINE_X_LABEL, BASELINE_Y_LABEL,
            BASELINE_COORD_SCALE,
            BASELINE_SLICE,
        )
    elif MODE == "candidate_coordinates":
        mode_candidate_coordinates(
            CANDIDATE_DIR, CANDIDATE_ONNX_PATH,
            CANDIDATE_X_LABEL, CANDIDATE_Y_LABEL,
            CANDIDATE_SLICE,
        )
    elif MODE == "speed":
        mode_speed(
            BASELINE_DIR, CANDIDATE_DIR,
            BASELINE_ONNX_PATH, CANDIDATE_ONNX_PATH,
            BASELINE_SPEED_LABEL, CANDIDATE_SPEED_LABEL,
            BASELINE_SLICE, CANDIDATE_SLICE,
        )
    elif MODE == "candidate_speed":
        mode_candidate_speed(
            CANDIDATE_DIR, CANDIDATE_ONNX_PATH,
            CANDIDATE_SPEED_LABEL,
            CANDIDATE_SLICE,
        )
    else:
        print(f"Unknown mode: {MODE!r}")
