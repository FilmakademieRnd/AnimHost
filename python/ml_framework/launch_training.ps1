# AnimHost Training Launcher (PowerShell)
# Automatically installs Miniconda (if needed), creates the training environment,
# and runs training scripts with json stdout real-time streaming to the C++ host application.
#
# CONDA INTEGRATION NOTE:
# This script uses a PATH-only conda installation approach suitable for enterprise
# environments with restricted PowerShell execution policies. It does NOT run 'conda init'
# which means:
#   - The 'conda activate' command will NOT work in your terminal
#   - Conda commands (conda install, conda env list, etc.) WILL work via PATH
#   - This script calls the environment's Python executable directly for real-time output
#
# To use conda activate in your terminal, manually run: conda init powershell
# (requires execution policy that allows profile scripts)

param(
    [Parameter(ValueFromRemainingArguments)]
    [string[]]$TrainingArgs
)

# Configuration
$TARGET_ENV = "animhost-ml-starke22"
$TRAINING_SCRIPT = Join-Path $PSScriptRoot "training.py"
$ENV_FILE = Join-Path $PSScriptRoot "environments\$TARGET_ENV.yml"

# Emits a JSON status message for consumption by TrainingPlugin (C++)
# Parameters:
#   $status - Status category (e.g., "Environment Error", "Downloading Miniconda")
#   $text   - Detailed status message
function Write-JsonStatus($status, $text) {
    $json = [ordered]@{
        status = $status
        text = $text
    } | ConvertTo-Json -Compress
    Write-Host $json
}

# Installs Miniconda using winget (Windows Package Manager)
# Returns $true on success, $false on failure
# Winget handles PATH and installation automatically
function Install-Miniconda {
    # Check if winget is available
    try {
        $null = Get-Command winget -ErrorAction Stop
    } catch {
        Write-JsonStatus "Environment Error" "winget not found. Please install winget to enable automatic Miniconda installation"
        Write-JsonStatus "Environment Error" "Winget comes pre-installed on Windows 11 and Windows 10 (1809+). Install 'App Installer' from Microsoft Store if missing"
        return $false
    }

    Write-JsonStatus "Installing Miniconda" "Installing via winget (this may take 2-5 minutes)..."

    # Install Miniconda using winget with silent mode
    & winget install Anaconda.Miniconda3 --silent --accept-package-agreements --accept-source-agreements
    if ($LASTEXITCODE -ne 0) {
        Write-JsonStatus "Install Error" "Miniconda installation via winget failed with exit code $LASTEXITCODE"
        return $false
    }

    # Find conda installation path
    Write-JsonStatus "Initializing Conda" "Locating conda installation..."
    $condaPaths = @(
        "$env:USERPROFILE\miniconda3",
        "$env:LOCALAPPDATA\miniconda3",
        "C:\ProgramData\miniconda3",
        "$env:USERPROFILE\AppData\Local\Microsoft\WindowsApps\Anaconda.Miniconda3_*"
    )

    $condaPath = $null
    foreach ($path in $condaPaths) {
        if (Test-Path $path\Scripts\conda.exe) {
            $condaPath = $path
            Write-JsonStatus "Conda Found" "Conda installation found at $condaPath"
            break
        }
    }

    if (-not $condaPath) {
        Write-JsonStatus "Initialization Warning" "Could not find conda installation path. You may need to restart your terminal"
        return $true
    }

    # Add conda to User PATH permanently for future sessions
    # The alternative conda init powershell doesn't work in enterprise environments with restricted execution policy
    Write-JsonStatus "Updating PATH" "Adding conda to system PATH..."
    $userPath = [System.Environment]::GetEnvironmentVariable("PATH", "User")
    $condaBinPaths = "$condaPath;$condaPath\Scripts;$condaPath\condabin"

    if ($userPath -notlike "*$condaPath*") {
        [System.Environment]::SetEnvironmentVariable(
            "PATH",
            "$condaBinPaths;$userPath",
            "User"
        )
        Write-JsonStatus "PATH Updated" "Conda added to User PATH for future sessions"
    }

    # Update current session PATH
    $env:PATH = "$condaBinPaths;$env:PATH"

    # Accept conda Terms of Service for required channels
    Write-JsonStatus "Accepting ToS" "Accepting conda channel Terms of Service..."
    & "$condaPath\Scripts\conda.exe" tos accept --override-channels --channel https://repo.anaconda.com/pkgs/main 2>&1 | Out-Null
    & "$condaPath\Scripts\conda.exe" tos accept --override-channels --channel https://repo.anaconda.com/pkgs/r 2>&1 | Out-Null
    & "$condaPath\Scripts\conda.exe" tos accept --override-channels --channel https://repo.anaconda.com/pkgs/msys2 2>&1 | Out-Null

    Write-JsonStatus "Installation Complete" "Miniconda installed and initialized successfully"
    return $true
}

