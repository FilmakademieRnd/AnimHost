#!/usr/bin/env python3
"""
Minimal tests for example_training - focused on expected JSON output.
Tests real subprocess execution and parsing.
"""

import json
from pathlib import Path

from external.example_training import example_training
from external.script_editing import read_script_variables
from experiment_tracker import ExperimentTracker


def test_end_to_end_training_example(capsys):
    """Verify training modifies script from 3 to 2 epochs, runs 2 epochs, then restores to 3."""
    tracker = ExperimentTracker()

    # Get script path
    script_path = Path(__file__).parent.parent.parent / "external" / "example_training_script.py"

    # Verify original script has 3 epochs
    original_values = read_script_variables(script_path, ["epochs"])
    assert original_values["epochs"] == 3, f"Script should originally have 3 epochs, got {original_values}"

    # Run the example training script with subprocess and output capture
    example_training(tracker)

    # Verify script is restored to 3 epochs after training
    final_values = read_script_variables(script_path, ["epochs"])
    assert final_values["epochs"] == 3, "Script should be restored to 3 epochs after training"

    captured = capsys.readouterr()
    output_lines = [line for line in captured.out.strip().split('\n') if line]

    # Verify we get expected JSON messages
    assert len(output_lines) >= 3  # Starting + 2 epochs + Completed

    # Check starting message
    start_data = json.loads(output_lines[0])
    assert start_data["status"] == "Starting"
    assert "Launching standalone training" in start_data["text"]

    # Check that we get epoch messages (actual values will vary due to randomness)
    epoch_messages = [line for line in output_lines[1:-1] if "training" in json.loads(line)["status"]]
    assert len(epoch_messages) == 2  # Should have 2 epoch messages (not 3)

    for i, epoch_line in enumerate(epoch_messages):
        epoch_data = json.loads(epoch_line)
        assert epoch_data["status"] == "Encoder training"
        assert epoch_data["metrics"]["epoch"] == i + 1
        assert "train_loss" in epoch_data["metrics"]
        assert isinstance(epoch_data["metrics"]["train_loss"], float)
        assert epoch_data["metrics"]["total_epochs"] == 2  # Should run only 2 epochs

    # Check completion message
    completion_data = json.loads(output_lines[-1])
    assert completion_data["status"] == "Completed"
    assert "completed successfully" in completion_data["text"]
