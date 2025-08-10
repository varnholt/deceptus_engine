# build_and_package.py
import os
import sys
import time
from pathlib import Path
import docker

def ts() -> str:
    return time.strftime("%Y-%m-%dT%H-%M-%S", time.gmtime())

def print_build_chunk(chunk: dict):
    # docker SDK returns mixed keys; print the useful ones
    for k in ("stream", "status", "progress", "error"):
        if k in chunk and chunk[k]:
            sys.stdout.write(str(chunk[k]))
            if not str(chunk[k]).endswith("\n"):
                sys.stdout.write("\n")
            sys.stdout.flush()
            break

def main():
    script_dir = Path(__file__).resolve().parent         # .../deceptus_engine/docker
    repo_root  = script_dir.parent                       # .../deceptus_engine
    tools_dir  = script_dir                              # mount this (contains bundle_deploy.sh)
    out_dir    = repo_root / "build_output"
    out_dir.mkdir(parents=True, exist_ok=True)

    bundler = tools_dir / "bundle_deploy.sh"
    if not bundler.is_file():
        print(f"ERROR: {bundler} not found next to this script.", file=sys.stderr)
        sys.exit(1)

    image_name = "deceptus_engine"
    artifact   = f"deceptus_{ts()}.tgz"

    # connect to docker
    client = docker.from_env()

    # build image from docker
    print("building image from docker/...")
    for chunk in client.api.build(
        path=str(tools_dir),
        dockerfile="Dockerfile",
        tag=image_name,
        rm=True,
        decode=True,
    ):
        print_build_chunk(chunk)
        if "error" in chunk:
            sys.exit(1)

    # ensure image is there
    image = client.images.get(image_name)
    image_ref = image.tags[0] if image.tags else image.id

    # volumes: mount build_output and docker
    volumes = {
        str(out_dir):  {"bind": "/home/builder/output", "mode": "rw"},
        str(tools_dir):{"bind": "/home/builder/tools",  "mode": "ro"},
    }

    # command to run inside the container
    container_cmd = r"""
    set -euo pipefail
    cd /home/builder/deceptus_engine/build

    # copy bundler from read-only mount to a writable temp path
    cp /home/builder/tools/bundle_deploy.sh /tmp/bundle_deploy.sh

    # normalize CRLF -> LF without editing the read-only source
    if command -v sed >/dev/null 2>&1; then
      sed 's/\r$//' /tmp/bundle_deploy.sh > /tmp/bundle_deploy.sh.lf || true
      mv /tmp/bundle_deploy.sh.lf /tmp/bundle_deploy.sh
    else
      # fallback using tr
      tr -d '\r' < /tmp/bundle_deploy.sh > /tmp/bundle_deploy.sh.lf || true
      mv /tmp/bundle_deploy.sh.lf /tmp/bundle_deploy.sh
    fi
    chmod +x /tmp/bundle_deploy.sh

    # bundle and package
    bash /tmp/bundle_deploy.sh
    tar czf "/home/builder/output/${ARTIFACT}" -C deploy .
    echo "tarball: /home/builder/output/${ARTIFACT}"
    """

    print("running container: bundle & package...")
    container = client.containers.create(
        image=image_ref,
        command=["bash", "-lc", container_cmd],
        environment={"ARTIFACT": artifact},
        volumes=volumes,
        detach=True,
    )
    try:
        container.start()
        for line in container.logs(stream=True):
            try:
                sys.stdout.write(line.decode("utf-8"))
            except Exception:
                sys.stdout.write(str(line))
        result = container.wait()
        code = result.get("StatusCode", 1)
        if code != 0:
            print(f"error: container exited with code {code}", file=sys.stderr)
            try:
                tail = container.logs(tail=50).decode("utf-8", errors="ignore")
                print(tail, file=sys.stderr)
            except Exception:
                pass
            sys.exit(code)
    finally:
        try:
            container.remove(force=True)
        except Exception:
            pass

    print("\nartifact ready:")
    print(out_dir / artifact)

if __name__ == "__main__":
    main()
