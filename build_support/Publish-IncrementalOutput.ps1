param(
    [Parameter(Mandatory = $true)][string]$BuildOutput,
    [Parameter(Mandatory = $true)][string]$LocalOutput
)

$ErrorActionPreference = 'Stop'

$source = [System.IO.Path]::GetFullPath($BuildOutput).TrimEnd([System.IO.Path]::DirectorySeparatorChar, [System.IO.Path]::AltDirectorySeparatorChar)
$destinationRoot = [System.IO.Path]::GetFullPath($LocalOutput).TrimEnd([System.IO.Path]::DirectorySeparatorChar, [System.IO.Path]::AltDirectorySeparatorChar)

if (-not (Test-Path -LiteralPath $source -PathType Container)) {
    throw "Build output does not exist: $source"
}

New-Item -ItemType Directory -Force -Path $destinationRoot | Out-Null

function Get-RelativePathCompat([string]$basePath, [string]$fullPath) {
    $baseUri = New-Object System.Uri(($basePath.TrimEnd([System.IO.Path]::DirectorySeparatorChar, [System.IO.Path]::AltDirectorySeparatorChar) + [System.IO.Path]::DirectorySeparatorChar))
    $fileUri = New-Object System.Uri($fullPath)
    $relativeUri = $baseUri.MakeRelativeUri($fileUri)
    return [System.Uri]::UnescapeDataString($relativeUri.ToString()).Replace([char]'/', [System.IO.Path]::DirectorySeparatorChar)
}

function Test-PreservedUserPath([string]$relativePath) {
    $normalized = $relativePath.Replace([char]'/', [System.IO.Path]::DirectorySeparatorChar)
    if ($normalized -ieq 'crashlogs.txt') { return $true }
    if ($normalized -ieq 'saves') { return $true }
    if ($normalized.StartsWith('saves' + [System.IO.Path]::DirectorySeparatorChar, [System.StringComparison]::OrdinalIgnoreCase)) { return $true }
    return $false
}

$manifestPath = Join-Path $destinationRoot '.vek-build-output-manifest.txt'

# v26.8.3 migration cleanup: old xcopy/copy_directory builds could have stale
# scripts/assets that predate this manifest. Purge these generated runtime trees
# once on every publish; they are cheap to recopy and never contain user saves.
foreach ($runtimeDirectory in @('scripts', 'assets')) {
    $runtimePath = Join-Path $destinationRoot $runtimeDirectory
    if (Test-Path -LiteralPath $runtimePath) {
        Remove-Item -LiteralPath $runtimePath -Recurse -Force
        Write-Host "  purged stale runtime tree: $runtimeDirectory"
    }
}

# Remove files that an earlier smart publish owned but the new build no longer
# emits. User-generated crash logs and saves are never owned by this manifest.
$oldOwned = @()
if (Test-Path -LiteralPath $manifestPath -PathType Leaf) {
    $oldOwned = Get-Content -LiteralPath $manifestPath | Where-Object { $_ -and -not (Test-PreservedUserPath $_) }
}

$currentFiles = Get-ChildItem -LiteralPath $source -Recurse -File
$currentOwned = New-Object 'System.Collections.Generic.HashSet[string]' ([System.StringComparer]::OrdinalIgnoreCase)
foreach ($file in $currentFiles) {
    $relative = Get-RelativePathCompat $source $file.FullName
    if (-not (Test-PreservedUserPath $relative)) {
        [void]$currentOwned.Add($relative)
    }
}

$removed = 0
foreach ($relative in $oldOwned) {
    if (-not $currentOwned.Contains($relative)) {
        $stalePath = Join-Path $destinationRoot $relative
        if (Test-Path -LiteralPath $stalePath -PathType Leaf) {
            Remove-Item -LiteralPath $stalePath -Force
            $removed++
            Write-Host "  removed stale published file: $relative"
        }
    }
}

$copied = 0
foreach ($file in $currentFiles) {
    $relative = Get-RelativePathCompat $source $file.FullName
    if (Test-PreservedUserPath $relative) { continue }

    $destination = Join-Path $destinationRoot $relative
    $parent = Split-Path -Parent $destination
    if (-not (Test-Path -LiteralPath $parent)) {
        New-Item -ItemType Directory -Force -Path $parent | Out-Null
    }
    [System.IO.File]::Copy($file.FullName, $destination, $true)
    $copied++
}

# Remove empty generated directories, but never touch the user's saves folder.
Get-ChildItem -LiteralPath $destinationRoot -Recurse -Directory |
    Sort-Object FullName -Descending |
    ForEach-Object {
        $relative = Get-RelativePathCompat $destinationRoot $_.FullName
        if (-not (Test-PreservedUserPath $relative)) {
            if (-not (Get-ChildItem -LiteralPath $_.FullName -Force | Select-Object -First 1)) {
                Remove-Item -LiteralPath $_.FullName -Force
            }
        }
    }

$currentOwned | Sort-Object | Set-Content -LiteralPath $manifestPath -Encoding UTF8

Write-Host ""
Write-Host "Authoritative local publish complete."
Write-Host "  Copied/updated: $copied"
Write-Host "  Removed stale:  $removed"
Write-Host "  Preserved:      saves/, crashlogs.txt"
Write-Host "  Output:         $destinationRoot"
