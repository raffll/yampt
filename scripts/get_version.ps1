$repoRoot = Split-Path $PSScriptRoot -Parent

try {
    $count = & git -C $repoRoot rev-list --count HEAD 2>$null
    if ($LASTEXITCODE -eq 0 -and $count) {
        Write-Output "0.$count"
        exit 0
    }
} catch {}

$versionFile = Join-Path $repoRoot "VERSION"
if (Test-Path $versionFile) {
    $content = (Get-Content $versionFile -Raw).Trim()
    if ($content) {
        Write-Output $content
        exit 0
    }
}

Write-Output "0.0"
