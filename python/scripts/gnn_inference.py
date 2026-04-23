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
BASELINE_DIR  = r"D:\anim-ws\survivor-experiments\survivor-1\baseline\GNN Data"
CANDIDATE_DIR = r"D:\anim-ws\quad-experiments\quadruped-run-7\GNN data"
# CANDIDATE_DIR = r"D:\anim-ws\survivor-experiments\PR50-30pae-150gnn-nomirror-survivor25\GNN Data"

# ONNX model paths — separate files because input dims differ (559 vs 530)
BASELINE_ONNX_PATH  = r"D:\anim-ws\quad-experiments\quadruped-run-10\e2509_20260225_0\GNN\Training\150.onnx"
CANDIDATE_ONNX_PATH = r"D:\anim-ws\quad-experiments\quadruped-run-7\GNN training\r7-unity-nomirror-150.onnx"
# CANDIDATE_ONNX_PATH = r"D:\anim-ws\survivor-experiments\PR50-30pae-150gnn-nomirror-survivor25\GNN Training\150.onnx"

MODE = "phase"  # "coordinates" | "baseline_coordinates" | "candidate_coordinates"
                      # "speed" | "candidate_speed" | "phase" | "input_phase"

BASELINE_SLICE  = (0, 1000)   # (start, end) rows from Input/Output.bin, or None
CANDIDATE_SLICE = (0, 1000)

# Scale applied to baseline OUTPUT positions before plotting (normalised → metres)
BASELINE_COORD_SCALE = 1.0

# ── Coordinate mode config ────────────────────────────────────────────────────
# Labels must be OUTPUT label names (from OutputLabels.txt).
# Baseline output has trajectory frames 8-13;  "TrajectoryPosition10X/Z" is
# the midpoint.  Candidate output prefixes positions with "out_".
# BASELINE_X_LABEL  = "TrajectoryPosition8X"
# BASELINE_Y_LABEL  = "TrajectoryPosition8Z"
BASELINE_X_LABEL  = "out_root_pos_x_7"
BASELINE_Y_LABEL  = "out_root_pos_y_7"
CANDIDATE_X_LABEL = "out_root_pos_x_7"
CANDIDATE_Y_LABEL = "out_root_pos_y_7"

# ── Speed mode config ──────────────────────────────────────────────────────────
# Use single-column output labels that represent a speed-like scalar.
# Baseline: RootVelocityX is the only single-axis root speed in the output.
# Candidate: out_root_speed_7 is an explicit scalar speed prediction.
# BASELINE_SPEED_LABEL  = "Actions8-3"
BASELINE_SPEED_LABEL  = "out_root_speed_7"
CANDIDATE_SPEED_LABEL = "out_root_speed_7"

# ── Phase config ──────────────────────────────────────────────────────────────
# Model training origin — controls output phase vector reordering.
# Unity:    interleaved (x, y, amp, freq) per channel per key
# AnimHost: grouped     [all phase2D | all amp | all freq] per key
BASELINE_MODEL_ORIGIN  = "animhost"   # "unity" | "animhost"
CANDIDATE_MODEL_ORIGIN = "animhost"

N_PHASE_CH          = 5
N_INPUT_PHASE_KEYS  = 13
N_OUTPUT_PHASE_KEYS = 7

PHASE_INPUT_PREFIX  = "PhaseSpace-"
PHASE_OUTPUT_PREFIX = "PhaseUpdate-"

BINS          = 100
_MAX_SCATTER  = 20_000   # max points per series for scatter plots

# Distribution check thresholds (same semantics as gnn_distribution_check.py)
PHASE_INNER_PCT = (1, 99)
PHASE_OUTER_PCT = (0.1, 99.9)
PHASE_FAIL_THR  = 5.0  # % outside to flag as FAILING

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


# ── Phase helpers ─────────────────────────────────────────────────────────────


def _find_label_range(labels: list[str], prefix: str) -> tuple[int, int]:
    """Return (start, end) column indices for labels starting with *prefix*."""
    indices = [i for i, l in enumerate(labels) if l.startswith(prefix)]
    if not indices:
        raise ValueError(
            f"No labels starting with '{prefix}'. "
            f"First 5: {labels[:5]} … ({len(labels)} total)"
        )
    return indices[0], indices[-1] + 1


