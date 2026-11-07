#!/usr/bin/env bash
# Deploy the latest Linux build of this game to a Steam Deck over SSH.
#
# Takes the newest green CI artifact from the default branch. If that build needs a
# newer glibc than SteamOS ships, it rebuilds the same commit locally (WSL on Windows,
# natively on Linux) and deploys that instead.
#
# Usage: steam/deploy_steamdeck.sh [--local] [--run-id <id>] [--force]
#   --local        skip CI, always build locally
#   --run-id <id>  deploy the artifact of a specific workflow run
#   --force        close the game on the Deck if it is running
#
# Connection settings come from steamdeck.env next to this script (gitignored),
# see steamdeck.env.example.

set -euo pipefail

# --- per-game config ------------------------------------------------------------

GAME_NAME="Deceptus"
REPO="varnholt/deceptus_engine"
BRANCH="master"
WORKFLOW="linux.yml"
ARTIFACT="deceptus-linux"
INSTALL_DIR="Games/Deceptus"   # relative to the Deck user's home, the Steam shortcut points here
MAIN_BINARY="deceptus"
EXECUTABLES="deceptus run.sh"
PRESERVE=""                     # saves and settings live in ~/.local/share/deceptus

BUILD_PACKAGES="gcc-14 g++-14 cmake ninja-build git \
   libglm-dev \
   libx11-dev libxrandr-dev libxinerama-dev libxcursor-dev libxi-dev libxkbcommon-dev \
   mesa-common-dev libgl-dev libglvnd-dev \
   libasound2-dev libpulse-dev \
   libogg-dev libvorbis-dev libflac-dev libopenal-dev \
   libfreetype-dev libpng-dev \
   libudev-dev"

# runs inside the checked out source tree, must fill $PKG with the same layout as the CI artifact
BUILD_SCRIPT='
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_COMPILER=gcc-14 -DCMAKE_CXX_COMPILER=g++-14
cmake --build build --parallel
mkdir -p "$PKG/lib"
cp build/deceptus "$PKG/"
cp -r data "$PKG/"
find build/_deps -name "*.so*" ! -type l -exec cp {} "$PKG/lib/" \;
'

# runs locally after a local build was unpacked into $PKG_DIR; same launcher as linux.yml
after_local_build() {
   cat > "$PKG_DIR/run.sh" <<'EOF'
#!/bin/sh
DIR="$(cd "$(dirname "$0")" && pwd)"
for versioned_lib in "$DIR/lib/"*.so.*; do
  base=$(basename "$versioned_lib")
  soname=$(echo "$base" | grep -oP '.*\.so\.\d+')
  if [ -n "$soname" ] && [ ! -e "$DIR/lib/$soname" ]; then
    ln -s "$base" "$DIR/lib/$soname"
  fi
done
LD_LIBRARY_PATH="$DIR/lib" exec "$DIR/deceptus" "$@"
EOF
}

# --- shared ---------------------------------------------------------------------

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SECRETS="$SCRIPT_DIR/steamdeck.env"
WSL_DISTRO="${WSL_DISTRO:-Ubuntu-24.04}"

LOCAL_BUILD=0
RUN_ID=""
FORCE=0
while [ $# -gt 0 ]; do
   case "$1" in
      --local) LOCAL_BUILD=1 ;;
      --run-id) RUN_ID="$2"; shift ;;
      --force) FORCE=1 ;;
      -h|--help) sed -n '2,15p' "$0"; exit 0 ;;
      *) echo "unknown option: $1" >&2; exit 2 ;;
   esac
   shift
done

log() { printf '\n== %s\n' "$*"; }
die() { printf 'error: %s\n' "$*" >&2; exit 1; }

[ -f "$SECRETS" ] || die "missing $SECRETS (copy steamdeck.env.example and fill it in)"
# shellcheck disable=SC1090
. <(tr -d '\r' < "$SECRETS")   # tolerate a steamdeck.env saved with CRLF
: "${DECK_HOST:?set DECK_HOST in steamdeck.env}"
DECK_USER="${DECK_USER:-deck}"
DECK_PASSWORD="${DECK_PASSWORD:-}"
DECK_HOSTKEY="${DECK_HOSTKEY:-}"

case "$(uname -s)" in
   MINGW*|MSYS*|CYGWIN*) ON_WINDOWS=1 ;;
   *) ON_WINDOWS=0 ;;
esac

# ssh / scp: PuTTY on Windows (takes a password), otherwise OpenSSH (with sshpass if a password is set)
PLINK="$(command -v plink || true)"; PSCP="$(command -v pscp || true)"
if [ "$ON_WINDOWS" = 1 ] && [ -z "$PLINK" ] && [ -x "/c/Program Files/PuTTY/plink" ]; then
   PLINK="/c/Program Files/PuTTY/plink"; PSCP="/c/Program Files/PuTTY/pscp"
fi

