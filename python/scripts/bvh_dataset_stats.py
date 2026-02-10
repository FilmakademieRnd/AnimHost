"""
BVH Dataset Statistics Calculator

Analyzes BVH files in a dataset directory and reports:
- Frame count per file
- Duration in seconds and minutes per file
- Total statistics for the entire dataset
"""

import os
import re
from pathlib import Path

# Configuration
DATASET_PATH = r"D:\anim-ws\MANN_qudruped_data"


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


def analyze_dataset(dataset_path: str):
    """Analyze all BVH files in the dataset directory."""
    path = Path(dataset_path)

    if not path.exists():
        print(f"Error: Dataset path does not exist: {dataset_path}")
        return

    bvh_files = sorted(path.glob("**/*.bvh"))

    if not bvh_files:
        print(f"No BVH files found in: {dataset_path}")
        return

    print(f"BVH Dataset Analysis")
    print(f"{'=' * 80}")
    print(f"Dataset: {dataset_path}")
    print(f"{'=' * 80}\n")

    total_frames = 0
    total_duration = 0.0
    file_count = 0
    frame_rates = set()

    print(f"{'File':<45} {'Frames':>10} {'Duration':>15} {'FPS':>8}")
    print(f"{'-' * 45} {'-' * 10} {'-' * 15} {'-' * 8}")

    for bvh_file in bvh_files:
        result = parse_bvh_metadata(str(bvh_file))

        if result:
            frames, frame_time = result
            duration_seconds = frames * frame_time
            fps = round(1.0 / frame_time) if frame_time > 0 else 0

            total_frames += frames
            total_duration += duration_seconds
            file_count += 1
            frame_rates.add(fps)

            filename = bvh_file.name
            if len(filename) > 43:
                filename = filename[:40] + "..."

            print(f"{filename:<45} {frames:>10} {format_duration(duration_seconds):>15} {fps:>8}")

    print(f"\n{'=' * 80}")
    print(f"SUMMARY")
    print(f"{'=' * 80}")
    print(f"Total files:      {file_count}")
    print(f"Total frames:     {total_frames:,}")
    print(f"Total duration:   {format_duration(total_duration)} ({total_duration:.2f} seconds)")
    print(f"Frame rates:      {', '.join(str(fps) + ' fps' for fps in sorted(frame_rates))}")

    if file_count > 0:
        avg_frames = total_frames / file_count
        avg_duration = total_duration / file_count
        print(f"\nAverage per file:")
        print(f"  Frames:         {avg_frames:,.1f}")
        print(f"  Duration:       {format_duration(avg_duration)}")


if __name__ == "__main__":
    analyze_dataset(DATASET_PATH)
