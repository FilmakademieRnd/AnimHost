@echo off
REM AnimHost Training Launcher
REM Ensures training runs in the correct conda environment

setlocal EnableDelayedExpansion

set TARGET_ENV=animhost-ml-starke22
set TRAINING_SCRIPT=%~dp0training.py
set ENV_FILE=%~dp0environments\%TARGET_ENV%.yml
set MAX_RETRIES=10
set RETRY_DELAY=2

REM Check if conda is available
where conda >nul 2>&1
if %ERRORLEVEL% neq 0 (
    echo {"status": "Environment Error", "text": "conda not found in PATH. Please install conda/miniconda and add it to your PATH"}
    exit /b 1
)

REM Check if target environment exists by attempting activation
call conda activate %TARGET_ENV% >nul 2>&1
if %ERRORLEVEL% neq 0 (
    echo {"status": "Environment Missing", "text": "Conda environment '%TARGET_ENV%' not found"}

    REM Check if environment YAML file exists
    if not exist "%ENV_FILE%" (
        echo {"status": "Environment Error", "text": "Environment file not found: %ENV_FILE%"}
        echo {"status": "Environment Error", "text": "Please create the environment file or run: conda env create -f environment.yml -n %TARGET_ENV%"}
        exit /b 1
    )

    REM Create environment automatically
    echo {"status": "One-time Env Setup", "text": "Creating environment from %TARGET_ENV%.yml - this may take 2-5 minutes..."}
    conda env create -f "%ENV_FILE%" -n %TARGET_ENV%

    echo {"status": "Verifying Environment", "text": "Waiting for environment to be registered..."}

    REM Try verification multiple times with increasing delays
    timeout /t 2 /nobreak >nul 2>&1
    conda run -n %TARGET_ENV% python --version >nul 2>&1
    if %ERRORLEVEL% equ 0 goto :ENV_SUCCESS

    echo {"status": "Verification Retry", "text": "Retry 1/3: Environment not ready, waiting 3 seconds..."}
    timeout /t 3 /nobreak >nul 2>&1
    conda info --envs >nul 2>&1
    conda run -n %TARGET_ENV% python --version >nul 2>&1
    if %ERRORLEVEL% equ 0 goto :ENV_SUCCESS

    echo {"status": "Verification Retry", "text": "Retry 2/3: Environment not ready, waiting 5 seconds..."}
    timeout /t 5 /nobreak >nul 2>&1
    conda info --envs >nul 2>&1
    conda run -n %TARGET_ENV% python --version >nul 2>&1
    if %ERRORLEVEL% equ 0 goto :ENV_SUCCESS

    echo {"status": "Verification Retry", "text": "Retry 3/3: Environment not ready, trying fallback..."}
    echo {"status": "Debug", "text": "Checking conda env list output..."}
    conda env list
    echo.
    echo {"status": "Debug", "text": "Testing conda run command..."}
    conda run -n %TARGET_ENV% python --version
    if %ERRORLEVEL% equ 0 (
        echo {"status": "Environment Ready", "text": "conda run successful - environment is functional"}
        goto :ENV_SUCCESS
    )
    echo.
    echo {"status": "Debug", "text": "conda run failed, checking conda env list..."}
    conda env list | findstr /C:"%TARGET_ENV%" >nul 2>&1
    if %ERRORLEVEL% equ 0 (
        echo {"status": "Environment Ready", "text": "Environment found in conda env list - assuming functional"}
        goto :ENV_SUCCESS
    )

    echo {"status": "Environment Error", "text": "Failed to verify newly created environment '%TARGET_ENV%'"}
    echo {"status": "Environment Error", "text": "Environment may have been created but not detected. Try: conda activate %TARGET_ENV%"}
    exit /b 1

    :ENV_SUCCESS
    echo {"status": "Environment Ready", "text": "Environment '%TARGET_ENV%' created successfully"}
)

REM Deactivate environment from initial check (line 21) if it was activated
call conda deactivate >nul 2>&1

REM Run training with conda run (more reliable than activate in batch scripts)
echo {"status": "Starting Training", "text": "Running training script in environment '%TARGET_ENV%'..."}
conda run -n %TARGET_ENV% python -u "%TRAINING_SCRIPT%" %*
set TRAINING_EXIT_CODE=%ERRORLEVEL%

REM Exit with training script's exit code
exit /b %TRAINING_EXIT_CODE%