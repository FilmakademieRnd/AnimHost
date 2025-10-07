#!/usr/bin/env python3
"""
Minimal tests for ExperimentTracker - focused on JSON output verification.
"""

import json
import logging
from experiment_tracker import ExperimentTracker


def test_log_epoch_with_epoch(capsys):
    """Verify JSON output contains epoch metrics when epoch provided."""
    tracker = ExperimentTracker()
    tracker.log_epoch("Training", {"epoch": 1, "loss": 0.5})
    captured = capsys.readouterr()
    
    data = json.loads(captured.out.strip())
    assert data["status"] == "Training"
    assert data["metrics"]["epoch"] == 1
    assert data["metrics"]["loss"] == 0.5


def test_log_epoch_without_epoch(capsys):
    """Verify debug message when epoch missing from metrics."""
    tracker = ExperimentTracker(log_level=logging.DEBUG)
    tracker.log_epoch("Training", {"loss": 0.5})
    captured = capsys.readouterr()
    
    data = json.loads(captured.out.strip())
    assert data["status"] == "DEBUG"
    assert "without epoch field" in data["text"]


def test_log_percentage_progress_enabled(capsys):
    """Verify JSON output when emit_percent_progress=True."""
    tracker = ExperimentTracker(emit_percent_progress=True)
    tracker.log_percentage_progress("Loading", 50.0)
    captured = capsys.readouterr()
    
    data = json.loads(captured.out.strip())
    assert data["status"] == "Loading"
    assert data["metrics"]["percentage"] == 50.0
    assert data["text"] == "Progress: 50.0%"


def test_log_percentage_progress_disabled(capsys):
    """Verify no output when emit_percent_progress=False."""
    tracker = ExperimentTracker(emit_percent_progress=False)
    tracker.log_percentage_progress("Loading", 50.0)
    captured = capsys.readouterr()
    
    assert captured.out == ""


def test_log_ui_status(capsys):
    """Verify JSON output with status and text fields."""
    tracker = ExperimentTracker()
    tracker.log_ui_status("Completed", "Training finished")
    captured = capsys.readouterr()
    
    data = json.loads(captured.out.strip())
    assert data["status"] == "Completed"
    assert data["text"] == "Training finished"


def test_log_std_record_above_threshold(capsys):
    """Verify JSON output when level >= log_level."""
    tracker = ExperimentTracker(log_level=logging.WARNING)
    tracker.log_std_record(logging.ERROR, "Error message")
    captured = capsys.readouterr()
    
    data = json.loads(captured.out.strip())
    assert data["status"] == "ERROR"
    assert data["text"] == "Error message"


def test_log_std_record_below_threshold(capsys):
    """Verify no output when level < log_level."""
    tracker = ExperimentTracker(log_level=logging.ERROR)
    tracker.log_std_record(logging.WARNING, "Warning message")
    captured = capsys.readouterr()
    
    assert captured.out == ""


def test_log_exception(capsys):
    """Verify JSON output with exception details and stderr traceback."""
    tracker = ExperimentTracker()
    
    try:
        raise ValueError("Test error")
    except ValueError as e:
        tracker.log_exception("Test context", e)

    captured = capsys.readouterr()
    
    # Check JSON output
    data = json.loads(captured.out.strip())
    assert data["status"] == "Error"
    assert "Test context: Test error" in data["text"]
    assert data["metrics"]["exception_type"] == "ValueError"
    assert "Traceback" in data["metrics"]["traceback"]
    
    # Check stderr traceback
    assert "EXCEPTION TRACEBACK" in captured.err
    assert "Test context" in captured.err
    assert "ValueError: Test error" in captured.err


def test_logging_capture_debug(capsys):
    """Verify logging.debug() calls are captured and produce JSON output."""
    tracker = ExperimentTracker(capture_stdlib_logging=True, log_level=logging.DEBUG)
    
    # Create a logger and emit debug message
    logger = logging.getLogger("test_logger")
    logger.debug("Debug message")
    
    captured = capsys.readouterr()
    data = json.loads(captured.out.strip())
    assert data["status"] == "DEBUG"
    assert data["text"] == "Debug message"


def test_logging_capture_info(capsys):
    """Verify logging.info() calls are captured and produce JSON output."""
    tracker = ExperimentTracker(capture_stdlib_logging=True, log_level=logging.INFO)
    
    logger = logging.getLogger("test_logger")  
    logger.info("Info message")
    
    captured = capsys.readouterr()
    data = json.loads(captured.out.strip())
    assert data["status"] == "INFO"
    assert data["text"] == "Info message"


def test_logging_capture_warning(capsys):
    """Verify logging.warning() calls are captured and produce JSON output."""
    tracker = ExperimentTracker(capture_stdlib_logging=True, log_level=logging.WARNING)
    
    logger = logging.getLogger("test_logger")
    logger.warning("Warning message")
    
    captured = capsys.readouterr()
    data = json.loads(captured.out.strip())
    assert data["status"] == "WARNING"
    assert data["text"] == "Warning message"


def test_logging_capture_error(capsys):
    """Verify logging.error() calls are captured and produce JSON output."""
    tracker = ExperimentTracker(capture_stdlib_logging=True, log_level=logging.ERROR)
    
    logger = logging.getLogger("test_logger")
    logger.error("Error message")
    
    captured = capsys.readouterr()
    data = json.loads(captured.out.strip())
    assert data["status"] == "ERROR"
    assert data["text"] == "Error message"


def test_multiple_tracker_instances_no_duplication(capsys):
    """Verify creating multiple trackers doesn't cause message duplication."""
    # Create first tracker
    tracker1 = ExperimentTracker(capture_stdlib_logging=True, log_level=logging.INFO)
    
    # Create second tracker (should clean up first)
    tracker2 = ExperimentTracker(capture_stdlib_logging=True, log_level=logging.INFO)
    
    logger = logging.getLogger("test_logger")
    logger.info("Test message")
    
    captured = capsys.readouterr()
    lines = captured.out.strip().split('\n')
    assert len(lines) == 1, f"Expected 1 JSON line with multiple trackers, got {len(lines)}: {lines}"
    
    # Verify it's valid JSON
    data = json.loads(lines[0])
    assert data["status"] == "INFO"
    assert data["text"] == "Test message"
