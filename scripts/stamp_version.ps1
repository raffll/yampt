$repoRoot = Split-Path $PSScriptRoot -Parent
$version = & "$PSScriptRoot\get_version.ps1"

$vcpkgPath = Join-Path $repoRoot "vcpkg.json"
$content = Get-Content $vcpkgPath -Raw
$updated = $content -replace '"version":\s*"[^"]*"', "`"version`": `"$version`""
[System.IO.File]::WriteAllText($vcpkgPath, $updated, (New-Object System.Text.UTF8Encoding $false))

$versionFile = Join-Path $repoRoot "VERSION"
[System.IO.File]::WriteAllText($versionFile, $version, (New-Object System.Text.UTF8Encoding $false))

Write-Host "Stamped version $version into vcpkg.json and VERSION"
