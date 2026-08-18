@echo off
docker build -t deceptus-engine-dev -f docker/Dockerfile.dev docker/
docker run -it --rm -v "%CD%:/workspace" -w /workspace deceptus-engine-dev bash -c "bash /workspace/docker/build.sh; bash"
