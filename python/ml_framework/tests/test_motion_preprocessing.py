#!/usr/bin/env python3
"""Tests for motion_preprocessing utilities."""

import pytest
from data.motion_preprocessing import FrameRange


def test_frame_range_full_window():
    """start_index=0 returns all 13 frames centered on the reference frame."""
    result = list(FrameRange(num_samples=13, fps=60, reference_frame=120, start_index=0))
    assert result == [60, 70, 80, 90, 100, 110, 120, 130, 140, 150, 160, 170, 180]


def test_frame_range_from_reference_frame():
    """start_index=6 starts at the reference frame, returning 7 frames."""
    result = list(FrameRange(num_samples=13, fps=60, reference_frame=120, start_index=6))
    assert result == [120, 130, 140, 150, 160, 170, 180]


def test_frame_range_future_window():
    """start_index=7 skips the reference frame, returning only future frames."""
    result = list(FrameRange(num_samples=13, fps=60, reference_frame=120, start_index=7))
    assert result == [130, 140, 150, 160, 170, 180]


def test_frame_range_includes_reference_frame():
    """start_index=6 lands on the reference frame (odd reference_frame=121)."""
    frame = 120 + 1  # +1 to reference leads to off by one error if input is 60 to 180 frames
    result = list(FrameRange(num_samples=13, fps=60, reference_frame=frame, start_index=6))
    assert result == [121, 131, 141, 151, 161, 171, 181]
