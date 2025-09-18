#!/usr/bin/env python3
"""
Mock Training Script for AnimHost TrainingNode Integration.

Supports both integrated (original) and standalone (subprocess) training modes.
"""

import sys
import os

# Add current directory to path for clean imports
sys.path.insert(0, os.path.dirname(__file__))
from external.example_training import example_training
from experiment_tracker import ExperimentTracker


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

    example_training(tracker)


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
