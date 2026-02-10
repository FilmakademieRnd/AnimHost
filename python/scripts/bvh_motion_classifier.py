"""
BVH Motion Classifier

Classifies BVH files into stationary vs non-stationary based on root velocity.
Useful for filtering out clips where the subject is standing/sitting still.
"""

import re
import numpy as np
import matplotlib.pyplot as plt
from pathlib import Path
from dataclasses import dataclass

# Configuration
DATASET_PATH = r"D:\anim-ws\MANN_qudruped_data"

# Classification threshold (units per second)
# Adjust based on your data scale - this is for typical mocap units (cm)
VELOCITY_THRESHOLD = 10.0  # Average velocity below this = stationary


@dataclass
class MotionStats:
    filename: str
    frames: int
    duration: float
    total_distance: float
    avg_velocity: float
    max_velocity: float
    displacement: float  # Start to end distance
    is_stationary: bool


def parse_bvh(filepath: str) -> tuple[int, float, np.ndarray] | None:
    """
    Parse BVH file and extract root positions.

    Returns:
        Tuple of (frames, frame_time, root_positions) or None if parsing fails
        root_positions is Nx3 array of XYZ positions
    """
    try:
        with open(filepath, 'r', encoding='utf-8') as f:
            content = f.read()

        # Extract metadata
        frames_match = re.search(r'Frames:\s*(\d+)', content)
        frame_time_match = re.search(r'Frame Time:\s*([\d.]+)', content)

        if not frames_match or not frame_time_match:
            return None

        frames = int(frames_match.group(1))
        frame_time = float(frame_time_match.group(1))

        # Find MOTION section and extract data
        motion_idx = content.find('MOTION')
        if motion_idx == -1:
            return None

        # Get lines after Frame Time
        lines = content[motion_idx:].split('\n')
        data_lines = []
        found_frame_time = False

        for line in lines:
            if 'Frame Time:' in line:
                found_frame_time = True
                continue
            if found_frame_time and line.strip():
                data_lines.append(line.strip())

        # Parse motion data - first 3 values are root XYZ position
        root_positions = []
        for line in data_lines[:frames]:
            values = [float(v) for v in line.split()]
            if len(values) >= 3:
                root_positions.append(values[:3])  # X, Y, Z position

        return frames, frame_time, np.array(root_positions)

    except Exception as e:
        print(f"Error parsing {filepath}: {e}")
        return None


def analyze_motion(filepath: str, velocity_threshold: float) -> MotionStats | None:
    """Analyze a BVH file and classify its motion."""
    result = parse_bvh(filepath)
    if result is None:
        return None

    frames, frame_time, positions = result

    if len(positions) < 2:
        return None

    duration = frames * frame_time

    # Calculate frame-to-frame displacements
    deltas = np.diff(positions, axis=0)
    frame_distances = np.linalg.norm(deltas, axis=1)

    # Calculate velocities (distance per second)
    velocities = frame_distances / frame_time

    # Statistics
    total_distance = np.sum(frame_distances)
    avg_velocity = np.mean(velocities)
    max_velocity = np.max(velocities)
    displacement = np.linalg.norm(positions[-1] - positions[0])

    is_stationary = avg_velocity < velocity_threshold

    return MotionStats(
        filename=Path(filepath).name,
        frames=frames,
        duration=duration,
        total_distance=total_distance,
        avg_velocity=avg_velocity,
        max_velocity=max_velocity,
        displacement=displacement,
        is_stationary=is_stationary
    )


