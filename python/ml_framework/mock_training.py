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
from experiment_tracker import get_experiment_tracker


def integrated_training() -> None:
    """
    Mock training script that outputs JSON progress updates.

    Simulates a training process with the following characteristics:

    - 5 epochs total
    - Updates every 0.5 seconds
    - Outputs: epoch, train_loss, val_loss, learning_rate

    :returns: None
    """
    exp_tracker = get_experiment_tracker()

    # Initial status, to confirm script start
    exp_tracker.log_ui_status("Starting", "Initializing training process...")

    total_epochs = 5

    # Simulate training epochs
    for epoch in range(1, total_epochs + 1):
        time.sleep(0.5)  # Simulate processing time

        # Mock training metrics
        train_loss = 1.0 - (epoch * 0.15) + random.uniform(-0.05, 0.05)
        val_loss = train_loss + random.uniform(0.01, 0.1)
        learning_rate = 0.001 * (0.9 ** (epoch - 1))

        # Log epoch data with metrics
        metrics = {
            "epoch": epoch,
            "total_epochs": total_epochs,
            "train_loss": round(train_loss, 4),
            "val_loss": round(val_loss, 4),
            "learning_rate": round(learning_rate, 6),
        }
        exp_tracker.log_epoch(
            status="Training",
            metrics=metrics,
            text=f"Training epoch {epoch}/{total_epochs}",
        )

        # Log percentage progress
        percentage = (epoch / total_epochs) * 100
        exp_tracker.log_percentage_progress(
            status="Training",
            percentage=percentage,
            text=f"Training epoch {epoch}/{total_epochs}",
        )

    # Final completion status
    exp_tracker.log_ui_status(
        "Completed", f"Training completed successfully after {total_epochs} epochs"
    )


def standalone_training() -> None:
    """
    Runs mock_standalone_training.py as subprocess and parses its text output.

    Converts parsed data to JSON format for TrainingNode integration.
    Launches the standalone training script in a subprocess and monitors
    its output in real-time.

    :returns: None
    """
    exp_tracker = get_experiment_tracker()

    # Initial status
    exp_tracker.log_ui_status("Starting", "Launching standalone training process...")

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

                metrics = {
                    "epoch": current_epoch,
                    "total_epochs": total_epochs,
                    "train_loss": round(train_loss, 4),
                }
                exp_tracker.log_epoch(
                    "Training",
                    metrics,
                    f"Standalone training epoch {current_epoch}/{total_epochs}",
                )

        # Wait for process completion
        return_code = process.wait()

        if return_code == 0:
            # Success status
            exp_tracker.log_ui_status(
                "Completed",
                f"Standalone training completed successfully after {total_epochs} epochs",
            )
        else:
            # Process failed
            stderr_output = process.stderr.read()
            exp_tracker.log_ui_status(
                "Error",
                f"Standalone training failed with return code {return_code}: {stderr_output}",
            )

    except FileNotFoundError:
        exp_tracker.log_ui_status(
            "Error", f"Standalone training script not found: {standalone_script}"
        )
    except Exception as e:
        exp_tracker.log_ui_status(
            "Error", f"Failed to launch standalone training: {str(e)}"
        )


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
    exp_tracker = get_experiment_tracker()

    # Initial status
    exp_tracker.log_ui_status(
        "Initializing", "Initializing Starke training pipeline..."
    )

    try:
        # Data preprocessing phase
        exp_tracker.log_ui_status(
            "Data Preprocessing", "Starting data preprocessing phase..."
        )

        # Data preprocessing (velocity filtering and export)
        preprocess_velocity_data(dataset_path=config.dataset_path)

        # Run PAE training phase
        run_pae_training(
            dataset_path=config.dataset_path, path_to_ai4anim=config.path_to_ai4anim
        )

        # Run GNN training phase
        run_gnn_training(config)

        # Final completion status
        exp_tracker.log_ui_status(
            "Completed Training", "Starke training pipeline completed successfully"
        )

    except Exception as e:
        exp_tracker.log_ui_status("Error", f"Starke training pipeline failed: {str(e)}")


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
        exp_tracker = get_experiment_tracker()
        exp_tracker.log_ui_status("Interrupted", "Training interrupted by user")
        sys.exit(1)
    except Exception as e:
        exp_tracker = get_experiment_tracker()
        exp_tracker.log_ui_status("Error", f"Training failed: {str(e)}")
        sys.exit(1)