def _unity_to_animhost_output_perm() -> list[int]:
    """Column permutation for ONE key: Unity interleaved → AnimHost grouped."""
    n = N_PHASE_CH
    perm: list[int] = []
    for c in range(n):                       # phase2D
        perm.extend([c * 4, c * 4 + 1])
    for c in range(n):                       # amplitude
        perm.append(c * 4 + 2)
    for c in range(n):                       # frequency
        perm.append(c * 4 + 3)
    return perm


def _reorder_output_phase(data: np.ndarray, origin: str) -> np.ndarray:
    """Reorder output phase columns to AnimHost (canonical) layout.

    data shape: (N, OUTPUT_PHASE_SIZE) — only the phase columns.
    """
    if origin == "animhost":
        return data
    vpk = N_PHASE_CH * 4
    key_perm = _unity_to_animhost_output_perm()
    full_perm: list[int] = []
    for k in range(N_OUTPUT_PHASE_KEYS):
        base = k * vpk
        full_perm.extend(base + p for p in key_perm)
    return data[:, full_perm]


def _decode_output_phase(data: np.ndarray) -> np.ndarray:
    """AnimHost-layout output phase (N, 140) → (N, 7, 5, 4).

    Last dim = [x, y, amplitude, frequency].
    Mirrors gnn_data_distribution.decode_output_phase.
    """
    N = data.shape[0]
    blocks  = data.reshape(N, N_OUTPUT_PHASE_KEYS, N_PHASE_CH * 4)
    phase2d = blocks[:, :, : N_PHASE_CH * 2].reshape(N, N_OUTPUT_PHASE_KEYS, N_PHASE_CH, 2)
    amp     = blocks[:, :, N_PHASE_CH * 2 : N_PHASE_CH * 3][..., np.newaxis]
    freq    = blocks[:, :, N_PHASE_CH * 3 :][..., np.newaxis]
    return np.concatenate([phase2d, amp, freq], axis=-1)


def _decode_input_phase(data: np.ndarray) -> np.ndarray:
    """Input phase (N, 130) → (N, 13, 5, 2)  last dim = [x, y]."""
    return data.reshape(data.shape[0], N_INPUT_PHASE_KEYS, N_PHASE_CH, 2)


def _pct_check(
    baseline_vals: np.ndarray,
    candidate_vals: np.ndarray,
) -> tuple[float, float, float]:
    """Percentile-based distribution check (like gnn_distribution_check.py).

    Returns (pct_within, pct_borderline, pct_outside).
    """
    lo_in  = np.percentile(baseline_vals, PHASE_INNER_PCT[0])
    hi_in  = np.percentile(baseline_vals, PHASE_INNER_PCT[1])
    lo_out = np.percentile(baseline_vals, PHASE_OUTER_PCT[0])
    hi_out = np.percentile(baseline_vals, PHASE_OUTER_PCT[1])

    n = len(candidate_vals)
    within = np.sum((candidate_vals >= lo_in) & (candidate_vals <= hi_in))
    in_outer = np.sum(
        ((candidate_vals >= lo_out) & (candidate_vals < lo_in))
        | ((candidate_vals > hi_in) & (candidate_vals <= hi_out))
    )
    outside = n - within - in_outer
    return 100.0 * within / n, 100.0 * in_outer / n, 100.0 * outside / n


def _subsample_xy(
    x: np.ndarray, y: np.ndarray,
) -> tuple[np.ndarray, np.ndarray]:
    if len(x) > _MAX_SCATTER:
        idx = np.random.default_rng(42).choice(len(x), _MAX_SCATTER, replace=False)
        return x[idx], y[idx]
    return x, y


def _flat_ch(arr: np.ndarray, ch: int) -> np.ndarray:
    """Flatten (N, K, 5[, ...]) → 1-D for channel *ch*."""
    return arr[:, :, ch].ravel()


# ── Phase plot helpers (mirrors gnn_data_distribution.py) ─────────────────────


