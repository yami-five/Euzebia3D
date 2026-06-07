param(
    [string]$BuildDir = "build-clean-windows",
    [string]$Config = "Release",
    [string]$Generator = "",
    [string]$CMakeExe = "",
    [string]$VcpkgRoot = "",
    [string]$VcpkgTriplet = "x64-windows",
    [string]$CMakeToolchainFile = "",
    [string]$Sdl3Dir = ""
)

$ErrorActionPreference = "Stop"

$repoRoot = Split-Path -Parent $PSScriptRoot
$buildPath = Join-Path $repoRoot $BuildDir
$resolvedRoot = (Resolve-Path $repoRoot).Path

function Get-CMakeVersion {
    param([string]$PathToCMake)
    $firstLine = (& $PathToCMake --version 2>$null | Select-Object -First 1)
    if ($LASTEXITCODE -ne 0 -or [string]::IsNullOrWhiteSpace($firstLine)) {
        return $null
    }
    if ($firstLine -match 'cmake version ([0-9]+\.[0-9]+\.[0-9]+)') {
        return [version]$Matches[1]
    }
    return $null
}

function Remove-DirectoryWithRetry {
    param(
        [string]$PathToRemove,
        [int]$MaxAttempts = 5,
        [int]$DelayMs = 500
    )

    for ($attempt = 1; $attempt -le $MaxAttempts; $attempt++) {
        try {
            Remove-Item -LiteralPath $PathToRemove -Recurse -Force -ErrorAction Stop
            return
        }
        catch {
            if ($attempt -eq $MaxAttempts) {
                $blocking = Get-Process cmake,cmake-gui,ninja,msbuild,devenv -ErrorAction SilentlyContinue |
                    Select-Object -ExpandProperty ProcessName -Unique

                if ($blocking) {
                    $blockingNames = ($blocking | Sort-Object) -join ", "
                    throw "Cannot remove build directory: $PathToRemove. It is likely locked by: $blockingNames. Close those processes and run again."
                }

                throw "Cannot remove build directory: $PathToRemove. Files are locked by another process."
            }

            Start-Sleep -Milliseconds $DelayMs
        }
    }
}

function Resolve-OptionalPath {
    param([string]$PathToResolve)

    if ([string]::IsNullOrWhiteSpace($PathToResolve)) {
        return ""
    }

    if (-not (Test-Path $PathToResolve)) {
        throw "Path not found: $PathToResolve"
    }

    return (Resolve-Path $PathToResolve).Path
}

function Resolve-VcpkgToolchain {
    param(
        [string]$ExplicitToolchain,
        [string]$ExplicitRoot,
        [string]$RepositoryRoot
    )

    if (-not [string]::IsNullOrWhiteSpace($ExplicitToolchain)) {
        return Resolve-OptionalPath $ExplicitToolchain
    }

    $vcpkgRoots = New-Object System.Collections.Generic.List[string]
    if (-not [string]::IsNullOrWhiteSpace($ExplicitRoot)) {
        $vcpkgRoots.Add($ExplicitRoot)
    }
    if (-not [string]::IsNullOrWhiteSpace($env:VCPKG_ROOT)) {
        $vcpkgRoots.Add($env:VCPKG_ROOT)
    }
    $vcpkgRoots.Add((Join-Path $RepositoryRoot "..\vcpkg"))
    $vcpkgRoots.Add("C:\Repos\vcpkg")
    $vcpkgRoots.Add("C:\vcpkg")

    $seen = New-Object System.Collections.Generic.HashSet[string]([System.StringComparer]::OrdinalIgnoreCase)
    foreach ($root in $vcpkgRoots) {
        if ([string]::IsNullOrWhiteSpace($root)) {
            continue
        }

        $toolchain = Join-Path $root "scripts\buildsystems\vcpkg.cmake"
        if ((Test-Path $toolchain) -and $seen.Add((Resolve-Path $toolchain).Path)) {
            return (Resolve-Path $toolchain).Path
        }
    }

    return ""
}

$cmakeExeResolved = $null
$cmakeVersion = $null

if (-not [string]::IsNullOrWhiteSpace($CMakeExe)) {
    if (-not (Test-Path $CMakeExe)) {
        throw "CMake executable not found: $CMakeExe"
    }
    $cmakeExeResolved = (Resolve-Path $CMakeExe).Path
    $cmakeVersion = Get-CMakeVersion $cmakeExeResolved
    if ($null -eq $cmakeVersion) {
        throw "Could not read CMake version from: $cmakeExeResolved"
    }
}
else {
    $candidates = New-Object System.Collections.Generic.List[string]
    $fromWhere = & where.exe cmake 2>$null
    if ($LASTEXITCODE -eq 0 -and $fromWhere) {
        foreach ($entry in $fromWhere) {
            if (-not [string]::IsNullOrWhiteSpace($entry) -and (Test-Path $entry)) {
                $candidates.Add((Resolve-Path $entry).Path)
            }
        }
    }

    $fallbacks = @(
        (Join-Path $env:ProgramFiles "CMake\bin\cmake.exe"),
        (Join-Path ${env:ProgramFiles(x86)} "CMake\bin\cmake.exe")
    )
    foreach ($fb in $fallbacks) {
        if (-not [string]::IsNullOrWhiteSpace($fb) -and (Test-Path $fb)) {
            $candidates.Add((Resolve-Path $fb).Path)
        }
    }

    $unique = New-Object System.Collections.Generic.HashSet[string]([System.StringComparer]::OrdinalIgnoreCase)
    $ranked = @()
    foreach ($candidate in $candidates) {
        if ($unique.Add($candidate)) {
            $ver = Get-CMakeVersion $candidate
            if ($null -ne $ver) {
                $ranked += [PSCustomObject]@{
                    Path = $candidate
                    Version = $ver
                }
            }
        }
    }

    if ($ranked.Count -eq 0) {
        throw "No working cmake.exe found. Install CMake or pass -CMakeExe with a full path."
    }

    $selected = $ranked | Sort-Object Version -Descending | Select-Object -First 1
    $cmakeExeResolved = $selected.Path
    $cmakeVersion = $selected.Version
}