deck() {
   if [ -n "$PLINK" ] && [ -n "$DECK_PASSWORD" ]; then
      "$PLINK" -ssh -batch ${DECK_HOSTKEY:+-hostkey "$DECK_HOSTKEY"} -pw "$DECK_PASSWORD" "$DECK_USER@$DECK_HOST" "$1"
   elif [ -n "$DECK_PASSWORD" ]; then
      command -v sshpass >/dev/null || die "install sshpass, or use an ssh key and leave DECK_PASSWORD empty"
      SSHPASS="$DECK_PASSWORD" sshpass -e ssh -o StrictHostKeyChecking=accept-new "$DECK_USER@$DECK_HOST" "$1"
   else
      ssh -o BatchMode=yes -o StrictHostKeyChecking=accept-new "$DECK_USER@$DECK_HOST" "$1"
   fi
}

deck_upload() {   # <local file> <remote path>
   if [ -n "$PSCP" ] && [ -n "$DECK_PASSWORD" ]; then
      local src="$1"; [ "$ON_WINDOWS" = 1 ] && src="$(cygpath -w "$1")"
      "$PSCP" -batch -q ${DECK_HOSTKEY:+-hostkey "$DECK_HOSTKEY"} -pw "$DECK_PASSWORD" "$src" "$DECK_USER@$DECK_HOST:$2"
   elif [ -n "$DECK_PASSWORD" ]; then
      SSHPASS="$DECK_PASSWORD" sshpass -e scp -q -o StrictHostKeyChecking=accept-new "$1" "$DECK_USER@$DECK_HOST:$2"
   else
      scp -q -o BatchMode=yes -o StrictHostKeyChecking=accept-new "$1" "$DECK_USER@$DECK_HOST:$2"
   fi
}

linux_path() {
   if [ "$ON_WINDOWS" = 1 ]; then wsl -d "$WSL_DISTRO" -- wslpath -a "$(cygpath -m "$1")"; else echo "$1"; fi
}

linux() {   # [--root] <script file>: run it on a Linux with glibc older than SteamOS
   local root=0; [ "$1" = --root ] && { root=1; shift; }
   if [ "$ON_WINDOWS" = 1 ]; then
      # keep Git Bash from rewriting the /mnt/... path into a Windows one
      if [ "$root" = 1 ]; then
         MSYS2_ARG_CONV_EXCL='*' wsl -d "$WSL_DISTRO" -u root -- bash "$(linux_path "$1")"
      else
         MSYS2_ARG_CONV_EXCL='*' wsl -d "$WSL_DISTRO" -- bash "$(linux_path "$1")"
      fi
   elif [ "$root" = 1 ]; then
      sudo bash "$1"
   else
      bash "$1"
   fi
}

newest_symbol() {   # <file> <prefix> -> highest versioned symbol, e.g. GLIBC_2.39
   LC_ALL=C grep -aoE "$2[0-9.]*[0-9]" "$1" | sort -uV | tail -1
}

WORK="$(mktemp -d)"
trap 'rm -rf "$WORK"' EXIT
PKG_DIR="$WORK/pkg"
mkdir -p "$PKG_DIR"

log "checking $DECK_USER@$DECK_HOST"
DECK_LIBS="$(deck 'grep -aoE "GLIBC_2\.[0-9]+" /usr/lib/libc.so.6 | sort -uV | tail -1; grep -aoE "GLIBCXX_3\.4\.[0-9]+" /usr/lib/libstdc++.so.6 | sort -uV | tail -1')" \
   || die "cannot reach the Deck at $DECK_HOST"
DECK_GLIBC="$(echo "$DECK_LIBS" | sed -n 1p | tr -d '\r')"
DECK_GLIBCXX="$(echo "$DECK_LIBS" | sed -n 2p | tr -d '\r')"
echo "Deck has $DECK_GLIBC, $DECK_GLIBCXX"

# --- 1. get a build ---------------------------------------------------------------

SHA=""
if [ "$LOCAL_BUILD" = 0 ]; then
   if [ -z "$RUN_ID" ]; then
      RUN_ID="$(gh run list -R "$REPO" -w "$WORKFLOW" -b "$BRANCH" -s success -L 1 --json databaseId --jq '.[0].databaseId')"
      [ -n "$RUN_ID" ] || die "no successful $WORKFLOW run on $BRANCH"
   fi
   SHA="$(gh run view "$RUN_ID" -R "$REPO" --json headSha --jq .headSha)"
   log "downloading $ARTIFACT from run $RUN_ID (${SHA:0:8})"
   gh run download "$RUN_ID" -R "$REPO" -n "$ARTIFACT" -D "$PKG_DIR"

   NEED_GLIBC="$(newest_symbol "$PKG_DIR/$MAIN_BINARY" 'GLIBC_2\.')"
   NEED_GLIBCXX="$(newest_symbol "$PKG_DIR/$MAIN_BINARY" 'GLIBCXX_3\.4\.')"
   echo "build needs $NEED_GLIBC, $NEED_GLIBCXX"
   for pair in "$NEED_GLIBC $DECK_GLIBC" "$NEED_GLIBCXX $DECK_GLIBCXX"; do
      set -- $pair
      [ -z "$1" ] && continue
      if [ "$(printf '%s\n%s\n' "$1" "$2" | sort -V | tail -1)" != "$2" ]; then
         echo "CI build needs $1 but the Deck only has $2, building locally instead"
         LOCAL_BUILD=1
      fi
   done
