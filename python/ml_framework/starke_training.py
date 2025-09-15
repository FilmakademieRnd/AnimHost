#!/usr/bin/env python3
"""
Starke Training Script - Converts LocalAutoPipeline.py to standalone training format
Supports real-time JSON progress updates for AnimHost TrainingNode integration
Handles both PAE (Phase Autoencoder) and GNN (Graph Neural Network) training phases
"""

import json
import time
import sys
import subprocess
import re
import os
import array
import struct
import shutil
import numpy as np
import pandas as pd
import scipy.signal
import git

from MotionPreprocessing import MotionProcessor, count_lines, read_csv_data, ReadBinary

#  Set to True to emit JSON for progress updates, False to ignore them
EMIT_PROGRESS_UPDATES = False 
# Set to True for verbose output from GNN training phase
VERBOSE = False

def get_git_root():
    """Returns the root directory of the git repository containing the current working directory."""
    try:
        repo = git.Repo(search_parent_directories=True)
        return repo.working_tree_dir
    except git.exc.InvalidGitRepositoryError:
        raise RuntimeError("Not in a git repository - cannot determine project root")

def starke_training():
    """
    Main training pipeline - coordinates PAE and GNN training phases
    Converts LocalAutoPipeline.py workflow to standalone format with JSON output
    """
    
    # Initial status
    status = {
        "status": "Initializing",
        "text": "Initializing Starke training pipeline..."
    }
    print(json.dumps(status), flush=True)
    
    try:
        # Data preprocessing phase
        preprocessing_status = {
            "status": "Data Preprocessing",
            "text": "Starting data preprocessing phase..."
        }
        print(json.dumps(preprocessing_status), flush=True)
        
        # Common configuration
        dataset_path = get_git_root() + r"\datasets\Survivor_Gen"
        path_to_ai4anim = get_git_root() + r"\AI4Animation\AI4Animation\SIGGRAPH_2022\PyTorch"
        
        # Initialize motion processor
        mp = MotionProcessor(dataset_path, path_to_ai4anim)   
        
        # Data preprocessing (velocity filtering and export)
        _perform_data_preprocessing(dataset_path)
        
        # Run PAE training phase
        run_pae_training(dataset_path, path_to_ai4anim)
        
        # Run GNN training phase  
        run_gnn_training(path_to_ai4anim, mp)
        
        # Final completion status
        completion_status = {
            "status": "Completed Training",
            "text": "Starke training pipeline completed successfully"
        }
        print(json.dumps(completion_status), flush=True)
        
    except Exception as e:
        error_status = {
            "status": "Error",
            "text": f"Starke training pipeline failed: {str(e)}"
        }
        print(json.dumps(error_status), flush=True)
        sys.exit(1)

def _perform_data_preprocessing(dataset_path):
    """Internal function to handle data preprocessing phase"""
    
    try:
        # Count number of velocity samples
        num_samples_total = count_lines(dataset_path + "/sequences_velocity.txt")
        num_features_velocity = 78  # 26 joints * 3 dimensions

        # Apply butterworth filter to velocity data to remove noise
        velocities = ReadBinary(dataset_path + "/joint_velocity.bin", num_samples_total, num_features_velocity)
        df_velocities = pd.DataFrame(velocities)
        velocity_ids = read_csv_data(dataset_path + "/sequences_velocity.txt")
        velocity_ids.columns = ["SeqId","Frame","Type", "File","SeqUUID"]
        df_velocities = pd.concat([velocity_ids, df_velocities], axis=1)
        df_velocities.set_index(["SeqId","Frame"],inplace = True)

        unique_seq_ids = df_velocities.index.get_level_values('SeqId').unique()
        
        # Prepare butterworth filter
        fc = 4.5
        w = fc / (60/2) 
        b, a = scipy.signal.butter(5, w, 'low')
        result_sequences = []

        # Apply filter to each sequence
        for seq_id in unique_seq_ids:
            sequence = df_velocities.xs(key=seq_id, level="SeqId")
            butterw_combined = []
            for col in range(num_features_velocity):
                butterw_result = scipy.signal.filtfilt(b, a, sequence[col])
                butterw_combined.append(butterw_result)
            result_sequences.append(np.vstack(butterw_combined).T)

        final = np.vstack(result_sequences)
        df_final = pd.DataFrame(final)

        # Save to binary
        out_array = array.array('d', df_final.to_numpy(dtype=np.float32).flatten())
        with open(dataset_path+ "/p_velocity.bin", 'wb') as file:
            s = struct.pack('f'*len(out_array), *out_array)
            file.write(s)
            
    except Exception as e:
        raise RuntimeError(f"Data preprocessing failed: {str(e)}")