def _plot_phase_scatter(
    a_xy: np.ndarray,
    b_xy: np.ndarray,
    title: str,
    label_a: str = "target",
    label_b: str = "inference",
) -> None:
    """2-D scatter per channel.  a_xy, b_xy: (N, K, 5, 2)."""
    fig, axes = plt.subplots(1, N_PHASE_CH, figsize=(20, 4))
    fig.suptitle(title)
    for ch, ax in enumerate(axes):
        ax1, ay1 = _subsample_xy(a_xy[:, :, ch, 0].ravel(), a_xy[:, :, ch, 1].ravel())
        bx1, by1 = _subsample_xy(b_xy[:, :, ch, 0].ravel(), b_xy[:, :, ch, 1].ravel())
        ax.scatter(ax1, ay1, s=1, alpha=0.2, color="tab:blue",   label=label_a, rasterized=True)
        ax.scatter(bx1, by1, s=1, alpha=0.2, color="tab:orange", label=label_b, rasterized=True)
        if ch == 0:
            ax.legend(fontsize=7, markerscale=5)
        ax.set_aspect("equal")
        ax.axhline(0, color="k", lw=0.4, alpha=0.4)
        ax.axvline(0, color="k", lw=0.4, alpha=0.4)
        ax.set_title(f"Ch {ch + 1}", fontsize=9)
        ax.set_xlabel("x", fontsize=8); ax.set_ylabel("y", fontsize=8)
        ax.grid(True, alpha=0.2)
    plt.tight_layout()


def _plot_phase_polar(
    a_xy: np.ndarray,
    b_xy: np.ndarray,
    title: str,
    label_a: str = "target",
    label_b: str = "inference",
) -> None:
    """Rose (polar histogram) of phase angle per channel.  a_xy, b_xy: (N, K, 5, 2)."""
    N_BINS_ = 36
    edges   = np.linspace(0, 2 * np.pi, N_BINS_ + 1)
    width   = 2 * np.pi / N_BINS_
    centers = (edges[:-1] + edges[1:]) / 2

    fig, axes = plt.subplots(1, N_PHASE_CH, figsize=(20, 4),
                             subplot_kw={"projection": "polar"})
    fig.suptitle(title)
    for ch, ax in enumerate(axes):
        a_theta = np.arctan2(a_xy[:, :, ch, 1].ravel(), a_xy[:, :, ch, 0].ravel()) % (2 * np.pi)
        b_theta = np.arctan2(b_xy[:, :, ch, 1].ravel(), b_xy[:, :, ch, 0].ravel()) % (2 * np.pi)
        a_cnt, _ = np.histogram(a_theta, bins=edges)
        b_cnt, _ = np.histogram(b_theta, bins=edges)
        a_cnt = a_cnt / max(a_cnt.max(), 1)
        b_cnt = b_cnt / max(b_cnt.max(), 1)
        ax.bar(centers,                a_cnt, width=width * 0.45, color="tab:blue",   alpha=0.65, align="center", label=label_a)
        ax.bar(centers + width * 0.45, b_cnt, width=width * 0.45, color="tab:orange", alpha=0.65, align="center", label=label_b)
        ax.set_title(f"Ch {ch + 1}", fontsize=9, pad=10)
        ax.set_yticks([])
    plt.tight_layout()


def _plot_phase_amplitude(
    a_r: np.ndarray,
    b_r: np.ndarray,
    title: str,
    label_a: str = "target",
    label_b: str = "inference",
) -> None:
    """Amplitude histogram per channel.  a_r, b_r: (N, K, 5)."""
    fig, axes = plt.subplots(1, N_PHASE_CH, figsize=(20, 4))
    fig.suptitle(title)
    for ch, ax in enumerate(axes):
        ar = _flat_ch(a_r, ch)
        br = _flat_ch(b_r, ch)
        lo = min(ar.min(), br.min())
        hi = max(ar.max(), br.max())
        bins = np.linspace(lo, hi, BINS + 1)
        ax.hist(ar, bins=bins, color="tab:blue",   alpha=0.55, density=True, label=label_a)
        ax.hist(br, bins=bins, color="tab:orange", alpha=0.55, density=True, label=label_b)
        if ch == 0:
            ax.legend(fontsize=7)
        ax.set_title(f"Ch {ch + 1}", fontsize=9)
        ax.set_xlabel("amplitude", fontsize=8); ax.set_ylabel("density", fontsize=8)
        ax.grid(True, alpha=0.25)
    plt.tight_layout()


