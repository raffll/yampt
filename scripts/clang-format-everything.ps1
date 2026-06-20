$clangFormat = "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Tools\Llvm\x64\bin\clang-format.exe"

if (-not (Test-Path $clangFormat)) {
    Write-Error "clang-format not found at: $clangFormat"
    exit 1
}

$files = Get-ChildItem -Recurse -Include *.cpp, *.hpp, *.c, *.h |
    Where-Object {
        $_.FullName -notmatch '\\external\\' -and
        $_.FullName -notmatch '\\vcpkg_installed\\' -and
        $_.FullName -notmatch '\\x64\\' -and
        $_.FullName -notmatch '\\.vs\\' -and
        $_.FullName -notmatch '\\.git\\'
    }

foreach ($f in $files) {
    Write-Host $f.FullName
    & $clangFormat -Werror -style=file -i $f.FullName
}
