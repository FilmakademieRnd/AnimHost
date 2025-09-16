#!/usr/bin/env python3
"""
Model configuration classes for AnimHost ML Framework.

Defines configuration dataclasses with validation for different training models.
"""

import logging
from dataclasses import dataclass
from pathlib import Path
from typing import Optional, Union


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
    processed_data_path: Path

    def __post_init__(self) -> None:
        """Convert string paths to Path objects if needed."""
        if isinstance(self.dataset_path, str):
            self.dataset_path = Path(self.dataset_path)
        if isinstance(self.path_to_ai4anim, str):
            self.path_to_ai4anim = Path(self.path_to_ai4anim)
        if isinstance(self.processed_data_path, str):
            self.processed_data_path = Path(self.processed_data_path)

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

        # Validate epochs > 0
        if self.pae_epochs <= 0:
            return f"PAE epochs must be greater than 0, got: {self.pae_epochs}"

        # Validate processed data path exists
        if not self.processed_data_path.exists():
            return f"Processed data path does not exist: {self.processed_data_path}"

        if not self.processed_data_path.is_dir():
            return f"Processed data path is not a directory: {self.processed_data_path}"

        return None
