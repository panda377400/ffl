param(
	[string]$SourceRoot = $env:MGBA_SOURCE_ROOT,
	[string]$DestinationRoot = (Join-Path $PSScriptRoot 'upstream')
)

$ErrorActionPreference = 'Stop'

if ([string]::IsNullOrWhiteSpace($SourceRoot)) {
	throw 'SourceRoot not set. Pass -SourceRoot or define MGBA_SOURCE_ROOT.'
}

if (-not (Test-Path $SourceRoot)) {
	throw "SourceRoot not found: $SourceRoot"
}

New-Item -ItemType Directory -Force -Path $DestinationRoot | Out-Null
New-Item -ItemType Directory -Force -Path (Join-Path $DestinationRoot 'include') | Out-Null
New-Item -ItemType Directory -Force -Path (Join-Path $DestinationRoot 'src') | Out-Null
New-Item -ItemType Directory -Force -Path (Join-Path $DestinationRoot 'src\\platform') | Out-Null
New-Item -ItemType Directory -Force -Path (Join-Path $DestinationRoot 'src\\third-party') | Out-Null

Copy-Item -Path (Join-Path $SourceRoot 'LICENSE') -Destination $DestinationRoot -Force

Copy-Item -Path (Join-Path $SourceRoot 'include\\mgba') -Destination (Join-Path $DestinationRoot 'include') -Recurse -Force
Copy-Item -Path (Join-Path $SourceRoot 'include\\mgba-util') -Destination (Join-Path $DestinationRoot 'include') -Recurse -Force

Copy-Item -Path (Join-Path $SourceRoot 'src\\arm') -Destination (Join-Path $DestinationRoot 'src') -Recurse -Force
Copy-Item -Path (Join-Path $SourceRoot 'src\\core') -Destination (Join-Path $DestinationRoot 'src') -Recurse -Force
Copy-Item -Path (Join-Path $SourceRoot 'src\\gb') -Destination (Join-Path $DestinationRoot 'src') -Recurse -Force
Copy-Item -Path (Join-Path $SourceRoot 'src\\gba') -Destination (Join-Path $DestinationRoot 'src') -Recurse -Force
Copy-Item -Path (Join-Path $SourceRoot 'src\\sm83') -Destination (Join-Path $DestinationRoot 'src') -Recurse -Force
Copy-Item -Path (Join-Path $SourceRoot 'src\\util') -Destination (Join-Path $DestinationRoot 'src') -Recurse -Force
Copy-Item -Path (Join-Path $SourceRoot 'src\\platform\\windows') -Destination (Join-Path $DestinationRoot 'src\\platform') -Recurse -Force
Copy-Item -Path (Join-Path $SourceRoot 'src\\third-party\\blip_buf') -Destination (Join-Path $DestinationRoot 'src\\third-party') -Recurse -Force
Copy-Item -Path (Join-Path $SourceRoot 'src\\third-party\\inih') -Destination (Join-Path $DestinationRoot 'src\\third-party') -Recurse -Force

$cleanupTargets = @(
	'arm\\debugger',
	'core\\test',
	'gba\\debugger',
	'gba\\test',
	'util\\gui',
	'util\\test',
	'platform\\windows\\setup',
	'arm\\CMakeLists.txt',
	'core\\CMakeLists.txt',
	'core\\flags.h.in',
	'core\\version.c.in',
	'gba\\CMakeLists.txt',
	'gba\\hle-bios.make',
	'util\\CMakeLists.txt'
)

foreach ($target in $cleanupTargets) {
	$fullPath = Join-Path (Join-Path $DestinationRoot 'src') $target
	if (Test-Path $fullPath) {
		Remove-Item -LiteralPath $fullPath -Recurse -Force
	}
}

$generator = Join-Path (Split-Path -Parent $MyInvocation.MyCommand.Path) 'generate_wrappers.ps1'
& $generator -Root (Split-Path -Parent $MyInvocation.MyCommand.Path)

Write-Host "mGBA embedded subtree synced to $DestinationRoot"
