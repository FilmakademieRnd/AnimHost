#!/usr/bin/env python3
"""
Tests for script_editing module.
"""

import pytest
import tempfile
import textwrap
from pathlib import Path

from external.script_editing import (
    read_script_variables,
    write_script_variables,
    validate_script_variables,
    reset_script,
)


@pytest.fixture
def temp_script():
    """Create a temporary script for testing."""
    content = textwrap.dedent("""
        #!/usr/bin/env python3
        # Test script
        epochs = 10
        batch_size = 32
        learning_rate = 0.001
    """).strip()
    with tempfile.NamedTemporaryFile(
        mode="w", suffix=".py", delete=False, encoding="utf-8"
    ) as f:
        f.write(content)
        temp_path = Path(f.name)

    yield temp_path

    # Cleanup
    temp_path.unlink(missing_ok=True)
    backup_path = temp_path.with_suffix(temp_path.suffix + ".animhost_backup")
    backup_path.unlink(missing_ok=True)


@pytest.fixture
def complex_script():
    """Create a script with complex assignments for testing."""
    content = textwrap.dedent("""
        #!/usr/bin/env python3
        import os

        # Simple assignments
        epochs = 5
        batch_size = 64

        # Other code
        def some_function():
            return 42

        # More assignments
        learning_rate = 0.01
    """).strip()
    with tempfile.NamedTemporaryFile(
        mode="w", suffix=".py", delete=False, encoding="utf-8"
    ) as f:
        f.write(content)
        temp_path = Path(f.name)

    yield temp_path

    # Cleanup
    temp_path.unlink(missing_ok=True)
    backup_path = temp_path.with_suffix(temp_path.suffix + ".animhost_backup")
    backup_path.unlink(missing_ok=True)


def test_read_existing_variables(temp_script):
    """Test reading existing variables."""
    result = read_script_variables(temp_script, ["epochs", "batch_size"])
    assert result == {"epochs": 10, "batch_size": 32}


def test_read_missing_variables(temp_script):
    """Test reading non-existent variables."""
    result = read_script_variables(temp_script, ["epochs", "missing_var"])
    assert result == {"epochs": 10, "missing_var": None}


def test_read_nonexistent_file():
    """Test reading from non-existent file."""
    result = read_script_variables(Path("nonexistent.py"), ["epochs"])
    assert result == {"epochs": None}


def test_write_variables_success(temp_script):
    """Test successful variable writing."""
    error = write_script_variables(temp_script, {"epochs": 20, "batch_size": 64})
    assert error is None

    # Verify changes
    result = read_script_variables(temp_script, ["epochs", "batch_size"])
    assert result == {"epochs": 20, "batch_size": 64}


def test_write_creates_backup(temp_script):
    """Test that backup is created."""
    original_content = temp_script.read_text(encoding="utf-8")

    write_script_variables(temp_script, {"epochs": 15})

    backup_path = temp_script.with_suffix(temp_script.suffix + ".animhost_backup")
    assert backup_path.exists()
    assert backup_path.read_text(encoding="utf-8") == original_content


def test_write_missing_variable(temp_script):
    """Test writing to non-existent variable."""
    error = write_script_variables(temp_script, {"missing_var": 42})
    assert error is not None
    assert "not found" in error


def test_write_nonexistent_file():
    """Test writing to non-existent file."""
    error = write_script_variables(Path("nonexistent.py"), {"epochs": 10})
    assert error is not None
    assert "not found" in error


def test_validate_valid_script(temp_script):
    """Test validating a valid script."""
    error = validate_script_variables(temp_script, ["epochs", "batch_size"])
    assert error is None


def test_validate_missing_variables(temp_script):
    """Test validating with missing variables."""
    error = validate_script_variables(temp_script, ["epochs", "missing_var"])
    assert error is not None
    assert "not found" in error


def test_validate_invalid_syntax():
    """Test validating script with invalid syntax."""
    content = "invalid python syntax ="
    with tempfile.NamedTemporaryFile(
        mode="w", suffix=".py", delete=False, encoding="utf-8"
    ) as f:
        f.write(content)
        temp_path = Path(f.name)

    try:
        error = validate_script_variables(temp_path, ["epochs"])
        assert error is not None
        assert "syntax" in error.lower()
    finally:
        temp_path.unlink(missing_ok=True)


def test_reset_script_success(temp_script):
    """Test successful script reset."""
    original_content = temp_script.read_text(encoding="utf-8")

    # Modify script
    write_script_variables(temp_script, {"epochs": 99})

    # Reset
    error = reset_script(temp_script)
    assert error is None

    # Verify restoration
    restored_content = temp_script.read_text(encoding="utf-8")
    assert restored_content == original_content


def test_reset_script_no_backup(temp_script):
    """Test reset without backup."""
    error = reset_script(temp_script)
    assert error is not None
    assert "not found" in error


def test_multiple_assignments_same_value():
    """Test multiple assignments with same value (should warn)."""
    content = textwrap.dedent("""
        epochs = 10
        epochs = 10
    """).strip()
    with tempfile.NamedTemporaryFile(
        mode="w", suffix=".py", delete=False, encoding="utf-8"
    ) as f:
        f.write(content)
        temp_path = Path(f.name)

    try:
        # Should succeed with warning
        error = write_script_variables(temp_path, {"epochs": 15})
        assert error is None
    finally:
        temp_path.unlink(missing_ok=True)
        backup_path = temp_path.with_suffix(temp_path.suffix + ".animhost_backup")
        backup_path.unlink(missing_ok=True)


def test_multiple_assignments_different_values():
    """Test multiple assignments with different values (should error)."""
    content = textwrap.dedent("""
        epochs = 10
        epochs = 20
    """).strip()
    with tempfile.NamedTemporaryFile(
        mode="w", suffix=".py", delete=False, encoding="utf-8"
    ) as f:
        f.write(content)
        temp_path = Path(f.name)

    try:
        error = write_script_variables(temp_path, {"epochs": 15})
        assert error is not None
        assert "different values" in error
    finally:
        temp_path.unlink(missing_ok=True)


def test_example_training_script_integration():
    """Test with actual example_training_script.py."""
    script_path = (
        Path(__file__).parent.parent.parent / "external" / "example_training_script.py"
    )

    if not script_path.exists():
        pytest.skip("example_training_script.py not found")

    # Read current values
    result = read_script_variables(script_path, ["epochs", "batch_size"])
    original_epochs = result["epochs"]
    original_batch_size = result["batch_size"]

    try:
        # Validate
        error = validate_script_variables(script_path, ["epochs", "batch_size"])
        assert error is None

        # Modify
        error = write_script_variables(script_path, {"epochs": 99, "batch_size": 128})
        assert error is None

        # Verify changes
        result = read_script_variables(script_path, ["epochs", "batch_size"])
        assert result == {"epochs": 99, "batch_size": 128}

    finally:
        # Always reset
        reset_script(script_path)

        # Verify reset
        result = read_script_variables(script_path, ["epochs", "batch_size"])
        assert result["epochs"] == original_epochs
        assert result["batch_size"] == original_batch_size
