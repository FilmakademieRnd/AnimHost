#!/usr/bin/env python3
"""
Mock Training Script for AnimHost TrainingNode Integration
Supports both integrated (original) and standalone (subprocess) training modes
"""

import json
import time
import sys
import random
import subprocess
import re
import os

# Feature flag: True for integrated mode, False for standalone subprocess mode
USE_INTEGRATED_TRAINING = False

def integrated_training():
    """
    Mock training script that outputs JSON progress updates
    - 5 epochs total
    - Updates every 0.5 seconds
    - Outputs: epoch, train_loss, val_loss, learning_rate
    """
    
    # Initial status, to confirm script start
    status = {
        "status": "Starting",
        "text": "Initializing training process..."
    }
    print(json.dumps(status), flush=True)
    
    total_epochs = 5
    
    # Simulate training epochs
    for epoch in range(1, total_epochs + 1):
        time.sleep(0.5)  # Simulate processing time
        
        # Mock training metrics
        train_loss = 1.0 - (epoch * 0.15) + random.uniform(-0.05, 0.05)
        val_loss = train_loss + random.uniform(0.01, 0.1)
        learning_rate = 0.001 * (0.9 ** (epoch - 1))
        
        progress = {
            "status": "Training",
            "text": f"Training epoch {epoch}/{total_epochs}",
            "metrics": {
                "epoch": epoch,
                "total_epochs": total_epochs,
                "train_loss": round(train_loss, 4),
                "val_loss": round(val_loss, 4),
                "learning_rate": round(learning_rate, 6)
            }
        }
        
        print(json.dumps(progress), flush=True)
    
    # Final completion status
    completion_status = {
        "status": "Completed",
        "text": f"Training completed successfully after {total_epochs} epochs",
        "metrics": {
            "final_train_loss": round(train_loss, 4),
            "final_val_loss": round(val_loss, 4),
            "total_epochs": total_epochs
        }
    }
    print(json.dumps(completion_status), flush=True)

def standalone_training():
    """
    Runs standalone_training.py as subprocess and parses its text output
    Converts parsed data to JSON format for TrainingNode integration
    """
    
    # Initial status
    status = {
        "status": "Starting",
        "text": "Launching standalone training process..."
    }
    print(json.dumps(status), flush=True)
    
    # Get path to standalone_training.py
    script_dir = os.path.dirname(os.path.abspath(__file__))
    standalone_script = os.path.join(script_dir, "mock_standalone_training.py")
    
    try:
        # Launch subprocess with real-time output parsing
        process = subprocess.Popen(
            [sys.executable, "-u", standalone_script], # -u for unbuffered output
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            universal_newlines=True,
            bufsize=1  # Flush line by line
        )
        
        total_epochs = 5
        current_epoch = 0
        
        # Regex patterns for parsing
        epoch_pattern = re.compile(r'Epoch\s+(\d+)\s+([\d.]+)')
        
        # Read output line by line
        while True:
            line = process.stdout.readline()
            if not line and process.poll() is not None:
                break
                
            line = line.strip()
            if not line:
                continue

            # Parse epoch completion
            epoch_match = epoch_pattern.search(line)
            if epoch_match:
                current_epoch = int(epoch_match.group(1))
                train_loss = float(epoch_match.group(2))

                epoch_status = {
                    "status": "Training",
                    "text": f"Standalone training epoch {current_epoch}/{total_epochs}",
                    "metrics": {
                        "epoch": current_epoch,
                        "total_epochs": total_epochs,
                        "train_loss": round(train_loss, 4)
                    }
                }
                print(json.dumps(epoch_status), flush=True)
        
        # Wait for process completion
        return_code = process.wait()
        
        if return_code == 0:
            # Success status
            completion_status = {
                "status": "Completed",
                "text": f"Standalone training completed successfully after {total_epochs} epochs",
                "metrics": {
                    "total_epochs": total_epochs
                }
            }
            print(json.dumps(completion_status), flush=True)
        else:
            # Process failed
            stderr_output = process.stderr.read()
            error_status = {
                "status": "Error",
                "text": f"Standalone training failed with return code {return_code}: {stderr_output}"
            }
            print(json.dumps(error_status), flush=True)
            
    except FileNotFoundError:
        error_status = {
            "status": "Error",
            "text": f"Standalone training script not found: {standalone_script}"
        }
        print(json.dumps(error_status), flush=True)
    except Exception as e:
        error_status = {
            "status": "Error",
            "text": f"Failed to launch standalone training: {str(e)}"
        }
        print(json.dumps(error_status), flush=True)

def main():
    """
    Main entry point - routes to integrated or standalone training based on feature flag
    """
    if USE_INTEGRATED_TRAINING:
        integrated_training()
    else:
        standalone_training()

if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        error_status = {
            "status": "Interrupted",
            "text": "Training interrupted by user"
        }
        print(json.dumps(error_status), flush=True)
        sys.exit(1)
    except Exception as e:
        error_status = {
            "status": "Error",
            "text": f"Training failed: {str(e)}"
        }
        print(json.dumps(error_status), flush=True)
        sys.exit(1)
