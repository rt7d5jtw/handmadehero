#!/bin/sh
set -e
cd "$(dirname "$0")"

# -----------------------------------------------------------------------------
# Default build configuration
# -----------------------------------------------------------------------------
EXECUTABLE="handmadehero"
# Base flags shared by both GCC and Clang
COMPILER_DEBUG_FLAGS="-g -fsanitize=address"
COMPILER_WARNING_FLAGS="-Wall -Wextra -pedantic -Wimplicit"
COMPILER_FLAGS="$COMPILER_DEBUG_FLAGS $COMPILER_WARNING_FLAGS"
LIBS="-lX11 -lasound -lm"

# Default to gcc if no argument is provided ($1 is empty)
COMPILER="${1:-gcc}"

# Validate compiler choice
case "$COMPILER" in
  gcc|clang)
    ;;
  *)
    echo "Error: Unsupported compiler '$COMPILER'. Usage: $0 [gcc|clang]" >&2
    exit 1
    ;;
esac

# Retrieve compiler version (supported by both GCC and Clang)
COMPILER_VERSION="$($COMPILER -dumpversion 2>/dev/null || echo "unknown")"
BUILD_DIR="$(pwd)/build"
EXECUTABLE_PATH="$BUILD_DIR/$EXECUTABLE"

echo "Building with $COMPILER ($COMPILER_VERSION)..."
echo "Executable location: $EXECUTABLE_PATH"

mkdir -p "build"

# compile and return
(
  cd "build"
  $COMPILER $COMPILER_FLAGS -o "$EXECUTABLE" ../src/main.c $LIBS
)
