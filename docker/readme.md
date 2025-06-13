
# Building the Deceptus Engine in Docker

---

## Linux/macOS

### 1. Build the Docker Image

```bash
docker build -t deceptus_engine .
```

---

### 2. Run the Container (Safe Mode, No Auto-Delete)

```bash
docker run -it \
  -v "$(pwd)/game_data:/home/builder/game_data" \
  -v "$(pwd)/build_output:/home/builder/output" \
  deceptus_engine
```

---

### 3. Export the Build

```bash
tar czf /home/builder/output/deceptus_build.tgz -C /home/builder/deceptus_engine/build .
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

### 3. Export the Build

```bash
tar czf /home/builder/output/deceptus_build.tgz -C /home/builder/deceptus_engine/build .
```

---
