#!/usr/bin/env python3
"""
Mock Training Script for AnimHost TrainingNode Integration.

Supports both integrated (original) and standalone (subprocess) training modes.
"""

import json
import time
import sys
import random
import subprocess
import re
import os
from pathlib import Path

# Add current directory to path for clean imports
sys.path.insert(0, os.path.dirname(__file__))
from config.config_manager import ConfigManager, StarkeModelConfig
from external.starke_training import (
    run_pae_training,
    run_gnn_training,
)
from data.velocity_preprocessing import preprocess_velocity_data
from experiment_logger import get_experiment_logger


def integrated_training() -> None:
    """
    Mock training script that outputs JSON progress updates.

    Simulates a training process with the following characteristics:

    - 5 epochs total
    - Updates every 0.5 seconds
    - Outputs: epoch, train_loss, val_loss, learning_rate

    :returns: None
    """
    exp_logger = get_experiment_logger()

    # Initial status, to confirm script start
    status = {"status": "Starting", "text": "Initializing training process..."}
    exp_logger.log_ui_status(status)

    total_epochs = 5

    # Simulate training epochs
    for epoch in range(1, total_epochs + 1):
        time.sleep(0.5)  # Simulate processing time

        # Mock training metrics
        train_loss = 1.0 - (epoch * 0.15) + random.uniform(-0.05, 0.05)
        val_loss = train_loss + random.uniform(0.01, 0.1)
        learning_rate = 0.001 * (0.9 ** (epoch - 1))

        progress = {
            "status": "Training",
            "text": f"Training epoch {epoch}/{total_epochs}",
            "metrics": {
                "epoch": epoch,
                "total_epochs": total_epochs,
                "train_loss": round(train_loss, 4),
                "val_loss": round(val_loss, 4),
                "learning_rate": round(learning_rate, 6),
            },
        }

        exp_logger.log_progress(progress)

    # Final completion status
    completion_status = {
        "status": "Completed",
        "text": f"Training completed successfully after {total_epochs} epochs",
        "metrics": {
            "final_train_loss": round(train_loss, 4),
            "final_val_loss": round(val_loss, 4),
            "total_epochs": total_epochs,
        },
    }
    exp_logger.log_ui_status(completion_status)


def standalone_training() -> None:
    """
    Runs mock_standalone_training.py as subprocess and parses its text output.

    Converts parsed data to JSON format for TrainingNode integration.
    Launches the standalone training script in a subprocess and monitors
    its output in real-time.

    :returns: None
    """
    exp_logger = get_experiment_logger()

    # Initial status
    status = {"status": "Starting", "text": "Launching standalone training process..."}
    exp_logger.log_ui_status(status)

    # Get path to standalone_training.py
    script_dir = Path(__file__).parent
    standalone_script = script_dir / "external" / "mock_standalone_training.py"

    try:
        # Launch subprocess with real-time output parsing
        process = subprocess.Popen(
            [sys.executable, "-u", standalone_script],  # -u for unbuffered output
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            universal_newlines=True,
            bufsize=1,  # Flush line by line
        )

        total_epochs = 5
        current_epoch = 0

        # Regex patterns for parsing
        epoch_pattern = re.compile(r"Epoch\s+(\d+)\s+([\d.]+)")

        # Read output line by line
        while True:
            line = process.stdout.readline()
            if not line and process.poll() is not None:
                break

            line = line.strip()
            if not line:
                continue

            # Parse epoch completion
            epoch_match = epoch_pattern.search(line)
            if epoch_match:
                current_epoch = int(epoch_match.group(1))
                train_loss = float(epoch_match.group(2))

                epoch_status = {
                    "status": "Training",
                    "text": f"Standalone training epoch {current_epoch}/{total_epochs}",
                    "metrics": {
                        "epoch": current_epoch,
                        "total_epochs": total_epochs,
                        "train_loss": round(train_loss, 4),
                    },
                }
                exp_logger.log_progress(epoch_status)

        # Wait for process completion
        return_code = process.wait()

        if return_code == 0:
            # Success status
            completion_status = {
                "status": "Completed",
                "text": f"Standalone training completed successfully after {total_epochs} epochs",
                "metrics": {"total_epochs": total_epochs},
            }
            exp_logger.log_ui_status(completion_status)
        else:
            # Process failed
            stderr_output = process.stderr.read()
            error_status = {
                "status": "Error",
                "text": f"Standalone training failed with return code {return_code}: {stderr_output}",
            }
            exp_logger.log_ui_status(error_status)

    except FileNotFoundError:
        error_status = {
            "status": "Error",
            "text": f"Standalone training script not found: {standalone_script}",
        }
        exp_logger.log_ui_status(error_status)
    except Exception as e:
        error_status = {
            "status": "Error",
            "text": f"Failed to launch standalone training: {str(e)}",
        }
        exp_logger.log_ui_status(error_status)


def starke_training(config: StarkeModelConfig) -> None:
    """
    Main training pipeline - coordinates PAE and GNN training phases.

    Converts LocalAutoPipeline.py workflow to standalone format with JSON output.
    Manages the complete Starke training pipeline including data preprocessing,
    PAE training, and GNN training phases.

    :param config: Configuration object containing training parameters
    :returns: None
    :raises RuntimeError: If any training phase fails
    """
    exp_logger = get_experiment_logger()

    # Initial status
    status = {
        "status": "Initializing",
        "text": "Initializing Starke training pipeline...",
    }
    exp_logger.log_ui_status(status)

    try:
        # Data preprocessing phase
        preprocessing_status = {
            "status": "Data Preprocessing",
            "text": "Starting data preprocessing phase...",
        }
        exp_logger.log_ui_status(preprocessing_status)

        # Data preprocessing (velocity filtering and export)
        preprocess_velocity_data(dataset_path=config.dataset_path)

        # Run PAE training phase
        run_pae_training(
            dataset_path=config.dataset_path, path_to_ai4anim=config.path_to_ai4anim
        )

        # Run GNN training phase
        run_gnn_training(config)

        # Final completion status
        completion_status = {
            "status": "Completed Training",
            "text": "Starke training pipeline completed successfully",
        }
        exp_logger.log_ui_status(completion_status)

    except Exception as e:
        error_status = {
            "status": "Error",
            "text": f"Starke training pipeline failed: {str(e)}",
        }
        exp_logger.log_ui_status(error_status)


def main() -> None:
    """
    Main entry point - routes to integrated or standalone training based on feature flag.

    Loads configuration from the starke_model_config.json file and initiates
    the Starke training pipeline.

    :returns: None
    :raises Exception: If configuration loading or training fails
    """
    config = ConfigManager.load_config("starke_model_config.json")
    starke_training(config)
    # integrated_training()
    # standalone_training()


if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        exp_logger = get_experiment_logger()
        error_status = {"status": "Interrupted", "text": "Training interrupted by user"}
        exp_logger.log_ui_status(error_status)
        sys.exit(1)
    except Exception as e:
        exp_logger = get_experiment_logger()
        error_status = {"status": "Error", "text": f"Training failed: {str(e)}"}
        exp_logger.log_ui_status(error_status)
        sys.exit(1)
