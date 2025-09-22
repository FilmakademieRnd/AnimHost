#!/usr/bin/env python3
"""
pytest configuration for ml_framework tests.
"""

import sys
from pathlib import Path

# Add ml_framework to path for clean imports when running tests from outside the tests directory
sys.path.insert(0, str(Path(__file__).parent.parent))
