param(
    [string]$BuildDir = "build",
    [int]$WaitSeconds = 120,
    [switch]$Clean
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

function Get-RpiRp2Drive {
    try {
        $volume = Get-Volume -FileSystemLabel "RPI-RP2" -ErrorAction Stop |
            Where-Object { $_.DriveLetter } |
            Select-Object -First 1

        if ($volume) {
            return "$($volume.DriveLetter):\"
        }
    }
    catch {
    }

    foreach ($drive in Get-PSDrive -PSProvider FileSystem) {
        $infoFile = Join-Path $drive.Root "INFO_UF2.TXT"
        if (Test-Path -LiteralPath $infoFile) {
            try {
                $info = Get-Content -LiteralPath $infoFile -Raw -ErrorAction Stop
                if ($info -match "UF2" -and $info -match "Raspberry Pi") {
                    return $drive.Root
                }
            }
            catch {
            }
        }
    }

    return $null
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

if ($Clean -and (Test-Path -LiteralPath $buildPath)) {
    Remove-Item -LiteralPath $buildPath -Recurse -Force
}

Write-Host "Configuring CMake..."
$configureArgs = @(
    "-S", $scriptRoot,
    "-B", $buildPath,
    "-G", "Ninja",
    "-DEUZEBIA3D_PLATFORM=PICO"
)
if ($picoBoard) {
    $configureArgs += "-DPICO_BOARD=$picoBoard"
}

& $cmakePath @configureArgs
if ($LASTEXITCODE -ne 0) {
    throw "CMake configure failed with exit code $LASTEXITCODE."
}

Write-Host "Building firmware..."
& $ninjaPath -C $buildPath
if ($LASTEXITCODE -ne 0) {
    throw "Build failed with exit code $LASTEXITCODE."
}

$uf2Path = Join-Path $buildPath "Euzebia3D.uf2"
if (-not (Test-Path -LiteralPath $uf2Path)) {
    throw "UF2 output not found: $uf2Path"
}

Write-Host "Waiting for Pico BOOTSEL drive RPI-RP2..."
Write-Host "If it is not visible, hold BOOTSEL while plugging the Pico into USB."

$deadline = (Get-Date).AddSeconds($WaitSeconds)
$driveRoot = $null
while ((Get-Date) -lt $deadline) {
    $driveRoot = Get-RpiRp2Drive
    if ($driveRoot) {
        break
    }

    Start-Sleep -Milliseconds 500
}

if (-not $driveRoot) {
    throw "RPI-RP2 drive not found within $WaitSeconds seconds."
}

$destination = Join-Path $driveRoot (Split-Path -Leaf $uf2Path)
Write-Host "Copying firmware to $destination..."
Copy-Item -LiteralPath $uf2Path -Destination $destination -Force

Write-Host "Done. The Pico should reboot automatically."
