#!/usr/bin/env python3
"""
Starke training script functions. Orchestration logic is separated into experiment classes.

Supports starke repo validation, python script parameter adjustements, and subprocess
training script calls with real-time output parsing.

Function-based Design Rationale:
- Functions provide clear, testable building blocks for training operations
- Easy to restructure and compose functions into potential PAE-only, GNN-only experiments
- Stateless functions are simpler to debug and reason about
"""
import logging
import re
import shutil
from pathlib import Path
from typing import Dict, Union

from data.motion_preprocessing import (
    MotionProcessor,
    count_lines,
    parse_input_output_features,
)
from .script_subprocess import run_script_subprocess
from .script_editing import read_script_variables, write_script_variables, reset_script
from config.model_configs import StarkeModelConfig
from experiment_tracker import ExperimentTracker

logger = logging.getLogger(__name__)


def init_model(config: StarkeModelConfig) -> None:
    """
    Initialize model by setting hyperparameters and parameters inferred from input data.

    :param config: Starke model configuration
    """
    # PAE data loading requires input feature metadata
    _init_pae_data_shape(config.dataset_path, config.path_to_ai4anim)

    # Initialize PAE network with epochs
    _init_pae_network_script(config.path_to_ai4anim, config.pae_epochs)

    # Initialize GNN network with epochs and input feature-based parameters
    gnn_updates = _init_gnn_feature_count(config.dataset_path)
    gnn_updates["epochs"] = config.gnn_epochs
    _init_gnn_network_script(config.path_to_ai4anim, gnn_updates)


def _init_pae_data_shape(dataset_path: Path, path_to_ai4anim: Path) -> None:
    """
    Initialize PAE data shape by reading sequence count and writing to DataShape.txt.

    :param dataset_path: Path to the dataset directory containing sequences_velocity.txt
    :param path_to_ai4anim: Path to AI4Animation framework
    :raises RuntimeError: If sequence file not found or writing fails
    """
    sequences_file = dataset_path / "sequences_velocity.txt"

    if not sequences_file.exists():
        logger.error(f"Sequences file not found: {sequences_file}")
        raise RuntimeError(f"Sequences file not found: {sequences_file}")

    # Count lines in sequences_velocity.txt
    velocity_samples = count_lines(str(sequences_file))
    logger.info(f"Found {velocity_samples} velocity samples in sequences file")

    # Write to PAE DataShape.txt
    pae_data_shapes_path = path_to_ai4anim / "PAE" / "Dataset" / "DataShape.txt"

    # Create backup if original exists
    if pae_data_shapes_path.exists():
        backup_path = pae_data_shapes_path.with_suffix(pae_data_shapes_path.suffix + '.animhost_backup')
        try:
            backup_content = pae_data_shapes_path.read_text(encoding='utf-8')
            backup_path.write_text(backup_content, encoding='utf-8')
            logger.info(f"Created backup: {backup_path}")
        except Exception as e:
            logger.error(f"Failed to create backup: {e}")
            raise RuntimeError(f"Failed to create backup: {e}")

    # Write new DataShape.txt content
    data_shapes_content = f"{velocity_samples}\n78"

    try:
        pae_data_shapes_path.write_text(data_shapes_content, encoding='utf-8')
        logger.info(f"Successfully wrote PAE DataShape.txt: {velocity_samples} samples, 78 features")
    except Exception as e:
        logger.error(f"Failed to write DataShape.txt: {e}")
        raise RuntimeError(f"Failed to write DataShape.txt: {e}")

def _init_pae_network_script(path_to_ai4anim: Path, pae_epochs: int) -> None:
    """
    Initialize PAE Network.py with epochs.

    :param path_to_ai4anim: Path to AI4Animation framework
    :param pae_epochs: Number of epochs for PAE training
    :raises RuntimeError: If script editing fails
    """
    pae_network_path = path_to_ai4anim / "PAE" / "Network.py"
    current_pae_values = read_script_variables(pae_network_path, ["epochs"])
    current_pae_epochs = current_pae_values.get("epochs")

    error = write_script_variables(pae_network_path, {"epochs": pae_epochs})
    if error:
        logger.error(f"Failed to update PAE epochs: {error}")
        raise RuntimeError(f"Failed to update PAE epochs: {error}")
    logger.info(f"Updated PAE epochs from {current_pae_epochs} to {pae_epochs}")


