# ML Framework Developer Documentation

```mermaid
graph TB
    subgraph "AnimHost Node Framework (Qt/C++)"
        A["TrainingNodeWidget<br/><small>Qt UI widget displaying training progress and status</small>"]   
        C["MLFramework::TrainingMessage<br/><small>C++ struct parsing JSON from Python (from_json method)</small>"]
        B["TrainingNode<br/><small>C++ node managing QProcess to launch Python training</small>"]
    end
    
    D["JSON Message<br/>"]
    
    subgraph "Python ML Framework"
        E["ExperimentTracker<br/><small>Captures and outputs training progress and std logger</small>"]
        F["Training Script<br/><small>Entry point coordinating training pipeline</small>"]
        G["Starke Training Functions<br/><small>Actual ML training implementation</small>"]
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
    classDef cppLayer fill:#90EE90,stroke:#2c3e50,stroke-width:2px
    classDef interfaceNode fill:#f8f9fa,stroke:#6c757d,stroke-width:2px
    classDef pythonLayer fill:#FFFF00,stroke:#3498db,stroke-width:2px
    
    class A,B,C cppLayer
    class D interfaceNode
    class E,F,G pythonLayer
```

## Integration Overview

The TrainingPlugin bridges AnimHost's C++ node framework with Python ML training scripts via JSON inter-process communication. TrainingNode launches Python processes using QProcess, while ExperimentTracker provides structured logging from Python back to the UI.

**Critical Interface Requirement**: Any changes to the JSON protocol must be synchronized between:
- `ExperimentTracker._emit_json()` method in Python (emits JSON with status, text, metrics fields)
- `MLFramework::TrainingMessage.from_json()` method in C++ (parses the same JSON structure)

This ensures consistent communication between the Python training pipeline and AnimHost's UI components. At this point, this implicit interface is preferred over an explicit message structure for ease of iteration.

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