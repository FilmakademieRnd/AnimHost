#!/usr/bin/env python3
"""
Script editing utilities for modifying variable assignments in external Python scripts.

Provides functions to read, write, validate, and reset variables in Python scripts
using simple line replacement for improved readability and performance.
"""

import ast
import re
import logging
from pathlib import Path
from typing import Optional, Dict, List, Union, Tuple

logger = logging.getLogger(__name__)


def _find_variable_assignments(content: str, variable_name: str) -> List[Tuple[int, str, str]]:
    """
    Find all assignments of a variable in script content, excluding self-assignments.

    :param content: Script content as string
    :param variable_name: Variable name to search for
    :return: List of (line_index, indent, value_string) tuples, excluding self-assignments
    """
    lines = content.splitlines()
    assignments = []

    # Pattern matches: var_name = value (with optional indentation, exclude comments)
    pattern = rf'^(\s*){re.escape(variable_name)}\s*=\s*([^#]+?)(?:\s*#.*)?$'

    for i, line in enumerate(lines):
        match = re.match(pattern, line)
        if match:
            indent = match.group(1)
            value_str = match.group(2).strip()

            # Exclude self-assignments (e.g., var_name = var_name)
            if value_str == variable_name or value_str.rstrip(',') == variable_name:
                continue
            
            assignments.append((i, indent, value_str))

    return assignments


def _validate_unique_assignments(variable_name: str, assignments: List[Tuple[int, str, str]]) -> Optional[str]:
    """
    Validate that all assignments have the same value.

    :param variable_name: Variable name for error messages
    :param assignments: List of (line_index, indent, value_string) tuples
    :return: Error message if validation fails, None if successful
    """
    if len(assignments) <= 1:
        return None

    # Check for multiple different assignments by comparing value strings
    values = [value_str for _, _, value_str in assignments]
    unique_values = list(set(values))

    if len(unique_values) > 1:
        return f"Multiple assignments with different values for '{variable_name}' in script: {unique_values}"
    elif len(assignments) > 1:
        logger.warning(f"Multiple assignments with same value for '{variable_name}': {unique_values[0]}")

    return None


def read_script_variable_from_content(content: str, variable_name: str) -> Tuple[Optional[str], Optional[str]]:
    """
    Read a single variable from script content with unique validation.

    :param content: Script content as string
    :param variable_name: Variable name to read
    :return: (value_string, error_message) - value is None if not found or error
    """
    assignments = _find_variable_assignments(content, variable_name)

    if not assignments:
        return None, None

    # Validate uniqueness
    error = _validate_unique_assignments(variable_name, assignments)
    if error:
        logger.error(error)
        return None, error

    # Return the last assignment value
    _, _, value_str = assignments[-1]
    return value_str, None


def write_script_variable_to_content(content: str, variable_name: str, value: Union[str, int, float, bool]) -> Tuple[str, Optional[str]]:
    """
    Write a single variable to script content.

    :param content: Script content as string
    :param variable_name: Variable name to write
    :param value: New value (strings treated as expressions, others use repr())
    :return: (new_content, error_message)
    """
    assignments = _find_variable_assignments(content, variable_name)

    if not assignments:
        return content, f"Variable '{variable_name}' not found in script"

    # Validate uniqueness
    error = _validate_unique_assignments(variable_name, assignments)
    if error:
        return content, error

    # Perform line replacements (process in reverse order to maintain line numbers)
    lines = content.splitlines()
    for line_idx, indent, _ in reversed(assignments):
        if isinstance(value, str):
            # String values are treated as expressions
            new_line = f"{indent}{variable_name} = {value}"
        else:
            # Numbers/booleans use repr
            new_line = f"{indent}{variable_name} = {repr(value)}"

        lines[line_idx] = new_line

    # Create modified content
    modified_content = '\n'.join(lines)
    return modified_content, None


def read_script_variables(script_path: Path, variable_names: List[str]) -> Dict[str, Union[str, None]]:
    """
    Read current values of specified variables from script as strings.
    Now includes unique value validation - errors on duplicate assignments with different values.

    :param script_path: Path to the Python script
    :param variable_names: List of variable names to read
    :return: Dictionary mapping variable names to their expression strings. Missing variables get None values.
    """
    if not script_path.exists():
        return {name: None for name in variable_names}

    try:
        content = script_path.read_text(encoding='utf-8')
        variables = {}

        for var_name in variable_names:
            value, error = read_script_variable_from_content(content, var_name)
            if error:
                # Log error and return None for this variable (consistent with original behavior)
                logger.error(f"Error reading variable '{var_name}': {error}")
                variables[var_name] = None
            else:
                variables[var_name] = value

        return variables

    except Exception as e:
        logger.error(f"Error reading script variables: {e}")
        return {name: None for name in variable_names}


def write_script_variables(script_path: Path, updates: Dict[str, Union[str, int, float, bool]]) -> Optional[str]:
    """
    Write new values to script variables using line replacement.

    Strings are treated as expressions and written directly.
    Numbers/booleans are converted using repr().
    Creates .animhost_backup before modification.

    :param script_path: Path to the Python script
    :param updates: Dictionary of variable names and their new values/expressions
    :return: None on success, error message string on failure

    Example:
        write_script_variables(script, {
            "foo": 20,  # becomes: foo = 20
            "bar": "torch.zeros([2, 4], dtype=torch.int32)"  # expression
        })
    """
    if not script_path.exists():
        return f"Script file not found: {script_path}"

    try:
        content = script_path.read_text(encoding='utf-8')

        # Validate original script syntax
        try:
            ast.parse(content)
        except SyntaxError as e:
            return f"Invalid Python syntax in script: {e}"

        # Create backup
        backup_path = script_path.with_suffix(script_path.suffix + '.animhost_backup')
        backup_path.write_text(content, encoding='utf-8')

        # Apply all updates to content
        modified_content = content
        for var_name, value in updates.items():
            new_content, error = write_script_variable_to_content(modified_content, var_name, value)
            if error:
                return error
            modified_content = new_content

        # Write modified content
        script_path.write_text(modified_content, encoding='utf-8')

        # Validate the final result has valid syntax
        try:
            ast.parse(modified_content)
        except SyntaxError as e:
            # Reset script if we generated invalid syntax
            reset_error = reset_script(script_path)
            if reset_error:
                return f"Generated invalid Python syntax and failed to reset: {e}. Reset error: {reset_error}"
            return f"Generated invalid Python syntax and reset script: {e}"

        return None

    except Exception as e:
        return f"Error writing script variables: {e}"



def reset_script(script_path: Path) -> Optional[str]:
    """
    Restore script from backup and remove the backup file.

    :param script_path: Path to the Python script to restore
    :return: None on success, error message string on failure
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


