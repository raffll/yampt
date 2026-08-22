param(
    [string]$Archive,
    [string]$ModId,
    [string]$ModFileId,
    [string]$Description
)

$repoRoot = Split-Path $PSScriptRoot -Parent
$version = & "$PSScriptRoot\get_version.ps1"
$baseUrl = "https://api.nexusmods.com/v3"
$gameDomain = "morrowind"
$modPageId = "44518"

if (!$Archive) {
    $Archive = Join-Path $repoRoot "build\yampt_$version.7z"
}

if (!(Test-Path $Archive)) {
    Write-Error "Archive not found: $Archive"
    exit 1
}

if (!$env:NEXUS_API_KEY) {
    Write-Error "NEXUS_API_KEY environment variable not set."
    Write-Host "Get your key from: https://www.nexusmods.com/settings/api-keys"
    exit 1
}

$headers = @{
    "apikey"       = $env:NEXUS_API_KEY
    "Content-Type" = "application/json"
}

# --- Resolve mod internal ID if not provided ---

if (!$ModId) {
    Write-Host "Resolving mod ID for $gameDomain/mods/$modPageId..."
    $modInfo = Invoke-RestMethod -Uri "$baseUrl/games/$gameDomain/mods/$modPageId" -Headers $headers
    $ModId = $modInfo.data.id
    Write-Host "  Mod ID: $ModId"
}

# --- Step 1: Create upload session ---

$fileInfo = Get-Item $Archive
$uploadBody = @{
    size_bytes = $fileInfo.Length
    filename   = $fileInfo.Name
} | ConvertTo-Json

Write-Host "Creating upload session ($($fileInfo.Length) bytes)..."
$uploadResponse = Invoke-RestMethod -Uri "$baseUrl/uploads" -Method Post -Headers $headers -Body $uploadBody
$uploadId = $uploadResponse.data.id
$presignedUrl = $uploadResponse.data.presigned_url
Write-Host "  Upload ID: $uploadId"

# --- Step 2: PUT file to presigned URL ---

Write-Host "Uploading file to S3..."
$fileBytes = [System.IO.File]::ReadAllBytes((Resolve-Path $Archive))
Invoke-RestMethod -Uri $presignedUrl -Method Put -Body $fileBytes -ContentType "application/octet-stream"
Write-Host "  Upload complete."

# --- Step 3: Finalise upload ---

Write-Host "Finalising upload..."
Invoke-RestMethod -Uri "$baseUrl/uploads/$uploadId/finalise" -Method Post -Headers $headers | Out-Null

# --- Step 4: Poll until available ---

$maxAttempts = 30
$attempt = 0
do {
    Start-Sleep -Seconds 2
    $attempt++
    $status = Invoke-RestMethod -Uri "$baseUrl/uploads/$uploadId" -Headers $headers
    $state = $status.data.state
    Write-Host "  Poll $attempt/$maxAttempts - state: $state"
} while ($state -ne "available" -and $attempt -lt $maxAttempts)

if ($state -ne "available") {
    Write-Error "Upload did not become available after $maxAttempts attempts."
    exit 1
}

# --- Step 5: Create mod file or new version ---

$changelogSection = ""
$changelogPath = Join-Path $repoRoot "CHANGELOG.md"
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
        $changelogSection = ($sectionLines -join "`n").Trim()
    }
}

if (!$Description) {
    $Description = $changelogSection
}
if (!$Description) {
    $Description = "Version $version"
}

if ($ModFileId) {
    Write-Host "Creating new version on mod file $ModFileId..."
    $versionBody = @{
        upload_id                    = $uploadId
        name                         = "yampt $version"
        version                      = $version
        file_category                = "main"
        description                  = $Description
        archive_existing_file        = $true
        primary_mod_manager_download = $true
        allow_mod_manager_download   = $true
    } | ConvertTo-Json -Depth 4

    $result = Invoke-RestMethod -Uri "$baseUrl/mod-files/$ModFileId/versions" `
        -Method Post -Headers $headers -Body $versionBody
    Write-Host "  Created version: $($result.data.version.id)"
}
else {
    Write-Host "Creating new mod file on mod $ModId..."
    $modFileBody = @{
        upload_id                    = $uploadId
        mod_id                       = $ModId
        name                         = "yampt $version"
        version                      = $version
        file_category                = "main"
        description                  = $Description
        primary_mod_manager_download = $true
        allow_mod_manager_download   = $true
    } | ConvertTo-Json -Depth 4

    $result = Invoke-RestMethod -Uri "$baseUrl/mod-files" -Method Post -Headers $headers -Body $modFileBody
    $newModFileId = $result.data.id
    Write-Host "  Created mod file: $newModFileId"
    Write-Host "  Save this ID for future updates: -ModFileId $newModFileId"
}

Write-Host "Done: https://www.nexusmods.com/$gameDomain/mods/$modPageId"
