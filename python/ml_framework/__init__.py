# ML Framework package for AnimHost training pipeline

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