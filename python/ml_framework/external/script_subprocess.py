#!/usr/bin/env python3
"""
Generic subprocess utilities for running external Python scripts.

Provides utilities for launching Python scripts in subprocesses with real-time
output parsing and monitoring capabilities.
"""

import subprocess
import sys
import os
from pathlib import Path
from typing import Dict, Optional, Callable


def run_script_subprocess(
    script_name: str,
    working_dir: Path,
    model_name: str,
    line_parser: Callable[[str, str], None],
    env_overrides: Optional[Dict[str, str]] = None,
) -> None:
    """
    Generic subprocess runner for scripts with real-time output parsing.

    Launches a Python script in a subprocess and processes output line by line
    using the provided line_parser function. Handles process creation, monitoring,
    and error reporting.

    :param script_name: Name of the Python script to run (e.g., "Network.py")
    :param working_dir: Directory to run the script in
    :param model_name: Display name for progress updates ("Encoder", "Controller", etc.)
    :param line_parser: Function to process each output line (line, model_name) -> None
    :param env_overrides: Optional environment variable overrides
    """
    # Setup environment
    env = os.environ.copy()
    if env_overrides:
        env.update(env_overrides)

    # Launch subprocess
    process = subprocess.Popen(
        [sys.executable, "-u", script_name],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        universal_newlines=True,
        bufsize=1,
        cwd=working_dir,
        env=env,
    )

    # Read output line by line and parse
    while True:
        line = process.stdout.readline()
        if not line and process.poll() is not None:
            break

        line = line.strip()
        if not line:
            continue

        # Process line with provided parser
        line_parser(line, model_name)

    # Wait for process completion
    return_code = process.wait()

    return return_code
