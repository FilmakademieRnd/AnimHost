"""
BVH Dataset Statistics Calculator

Analyzes BVH files in a dataset directory and reports:
- Frame count per file
- Duration in seconds and minutes per file
- Total statistics for the entire dataset
- Comparison between raw BVH dataset and Unity project
"""

import os
import re
from pathlib import Path

# Configuration
RAW_DATASET_PATH = r"D:\anim-ws\MANN_qudruped_data\all"
UNITY_DATASET_PATH = r"D:\anim-ws\AI4Animation-master\AI4Animation\SIGGRAPH_2022\Unity\Assets\Projects\DeepPhase\Demos\Quadruped\Assets"
SEQUENCES_FILE = r"D:\anim-ws\quad-experiments\quadruped-run-1\PAE Dataset\Sequences.txt"

def parse_bvh_metadata(filepath: str) -> tuple[int, float] | None:
    """
    Parse a BVH file and extract frame count and frame time.

    Returns:
        Tuple of (frames, frame_time) or None if parsing fails
    """
    try:
        with open(filepath, 'r', encoding='utf-8') as f:
            content = f.read()

        frames_match = re.search(r'Frames:\s*(\d+)', content)
        frame_time_match = re.search(r'Frame Time:\s*([\d.]+)', content)

        if frames_match and frame_time_match:
            frames = int(frames_match.group(1))
            frame_time = float(frame_time_match.group(1))
            return frames, frame_time
        return None
    except Exception as e:
        print(f"Error reading {filepath}: {e}")
        return None