def _plot_phase_frequency(
    a_freq: np.ndarray,
    b_freq: np.ndarray,
    title: str,
    label_a: str = "target",
    label_b: str = "inference",
) -> None:
    """Frequency histogram per channel.  a_freq, b_freq: (N, K, 5)."""
    fig, axes = plt.subplots(1, N_PHASE_CH, figsize=(20, 4))
    fig.suptitle(title)
    for ch, ax in enumerate(axes):
        af = _flat_ch(a_freq, ch)
        bf = _flat_ch(b_freq, ch)
        lo = min(af.min(), bf.min())
        hi = max(af.max(), bf.max())
        bins = np.linspace(lo, hi, BINS + 1)
        ax.hist(af, bins=bins, color="tab:blue",   alpha=0.55, density=True, label=label_a)
        ax.hist(bf, bins=bins, color="tab:orange", alpha=0.55, density=True, label=label_b)
        if ch == 0:
            ax.legend(fontsize=7)
        ax.set_title(f"Ch {ch + 1}", fontsize=9)
        ax.set_xlabel("frequency", fontsize=8); ax.set_ylabel("density", fontsize=8)
        ax.grid(True, alpha=0.25)
    plt.tight_layout()


def _plot_phase_temporal(
    a_ph: np.ndarray,
    b_ph: np.ndarray,
    title: str,
    label_a: str = "baseline",
    label_b: str = "candidate",
) -> None:
    """Heatmap of mean amplitude over (keys x channels).  Input phase only.

    a_ph, b_ph: (N, 13, 5, 2).
    """
    a_r = np.sqrt((a_ph ** 2).sum(-1)).mean(0)   # (13, 5)
    b_r = np.sqrt((b_ph ** 2).sum(-1)).mean(0)
    vmin = min(a_r.min(), b_r.min())
    vmax = max(a_r.max(), b_r.max())

    fig, (ax_a, ax_b) = plt.subplots(1, 2, figsize=(14, 4))
    fig.suptitle(title)
    for ax, mat, lbl in [(ax_a, a_r, label_a), (ax_b, b_r, label_b)]:
        im = ax.imshow(mat.T, aspect="auto", origin="lower", vmin=vmin, vmax=vmax, cmap="viridis")
        ax.set_title(lbl, fontsize=9)
        ax.set_xlabel("key offset  (0 = far past … 12 = far future)", fontsize=8)
        ax.set_ylabel("channel", fontsize=8)
        ax.set_yticks(range(N_PHASE_CH))
        ax.set_yticklabels([f"Ch {c + 1}" for c in range(N_PHASE_CH)], fontsize=8)
        fig.colorbar(im, ax=ax, label="mean amplitude")
    plt.tight_layout()


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


def _phase_plots_output(
    tgt_ph: np.ndarray,
    inf_ph: np.ndarray,
    tag: str,
) -> None:
    """Produce the full gnn_data_distribution-style output phase plot set.

    tgt_ph, inf_ph: (N, 7, 5, 4)  last dim = [x, y, amp, freq].
    """
    tgt_xy   = tgt_ph[..., :2]          # (N, 7, 5, 2)
    inf_xy   = inf_ph[..., :2]
    tgt_amp  = tgt_ph[..., 2]           # (N, 7, 5)
    inf_amp  = inf_ph[..., 2]
    tgt_freq = tgt_ph[..., 3]
    inf_freq = inf_ph[..., 3]

    _plot_phase_scatter(tgt_xy, inf_xy,
                        f"Output Phase Scatter — {tag}")
    _plot_phase_polar(tgt_xy, inf_xy,
                      f"Output Phase Angle Distribution — {tag}")
    _plot_phase_amplitude(tgt_amp, inf_amp,
                          f"Output Phase Amplitude — {tag}")
    _plot_phase_frequency(tgt_freq, inf_freq,
                          f"Output Phase Frequency — {tag}")


