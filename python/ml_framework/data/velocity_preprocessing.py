#!/usr/bin/env python3
"""
Velocity data preprocessing for AnimHost ML Framework.

Handles preprocessing of motion capture velocity data including filtering
and binary export operations.
"""

import array
import struct
from pathlib import Path

import numpy as np
import pandas as pd
import scipy.signal

from data.motion_preprocessing import (
    count_lines,
    read_csv_data,
    ReadBinary,
)


def preprocess_velocity_data(dataset_path: str) -> None:
    """
    Preprocess velocity data with Butterworth filtering and binary export.

    Applies Butterworth filtering to velocity data to remove noise and exports
    the processed data to binary format for PAE training.

    :param dataset_path: Path to the dataset directory containing velocity data
    :raises RuntimeError: If data preprocessing fails
    """
    try:
        # Count number of velocity samples
        dataset_dir = Path(dataset_path)
        num_samples_total = count_lines(str(dataset_dir / "sequences_velocity.txt"))
        num_features_velocity = 78  # 26 joints * 3 dimensions

        # Apply butterworth filter to velocity data to remove noise
        velocities = ReadBinary(
            str(dataset_dir / "joint_velocity.bin"),
            num_samples_total,
            num_features_velocity,
        )
        df_velocities = pd.DataFrame(velocities)
        velocity_ids = read_csv_data(str(dataset_dir / "sequences_velocity.txt"))
        velocity_ids.columns = ["SeqId", "Frame", "Type", "File", "SeqUUID"]
        df_velocities = pd.concat([velocity_ids, df_velocities], axis=1)
        df_velocities.set_index(["SeqId", "Frame"], inplace=True)

        unique_seq_ids = df_velocities.index.get_level_values("SeqId").unique()

        # Prepare butterworth filter
        fc = 4.5
        w = fc / (60 / 2)
        b, a = scipy.signal.butter(5, w, "low")
        result_sequences = []

        # Apply filter to each sequence
        for seq_id in unique_seq_ids:
            sequence = df_velocities.xs(key=seq_id, level="SeqId")
            butterw_combined = []
            for col in range(num_features_velocity):
                butterw_result = scipy.signal.filtfilt(b, a, sequence[col])
                butterw_combined.append(butterw_result)
            result_sequences.append(np.vstack(butterw_combined).T)

        final = np.vstack(result_sequences)
        df_final = pd.DataFrame(final)

        # Save to binary
        out_array = array.array("d", df_final.to_numpy(dtype=np.float32).flatten())
        with open(dataset_dir / "p_velocity.bin", "wb") as file:
            s = struct.pack("f" * len(out_array), *out_array)
            file.write(s)

    except Exception as e:
        raise RuntimeError(f"Data preprocessing failed: {str(e)}")