def _init_gnn_network_script(path_to_ai4anim: Path, updates: Dict[str, Union[str, int]]) -> None:
    """
    Initialize GNN Network.py with all parameter updates in a single operation.

    :param path_to_ai4anim: Path to AI4Animation framework
    :param updates: Dictionary of parameter updates (epochs, gating_indices, main_indices)
    :raises RuntimeError: If script editing fails
    """
    gnn_network_path = path_to_ai4anim / "GNN" / "Network.py"

    # Read current values for logging
    current_values = read_script_variables(gnn_network_path, list(updates.keys()))

    # Apply all updates in a single operation
    error = write_script_variables(gnn_network_path, updates)
    if error:
        logger.error(f"Failed to update GNN Network.py: {error}")
        raise RuntimeError(f"Failed to update GNN Network.py: {error}")

    # Log all changes
    logger.info(f"Successfully updated GNN Network.py with:")
    for key, new_value in updates.items():
        old_value = current_values.get(key)
        logger.info(f"  {key}: {old_value} -> {new_value}")


def _init_gnn_feature_count(dataset_path: Path) -> Dict[str, str]:
    """
    Read dataset metadata and return GNN Network.py parameter updates.

    :param dataset_path: Path to the dataset directory containing metadata.txt
    :return: Dictionary of parameter updates for GNN Network.py
    :raises RuntimeError: If parsing metadata fails
    """
    metadata_file = dataset_path / "metadata.txt"

    if not metadata_file.exists():
        logger.error(f"Metadata file not found: {metadata_file}")
        raise RuntimeError(f"Metadata file not found: {metadata_file}")

    # Parse input/output features from metadata.txt
    input_feature_count, output_feature_count = parse_input_output_features(
        str(metadata_file)
    )
    logger.info(
        f"Parsed features - Input: {input_feature_count}, Output: {output_feature_count}"
    )

    # Generate gating_indices and main_indices tensors
    gating_indices_expr = (
        f"torch.tensor([({input_feature_count} + i) for i in range(130)])"
    )
    main_indices_expr = f"torch.tensor([(0 + i) for i in range({input_feature_count})])"

    logger.info(f"Generated GNN parameter updates:")
    logger.info(f"  gating_indices = {gating_indices_expr}")
    logger.info(f"  main_indices = {main_indices_expr}")

    return {"gating_indices": gating_indices_expr, "main_indices": main_indices_expr}


def reset_model(path_to_ai4anim: Path) -> None:
    """
    Reset PAE and GNN Network.py files and PAE DataShape.txt from their backup copies.

    :param path_to_ai4anim: Path to AI4Animation framework
    :raises RuntimeError: If reset fails
    """
    # Reset PAE Network.py
    pae_network_path = path_to_ai4anim / "PAE" / "Network.py"
    error = reset_script(pae_network_path)
    if error:
        logger.error(f"Failed to reset PAE Network.py: {error}")
        raise RuntimeError(f"Failed to reset PAE Network.py: {error}")
    logger.info("Reset PAE Network.py from backup")

    # Reset PAE DataShape.txt
    pae_data_shapes_path = path_to_ai4anim / "PAE" / "Dataset" / "DataShape.txt"
    backup_path = pae_data_shapes_path.with_suffix(pae_data_shapes_path.suffix + '.animhost_backup')

    if backup_path.exists():
        try:
            backup_content = backup_path.read_text(encoding='utf-8')
            pae_data_shapes_path.write_text(backup_content, encoding='utf-8')
            backup_path.unlink()
            logger.info("Reset PAE DataShape.txt from backup")
        except Exception as e:
            logger.error(f"Failed to reset PAE DataShape.txt: {e}")
            raise RuntimeError(f"Failed to reset PAE DataShape.txt: {e}")
    else:
        logger.warning("No backup found for PAE DataShape.txt")

    # Reset GNN Network.py
    gnn_network_path = path_to_ai4anim / "GNN" / "Network.py"
    error = reset_script(gnn_network_path)
    if error:
        logger.error(f"Failed to reset GNN Network.py: {error}")
        raise RuntimeError(f"Failed to reset GNN Network.py: {error}")
    logger.info("Reset GNN Network.py from backup")


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


