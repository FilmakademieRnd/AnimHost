#!/usr/bin/env python3
"""
Starke Training Script - Converts LocalAutoPipeline.py to standalone training format.

Supports real-time JSON progress updates for AnimHost TrainingNode integration.
Handles both PAE (Phase Autoencoder) and GNN (Graph Neural Network) training phases.
"""

import json
import time
import sys
import subprocess
import re
import os
import shutil
from pathlib import Path
from typing import Dict, Any, Optional

from python.ml_framework.data.motion_preprocessing import MotionProcessor
from data.velocity_preprocessing import preprocess_velocity_data

#  Set to True to emit JSON for progress updates, False to ignore them
EMIT_PROGRESS_UPDATES = False
# Set to True for verbose output from GNN training phase
VERBOSE = False




def run_pae_training(dataset_path: str, path_to_ai4anim: str) -> None:
    """
    PAE (Phase Autoencoder) training subprocess with real-time parsing.

    Copies training data to PAE directory and launches the PAE Network.py subprocess
    with real-time output parsing for progress updates.

    :param dataset_path: Path to the dataset directory
    :param path_to_ai4anim: Path to the AI4Animation framework directory
    :raises RuntimeError: If PAE training subprocess fails
    """

    pae_status = {
        "status": "Starting training 1/2 ...",
        "text": "Starting PAE training phase...",
    }
    print(json.dumps(pae_status), flush=True)

    try:
        # PAE preprocessing - copy training data to PAE folder
        ai4anim_path = Path(path_to_ai4anim)
        pae_path = ai4anim_path / "PAE"
        pae_dataset_path = pae_path / "Dataset"
        dataset_dir = Path(dataset_path)

        # Copy files: p_velocity.bin -> Data.bin, sequences_velocity.txt -> Sequences.txt
        shutil.copyfile(
            dataset_dir / "p_velocity.bin", pae_dataset_path / "Data.bin"
        )
        shutil.copyfile(
            dataset_dir / "sequences_velocity.txt",
            pae_dataset_path / "Sequences.txt",
        )

        # Launch PAE Network.py subprocess
        # Use MPLBACKEND=Agg to suppress matplotlib windows because they don't show anything
        env = os.environ.copy()
        env["MPLBACKEND"] = "Agg"
        process = subprocess.Popen(
            [sys.executable, "-u", "Network.py"],
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            universal_newlines=True,
            bufsize=1,
            cwd=pae_path,
            env=env,
        )

        # Read output line by line and parse
        while True:
            line = process.stdout.readline()
            if not line and process.poll() is not None:
                break

            line = line.strip()
            if not line:
                continue

            # Parse PAE output and emit JSON if relevant
            parsed_output = _parse_pae_output(line)
            if parsed_output:
                print(json.dumps(parsed_output), flush=True)

        # Wait for process completion
        return_code = process.wait()

        if return_code != 0:
            stderr_output = process.stderr.read()
            raise RuntimeError(
                f"PAE training subprocess failed with return code {return_code}: {stderr_output}"
            )

    except Exception as e:
        error_status = {
            "status": "Error",
            "text": f"PAE preprocessing/training failed: {str(e)}",
        }
        print(json.dumps(error_status), flush=True)
        raise


