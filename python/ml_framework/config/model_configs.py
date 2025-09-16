#!/usr/bin/env python3
"""
Model configuration classes for AnimHost ML Framework.

Defines configuration dataclasses with validation for different training models.
"""

import logging
from dataclasses import dataclass
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

    dataset_path: str
    path_to_ai4anim: str
    pae_epochs: int

    def validate(self) -> Optional[str]:
        """
        Validates the configuration parameters.
        
        Checks that all required directories exist and parameters are within
        valid ranges.
        
        :returns: None if valid, error message string if invalid
        """
        # Validate dataset path exists
        dataset_dir = Path(self.dataset_path)
        if not dataset_dir.exists():
            return f"Dataset path does not exist: {self.dataset_path}"

        if not dataset_dir.is_dir():
            return f"Dataset path is not a directory: {self.dataset_path}"

        # Validate AI4Animation path exists
        ai4anim_dir = Path(self.path_to_ai4anim)
        if not ai4anim_dir.exists():
            return f"AI4Animation path does not exist: {self.path_to_ai4anim}"

        if not ai4anim_dir.is_dir():
            return f"AI4Animation path is not a directory: {self.path_to_ai4anim}"

        # Validate epochs > 0
        if self.pae_epochs <= 0:
            return f"PAE epochs must be greater than 0, got: {self.pae_epochs}"

        return None
