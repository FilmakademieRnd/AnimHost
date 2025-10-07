# AnimHost Training Launcher (PowerShell)
# Ensures training runs in the correct conda environment

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

    # Refresh environment to pick up PATH changes
    $env:PATH = [System.Environment]::GetEnvironmentVariable("PATH", "User") + ";" + [System.Environment]::GetEnvironmentVariable("PATH", "Machine")

    # Initialize conda for PowerShell in the current session
    Write-JsonStatus "Initializing Conda" "Setting up conda for PowerShell..."

    # Find conda installation path
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

    # Initialize conda for PowerShell (run conda init powershell)
    & "$condaPath\Scripts\conda.exe" init powershell 2>&1 | Out-Null

    # Source the conda hook for current session
    $condaHook = "$env:USERPROFILE\Documents\WindowsPowerShell\profile.ps1"
    if (Test-Path $condaHook) {
        . $condaHook
    }

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

# Check if target environment exists by attempting activation
Write-JsonStatus "Checking Environment" "Checking if environment $TARGET_ENV exists..."
$activateResult = & conda activate $TARGET_ENV 2>$null
if ($LASTEXITCODE -ne 0) {
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
        & conda env remove -n $TARGET_ENV
        Write-JsonStatus "Environment Error" "Please try running the script again or manually create the environment with: conda env create -f $ENV_FILE -n $TARGET_ENV"
        exit 1
    }

    Write-JsonStatus "Verifying Environment" "Waiting for environment to be registered..."

    # Verify environment creation by attempting activation
    $verified = $false
    $testResult = & conda activate $TARGET_ENV 2>$null
    if ($LASTEXITCODE -eq 0) {
        $verified = $true
    }

    # Retry - 3 second wait with cache refresh
    if (-not $verified) {
        Write-JsonStatus "Verification Retry" "Retry: Environment not ready, waiting 3 seconds..."
        Start-Sleep 3
        $null = & conda info --envs 2>$null
        $testResult = & conda activate $TARGET_ENV 2>$null
        if ($LASTEXITCODE -eq 0) {
            $verified = $true
        }
    }

    if (-not $verified) {
        Write-JsonStatus "Environment Error" "Failed to verify newly created environment $TARGET_ENV. Removing incomplete environment."
        & conda env remove -n $TARGET_ENV
        Write-JsonStatus "Environment Error" "Please try running the script again or manually create the environment with: conda env create -f $ENV_FILE -n $TARGET_ENV"
        exit 1
    }

    Write-JsonStatus "Environment Ready" "Environment $TARGET_ENV created successfully"
}

# Run training with unbuffered output for real-time streaming
$trainingExitCode = 0
try {
    if ($TrainingArgs) {
        & python -u $TRAINING_SCRIPT $TrainingArgs
    } else {
        & python -u $TRAINING_SCRIPT
    }
    $trainingExitCode = $LASTEXITCODE
} finally {
    # Deactivate conda environment
    & conda deactivate 2>$null
}

# Exit with training script's exit code
exit $trainingExitCode
