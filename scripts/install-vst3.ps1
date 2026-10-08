param(
    [ValidateSet('debug', 'release')][string]$Preset = 'release',
    [string]$InstallPath = (Join-Path $env:CommonProgramFiles 'VST3')
)

$ErrorActionPreference = 'Stop'
$repoRoot = Split-Path $PSScriptRoot -Parent
$configuration = if ($Preset -eq 'debug') { 'Debug' } else { 'Release' }
$bundlePath = Join-Path $repoRoot "build\$Preset\SoundImagine_artefacts\$configuration\VST3\SoundImagine.vst3"

if (!(Test-Path -LiteralPath $bundlePath -PathType Container)) {
    throw "VST3 bundle was not found: $bundlePath`nBuild it first with .\scripts\build.ps1 -Preset $Preset."
}

$installPath = [System.IO.Path]::GetFullPath($InstallPath)
$destination = Join-Path $installPath 'SoundImagine.vst3'
if (!(Test-Path -LiteralPath $installPath -PathType Container)) {
    New-Item -ItemType Directory -Path $installPath -Force | Out-Null
}

try {
    # Copy each file to its exact relative destination. Copying the bundle itself
    # into an existing directory would nest SoundImagine.vst3 inside itself.
    foreach ($file in Get-ChildItem -LiteralPath $bundlePath -File -Recurse) {
        $relativePath = $file.FullName.Substring($bundlePath.Length + 1)
        $targetFile = Join-Path $destination $relativePath
        $targetDirectory = Split-Path $targetFile -Parent
        if (!(Test-Path -LiteralPath $targetDirectory -PathType Container)) {
            New-Item -ItemType Directory -Path $targetDirectory -Force | Out-Null
        }
        Copy-Item -LiteralPath $file.FullName -Destination $targetFile -Force
    }
} catch [System.UnauthorizedAccessException] {
    throw "Access denied while installing to '$installPath'. Run PowerShell as Administrator, then retry."
} catch [System.IO.IOException] {
    if ($_.Exception.HResult -eq -2147024891) {
        throw "Access denied while installing to '$installPath'. Run PowerShell as Administrator, then retry."
    }
    throw
}

Write-Host "Installed SoundImagine VST3 to: $destination"

