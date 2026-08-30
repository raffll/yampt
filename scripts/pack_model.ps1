$repoRoot = Split-Path $PSScriptRoot -Parent
$src = Join-Path $repoRoot "models\nllb-600M"
$buildDir = Join-Path $repoRoot "build"
$version = "1.0"
$out = Join-Path $buildDir "nllb-600M-v$version.zip"

New-Item -ItemType Directory -Path $buildDir -Force | Out-Null
if (Test-Path $out) { Remove-Item $out }

$tmp = Join-Path $repoRoot "build\pack_tmp\models\nllb-600M"
$tmpRoot = Join-Path $repoRoot "build\pack_tmp"
if (Test-Path $tmpRoot) { Remove-Item -Recurse -Force $tmpRoot }

New-Item -ItemType Directory -Path "$tmp\model" -Force | Out-Null

Copy-Item (Join-Path $src "sentencepiece.bpe.model") "$tmp\"
Copy-Item (Join-Path $src "model\model.bin") "$tmp\model\"
Copy-Item (Join-Path $src "model\config.json") "$tmp\model\"
Copy-Item (Join-Path $src "model\shared_vocabulary.json") "$tmp\model\"

$license = @"
Creative Commons Attribution-NonCommercial 4.0 International (CC BY-NC 4.0)

https://creativecommons.org/licenses/by-nc/4.0/

You are free to:
- Share: copy and redistribute the material in any medium or format
- Adapt: remix, transform, and build upon the material

Under the following terms:
- Attribution: You must give appropriate credit, provide a link to the license,
  and indicate if changes were made.
- NonCommercial: You may not use the material for commercial purposes.
"@

$readme = @"
# NLLB-200 Distilled 600M (CTranslate2 int8)

This is an int8-quantized CTranslate2 conversion of Meta's NLLB-200 model.

Source: facebook/nllb-200-distilled-600M
https://huggingface.co/facebook/nllb-200-distilled-600M

Converted using CTranslate2 TransformersConverter with int8 quantization.
Supports translation between 200 languages.

License: CC-BY-NC-4.0 (same as the original model)
"@

Set-Content -Path "$tmp\LICENSE" -Value $license -Encoding UTF8
Set-Content -Path "$tmp\README.md" -Value $readme -Encoding UTF8

Compress-Archive -Path (Join-Path $repoRoot "build\pack_tmp\models") -DestinationPath $out -Force

Remove-Item -Recurse -Force $tmpRoot

Write-Host "Created $out ($('{0:N1}' -f ((Get-Item $out).Length / 1MB)) MB)"
