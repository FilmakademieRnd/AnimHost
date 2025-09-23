#Imports

from itertools import count
import numpy as np
import pandas as pd
import struct
import os
import subprocess 
import array
import shutil
import plotly.express as px
import plotly.graph_objects as go
import math
import time
import scipy.signal

from data.motion_preprocessing import MotionProcessor, count_lines, read_csv_data, ReadBinary 


    

def main():
    # common config
    # dataset_path = r"C:\DEV\DATASETS\Survivor_Gen"
    # path_to_ai4anim = r"C:\DEV\AI4Animation\AI4Animation\SIGGRAPH_2022\PyTorch"
    dataset_path = r"C:/anim-ws/AnimHost/datasets/Survivor_Gen"
    path_to_ai4anim = r"C:/anim-ws/AI4Animation/AI4Animation/SIGGRAPH_2022/PyTorch"
    mp = MotionProcessor(dataset_path, path_to_ai4anim)   
    # count number of velcocity samples
    num_samples_total = count_lines(dataset_path + "/sequences_velocity.txt")

    num_features_velocity = 78 # 26 joints * 3 dimensions

    # Apply butterworth filter to velocity data to remove noise

    velocities = ReadBinary(dataset_path + "/joint_velocity.bin", num_samples_total, num_features_velocity)
    df_velocities = pd.DataFrame(velocities)
    velocity_ids = read_csv_data(dataset_path + "/sequences_velocity.txt")
    velocity_ids.columns = ["SeqId","Frame","Type", "File","SeqUUID"]
    df_velocities = pd.concat([velocity_ids, df_velocities], axis=1)
    df_velocities.set_index(["SeqId","Frame"],inplace = True)

    unique_seq_ids = df_velocities.index.get_level_values('SeqId').unique()
    print(f"Applying Butterworth filter to {len(unique_seq_ids)} sequences. First sequence ID: {unique_seq_ids[0]}.")
    #prepare filter
    fc = 4.5
    w = fc / (60/2) 
    print(w)
    b, a = scipy.signal.butter(5, w, 'low')
    #buttered = scipy.signal.filtfilt(b, a, sequence["out_jvel_y_hand_L"]) 
    result_sequences = []

    for seq_id in unique_seq_ids:
        #select sequence by index
        sequence = df_velocities.xs(key=seq_id, level="SeqId")
        butterw_combined = []
        #apply butterworth to every column
        for col in range(num_features_velocity):
            butterw_result = scipy.signal.filtfilt(b, a, sequence[col])
            butterw_combined.append(butterw_result)

        result_sequences.append(np.vstack(butterw_combined).T)

    final = np.vstack(result_sequences)
    df_final = pd.DataFrame(final)

    #save to binary

    out_array = array.array('d', df_final.to_numpy(dtype=np.float32).flatten())
    with open(dataset_path+ "/p_velocity.bin", 'wb') as file:
        s = struct.pack('f'*len(out_array), *out_array)
        file.write(s)

    # copy training data to PAE folder
    pae_path =  path_to_ai4anim +  r"\PAE"
    pae_dataset_path = path_to_ai4anim + r"\PAE\Dataset"

    # files to copy p_velocity.bin & sequencees_velocity.txt
    # rename p_velocity.bin to joint_velocity.bin to  Data.bin & Sequences.txt

    shutil.copyfile(dataset_path+ "/p_velocity.bin", pae_dataset_path + "/Data.bin")
    shutil.copyfile(dataset_path+ "/sequences_velocity.txt", pae_dataset_path + "/Sequences.txt")

    # start training call Network.py 
    subprocess.run(f'python Network.py', cwd=pae_path)

    #prepare training data for generator
    
    df_input_data = mp.input_preprocessing()
    df_output_data = mp.output_preprocessing()
    mp.export_data()

    # copy training data to GNN folder
    processed_path = r"..\data"
    gnn_path = path_to_ai4anim + r"\GNN"

    # copy all files of processed folder to GNN folder
    for file in os.listdir(processed_path):
        shutil.copyfile(processed_path + "/" + file, gnn_path + "/Data/" + file)

    # start training call Network.py
    subprocess.run(f'python Network.py', cwd=gnn_path)

if __name__ == "__main__":
    main()