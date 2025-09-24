#!/usr/bin/env python3
"""
Base experiment class for AnimHost ML Framework.

Defines the standard interface for all experiments with init/run/cleanup phases.
"""

from abc import ABC, abstractmethod


class Experiment(ABC):
    """
    Abstract base class for all ML training experiments.

    Experiments follow a three-phase lifecycle:
    1. init() - Setup and validation
    2. run() - Execute the training pipeline
    3. cleanup() - Reset state and cleanup resources
    """

    @abstractmethod
    def init(self) -> None:
        """
        Initialize the experiment.

        This phase should handle:
        - Validation of required resources
        - Model initialization and configuration
        - Any setup that might fail

        :raises RuntimeError: If initialization fails
        """
        pass

    @abstractmethod
    def run(self) -> None:
        """
        Run the main experiment.

        This phase should handle:
        - Data preprocessing
        - Model training
        - Progress tracking

        :raises RuntimeError: If training fails
        """
        pass

    @abstractmethod
    def cleanup(self) -> None:
        """
        Clean up experiment resources.

        This phase should handle:
        - Resetting modified files
        - Cleaning up temporary resources
        - Should work even if init() or run() failed

        Note: Cleanup should be non-fatal - use try/except to log
        warnings rather than raising exceptions that could mask
        the main operation's success or failure.
        """
        pass
