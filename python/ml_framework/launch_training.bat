@echo off
REM AnimHost Training Launcher
REM Ensures training runs in the correct conda environment

setlocal

set TARGET_ENV=animhost-ml-starke22
set TRAINING_SCRIPT=%~dp0training.py

REM Check if conda is available
where conda >nul 2>&1
if %ERRORLEVEL% neq 0 (
    echo Error: conda not found in PATH
    echo Please install conda/miniconda and add it to your PATH
    exit /b 1
)

REM Check if target environment exists
conda info --envs | findstr "%TARGET_ENV%" >nul 2>&1
if %ERRORLEVEL% neq 0 (
    echo Error: conda environment '%TARGET_ENV%' not found
    echo.
    echo To create the environment, run:
    echo   conda env create -f environment.yml -n %TARGET_ENV%
    echo.
    echo TODO: Implement automatic environment creation from environment.yml
    exit /b 1
)

REM Activate conda environment, run training with real-time output, then deactivate
call conda activate %TARGET_ENV%
if %ERRORLEVEL% neq 0 (
    echo Error: Failed to activate conda environment '%TARGET_ENV%'
    exit /b 1
)

REM Run training with unbuffered output for real-time streaming
python -u "%TRAINING_SCRIPT%" %*
set TRAINING_EXIT_CODE=%ERRORLEVEL%

REM Deactivate conda environment
call conda deactivate

REM Exit with training script's exit code
exit /b %TRAINING_EXIT_CODE%