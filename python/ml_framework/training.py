#!/usr/bin/env python3
"""
Entry point for AnimHost TrainingNode Integration or standalone script execution.

Runs the selected experiment and outputs JSON status messages for real-time tracking.
"""

import sys

from config.config_manager import ConfigManager, StarkeModelConfig
from external.starke_training import (
    run_pae_training,
    run_gnn_training,
)
from data.velocity_preprocessing import preprocess_velocity_data
from experiment_tracker import ExperimentTracker


def starke_training(config: StarkeModelConfig, tracker: ExperimentTracker) -> None:
    """
    Main training pipeline - coordinates PAE and GNN training phases.

    Converts LocalAutoPipeline.py workflow to standalone format with JSON output.
    Manages the complete Starke training pipeline including data preprocessing,
    PAE training, and GNN training phases.

    :param config: Configuration object containing training parameters
    :param tracker: ExperimentTracker instance for logging
    :returns: None
    :raises RuntimeError: If any training phase fails
    """

    # Initial status
    tracker.log_ui_status("Initializing", "Initializing Starke training pipeline...")

    try:
        # Data preprocessing phase
        tracker.log_ui_status(
            "Data Preprocessing", "Starting data preprocessing phase..."
        )

        # Data preprocessing (velocity filtering and export)
        preprocess_velocity_data(dataset_path=config.dataset_path)

        # Run PAE training phase
        run_pae_training(
            dataset_path=config.dataset_path,
            path_to_ai4anim=config.path_to_ai4anim,
            tracker=tracker,
        )

        # Run GNN training phase
        run_gnn_training(config, tracker=tracker)

        # Final completion status
        tracker.log_ui_status(
            "Completed Training", "Starke training pipeline completed successfully"
        )

    except Exception as e:
        tracker.log_ui_status("Error", f"Starke training pipeline failed: {str(e)}")


def main() -> None:
    """
    Main entry point - routes to integrated or standalone training based on feature flag.

    Loads configuration from the starke_model_config.json file and initiates
    the Starke training pipeline.

    :returns: None
    :raises Exception: If configuration loading or training fails
    """
    # Create experiment tracker with default configuration
    tracker = ExperimentTracker(
        capture_stdlib_logging=True, emit_percent_progress=False
    )

    config = ConfigManager.load_config("starke_model_config.json")
    starke_training(config, tracker)


if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        tracker = ExperimentTracker()
        tracker.log_exception("Training interrupted by user", KeyboardInterrupt())
        sys.exit(1)
    except Exception as e:
        tracker = ExperimentTracker()
        tracker.log_exception("Training failed", e)
        sys.exit(1)