def _phase_plots_input(
    b_ph: np.ndarray,
    c_ph: np.ndarray,
) -> None:
    """Produce the full gnn_data_distribution-style input phase plot set.

    b_ph, c_ph: (N, 13, 5, 2)  last dim = [x, y].
    """
    b_r = np.sqrt((b_ph ** 2).sum(-1))   # (N, 13, 5)
    c_r = np.sqrt((c_ph ** 2).sum(-1))

    _plot_phase_scatter(b_ph, c_ph,
                        "Input Phase Scatter — Baseline vs Candidate",
                        label_a="baseline", label_b="candidate")
    _plot_phase_polar(b_ph, c_ph,
                      "Input Phase Angle Distribution — Baseline vs Candidate",
                      label_a="baseline", label_b="candidate")
    _plot_phase_amplitude(b_r, c_r,
                          "Input Phase Amplitude — Baseline vs Candidate",
                          label_a="baseline", label_b="candidate")
    _plot_phase_temporal(b_ph, c_ph,
                         "Input Phase Temporal Amplitude — Baseline vs Candidate")


def mode_phase(
    baseline_dir: str,
    candidate_dir: str,
    baseline_onnx: str,
    candidate_onnx: str,
    baseline_model_origin: str,
    candidate_model_origin: str,
    baseline_slice: tuple[int, int] | None = None,
    candidate_slice: tuple[int, int] | None = None,
) -> None:
    """Output phase: target vs inference — same plots as gnn_data_distribution.

    Produces scatter, polar, amplitude and frequency figures for each dataset.
    Reorders output phase to canonical AnimHost layout when needed.
    """
    b_tgt, b_inf, b_lbl = _load_pair(baseline_dir, baseline_onnx, baseline_slice, "baseline")
    c_tgt, c_inf, c_lbl = _load_pair(candidate_dir, candidate_onnx, candidate_slice, "candidate")

    # Locate output phase columns
    bs, be = _find_label_range(b_lbl, PHASE_OUTPUT_PREFIX)
    cs, ce = _find_label_range(c_lbl, PHASE_OUTPUT_PREFIX)
    print(f"  Baseline  output phase cols [{bs}:{be}] ({be - bs} features, {baseline_model_origin})")
    print(f"  Candidate output phase cols [{cs}:{ce}] ({ce - cs} features, {candidate_model_origin})")

    # Extract, reorder to canonical AnimHost layout, decode to (N, 7, 5, 4)
    b_tgt_ph = _decode_output_phase(_reorder_output_phase(b_tgt[:, bs:be], baseline_model_origin))
    b_inf_ph = _decode_output_phase(_reorder_output_phase(b_inf[:, bs:be], baseline_model_origin))
    c_tgt_ph = _decode_output_phase(_reorder_output_phase(c_tgt[:, cs:ce], candidate_model_origin))
    c_inf_ph = _decode_output_phase(_reorder_output_phase(c_inf[:, cs:ce], candidate_model_origin))

    # Print per-component, per-channel error stats
    print(f"\n── Output Phase Error (target vs inference) ─────────────────")
    for name, idx_or_fn in [("phase2d", "xy"), ("amplitude", 2), ("frequency", 3)]:
        if name == "phase2d":
            b_e = np.sqrt(((b_tgt_ph[..., :2] - b_inf_ph[..., :2]) ** 2).sum(axis=-1))
            c_e = np.sqrt(((c_tgt_ph[..., :2] - c_inf_ph[..., :2]) ** 2).sum(axis=-1))
        else:
            b_e = np.abs(b_tgt_ph[..., idx_or_fn] - b_inf_ph[..., idx_or_fn])
            c_e = np.abs(c_tgt_ph[..., idx_or_fn] - c_inf_ph[..., idx_or_fn])
        print(f"  {name:>10s}  baseline mean={b_e.mean():.4f}  candidate mean={c_e.mean():.4f}")
        for ch in range(N_PHASE_CH):
            print(f"    ch{ch}  baseline={b_e[:, :, ch].mean():.4f}  candidate={c_e[:, :, ch].mean():.4f}")

    # Phase plots — 4 figures per dataset (scatter, polar, amplitude, frequency)
    _phase_plots_output(b_tgt_ph, b_inf_ph, "Baseline: Target vs Inference")
    _phase_plots_output(c_tgt_ph, c_inf_ph, "Candidate: Target vs Inference")
    plt.show()


