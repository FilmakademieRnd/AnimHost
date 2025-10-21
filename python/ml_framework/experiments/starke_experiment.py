#!/usr/bin/env python3
"""
Starke training experiment implementation.

Implements the complete Starke training pipeline using the Experiment interface.
Handles PAE and GNN training phases with proper initialization and cleanup.
"""

import dataclasses
import json
import logging
from pathlib import Path

from .base import Experiment
from config.model_configs import StarkeModelConfig
from experiment_tracker import ExperimentTracker
from external.starke_training import (
    validate_ai4animation_structure,
    init_model,
    reset_model,
    run_pae_training,
    run_gnn_training,
    move_pae_artifacts,
    move_gnn_artifacts,
)
from data.velocity_preprocessing import preprocess_velocity_data

logger = logging.getLogger(__name__)


class StarkeExperiment(Experiment):
    """
    Complete Starke training experiment.

    Manages the full pipeline including PAE and GNN training phases
    with external repo validation, code parameter changes, and cleanup.
    """

    def __init__(self, config: StarkeModelConfig, tracker: ExperimentTracker):
        """
        Initialize StarkeExperiment.

        :param config: Configuration containing all training parameters
        :param tracker: ExperimentTracker for progress logging
        """
        self.config = config
        self.tracker = tracker
        self._initialized = False

    def init(self) -> None:
        """
        Initialize the Starke experiment.

        Validates AI4Animation structure for both PAE and GNN phases
        and adjusts model parameters as per configuration.

        :raises RuntimeError: If validation or initialization fails
        """
        self.tracker.log_ui_status("Initializing", "Initializing Starke training pipeline...")

        # Validate AI4Animation structure for both PAE and GNN phases
        if not validate_ai4animation_structure(self.config.path_to_ai4anim):
            raise RuntimeError("AI4Animation structure validation failed for PAE and/or GNN")

        # Initialize model hyperparameters and dataset parameters
        init_model(self.config)

        self._initialized = True
        self.tracker.log_ui_status("Initialized", "Starke training pipeline initialized successfully.")

    def run(self) -> None:
        """
        Run the complete Starke training pipeline.

        Executes data preprocessing, PAE training, and GNN training phases.
        Assumes init() has been called and validation/initialization is complete.

        :raises RuntimeError: If training fails or init() was not called
        """
        if not self._initialized:
            raise RuntimeError("Must call init() before run()")

        # Data preprocessing phase
        self.tracker.log_ui_status(
            "Data Preprocessing", "Starting data preprocessing phase..."
        )
        preprocess_velocity_data(dataset_path=self.config.dataset_path)

        # Run PAE training phase
        pae_success = run_pae_training(
            config=self.config,
            tracker=self.tracker,
        )

        if not pae_success:
            raise RuntimeError("PAE training failed. See logs for details.")

        # Run GNN training phase
        gnn_success = run_gnn_training(self.config, tracker=self.tracker)

        if not gnn_success:
            raise RuntimeError("GNN training failed. See logs for details.")

        # Final completion status
        self.tracker.log_ui_status(
            "Completed Training", "Starke training pipeline completed successfully"
        )

    def preserve(self) -> None:
        """
        Preserve experiment artifacts for reproducibility.

        Saves the following artifacts to config.run_dir:
        - Input JSON configuration (defines experiment setup)
        - PAE training outputs (moved from AI4Animation/SIGGRAPH_2022/PyTorch/PAE/Training/)
        - GNN training outputs (moved from AI4Animation/SIGGRAPH_2022/PyTorch/GNN/Training/)

        If config.run_dir is not set, preservation is skipped.
        Logs errors but does not raise exceptions to avoid masking training success.
        """
        if not self.config.run_dir:
            self.tracker.log_ui_status("Preservation Disabled", "Artifact preservation disabled (no run_dir configured)")
            return

        self.tracker.log_ui_status("Preserving Artifacts", "Starting artifact preservation...")

        # Save configuration as JSON using dataclasses.asdict()
        try:
            config_path = self.config.run_dir / "config.json"
            with open(config_path, 'w') as f:
                json.dump(dataclasses.asdict(self.config), f, indent=2, default=str)
            logger.info(f"Saved configuration to {config_path}")
        except Exception as e:
            self.tracker.log_exception("Config preservation failed", e)

        # Move PAE and GNN training artifacts
        pae_success = move_pae_artifacts(self.config.path_to_ai4anim, self.config.run_dir)
        gnn_success = move_gnn_artifacts(self.config.path_to_ai4anim, self.config.run_dir)

        if pae_success and gnn_success:
            self.tracker.log_ui_status("Preservation Complete", f"Artifacts preserved to {self.config.run_dir}")
        else:
            self.tracker.log_ui_status("Preservation Partial", "Some artifacts could not be moved (see logs)")


    def cleanup(self) -> None:
        """
        Clean up Starke experiment resources.

        Resets model files to their original state. Safe to call
        even if init() or run() failed.
        """
        try:
            reset_model(self.config.path_to_ai4anim)
            self.tracker.log_ui_status("Completed Experiment", "Starke experiment cleaned up successfully.")
        except Exception as e:
            self.tracker.log_exception("Cleanup failed", e)
