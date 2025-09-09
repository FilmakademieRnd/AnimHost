#!/usr/bin/env python3
"""
Mock Training Script for AnimHost TrainingNode Integration
Simulates a 5-epoch training process with progress updates via JSON stdout
"""

import json
import time
import sys
import random

def main():
    """
    Mock training script that outputs JSON progress updates
    - 5 epochs total
    - Updates every 0.5 seconds
    - Outputs: epoch, train_loss, val_loss, learning_rate
    """
    
    # Initial status, to confirm script start
    status = {
        "status": "starting",
        "message": "Initializing training process..."
    }
    print(json.dumps(status), flush=True)
    
    total_epochs = 5
    
    # Simulate training epochs
    for epoch in range(1, total_epochs + 1):
        time.sleep(0.5)  # Simulate processing time
        
        # Mock training metrics with realistic values
        train_loss = 1.0 - (epoch * 0.15) + random.uniform(-0.05, 0.05)  # Decreasing loss
        val_loss = train_loss + random.uniform(0.01, 0.1)  # Val loss slightly higher
        learning_rate = 0.001 * (0.9 ** (epoch - 1))  # Decaying learning rate
        
        progress = {
            "status": "training",
            "epoch": epoch,
            "total_epochs": total_epochs,
            "train_loss": round(train_loss, 4),
            "val_loss": round(val_loss, 4),
            "learning_rate": round(learning_rate, 6)
        }
        
        print(json.dumps(progress), flush=True)
    
    # Final completion status
    completion_status = {
        "status": "completed",
        "message": f"Training completed successfully after {total_epochs} epochs",
        "final_train_loss": round(train_loss, 4),
        "final_val_loss": round(val_loss, 4)
    }
    print(json.dumps(completion_status), flush=True)

if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        error_status = {
            "status": "interrupted",
            "message": "Training interrupted by user"
        }
        print(json.dumps(error_status), flush=True)
        sys.exit(1)
    except Exception as e:
        error_status = {
            "status": "error",
            "message": f"Training failed: {str(e)}"
        }
        print(json.dumps(error_status), flush=True)
        sys.exit(1)