"""
Copy BVH files with high usage ratio from sequences file.

Copies files from RAW_DATASET_PATH to TARGET_DIR if the ratio of
frames used in sequences (Standard mode) to raw frames exceeds VALID_THRESHOLD.
"""

import shutil
from pathlib import Path
from collections import defaultdict

from bvh_dataset_stats import parse_bvh_metadata, analyze_raw_dataset

# Configuration
RAW_DATASET_PATH = r"D:\anim-ws\MANN_qudruped_data\all"
TARGET_DIR = r"D:\anim-ws\MANN_qudruped_data\p100"
SEQUENCES_FILE = r"D:\anim-ws\quad-experiments\quadruped-run-1\PAE Dataset\Sequences.txt"
VALID_THRESHOLD = 1  # 100%


def analyze_sequences_file(sequences_path: str) -> dict[str, int]:
    """Analyze the sequences file and count standard frames per file.

    Returns:
        Dict mapping filename (without .bvh) to standard frame count
    """
    path = Path(sequences_path)
    if not path.exists():
        print(f"Error: Sequences file does not exist: {sequences_path}")
        return {}

    seq_counts = defaultdict(int)

    with open(path, 'r', encoding='utf-8') as f:
        for line in f:
            parts = line.strip().split()
            if len(parts) >= 4:
                mode = parts[2]
                filename = parts[3].replace('.bvh', '')

                if mode == 'Standard':
                    seq_counts[filename] += 1

    return dict(seq_counts)


def copy_high_usage_files():
    """Copy BVH files with usage ratio above threshold."""
    print(f"Raw dataset:    {RAW_DATASET_PATH}")
    print(f"Target dir:     {TARGET_DIR}")
    print(f"Sequences file: {SEQUENCES_FILE}")
    print(f"Threshold:      {VALID_THRESHOLD * 100:.0f}%")
    print()

    # Create target directory
    target_path = Path(TARGET_DIR)
    target_path.mkdir(parents=True, exist_ok=True)

    # Get raw stats and sequence counts
    raw_stats = analyze_raw_dataset(RAW_DATASET_PATH)
    seq_counts = analyze_sequences_file(SEQUENCES_FILE)

    if not raw_stats:
        print("Error: No raw BVH files found")
        return

    copied = []
    skipped = []

    print(f"{'Filename':<30} {'Raw':>8} {'Seq':>8} {'Ratio':>8} {'Action':>10}")
    print("-" * 70)

    for filename, data in sorted(raw_stats.items()):
        raw_frames = data['frames']
        seq_frames = seq_counts.get(filename, 0)
        ratio = seq_frames / raw_frames if raw_frames > 0 else 0

        source_path = data['path']
        dest_path = target_path / f"{filename}.bvh"

        if ratio >= VALID_THRESHOLD:
            shutil.copy2(source_path, dest_path)
            action = "COPIED"
            copied.append((filename, ratio))
        else:
            action = "skipped"
            skipped.append((filename, ratio))

        print(f"{filename:<30} {raw_frames:>8} {seq_frames:>8} {ratio:>7.1%} {action:>10}")

    print("-" * 70)
    print(f"\nCopied:  {len(copied)} files")
    print(f"Skipped: {len(skipped)} files")
    print(f"\nFiles copied to: {TARGET_DIR}")

    if copied:
        print("\n--- Copied files ---")
        for name, ratio in copied:
            print(f"  {name}: {ratio:.1%}")


if __name__ == "__main__":
    copy_high_usage_files()
