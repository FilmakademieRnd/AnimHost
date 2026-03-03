"""
Velocity Segment Plotter

Loads two p_velocity.bin datasets (baseline + candidate) and plots motion
curves for a chosen segment. Produces one figure with 3 subplots (X, Y, Z
velocity) with both datasets overlaid.

X axis : frame tick within the segment (0-based consecutive)
Y axis : 1D velocity (raw float from p_velocity.bin)
"""

import struct
from pathlib import Path

import matplotlib.cm as cm
import matplotlib.lines as mlines
import matplotlib.pyplot as plt
import numpy as np

# ── Config ────────────────────────────────────────────────────────────────────

BASELINE_DIR  = r"D:\anim-ws\quad-experiments\quadruped-run-7\PAE data"
CANDIDATE_DIR = r"D:\anim-ws\quad-experiments\quadruped-run-10\e2509_20260225_0\PAE\Dataset"

SEGMENT_INDEX = 0     # 0-based index of segment to plot (0 = first segment)
JOINT_INDICES = [0]   # empty list → all 26 joints; or e.g. [0, 3, 12]

# ─────────────────────────────────────────────────────────────────────────────

NUM_JOINTS   = 26
NUM_FEATURES = NUM_JOINTS * 3   # 78
DIM_NAMES    = ["X", "Y", "Z"]


def _read_binary(path: str, num_samples: int) -> np.ndarray:
    """Read float32 Data.bin into (num_samples, NUM_FEATURES) array."""
    row_bytes = NUM_FEATURES * 4
    data = np.empty((num_samples, NUM_FEATURES), dtype=np.float32)
    with open(path, "rb") as f:
        for i in range(num_samples):
            raw = f.read(row_bytes)
            if len(raw) < row_bytes:
                raise EOFError(f"Unexpected EOF at row {i} in {path}")
            data[i] = struct.unpack(f"{NUM_FEATURES}f", raw)
    return data


def _read_seq_ids(txt_path: str) -> list[int]:
    """Read col[0] (SeqId) from Sequences.txt, one entry per row."""
    seq_ids: list[int] = []
    with open(txt_path, "r", encoding="utf-8") as f:
        for lineno, line in enumerate(f, 1):
            cols = line.split()
            if not cols:
                continue
            if len(cols) < 2:
                print(f"  WARNING: short line {lineno}, skipping")
                continue
            seq_ids.append(int(cols[0]))
    return seq_ids


def _segment_slice(seq_ids: list[int], segment_index: int) -> slice:
    """
    Return the row slice for the requested 0-based segment_index.
    Segments are ordered by first appearance of each unique SeqId.
    """
    seen: dict[int, None] = {}
    for sid in seq_ids:
        seen.setdefault(sid, None)
    unique = list(seen.keys())

    if segment_index >= len(unique):
        raise ValueError(
            f"segment_index {segment_index} is out of range "
            f"(file has {len(unique)} segments: 0–{len(unique) - 1})"
        )

    target = unique[segment_index]
    rows = [i for i, sid in enumerate(seq_ids) if sid == target]
    return slice(rows[0], rows[-1] + 1)


def _load_dataset(directory: str) -> tuple[list[int], np.ndarray]:
    d = Path(directory)
    seq_ids = _read_seq_ids(str(d / "Sequences.txt"))
    data    = _read_binary(str(d / "Data.bin"), len(seq_ids))
    return seq_ids, data


