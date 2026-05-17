#!/bin/bash
# bundle_deploy.sh — collect the deceptus binary and all runtime shared libraries
# into a self-contained deploy/ directory, then optionally tar it up.
#
# Usage:
#   bundle_deploy.sh [OPTIONS]
#
# Options:
#   --no-strip-render-libs   Bundle GPU/render libs (libGL, libEGL, libvulkan,
#                            libOpenGL, libGLX, libGLdispatch, libdrm, libnvidia)
#                            instead of excluding them. Use this only when the
#                            target system has no working Mesa/driver stack of
#                            its own. Default: render libs are excluded so the
#                            host GPU driver stack is used (correct for Steam Deck).
#   --help                   Print this message and exit.
#
# Output:
#   deploy/
#     deceptus       — stripped release binary
#     lib/           — all bundled .so files (flat, symlinks resolved)
#     run.sh         — wrapper that sets LD_LIBRARY_PATH and execs the binary
#
# Notes:
#   - Run from inside the CMake build directory (where the deceptus binary lives).
#   - SDL3, SFML, Lua, and GLEW are built from source via FetchContent; ldd
#     resolves them through the binary's RPATH so they are picked up automatically.
#     SDL3 is also copied explicitly in case RPATH is not set for it.
#   - glibc (libc, libm, ld-linux) is intentionally bundled: the Arch-built
#     versions are ABI-stable and the Steam Deck ships a compatible glibc.

set -e

STRIP_RENDER_LIBS=1

for argument in "$@"; do
    case "$argument" in
        --no-strip-render-libs)
            STRIP_RENDER_LIBS=0
            ;;
        --help)
            sed -n '2,/^[^#]/{ /^#/{ s/^# \?//; p }; /^[^#]/q }' "$0"
            exit 0
            ;;
        *)
            echo "error: unknown argument: $argument" >&2
            echo "run with --help for usage" >&2
            exit 1
            ;;
    esac
done

BUILD_DIR="$(pwd)"
DEPLOY_DIR="$BUILD_DIR/deploy"
LIB_DIR="$DEPLOY_DIR/lib"
BIN_NAME="deceptus"

RENDER_LIB_PATTERN="libGL\|libGLX\|libGLdispatch\|libOpenGL\|libEGL\|libvulkan\|libdrm\|libnvidia"

rm -rf "$DEPLOY_DIR"
mkdir -p "$LIB_DIR"

cp "$BUILD_DIR/$BIN_NAME" "$DEPLOY_DIR/"

# collect all runtime dependencies reported by the dynamic linker
if [ "$STRIP_RENDER_LIBS" -eq 1 ]; then
    ldd "$BUILD_DIR/$BIN_NAME" \
        | awk '/=> \/|=>\// {print $3}' \
        | grep -v "$RENDER_LIB_PATTERN" \
        | while read resolved_lib; do
            cp -vL "$resolved_lib" "$LIB_DIR/"
        done
else
    ldd "$BUILD_DIR/$BIN_NAME" \
        | awk '/=> \/|=>\// {print $3}' \
        | while read resolved_lib; do
            cp -vL "$resolved_lib" "$LIB_DIR/"
        done
fi

# SDL3 is built from source; copy all versioned .so variants explicitly
# in case the binary's RPATH does not expose them to ldd
for sdl3_lib in "$BUILD_DIR"/_deps/sdl3-build/libSDL3.so*; do
    if [ -f "$sdl3_lib" ]; then
        cp -vL "$sdl3_lib" "$LIB_DIR/"
    fi
done

cat > "$DEPLOY_DIR/run.sh" <<'EOF'
#!/bin/bash
DIR="$(cd "$(dirname "$0")" && pwd)"
export LD_LIBRARY_PATH="$DIR/lib"
exec "$DIR/deceptus" "$@"
EOF
chmod +x "$DEPLOY_DIR/run.sh"

echo ""
if [ "$STRIP_RENDER_LIBS" -eq 1 ]; then
    echo "render libs excluded (host GPU driver stack will be used)"
else
    echo "render libs included"
fi
echo "bundled in: $DEPLOY_DIR"
