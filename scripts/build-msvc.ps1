[CmdletBinding()]
param(
    [ValidateSet("Debug", "Release", "RelWithDebInfo", "MinSizeRel")]
    [string] $Configuration = "Release",

    [ValidateSet("Auto", "Ninja", "NMake")]
    [string] $Generator = "Auto",

    [switch] $Vulkan,
    [switch] $FullSDL,
    [switch] $Install,
    [switch] $Package,
    [switch] $SkipTests,
    [switch] $Fresh,

    [string] $BuildDirectory = "",
    [string] $InstallDirectory = ""
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

function Get-RequiredCommand {
    param(
        [Parameter(Mandatory = $true)]
        [string] $Name,

        [Parameter(Mandatory = $true)]
        [string] $Description
    )

    $command = Get-Command $Name -ErrorAction SilentlyContinue
    if ($null -eq $command) {
        throw "$Description ('$Name') was not found in PATH. Initialize your standalone MSVC/Windows SDK environment, then try again."
    }
    return $command.Source
}

function Invoke-NativeCommand {
    param(
        [Parameter(Mandatory = $true)]
        [string] $Executable,

        [Parameter(Mandatory = $true)]
        [string[]] $Arguments
    )

    & $Executable @Arguments
    if ($LASTEXITCODE -ne 0) {
        throw "Command failed with exit code $LASTEXITCODE`: $Executable $($Arguments -join ' ')"
    }
}

function ConvertTo-CMakePath {
    param(
        [Parameter(Mandatory = $true)]
        [string] $Path
    )

    # CMake stores compiler paths inside generated .cmake files, where a raw
    # Windows path such as C:\msvc\... would be parsed as escape sequences.
    # Forward slashes are native to CMake and work correctly with Windows tools.
    return ([System.IO.Path]::GetFullPath($Path)).Replace([char] 92, [char] 47)
}

function Get-CMakeVersion {
    param(
        [Parameter(Mandatory = $true)]
        [string] $CMakeExecutable
    )

    $firstLine = (& $CMakeExecutable --version | Select-Object -First 1)
    if ($LASTEXITCODE -ne 0 -or $firstLine -notmatch "cmake version ([0-9]+\.[0-9]+\.[0-9]+)") {
        throw "Could not determine the installed CMake version."
    }
    return [version] $Matches[1]
}

function Assert-RequiredFile {
    param(
        [Parameter(Mandatory = $true)]
        [string] $Path,

        [Parameter(Mandatory = $true)]
        [string] $Description
    )

    if (-not (Test-Path -LiteralPath $Path -PathType Leaf)) {
        throw "$Description is missing: $Path`nRe-extract the complete Coffee source archive and try again."
    }
}

$projectRoot = Split-Path -Parent $PSScriptRoot
Assert-RequiredFile -Path (Join-Path $projectRoot "deps/glad/include/glad/gl.h") -Description "Bundled GLAD header"
Assert-RequiredFile -Path (Join-Path $projectRoot "deps/glad/include/KHR/khrplatform.h") -Description "Bundled Khronos platform header"
Assert-RequiredFile -Path (Join-Path $projectRoot "deps/glad/src/gl.c") -Description "Bundled GLAD implementation"

$cmake = Get-RequiredCommand -Name "cmake" -Description "CMake"
$ctest = Get-RequiredCommand -Name "ctest" -Description "CTest"
$cl = Get-RequiredCommand -Name "cl" -Description "MSVC C/C++ compiler"
$link = Get-RequiredCommand -Name "link" -Description "MSVC linker"
$rc = Get-RequiredCommand -Name "rc" -Description "Windows SDK resource compiler"
$mt = Get-RequiredCommand -Name "mt" -Description "Windows SDK manifest tool"
$cmakeVersion = Get-CMakeVersion -CMakeExecutable $cmake

$cmakeProjectRoot = ConvertTo-CMakePath -Path $projectRoot
$cmakeCl = ConvertTo-CMakePath -Path $cl
$cmakeRc = ConvertTo-CMakePath -Path $rc
$cmakeMt = ConvertTo-CMakePath -Path $mt

if ([string]::IsNullOrWhiteSpace($env:INCLUDE)) {
    throw "The INCLUDE environment variable is empty. Add the MSVC and Windows SDK include directories."
}
if ([string]::IsNullOrWhiteSpace($env:LIB)) {
    throw "The LIB environment variable is empty. Add the MSVC and Windows SDK library directories."
}

$selectedGenerator = $Generator
if ($selectedGenerator -eq "Auto") {
    if ($null -ne (Get-Command "ninja" -ErrorAction SilentlyContinue)) {
        $selectedGenerator = "Ninja"
    }
    elseif ($null -ne (Get-Command "nmake" -ErrorAction SilentlyContinue)) {
        $selectedGenerator = "NMake"
    }
    else {
        throw "Neither Ninja nor NMake was found. Install Ninja or place nmake.exe in PATH."
    }
}

if ($selectedGenerator -eq "Ninja") {
    Get-RequiredCommand -Name "ninja" -Description "Ninja build tool" | Out-Null
    $cmakeGenerator = "Ninja"
    $generatorSlug = "ninja"
}
else {
    Get-RequiredCommand -Name "nmake" -Description "NMake build tool" | Out-Null
    $cmakeGenerator = "NMake Makefiles"
    $generatorSlug = "nmake"
}

$configurationSlug = $Configuration.ToLowerInvariant()
if ([string]::IsNullOrWhiteSpace($BuildDirectory)) {
    $BuildDirectory = Join-Path $projectRoot "build/standalone-msvc-$generatorSlug-$configurationSlug"
}
if ([string]::IsNullOrWhiteSpace($InstallDirectory)) {
    $InstallDirectory = Join-Path $projectRoot "dist/Coffee"
}

$vulkanValue = if ($Vulkan) { "ON" } else { "OFF" }
$leanSdlValue = if ($FullSDL) { "OFF" } else { "ON" }
$cmakeBuildDirectory = ConvertTo-CMakePath -Path $BuildDirectory
$cmakeInstallDirectory = ConvertTo-CMakePath -Path $InstallDirectory
$cmakePackageDirectory = ConvertTo-CMakePath -Path (Join-Path $projectRoot "dist/packages")
$configureStamp = Join-Path $BuildDirectory ".coffee-configured"
$configureArguments = @()

# A successful configure writes a small stamp. Until it exists, CMake 3.24+
# discards partial metadata left by an interrupted or failed configure. Normal
# repeat builds retain their cache and stay fast. -Fresh forces the same repair.
if ($cmakeVersion -ge [version] "3.24.0" -and
    ($Fresh -or -not (Test-Path -LiteralPath $configureStamp -PathType Leaf))) {
    $configureArguments += "--fresh"
}

$configureArguments += @(
    "-S", $cmakeProjectRoot,
    "-B", $cmakeBuildDirectory,
    "-G", $cmakeGenerator,
    "-DCMAKE_C_COMPILER:FILEPATH=$cmakeCl",
    "-DCMAKE_CXX_COMPILER:FILEPATH=$cmakeCl",
    "-DCMAKE_RC_COMPILER:FILEPATH=$cmakeRc",
    "-DCMAKE_MT:FILEPATH=$cmakeMt",
    "-DCMAKE_BUILD_TYPE=$Configuration",
    "-DCOFFEE_BUILD_EXAMPLES=ON",
    "-DCOFFEE_BUILD_TESTS=ON",
    "-DCOFFEE_BUILD_VULKAN_MODULE=$vulkanValue",
    "-DCOFFEE_LEAN_SDL=$leanSdlValue"
)

Write-Host "Configuring Coffee with $cmakeGenerator and $Configuration MSVC..."
Invoke-NativeCommand -Executable $cmake -Arguments $configureArguments
[System.IO.File]::WriteAllText($configureStamp, "Coffee $Configuration $cmakeGenerator configured successfully.`n")

$buildArguments = @("--build", $cmakeBuildDirectory)
if ($selectedGenerator -eq "Ninja") {
    $buildArguments += "--parallel"
}

Write-Host "Building Coffee..."
Invoke-NativeCommand -Executable $cmake -Arguments $buildArguments

if (-not $SkipTests) {
    Write-Host "Running Coffee tests..."
    Invoke-NativeCommand -Executable $ctest -Arguments @(
        "--test-dir", $cmakeBuildDirectory,
        "--output-on-failure"
    )
}

if ($Install) {
    Write-Host "Installing the Coffee SDK to $InstallDirectory..."
    Invoke-NativeCommand -Executable $cmake -Arguments @(
        "--install", $cmakeBuildDirectory,
        "--prefix", $cmakeInstallDirectory
    )
}

if ($Package) {
    $cpack = Get-RequiredCommand -Name "cpack" -Description "CPack"
    Write-Host "Creating the Coffee SDK package..."
    Invoke-NativeCommand -Executable $cpack -Arguments @(
        "--config", "$cmakeBuildDirectory/CPackConfig.cmake",
        "-G", "ZIP",
        "-C", $Configuration,
        "-B", $cmakePackageDirectory
    )
}

Write-Host ""
Write-Host "Coffee build completed."
Write-Host "  Compiler:  $cl"
Write-Host "  Linker:    $link"
Write-Host "  RC tool:   $rc"
Write-Host "  MT tool:   $mt"
Write-Host "  Generator: $cmakeGenerator"
Write-Host "  SDL mode:  $(if ($FullSDL) { 'Full' } else { 'Lean' })"
Write-Host "  Output:    $(Join-Path $BuildDirectory 'bin')"
if ($Install) {
    Write-Host "  SDK:       $InstallDirectory"
}
