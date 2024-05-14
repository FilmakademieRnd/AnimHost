import numpy as np
import pandas as pd
import struct
import array  # Add missing import statement for 'array' module
import plotly.express as px
import plotly.graph_objects as go
import math
import time


def ReadBinary(binaryFile, sampleCount, featureCount):
    bytesPerLine = featureCount * 4
    data = np.empty((sampleCount, featureCount), dtype=np.float32)
    with open(binaryFile, "rb") as f:
        for i in range(sampleCount):
            if i % max(int(sampleCount / 10), 1) == 0:
                print('Reading binary ' + binaryFile + '...', round(100 * i / sampleCount, 2), "%", end="\r")
            f.seek(i * bytesPerLine)
            bytes = f.read(bytesPerLine)
            floats = struct.unpack('f' * (len(bytes) // struct.calcsize('f')), bytes)
            data[i] = np.array(floats, dtype=np.float32)
    print('Reading binary ' + binaryFile + '...', 100, "%", end="\r")
    print("")
    return data


def read_csv_style_data(file_path, delimiter=' ', dtype='float32'):
    """
    Read CSV-style data from a file into a NumPy array.

    Parameters:
    - file_path: str, path to the file containing the data.
    - delimiter: str, the delimiter used in the data (default is space).

    Returns:
    - data_array: np.ndarray, the NumPy array containing the data.
    """
    try:
        data_array = np.genfromtxt(file_path, delimiter=delimiter,dtype=None, encoding=None)
        return data_array
    except Exception as e:
        print(f"Error reading data from {file_path}: {e}")
        return None
    

def read_csv_data(file_path, delimiter=' '):
    """
    Read CSV data from a file into a pandas DataFrame.

    Parameters:
    - file_path: str, path to the file containing the data.
    - delimiter: str, the delimiter used in the data (default is space).

    Returns:
    - data_frame: pd.DataFrame, the pandas DataFrame containing the data.
    """
    try:
        data_frame = pd.read_csv(file_path, delimiter=delimiter, header=None)
        return data_frame
    except Exception as e:
        print(f"Error reading data from {file_path}: {e}")
        return None
    

def write_dataframe_to_cpp_vector_file(dataframe, file_path):
    """
    Example usage:
    data = IN.loc[IN.index.get_level_values('SeqId') == 1]
    df = pd.DataFrame(data)

    file_path = 'dummy_input_vector.txt'
    write_dataframe_to_cpp_vector_file(df, file_path)
    """
    
    if not isinstance(dataframe, pd.DataFrame):
        raise ValueError("Input must be a Pandas DataFrame")

    rows, columns = dataframe.shape
    cpp_vector = "std::vector<float>{"

    for i in range(rows):
        cpp_vector += "{"
        for j in range(columns):
            value = dataframe.iloc[i, j]
            cpp_vector += f"{value:.6f}"  # Adjust the format as needed
            if j < columns - 1:
                cpp_vector += ", "
        cpp_vector += "}"
        if i < rows - 1:
            cpp_vector += ", \n"

    cpp_vector += "};"

    with open(file_path, 'w') as file:
        file.write(cpp_vector)
    

def calculate_2d_phase_values(row):
    phases = row[['PhaseValue1', 'PhaseValue2', 'PhaseValue3', 'PhaseValue4', 'PhaseValue5']].to_numpy().astype(float)
    amplitudes = row[['PhaseAmp1', 'PhaseAmp2', 'PhaseAmp3', 'PhaseAmp4', 'PhaseAmp5']].to_numpy().astype(float)
    
    phases *= 2.0 * np.pi
    sin_values = np.sin(phases)
    cos_values = np.cos(phases)
    phase_2d = np.column_stack((sin_values, cos_values))
    phase_2d *= amplitudes[:, np.newaxis]
    return phase_2d

def get_window_values(row, phaseValues, selected_columns, window_size=1):
    seq_id,frame  = row.name
    start_frame = max(0, frame - window_size)
    end_frame = frame + window_size 

    window = phaseValues.loc[(seq_id,start_frame):(seq_id,end_frame)]
    if frame % 100 == 0:
        print(f"Progress: {seq_id}/{frame}", end='\r')
    return window[selected_columns].values.flatten()

def get_future_window_values(row, phaseValues, selected_columns, window_size=7):
    """
    Retrieves the future window values for a given row in a DataFrame.

    Parameters:
    - row: The row containing the sequence ID and frame number.
    - phaseValues: The DataFrame containing the phase values.
    - selected_columns: The list of columns to select from the window.
    - window_size: The size of the window to retrieve (default is 7).

    Returns:
    - The flattened array of values from the future window.

    """
    seq_id, frame = row.name
    start_frame = frame + 1
    end_frame = frame + window_size 

    window = phaseValues.loc[(seq_id, start_frame):(seq_id, end_frame)]
    if frame % 100 == 0:
        print(f"Progress: {seq_id}/{frame}", end='\r')
    return window[selected_columns].values.flatten()




def run_motion_preprocessing(num_phase_channel,dataset_path, phase_param_file, phase_sequence_file):
    return 0

class MotionProcessor:
    def __init__(self):
        self.num_phase_channel = 5
        self.input_feature_count = 475
        self.output_feature_count = 436
        self.sample_count = 28349

        self.dataset_path = r"C:\DEV\DATASETS\Survivor_Gen"
        self.trained_phase_param_file = r"C:\DEV\AI4Animation\AI4Animation\SIGGRAPH_2022\PyTorch\PAE\Training\Parameters_30.txt"
        self.trained_phase_sequence_file = r"C:\DEV\AI4Animation\AI4Animation\SIGGRAPH_2022\PyTorch\PAE\Dataset\Sequences.txt"

        self.PhaseData = None
        self.InputData = None
        self.OutputData = None

    
    def run_motion_preprocessing(self):
        print("Running motion preprocessing...")
        start_time = time.time()
        input_data = self.input_preprocessing()
        end_time = time.time()
        print("Motion preprocessing completed in", round(end_time - start_time, 2), "seconds.")
        return 0
    
    
    def input_preprocessing(self):
        print("Running input preprocessing...")
        start_time = time.time()

        ## Read phase parameters
        phaseData = read_csv_style_data(self.trained_phase_param_file)
        phaseSequence = read_csv_data(self.trained_phase_sequence_file)
        phaseSequence.columns = ["SeqId","Frame","Type", "File","SeqUUID"]

        phaseData_header = []
        [phaseData_header.append(f"PhaseValue{i+1}") for i in range(self.num_phase_channel)]
        [phaseData_header.append(f"PhaseFreq{i+1}") for i in range(self.num_phase_channel)]
        [phaseData_header.append(f"PhaseAmp{i+1}") for i in range(self.num_phase_channel)]
        [phaseData_header.append(f"PhaseOff{i+1}") for i in range(self.num_phase_channel)]

        df_phaseData = pd.DataFrame(phaseData, columns=phaseData_header)
        df_phaseData = pd.concat([phaseSequence, df_phaseData], axis=1)

        ## Calculate 2D phase values
        df_phaseValues2D = np.vstack(df_phaseData.apply(lambda row: calculate_2d_phase_values(row), axis=1).to_numpy())
        df_phaseValues2D = np.reshape(df_phaseValues2D, (phaseData.shape[0], self.num_phase_channel * 2))

        ## Generate Cloumn names for phase values
        column_names = [f"Phase2D_X_{i+1}" for i in range(self.num_phase_channel)] + [f"Phase2D_Y_{i+1}" for i in range(self.num_phase_channel)]
        phaseValues2D_header =  [column for pair in zip(column_names[:self.num_phase_channel], column_names[self.num_phase_channel:]) for column in pair]

        df_phaseValues2D = pd.DataFrame(df_phaseValues2D, columns=phaseValues2D_header)
        df_phaseData = pd.concat([df_phaseData, df_phaseValues2D], axis=1)

        ## Read input data
        raw_input_data = ReadBinary(self.dataset_path + "/data_x.bin", self.sample_count, self.input_feature_count)
        print("Raw input data shape:", raw_input_data.shape)

        #Check for nan in input data (generated by animhost)
        nan_indices = np.where(np.isnan(raw_input_data[-1]))
        if len(nan_indices[0]) > 0:
            print("Array contains NaN values at indices:")
            for i in range(len(nan_indices[0])):
                print(f"  Row: {nan_indices[0][i]}")
        else:
            print("Array does not contain NaN values.")

        input_label = read_csv_data(self.dataset_path + "/metadata.txt",",")
        row = input_label.iloc[0]
        start= 1
        end= self.input_feature_count +1
        label_input = []
        [label_input.append(row.iloc[i]) for i in range(start,end)]
        df_inputData = pd.DataFrame(raw_input_data, columns=label_input)
        sequence_inputData = read_csv_data(self.dataset_path + "/sequences_mann.txt")
        sequence_inputData.columns = ["SeqId","Frame","Type", "File","SeqUUID"]
        df_inputData = pd.concat([sequence_inputData, df_inputData], axis=1)

        ## create sliding window of 2d phases and concat to input data

        # Set multi-index for both dataframes
        for df in [df_inputData, df_phaseData]:
            df.set_index(['SeqId', 'Frame'], inplace=True)

        # Get columns that start with 'Phase2D_'
        selected_columns = df_phaseData.columns[df_phaseData.columns.str.startswith('Phase2D_')]

        # Apply the get_window_values function
        phase_window = df_inputData.apply(get_window_values, args=(df_phaseData, selected_columns, 6), axis=1)

        # Merge the dataframes, rename the merged column, and expand the 'PhaseSpace' column into its own dataframe
        df_inputData = df_inputData.merge(phase_window.rename('PhaseSpace').to_frame(), left_index=True, right_index=True)
        df_phaseSpace = df_inputData.pop('PhaseSpace').apply(pd.Series)
        df_phaseSpace.columns = [f"PhaseSpace-{i+1}" for i in range(df_phaseSpace.shape[1])]

        # Concatenate the dataframes along the columns
        df_inputData = pd.concat([df_inputData, df_phaseSpace], axis=1)

        ## Done
        end_time = time.time()
        print("Input preprocessing completed in", round(end_time - start_time, 2), "seconds.")

        self.InputData = df_inputData
        self.PhaseData = df_phaseData
        
        return df_inputData
    
    def output_preprocessing(self):
        print("Running output preprocessing...")
        start_time = time.time()

        raw_output_data = ReadBinary(self.dataset_path + "/data_y.bin", self.sample_count, self.output_feature_count)
        
        output_label = read_csv_data(self.dataset_path + "/metadata.txt",",")
        out_row = output_label.iloc[1]
        label_output = []
        [label_output.append(out_row.iloc[i]) for i in range(1,self.output_feature_count +1)]
        df_OutputData = pd.DataFrame(raw_output_data, columns=label_output)

        sequence_OutData = read_csv_data(self.dataset_path + "/sequences_mann.txt")
        sequence_OutData.columns = ["SeqId","Frame","Type", "File","SeqUUID"]
        df_OutputData = pd.concat([sequence_OutData, df_OutputData], axis=1)
        df_OutputData.set_index(['SeqId', 'Frame'], inplace=True)

        select_phase2d = [col for col in self.PhaseData.columns if "Phase2D_" in col]
        select_amplitude = [col for col in self.PhaseData.columns if "PhaseAmp" in col]
        select_freq = [col for col in self.PhaseData.columns if "PhaseFreq" in col]
        select_combined = select_phase2d + select_amplitude + select_freq

        # Collect Future Phase Values
        future_phase_values = self.InputData.apply(get_future_window_values, args=(self.PhaseData, select_combined, 7), axis=1)
        
        dfOutDataMrg = pd.merge(df_OutputData, future_phase_values.to_frame(), on=['SeqId','Frame'],how='inner')
        dfOutDataMrg.rename(columns={0: 'PhaseUpdate'}, inplace=True)
        dfOutPhaseUpdate = dfOutDataMrg['PhaseUpdate'].apply(pd.Series)
        dfOutPhaseUpdate.columns = [f"PhaseUpdate-{i+1}" for i in range(future_phase_values.values[0].shape[0])]

        dfOutDataExp = pd.concat([dfOutDataMrg, dfOutPhaseUpdate], axis=1)
        dfOutDataExp.drop('PhaseUpdate', axis=1, inplace=True)

        self.OutputData = dfOutDataExp
        
        end_time = time.time()
        print("Output preprocessing completed in", round(end_time - start_time, 2), "seconds.")
        return self.OutputData
    
    def export_data(self, folder_path= "../data/"):
        # Export input data
        in_dropped = self.InputData.drop(["Type", "File","SeqUUID"], axis=1)

        # Convert DataFrame to a flat float array
        flat_in = array.array('d', in_dropped.to_numpy(dtype=np.float32).flatten())

        IN_mn =  in_dropped.mean().to_numpy(dtype=np.float32).flatten()
        IN_std =  in_dropped.std().replace(0, 1).to_numpy(dtype=np.float32).flatten()

        # Save the array to a binary file
        with open(folder_path +'Input.bin', 'wb') as file:
            s = struct.pack('f'*len(flat_in), *flat_in)
            file.write(s)

        with open(folder_path +'InputLabels.txt', 'w') as file:
            for idx, col in enumerate(in_dropped.columns):
                file.write(f"[{idx}] {col}\n")

        with open(folder_path +'InputShape.txt', 'w') as file:
            file.write(f"{len(in_dropped)}\n{len(in_dropped.columns)}")

        with open(folder_path +'InputNormalization.txt', 'w') as file:
            file.write(" ".join(map(str, IN_mn)) + "\n")
            file.write(" ".join(map(str, IN_std)) + "\n")

        # Export output data
        out_dropped = self.OutputData.drop(["Type", "File","SeqUUID"], axis=1)

        flat_out = array.array('d', out_dropped.to_numpy(dtype=np.float32).flatten())

        OUT_mn =  out_dropped.mean().to_numpy(dtype=np.float32).flatten()
        OUT_std =  out_dropped.std().replace(0, 1).to_numpy(dtype=np.float32).flatten()

        # Save the array to a binary file
        with open(folder_path +'Output.bin', 'wb') as file:
            s = struct.pack('f'*len(flat_out), *flat_out)
            file.write(s)

        with open(folder_path +'OutputLabels.txt', 'w') as file:
            for idx, col in enumerate(out_dropped.columns):
                file.write(f"[{idx}] {col}\n")

        with open(folder_path +'OutputShape.txt', 'w') as file:
            file.write(f"{len(out_dropped)}\n{len(out_dropped.columns)}")

        with open(folder_path +'OutputNormalization.txt', 'w') as file:
            file.write(" ".join(map(str, OUT_mn)) + "\n")
            file.write(" ".join(map(str, OUT_std)) + "\n")


    def get_raw_input_columns(self):
        return self.InputData.drop(["Type", "File","SeqUUID"], axis=1).columns
    
    def get_raw_output_columns(self):
        return self.OutputData.drop(["Type", "File","SeqUUID"], axis=1).columns
    
    