# Builds the DiskLED release packages in dist\ : the x64 and x86 programs, a zip of each (portable use) and, when Inno Setup 6 is
# installed, a setup program of each. Nothing is published, tagged or pushed.
#
#   powershell -ExecutionPolicy Bypass -File installer\Build-Release.ps1 -Version 3.1

param (
	[string]$Version = '3.1',
	[string]$PlatformToolset = 'v143'
)

$ErrorActionPreference = 'Stop'

$root = Split-Path $PSScriptRoot -Parent
$dist = Join-Path $root 'dist'

# MSBuild from Visual Studio
$vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
$msbuild = & $vswhere -latest -requires Microsoft.Component.MSBuild -find 'MSBuild\**\Bin\MSBuild.exe' | Select-Object -First 1
if (-not $msbuild) { throw 'MSBuild (Visual Studio 2022 with the C++ workload and MFC) was not found.' }

# Inno Setup is optional
$iscc = @(
	(Join-Path ${env:ProgramFiles(x86)} 'Inno Setup 6\ISCC.exe'),
	(Join-Path $env:ProgramFiles 'Inno Setup 6\ISCC.exe'),
	(Join-Path $env:LOCALAPPDATA 'Programs\Inno Setup 6\ISCC.exe')
) | Where-Object { Test-Path $_ } | Select-Object -First 1
if (-not $iscc) { Write-Warning 'Inno Setup 6 was not found: only the zip files are made.' }

New-Item -ItemType Directory -Force $dist | Out-Null

foreach ($platform in @(@{ Platform = 'x64'; Name = 'x64' }, @{ Platform = 'Win32'; Name = 'x86' })) {
	$name  = $platform.Name
	$stage = Join-Path $dist "DiskLED-$Version-$name"
	$zip   = Join-Path $dist "DiskLED-$Version-$name.zip"

	Write-Host "== Building $name"
	if (Test-Path $stage) { Remove-Item $stage -Recurse -Force }
	& $msbuild (Join-Path $root 'DiskLED.vcxproj') /t:Rebuild /p:Configuration=Release /p:Platform=$($platform.Platform) /p:PlatformToolset=$PlatformToolset `
		"/p:OutDir=$stage\" "/p:IntDir=$dist\obj-$name\" /v:minimal /nologo
	if ($LASTEXITCODE -ne 0) { throw "The $name build failed." }

	# The program only, plus what goes into the zip. The portable marker is not part of the setup, which takes the program from here.
	Get-ChildItem $stage -File | Where-Object { $_.Name -ne 'DiskLED.exe' } | Remove-Item
	Copy-Item (Join-Path $root 'LICENSE') (Join-Path $stage 'LICENSE.txt')
	Copy-Item (Join-Path $root 'Additional Files\Release Notes.txt') $stage
	Set-Content -Path (Join-Path $stage 'portable') -Encoding ASCII -Value 'DiskLED portable mode: while this file is next to DiskLED.exe, the settings are kept in DiskLED.ini next to the program. Delete this file to keep them in %AppData%\DiskLED instead.'

	Write-Host "== Zip $zip"
	if (Test-Path $zip) { Remove-Item $zip }
	Compress-Archive -Path (Join-Path $stage '*') -DestinationPath $zip

	if ($iscc) {
		Write-Host "== Setup $name"
		& $iscc "/DVersion=$Version" "/DArch=$name" (Join-Path $PSScriptRoot 'DiskLED.iss')
		if ($LASTEXITCODE -ne 0) { throw "The $name setup could not be made." }
	}
}

Write-Host ''
Get-ChildItem $dist -File | Where-Object { $_.Extension -in '.zip', '.exe' } | Select-Object Name, @{ Name = 'MB'; Expression = { [math]::Round($_.Length / 1MB, 2) } }
