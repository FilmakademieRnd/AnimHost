"""
Directory Comparison Script

Compares file sizes and content (via MD5 hash) between two directories.
Supports ignoring specific files.
For text files with hash differences, performs line-by-line comparison.
"""

import difflib
import hashlib
from pathlib import Path

# Configuration
BASELINE_DIR = r"D:\anim-ws\quad-experiments\quadruped-run-5\v0.1.1-processing"
CANDIDATE_DIR = r"D:\anim-ws\quad-experiments\quadruped-run-5\0cc24a3-processing"
IGNORE_FILES = ["LogOutput.txt"]


def get_file_hash(filepath: Path) -> str:
    """Calculate MD5 hash of a file."""
    hash_md5 = hashlib.md5()
    with open(filepath, "rb") as f:
        for chunk in iter(lambda: f.read(8192), b""):
            hash_md5.update(chunk)
    return hash_md5.hexdigest()


def strip_guid(line: str) -> str:
    """Remove GUID pattern {xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx} from line."""
    import re
    return re.sub(r'\{[a-f0-9-]{36}\}', '{GUID}', line)


def compare_text_files(baseline_file: Path, candidate_file: Path, ignore_guid: bool = False) -> tuple[list[str], list[str]]:
    """Compare two text files line by line and return differences.

    Returns:
        Tuple of (raw_diff_lines, normalized_diff_lines) where normalized ignores GUIDs
    """
    with open(baseline_file, "r", encoding="utf-8") as f:
        baseline_lines = f.readlines()
    with open(candidate_file, "r", encoding="utf-8") as f:
        candidate_lines = f.readlines()

    # Raw diff
    raw_diff = list(difflib.unified_diff(
        baseline_lines,
        candidate_lines,
        fromfile="baseline",
        tofile="candidate",
        lineterm=""
    ))

    # Normalized diff (ignoring GUIDs)
    if ignore_guid:
        baseline_normalized = [strip_guid(line) for line in baseline_lines]
        candidate_normalized = [strip_guid(line) for line in candidate_lines]
        normalized_diff = list(difflib.unified_diff(
            baseline_normalized,
            candidate_normalized,
            fromfile="baseline",
            tofile="candidate",
            lineterm=""
        ))
    else:
        normalized_diff = raw_diff

    return raw_diff, normalized_diff


def get_dir_files(dir_path: Path, ignore_files: list[str]) -> dict[str, Path]:
    """Get all files in directory, excluding ignored files."""
    files = {}
    for item in dir_path.iterdir():
        if item.is_file() and item.name not in ignore_files:
            files[item.name] = item
    return files


