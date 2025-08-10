
#!/bin/bash
set -e

# paths
BUILD_DIR="$(pwd)"
DEPLOY_DIR="$BUILD_DIR/deploy"
BIN_NAME="deceptus"

# prepare deploy directory
rm -rf "$DEPLOY_DIR"
mkdir -p "$DEPLOY_DIR"

# copy binary
cp "$BUILD_DIR/$BIN_NAME" "$DEPLOY_DIR/"

# copy runtime shared libraries
ldd "$BUILD_DIR/$BIN_NAME" | awk '/=> \/|=>\// {print $3}' | while read lib; do
    cp -v --parents "$lib" "$DEPLOY_DIR/"
done

# copy libSDL3 built from source if it's local
if [ -f "$BUILD_DIR/_deps/sdl3-build/libSDL3.so.0" ]; then
    cp "$BUILD_DIR/_deps/sdl3-build/libSDL3.so.0" "$DEPLOY_DIR/"
fi

# flatten and clean symlinks
cd "$DEPLOY_DIR"
find . -type l -exec sh -c 'target=$(readlink "{}"); cp -vL "{}" "real_${target##*/}"' \;

# add wrapper script
cat > run.sh <<EOF
#!/bin/bash
DIR="\$(cd "\$(dirname "\$0")" && pwd)"
export LD_LIBRARY_PATH="\$DIR"
exec "\$DIR/$BIN_NAME" "\$@"
EOF
chmod +x run.sh

echo "all libraries and binary bundled in: $DEPLOY_DIR"
