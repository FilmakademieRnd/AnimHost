# ML Framework Development Notes

## Python Scripts in Build System

### Current Implementation (Option 1)
The TrainingNode currently references Python scripts directly from the source tree:
```cpp
_pythonScriptPath = QApplication::applicationDirPath() + "/../../python/ml_framework/mock_training.py";
```

This approach works for development but has deployment limitations.

### TODO: Production Build Integration (Option 2)
Consider implementing CMake install for Python files in future releases:

```cmake
# Add to TrainingPlugin/CMakeLists.txt
install(FILES 
    ${CMAKE_SOURCE_DIR}/python/ml_framework/mock_training.py
    # Add other training scripts here
    DESTINATION ${CMAKE_INSTALL_BINDIR}/python/ml_framework
)
```

**Benefits of future implementation:**
- Predictable deployment paths
- Clean separation of source vs runtime files  
- Better version control alignment between C++ and Python components
- Simplified distribution packaging

**Implementation considerations:**
- Add CMake flag to toggle between development (source references) and production (installed files) modes
- Update `TrainingNode` constructor to detect and use installed Python files when available
- Ensure development workflow remains smooth for rapid Python script iteration

**Priority:** Low - current approach sufficient for development and testing phase.