def run_pae_training(dataset_path, path_to_ai4anim):
    """
    PAE (Phase Autoencoder) training subprocess with real-time parsing
    """
    
    pae_status = {
        "status": "Starting training 1/2 ...",
        "text": "Starting PAE training phase..."
    }
    print(json.dumps(pae_status), flush=True)
    
    try:
        # PAE preprocessing - copy training data to PAE folder
        pae_path = path_to_ai4anim + r"\PAE"
        pae_dataset_path = path_to_ai4anim + r"\PAE\Dataset"

        # Copy files: p_velocity.bin -> Data.bin, sequences_velocity.txt -> Sequences.txt
        shutil.copyfile(dataset_path + "/p_velocity.bin", pae_dataset_path + "/Data.bin")
        shutil.copyfile(dataset_path + "/sequences_velocity.txt", pae_dataset_path + "/Sequences.txt")

        # Launch PAE Network.py subprocess
        # Run with --no-display until plotting is fixed
        process = subprocess.Popen(
            [sys.executable, "-u", "Network.py", '--no-display'],
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            universal_newlines=True,
            bufsize=1,
            cwd=pae_path
        )
        
        # Read output line by line and parse
        while True:
            line = process.stdout.readline()
            if not line and process.poll() is not None:
                break
                
            line = line.strip()
            if not line:
                continue

            # Parse PAE output and emit JSON if relevant
            parsed_output = parse_pae_output(line)
            if parsed_output:
                print(json.dumps(parsed_output), flush=True)
        
        # Wait for process completion
        return_code = process.wait()
        
        if return_code != 0:
            stderr_output = process.stderr.read()
            raise RuntimeError(f"PAE training subprocess failed with return code {return_code}: {stderr_output}")
            
    except Exception as e:
        error_status = {
            "status": "Error",
            "text": f"PAE preprocessing/training failed: {str(e)}"
        }
        print(json.dumps(error_status), flush=True)
        raise

def run_gnn_training(path_to_ai4anim, mp):
    """
    GNN (Graph Neural Network) training subprocess with real-time parsing
    """
    
    gnn_status = {
        "status": "Starting training 2/2 ...",
        "text": "Starting GNN training phase..."
    }
    print(json.dumps(gnn_status), flush=True)
    
    try:
        # GNN preprocessing - prepare training data for generator
        df_input_data = mp.input_preprocessing()
        df_output_data = mp.output_preprocessing()
        mp.export_data()

        # Copy training data to GNN folder
        processed_path = r"..\data"
        gnn_path = path_to_ai4anim + r"\GNN"

        # Copy all files from processed folder to GNN folder
        for file in os.listdir(processed_path):
            shutil.copyfile(processed_path + "/" + file, gnn_path + "/Data/" + file)

        # Launch GNN Network.py subprocess
        process = subprocess.Popen(
            [sys.executable, "-u", "Network.py"],
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            universal_newlines=True,
            bufsize=1,
            cwd=gnn_path
        )
        
        # Read output line by line and parse
        while True:
            line = process.stdout.readline()
            if not line and process.poll() is not None:
                break
                
            line = line.strip()
            if not line:
                continue

            # Parse GNN output and emit JSON if relevant
            parsed_output = parse_gnn_output(line)
            if parsed_output:
                print(json.dumps(parsed_output), flush=True)
        
        # Wait for process completion
        return_code = process.wait()
        
        if return_code != 0:
            stderr_output = process.stderr.read()
            raise RuntimeError(f"GNN training subprocess failed with return code {return_code}: {stderr_output}")
            
    except Exception as e:
        error_status = {
            "status": "Error",
            "text": f"GNN preprocessing/training failed: {str(e)}"
        }
        print(json.dumps(error_status), flush=True)
        raise

def parse_training_output(line, phase_name):
    """
    Parse Network.py output (both PAE and GNN) and convert to JSON format
    Returns JSON dict for relevant lines, None for irrelevant lines
    
    Handles consistent output format:
    - "Epoch 1 0.32931875690483264" (epoch + loss)
    - "Progress 23.42 %" (progress percentage)
    - All other lines go through optional VERBOSE mode
    
    Args:
        line: Output line from Network.py
        phase_name: "PAE" or "GNN" for prefixing messages
    """
    
    # Pattern 1: "Epoch 1 0.32931875690483264"
    epoch_pattern = re.compile(r'^Epoch\s+(\d+)\s+([\d.]+)$')
    match = epoch_pattern.search(line)
    if match:
        epoch = int(match.group(1))
        loss = float(match.group(2))
        return {
            "status": f"{phase_name} training",
            "text": f"{phase_name} epoch {epoch} completed",
            "metrics": {
                "epoch": epoch,
                "train_loss": round(loss, 4)
            }
        }
    
    # Pattern 2: "Progress 23.42 %" (configurable progress updates)
    progress_pattern = re.compile(r'^Progress\s+([\d.]+)\s*%+$')
    match = progress_pattern.search(line)
    if match:
        if EMIT_PROGRESS_UPDATES:
            progress = float(match.group(1))
            return {
                "status": f"{phase_name} training",
                "text": f"{phase_name} Progress: {progress}%",
                "metrics": {
                    "progress_percent": round(progress, 2)
                }
            }
        else:
            # Don't emit JSON for progress updates as they overwrite each other
            return None
    
    # Pattern 3: All other lines go through VERBOSE mode
    if VERBOSE:
        # Emit the full line as a message if verbose mode is enabled
        return {
            "status": f"{phase_name} verbose",
            "text": f"{phase_name}: {line}"
        }
    
    # Return None for lines that don't match expected patterns
    return None

def parse_pae_output(line):
    """
    Parse PAE Network.py output - wrapper for parse_training_output
    """
    return parse_training_output(line, "Encoder")

def parse_gnn_output(line):
    """
    Parse GNN Network.py output - wrapper for parse_training_output
    """
    return parse_training_output(line, "Controller")

def main():
    """
    Main entry point for starke_training script
    """
    try:
        starke_training()
    except KeyboardInterrupt:
        error_status = {
            "status": "Interrupted",
            "text": "Starke training interrupted by user"
        }
        print(json.dumps(error_status), flush=True)
        sys.exit(1)
    except Exception as e:
        error_status = {
            "status": "Error",
            "text": f"Starke training failed: {str(e)}"
        }
        print(json.dumps(error_status), flush=True)
        sys.exit(1)

if __name__ == "__main__":
    main()