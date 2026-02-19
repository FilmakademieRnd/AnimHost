"""
Sequence File Comparison Script

Compares two space-delimited sequence files by frame index per file stem.
Detects missing/extra frames between baseline and candidate.

Column layout: col1 col2=frame_idx col3 col4=name.ext col5
"""

from collections import defaultdict
from pathlib import Path

# Configuration
BASELINE = r"D:\anim-ws\quad-experiments\quadruped-run-6\Sequences-run-1.txt"
BASELINE_ZERO_INDEX = False  # 1-indexed → will be shifted to 0-based for comparison

CANDIDATE = r"D:\anim-ws\quad-experiments\quadruped-run-6\preprocessing-filtered\sequences_velocity.txt"
CANDIDATE_ZERO_INDEX = True  # already 0-indexed


def parse_sequence_file(filepath: str, zero_indexed: bool) -> dict[str, set[int]]:
    """Parse a sequence file and return {stem: set_of_0based_frame_indices}."""
    stem_frames: dict[str, set[int]] = defaultdict(set)
    path = Path(filepath)
    if not path.exists():
        raise FileNotFoundError(f"File not found: {filepath}")

    with open(path, "r", encoding="utf-8") as f:
        for lineno, line in enumerate(f, 1):
            line = line.strip()
            if not line:
                continue
            cols = line.split()
            if len(cols) < 4:
                print(f"  WARNING: skipping short line {lineno}: {line!r}")
                continue
            try:
                frame_idx = int(cols[1])
            except ValueError:
                print(f"  WARNING: non-integer frame_idx at line {lineno}: {cols[1]!r}")
                continue
            stem = Path(cols[3]).stem
            # Normalise to 0-based
            if not zero_indexed:
                frame_idx -= 1
            stem_frames[stem].add(frame_idx)

    return dict(stem_frames)


def report_missing_indices(label: str, stem: str, indices: set[int]) -> None:
    """Print a compact report of missing/extra index ranges."""
    if not indices:
        return
    sorted_idx = sorted(indices)
    # Collapse into ranges
    ranges = []
    start = prev = sorted_idx[0]
    for i in sorted_idx[1:]:
        if i == prev + 1:
            prev = i
        else:
            ranges.append((start, prev))
            start = prev = i
    ranges.append((start, prev))

    range_strs = [str(s) if s == e else f"{s}-{e}" for s, e in ranges]
    print(f"    [{label}] {stem}: {len(indices)} frames missing — {', '.join(range_strs)}")


def compare_sequence_files(
    baseline_path: str,
    baseline_zero_index: bool,
    candidate_path: str,
    candidate_zero_index: bool,
) -> None:
    print("=" * 80)
    print("Sequence File Comparison")
    print("=" * 80)
    print(f"Baseline:  {baseline_path}  (zero_indexed={baseline_zero_index})")
    print(f"Candidate: {candidate_path}  (zero_indexed={candidate_zero_index})")
    print()

    baseline = parse_sequence_file(baseline_path, baseline_zero_index)
    candidate = parse_sequence_file(candidate_path, candidate_zero_index)

    baseline_stems = set(baseline.keys())
    candidate_stems = set(candidate.keys())

    only_in_baseline = sorted(baseline_stems - candidate_stems)
    only_in_candidate = sorted(candidate_stems - baseline_stems)
    common_stems = sorted(baseline_stems & candidate_stems)

    if only_in_baseline:
        print(f"Stems only in BASELINE ({len(only_in_baseline)}):")
        for s in only_in_baseline:
            print(f"  - {s}")
        print()

    if only_in_candidate:
        print(f"Stems only in CANDIDATE ({len(only_in_candidate)}):")
        for s in only_in_candidate:
            print(f"  + {s}")
        print()

    # Per-stem frame comparison
    print(f"{'Stem':<40} {'Baseline':>8} {'Candidate':>10} {'Missing in Cand':>16} {'Extra in Cand':>14}")
    print(f"{'-'*40} {'-'*8} {'-'*10} {'-'*16} {'-'*14}")

    total_missing = 0
    total_extra = 0
    stems_with_diff: list[tuple[str, set[int], set[int]]] = []

    for stem in common_stems:
        b_frames = baseline[stem]
        c_frames = candidate[stem]

        missing_in_candidate = b_frames - c_frames  # in baseline but not candidate
        extra_in_candidate = c_frames - b_frames    # in candidate but not baseline

        total_missing += len(missing_in_candidate)
        total_extra += len(extra_in_candidate)

        status = "OK" if not missing_in_candidate and not extra_in_candidate else "DIFF"
        print(
            f"{stem:<40} {len(b_frames):>8} {len(c_frames):>10}"
            f" {len(missing_in_candidate):>16} {len(extra_in_candidate):>14}  {status}"
        )

        if missing_in_candidate or extra_in_candidate:
            stems_with_diff.append((stem, missing_in_candidate, extra_in_candidate))

    # Detailed missing/extra index report
    if stems_with_diff:
        print()
        print("=" * 80)
        print("DETAILED INDEX DIFFERENCES")
        print("=" * 80)
        for stem, missing, extra in stems_with_diff:
            print(f"  {stem}")
            report_missing_indices("missing in candidate", stem, missing)
            report_missing_indices("extra in candidate  ", stem, extra)

    # Summary
    print()
    print("=" * 80)
    print("SUMMARY")
    print("=" * 80)
    print(f"Stems in baseline:       {len(baseline_stems)}")
    print(f"Stems in candidate:      {len(candidate_stems)}")
    print(f"Common stems:            {len(common_stems)}")
    print(f"Only in baseline:        {len(only_in_baseline)}")
    print(f"Only in candidate:       {len(only_in_candidate)}")
    print(f"Stems with differences:  {len(stems_with_diff)}")
    print(f"Total frames missing in candidate: {total_missing}")
    print(f"Total frames extra in candidate:   {total_extra}")

    if not only_in_baseline and not only_in_candidate and total_missing == 0 and total_extra == 0:
        print("\nRESULT: Files are EQUIVALENT")
    else:
        print("\nRESULT: Files have DIFFERENCES")


if __name__ == "__main__":
    compare_sequence_files(BASELINE, BASELINE_ZERO_INDEX, CANDIDATE, CANDIDATE_ZERO_INDEX)
