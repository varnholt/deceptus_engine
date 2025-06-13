
# Building the Deceptus Engine in Docker


## Linux/macOS

### 1. Build the Docker Image

```bash
docker build -t deceptus_engine .
```

---

### 2. Run the Container

```bash
docker run -it \
  -v "$(pwd)/game_data:/home/builder/game_data" \
  -v "$(pwd)/build_output:/home/builder/output" \
  deceptus_engine
```

---

### 3. Bundle Binary + Dependencies

Inside the container, create this script as `bundle_deploy.sh`:

```bash
#!/bin/bash
set -e

# Paths
BUILD_DIR="$(pwd)"
DEPLOY_DIR="$BUILD_DIR/deploy"
BIN_NAME="deceptus"
LIB_DIR="$DEPLOY_DIR/lib"

# Prepare deploy directory
rm -rf "$DEPLOY_DIR"
mkdir -p "$DEPLOY_DIR"
mkdir -p "$LIB_DIR"

# Copy binary
cp "$BUILD_DIR/$BIN_NAME" "$DEPLOY_DIR/"

# Copy runtime shared libraries into deploy/lib
ldd "$BUILD_DIR/$BIN_NAME" | awk '/=> \/|=>\// {print $3}' | while read lib; do
    cp -v "$lib" "$LIB_DIR/"
done

# Copy SDL3 if built locally
if [ -f "$BUILD_DIR/_deps/sdl3-build/libSDL3.so.0" ]; then
    cp "$BUILD_DIR/_deps/sdl3-build/libSDL3.so.0" "$LIB_DIR/"
fi

# Add run.sh wrapper
cat > "$DEPLOY_DIR/run.sh" <<EOF
#!/bin/bash
DIR="\$(cd \$(dirname "\$0") && pwd)"
export LD_LIBRARY_PATH="\$DIR/lib"
exec "\$DIR/$BIN_NAME" "\$@"
EOF

chmod +x "$DEPLOY_DIR/run.sh"

echo "Deceptus bundled with all required libraries in: $DEPLOY_DIR"
```

Run it:

```bash
chmod +x bundle_deploy.sh
./bundle_deploy.sh
```

---

### 4. Export as .tgz

```bash
tar czf /home/builder/output/deceptus_build.tgz -C deploy .
```

---

## Windows

### 1. Build the Docker Image

```cmd
docker build -t deceptus_engine .
```

---

### 2. Run the Container

```cmd
docker run -it ^
  -v "%CD%\game_data:/home/builder/game_data" ^
  -v "%CD%\build_output:/home/builder/output" ^
  deceptus_engine
```

---

### 3. Bundle Binary + Dependencies

Inside the container, create and run `bundle_deploy.sh` exactly as shown in the Linux/macOS section above.

---

### 4. Export as .tgz

```bash
tar czf /home/builder/output/deceptus_build.tgz -C deploy .
```

