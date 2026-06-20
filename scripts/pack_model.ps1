$src = "models\nllb-600M"
$out = "nllb-600M.zip"

if (Test-Path $out) { Remove-Item $out }

$tmp = "pack_tmp\nllb-600M"
if (Test-Path "pack_tmp") { Remove-Item -Recurse -Force "pack_tmp" }

New-Item -ItemType Directory -Path "$tmp\model" -Force | Out-Null

Copy-Item "$src\sentencepiece.bpe.model" "$tmp\"
Copy-Item "$src\model\model.bin" "$tmp\model\"
Copy-Item "$src\model\config.json" "$tmp\model\"
Copy-Item "$src\model\shared_vocabulary.json" "$tmp\model\"

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

Compress-Archive -Path "pack_tmp\nllb-600M" -DestinationPath $out -Force

Remove-Item -Recurse -Force "pack_tmp"

Write-Host "Created $out ($('{0:N1}' -f ((Get-Item $out).Length / 1MB)) MB)"
