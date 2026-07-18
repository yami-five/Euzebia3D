param(
    [string]$BuildDir = "build-pico",
    [string]$ProgramName = "Euzebia3D",
    [string]$Interface = "interface/cmsis-dap.cfg",
    [string]$Target = "",
    [int]$AdapterSpeed = 5000,
    [int]$OpenOcdStartupSeconds = 2,
    [switch]$NoBuild,
    [switch]$BuildOnly,
    [switch]$ServerOnly
)

$ErrorActionPreference = "Stop"

function Resolve-OpenOcdRoot {
    param(
        [Parameter(Mandatory = $true)][string]$PicoSdkRoot
    )

    $defaultRoot = Join-Path $PicoSdkRoot "openocd\0.12.0+dev"
    if (Test-Path -LiteralPath $defaultRoot) {
        return (Resolve-Path -LiteralPath $defaultRoot).Path
    }

    $openOcdParent = Join-Path $PicoSdkRoot "openocd"
    if (-not (Test-Path -LiteralPath $openOcdParent)) {
        throw "OpenOCD directory not found: $openOcdParent"
    }

    $root = Get-ChildItem -LiteralPath $openOcdParent -Directory |
        Sort-Object LastWriteTime -Descending |
        Select-Object -First 1
    if (-not $root) {
        throw "No OpenOCD installation found in $openOcdParent"
    }

    return $root.FullName
}

function Get-OpenOcdTarget {
    param([string]$Board)

    if ($Board -match "^pico2" -or $Board -match "rp2350") {
        return "target/rp2350.cfg"
    }

    return "target/rp2040.cfg"
}

$repositoryRoot = (Resolve-Path -LiteralPath (Split-Path -Parent $PSScriptRoot)).Path
$commonScript = Join-Path $repositoryRoot "tools\pico_common.ps1"
if (-not (Test-Path -LiteralPath $commonScript)) {
    throw "Shared Pico build helpers not found: $commonScript"
}
. $commonScript

$buildContext = Get-PicoBuildContext -RepositoryRoot $repositoryRoot -BuildDir $BuildDir
Initialize-PicoBuildEnvironment -BuildContext $buildContext

if (-not $Target) {
    $Target = Get-OpenOcdTarget -Board $buildContext.Board
}

$picoSdkRoot = Split-Path -Parent (Split-Path -Parent $buildContext.SdkPath)
$openOcdRoot = Resolve-OpenOcdRoot -PicoSdkRoot $picoSdkRoot
$openOcdPath = Resolve-RequiredPath -Path (Join-Path $openOcdRoot "openocd.exe") -Name "OpenOCD"
$openOcdScriptsPath = Resolve-RequiredPath -Path (Join-Path $openOcdRoot "scripts") -Name "OpenOCD scripts"
$gdbPath = Resolve-RequiredPath -Path (Join-Path $buildContext.ToolchainPath "bin\arm-none-eabi-gdb.exe") -Name "GDB"

if (-not $NoBuild) {
    Invoke-PicoBuild -RepositoryRoot $repositoryRoot -BuildContext $buildContext -BuildType "Debug"
}

$elfPath = Join-Path $buildContext.BuildPath "$ProgramName.elf"
if (-not (Test-Path -LiteralPath $elfPath)) {
    throw "ELF output not found: $elfPath"
}

if ($BuildOnly) {
    Write-Host "Debug ELF ready: $elfPath"
    exit 0
}

$openOcdStdout = Join-Path $buildContext.BuildPath "openocd.out.log"
$openOcdStderr = Join-Path $buildContext.BuildPath "openocd.err.log"
$openOcdArgs = @(
    "-s", $openOcdScriptsPath,
    "-f", $Interface,
    "-f", $Target,
    "-c", "adapter speed $AdapterSpeed"
)

Write-Host "Starting OpenOCD..."
Write-Host "Interface: $Interface"
Write-Host "Target: $Target"
$openOcdProcess = Start-Process `
    -FilePath $openOcdPath `
    -ArgumentList $openOcdArgs `
    -WorkingDirectory $openOcdScriptsPath `
    -RedirectStandardOutput $openOcdStdout `
    -RedirectStandardError $openOcdStderr `
    -WindowStyle Hidden `
    -PassThru

Start-Sleep -Seconds $OpenOcdStartupSeconds

if ($openOcdProcess.HasExited) {
    Write-Host "OpenOCD stdout:"
    if (Test-Path -LiteralPath $openOcdStdout) {
        Get-Content -LiteralPath $openOcdStdout
    }
    Write-Host "OpenOCD stderr:"
    if (Test-Path -LiteralPath $openOcdStderr) {
        Get-Content -LiteralPath $openOcdStderr
    }
    throw "OpenOCD exited early with code $($openOcdProcess.ExitCode)."
}

if ($ServerOnly) {
    Write-Host "OpenOCD is running. PID: $($openOcdProcess.Id)"
    Write-Host "GDB target: localhost:3333"
    Write-Host "Logs: $openOcdStdout and $openOcdStderr"
    exit 0
}

try {
    Write-Host "Starting GDB..."
    Write-Host "The firmware will be loaded, reset, and stopped at main."
    $gdbElfPath = $elfPath.Replace("\", "/")
    $gdbArgs = @(
        "-ex", "target extended-remote localhost:3333",
        "-ex", "monitor reset halt",
        "-ex", "load",
        "-ex", "break main",
        "-ex", "continue",
        $gdbElfPath
    )
    & $gdbPath @gdbArgs
}
finally {
    if ($openOcdProcess -and -not $openOcdProcess.HasExited) {
        Stop-Process -Id $openOcdProcess.Id
    }
}
