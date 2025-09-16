#!/usr/bin/env python3
"""
ExperimentLogger - Centralized logging for AnimHost training experiments.

Handles communication between Python training scripts and C++ AnimHost via JSON messages.
Supports structured protocols for progress updates and UI status messages.
"""

import json
import logging
import sys
import traceback
from typing import Dict, Any, Optional, Union
from enum import Enum


class ProtocolType(Enum):
    """Message protocol types for experiment communication."""
    PROGRESS = "progress"
    UI_STATUS = "ui_status"


class ExperimentLogger:
    """
    Centralized logger for training experiments with JSON protocol support.
    
    Provides structured logging protocols while maintaining backward compatibility
    with existing JSON output format. Captures standard Python logging and routes
    through structured protocols.
    """
    
    def __init__(self, 
                 capture_stdlib_logging: bool = True,
                 log_level: int = logging.ERROR,
                 emit_percent_progress: bool = False,
                 enable_file_logging: bool = False,
                 log_file_path: Optional[str] = None):
        """
        Initialize ExperimentLogger.
        
        :param capture_stdlib_logging: Whether to capture standard logging calls
        :param log_level: Minimum logging level to output to json
        :param enable_file_logging: Enable file logging (placeholder for future)
        :param log_file_path: Path for log file (placeholder for future)
        """
        self.capture_stdlib_logging = capture_stdlib_logging
        self.log_level = log_level
        self.emit_percent_progress = emit_percent_progress
        self.enable_file_logging = enable_file_logging
        self.log_file_path = log_file_path
        
        # Setup logging handler redirection if requested
        if self.capture_stdlib_logging:
            self._setup_logging_capture()
    
    def _setup_logging_capture(self) -> None:
        """Setup custom handler to capture standard logging calls."""
        # Create custom handler that routes to our JSON protocols
        handler = ExperimentLogHandler(self)
        handler.setLevel(logging.INFO)
        
        # Get root logger and add our handler
        root_logger = logging.getLogger()
        root_logger.addHandler(handler)
        root_logger.setLevel(logging.INFO)
    
    def log_progress(self, data: Dict[str, Any]) -> None:
        """
        Emit progress update message.
        
        :param data: Progress data dictionary (epoch, loss, etc.)
        """
        if "progress_percent" in data["metrics"] and not self.emit_percent_progress:
            return None
        self._emit_json(ProtocolType.PROGRESS, data)
    
    def log_ui_status(self, data: Dict[str, Any]) -> None:
        """
        Emit UI status message.
        
        :param data: Status data dictionary (status, text, etc.)
        """
        self._emit_json(ProtocolType.UI_STATUS, data)
    
    def log_std_record(self, level: int, message: str) -> None:
        """
        Log a standard logging record, respecting the log_level setting.
        
        :param level: Logging level (logging.INFO, logging.WARNING, etc.)
        :param message: The formatted log message
        """
        # Only emit if the level meets our threshold
        if level < self.log_level:
            return
            
        # Route based on log level
        if level >= logging.ERROR:
            status_data = {
                "status": "Error",
                "text": message
            }
        elif level >= logging.WARNING:
            status_data = {
                "status": "Warning", 
                "text": message
            }
        else:
            status_data = {
                "status": "Info",
                "text": message
            }
        
        self.log_ui_status(status_data)
    
    def log_exception(self, context: str, exception: Exception) -> None:
        """
        Log an exception with full traceback details.
        
        :param context: Context description of where the exception occurred
        :param exception: The exception that was caught
        """
        error_data = {
            "status": "Error",
            "text": f"{context}: {str(exception)}",
            "exception_type": type(exception).__name__,
            "traceback": traceback.format_exc()
        }
        # JSON for C++ parser
        self.log_ui_status(error_data)
        # Readable traceback to stderr for debugging
        print(f"\n=== EXCEPTION TRACEBACK ===", file=sys.stderr, flush=True)
        print(f"Context: {context}", file=sys.stderr, flush=True)
        print(f"Exception: {type(exception).__name__}: {str(exception)}", file=sys.stderr, flush=True)
        print(traceback.format_exc(), file=sys.stderr, flush=True)
        print("=" * 30, file=sys.stderr, flush=True)
    
    def _emit_json(self, protocol: ProtocolType, data: Dict[str, Any]) -> None:
        """
        Emit JSON message to stdout with error handling.
        
        :param protocol: Protocol type for the message
        :param data: Data to serialize and emit
        """
        try:
            # Emit compact JSON for C++ parser
            print(json.dumps(data), flush=True)
        except (TypeError, ValueError) as e:
            # Log serialization errors to stderr and continue
            error_msg = f"ExperimentLogger JSON serialization failed: {e}"
            print(error_msg, file=sys.stderr, flush=True)
        except Exception as e:
            # Handle any other errors (stdout unavailable, etc.)
            error_msg = f"ExperimentLogger emit failed: {e}"
            print(error_msg, file=sys.stderr, flush=True)
    
    def _log_to_file(self, message: str) -> None:
        """
        Placeholder for file logging functionality.
        
        :param message: Message to log to file
        """
        # TODO: Implement file logging when needed
        pass


class ExperimentLogHandler(logging.Handler):
    """Custom logging handler that routes messages through ExperimentLogger."""
    
    def __init__(self, experiment_logger: ExperimentLogger):
        super().__init__()
        self.experiment_logger = experiment_logger
    
    def emit(self, record: logging.LogRecord) -> None:
        """
        Handle a logging record by routing through ExperimentLogger protocols.
        
        :param record: The logging record to handle
        """
        try:
            # Format the message
            message = self.format(record)
            
            # Route through the ExperimentLogger's log_std_record method
            self.experiment_logger.log_std_record(record.levelno, message)
                
        except Exception:
            # Avoid recursive logging errors
            pass



# Global instance for easy access
_global_logger: Optional[ExperimentLogger] = None


def get_experiment_logger() -> ExperimentLogger:
    """
    Get or create the global ExperimentLogger instance.
    
    :returns: Global ExperimentLogger instance
    """
    global _global_logger
    if _global_logger is None:
        _global_logger = ExperimentLogger()
    return _global_logger


def init_experiment_logger(**kwargs) -> ExperimentLogger:
    """
    Initialize the global ExperimentLogger with custom configuration.
    
    :param kwargs: Configuration arguments for ExperimentLogger
    :returns: Initialized ExperimentLogger instance
    """
    global _global_logger
    _global_logger = ExperimentLogger(**kwargs)
    return _global_logger