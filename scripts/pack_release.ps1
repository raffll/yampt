$outDir = "x64\Release"
$buildNum = (git rev-list --count HEAD)
$buildDir = "build"
$packDir = "$buildDir\yampt_build$buildNum"
$zipName = "$buildDir\yampt_build$buildNum.7z"
$sevenZip = "$PSScriptRoot\7za.exe"

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
Copy-Item "$outDir\7za.exe" $packDir
Copy-Item "$outDir\*.dll" $packDir

if (Test-Path "$outDir\dictionaries") {
    Copy-Item -Recurse "$outDir\dictionaries" "$packDir\dictionaries"
}

if (Test-Path "$outDir\platforms") {
    New-Item -ItemType Directory -Force "$packDir\platforms" | Out-Null
    Copy-Item "$outDir\platforms\*.dll" "$packDir\platforms"
}

& $sevenZip a -t7z -mx=9 $zipName "$packDir\*"

Write-Host "Created $zipName (build $buildNum)"
