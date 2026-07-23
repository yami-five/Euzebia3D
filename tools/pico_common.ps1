function Get-CMakeSetValue {
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

function Resolve-RequiredPath {
    param(
        [Parameter(Mandatory = $true)][string]$Path,
        [Parameter(Mandatory = $true)][string]$Name
    )

    if (-not (Test-Path -LiteralPath $Path)) {
        throw "$Name not found: $Path"
    }

    return (Resolve-Path -LiteralPath $Path).Path
}

function Resolve-ProjectBuildPath {
    param(
        [Parameter(Mandatory = $true)][string]$RepositoryRoot,
        [Parameter(Mandatory = $true)][string]$BuildDir
    )

    $buildPath = if ([System.IO.Path]::IsPathRooted($BuildDir)) {
        [System.IO.Path]::GetFullPath($BuildDir)
    }
    else {
        [System.IO.Path]::GetFullPath((Join-Path $RepositoryRoot $BuildDir))
    }

    $normalizedRoot = $RepositoryRoot.TrimEnd('\')
    if (-not $buildPath.StartsWith("$normalizedRoot\", [System.StringComparison]::OrdinalIgnoreCase)) {
        throw "Build directory must be inside the repository: $buildPath"
    }

    return $buildPath
}

function Remove-ProjectBuildDirectory {
    param(
        [Parameter(Mandatory = $true)][string]$RepositoryRoot,
        [Parameter(Mandatory = $true)][string]$BuildPath
    )

    if (-not (Test-Path -LiteralPath $BuildPath)) {
        return
    }

    $resolvedBuildPath = (Resolve-Path -LiteralPath $BuildPath).Path
    $normalizedRoot = $RepositoryRoot.TrimEnd('\')
    if (-not $resolvedBuildPath.StartsWith("$normalizedRoot\", [System.StringComparison]::OrdinalIgnoreCase)) {
        throw "Refusing to remove build directory outside the repository: $resolvedBuildPath"
    }

    Remove-Item -LiteralPath $resolvedBuildPath -Recurse -Force -ErrorAction Stop
}

function Get-PicoBuildContext {
    param(
        [Parameter(Mandatory = $true)][string]$RepositoryRoot,
        [Parameter(Mandatory = $true)][string]$BuildDir
    )

    $cmakeListsPath = Join-Path $RepositoryRoot "CMakeLists.txt"
    $cmakeLists = Get-Content -LiteralPath $cmakeListsPath -Raw
    $sdkVersion = Get-CMakeSetValue -Content $cmakeLists -Name "sdkVersion"
    $toolchainVersion = Get-CMakeSetValue -Content $cmakeLists -Name "toolchainVersion"
    $picoBoard = Get-CMakeSetValue -Content $cmakeLists -Name "PICO_BOARD"

    if (-not $sdkVersion) {
        throw "Could not read sdkVersion from CMakeLists.txt."
    }
    if (-not $toolchainVersion) {
        throw "Could not read toolchainVersion from CMakeLists.txt."
    }

    $picoHomeDirectory = $env:USERPROFILE
    if (-not $picoHomeDirectory) {
        $picoHomeDirectory = $env:HOME
    }
    if (-not $picoHomeDirectory) {
        throw "Could not resolve user home directory."
    }

    $picoSdkRoot = Join-Path $picoHomeDirectory ".pico-sdk"
    $sdkPath = Resolve-RequiredPath -Path (Join-Path $picoSdkRoot "sdk\$sdkVersion") -Name "Pico SDK $sdkVersion"
    $toolchainPath = Resolve-RequiredPath -Path (Join-Path $picoSdkRoot "toolchain\$toolchainVersion") -Name "Pico toolchain $toolchainVersion"
    $cmakePath = Resolve-RequiredPath -Path (Join-Path $picoSdkRoot "cmake\v4.3.4\bin\cmake.exe") -Name "CMake"
    $ninjaPath = Resolve-RequiredPath -Path (Join-Path $picoSdkRoot "ninja\v1.13.2\ninja.exe") -Name "Ninja"

    return [PSCustomObject]@{
        Board = $picoBoard
        BuildPath = Resolve-ProjectBuildPath -RepositoryRoot $RepositoryRoot -BuildDir $BuildDir
        CmakePath = $cmakePath
        NinjaPath = $ninjaPath
        SdkPath = $sdkPath
        ToolchainPath = $toolchainPath
    }
}

function Initialize-PicoBuildEnvironment {
    param([Parameter(Mandatory = $true)]$BuildContext)

    $env:PICO_SDK_PATH = $BuildContext.SdkPath
    $env:PICO_TOOLCHAIN_PATH = $BuildContext.ToolchainPath
    $env:Path = @(
        (Join-Path $BuildContext.ToolchainPath "bin")
        (Split-Path -Parent $BuildContext.CmakePath)
        (Split-Path -Parent $BuildContext.NinjaPath)
        $env:Path
    ) -join ";"
}

function Invoke-PicoBuild {
    param(
        [Parameter(Mandatory = $true)][string]$RepositoryRoot,
        [Parameter(Mandatory = $true)]$BuildContext,
        [Parameter(Mandatory = $true)][ValidateSet("Debug", "Release", "RelWithDebInfo", "MinSizeRel")][string]$BuildType
    )

    Write-Host "Configuring CMake for $BuildType..."
    $configureArgs = @(
        "-S", $RepositoryRoot,
        "-B", $BuildContext.BuildPath,
        "-G", "Ninja",
        "-DCMAKE_BUILD_TYPE=$BuildType"
    )
    if ($BuildContext.Board) {
        $configureArgs += "-DPICO_BOARD=$($BuildContext.Board)"
    }

    & $BuildContext.CmakePath @configureArgs
    if ($LASTEXITCODE -ne 0) {
        throw "CMake configure failed with exit code $LASTEXITCODE."
    }

    Write-Host "Building Pico firmware..."
    & $BuildContext.CmakePath "--build" $BuildContext.BuildPath
    if ($LASTEXITCODE -ne 0) {
        throw "Build failed with exit code $LASTEXITCODE."
    }
}
