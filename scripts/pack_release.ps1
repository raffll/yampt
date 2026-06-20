$outDir = "x64\Release"
$buildNum = (git rev-list --count HEAD)
$buildDir = "build"
$packDir = "$buildDir\yampt_build$buildNum"
$zipName = "$buildDir\yampt_build$buildNum.zip"

if (!(Test-Path $buildDir)) { New-Item -ItemType Directory -Force $buildDir | Out-Null }
if (Test-Path $packDir) { Remove-Item -Recurse -Force $packDir }
if (Test-Path $zipName) { Remove-Item -Force $zipName }

New-Item -ItemType Directory -Force $packDir | Out-Null

Copy-Item "$outDir\yampt.exe" $packDir
Copy-Item "$outDir\yampt.translator.exe" $packDir
Copy-Item "$outDir\yampt.editor.exe" $packDir
Copy-Item "$outDir\*.dll" $packDir

if (Test-Path "$outDir\dictionaries") {
    Copy-Item -Recurse "$outDir\dictionaries" "$packDir\dictionaries"
}

if (Test-Path "$outDir\platforms") {
    New-Item -ItemType Directory -Force "$packDir\platforms" | Out-Null
    Copy-Item "$outDir\platforms\*.dll" "$packDir\platforms"
}

Compress-Archive -Path "$packDir\*" -DestinationPath $zipName -Force

Write-Host "Created $zipName (build $buildNum)"
