# Training a New Motion Generator for AnimHost

## 1. Prepare Data
1. **Retarget Animation:** Align character to **Y-up** and motion on **X-Z**.  
   - Follow iClone/Blender workflow if needed (TODO: detail this).
2. **Export to FBX** with the correct character setup.

## 2. Run `ModeAdaptive.flow` in AnimHost
1. **Animation Importer Node:**
   - Check **"Enable Subsample for GNN"** (removes leaf/hand bones, applies root transform).
   - Set the **Trainingdata Folder** path.
2. **Data Export & Locomotion Nodes:**
   - Set **Output Paths** (same folder).
   - Check **"overwrite existing data"** and **"Write Binary Data"**.
   - Root bone defaults to hip (no need to change).
3. **Start the Graph** and wait for AnimHost to generate `metadata.txt`, `sequences_velocity.txt`, `data_x.bin`, `data_y.bin`, etc.

## 3. Phase Autoencoder (PAE) & Gated Neural Network (GNN)
1. **Clone `AI4Animation`** (latest) to get `SIGGRAPH_2022\PyTorch`.
2. **Check PAE Bone Count**  
   - In `...\PAE\Network.py`: `joints = [your_bone_count]`.
3. **(If Needed) Set Gating Indices**  
   - In `...\GNN\Network.py`, set `main_indices` and `gating_indices`
   - Use `metadata.txt` to find input dimensions, e.g., `403` for input features, plus `130` for the gating window (Windowsize * phase channel * ).

## 4. Local Training Pipeline
1. **Paths**: Adjust paths in the Python script (`LocalAutoPipeline.py` or similar) if needed:
   - `dataset_path` = `C:\DEV\DATASETS\...`
   - `path_to_ai4anim` = `C:\DEV\AI4Animation\...`
2. **Automated Steps**:
   - Reads sample counts from `sequences_velocity.txt`.
   - Reads feature sizes from `metadata.txt`.
   - Filters velocity data, copies to PAE folder, and runs **PAE**.
   - Prepares input/output data, copies to GNN folder, and runs **GNN**.
3. **Run**:
   ```bash
   python LocalAutoPipeline.py
   ```

5. After Training
- PAE outputs: `...\PAE\Training\...`
- GNN outputs: `...\GNN\Training\...`
- Use these trained networks in AnimHost or your runtime environment!
> **Note**: If you still need to manually update gating indices in the GNN Network.py, do so based on your new feature counts. Everything else is mostly automated!