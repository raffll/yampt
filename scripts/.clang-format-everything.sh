#!/bin/bash

CLANG_FORMAT_PATH="/c/Program Files/Microsoft Visual Studio/18/Community/VC/Tools/Llvm/x64/bin/clang-format.exe"

if [[ -f "$CLANG_FORMAT_PATH" ]]; then
    find . -regex '.*\.\(cpp\|hpp\|c\|h\)' \
        -not -path "./external/*" \
        -not -path "./vcpkg_installed/*" \
        -not -path "./x64/*" \
        -not -path "./.vs/*" \
        -not -path "./.git/*" \
        -print \
        -exec "$CLANG_FORMAT_PATH" -Werror -style=file -i {} \;
else
    echo "clang-format not found at: $CLANG_FORMAT_PATH"
    exit 1
fi
