#!/bin/sh
set -e
cd "$(dirname "$0")"

COMPILER="gcc"
BUILD_MODE="debug"

for arg in "$@"; do
  case "$arg" in
    clean)
      echo "Cleaning build directory..."
      rm -rf "build"
      echo "Cleanup complete."
      exit 0
      ;;
    release)
      BUILD_MODE="release"
      ;;
    debug)
      BUILD_MODE="debug"
      ;;
    gcc|clang)
      COMPILER="$arg"
      ;;
    *)
      echo "Error: Unknown argument '$arg'." >&2
      echo "Usage: $0 [gcc|clang] [debug|release] [clean]" >&2
      exit 1
      ;;
  esac
done

# -----------------------------------------------------------------------------
# Build Configuration & Flags
# -----------------------------------------------------------------------------
EXECUTABLE="handmadehero"
WARNING_FLAGS="-Wall -Wextra -pedantic -Wimplicit"
LIBS="-lX11 -lasound -lm"

if [ "$BUILD_MODE" = "release" ]; then
  # -O2: Standard optimizations
  # -DNDEBUG: Disables standard C asserts
  MODE_FLAGS="-O2 -DNDEBUG"
else
  # -g: Full debug symbols
  # -fsanitize=address: Memory error checks (AddressSanitizer)
  MODE_FLAGS="-g -fsanitize=address -DDEBUG"
fi

COMPILER_FLAGS="$MODE_FLAGS $WARNING_FLAGS"

COMPILER_VERSION="$($COMPILER -dumpversion 2>/dev/null || echo "unknown")"
BUILD_DIR="$(pwd)/build"
EXECUTABLE_PATH="$BUILD_DIR/$EXECUTABLE"

echo "Building $EXECUTABLE [$BUILD_MODE] with $COMPILER ($COMPILER_VERSION)..."
echo "Target location: $EXECUTABLE_PATH"

mkdir -p "build"

(
  cd "build"
  $COMPILER "$COMPILER_FLAGS" -o "$EXECUTABLE" ../src/main.c "$LIBS"
)