def plot_velocity_segment(
    baseline_dir:  str,
    candidate_dir: str,
    segment_index: int,
    joint_indices: list[int],
) -> None:
    print(f"Loading baseline  : {baseline_dir}")
    b_ids, b_data = _load_dataset(baseline_dir)
    print(f"Loading candidate : {candidate_dir}")
    c_ids, c_data = _load_dataset(candidate_dir)

    b_sl = _segment_slice(b_ids, segment_index)
    c_sl = _segment_slice(c_ids, segment_index)

    b_seg = b_data[b_sl]
    c_seg = c_data[c_sl]

    print(f"\nSegment {segment_index}  —  baseline: {len(b_seg)} frames  |  candidate: {len(c_seg)} frames")

    # ── Alignment diagnostic: joint-magnitude correlation matrix ─────────────
    # Trim to same length for correlation (shorter of the two)
    n = min(len(b_seg), len(c_seg))
    b_mag = np.stack([np.linalg.norm(b_seg[:n, j*3:j*3+3], axis=1) for j in range(NUM_JOINTS)], axis=1)  # (n, 26)
    c_mag = np.stack([np.linalg.norm(c_seg[:n, j*3:j*3+3], axis=1) for j in range(NUM_JOINTS)], axis=1)  # (n, 26)

    # Pearson correlation: normalise each column to zero-mean unit-variance
    def _zscore(x):
        std = x.std(axis=0)
        std[std == 0] = 1
        return (x - x.mean(axis=0)) / std

    corr = (_zscore(b_mag).T @ _zscore(c_mag)) / n   # (26, 26)  B-joint × C-joint

    print(f"\n── Joint-magnitude correlation  (B rows × C cols, top match per B-joint) {'─' * 15}")
    print(f"  Scale check  —  baseline mag mean: {b_mag.mean():.4f}  candidate mag mean: {c_mag.mean():.4f}")
    print()
    header_col = "B\\C"
    print(f"  {header_col:<6}", "  ".join(f"{j:>5}" for j in range(NUM_JOINTS)))
    for bj in range(NUM_JOINTS):
        row = corr[bj]
        best_cj = int(np.argmax(row))
        best_r  = row[best_cj]
        # Only print rows that have any notable correlation
        if best_r < 0.3:
            print(f"  j{bj:<4}  (no match, best r={best_r:.2f} @ cj{best_cj})")
        else:
            cells = "  ".join(f"{'**' if cj == best_cj else '':>3}{row[cj]:+.2f}" for cj in range(NUM_JOINTS))
            print(f"  j{bj:<4}  best→cj{best_cj} r={best_r:.2f}   {cells}")
    print()

    joints = joint_indices if len(joint_indices) > 0 else list(range(NUM_JOINTS))

    # ── Per-joint / per-dimension stats ──────────────────────────────────────
    print(f"\n── Segment {segment_index} Stats {'─' * 50}")
    print(f"  {'':12}  {'median':>10}   {'p95':>10}")
    for joint in joints:
        for dim, dim_name in enumerate(DIM_NAMES):
            col = joint * 3 + dim
            for label, seg in [("B", b_seg), ("C", c_seg)]:
                vals = seg[:, col]
                p5, med, p95 = np.percentile(vals, [5, 50, 95])
                tag = f"j{joint} {dim_name} [{label}]"
                print(f"  {tag:12}  {med:>10.10f}  {p95:>10.10f}")
        print()
    print()

    colors = cm.tab20.colors  # 20 distinct colours, cycled for more joints

    fig, axes = plt.subplots(4, 1, figsize=(14, 12), sharex=False)
    fig.suptitle(
        f"p_velocity  –  Segment {segment_index}"
        f"   (baseline: {len(b_seg)} frames,  candidate: {len(c_seg)} frames)",
        fontsize=11,
    )

    b_ticks = np.arange(len(b_seg))
    c_ticks = np.arange(len(c_seg))

    for dim, (ax, dim_name) in enumerate(zip(axes[:3], DIM_NAMES)):
        for rank, joint in enumerate(joints):
            feature_col = joint * 3 + dim
            color = colors[rank % len(colors)]
            j_label = f"j{joint}"

            ax.plot(b_ticks, b_seg[:, feature_col],
                    color=color, lw=1.1, alpha=0.9,
                    label=f"B {j_label}")
            ax.plot(c_ticks, c_seg[:, feature_col],
                    color=color, lw=1.1, alpha=0.9,
                    ls="--", label=f"C {j_label}")

        ax.set_title(f"Dimension {dim_name}", fontsize=10)
        ax.set_ylabel("velocity")
        ax.set_xlabel("frame tick")
        ax.grid(True, alpha=0.25)

    # ── Magnitude subplot (axis-order invariant diagnostic) ───────────────────
    ax_mag = axes[3]
    for rank, joint in enumerate(joints):
        color = colors[rank % len(colors)]
        b_mag = np.linalg.norm(b_seg[:, joint * 3: joint * 3 + 3], axis=1)
        c_mag = np.linalg.norm(c_seg[:, joint * 3: joint * 3 + 3], axis=1)
        ax_mag.plot(b_ticks, b_mag, color=color, lw=1.1, alpha=0.9,  label=f"B j{joint}")
        ax_mag.plot(c_ticks, c_mag, color=color, lw=1.1, alpha=0.9, ls="--", label=f"C j{joint}")
    ax_mag.set_title("Magnitude  |xyz|  (axis-invariant)", fontsize=10)
    ax_mag.set_ylabel("velocity")
    ax_mag.set_xlabel("frame tick")
    ax_mag.grid(True, alpha=0.25)

    # Legend: one colour entry per joint + style key for baseline/candidate
    style_b = mlines.Line2D([], [], color="grey", lw=1.5, label="baseline (solid)")
    style_c = mlines.Line2D([], [], color="grey", lw=1.5, ls="--", label="candidate (dashed)")

    if len(joints) <= 10:
        handles, labels = axes[3].get_legend_handles_labels()
        b_handles = [h for h, l in zip(handles, labels) if l.startswith("B")]
        b_labels  = [l[2:] for l in labels if l.startswith("B")]
        axes[3].legend(
            b_handles + [style_b, style_c],
            b_labels  + ["baseline", "candidate"],
            ncol=min(len(joints) + 2, 6), fontsize=7, loc="upper right",
        )
    else:
        axes[3].legend(handles=[style_b, style_c], loc="upper right", fontsize=9)

    plt.tight_layout()
    plt.show()


if __name__ == "__main__":
    plot_velocity_segment(
        BASELINE_DIR,
        CANDIDATE_DIR,
        SEGMENT_INDEX,
        JOINT_INDICES,
    )