Write-Host ("Using CMake: {0} (v{1})" -f $cmakeExeResolved, $cmakeVersion)
$cmakeHelp = (& $cmakeExeResolved --help) -join "`n"
$vcpkgToolchain = Resolve-VcpkgToolchain -ExplicitToolchain $CMakeToolchainFile -ExplicitRoot $VcpkgRoot -RepositoryRoot $repoRoot
$sdl3DirResolved = Resolve-OptionalPath $Sdl3Dir

if (-not [string]::IsNullOrWhiteSpace($vcpkgToolchain)) {
    Write-Host ("Using vcpkg toolchain: {0}" -f $vcpkgToolchain)
    Write-Host ("Using vcpkg triplet: {0}" -f $VcpkgTriplet)
}
elseif ([string]::IsNullOrWhiteSpace($sdl3DirResolved)) {
    Write-Host "No vcpkg toolchain auto-detected. If SDL3 is not on CMAKE_PREFIX_PATH, pass -VcpkgRoot, -CMakeToolchainFile, or -Sdl3Dir."
}

if (-not [string]::IsNullOrWhiteSpace($sdl3DirResolved)) {
    Write-Host ("Using SDL3_DIR: {0}" -f $sdl3DirResolved)
}

if (Test-Path $buildPath) {
    $resolvedBuild = (Resolve-Path $buildPath).Path
    if (-not $resolvedBuild.StartsWith($resolvedRoot, [System.StringComparison]::OrdinalIgnoreCase)) {
        throw "Refusing to delete build outside repository: $resolvedBuild"
    }

    Remove-DirectoryWithRetry -PathToRemove $resolvedBuild
}

if (-not [string]::IsNullOrWhiteSpace($Generator)) {
    if ($cmakeHelp -notmatch [regex]::Escape($Generator)) {
        throw "Generator '$Generator' is not available in this CMake. Check 'cmake --help' or upgrade CMake."
    }
}
else {
    $cmakeHasVs18 = $cmakeHelp -match 'Visual Studio 18 2026'
    $cmakeHasVs17 = $cmakeHelp -match 'Visual Studio 17 2022'

    $vswhere = Join-Path ${env:ProgramFiles(x86)} "Microsoft Visual Studio\Installer\vswhere.exe"
    $hasVs18 = $false
    $hasVs17 = $false

    if (Test-Path $vswhere) {
        $vs18 = (& $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -version "[18.0,19.0)" -property installationPath 2>$null) | Select-Object -First 1
        $hasVs18 = -not [string]::IsNullOrWhiteSpace($vs18)
        $vs17 = (& $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -version "[17.0,18.0)" -property installationPath 2>$null) | Select-Object -First 1
        $hasVs17 = -not [string]::IsNullOrWhiteSpace($vs17)
    }

    if ($hasVs18 -and $cmakeHasVs18) {
        $Generator = "Visual Studio 18 2026"
    }
    elseif ($hasVs17 -and $cmakeHasVs17) {
        $Generator = "Visual Studio 17 2022"
    }
    elseif ($hasVs18 -and -not $cmakeHasVs18) {
        throw "Visual Studio 2026 is installed, but this CMake does not support generator 'Visual Studio 18 2026'. Upgrade CMake (4.2+) or install/use Visual Studio 2022."
    }
    elseif ($hasVs17 -and -not $cmakeHasVs17) {
        throw "Visual Studio 2022 is installed, but this CMake does not support generator 'Visual Studio 17 2022'. Upgrade CMake."
    }
    else {
        throw "No supported Visual Studio instance with C++ tools was found. Install Visual Studio 2022/2026 with 'Desktop development with C++', or run this script with -Generator and a working toolchain."
    }
}

$configureArgs = @(
    "-S", $repoRoot,
    "-B", $buildPath,
    "-DEUZEBIA3D_PLATFORM=WINDOWS"
)

if (-not [string]::IsNullOrWhiteSpace($vcpkgToolchain)) {
    $configureArgs += @(
        "-DCMAKE_TOOLCHAIN_FILE=$vcpkgToolchain",
        "-DVCPKG_TARGET_TRIPLET=$VcpkgTriplet"
    )
}

if (-not [string]::IsNullOrWhiteSpace($sdl3DirResolved)) {
    $configureArgs += "-DSDL3_DIR=$sdl3DirResolved"
}

if (-not [string]::IsNullOrWhiteSpace($Generator)) {
    $configureArgs += @("-G", $Generator, "-A", "x64")
}

& $cmakeExeResolved @configureArgs
if ($LASTEXITCODE -ne 0) {
    exit $LASTEXITCODE
}

$buildArgs = @(
    "--build", $buildPath,
    "--config", $Config
)

& $cmakeExeResolved @buildArgs
exit $LASTEXITCODE
