#!/usr/bin/env bash
#
# Copies the Emscripten WASM build artifacts from ../../build_wasm and the mobile touch
# shell from ../mobile_controls into the Android app's assets, so they get packed into the APK.
#
# Run this after every WASM rebuild (build_wasm.bat), then rebuild the APK.
#
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="$SCRIPT_DIR/../../build_wasm"
SHELL_HTML="$SCRIPT_DIR/../mobile_controls/mobile.html"
ASSET_DIR="$SCRIPT_DIR/app/src/main/assets/game"

if [ ! -d "$BUILD_DIR" ]; then
    echo "error: build_wasm not found at $BUILD_DIR — run build_wasm.bat first" >&2
    exit 1
fi

mkdir -p "$ASSET_DIR"

for artifact in deceptus.js deceptus.wasm deceptus.data; do
    if [ ! -f "$BUILD_DIR/$artifact" ]; then
        echo "error: missing $artifact in $BUILD_DIR — is the WASM build complete?" >&2
        exit 1
    fi
    cp -v "$BUILD_DIR/$artifact" "$ASSET_DIR/$artifact"
done

# The mobile touch shell doubles as the entry page (index.html). No service-worker header hack is
# needed: the in-app AssetHttpServer sends real COOP/COEP headers and GeckoView honors them.
cp -v "$SHELL_HTML" "$ASSET_DIR/index.html"

echo
echo "assets ready in $ASSET_DIR:"
ls -la --block-size=1 "$ASSET_DIR" | awk 'NR>1 {print "  "$5"\t"$NF}'
