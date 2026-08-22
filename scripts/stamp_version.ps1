$repoRoot = Split-Path $PSScriptRoot -Parent
$version = & "$PSScriptRoot\get_version.ps1"

$vcpkgPath = Join-Path $repoRoot "vcpkg.json"
$content = Get-Content $vcpkgPath -Raw
$updated = $content -replace '"version":\s*"[^"]*"', "`"version`": `"$version`""
[System.IO.File]::WriteAllText($vcpkgPath, $updated, (New-Object System.Text.UTF8Encoding $false))

$versionFile = Join-Path $repoRoot "VERSION"
[System.IO.File]::WriteAllText($versionFile, $version, (New-Object System.Text.UTF8Encoding $false))

$changelogPath = Join-Path $repoRoot "CHANGELOG.md"
if (Test-Path $changelogPath) {
    $clContent = Get-Content $changelogPath -Raw
    $today = Get-Date -Format "yyyy-MM-dd"
    $header = "## [$version] - $today"
    $clUpdated = $clContent -replace '## \[xxx\]', $header
    if ($clUpdated -eq $clContent) {
        $pattern = [regex]'## \[\d+\.\d+\]( - \d{4}-\d{2}-\d{2})?'
        $clUpdated = $pattern.Replace($clContent, $header, 1)
    }
    if ($clUpdated -ne $clContent) {
        [System.IO.File]::WriteAllText($changelogPath, $clUpdated, (New-Object System.Text.UTF8Encoding $false))
    }
}

Write-Host "Stamped version $version into vcpkg.json and VERSION"
