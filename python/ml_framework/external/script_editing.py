#!/usr/bin/env python3
"""
Script editing utilities for modifying variable assignments in external Python scripts.

Provides functions to read, write, validate, and reset variables in Python scripts
using AST parsing for robust and reliable modifications.
"""

import ast
import logging
from pathlib import Path
from typing import Optional, Dict, List, Any

logger = logging.getLogger(__name__)


def read_script_variables(script_path: Path, variable_names: List[str]) -> Dict[str, Any]:
    """
    Read current values of specified variables from script.

    Args:
        script_path: Path to the Python script
        variable_names: List of variable names to read

    Returns:
        Dictionary mapping variable names to their values.
        Missing variables get None values.
    """
    if not script_path.exists():
        return {name: None for name in variable_names}

    try:
        with open(script_path, 'r', encoding='utf-8') as f:
            content = f.read()

        tree = ast.parse(content)
        variables = {}

        # Walk through all nodes, including those inside functions
        for node in ast.walk(tree):
            if isinstance(node, ast.Assign):
                for target in node.targets:
                    if isinstance(target, ast.Name) and target.id in variable_names:
                        try:
                            value = ast.literal_eval(node.value)
                            # For multiple assignments, use the last one found
                            variables[target.id] = value
                        except (ValueError, TypeError):
                            variables[target.id] = None

        # Fill in missing variables with None
        for name in variable_names:
            if name not in variables:
                variables[name] = None

        return variables

    except Exception:
        return {name: None for name in variable_names}


def write_script_variables(script_path: Path, updates: Dict[str, Any]) -> Optional[str]:
    """
    Write new values to script variables.

    Args:
        script_path: Path to the Python script
        updates: Dictionary of variable names and their new values

    Returns:
        None on success, error message string on failure.
        Creates .animhost_backup before modification.
    """
    if not script_path.exists():
        return f"Script file not found: {script_path}"

    try:
        with open(script_path, 'r', encoding='utf-8') as f:
            content = f.read()

        # Validate script syntax
        try:
            tree = ast.parse(content)
        except SyntaxError as e:
            return f"Invalid Python syntax in script: {e}"

        # Check for multiple assignments and collect current values
        variable_assignments = {}
        for node in ast.walk(tree):
            if isinstance(node, ast.Assign):
                for target in node.targets:
                    if isinstance(target, ast.Name) and target.id in updates:
                        try:
                            current_value = ast.literal_eval(node.value)
                            if target.id in variable_assignments:
                                # Multiple assignments found
                                prev_value = variable_assignments[target.id]
                                if prev_value != current_value:
                                    return f"Multiple assignments with different values for '{target.id}': {prev_value} and {current_value}"
                                else:
                                    logger.warning(f"Multiple assignments with same value for '{target.id}': {current_value}")
                            variable_assignments[target.id] = current_value
                        except (ValueError, TypeError):
                            pass

        # Check for missing variables
        missing_vars = [var for var in updates.keys() if var not in variable_assignments]
        if missing_vars:
            return f"Variables not found in script: {missing_vars}"

        # Create backup
        backup_path = script_path.with_suffix(script_path.suffix + '.animhost_backup')
        backup_path.write_text(content, encoding='utf-8')

        # Perform replacements
        lines = content.split('\n')

        # Parse again to get line numbers for assignments
        tree = ast.parse(content)
        for node in ast.walk(tree):
            if isinstance(node, ast.Assign):
                for target in node.targets:
                    if isinstance(target, ast.Name) and target.id in updates:
                        line_num = node.lineno - 1  # Convert to 0-based
                        if line_num < len(lines):
                            line = lines[line_num]
                            # Simple replacement for basic assignments
                            if '=' in line and target.id in line:
                                # Handle both top-level and indented assignments
                                parts = line.split('=', 1)
                                var_part = parts[0].strip()
                                if var_part == target.id:
                                    new_value = repr(updates[target.id])
                                    # Preserve original indentation
                                    indent = line[:line.index(target.id)]
                                    lines[line_num] = f"{indent}{target.id} = {new_value}"

        # Write modified content
        modified_content = '\n'.join(lines)
        with open(script_path, 'w', encoding='utf-8') as f:
            f.write(modified_content)

        return None

    except Exception as e:
        return f"Error writing script variables: {e}"


def validate_script_variables(script_path: Path, variable_names: List[str]) -> Optional[str]:
    """
    Check if variables exist and script is valid Python.

    Args:
        script_path: Path to the Python script
        variable_names: List of variable names to check

    Returns:
        None if valid, error message string if issues.
    """
    if not script_path.exists():
        return f"Script file not found: {script_path}"

    try:
        with open(script_path, 'r', encoding='utf-8') as f:
            content = f.read()

        # Check syntax
        try:
            tree = ast.parse(content)
        except SyntaxError as e:
            return f"Invalid Python syntax: {e}"

        # Check for variables (including inside functions)
        found_vars = set()
        for node in ast.walk(tree):
            if isinstance(node, ast.Assign):
                for target in node.targets:
                    if isinstance(target, ast.Name):
                        found_vars.add(target.id)

        missing_vars = [var for var in variable_names if var not in found_vars]
        if missing_vars:
            return f"Variables not found: {missing_vars}"

        return None

    except Exception as e:
        return f"Error validating script: {e}"


def reset_script(script_path: Path) -> Optional[str]:
    """
    Restore script from backup and remove the backup file.

    Args:
        script_path: Path to the Python script to restore

    Returns:
        None on success, error message string on failure.
    """
    backup_path = script_path.with_suffix(script_path.suffix + '.animhost_backup')

    if not backup_path.exists():
        return f"Backup file not found: {backup_path}"

    try:
        backup_content = backup_path.read_text(encoding='utf-8')
        script_path.write_text(backup_content, encoding='utf-8')
        backup_path.unlink()
        return None

    except Exception as e:
        return f"Error restoring from backup: {e}"