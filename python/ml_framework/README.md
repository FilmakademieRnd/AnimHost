# How to run

## Option 1: Automated launcher (Recommended)
```powershell
cd python/ml_framework
launch_training.ps1
```

Or from the AnimHost root directory:
```powershell
python\ml_framework\launch_training.bat
```

This automatically activates the `animhost-ml-starke22` conda environment, runs training with real-time output streaming, and cleanly deactivates the environment when complete.

## Option 2: Direct Python execution (Legacy)
```powershell
cd python/ml_framework
python training.py
```
Note: Requires manual activation of `animhost-ml-starke22` conda environment first.

## Option 3: AnimHost GUI
1. Launch AnimHost application
2. Load `TestScenes/TrainingPipeline.flow`
3. Execute the training pipeline through the node interface

# How to test

## Run all tests
```bash
cd python/ml_framework/tests
pytest tests/ -v
```

## Run specific test modules
```bash
# Test experiment tracker only
pytest tests/test_experiment_tracker.py -v

# Test external training integration
pytest tests/test_external/test_example_training.py -v
```

# Developer Documentation

```mermaid
graph TB
    subgraph "AnimHost Node Framework (Qt/C++)"
        A["**TrainingNodeWidget**<br/><small>Qt UI widget displaying training progress and status</small>"]
        C["**MLFramework::TrainingMessage**<br/><small>C++ struct parsing JSON from Python (from_json method)</small>"]
        B["**TrainingNode**<br/><small>C++ node managing QProcess to launch Python training</small>"]
    end

    D["**JSON Message**<br/>"]

    subgraph "Python ML Framework"
        E["**ExperimentTracker**<br/><small>Captures and outputs training progress and std logger</small>"]
        F["**Training Script**<br/><small>Entry point coordinating training pipeline</small>"]
        G["**Starke Training Functions**<br/><small>Actual ML training implementation</small>"]
    end
    
    %% Connections
    B --> |"QProcess->start()"| F
    F --> |"subprocess.Popen(...)"| G
    G --> |"log_epoch(...)"| E
    E --- |"_emit_json(...)"| D
    D --> |"onTrainingOutput()"| B
    B --- C
    C --> |"updateFromMessage(*)"| A
    
    %% Styling
    classDef cppLayer fill:#065f46,stroke:#10b981,stroke-width:2px,color:#ffffff
    classDef interfaceNode fill:#374151,stroke:#6b7280,stroke-width:2px,color:#ffffff
    classDef pythonLayer fill:#1e40af,stroke:#3b82f6,stroke-width:2px,color:#ffffff
    
    class A,B,C cppLayer
    class D interfaceNode
    class E,F,G pythonLayer
```

## Integration Overview

The TrainingPlugin bridges AnimHost's C++ node framework with Python ML training scripts via JSON inter-process communication. The TrainingNode launches a Python training processes using QProcess. The training process will launch an additional subprocess when using an external experiment script. The ExperimentTracker provides structured logging from Python back to the UI. The ExperimentTracker also captures and shares external training script output and standard logger.

**Critical Interface Requirement**: Any changes to the JSON protocol must be synchronized between:
- `ExperimentTracker._emit_json()` method in Python (emits JSON with status, text, metrics fields)
- `MLFramework::TrainingMessage.from_json()` method in C++ (parses the same JSON structure)

This ensures consistent communication between the Python training pipeline and AnimHost's UI components. At this point, this implicit interface is preferred over a strictly enforced message structure for ease of iteration.

## External Training Script Integration

External training scripts (like Starke SIGGRAPH 2022's Network.py) are integrated as subprocesses with customized line parsers:

```python
# Example from starke_training.py
run_script_subprocess(
    script_name="Network.py",
    working_dir=gnn_path,
    model_name="Controller", 
    line_parser=lambda line, model_name: parse_training_output(line, model_name, tracker)
)
```

The `line_parser` function converts script-specific output formats into standardized JSON messages:
- Parses "Epoch 1 0.329..." → `tracker.log_epoch("Controller training", {"epoch": 1, "loss": 0.329})`
- Parses "Progress 23.42 %" → `tracker.log_percentage_progress("Controller training", 23.42)`

This architecture allows integration of any external training script by providing an appropriate line parser that translates its output format into the standard JSON protocol.