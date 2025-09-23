#!/usr/bin/env python3
"""
Entry point for AnimHost TrainingNode Integration or standalone script execution.

Runs the selected experiment and outputs JSON status messages for real-time tracking.
"""

import sys

from config.config_manager import ConfigManager, StarkeModelConfig
from experiments.starke_experiment import StarkeExperiment
from experiment_tracker import ExperimentTracker


def main() -> None:
    """
    Main entry point for StarkeExperiment training.

    Loads configuration from starke_model_config.json and runs the complete
    Starke training pipeline using the StarkeExperiment class with proper
    initialization, execution, and cleanup phases.

    :returns: None
    :raises Exception: If configuration loading or training fails
    """
    # Create experiment tracker with default configuration
    tracker = ExperimentTracker(
        capture_stdlib_logging=True, emit_percent_progress=False
    )

    experiment = None
    try:
        config = ConfigManager.load_config("starke_model_config.json")
        experiment = StarkeExperiment(config, tracker)
        experiment.init()
        experiment.run()
    except Exception as e:
        tracker.log_exception("Starke training failed", e)
        raise
    finally:
        if experiment is not None:
            experiment.cleanup()


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
