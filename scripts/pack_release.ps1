$repoRoot = Split-Path $PSScriptRoot -Parent
$outDir = Join-Path $repoRoot "x64\Release"
$version = & "$PSScriptRoot\get_version.ps1"
$buildDir = Join-Path $repoRoot "build"
$packDir = Join-Path $buildDir "yampt_$version"
$zipName = Join-Path $buildDir "yampt_$version.7z"
$sevenZip = Join-Path $repoRoot "external\7za.exe"

if (!(Test-Path $sevenZip)) {
    Write-Error "7za.exe not found at: $sevenZip"
    exit 1
}

& "$PSScriptRoot\stamp_version.ps1"

if (!(Test-Path $buildDir)) { New-Item -ItemType Directory -Force $buildDir | Out-Null }
if (Test-Path $packDir) { Remove-Item -Recurse -Force $packDir }
if (Test-Path $zipName) { Remove-Item -Force $zipName }

New-Item -ItemType Directory -Force $packDir | Out-Null

Copy-Item (Join-Path $outDir "yampt.exe") $packDir
Copy-Item (Join-Path $outDir "yTranslator.exe") $packDir
Copy-Item (Join-Path $outDir "yEditor.exe") $packDir
Copy-Item (Join-Path $outDir "*.dll") $packDir

$dictSrc = Join-Path $outDir "dictionaries"
if (Test-Path $dictSrc) {
    Copy-Item -Recurse $dictSrc (Join-Path $packDir "dictionaries")
}

$platSrc = Join-Path $outDir "platforms"
if (Test-Path $platSrc) {
    $platDst = Join-Path $packDir "platforms"
    New-Item -ItemType Directory -Force $platDst | Out-Null
    Copy-Item (Join-Path $platSrc "*.dll") $platDst
}

$provSrc = Join-Path $outDir "providers"
if (Test-Path $provSrc) {
    Copy-Item -Recurse $provSrc (Join-Path $packDir "providers")
}

$langSrc = Join-Path $outDir "languages.json"
if (Test-Path $langSrc) {
    Copy-Item $langSrc $packDir
}

$docsDst = Join-Path $packDir "docs"
New-Item -ItemType Directory -Force $docsDst | Out-Null
Copy-Item (Join-Path $repoRoot "README.md") $docsDst
Copy-Item (Join-Path $repoRoot "CHANGELOG.md") $docsDst
$docsSrc = Join-Path $repoRoot "docs"
if (Test-Path $docsSrc) {
    Copy-Item (Join-Path $docsSrc "*.md") $docsDst
}

Push-Location $buildDir
& $sevenZip a -t7z -mx=9 $zipName "yampt_$version"
Pop-Location

Write-Host "Created $zipName (version $version)"
