#!/usr/bin/env python3
"""
Example Training Module for AnimHost TrainingNode Integration.

Contains standalone training functions that demonstrate subprocess integration
using the script_subprocess utility.
"""

import re
from pathlib import Path

from .script_subprocess import run_script_subprocess
from .script_editing import write_script_variables, reset_script
from experiment_tracker import ExperimentTracker


def example_training(tracker: ExperimentTracker) -> None:
    """
    Runs example_training_script.py as subprocess using script_subprocess utility.

    Uses the generic script_subprocess utility to launch and monitor the standalone
    training script, with real-time output parsing through a line parser function.

    Temporarily modifies the script to run 2 epochs instead of the default 3,
    then restores the original script afterward.

    :param tracker: ExperimentTracker instance for logging
    :returns: None
    """

    # Initial status
    tracker.log_ui_status("Starting", "Launching standalone training process...")

    # Get script directory and working directory
    script_dir = Path(__file__).parent
    script_name = "example_training_script.py"
    script_path = script_dir / script_name

    # State variables for parsing
    total_epochs = 2
    current_epoch = 0
    
    # Regex patterns for parsing
    epoch_pattern = re.compile(r"Epoch\s+(\d+)\s+([\d.]+)")

    def parse_training_output(line: str, model_name: str = "Encoder") -> None:
        """Parse training output lines and update tracker."""
        nonlocal current_epoch
        
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
            tracker.log_epoch(
                status=f"{model_name} training",
                metrics=metrics,
                text=f"{model_name} training epoch {current_epoch}/{total_epochs}",
            )

    try:
        # Temporarily modify script to use 2 epochs instead of 3
        error = write_script_variables(script_path, {"epochs": total_epochs})
        if error:
            tracker.log_ui_status("Error", error)
            return

        # Use script_subprocess utility to run the training
        run_script_subprocess(
            script_name=script_name,
            working_dir=script_dir,
            model_name="Encoder",
            line_parser=parse_training_output
        )
    except FileNotFoundError:
        tracker.log_exception("Standalone training script not found", FileNotFoundError())
    except Exception as e:
        tracker.log_exception("Standalone training failed", e)
    finally:
        # Always restore the original script
        reset_error = reset_script(script_path)
        if reset_error:
            tracker.log_ui_status("Error", reset_error)

    tracker.log_ui_status("Completed", f"Training completed successfully after {total_epochs} epochs")
