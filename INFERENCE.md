# Running Character Animation Inference

This guide walks you through running AI-powered character animation inference using trained models in the AnimHost pipeline. You will load a Gated Neural Network (GNN) model to generate character animations in real-time.

## Prerequisites

- **AnimHost Release**: Download the latest release from [GitHub Releases](https://github.com/FilmakademieRnd/AnimHost/releases/tag/v0.1.0)
- **Blender**: Blender 4.2 (4.2.9 confirmed working, 4.1 does not work)
- **DataHub**: For communication between AnimHost and Blender
- **Trained Models**: Pre-trained ONNX models (see Dev Exchange/Model Zoo or train your own using [TRAINING.md](TRAINING.md))

## Quick Start: Complete Inference Workflow

### Step 1: Setup Blender

1. Install **Blender 4.2** (version 4.2.9 confirmed working)
   - Download from [blender.org](https://www.blender.org/download/)
   - **Important**: Blender 4.1 does not work with this pipeline

2. Install the TracerSceneDistribution Blender add-on:
   - Clone the repository: [TracerSceneDistribution](https://github.com/FilmakademieRnd/TracerSceneDistribution/tree/dev)
   - Use the `dev` branch (also tested successfully on merged main branch)
   - Zip the `Blender` directory from the repository
   - In Blender: **Edit → Preferences → Add-ons → Install**
   - Select the zipped Blender directory and enable the add-on

3. Open the Survivor character scene:
   - Use `Survivor_Base_wPath` scene file

### Step 2: Run DataHub

DataHub facilitates communication between AnimHost and Blender.

1. Clone the DataHub repository: [DataHub GitHub](https://github.com/FilmakademieRnd/DataHub)

2. Launch DataHub with the following command:
```powershell
.\DataHub.exe -nl -np -ownIP 127.0.0.1 -d
```

**Command line parameters:**
- `-nl`: No logging to file
- `-np`: No port forwarding
- `-ownIP 127.0.0.1`: Set local IP address
- `-d`: Debug mode

3. Keep DataHub running in the background during inference

### Step 3: Configure AnimHost for Inference

1. Launch `AnimHost.exe` from the release package

2. Load the inference pipeline: **File → Load Scene → TestScenes/TrainingPipeline.flow**

3. Configure the Animation Import node:
   - Select an animation example from your dataset (e.g., `Survivor_AnimHost_Mocap_Dataset_2025/FBX/Skeleton_Example`)
   - This serves as a reference skeleton/animation for the inference system

4. Configure the ControlPathDecoderNode:
   - **Important**: Make sure `Path from Blender` is **checked**
   - This enables the node to receive path control data from Blender

5. Configure the Adjust Locomotion Generator (2D Spline) node:
   - Select the desired trained ONNX model (see Dev Exchange/Model Zoo or use your trained model from [TRAINING.md](TRAINING.md))
   - Set `Path Position Influence` to **0.48**
   - Set `Path Rotation Influence` to **0.3**
   - These values control how strongly the character follows the path. Lower parameters means forcing the model output to follow the input path more strictly.

### Step 4: Request Animation in Blender


1. In Blender, with the `Survivor_Base_wPath` scene open:
   - Follow the PDF add-on instructions provided with TracerSceneDistribution

2. **Quick workflow** (TLDR):
   - Click the **red highlighted buttons** in the add-on panel
   - Use the default request mode: **"As a Block"**
   - Click **Request Animation**

3. The trained model will generate animation based on:
   - The path/spline you defined in Blender
   - The control parameters set in AnimHost
   - The locomotion patterns learned during training

4. Once you see a Animation received message on the bottom of the screen you can play the animation.