def classify_dataset(dataset_path: str, velocity_threshold: float = VELOCITY_THRESHOLD):
    """Classify all BVH files in the dataset."""
    path = Path(dataset_path)

    if not path.exists():
        print(f"Error: Dataset path does not exist: {dataset_path}")
        return

    bvh_files = sorted(path.glob("**/*.bvh"))

    if not bvh_files:
        print(f"No BVH files found in: {dataset_path}")
        return

    print(f"BVH Motion Classification")
    print(f"{'=' * 90}")
    print(f"Dataset: {dataset_path}")
    print(f"Velocity threshold: {velocity_threshold} units/sec")
    print(f"{'=' * 90}\n")

    # Collect all stats first
    all_stats = []
    for bvh_file in bvh_files:
        stats = analyze_motion(str(bvh_file), velocity_threshold)
        if stats:
            all_stats.append(stats)

    # Sort by average velocity
    all_stats.sort(key=lambda x: x.avg_velocity)

    stationary = [s for s in all_stats if s.is_stationary]
    non_stationary = [s for s in all_stats if not s.is_stationary]

    print(f"{'File':<35} {'Dur':>7} {'AvgVel':>10} {'MaxVel':>10} {'Dist':>10} {'Class':<12}")
    print(f"{'-' * 35} {'-' * 7} {'-' * 10} {'-' * 10} {'-' * 10} {'-' * 12}")

    for stats in all_stats:
        classification = "STATIONARY" if stats.is_stationary else "MOVING"
        filename = stats.filename
        if len(filename) > 33:
            filename = filename[:30] + "..."

        print(f"{filename:<35} {stats.duration:>6.1f}s {stats.avg_velocity:>10.2f} "
              f"{stats.max_velocity:>10.2f} {stats.total_distance:>10.1f} {classification:<12}")

    # Summary
    print(f"\n{'=' * 90}")
    print(f"CLASSIFICATION SUMMARY")
    print(f"{'=' * 90}")
    print(f"\nStationary clips:     {len(stationary):>4} ({100*len(stationary)/(len(stationary)+len(non_stationary)):.1f}%)")
    print(f"Non-stationary clips: {len(non_stationary):>4} ({100*len(non_stationary)/(len(stationary)+len(non_stationary)):.1f}%)")

    if stationary:
        total_stat_dur = sum(s.duration for s in stationary)
        print(f"\nStationary total duration: {total_stat_dur:.1f}s ({total_stat_dur/60:.1f} min)")

    if non_stationary:
        total_move_dur = sum(s.duration for s in non_stationary)
        print(f"Moving total duration:     {total_move_dur:.1f}s ({total_move_dur/60:.1f} min)")

    # List files by category
    print(f"\n{'=' * 90}")
    print("STATIONARY FILES:")
    print(f"{'=' * 90}")
    print(f"  {'File':<35} {'Dur':>7} {'AvgVel':>10} {'MaxVel':>10} {'Dist':>10}")
    print(f"  {'-' * 35} {'-' * 7} {'-' * 10} {'-' * 10} {'-' * 10}")
    for s in sorted(stationary, key=lambda x: x.avg_velocity):
        print(f"  {s.filename:<35} {s.duration:>6.1f}s {s.avg_velocity:>10.2f} {s.max_velocity:>10.2f} {s.total_distance:>10.1f}")

    print(f"\n{'=' * 90}")
    print("NON-STATIONARY FILES:")
    print(f"{'=' * 90}")
    print(f"  {'File':<35} {'Dur':>7} {'AvgVel':>10} {'MaxVel':>10} {'Dist':>10}")
    print(f"  {'-' * 35} {'-' * 7} {'-' * 10} {'-' * 10} {'-' * 10}")
    for s in sorted(non_stationary, key=lambda x: x.avg_velocity, reverse=True):
        print(f"  {s.filename:<35} {s.duration:>6.1f}s {s.avg_velocity:>10.2f} {s.max_velocity:>10.2f} {s.total_distance:>10.1f}")


def plot_velocity_distribution(dataset_path: str, velocity_threshold: float = VELOCITY_THRESHOLD):
    """Plot interactive scatter plot of average vs max velocity."""
    path = Path(dataset_path)

    if not path.exists():
        print(f"Error: Dataset path does not exist: {dataset_path}")
        return

    bvh_files = sorted(path.glob("**/*.bvh"))

    if not bvh_files:
        print(f"No BVH files found in: {dataset_path}")
        return

    avg_vels = []
    max_vels = []
    names = []

    for bvh_file in bvh_files:
        result = parse_bvh(str(bvh_file))
        if result:
            frames, frame_time, positions = result
            if len(positions) < 2:
                continue
            deltas = np.diff(positions, axis=0)
            frame_distances = np.linalg.norm(deltas, axis=1)
            velocities = frame_distances / frame_time
            avg_vels.append(np.mean(velocities))
            max_vels.append(np.max(velocities))
            names.append(bvh_file.stem)

    avg_vels = np.array(avg_vels)
    max_vels = np.array(max_vels)

    stationary_mask = avg_vels < velocity_threshold
    moving_mask = ~stationary_mask

    fig, ax = plt.subplots(figsize=(10, 8))

    scatter_stat = ax.scatter(
        avg_vels[stationary_mask], max_vels[stationary_mask],
        alpha=0.7, s=80, c='gray', edgecolors='black', linewidth=0.5,
        label=f'Stationary (n={stationary_mask.sum()})', picker=True
    )
    scatter_move = ax.scatter(
        avg_vels[moving_mask], max_vels[moving_mask],
        alpha=0.7, s=80, c='steelblue', edgecolors='black', linewidth=0.5,
        label=f'Moving (n={moving_mask.sum()})', picker=True
    )

    ax.set_xlabel('Average Velocity (units/sec)', fontsize=12)
    ax.set_ylabel('Max Velocity (units/sec)', fontsize=12)
    ax.set_title('BVH Dataset: Average vs Max Root Velocity', fontsize=14)
    ax.grid(True, alpha=0.3)
    ax.axvline(x=velocity_threshold, color='gray', linestyle=':', alpha=0.5)
    ax.legend()

    # Annotation for displaying filename on click
    annot = ax.annotate(
        "", xy=(0, 0), xytext=(10, 10),
        textcoords="offset points",
        bbox=dict(boxstyle="round", fc="white", ec="gray", alpha=0.9),
        fontsize=10
    )
    annot.set_visible(False)

    # Build index mapping for picking
    stat_names = [names[i] for i in range(len(names)) if stationary_mask[i]]
    move_names = [names[i] for i in range(len(names)) if moving_mask[i]]

    def on_pick(event):
        if event.artist == scatter_stat:
            ind = event.ind[0]
            name = stat_names[ind]
            x = avg_vels[stationary_mask][ind]
            y = max_vels[stationary_mask][ind]
        elif event.artist == scatter_move:
            ind = event.ind[0]
            name = move_names[ind]
            x = avg_vels[moving_mask][ind]
            y = max_vels[moving_mask][ind]
        else:
            return

        annot.xy = (x, y)
        annot.set_text(name)
        annot.set_visible(True)
        fig.canvas.draw_idle()

    fig.canvas.mpl_connect('pick_event', on_pick)

    plt.tight_layout()
    plt.show()


if __name__ == "__main__":
    classify_dataset(DATASET_PATH)
    plot_velocity_distribution(DATASET_PATH)
