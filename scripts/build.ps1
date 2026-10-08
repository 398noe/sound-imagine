param([ValidateSet('debug', 'release')][string]$Preset = 'debug', [switch]$Fresh)
$ErrorActionPreference = 'Stop'
$repoRoot = Split-Path $PSScriptRoot -Parent
$vswherePath = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
if (!(Test-Path -LiteralPath $vswherePath)) { throw 'Install MSVC Build Tools with Desktop development with C++.' }
$vsRoot = & $vswherePath -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if (!$vsRoot) { throw 'MSVC x64 tools were not found.' }
$devCmdPath = Join-Path $vsRoot 'Common7\Tools\VsDevCmd.bat'
$environmentLines = & $env:ComSpec /d /s /c "`"$devCmdPath`" -arch=x64 -host_arch=x64 >nul && set"
if ($LASTEXITCODE -ne 0) { throw 'VsDevCmd failed.' }
foreach ($line in $environmentLines) {
    if ($line -match '^([^=]+)=(.*)$') { [Environment]::SetEnvironmentVariable($Matches[1], $Matches[2], 'Process') }
}
$cmakeRoot = Join-Path $vsRoot 'Common7\IDE\CommonExtensions\Microsoft\CMake'
$env:Path = "$(Join-Path $cmakeRoot 'CMake\bin');$(Join-Path $cmakeRoot 'Ninja');$env:Path"
$env:VSLANG = '1033'
Push-Location $repoRoot
try {
    if ($Fresh) { & cmake --fresh --preset $Preset }
    else { & cmake --preset $Preset }
    if ($LASTEXITCODE -ne 0) { throw 'CMake configure failed.' }
    & cmake --build --preset $Preset
    if ($LASTEXITCODE -ne 0) { throw 'Build failed.' }
    & ctest --preset $Preset
    if ($LASTEXITCODE -ne 0) { throw 'Tests failed.' }
} finally { Pop-Location }
