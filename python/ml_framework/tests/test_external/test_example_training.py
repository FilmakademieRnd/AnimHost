#!/usr/bin/env python3
"""
Minimal tests for example_training - focused on expected JSON output.
Tests real subprocess execution and parsing.
"""

import json

from external.example_training import example_training
from experiment_tracker import ExperimentTracker


def test_example_training_success(capsys):
    """Verify successful training outputs expected JSON messages with real subprocess."""
    tracker = ExperimentTracker()
    
    # Run the example training script with subprocess and output capture
    example_training(tracker)
    
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
    assert len(epoch_messages) == 2  # Should have 2 epoch messages
    
    for i, epoch_line in enumerate(epoch_messages):
        epoch_data = json.loads(epoch_line)
        assert epoch_data["status"] == "Encoder training"
        assert epoch_data["metrics"]["epoch"] == i + 1
        assert "train_loss" in epoch_data["metrics"]
        assert isinstance(epoch_data["metrics"]["train_loss"], float)
        assert epoch_data["metrics"]["total_epochs"] == 2
    
    # Check completion message
    completion_data = json.loads(output_lines[-1])
    assert completion_data["status"] == "Completed"
    assert "completed successfully" in completion_data["text"]
