param(
    [string]$BuildDir = "build",
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

function Read-CMakeSetValue {
    param(
        [Parameter(Mandatory = $true)][string]$Content,
        [Parameter(Mandatory = $true)][string]$Name
    )

    $escapedName = [regex]::Escape($Name)
    $match = [regex]::Match($Content, "(?m)^\s*set\(\s*$escapedName\s+([^\s\)]+)")
    if (-not $match.Success) {
        return $null
    }

    return $match.Groups[1].Value.Trim('"')
}

function Resolve-Tool {
    param(
        [Parameter(Mandatory = $true)][string]$Path,
        [Parameter(Mandatory = $true)][string]$Name
    )

    if (-not (Test-Path -LiteralPath $Path)) {
        throw "$Name not found: $Path"
    }

    return (Resolve-Path -LiteralPath $Path).Path
}

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
    param(
        [string]$Board
    )

    if ($Board -match "^pico2" -or $Board -match "rp2350") {
        return "target/rp2350.cfg"
    }

    return "target/rp2040.cfg"
}

$scriptRoot = Split-Path -Parent $MyInvocation.MyCommand.Path
$cmakeListsPath = Join-Path $scriptRoot "CMakeLists.txt"

if (-not (Test-Path -LiteralPath $cmakeListsPath)) {
    throw "CMakeLists.txt not found next to this script."
}

$cmakeLists = Get-Content -LiteralPath $cmakeListsPath -Raw
$sdkVersion = Read-CMakeSetValue -Content $cmakeLists -Name "sdkVersion"
$toolchainVersion = Read-CMakeSetValue -Content $cmakeLists -Name "toolchainVersion"
$picoBoard = Read-CMakeSetValue -Content $cmakeLists -Name "PICO_BOARD"

if (-not $sdkVersion) {
    throw "Could not read sdkVersion from CMakeLists.txt."
}
if (-not $toolchainVersion) {
    throw "Could not read toolchainVersion from CMakeLists.txt."
}
if (-not $Target) {
    $Target = Get-OpenOcdTarget -Board $picoBoard
}

$homeDir = $env:USERPROFILE
if (-not $homeDir) {
    $homeDir = $HOME
}
if (-not $homeDir) {
    throw "Could not resolve user home directory."
}

$picoSdkRoot = Join-Path $homeDir ".pico-sdk"
$sdkPath = Resolve-Tool -Path (Join-Path $picoSdkRoot "sdk\$sdkVersion") -Name "Pico SDK $sdkVersion"
$toolchainPath = Resolve-Tool -Path (Join-Path $picoSdkRoot "toolchain\$toolchainVersion") -Name "Pico toolchain $toolchainVersion"
$cmakePath = Resolve-Tool -Path (Join-Path $picoSdkRoot "cmake\v4.3.4\bin\cmake.exe") -Name "CMake"
$ninjaPath = Resolve-Tool -Path (Join-Path $picoSdkRoot "ninja\v1.13.2\ninja.exe") -Name "Ninja"
$openOcdRoot = Resolve-OpenOcdRoot -PicoSdkRoot $picoSdkRoot
$openOcdPath = Resolve-Tool -Path (Join-Path $openOcdRoot "openocd.exe") -Name "OpenOCD"
$openOcdScriptsPath = Resolve-Tool -Path (Join-Path $openOcdRoot "scripts") -Name "OpenOCD scripts"
$gdbPath = Resolve-Tool -Path (Join-Path $toolchainPath "bin\arm-none-eabi-gdb.exe") -Name "GDB"

$env:PICO_SDK_PATH = $sdkPath
$env:PICO_TOOLCHAIN_PATH = $toolchainPath
$env:Path = @(
    (Join-Path $toolchainPath "bin")
    (Split-Path -Parent $cmakePath)
    (Split-Path -Parent $ninjaPath)
    $env:Path
) -join ";"

$buildPath = if ([System.IO.Path]::IsPathRooted($BuildDir)) {
    $BuildDir
}
else {
    Join-Path $scriptRoot $BuildDir
}

if (-not $NoBuild) {
    Write-Host "Configuring CMake for debug..."
    $configureArgs = @(
        "-S", $scriptRoot,
        "-B", $buildPath,
        "-G", "Ninja",
        "-DEUZEBIA3D_PLATFORM=PICO",
        "-DCMAKE_BUILD_TYPE=Debug"
    )
    if ($picoBoard) {
        $configureArgs += "-DPICO_BOARD=$picoBoard"
    }

    & $cmakePath @configureArgs
    if ($LASTEXITCODE -ne 0) {
        throw "CMake configure failed with exit code $LASTEXITCODE."
    }

    Write-Host "Building debug firmware..."
    & $ninjaPath -C $buildPath
    if ($LASTEXITCODE -ne 0) {
        throw "Build failed with exit code $LASTEXITCODE."
    }
}

$elfPath = Join-Path $buildPath "$ProgramName.elf"
if (-not (Test-Path -LiteralPath $elfPath)) {
    throw "ELF output not found: $elfPath"
}

if ($BuildOnly) {
    Write-Host "Debug ELF ready: $elfPath"
    exit 0
}

$openOcdStdout = Join-Path $buildPath "openocd.out.log"
$openOcdStderr = Join-Path $buildPath "openocd.err.log"
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
