$repoRoot = Split-Path $PSScriptRoot -Parent
$ct2Dir = Join-Path $repoRoot "external\CTranslate2"
$cmake = "C:\OMEN\Morrowind\vcpkg\downloads\tools\cmake-4.3.2-windows\cmake-4.3.2-windows-x86_64\bin\cmake.exe"

$buildDir = Join-Path $ct2Dir "build"
Remove-Item -Recurse -Force $buildDir -ErrorAction SilentlyContinue

& $cmake -S $ct2Dir -B $buildDir `
    -G "Visual Studio 18 2026" `
    -A x64 `
    -DWITH_MKL=OFF `
    -DWITH_CUDA=OFF `
    -DWITH_CUDNN=OFF `
    -DWITH_DNNL=OFF `
    -DWITH_OPENBLAS=OFF `
    -DWITH_RUY=ON `
    -DBUILD_CLI=OFF `
    -DBUILD_TESTS=OFF `
    -DOPENMP_RUNTIME=NONE `
    "-DCMAKE_POLICY_VERSION_MINIMUM=3.5"

if ($LASTEXITCODE -eq 0) {
    & $cmake --build $buildDir --config Release
}
