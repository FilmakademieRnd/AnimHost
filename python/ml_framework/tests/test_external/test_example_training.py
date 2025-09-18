#!/usr/bin/env python3
"""
Minimal tests for example_training - focused on expected JSON output.
"""

import json
from unittest.mock import patch, MagicMock
from pathlib import Path

from external.example_training import example_training
from experiment_tracker import ExperimentTracker


def test_example_training_success(capsys):
    """Verify successful training outputs expected JSON messages."""
    tracker = ExperimentTracker()
    
    # Mock the subprocess to simulate successful training
    with patch('external.script_subprocess.run_script_subprocess') as mock_run:
        # Configure mock to call the line parser with expected output
        def mock_subprocess(script_name, working_dir, model_name, line_parser, env_overrides=None):
            # Simulate the training script output
            line_parser("Epoch 1 0.8500", "Encoder")
            line_parser("Epoch 2 0.7500", "Encoder")
        
        mock_run.side_effect = mock_subprocess
        
        # Run the training
        example_training(tracker)
        
        # Verify subprocess was called correctly
        mock_run.assert_called_once()
        args = mock_run.call_args
        assert args[1]['script_name'] == "example_training_script.py"
        assert args[1]['model_name'] == "Encoder"
    
    captured = capsys.readouterr()
    output_lines = [line for line in captured.out.strip().split('\n') if line]
    
    # Verify we get expected JSON messages
    assert len(output_lines) >= 3  # Starting + 2 epochs + Completed
    
    # Check starting message
    start_data = json.loads(output_lines[0])
    assert start_data["status"] == "Starting"
    assert "Launching standalone training" in start_data["text"]
    
    # Check epoch messages
    epoch1_data = json.loads(output_lines[1])
    assert epoch1_data["status"] == "Encoder training"
    assert epoch1_data["metrics"]["epoch"] == 1
    assert epoch1_data["metrics"]["train_loss"] == 0.85
    
    epoch2_data = json.loads(output_lines[2])
    assert epoch2_data["status"] == "Encoder training"
    assert epoch2_data["metrics"]["epoch"] == 2
    assert epoch2_data["metrics"]["train_loss"] == 0.75
    
    # Check completion message
    completion_data = json.loads(output_lines[-1])
    assert completion_data["status"] == "Completed"
    assert "completed successfully" in completion_data["text"]


def test_example_training_script_not_found(capsys):
    """Verify FileNotFoundError handling."""
    tracker = ExperimentTracker()
    
    # Mock subprocess to raise FileNotFoundError
    with patch('external.script_subprocess.run_script_subprocess') as mock_run:
        mock_run.side_effect = FileNotFoundError("Script not found")
        
        # Run the training
        example_training(tracker)
    
    captured = capsys.readouterr()
    output_lines = [line for line in captured.out.strip().split('\n') if line]
    
    # Should get starting message and error message
    assert len(output_lines) >= 2
    
    # Check starting message
    start_data = json.loads(output_lines[0])
    assert start_data["status"] == "Starting"
    
    # Check error message - should be log_exception output
    error_data = json.loads(output_lines[1])
    assert error_data["status"] == "Error"
    assert "Standalone training script not found" in error_data["text"]
    assert error_data["metrics"]["exception_type"] == "FileNotFoundError"