def run_gnn_training(dataset_path: str, path_to_ai4anim: str, pae_epochs: int) -> None:
    """
    GNN (Graph Neural Network) training subprocess with real-time parsing.

    Preprocesses motion data using MotionProcessor and launches the GNN Network.py
    subprocess with real-time output parsing for progress updates.

    :param dataset_path: Path to the dataset directory
    :param path_to_ai4anim: Path to the AI4Animation framework directory
    :param pae_epochs: Number of epochs used in PAE training
    :raises RuntimeError: If GNN training subprocess fails
    """

    gnn_status = {
        "status": "Starting training 2/2 ...",
        "text": "Starting GNN training phase...",
    }
    print(json.dumps(gnn_status), flush=True)

    try:
        # Initialize motion processor
        mp = MotionProcessor(dataset_path, path_to_ai4anim, pae_epochs)

        # GNN preprocessing - prepare training data for generator
        df_input_data = mp.input_preprocessing()
        df_output_data = mp.output_preprocessing()
        mp.export_data()

        # Copy training data to GNN folder
        processed_path = Path("../data")
        ai4anim_path = Path(path_to_ai4anim)
        gnn_path = ai4anim_path / "GNN"

        # Copy all files from processed folder to GNN folder
        for file in os.listdir(processed_path):
            shutil.copyfile(processed_path / file, gnn_path / "Data" / file)

        # Launch GNN Network.py subprocess
        process = subprocess.Popen(
            [sys.executable, "-u", "Network.py"],
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            universal_newlines=True,
            bufsize=1,
            cwd=gnn_path,
        )

        # Read output line by line and parse
        while True:
            line = process.stdout.readline()
            if not line and process.poll() is not None:
                break

            line = line.strip()
            if not line:
                continue

            # Parse GNN output and emit JSON if relevant
            parsed_output = _parse_gnn_output(line)
            if parsed_output:
                print(json.dumps(parsed_output), flush=True)

        # Wait for process completion
        return_code = process.wait()

        if return_code != 0:
            stderr_output = process.stderr.read()
            raise RuntimeError(
                f"GNN training subprocess failed with return code {return_code}: {stderr_output}"
            )

    except Exception as e:
        error_status = {
            "status": "Error",
            "text": f"GNN preprocessing/training failed: {str(e)}",
        }
        print(json.dumps(error_status), flush=True)
        raise


def _parse_training_output(line: str, phase_name: str) -> Optional[Dict[str, Any]]:
    """
    Parse Network.py output (both PAE and GNN) and convert to JSON format.

    Returns JSON dict for relevant lines, None for irrelevant lines.
    Handles consistent output format:

    - "Epoch 1 0.32931875690483264" (epoch + loss)
    - "Progress 23.42 %" (progress percentage)
    - All other lines go through optional VERBOSE mode

    :param line: Output line from Network.py
    :param phase_name: "PAE" or "GNN" for prefixing messages
    :returns: JSON dict for relevant lines, None for irrelevant lines
    """

    # Pattern 1: "Epoch 1 0.32931875690483264"
    epoch_pattern = re.compile(r"^Epoch\s+(\d+)\s+([\d.]+)$")
    match = epoch_pattern.search(line)
    if match:
        epoch = int(match.group(1))
        loss = float(match.group(2))
        return {
            "status": f"{phase_name} training",
            "text": f"{phase_name} epoch {epoch} completed",
            "metrics": {"epoch": epoch, "train_loss": loss},
        }

    # Pattern 2: "Progress 23.42 %" (configurable progress updates)
    progress_pattern = re.compile(r"^Progress\s+([\d.]+)\s*%+$")
    match = progress_pattern.search(line)
    if match:
        if EMIT_PROGRESS_UPDATES:
            progress = float(match.group(1))
            return {
                "status": f"{phase_name} training",
                "text": f"{phase_name} Progress: {progress}%",
                "metrics": {"progress_percent": progress},
            }
        else:
            # Don't emit JSON for progress updates as they overwrite each other
            return None

    # Pattern 3: All other lines go through VERBOSE mode
    if VERBOSE:
        # Emit the full line as a message if verbose mode is enabled
        return {"status": f"{phase_name} verbose", "text": f"{phase_name}: {line}"}

    # Return None for lines that don't match expected patterns
    return None


def _parse_pae_output(line: str) -> Optional[Dict[str, Any]]:
    """
    Parse PAE Network.py output - wrapper for parse_training_output.

    :param line: Output line from PAE Network.py
    :returns: Parsed output as JSON dict or None
    """
    return _parse_training_output(line, "Encoder")


def _parse_gnn_output(line: str) -> Optional[Dict[str, Any]]:
    """
    Parse GNN Network.py output - wrapper for parse_training_output.

    :param line: Output line from GNN Network.py
    :returns: Parsed output as JSON dict or None
    """
    return _parse_training_output(line, "Controller")