# Check if conda is available
try {
    $null = Get-Command conda -ErrorAction Stop
} catch {
    Write-JsonStatus "Conda Missing" "conda not found in PATH. Attempting automatic installation..."

    $installed = Install-Miniconda
    if (-not $installed) {
        Write-JsonStatus "Environment Error" "Failed to install Miniconda automatically"
        Write-JsonStatus "Environment Error" "Please install conda/miniconda manually from https://docs.conda.io/en/latest/miniconda.html"
        exit 1
    }

    # Verify conda is now available
    try {
        $null = Get-Command conda -ErrorAction Stop
        Write-JsonStatus "Conda Ready" "Conda installation verified and ready to use"
    } catch {
        Write-JsonStatus "Environment Error" "Conda installed but not found in PATH. Please restart your terminal and try again"
        exit 1
    }
}

# Check if target environment exists
Write-JsonStatus "Checking Environment" "Checking if environment $TARGET_ENV exists..."
$envList = & conda env list 2>$null | Select-String -Pattern "^$TARGET_ENV\s"
if (-not $envList) {
    Write-JsonStatus "Environment Missing" "Conda environment $TARGET_ENV not found"

    # Check if environment YAML file exists
    if (-not (Test-Path $ENV_FILE)) {
        Write-JsonStatus "Environment Error" "Environment file not found: $ENV_FILE"
        Write-JsonStatus "Environment Error" "Please create the environment file or run: conda env create -f environment.yml -n $TARGET_ENV"
        exit 1
    }

    # Create environment automatically
    Write-JsonStatus "One-time Env Setup" "Creating environment from $TARGET_ENV.yml - this may take 2-5 minutes..."
    & conda env create -f $ENV_FILE -n $TARGET_ENV
    if ($LASTEXITCODE -ne 0) {
        Write-JsonStatus "Environment Error" "Failed to create environment $TARGET_ENV. Removing incomplete environment."
        & conda env remove -n $TARGET_ENV -y
        Write-JsonStatus "Environment Error" "Please try running the script again or manually create the environment with: conda env create -f $ENV_FILE -n $TARGET_ENV"
        exit 1
    }

    Write-JsonStatus "Verifying Environment" "Waiting for environment to be registered..."

    # Verify environment creation by checking if it appears in env list
    $verified = $false
    $envList = & conda env list 2>$null | Select-String -Pattern "^$TARGET_ENV\s"
    if ($envList) {
        $verified = $true
    }

    # Retry - 3 second wait with cache refresh
    if (-not $verified) {
        Write-JsonStatus "Verification Retry" "Retry: Environment not ready, waiting 3 seconds..."
        Start-Sleep 3
        $null = & conda info --envs 2>$null
        $envList = & conda env list 2>$null | Select-String -Pattern "^$TARGET_ENV\s"
        if ($envList) {
            $verified = $true
        }
    }

    if (-not $verified) {
        Write-JsonStatus "Environment Error" "Failed to verify newly created environment $TARGET_ENV. Removing incomplete environment."
        & conda env remove -n $TARGET_ENV -y
        Write-JsonStatus "Environment Error" "Please try running the script again or manually create the environment with: conda env create -f $ENV_FILE -n $TARGET_ENV"
        exit 1
    }

    Write-JsonStatus "Environment Ready" "Environment $TARGET_ENV created successfully"
}

# Get the environment's Python executable path
$envPythonPath = & conda run -n $TARGET_ENV where python 2>$null | Select-Object -First 1
if (-not $envPythonPath) {
    Write-JsonStatus "Environment Error" "Could not find Python in environment $TARGET_ENV"
    exit 1
}

# Run training with unbuffered output for real-time streaming
# Use the environment's Python directly to preserve real-time stdout
$trainingExitCode = 0
if ($TrainingArgs) {
    & $envPythonPath -u $TRAINING_SCRIPT $TrainingArgs
    $trainingExitCode = $LASTEXITCODE
} else {
    & $envPythonPath -u $TRAINING_SCRIPT
    $trainingExitCode = $LASTEXITCODE
}

# Exit with training script's exit code
exit $trainingExitCode
