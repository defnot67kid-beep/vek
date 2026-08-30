param(
    [Parameter(Mandatory = $true)][string]$SourceRoot,
    [Parameter(Mandatory = $true)][string]$MirrorRoot
)

$ErrorActionPreference = 'Stop'

$source = [System.IO.Path]::GetFullPath($SourceRoot).TrimEnd([System.IO.Path]::DirectorySeparatorChar, [System.IO.Path]::AltDirectorySeparatorChar)
$mirror = [System.IO.Path]::GetFullPath($MirrorRoot).TrimEnd([System.IO.Path]::DirectorySeparatorChar, [System.IO.Path]::AltDirectorySeparatorChar)

if (-not (Test-Path -LiteralPath $source -PathType Container)) {
    throw "Source root does not exist: $source"
}

New-Item -ItemType Directory -Force -Path $mirror | Out-Null

# Generated/private folders never belong in the persistent canonical source mirror.
$excludedDirectoryNames = @(
    '.git', '.vs', 'build', 'build-dev', 'out', 'developer_keys',
    '__pycache__', '.idea', '.vscode'
)

$excludedFileExtensions = @('.tmp', '.bak', '.old', '.orig', '.rej')

function Test-ExcludedRelativePath([string]$relativePath) {
    $parts = $relativePath -split '[\\/]'
    foreach ($part in $parts) {
        if ($excludedDirectoryNames -contains $part) { return $true }
        if ($part -like 'cmake-build-*') { return $true }
    }

    $leaf = [System.IO.Path]::GetFileName($relativePath)
    $extension = [System.IO.Path]::GetExtension($leaf).ToLowerInvariant()
    if ($excludedFileExtensions -contains $extension) { return $true }
    if ($leaf.EndsWith('~')) { return $true }
    return $false
}

function Get-RelativePathCompat([string]$basePath, [string]$fullPath) {
    $baseUri = New-Object System.Uri(($basePath.TrimEnd([System.IO.Path]::DirectorySeparatorChar, [System.IO.Path]::AltDirectorySeparatorChar) + [System.IO.Path]::DirectorySeparatorChar))
    $fileUri = New-Object System.Uri($fullPath)
    $relativeUri = $baseUri.MakeRelativeUri($fileUri)
    return [System.Uri]::UnescapeDataString($relativeUri.ToString()).Replace([char]'/', [System.IO.Path]::DirectorySeparatorChar)
}

function Get-Sha256([string]$path) {
    return (Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash
}

$sourceFiles = Get-ChildItem -LiteralPath $source -Recurse -File | Where-Object {
    $rel = Get-RelativePathCompat $source $_.FullName
    -not (Test-ExcludedRelativePath $rel)
}

$wanted = New-Object 'System.Collections.Generic.HashSet[string]' ([System.StringComparer]::OrdinalIgnoreCase)
$added = 0
$updated = 0
$unchanged = 0

foreach ($file in $sourceFiles) {
    $relative = Get-RelativePathCompat $source $file.FullName
    [void]$wanted.Add($relative)
    $destination = Join-Path $mirror $relative
    $destinationDirectory = Split-Path -Parent $destination
    if (-not (Test-Path -LiteralPath $destinationDirectory)) {
        New-Item -ItemType Directory -Force -Path $destinationDirectory | Out-Null
    }

    $needsCopy = $true
    $alreadyExists = Test-Path -LiteralPath $destination -PathType Leaf
    if ($alreadyExists) {
        $sourceLength = $file.Length
        $destinationLength = (Get-Item -LiteralPath $destination).Length
        if ($sourceLength -eq $destinationLength) {
            $needsCopy = ((Get-Sha256 $file.FullName) -ne (Get-Sha256 $destination))
        }
    }

    if ($needsCopy) {
        [System.IO.File]::Copy($file.FullName, $destination, $true)
        # New ZIPs may preserve an old timestamp. Touch changed mirror files so
        # MSBuild/CMake definitely sees changed content as newer than old objects.
        [System.IO.File]::SetLastWriteTimeUtc($destination, [DateTime]::UtcNow)
        if ($alreadyExists) { $updated++ } else { $added++ }
        Write-Host "  changed: $relative"
    } else {
        $unchanged++
    }
}

$deleted = 0
$mirrorFiles = Get-ChildItem -LiteralPath $mirror -Recurse -File
foreach ($file in $mirrorFiles) {
    $relative = Get-RelativePathCompat $mirror $file.FullName
    if (-not $wanted.Contains($relative)) {
        Remove-Item -LiteralPath $file.FullName -Force
        $deleted++
        Write-Host "  removed: $relative"
    }
}

# Remove empty directories left by deleted/renamed source files.
Get-ChildItem -LiteralPath $mirror -Recurse -Directory |
    Sort-Object FullName -Descending |
    ForEach-Object {
        if (-not (Get-ChildItem -LiteralPath $_.FullName -Force | Select-Object -First 1)) {
            Remove-Item -LiteralPath $_.FullName -Force
        }
    }

Write-Host ""
Write-Host "Smart source sync complete."
Write-Host "  Added:     $added"
Write-Host "  Updated:   $updated"
Write-Host "  Removed:   $deleted"
Write-Host "  Reused:    $unchanged"
Write-Host "  Mirror:    $mirror"
