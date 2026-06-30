param(
    [switch]$Upload
)

$outDir = "x64\Release"
$buildNum = (git rev-list --count HEAD)
$buildDir = "build"
$packDir = "$buildDir\yampt_build$buildNum"
$zipName = "$buildDir\yampt_build$buildNum.7z"
$sevenZip = "$PSScriptRoot\7za.exe"
$repo = "raffll/yampt"
$tag = "build$buildNum"

if (!(Test-Path $sevenZip)) {
    Write-Error "7z.exe not found at: $sevenZip"
    exit 1
}

if (!(Test-Path $buildDir)) { New-Item -ItemType Directory -Force $buildDir | Out-Null }
if (Test-Path $packDir) { Remove-Item -Recurse -Force $packDir }
if (Test-Path $zipName) { Remove-Item -Force $zipName }

New-Item -ItemType Directory -Force $packDir | Out-Null

Copy-Item "$outDir\yampt.exe" $packDir
Copy-Item "$outDir\yTranslator.exe" $packDir
Copy-Item "$outDir\yEditor.exe" $packDir
Copy-Item "$outDir\*.dll" $packDir

if (Test-Path "$outDir\dictionaries") {
    Copy-Item -Recurse "$outDir\dictionaries" "$packDir\dictionaries"
}

if (Test-Path "$outDir\platforms") {
    New-Item -ItemType Directory -Force "$packDir\platforms" | Out-Null
    Copy-Item "$outDir\platforms\*.dll" "$packDir\platforms"
}

$zipFullPath = Join-Path (Resolve-Path $buildDir).Path "yampt_build$buildNum.7z"
Push-Location $buildDir
& $sevenZip a -t7z -mx=9 $zipFullPath "yampt_build$buildNum"
Pop-Location

Write-Host "Created $zipName (build $buildNum)"

if (!$Upload) {
    exit 0
}

# --- GitHub Release Upload ---

if (!$env:GITHUB_TOKEN) {
    Write-Error "GITHUB_TOKEN environment variable not set. Cannot upload."
    exit 1
}

$headers = @{
    "Authorization" = "token $env:GITHUB_TOKEN"
    "Accept"        = "application/vnd.github+json"
}

$commitSha = (git rev-parse HEAD)
$body = @{
    tag_name         = $tag
    target_commitish = $commitSha
    name             = "Build $buildNum"
    body             = "Automated release build $buildNum"
    draft            = $false
    prerelease       = $true
} | ConvertTo-Json

Write-Host "Creating release $tag..."
$release = Invoke-RestMethod -Uri "https://api.github.com/repos/$repo/releases" `
    -Method Post -Headers $headers -Body $body -ContentType "application/json"

$uploadUrl = $release.upload_url -replace '\{\?name,label\}', ''
$fileName = Split-Path $zipName -Leaf

Write-Host "Uploading $fileName..."
$fileBytes = [System.IO.File]::ReadAllBytes((Resolve-Path $zipName))
Invoke-RestMethod -Uri "$uploadUrl`?name=$fileName" `
    -Method Post `
    -Headers @{
        "Authorization" = "token $env:GITHUB_TOKEN"
        "Content-Type"  = "application/x-7z-compressed"
    } `
    -Body $fileBytes | Out-Null

Write-Host "Uploaded to https://github.com/$repo/releases/tag/$tag"
