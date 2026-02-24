#!/usr/bin/env python3
"""
End-to-end test for motion preprocessing.

Tests that MotionProcessor.input_preprocessing() and output_preprocessing()
work correctly with aligned sequences using live data.

Usage:
    python test_motion_preprocessing_e2e.py
"""

import sys
import pandas as pd
from pathlib import Path

# ============================================================================
# Configuration Constants
# ============================================================================
DATASET_PATH = r"D:\anim-ws\quad-experiments\quadruped-run-6\processing-filtered"
PAE_PATH = r"D:\anim-ws\quad-experiments\quadruped-run-6\jaspe_20260224_3"

# Add parent directory to path for imports
sys.path.insert(0, str(Path(__file__).parent.parent / "ml_framework"))

from data.motion_preprocessing import MotionProcessor


def verify_sequence_alignment(dataset_path: Path, pae_path: Path):
    """Verify mann frames are subset of PAE frames."""

    mann_seq = pd.read_csv(
        dataset_path / "sequences_mann.txt",
        sep=' ', header=None,
        names=['SeqId', 'Frame', 'Type', 'File', 'UUID']
    )

    pae_seq = pd.read_csv(
        pae_path / "PAE" / "Dataset" / "Sequences.txt",
        sep=' ', header=None,
        names=['SeqId', 'Frame', 'Type', 'File', 'UUID']
    )

    mann_seqids = set(mann_seq['SeqId'].unique())
    pae_seqids = set(pae_seq['SeqId'].unique())

    # Check all mann SeqIds exist in PAE
    missing = mann_seqids - pae_seqids
    if missing:
        print(f"  X Mann SeqIds missing from PAE: {missing}")
        return False

    # Check frame alignment per SeqId
    errors = []
    for seqid in mann_seqids:
        mann_frames = set(mann_seq[mann_seq['SeqId'] == seqid]['Frame'])
        pae_frames = set(pae_seq[pae_seq['SeqId'] == seqid]['Frame'])

        missing_frames = mann_frames - pae_frames
        if missing_frames:
            errors.append(f"SeqId {seqid}: frames {list(missing_frames)[:5]}... missing")

    if errors:
        print("  X Frame alignment errors:")
        for err in errors:
            print(f"     {err}")
        return False

    print(f"  ✓ All {len(mann_seqids)} SeqIds properly aligned")
    return True


def test_motion_preprocessing(dataset_path: str, pae_path: str):
    """Run motion preprocessing end-to-end test."""

    print("\n" + "=" * 60)
    print("Motion Preprocessing E2E Test")
    print("=" * 60)

    # Verify sequence alignment first
    print("\n1. Verifying sequence alignment...")
    if not verify_sequence_alignment(Path(dataset_path), Path(pae_path)):
        print("\nX TEST FAILED: Sequences not aligned")
        return False

    # Initialize MotionProcessor
    print("\n2. Initializing MotionProcessor...")
    mp = MotionProcessor(
        dataset_path=dataset_path,
        ai4animation_path=pae_path,
        pae_epochs=1
    )
    print("  ✓ Initialized")

    # Run input preprocessing
    print("\n3. Running input_preprocessing()...")
    try:
        input_data = mp.input_preprocessing()

        if input_data is None or len(input_data) == 0:
            print("  X Input data is empty")
            return False

        if input_data.index.names != ['SeqId', 'Frame']:
            print(f"  X Incorrect index names: {input_data.index.names}")
            return False

        phase_cols = [c for c in input_data.columns if c.startswith('PhaseSpace-')]
        if len(phase_cols) == 0:
            print("  X No PhaseSpace columns found")
            return False

        print(f"  ✓ Input data shape: {input_data.shape}")
        print(f"  ✓ PhaseSpace columns: {len(phase_cols)}")

    except KeyError as e:
        print(f"  X KeyError during input preprocessing: {e}")
        return False
    except Exception as e:
        print(f"  X Error during input preprocessing: {e}")
        import traceback
        traceback.print_exc()
        return False

    # Run output preprocessing
    print("\n4. Running output_preprocessing()...")
    try:
        output_data = mp.output_preprocessing()

        if output_data is None or len(output_data) == 0:
            print("  X Output data is empty")
            return False

        phase_update_cols = [c for c in output_data.columns if c.startswith('PhaseUpdate-')]
        if len(phase_update_cols) == 0:
            print("  X No PhaseUpdate columns found")
            return False

        print(f"  ✓ Output data shape: {output_data.shape}")
        print(f"  ✓ PhaseUpdate columns: {len(phase_update_cols)}")

    except KeyError as e:
        print(f"  X KeyError during output preprocessing: {e}")
        return False
    except Exception as e:
        print(f"  X Error during output preprocessing: {e}")
        import traceback
        traceback.print_exc()
        return False

    # Verify row counts match
    if len(input_data) != len(output_data):
        print(f"  X Row count mismatch: input={len(input_data)}, output={len(output_data)}")
        return False

    print("\n" + "=" * 60)
    print("✓ ALL TESTS PASSED")
    print("=" * 60)
    return True


def main():
    """Run E2E test with configured dataset paths."""

    print(f"Using live data:")
    print(f"  Dataset: {DATASET_PATH}")
    print(f"  PAE: {PAE_PATH}")

    # Verify paths exist
    dataset_path = Path(DATASET_PATH)
    pae_path = Path(PAE_PATH)

    if not dataset_path.exists():
        print(f"\nX ERROR: Dataset path does not exist: {DATASET_PATH}")
        print("Please update DATASET_PATH constant in the script.")
        return 1

    if not pae_path.exists():
        print(f"\nX ERROR: PAE path does not exist: {PAE_PATH}")
        print("Please update PAE_PATH constant in the script.")
        return 1

    success = test_motion_preprocessing(DATASET_PATH, PAE_PATH)
    return 0 if success else 1


if __name__ == "__main__":
    sys.exit(main())
