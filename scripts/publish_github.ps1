param(
    [string]$Archive,
    [switch]$Draft
)

$repoRoot = Split-Path $PSScriptRoot -Parent
$version = & "$PSScriptRoot\get_version.ps1"
$repo = "raffll/yampt"
$tag = "v$version"

if (!$Archive) {
    $Archive = Join-Path $repoRoot "build\yampt_$version.7z"
}

if (!(Test-Path $Archive)) {
    Write-Error "Archive not found: $Archive"
    exit 1
}

if (!$env:GITHUB_TOKEN) {
    Write-Error "GITHUB_TOKEN environment variable not set."
    exit 1
}

$headers = @{
    "Authorization" = "token $env:GITHUB_TOKEN"
    "Accept"        = "application/vnd.github+json"
}

$commitSha = & git -C $repoRoot rev-parse HEAD

$changelogPath = Join-Path $repoRoot "CHANGELOG.md"
$releaseBody = "Release $version"
if (Test-Path $changelogPath) {
    $lines = Get-Content $changelogPath
    $sectionLines = @()
    $inSection = $false
    foreach ($line in $lines) {
        if ($line -match "^## \[$version\]") {
            $inSection = $true
            continue
        }
        if ($inSection -and $line -match "^## \[") {
            break
        }
        if ($inSection) {
            $sectionLines += $line
        }
    }
    if ($sectionLines.Count -gt 0) {
        $releaseBody = ($sectionLines -join "`n").Trim()
    }
}

$body = @{
    tag_name         = $tag
    target_commitish = $commitSha
    name             = "yampt $version"
    body             = $releaseBody
    draft            = [bool]$Draft
    prerelease       = $true
} | ConvertTo-Json -Depth 4

Write-Host "Creating GitHub release $tag..."
$release = Invoke-RestMethod -Uri "https://api.github.com/repos/$repo/releases" `
    -Method Post -Headers $headers -Body $body -ContentType "application/json"

$uploadUrl = $release.upload_url -replace '\{\?name,label\}', ''
$fileName = Split-Path $Archive -Leaf
$fileBytes = [System.IO.File]::ReadAllBytes((Resolve-Path $Archive))

Write-Host "Uploading $fileName..."
Invoke-RestMethod -Uri "$uploadUrl`?name=$fileName" `
    -Method Post `
    -Headers @{
        "Authorization" = "token $env:GITHUB_TOKEN"
        "Content-Type"  = "application/x-7z-compressed"
    } `
    -Body $fileBytes | Out-Null

Write-Host "Done: https://github.com/$repo/releases/tag/$tag"
