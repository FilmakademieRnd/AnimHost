#!/usr/bin/env python3
"""
Model configuration classes for AnimHost ML Framework.

Defines configuration dataclasses with validation for different training models.
"""

import logging
from dataclasses import dataclass
import os
from pathlib import Path
from typing import Optional


logger = logging.getLogger(__name__)


@dataclass
class StarkeModelConfig:
    """
    Configuration for Starke model training pipeline.

    Contains all necessary parameters for configuring the Starke training pipeline,
    including dataset paths, AI4Animation framework location, and training parameters.
    """

    dataset_path: Path
    path_to_ai4anim: Path
    pae_epochs: int
    pae_learning_rate: float
    gnn_epochs: int
    gnn_learning_rate: float
    gnn_dropout: float
    run_dir: Optional[Path]

    def __post_init__(self) -> None:
        """Convert string paths to Path objects if needed."""
        if isinstance(self.dataset_path, str):
            self.dataset_path = Path(self.dataset_path)
        if isinstance(self.path_to_ai4anim, str):
            self.path_to_ai4anim = Path(self.path_to_ai4anim)
        if isinstance(self.run_dir, str):
            self.run_dir = Path(self.run_dir)

    def validate(self) -> Optional[str]:
        """
        Validates the configuration parameters.

        Checks that all required directories exist and parameters are within
        valid ranges.

        :returns: None if valid, error message string if invalid
        """
        # Validate dataset path exists
        if not self.dataset_path.exists():
            return f"Dataset path does not exist: {self.dataset_path}"
        if not self.dataset_path.is_dir():
            return f"Dataset path is not a directory: {self.dataset_path}"

        # Validate AI4Animation path exists
        if not self.path_to_ai4anim.exists():
            return f"AI4Animation path does not exist: {self.path_to_ai4anim}"
        if not self.path_to_ai4anim.is_dir():
            return f"AI4Animation path is not a directory: {self.path_to_ai4anim}"
        if not os.access(self.path_to_ai4anim, os.R_OK):
            return f"AI4Animation path is not readable: {self.path_to_ai4anim}"

        # Validate epochs > 0
        if self.pae_epochs <= 0:
            return f"PAE epochs must be greater than 0, got: {self.pae_epochs}"
        if self.gnn_epochs <= 0:
            return f"GNN epochs must be greater than 0, got: {self.gnn_epochs}"

        # Validate learning rates > 0
        if self.pae_learning_rate <= 0:
            return f"PAE learning rate must be greater than 0, got: {self.pae_learning_rate}"
        if self.gnn_learning_rate <= 0:
            return f"GNN learning rate must be greater than 0, got: {self.gnn_learning_rate}"

        # Validate dropout between 0 and 1
        if not (0 <= self.gnn_dropout <= 1):
            return f"GNN dropout must be between 0 and 1, got: {self.gnn_dropout}"
        # Validate run_dir if provided
        if self.run_dir is not None:
            if not self.run_dir.exists():
                return f"Run directory does not exist: {self.run_dir}"
            if not self.run_dir.is_dir():
                return f"Run directory path is not a directory: {self.run_dir}"
            if not os.access(self.run_dir, os.W_OK):
                return f"Run directory is not writable: {self.run_dir}"

        return None