def format_duration(seconds: float) -> str:
    """Format duration as minutes:seconds"""
    minutes = int(seconds // 60)
    secs = seconds % 60
    return f"{minutes}m {secs:.2f}s"


def analyze_raw_dataset(dataset_path: str) -> dict[str, dict]:
    """Analyze all BVH files in the raw dataset directory.

    Returns:
        Dict mapping base name (without .bvh) to stats dict
    """
    path = Path(dataset_path)
    stats = {}

    if not path.exists():
        print(f"Error: Dataset path does not exist: {dataset_path}")
        return stats

    bvh_files = sorted(path.glob("**/*.bvh"))

    if not bvh_files:
        print(f"No BVH files found in: {dataset_path}")
        return stats

    for bvh_file in bvh_files:
        result = parse_bvh_metadata(str(bvh_file))
        if result:
            frames, frame_time = result
            duration_seconds = frames * frame_time
            fps = round(1.0 / frame_time) if frame_time > 0 else 0

            # Use stem (filename without .bvh) as key
            base_name = bvh_file.stem
            stats[base_name] = {
                'path': bvh_file,
                'frames': frames,
                'frame_time': frame_time,
                'duration': duration_seconds,
                'fps': fps
            }

    return stats


def get_unity_asset_names(unity_path: str) -> list[str]:
    """Get list of asset names from Unity project subdirectories."""
    path = Path(unity_path)
    names = []

    if not path.exists():
        print(f"Error: Unity path does not exist: {unity_path}")
        return names

    # Each subdirectory name corresponds to a BVH file
    for item in sorted(path.iterdir()):
        if item.is_dir() and not item.name.endswith('.meta'):
            names.append(item.name)

    return names


def print_dataset_stats(name: str, stats: dict[str, dict]):
    """Print statistics for a dataset."""
    print(f"\n{'=' * 80}")
    print(f"{name}")
    print(f"{'=' * 80}\n")

    if not stats:
        print("No files found.")
        return

    total_frames = 0
    total_duration = 0.0
    frame_rates = set()

    print(f"{'File':<45} {'Frames':>10} {'Duration':>15} {'FPS':>8}")
    print(f"{'-' * 45} {'-' * 10} {'-' * 15} {'-' * 8}")

    for name, data in sorted(stats.items()):
        total_frames += data['frames']
        total_duration += data['duration']
        frame_rates.add(data['fps'])

        display_name = name
        if len(display_name) > 43:
            display_name = display_name[:40] + "..."

        print(f"{display_name:<45} {data['frames']:>10} {format_duration(data['duration']):>15} {data['fps']:>8}")

    print(f"\n{'-' * 80}")
    print(f"Total files:      {len(stats)}")
    print(f"Total frames:     {total_frames:,}")
    print(f"Total duration:   {format_duration(total_duration)} ({total_duration:.2f} seconds)")
    print(f"Frame rates:      {', '.join(str(fps) + ' fps' for fps in sorted(frame_rates))}")

    if stats:
        avg_frames = total_frames / len(stats)
        avg_duration = total_duration / len(stats)
        print(f"\nAverage per file:")
        print(f"  Frames:         {avg_frames:,.1f}")
        print(f"  Duration:       {format_duration(avg_duration)}")


def analyze_sequences_file(sequences_path: str) -> dict[str, dict]:
    """Analyze the sequences file and count frames per file.

    Returns:
        Dict mapping filename (without .bvh) to stats dict with standard/mirrored frame counts
    """
    from collections import defaultdict

    path = Path(sequences_path)
    if not path.exists():
        print(f"Error: Sequences file does not exist: {sequences_path}")
        return {}

    seq_stats = defaultdict(lambda: {'standard': 0, 'mirrored': 0, 'scenes': []})

    with open(path, 'r', encoding='utf-8') as f:
        for line in f:
            parts = line.strip().split()
            if len(parts) >= 4:
                scene_id = int(parts[0])
                mode = parts[2]
                filename = parts[3].replace('.bvh', '')

                if mode == 'Standard':
                    seq_stats[filename]['standard'] += 1
                else:
                    seq_stats[filename]['mirrored'] += 1
                if scene_id not in seq_stats[filename]['scenes']:
                    seq_stats[filename]['scenes'].append(scene_id)

    return dict(seq_stats)


def compare_sequences_with_raw(raw_stats: dict[str, dict], seq_stats: dict[str, dict]):
    """Compare sequences file with raw BVH dataset."""
    print(f"\n{'=' * 80}")
    print("COMPARISON: Sequences File vs Raw BVH Dataset")
    print(f"{'=' * 80}\n")

    raw_names = set(raw_stats.keys())
    seq_names = set(seq_stats.keys())

    missing_from_seq = raw_names - seq_names
    missing_from_raw = seq_names - raw_names

    print(f"Raw BVH files:        {len(raw_names)}")
    print(f"Files in sequences:   {len(seq_names)}")
    print(f"Missing from seq:     {len(missing_from_seq)}")

    if missing_from_seq:
        print(f"\n--- Raw BVH files NOT in sequences ({len(missing_from_seq)}) ---")
        for name in sorted(missing_from_seq):
            print(f"  {name}")

    if missing_from_raw:
        print(f"\n--- Sequences files NOT in raw dataset ({len(missing_from_raw)}) ---")
        for name in sorted(missing_from_raw):
            print(f"  {name}")

    # Frame count comparison
    print(f"\n{'Filename':<28} {'Raw':>10} {'Seq Std':>10} {'Ratio':>8} {'Status':>10}")
    print("-" * 70)

    full_match = []
    total_seq_frames = 0

    for filename in sorted(raw_stats.keys()):
        raw_frames = raw_stats[filename]['frames']
        seq_std = seq_stats.get(filename, {}).get('standard', 0)
        total_seq_frames += seq_std

        ratio = f"{seq_std/raw_frames*100:.1f}%" if raw_frames > 0 else "-"

        if seq_std == 0:
            status = "MISSING"
        elif seq_std == raw_frames:
            status = "FULL"
            full_match.append(filename)
        else:
            status = f"{seq_std}/{raw_frames}"

        print(f"{filename:<28} {raw_frames:>10} {seq_std:>10} {ratio:>8} {status:>10}")

    print("-" * 70)
    print(f"\nTotal sequence frames (Standard): {total_seq_frames:,}")
    print(f"Files with FULL frame count: {len(full_match)}")
    if full_match:
        for f in full_match:
            print(f"  - {f} ({raw_stats[f]['frames']} frames)")


def compare_datasets(raw_stats: dict[str, dict], unity_names: list[str]):
    """Compare raw BVH dataset with Unity project assets."""
    print(f"\n{'=' * 80}")
    print("COMPARISON: Unity Project vs Raw BVH Dataset")
    print(f"{'=' * 80}\n")

    raw_names = set(raw_stats.keys())
    unity_set = set(unity_names)

    # Files in Unity but not in raw dataset
    missing_from_raw = unity_set - raw_names
    # Files in raw dataset but not in Unity
    missing_from_unity = raw_names - unity_set
    # Files in both
    matched = raw_names & unity_set

    print(f"Unity assets:     {len(unity_names)}")
    print(f"Raw BVH files:    {len(raw_names)}")
    print(f"Matched:          {len(matched)}")

    if missing_from_raw:
        print(f"\n--- Unity assets WITHOUT matching raw BVH ({len(missing_from_raw)}) ---")
        for name in sorted(missing_from_raw):
            print(f"  {name}")

    if missing_from_unity:
        print(f"\n--- Raw BVH files NOT in Unity project ({len(missing_from_unity)}) ---")
        for name in sorted(missing_from_unity):
            print(f"  {name}")

    if not missing_from_raw and not missing_from_unity:
        print("\nAll files matched!")

    # Stats for matched Unity assets
    if matched:
        matched_stats = {k: v for k, v in raw_stats.items() if k in matched}
        total_frames = sum(d['frames'] for d in matched_stats.values())
        total_duration = sum(d['duration'] for d in matched_stats.values())

        print(f"\n--- Unity Project Statistics (from matched raw files) ---")
        print(f"Total files:      {len(matched_stats)}")
        print(f"Total frames:     {total_frames:,}")
        print(f"Total duration:   {format_duration(total_duration)} ({total_duration:.2f} seconds)")


if __name__ == "__main__":
    print("BVH Dataset Analysis")
    print(f"Raw dataset:   {RAW_DATASET_PATH}")
    print(f"Unity project: {UNITY_DATASET_PATH}")
    print(f"Sequences:     {SEQUENCES_FILE}")

    # Analyze raw BVH dataset
    raw_stats = analyze_raw_dataset(RAW_DATASET_PATH)
    print_dataset_stats("RAW BVH DATASET", raw_stats)

    # Get Unity asset names
    unity_names = get_unity_asset_names(UNITY_DATASET_PATH)

    # Compare datasets
    compare_datasets(raw_stats, unity_names)

    # Analyze sequences file
    seq_stats = analyze_sequences_file(SEQUENCES_FILE)
    if seq_stats:
        compare_sequences_with_raw(raw_stats, seq_stats)
