#!/bin/bash

CLANG_FORMAT_PATH="C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\Llvm\x64\bin\clang-format.exe"

if [[ -f "$CLANG_FORMAT_PATH" ]]; then
    find . -regex '.*\.\(cpp\|hpp\|c\|h\)' \
	-not -path "./external/*" \
	-not -path "./vcpkg_installed/*" \
	-not -path "./Binaries/*" \
	-not -path "./Temporary/*" \
	-print \
	-exec "$CLANG_FORMAT_PATH" -Werror -style=file -i {} \;
fi
