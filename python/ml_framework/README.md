# Training Character Animation Models

This guide walks you through training AI-powered character animation models using the AnimHost pipeline and motion capture data. You will train a Phase Autoencoder (PAE) and Gated Neural Network (GNN).

## Prerequisites

- **AnimHost Release**: Download the latest release from [GitHub Releases](https://github.com/FilmakademieRnd/AnimHost/releases)
- **Windows**: Windows 10/11 with PowerShell
- **GPU**: NVIDIA GPU recommended for faster training
- **Disk Space**: ~20GB for intermediate trained model onnx files (assumung 30 PAE and 300 GNN epochs)

## Quick Start: Complete Training Workflow

### Step 1: Download AI4Animation Framework

Download the [AI4Animation repository](https://github.com/sebastianstarke/AI4Animation) code as a zip file and unpack it next to your AnimHost release. The compressed repository is 3.3GB so this will take some time.

![alt text](<../../doc/resources/ai4animation_code_download_zip.png>)

**Recommended directory structure:**
```
C:/My-AnimHost-Run/
├── AnimHost/
│   ├── AnimHost.exe
│   ├── TestScenes/
│   └── python/
└── AI4Animation-master/
    └── AI4Animation/
        └── SIGGRAPH_2022/
```

**Copyright Notice**: AI4Animation is only for research or education purposes, and not freely available for commercial use or redistribution.

### Step 2: Download Motion Capture Dataset

Download the *SURVIVOR AnimHost Motion Capture Dataset 2025* under the *ANIMHOST MOTION CAPTURE DATASET 2024 & 2025* section [here](https://animationsinstitut.de/en/research/projects/max-r). Place it with the AnimHost and AI4Animation directory. If you want to train without mirrored data you need to move the data files into their own dir (below Default).
```
C:/My-AnimHost-Run/
...
└── Survivor_AnimHost_Mocap_Dataset_2025/
    ├── FBX/
    │   ├── Default/
    │   │   ├── 01_....fbx
    │   │   └── 02_....fbx
    │   └── Mirror/
    │       ├── 01_...Mirrored.fbx
    │       └── 02_...Mirrored.fbx
    └── Survivor_Mocap_dataset_2025_Readme_License.html
```

### Step 3: Preprocess Motion Capture Data

1. Create a directory for your processed dataset. E.g.:
```
C:/My-AnimHost-Run/
...
└── Survivor_Training_Data/
```
Note: Clear this dir if you already have it, there is a bug that blocks overwriting of some files.

2. Launch `AnimHost.exe` from the release package

3. Load the preprocessing pipeline: **File → Open → AnimHost/TestScenes/Preprocessing.flow**

4. Configure the nodes:
   - In the *Animation Import* node select the SURVIVOR dataset directory *.../FBX/Default* (or *.../FBX/Test* with just a few files for small trial run dataset)
   - In the *DataExportPlugin* node select the output dir *C:/My-AnimHost-Run/Survivor_Training_Data*. And make sure `overwrite` and `writeBin` are checked.
   - In the *LocomotionPreprocessNode* select the output dir *C:/My-AnimHost-Run/Survivor_Training_Data* and `overwrite` option.

5. Click **Run** to generate training data files

This outputs preprocessed binary data (`data_x.bin`, `data_y.bin`) and metadata files needed for training.

### Step 4: Train the Animation Model

1. In AnimHost, load the training pipeline: **File → Open → TestScenes/TrainingPipeline.flow**

2. Configure the dataset node:
    - Select the `Dataset Path` you used, *C:/My-AnimHost-Run/Survivor_Training_Data*.
    - Select the `AI4Animation Path` you used, *C:/My-AnimHost-Run/AI4Animation-master*.
    - Set the `PAE epochs` and `GNN epochs` to 2 epochs for a test run. Or use the default configuration for a training run that takes ~12 hours on a recent Nvidia GPU.

3. Click **Run** to start training

The training process will:
- Automatically install Miniconda if not present (Windows only)
- Create the `animhost-ml-starke22` conda environment
- Train the Phase Autoencoder (PAE) and Gated Neural Network (GNN)
- Display real-time training progress in the UI



## Outputs

After training completes, you'll find:

- **Phase Autoencoder**: `AI4Animation/SIGGRAPH_2022/PyTorch/PAE/Training/`
- **Gated Neural Network**: `AI4Animation/SIGGRAPH_2022/PyTorch/GNN/Training/`

These trained models can be integrated into AnimHost for real-time character animation.


## Alternative Training Methods

For developers and advanced users, see [DEV_GUIDE.md](DEV_GUIDE.md) for:
- **Option 1**: Self built AnimHost GUI
- **Option 2**: Automated PowerShell launcher
- **Option 3**: Direct Python execution (cross-platform)
- Developer documentation and architecture diagrams
- Testing instructions


## Troubleshooting

**Issue**: The TrainingNode looks stuck at `One-time Env Setup`.
**Solution**: This does take some time because it downloads and installs larger libraries like PyTorch and Cuda. Do expect a few minutes. You'll only need this step for your very first run.


For technical issues, see [DEV_GUIDE.md](DEV_GUIDE.md) or [open an issue](https://github.com/FilmakademieRnd/AnimHost/issues).

---

## About
![](/doc/resources/FA_AI_Logo.png) &nbsp;&nbsp;&nbsp;&nbsp;
![](/doc/resources/logo_rnd.jpg) &nbsp;&nbsp;&nbsp;&nbsp;
![](/doc/resources/Max-R_Logo.png)

AnimHost is a development by [Filmakademie Baden-Wuerttemberg](https://filmakademie.de/), [Animationsinstitut R&D Labs](http://research.animationsinstitut.de/) in the scope of the EU funded project [MAX-R](https://max-r.eu/) (101070072).

## Funding
![Animationsinstitut R&D](/doc/resources/EN_FundedbytheEU_RGB_POS_rs.png)

This project has received funding from the European Union's Horizon Europe Research and Innovation Programme under Grant Agreement No 101070072 MAX-R.

## License
AnimHost is a open-sorce development by Filmakademie Baden-Wuerttemberg's Animationsinstitut.  
The framework is licensed under [MIT](LICENSE.txt). See [License info file](LICENSE_Info.txt) for more details.
