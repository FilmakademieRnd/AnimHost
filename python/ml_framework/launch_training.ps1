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
$MAX_RETRIES = 10
$RETRY_DELAY = 2

function Write-JsonStatus($status, $text) {
    $json = @{
        status = $status
        text = $text
    } | ConvertTo-Json -Compress
    Write-Host $json
}

# Check if conda is available
try {
    $null = Get-Command conda -ErrorAction Stop
} catch {
    Write-JsonStatus "Environment Error" "conda not found in PATH. Please install conda/miniconda and add it to your PATH"
    exit 1
}

# Check if target environment exists by attempting activation
Write-JsonStatus "Checking Environment" "Checking if environment '$TARGET_ENV' exists..."
$activateResult = & conda activate $TARGET_ENV 2>$null
if ($LASTEXITCODE -ne 0) {
    Write-JsonStatus "Environment Missing" "Conda environment '$TARGET_ENV' not found"

    # Check if environment YAML file exists
    if (-not (Test-Path $ENV_FILE)) {
        Write-JsonStatus "Environment Error" "Environment file not found: $ENV_FILE"
        Write-JsonStatus "Environment Error" "Please create the environment file or run: conda env create -f environment.yml -n $TARGET_ENV"
        exit 1
    }

    # Create environment automatically
    Write-JsonStatus "One-time Env Setup" "Creating environment from $TARGET_ENV.yml - this may take 2-5 minutes..."
    & conda env create -f $ENV_FILE -n $TARGET_ENV

    Write-JsonStatus "Verifying Environment" "Waiting for environment to be registered..."

    # Retry verification with increasing delays
    $verified = $false

    # First attempt - no wait
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
        Write-JsonStatus "Environment Error" "Failed to verify newly created environment '$TARGET_ENV'. Removeing incomplete environment."
        Write-JsonStatus "Environment Error" "Please try running the script again or manually create the environment with: conda env create -f $ENV_FILE -n $TARGET_ENV"
        & conda env remove -n $TARGET_ENV
        exit 1
    }

    Write-JsonStatus "Environment Ready" "Environment '$TARGET_ENV' created successfully"
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