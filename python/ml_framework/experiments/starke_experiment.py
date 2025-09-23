#!/usr/bin/env python3
"""
Starke training experiment implementation.

Implements the complete Starke training pipeline using the Experiment interface.
Handles PAE and GNN training phases with proper initialization and cleanup.
"""

import logging

from .base import Experiment
from config.model_configs import StarkeModelConfig
from experiment_tracker import ExperimentTracker
from external.starke_training import (
    validate_ai4animation_structure,
    init_model,
    reset_model,
    run_pae_training,
    run_gnn_training,
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

        # Validate AI4Animation structure for both phases
        if not validate_ai4animation_structure(self.config.path_to_ai4anim, "PAE"):
            raise RuntimeError("PAE training structure validation failed")
        if not validate_ai4animation_structure(self.config.path_to_ai4anim, "GNN"):
            raise RuntimeError("GNN training structure validation failed")

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
        run_pae_training(
            dataset_path=self.config.dataset_path,
            path_to_ai4anim=self.config.path_to_ai4anim,
            tracker=self.tracker,
        )

        # Run GNN training phase
        run_gnn_training(self.config, tracker=self.tracker)

        # Final completion status
        self.tracker.log_ui_status(
            "Completed Training", "Starke training pipeline completed successfully"
        )

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