fi

if [ "$LOCAL_BUILD" = 1 ]; then
   [ -n "$SHA" ] || SHA="$(gh api "repos/$REPO/commits/$BRANCH" --jq .sha)"
   log "building ${SHA:0:8} locally$( [ "$ON_WINDOWS" = 1 ] && echo " in WSL ($WSL_DISTRO)")"
   rm -rf "$PKG_DIR"; mkdir -p "$PKG_DIR"
   OUT_TAR="$WORK/local.tar"

   cat > "$WORK/deps.sh" <<EOF
missing=\$(dpkg-query -W -f='\${Status} \${Package}\n' $BUILD_PACKAGES 2>&1 | grep -v '^install ok installed' | awk '{print \$NF}')
[ -z "\$missing" ] && exit 0
echo "installing build packages:" \$missing
apt-get update -qq && DEBIAN_FRONTEND=noninteractive apt-get install -y -qq $BUILD_PACKAGES >/dev/null
EOF
   linux --root "$WORK/deps.sh"

   cat > "$WORK/build.sh" <<EOF
set -euo pipefail
SRC="\$HOME/.cache/steamdeck_deploy/${REPO##*/}"
PKG="\$SRC.pkg"
[ -d "\$SRC/.git" ] || git clone -q --filter=blob:none https://github.com/$REPO.git "\$SRC"
cd "\$SRC"
git fetch -q origin $SHA
git checkout -q --force $SHA
git clean -qfdx -e client/build -e server/build -e build
rm -rf "\$PKG"; mkdir -p "\$PKG"
$BUILD_SCRIPT
tar -cf '$(linux_path "$OUT_TAR")' -C "\$PKG" .
EOF
   linux "$WORK/build.sh"
   tar -xf "$OUT_TAR" -C "$PKG_DIR"
   rm -f "$OUT_TAR"
   after_local_build
   RUN_ID="local"
fi

printf 'commit %s\nsource %s\ndeployed %s\n' "$SHA" "$( [ "$RUN_ID" = local ] && echo 'local build' || echo "CI run $RUN_ID")" "$(date -u +%FT%TZ)" > "$PKG_DIR/VERSION"

# --- 2. upload and swap -----------------------------------------------------------

log "packing"
TAR="$WORK/${REPO##*/}.tar"
(cd "$PKG_DIR" && tar -cf "$TAR" .)
du -h "$TAR" | cut -f1

if deck "pgrep -f '^/home/$DECK_USER/$INSTALL_DIR/' >/dev/null"; then
   [ "$FORCE" = 1 ] || die "$GAME_NAME is running on the Deck, close it or pass --force"
   deck "p='^/home/$DECK_USER/$INSTALL_DIR/'; pkill -f \"\$p\"
      for i in \$(seq 1 10); do pgrep -f \"\$p\" >/dev/null || exit 0; sleep 1; done
      pkill -KILL -f \"\$p\"; sleep 1"
fi

log "uploading"
deck "mkdir -p ~/Games/.incoming"
deck_upload "$TAR" "/home/$DECK_USER/Games/.incoming/${REPO##*/}.tar"

log "installing to ~/$INSTALL_DIR"
deck "set -e
   DEST=~/$INSTALL_DIR
   NEW=\$DEST.new
   TAR=~/Games/.incoming/${REPO##*/}.tar
   rm -rf \"\$NEW\"; mkdir -p \"\$NEW\"
   tar -xf \"\$TAR\" -C \"\$NEW\"; rm -f \"\$TAR\"
   cd \"\$NEW\"
   chmod +x $EXECUTABLES
   if [ -d lib ]; then
      for f in lib/*.so.*; do
         soname=\$(echo \"\${f##*/}\" | grep -oE '.*\.so\.[0-9]+'); [ -e \"lib/\$soname\" ] || ln -s \"\${f##*/}\" \"lib/\$soname\"
      done
   fi
   missing=\$(LD_LIBRARY_PATH=\$NEW/lib ldd ./$MAIN_BINARY 2>&1 | grep -i 'not found' || true)
   if [ -n \"\$missing\" ]; then echo \"\$missing\"; rm -rf \"\$NEW\"; echo 'not installed, the old version is untouched'; exit 1; fi
   for f in $PRESERVE; do
      if [ -e \"\$DEST/\$f\" ]; then mkdir -p \"\$(dirname \"\$f\")\"; cp -p \"\$DEST/\$f\" \"\$f\"; echo \"kept \$f\"; fi
   done
   rm -rf \"\$DEST\"; mv \"\$NEW\" \"\$DEST\"
   cat \"\$DEST/VERSION\""

log "$GAME_NAME deployed"
