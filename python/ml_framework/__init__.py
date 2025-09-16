"""
ML Framework package for AnimHost training pipeline.

Provides machine learning training capabilities for the AnimHost animation system,
including configuration management, training orchestration, and external process
integration.

This package automatically configures the Python import path to ensure proper
access to all ml_framework modules.
"""

import sys
import os
from pathlib import Path

# Automatically set up Python path for ml_framework imports
# Find the python/ directory from this file's location
current_file = Path(__file__).resolve()
python_dir = current_file.parent.parent  # python/ml_framework/__init__.py -> python/

# Add python directory to sys.path if not already present
python_dir_str = str(python_dir)
if python_dir_str not in sys.path:
    sys.path.insert(0, python_dir_str)