def mode_input_phase(
    baseline_dir: str,
    candidate_dir: str,
    baseline_slice: tuple[int, int] | None = None,
    candidate_slice: tuple[int, int] | None = None,
) -> None:
    """Input phase distribution: baseline vs candidate.

    Same plots as gnn_data_distribution INPUT_PHASE_MODE + COMPARE_MODE:
    scatter, polar, amplitude histogram, temporal heatmap.
    Also prints a percentile-based distribution check per channel.
    No ONNX inference needed.
    """
    print("\nLoading baseline input …")
    b_in, b_in_lbl = load_input(baseline_dir)
    print("Loading candidate input …")
    c_in, c_in_lbl = load_input(candidate_dir)

    if baseline_slice is not None:
        b_in = b_in[baseline_slice[0]:baseline_slice[1]]
        print(f"  Baseline  sliced [{baseline_slice[0]}:{baseline_slice[1]}]")
    if candidate_slice is not None:
        c_in = c_in[candidate_slice[0]:candidate_slice[1]]
        print(f"  Candidate sliced [{candidate_slice[0]}:{candidate_slice[1]}]")

    # Locate input phase columns (layout is identical for Unity and AnimHost)
    bs, be = _find_label_range(b_in_lbl, PHASE_INPUT_PREFIX)
    cs, ce = _find_label_range(c_in_lbl, PHASE_INPUT_PREFIX)
    print(f"  Baseline  input phase cols [{bs}:{be}] ({be - bs} features)")
    print(f"  Candidate input phase cols [{cs}:{ce}] ({ce - cs} features)")

    b_phase = _decode_input_phase(b_in[:, bs:be])  # (N, 13, 5, 2)
    c_phase = _decode_input_phase(c_in[:, cs:ce])

    # Percentile-based distribution check per channel
    b_amp = np.sqrt((b_phase ** 2).sum(axis=-1))  # (N, 13, 5)
    c_amp = np.sqrt((c_phase ** 2).sum(axis=-1))

    n_fail = 0
    print(f"\n── Input Phase Distribution Check ──────────────────────────")
    print(f"  {'ch':>4s}  {'status':>4s}  {'within':>10s}  {'border':>10s}  {'outside':>10s}  "
          f"{'baseline µ±σ':>16s}  {'candidate µ±σ':>16s}")
    for ch in range(N_PHASE_CH):
        b_vals = b_amp[:, :, ch].ravel()
        c_vals = c_amp[:, :, ch].ravel()
        within, border, outside = _pct_check(b_vals, c_vals)
        status = "FAIL" if outside >= PHASE_FAIL_THR else ("WARN" if outside > 0 else "PASS")
        if outside >= PHASE_FAIL_THR:
            n_fail += 1
        print(
            f"  ch{ch}   {status:>4s}  {within:9.1f}%  {border:9.1f}%  {outside:9.1f}%  "
            f"{b_vals.mean():7.3f}±{b_vals.std():<6.3f}  "
            f"{c_vals.mean():7.3f}±{c_vals.std():<6.3f}"
        )

    print(f"\n  Input phase verdict: {'PASS' if n_fail == 0 else f'FAIL ({n_fail} feature(s))'}")

    # Phase plots — scatter, polar, amplitude, temporal (4 figures)
    _phase_plots_input(b_phase, c_phase)
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
    elif MODE == "phase":
        mode_phase(
            BASELINE_DIR, CANDIDATE_DIR,
            BASELINE_ONNX_PATH, CANDIDATE_ONNX_PATH,
            BASELINE_MODEL_ORIGIN, CANDIDATE_MODEL_ORIGIN,
            BASELINE_SLICE, CANDIDATE_SLICE,
        )
    elif MODE == "input_phase":
        mode_input_phase(
            BASELINE_DIR, CANDIDATE_DIR,
            BASELINE_SLICE, CANDIDATE_SLICE,
        )
    else:
        print(f"Unknown mode: {MODE!r}")
