#!/bin/bash
set -e

cd "$(dirname "$0")/.."

REQUIRED_MAJOR=22

VERSION=$(clang-format --version | grep -oP '\d+' | head -1)

if [ "$VERSION" != "$REQUIRED_MAJOR" ]; then
    echo "Error: clang-format major version $REQUIRED_MAJOR required (found $VERSION)"
    echo "Install clang-format $REQUIRED_MAJOR to ensure consistent formatting across systems."
    exit 1
fi

find yampt.core/source yampt.qt/source yampt.cli/source yampt.editor/source yampt.translator/source yampt.tests/source \
    -name "*.cpp" -o -name "*.hpp" | xargs clang-format -i

echo "Done."
