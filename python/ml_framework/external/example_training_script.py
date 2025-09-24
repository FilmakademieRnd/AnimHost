#!/usr/bin/env python3
"""
Standalone Training Script - Simulates external training process.

Outputs text-based progress updates that will be parsed by training.py.
"""

import time
import random
import sys


def main() -> None:
    """
    Standalone training script that outputs text-based progress updates.

    Simulates a training process with the following characteristics:

    - Progress percentage updates during epochs
    - Epoch completion with train loss

    :returns: None
    """

    print("Training Phases")

    total_epochs = 3

    # Simulate training epochs
    for epoch in range(1, total_epochs + 1):
        epoch_duration = 0.5  # 0.5 seconds
        update_interval = 0.1  # 100ms
        total_updates = int(epoch_duration / update_interval)

        for i in range(total_updates):
            progress = (i + 1) / total_updates * 100
            time.sleep(update_interval)
            print(f"Progress {progress:.2f} %")

        # Brief pause before epoch completion
        time.sleep(0.1)

        # Mock training loss
        train_loss = 1.0 - (epoch * 0.15) + random.uniform(-0.05, 0.05)

        print(f"Epoch {epoch} {train_loss:.11f}")
        print("Saving Parameters")

        # Brief pause before next epoch
        if epoch < total_epochs:
            time.sleep(0.1)

    print("Training completed successfully")


if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        print("Training interrupted by user", file=sys.stderr)
        sys.exit(1)
    except Exception as e:
        print(f"Training failed: {str(e)}", file=sys.stderr)
        sys.exit(1)
