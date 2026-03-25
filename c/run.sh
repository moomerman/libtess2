#!/bin/sh
# Build and run the libtess2 C tests against the pre-built library.
#
# Usage:  sh run.sh
#
# Automatically picks the right library for the current platform.

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
LIB_DIR="$SCRIPT_DIR/../lib"

OS="$(uname -s)"
ARCH="$(uname -m)"

case "$OS" in
    Darwin)
        case "$ARCH" in
            arm64) LIB="$LIB_DIR/libtess2_darwin_arm64.a" ;;
            *)     LIB="$LIB_DIR/libtess2_darwin_amd64.a" ;;
        esac
        ;;
    Linux)
        LIB="$LIB_DIR/libtess2_linux_amd64.a"
        ;;
    *)
        echo "Unsupported platform: $OS $ARCH" >&2
        exit 1
        ;;
esac

if [ ! -f "$LIB" ]; then
    echo "Library not found: $LIB" >&2
    exit 1
fi

cc -o "$SCRIPT_DIR/tess2_test" "$SCRIPT_DIR/tess2_test.c" "$LIB" -lm
"$SCRIPT_DIR/tess2_test"
rm -f "$SCRIPT_DIR/tess2_test"
