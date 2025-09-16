#!/usr/bin/env python3
"""
Starke Training Script - Converts LocalAutoPipeline.py to standalone training format.

Supports real-time JSON progress updates for AnimHost TrainingNode integration.
Handles both PAE (Phase Autoencoder) and GNN (Graph Neural Network) training phases.
"""

import json
import os
import re
import shutil
from pathlib import Path

from data.motion_preprocessing import MotionProcessor
from .script_subprocess import run_script_subprocess

# TODO(PR): Make this configurable in new ExperimentLogger
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
        input_data_dir = Path(dataset_path)

        # Copy files: p_velocity.bin -> Data.bin, sequences_velocity.txt -> Sequences.txt
        shutil.copyfile(input_data_dir / "p_velocity.bin", pae_dataset_path / "Data.bin")
        shutil.copyfile(
            input_data_dir / "sequences_velocity.txt",
            pae_dataset_path / "Sequences.txt",
        )

        # Launch PAE Network.py subprocess
        # Use MPLBACKEND=Agg to suppress matplotlib windows because they don't show anything
        run_script_subprocess(
            script_name="Network.py",
            working_dir=pae_path,
            model_name="Encoder",
            line_parser=parse_training_output,
            env_overrides={"MPLBACKEND": "Agg"},
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
        mp.input_preprocessing()
        mp.output_preprocessing()
        mp.export_data()

        # Copy training data to GNN folder
        # TODO(jasper2xf): Make this more robust - use temp dirs or explicit paths
        processed_data_dir = Path("../data")
        ai4anim_path = Path(path_to_ai4anim)
        gnn_path = ai4anim_path / "GNN"

        # Copy all files from processed folder to GNN folder
        for file in os.listdir(processed_data_dir):
            shutil.copyfile(processed_data_dir / file, gnn_path / "Data" / file)

        # Launch GNN Network.py subprocess
        run_script_subprocess(
            script_name="Network.py",
            working_dir=gnn_path,
            model_name="Controller",
            line_parser=parse_training_output,
        )

    except Exception as e:
        error_status = {
            "status": "Error",
            "text": f"GNN preprocessing/training failed: {str(e)}",
        }
        print(json.dumps(error_status), flush=True)
        raise


def parse_training_output(line: str, model_name: str) -> None:
    """
    Parse Network.py output (both PAE and GNN) and emit JSON to stdout.

    Handles consistent output format:

    - "Epoch 1 0.32931875690483264" (epoch + loss)
    - "Progress 23.42 %" (progress percentage)
    - All other lines go through optional VERBOSE mode

    :param line: Output line from Network.py
    :param model_name: "PAE" or "GNN" for prefixing messages
    """
    parsed_output = None

    # Pattern 1: "Epoch 1 0.32931875690483264"
    epoch_pattern = re.compile(r"^Epoch\s+(\d+)\s+([\d.]+)$")
    match = epoch_pattern.search(line)
    if match:
        epoch = int(match.group(1))
        loss = float(match.group(2))
        parsed_output = {
            "status": f"{model_name} training",
            "text": f"{model_name} epoch {epoch} completed",
            "metrics": {"epoch": epoch, "train_loss": loss},
        }

    # Pattern 2: "Progress 23.42 %" (configurable progress updates)
    progress_pattern = re.compile(r"^Progress\s+([\d.]+)\s*%+$")
    match = progress_pattern.search(line)
    if match:
        if EMIT_PROGRESS_UPDATES:
            progress = float(match.group(1))
            parsed_output = {
                "status": f"{model_name} training",
                "text": f"{model_name} Progress: {progress}%",
                "metrics": {"progress_percent": progress},
            }

    # Pattern 3: All other lines go through VERBOSE mode
    if VERBOSE and not parsed_output:
        # Emit the full line as a message if verbose mode is enabled
        parsed_output = {
            "status": f"{model_name} verbose",
            "text": f"{model_name}: {line}",
        }

    # Emit JSON if we have parsed output
    if parsed_output:
        print(json.dumps(parsed_output), flush=True)
