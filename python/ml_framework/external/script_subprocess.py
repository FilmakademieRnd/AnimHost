#!/usr/bin/env python3
"""
Generic subprocess utilities for running external Python scripts.

Provides utilities for launching Python scripts in subprocesses with real-time
output parsing and monitoring capabilities.
"""

import subprocess
import sys
import os
import logging
from pathlib import Path
from typing import Dict, Optional, Callable, Tuple

logger = logging.getLogger(__name__)


def run_script_subprocess(
    script_name: str,
    working_dir: Path,
    model_name: str,
    line_parser: Callable[[str, str], None],
    env_overrides: Optional[Dict[str, str]] = None,
) -> Tuple[int, str]:
    """
    Generic subprocess runner for scripts with real-time output parsing.

    Launches a Python script in a subprocess and processes stdout line by line
    in real-time using the provided line_parser function. Reads stderr after
    process completion.

    :param script_name: Name of the Python script to run (e.g., "Network.py")
    :param working_dir: Directory to run the script in
    :param model_name: Display name for progress updates ("Encoder", "Controller", etc.)
    :param line_parser: Function to process each output line (line, model_name) -> None
    :param env_overrides: Optional environment variable overrides
    :return: Tuple of (return_code, stderr_text)
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

    # Read stdout line by line in real-time
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

    # Read all stderr after completion
    stderr_text = process.stderr.read()

    # Log stderr if present
    if stderr_text:
        for line in stderr_text.strip().split('\n'):
            if line:
                logger.error(f"{model_name} stderr: {line}")

    return return_code, stderr_text