def compare_directories(baseline_dir: str, candidate_dir: str, ignore_files: list[str]):
    """Compare two directories for file size and content differences."""
    baseline_path = Path(baseline_dir)
    candidate_path = Path(candidate_dir)

    if not baseline_path.exists():
        print(f"Error: Baseline directory does not exist: {baseline_dir}")
        return
    if not candidate_path.exists():
        print(f"Error: Candidate directory does not exist: {candidate_dir}")
        return

    print(f"{'=' * 80}")
    print("Directory Comparison")
    print(f"{'=' * 80}")
    print(f"Baseline:  {baseline_dir}")
    print(f"Candidate: {candidate_dir}")
    print(f"Ignored:   {', '.join(ignore_files) if ignore_files else 'None'}")
    print()

    baseline_files = get_dir_files(baseline_path, ignore_files)
    candidate_files = get_dir_files(candidate_path, ignore_files)

    baseline_names = set(baseline_files.keys())
    candidate_names = set(candidate_files.keys())

    # Files only in baseline
    only_in_baseline = baseline_names - candidate_names
    # Files only in candidate
    only_in_candidate = candidate_names - baseline_names
    # Files in both
    common_files = baseline_names & candidate_names

    # Report missing files
    if only_in_baseline:
        print(f"Files only in BASELINE ({len(only_in_baseline)}):")
        for name in sorted(only_in_baseline):
            print(f"  - {name}")
        print()

    if only_in_candidate:
        print(f"Files only in CANDIDATE ({len(only_in_candidate)}):")
        for name in sorted(only_in_candidate):
            print(f"  + {name}")
        print()

    # Compare common files
    print(f"{'File':<30} {'Size Match':>12} {'Content Match':>15} {'Details'}")
    print(f"{'-' * 30} {'-' * 12} {'-' * 15} {'-' * 20}")

    size_mismatches = []
    content_mismatches = []
    guid_only_diffs = []  # Files that differ only in GUID
    matches = []
    text_file_diffs = {}

    for name in sorted(common_files):
        baseline_file = baseline_files[name]
        candidate_file = candidate_files[name]

        baseline_size = baseline_file.stat().st_size
        candidate_size = candidate_file.stat().st_size

        size_match = baseline_size == candidate_size

        if size_match:
            baseline_hash = get_file_hash(baseline_file)
            candidate_hash = get_file_hash(candidate_file)
            content_match = baseline_hash == candidate_hash
        else:
            content_match = False

        size_status = "YES" if size_match else "NO"
        content_status = "YES" if content_match else "NO"

        if size_match and content_match:
            details = ""
            matches.append(name)
        elif not size_match:
            diff = candidate_size - baseline_size
            sign = "+" if diff > 0 else ""
            details = f"{sign}{diff} bytes"
            size_mismatches.append((name, baseline_size, candidate_size))
        else:
            details = "hash differs"
            content_mismatches.append(name)
            # For text files, capture line-by-line diff
            if name.endswith(".txt"):
                raw_diff, normalized_diff = compare_text_files(baseline_file, candidate_file, ignore_guid=True)
                text_file_diffs[name] = (raw_diff, normalized_diff)

        print(f"{name:<30} {size_status:>12} {content_status:>15} {details}")

    # Print line-by-line diffs for text files
    if text_file_diffs:
        print()
        print(f"{'=' * 80}")
        print("TEXT FILE DIFFERENCES (line-by-line)")
        print(f"{'=' * 80}")
        for name, (raw_diff, normalized_diff) in text_file_diffs.items():
            print(f"\n--- {name} ---")

            # Count raw differences
            raw_additions = sum(1 for l in raw_diff if l.startswith("+") and not l.startswith("+++"))
            raw_deletions = sum(1 for l in raw_diff if l.startswith("-") and not l.startswith("---"))

            # Count normalized differences (ignoring GUIDs)
            norm_additions = sum(1 for l in normalized_diff if l.startswith("+") and not l.startswith("+++"))
            norm_deletions = sum(1 for l in normalized_diff if l.startswith("-") and not l.startswith("---"))

            print(f"Raw diff:        {raw_additions} additions, {raw_deletions} deletions")
            print(f"Ignoring GUIDs:  {norm_additions} additions, {norm_deletions} deletions")

            if norm_additions == 0 and norm_deletions == 0:
                print("=> Files are IDENTICAL when ignoring GUIDs (dataSetID)")
                guid_only_diffs.append(name)
                content_mismatches.remove(name)
            elif normalized_diff:
                # Show first N normalized diff lines as preview
                max_preview = 20
                preview_lines = [l for l in normalized_diff if not l.startswith("@@")][:max_preview]
                print()
                for line in preview_lines:
                    print(line.rstrip("\n"))

                if len(normalized_diff) > max_preview:
                    print(f"\n... ({len(normalized_diff) - max_preview} more lines, showing first {max_preview})")

    # Summary
    print()
    print(f"{'=' * 80}")
    print("SUMMARY")
    print(f"{'=' * 80}")
    equivalent_files = len(matches) + len(guid_only_diffs)
    print(f"Total files compared:    {len(common_files)}")
    print(f"Identical files:         {len(matches)}")
    print(f"Equivalent (GUID diff):  {len(guid_only_diffs)}")
    print(f"Size mismatches:         {len(size_mismatches)}")
    print(f"Content mismatches:      {len(content_mismatches)}")
    print(f"Only in baseline:        {len(only_in_baseline)}")
    print(f"Only in candidate:       {len(only_in_candidate)}")

    if equivalent_files == len(common_files) and not only_in_baseline and not only_in_candidate:
        print("\nRESULT: Directories are EQUIVALENT (excluding ignored files)")
    else:
        print("\nRESULT: Directories have DIFFERENCES")


if __name__ == "__main__":
    compare_directories(BASELINE_DIR, CANDIDATE_DIR, IGNORE_FILES)
