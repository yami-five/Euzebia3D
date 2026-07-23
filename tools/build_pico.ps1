param(
    [string]$BuildDir = "build-pico",
    [ValidateSet("Debug", "Release", "RelWithDebInfo", "MinSizeRel")][string]$BuildType = "Release",
    [int]$WaitSeconds = 120,
    [switch]$Clean
)

$ErrorActionPreference = "Stop"

function Get-RpiRp2Drive {
    try {
        $volume = Get-Volume -FileSystemLabel "RPI-RP2" -ErrorAction Stop |
            Where-Object { $_.DriveLetter } |
            Select-Object -First 1

        if ($volume) {
            return "$($volume.DriveLetter):\\"
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

$repositoryRoot = (Resolve-Path -LiteralPath (Split-Path -Parent $PSScriptRoot)).Path
$platformSelector = Join-Path $repositoryRoot "tools\select_platform.ps1"
if (-not (Test-Path -LiteralPath $platformSelector)) {
    throw "Platform selector not found: $platformSelector"
}
& $platformSelector -Platform Pico

$commonScript = Join-Path $repositoryRoot "tools\pico_common.ps1"
if (-not (Test-Path -LiteralPath $commonScript)) {
    throw "Shared Pico build helpers not found: $commonScript"
}
. $commonScript

$buildContext = Get-PicoBuildContext -RepositoryRoot $repositoryRoot -BuildDir $BuildDir
Initialize-PicoBuildEnvironment -BuildContext $buildContext

if ($Clean) {
    Remove-ProjectBuildDirectory -RepositoryRoot $repositoryRoot -BuildPath $buildContext.BuildPath
}

Invoke-PicoBuild -RepositoryRoot $repositoryRoot -BuildContext $buildContext -BuildType $BuildType

$uf2Path = Join-Path $buildContext.BuildPath "Euzebia3D.uf2"
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
