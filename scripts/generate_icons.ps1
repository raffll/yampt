# Converts SVG icons in resources/ to .ico files for Windows builds.
# Requires Inkscape or magick (ImageMagick) in PATH.
# Run from repo root: .\scripts\generate_icons.ps1

$ErrorActionPreference = "Stop"
$resourceDir = "$PSScriptRoot\..\resources"

$icons = @(
    @{ svg = "yampt-translator.svg"; ico = "yampt-translator.ico" },
    @{ svg = "yampt-editor.svg"; ico = "yampt-editor.ico" }
)

$sizes = @(16, 24, 32, 48, 64, 128, 256)

function Find-Converter {
    if (Get-Command "magick" -ErrorAction SilentlyContinue) { return "magick" }
    if (Get-Command "inkscape" -ErrorAction SilentlyContinue) { return "inkscape" }
    return $null
}

$converter = Find-Converter
if (-not $converter) {
    Write-Error "Neither ImageMagick (magick) nor Inkscape found in PATH."
    exit 1
}

foreach ($icon in $icons) {
    $svgPath = Join-Path $resourceDir $icon.svg
    $icoPath = Join-Path $resourceDir $icon.ico

    if (-not (Test-Path $svgPath)) {
        Write-Warning "SVG not found: $svgPath"
        continue
    }

    $pngFiles = @()

    foreach ($size in $sizes) {
        $pngFile = Join-Path $resourceDir "tmp_${size}.png"
        $pngFiles += $pngFile

        if ($converter -eq "magick") {
            & magick -background none -density 384 $svgPath -resize "${size}x${size}" $pngFile
        } else {
            & inkscape $svgPath --export-type=png --export-filename=$pngFile -w $size -h $size 2>$null
        }
    }

    if ($converter -eq "magick") {
        & magick @pngFiles $icoPath
    } else {
        & magick @pngFiles $icoPath
    }

    foreach ($png in $pngFiles) {
        Remove-Item $png -ErrorAction SilentlyContinue
    }

    Write-Host "Generated: $icoPath"
}
