#!/usr/bin/env python3
"""
Starke Training Script - Converts LocalAutoPipeline.py to standalone training format.

Supports real-time JSON progress updates for AnimHost TrainingNode integration.
Handles both PAE (Phase Autoencoder) and GNN (Graph Neural Network) training phases.
"""

import json
import logging
import re
import shutil
from pathlib import Path

from data.motion_preprocessing import MotionProcessor
from .script_subprocess import run_script_subprocess
from config.model_configs import StarkeModelConfig

logger = logging.getLogger(__name__)

# TODO(PR): Make this configurable in new ExperimentLogger
#  Set to True to emit JSON for progress updates, False to ignore them
EMIT_PROGRESS_UPDATES = False
# Set to True for verbose output from GNN training phase
VERBOSE = False


def validate_ai4animation_structure(path_to_ai4anim: Path, phase: str) -> bool:
    """
    Validate AI4Animation framework structure for given training phase.

    :param path_to_ai4anim: Path to AI4Animation framework
    :param phase: Training phase ("PAE" or "GNN")
    :returns: True if structure is valid, False otherwise
    """
    phase_path = path_to_ai4anim / phase

    if not phase_path.exists() or not phase_path.is_dir():
        logger.error(f"AI4Animation {phase} directory not found at {phase_path}")
        return False

    network_script = phase_path / "Network.py"
    if not network_script.exists() or not network_script.is_file():
        logger.error(
            f"AI4Animation {phase} Network.py script not found at {network_script}"
        )
        return False

    if phase == "PAE":
        dataset_path = phase_path / "Dataset"
        if not dataset_path.exists() or not dataset_path.is_dir():
            logger.error(
                f"AI4Animation PAE Dataset directory not found at {dataset_path}"
            )
            return False
    elif phase == "GNN":
        data_path = phase_path / "Data"
        if not data_path.exists() or not data_path.is_dir():
            logger.error(f"AI4Animation GNN Data directory not found at {data_path}")
            return False

    return True


def run_pae_training(dataset_path: Path, path_to_ai4anim: Path) -> None:
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

    # Validate AI4Animation PAE structure
    if not validate_ai4animation_structure(path_to_ai4anim, "PAE"):
        raise RuntimeError("PAE training structure validation failed")

    # Validate input files exist
    p_velocity_file = dataset_path / "p_velocity.bin"
    sequences_file = dataset_path / "sequences_velocity.txt"

    if not p_velocity_file.exists() or not p_velocity_file.is_file():
        logger.error(f"Required PAE input file not found: {p_velocity_file}")
        raise RuntimeError("Required PAE input file missing: p_velocity.bin")

    if not sequences_file.exists() or not sequences_file.is_file():
        logger.error(f"Required PAE input file not found: {sequences_file}")
        raise RuntimeError("Required PAE input file missing: sequences_velocity.txt")

    # PAE preprocessing - copy training data to PAE folder
    pae_path = path_to_ai4anim / "PAE"
    pae_dataset_path = pae_path / "Dataset"

    try:
        # Copy files: p_velocity.bin -> Data.bin, sequences_velocity.txt -> Sequences.txt
        shutil.copyfile(p_velocity_file, pae_dataset_path / "Data.bin")
        shutil.copyfile(sequences_file, pae_dataset_path / "Sequences.txt")

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
        logger.error(f"PAE preprocessing/training failed: {str(e)}")
        raise


def run_gnn_training(config: StarkeModelConfig) -> None:
    """
    GNN (Graph Neural Network) training subprocess with real-time parsing.

    Preprocesses motion data using MotionProcessor and launches the GNN Network.py
    subprocess with real-time output parsing for progress updates.

    :param config: StarkeModelConfig containing all training parameters
    :raises RuntimeError: If GNN training subprocess fails
    """

    gnn_status = {
        "status": "Starting training 2/2 ...",
        "text": "Starting GNN training phase...",
    }
    print(json.dumps(gnn_status), flush=True)

    # Validate AI4Animation GNN structure
    if not validate_ai4animation_structure(config.path_to_ai4anim, "GNN"):
        raise RuntimeError("GNN training structure validation failed")

    try:
        # Initialize motion processor
        mp = MotionProcessor(
            config.dataset_path, config.path_to_ai4anim, config.pae_epochs
        )

        # GNN preprocessing - prepare training data for generator
        mp.input_preprocessing()
        mp.output_preprocessing()
        mp.export_data()

        # Copy all files from processed folder to GNN folder
        gnn_path = config.path_to_ai4anim / "GNN"
        for file_path in config.processed_data_path.iterdir():
            if file_path.is_file():  # Only copy files, not directories
                shutil.copyfile(file_path, gnn_path / "Data" / file_path.name)

        # Launch GNN Network.py subprocess
        run_script_subprocess(
            script_name="Network.py",
            working_dir=gnn_path,
            model_name="Controller",
            line_parser=parse_training_output,
        )
    except Exception as e:
        logger.error(f"GNN preprocessing/training failed: {str(e)}")
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
