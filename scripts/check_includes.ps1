# Checks that project include boundaries are respected.
# yampt.core must not include Qt or other yampt projects.
# yampt.qt must not include yampt.editor or yampt.translator.
# Run from repo root: .\scripts\check_includes.ps1

$errors = @()

# yampt.core: no Qt, no other projects
$coreFiles = Get-ChildItem -Path "yampt.core\source" -Recurse -Include "*.hpp","*.cpp"
foreach ($file in $coreFiles) {
    $lines = Get-Content $file.FullName
    for ($i = 0; $i -lt $lines.Count; $i++) {
        $line = $lines[$i]
        if ($line -match '^\s*#include\s*[<"]Q') {
            $errors += "$($file.FullName):$($i+1): Qt include in yampt.core: $line"
        }
        if ($line -match '^\s*#include.*yampt\.(qt|editor|translator)') {
            $errors += "$($file.FullName):$($i+1): cross-project include in yampt.core: $line"
        }
        if ($line -match '^\s*#include\s*[<"](app_settings|theme_system|conflict_types)\.hpp') {
            $errors += "$($file.FullName):$($i+1): yampt.qt header in yampt.core: $line"
        }
    }
}

# yampt.qt: no yampt.editor, no yampt.translator
$qtFiles = Get-ChildItem -Path "yampt.qt\source" -Recurse -Include "*.hpp","*.cpp"
foreach ($file in $qtFiles) {
    $lines = Get-Content $file.FullName
    for ($i = 0; $i -lt $lines.Count; $i++) {
        $line = $lines[$i]
        if ($line -match '^\s*#include.*yampt\.(editor|translator)') {
            $errors += "$($file.FullName):$($i+1): cross-project include in yampt.qt: $line"
        }
    }
}

# yampt.cli: no Qt
$cliFiles = Get-ChildItem -Path "yampt.cli\source" -Recurse -Include "*.hpp","*.cpp"
foreach ($file in $cliFiles) {
    $lines = Get-Content $file.FullName
    for ($i = 0; $i -lt $lines.Count; $i++) {
        $line = $lines[$i]
        if ($line -match '^\s*#include\s*[<"]Q') {
            $errors += "$($file.FullName):$($i+1): Qt include in yampt.cli: $line"
        }
    }
}

# Report
if ($errors.Count -eq 0) {
    Write-Host "OK: No include boundary violations found." -ForegroundColor Green
    exit 0
} else {
    Write-Host "FAIL: $($errors.Count) violation(s) found:" -ForegroundColor Red
    foreach ($e in $errors) {
        Write-Host "  $e" -ForegroundColor Yellow
    }
    exit 1
}
