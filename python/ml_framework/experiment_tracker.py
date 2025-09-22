#!/usr/bin/env python3
"""
ExperimentTracker - Centralized logging for AnimHost training experiments.

Handles JSON communication between Python training scripts and C++ AnimHost. Supports
messages about training progress, epoch metrics, and system status. Captures standard
logging and reroutes it to structured JSON messages.

NOTE: The _emit_json function in this class is solely responsible for ensuring the
implicit JSON interface is consistent with AnimHost MLFrameworkTypes.h json parsing.
"""

import json
import logging
import sys
import traceback
from typing import Dict, Any, Optional


class ExperimentTracker:
    """
    Centralized logger for training experiments with JSON protocol support.

    Outputs logging messages as structured JSON to stdout for parsing by AnimHost.
    Has an init flag to capture standard logging calls and reroute them to JSON stdout.
    """

    _active_handler = None  # Class variable to track current logging handler

    def __init__(
        self,
        capture_stdlib_logging: bool = True,
        log_level: int = logging.ERROR,
        emit_percent_progress: bool = False,
    ):
        """
        Initialize ExperimentTracker.

        :param capture_stdlib_logging: Whether to capture standard logging calls
        :param log_level: Minimum logging level to output to json
        :param emit_percent_progress: Whether to emit percentage progress updates
        """
        self.capture_stdlib_logging = capture_stdlib_logging
        self.log_level = log_level
        self.emit_percent_progress = emit_percent_progress

        # Clean up any previous handler and setup new one if requested
        if self.capture_stdlib_logging:
            self._cleanup_previous_handler()
            self._setup_logging_capture()

    def _cleanup_previous_handler(self) -> None:
        """
        Remove any existing ExperimentLogHandler from root logger.

        This is necessary to avoid duplicate logging if multiple ExperimentTracker
        instances are created with capture_stdlib_logging enabled.
        """
        if ExperimentTracker._active_handler:
            logging.getLogger().removeHandler(ExperimentTracker._active_handler)
            ExperimentTracker._active_handler = None

    def _setup_logging_capture(self) -> None:
        """Setup custom handler to capture standard logging calls."""
        # Create custom handler that routes to our JSON protocols
        handler = ExperimentLogHandler(self)
        handler.setLevel(logging.DEBUG)

        # Get root logger and add our handler
        root_logger = logging.getLogger()
        root_logger.addHandler(handler)
        root_logger.setLevel(logging.DEBUG)

        # Track the active handler
        ExperimentTracker._active_handler = handler

    def log_epoch(
        self, status: str, metrics: Dict[str, Any], text: Optional[str] = None
    ) -> None:
        """
        Emit epoch update message.

        :param status: Status string (e.g. "Training", "<Model> training")
        :param metrics: Metrics dictionary containing epoch, loss, etc.
        :param text: Optional status text. Defaults to "Epoch {epoch} completed" if metrics contains epoch
        """
        # Check if epoch exists in metrics
        if "epoch" not in metrics:
            self.log_std_record(
                logging.DEBUG,
                f"Epoch data '{status}' without epoch field, metrics: {metrics}",
            )
            return

        if text is None:
            text = f"Epoch {metrics['epoch']} completed"

        self._emit_json(status, text, metrics)

    def log_percentage_progress(
        self, status: str, percentage: float, text: Optional[str] = None
    ) -> None:
        """
        Emit percentage progress update message.

        :param status: Status string (e.g. "Initializing", "Loading data", "Epoch 3/10")
        :param percentage: Progress percentage (0-100)
        :param text: Optional status text. Defaults to "Progress: {percentage}%"
        """
        if not self.emit_percent_progress:
            return None

        if text is None:
            text = f"Progress: {percentage}%"

        self._emit_json(status, text, {"percentage": percentage})

    def log_ui_status(self, status: str, text: Optional[str] = "") -> None:
        """
        Emit UI status message that will be displayed to the user.

        :param status: Status string (e.g. "Starting", "Training", "Completed")
        :param text: Optional status text. Defaults to ""
        """
        self._emit_json(status, text)

    def log_std_record(self, level: int, message: str) -> None:
        """
        Log a standard logging record, respecting the log_level setting.

        :param level: Logging level (logging.INFO, logging.WARNING, etc.)
        :param message: The formatted log message
        """
        # Only emit if the level meets our threshold
        if level < self.log_level:
            return

        self.log_ui_status(status=logging.getLevelName(level), text=message)

    def log_exception(self, context: str, exception: Exception) -> None:
        """
        Log an exception with full traceback details.

        :param context: Context description of where the exception occurred
        :param exception: The exception that was caught
        """
        # JSON for C++ parser - include additional fields in the data
        metrics = {
            "exception_type": type(exception).__name__,
            "traceback": traceback.format_exc(),
        }
        self._emit_json("Error", f"{context}: {str(exception)}", metrics)
        # Readable traceback to stderr for debugging
        print(f"\n=== EXCEPTION TRACEBACK ===", file=sys.stderr, flush=True)
        print(f"Context: {context}", file=sys.stderr, flush=True)
        print(
            f"Exception: {type(exception).__name__}: {str(exception)}",
            file=sys.stderr,
            flush=True,
        )
        print(traceback.format_exc(), file=sys.stderr, flush=True)
        print("=" * 30, file=sys.stderr, flush=True)

    def _emit_json(
        self, status: str, text: str, metrics: Optional[Dict[str, Any]] = None
    ) -> None:
        """
        Emit JSON message to stdout with error handling.

        :param status: Status string for the message
        :param text: Text content for the message
        :param metrics: Optional metrics dictionary to include
        """
        try:
            # Build data dictionary with required fields
            data = {"status": status, "text": text}
            if metrics:
                data["metrics"] = metrics

            # Emit compact JSON for C++ parser
            print(json.dumps(data), flush=True)
        except (TypeError, ValueError) as e:
            # Log serialization errors to stderr and continue
            error_msg = f"ExperimentTracker JSON serialization failed: {e}"
            print(error_msg, file=sys.stderr, flush=True)
        except Exception as e:
            # Handle any other errors (stdout unavailable, etc.)
            error_msg = f"ExperimentTracker emit failed: {e}"
            print(error_msg, file=sys.stderr, flush=True)


class ExperimentLogHandler(logging.Handler):
    """Custom logging handler that routes messages through ExperimentTracker."""

    def __init__(self, experiment_logger: ExperimentTracker):
        super().__init__()
        self.experiment_logger = experiment_logger

    def emit(self, record: logging.LogRecord) -> None:
        """
        Handle a logging record by routing through ExperimentTracker protocols.

        :param record: The logging record to handle
        """
        try:
            # Format the message
            message = self.format(record)

            # Route through the ExperimentTracker's log_std_record method
            self.experiment_logger.log_std_record(record.levelno, message)

        except Exception as e:
            # Avoid recursive logging errors
            self.experiment_logger.log_exception("Logging handler emit error", e)
            pass