def run_pae_training(
    dataset_path: Path, path_to_ai4anim: Path, tracker: ExperimentTracker
) -> None:
    """
    PAE (Phase Autoencoder) training subprocess with real-time parsing.

    Copies training data to PAE directory and launches the PAE Network.py subprocess
    with real-time output parsing for progress updates.

    :param dataset_path: Path to the dataset directory
    :param path_to_ai4anim: Path to the AI4Animation framework directory
    :param tracker: ExperimentTracker instance for logging
    :raises RuntimeError: If PAE training subprocess fails
    """
    tracker.log_ui_status("Starting training 1/2 ...", "Starting PAE training phase...")

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

    # Copy files: p_velocity.bin -> Data.bin, sequences_velocity.txt -> Sequences.txt
    shutil.copyfile(p_velocity_file, pae_dataset_path / "Data.bin")
    shutil.copyfile(sequences_file, pae_dataset_path / "Sequences.txt")

    # Launch PAE Network.py subprocess
    # Use MPLBACKEND=Agg to suppress matplotlib windows because they don't show anything
    run_script_subprocess(
        script_name="Network.py",
        working_dir=pae_path,
        model_name="Encoder",
        line_parser=lambda line, model_name: parse_training_output(
            line, model_name, tracker
        ),
        env_overrides={"MPLBACKEND": "Agg"},
    )


def run_gnn_training(config: StarkeModelConfig, tracker: ExperimentTracker) -> None:
    """
    GNN (Graph Neural Network) training subprocess with real-time parsing.

    Preprocesses motion data using MotionProcessor and launches the GNN Network.py
    subprocess with real-time output parsing for progress updates.

    :param config: StarkeModelConfig containing all training parameters
    :param tracker: ExperimentTracker instance for logging
    :raises RuntimeError: If GNN training subprocess fails
    """
    tracker.log_ui_status("Starting training 2/2 ...", "Starting GNN training phase...")

    # Initialize motion processor
    mp = MotionProcessor(
        str(config.dataset_path), str(config.path_to_ai4anim), config.pae_epochs
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
        line_parser=lambda line, model_name: parse_training_output(
            line, model_name, tracker
        ),
    )


def parse_training_output(
    line: str, model_name: str, tracker: ExperimentTracker
) -> None:
    """
    Parse Network.py output (both PAE and GNN) and emit JSON to stdout.

    Handles consistent output format:

    - "Epoch 1 0.32931875690483264" (epoch + loss)
    - "Progress 23.42 %" (progress percentage)
    - All other lines go getted logged as debug info

    :param line: Output line from Network.py
    :param model_name: "PAE" or "GNN" for prefixing messages
    :param tracker: ExperimentTracker instance for logging
    """
    # Pattern 1: "Epoch 1 0.32931875690483264"
    epoch_pattern = re.compile(r"^Epoch\s+(\d+)\s+([\d.]+)$")
    match = epoch_pattern.search(line)
    if match:
        epoch = int(match.group(1))
        loss = float(match.group(2))
        tracker.log_epoch(
            status=f"{model_name} training",
            metrics={"epoch": epoch, "train_loss": loss},
            text=f"{model_name} epoch {epoch} completed",
        )
        return None

    # Pattern 2: "Progress 23.42 %" (configurable progress updates)
    progress_pattern = re.compile(r"^Progress\s+([\d.]+)\s*%+$")
    match = progress_pattern.search(line)
    if match:
        progress = float(match.group(1))
        tracker.log_percentage_progress(
            f"{model_name} training", progress, f"{model_name} Progress: {progress}%"
        )
        return None

    logger.debug(f"{model_name} output: {line.strip()}")
