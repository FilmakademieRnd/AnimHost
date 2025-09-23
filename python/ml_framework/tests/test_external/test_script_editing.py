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
    reset_script,
    read_script_variable,
    write_script_variable,
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
    assert result == {"epochs": "10", "batch_size": "32"}


def test_read_missing_variables(temp_script):
    """Test reading non-existent variables."""
    result = read_script_variables(temp_script, ["epochs", "missing_var"])
    assert result == {"epochs": "10", "missing_var": None}


def test_read_nonexistent_file():
    """Test reading from non-existent file."""
    result = read_script_variables(Path("nonexistent.py"), ["epochs"])
    assert result == {"epochs": None}


def test_write_variables_success(temp_script):
    """Test successful variable writing."""
    error = write_script_variables(temp_script, {"epochs": "20", "batch_size": "64"})
    assert error is None

    # Verify changes
    result = read_script_variables(temp_script, ["epochs", "batch_size"])
    assert result == {"epochs": "20", "batch_size": "64"}


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


def test_write_complex_expressions():
    """Test writing complex expressions and mixed data types."""
    content = textwrap.dedent("""
        epochs = 10
        batch_size = 32
        tensor = None
        device = None
    """).strip()
    with tempfile.NamedTemporaryFile(
        mode="w", suffix=".py", delete=False, encoding="utf-8"
    ) as f:
        f.write(content)
        temp_path = Path(f.name)

    try:
        error = write_script_variables(temp_path, {
            "epochs": 20,  # number
            "batch_size": 64,  # number
            "tensor": "torch.zeros([2, 4], dtype=torch.int32)",  # expression
            "device": "'cuda'",  # string expression
        })
        assert error is None

        # Verify all values were written correctly
        result = read_script_variables(temp_path, ["epochs", "batch_size", "tensor", "device"])
        assert result["epochs"] == "20"
        assert result["batch_size"] == "64"
        assert result["tensor"] == "torch.zeros([2, 4], dtype=torch.int32)"
        assert result["device"] == "'cuda'"
    finally:
        temp_path.unlink(missing_ok=True)
        backup_path = temp_path.with_suffix(temp_path.suffix + ".animhost_backup")
        backup_path.unlink(missing_ok=True)


def test_read_validates_duplicates():
    """Test that read now validates duplicate assignments."""
    content = textwrap.dedent("""
        epochs = 10
        batch_size = 32
        epochs = 15  # Different value - should error
    """).strip()
    with tempfile.NamedTemporaryFile(
        mode="w", suffix=".py", delete=False, encoding="utf-8"
    ) as f:
        f.write(content)
        temp_path = Path(f.name)

    try:
        # Read should return None for epochs due to validation error
        result = read_script_variables(temp_path, ["epochs", "batch_size"])
        assert result["epochs"] is None  # Should be None due to duplicate validation
        assert result["batch_size"] == "32"  # Should work fine
    finally:
        temp_path.unlink(missing_ok=True)


def test_single_variable_functions(temp_script):
    """Test new single-variable functions."""
    # Test read_script_variable
    value, error = read_script_variable(temp_script, "epochs")
    assert error is None
    assert value == "10"

    # Test read non-existent variable
    value, error = read_script_variable(temp_script, "missing_var")
    assert error is None
    assert value is None

    # Test write_script_variable
    error = write_script_variable(temp_script, "epochs", 42)
    assert error is None

    # Verify the write
    value, error = read_script_variable(temp_script, "epochs")
    assert error is None
    assert value == "42"


def test_self_assignment_ignored():
    """Test that self-assignments (foo = foo) are ignored during read/write operations."""
    content = textwrap.dedent("""
        #!/usr/bin/env python3
        # Script with self-assignments
        foo = 10
        bar = bar  # Self-assignment should be ignored
        foo = foo  # Self-assignment should be ignored
        baz = 20
        foo = foo,  # Self-assignment with comma should be ignored
    """).strip()

    with tempfile.NamedTemporaryFile(
        mode="w", suffix=".py", delete=False, encoding="utf-8"
    ) as f:
        f.write(content)
        temp_path = Path(f.name)

    try:
        # Test reading - should only find the real assignment
        result = read_script_variables(temp_path, ["foo", "bar", "baz"])
        assert result["foo"] == "10"  # Should find the real assignment
        assert result["bar"] is None  # Self-assignment should be ignored, no real assignment found
        assert result["baz"] == "20"  # Should find the real assignment

        # Test writing - should work without conflicts
        error = write_script_variables(temp_path, {"foo": 42})
        assert error is None  # Should succeed despite self-assignments

        # Verify the write worked
        updated_result = read_script_variables(temp_path, ["foo"])
        assert updated_result["foo"] == "42"

        # Verify self-assignments are still in the file but ignored
        content_after = temp_path.read_text(encoding="utf-8")
        assert "bar = bar" in content_after  # Self-assignment should still be there
        assert "foo = foo" in content_after  # Self-assignment should still be there
        assert "foo = 42" in content_after   # New assignment should be there

    finally:
        temp_path.unlink(missing_ok=True)
        backup_path = temp_path.with_suffix(temp_path.suffix + ".animhost_backup")
        backup_path.unlink(missing_ok=True)
