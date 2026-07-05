$ErrorActionPreference = "Stop"

$yampt = "x64\Release\yampt.exe"
$master = "..\master"
$testDir = "tests"
$converterDir = "$master\converter"

if (-not (Test-Path $yampt)) {
    Write-Host "[error] yampt.exe not found at $yampt" -ForegroundColor Red
    exit 1
}

if (-not (Test-Path $master)) {
    Write-Host "[error] master directory not found at $master" -ForegroundColor Red
    exit 1
}

# cleanup
if (Test-Path $testDir) {
    Remove-Item -Recurse -Force $testDir
}
New-Item -ItemType Directory -Path "$testDir\en" -Force | Out-Null
New-Item -ItemType Directory -Path "$testDir\pl" -Force | Out-Null
New-Item -ItemType Directory -Path "$testDir\de" -Force | Out-Null
New-Item -ItemType Directory -Path "$testDir\fr" -Force | Out-Null
New-Item -ItemType Directory -Path "$testDir\converter" -Force | Out-Null

Write-Host "=== make EN, no base dict ===" -ForegroundColor Cyan
foreach ($name in @("Morrowind", "Tribunal", "Bloodmoon")) {
    Write-Host "  $name..."
    & $yampt --make -f "$master\en\$name.esm" -o "$testDir\en\${name}_en.json"
    if ($LASTEXITCODE -ne 0) { Write-Host "  FAILED" -ForegroundColor Red; exit 1 }
}

Write-Host "=== make-base EN to PL ===" -ForegroundColor Cyan
foreach ($name in @("Morrowind", "Tribunal", "Bloodmoon")) {
    Write-Host "  $name..."
    & $yampt --make-base -f "$master\pl\$name.esm" "$master\en\$name.esm"
    if ($LASTEXITCODE -ne 0) { Write-Host "  FAILED" -ForegroundColor Red; exit 1 }
    # CLI outputs {native_name}.BASE.json in cwd
    Move-Item -Force "$name.BASE.json" "$testDir\pl\${name}_en_pl.json"
}

Write-Host "=== make-base EN to DE ===" -ForegroundColor Cyan
foreach ($name in @("Morrowind", "Tribunal", "Bloodmoon")) {
    Write-Host "  $name..."
    & $yampt --make-base -f "$master\de\$name.esm" "$master\en\$name.esm"
    if ($LASTEXITCODE -ne 0) { Write-Host "  FAILED" -ForegroundColor Red; exit 1 }
    Move-Item -Force "$name.BASE.json" "$testDir\de\${name}_en_de.json"
}

Write-Host "=== make-base EN to FR ===" -ForegroundColor Cyan
foreach ($name in @("Morrowind", "Tribunal", "Bloodmoon")) {
    Write-Host "  $name..."
    & $yampt --make-base -f "$master\fr\$name.esm" "$master\en\$name.esm"
    if ($LASTEXITCODE -ne 0) { Write-Host "  FAILED" -ForegroundColor Red; exit 1 }
    Move-Item -Force "$name.BASE.json" "$testDir\fr\${name}_en_fr.json"
}

Write-Host "=== merge EN to PL ===" -ForegroundColor Cyan
& $yampt --merge -d "$testDir\pl\Morrowind_en_pl.json" "$testDir\pl\Tribunal_en_pl.json" "$testDir\pl\Bloodmoon_en_pl.json" -o "$testDir\pl\Merged_en_pl.json"
if ($LASTEXITCODE -ne 0) { Write-Host "  FAILED" -ForegroundColor Red; exit 1 }

Write-Host "=== make EN with base dict ===" -ForegroundColor Cyan
& $yampt --make -f "$master\en\Morrowind.esm" -d "$testDir\pl\Morrowind_en_pl.json" -o "$testDir\en\Morrowind_en_with_base.json"
if ($LASTEXITCODE -ne 0) { Write-Host "  FAILED" -ForegroundColor Red; exit 1 }

Write-Host "=== convert all ESPs with PL base dict ===" -ForegroundColor Cyan
$esps = Get-ChildItem $converterDir -File | Where-Object { $_.Extension -match "^\.(esp|omwaddon)$" }
foreach ($esp in $esps) {
    Write-Host "  $($esp.Name)..."
    & $yampt --convert -f $esp.FullName -d "$testDir\pl\Morrowind_en_pl.json" "$testDir\pl\Tribunal_en_pl.json" "$testDir\pl\Bloodmoon_en_pl.json"
    if ($LASTEXITCODE -ne 0) { Write-Host "  FAILED" -ForegroundColor Red; exit 1 }
    # CLI outputs {name}{ext} in cwd (no suffix = same filename as input)
    $outName = $esp.Name
    if (Test-Path $outName) {
        Move-Item -Force $outName "$testDir\converted\$outName"
    }
}

# cleanup yampt log files from cwd
Get-ChildItem -Filter "yampt_*.log" | Remove-Item -Force

Write-Host ""
Write-Host "=== ALL DONE ===" -ForegroundColor Green
Write-Host "Output in $testDir\"
