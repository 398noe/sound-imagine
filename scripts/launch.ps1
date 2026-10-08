param([ValidateSet('debug', 'release')][string]$Preset = 'release')
$ErrorActionPreference = 'Stop'
$configuration = if ($Preset -eq 'debug') { 'Debug' } else { 'Release' }
$repoRoot = Split-Path $PSScriptRoot -Parent
$executable = Join-Path $repoRoot "build\$Preset\SoundImagine_artefacts\$configuration\Standalone\SoundImagine.exe"
if (!(Test-Path -LiteralPath $executable -PathType Leaf)) { throw "Build first: .\scripts\build.ps1 -Preset $Preset" }
Start-Process -FilePath $executable -WindowStyle Hidden
