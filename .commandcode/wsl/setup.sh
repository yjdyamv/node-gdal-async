#!/bin/bash
# Provisions a root-free Linux toolchain for building/testing gdal-async in WSL.
#
# Downloads, into $HOME/toolchain:
#   * Node.js Linux build (provides both node and the C++ headers)
#   * libgdal-dev + the full set of runtime libraries, extracted from .deb
# No root / apt install is required (sudo is not usable in this environment).
set -e
TC=${TC:-$HOME/toolchain}
DL=$TC/dl
mkdir -p "$TC" "$DL"

NODEV=${NODEV:-v24.19.0}
if [ ! -x "$TC/node/bin/node" ]; then
  curl -sSL -o "$DL/node-$NODEV-linux-x64.tar.xz" \
    "https://cdn.npmmirror.com/binaries/node/$NODEV/node-$NODEV-linux-x64.tar.xz"
  tar -C "$TC" -xf "$DL/node-$NODEV-linux-x64.tar.xz"
  ln -sfn "$TC/node-$NODEV-linux-x64" "$TC/node"
fi

# GDAL: use the archive version matching the distro glibc, extracted without root
PREFIX=$TC/gdal-sys
mkdir -p "$PREFIX" "$DL/debs"
cd "$DL/debs"
RUNTIME=$(apt-cache depends libgdal-dev 2>/dev/null | grep -oE 'libgdal[0-9]+' | head -1)
RAW=$(apt-cache depends --recurse --no-recommends --no-suggests --no-conflicts \
      --no-breaks --no-replaces --no-enhances "$RUNTIME" 2>/dev/null || true)
PKGS=$(echo "$RAW" | grep -E '^[A-Za-z0-9][A-Za-z0-9.+-]*$' | sort -u)
for p in $RUNTIME libgdal-dev $PKGS libarmadillo15 libproj-dev gdal-data; do
  st=$(dpkg-query -W -f='${Status}' "$p" 2>/dev/null)
  [ "$st" = "install ok installed" ] && continue
  ls ${p}_*.deb >/dev/null 2>&1 || apt-get download "$p" >/dev/null 2>&1 || true
  f=$(ls -t ${p}_*.deb 2>/dev/null | head -1)
  [ -n "$f" ] && dpkg-deb -x "$f" "$PREFIX" 2>/dev/null || true
done
echo "toolchain ready: $($TC/node/bin/node --version), $(ls $PREFIX/usr/lib/x86_64-linux-gnu/libgdal.so* 2>/dev/null | head -1)"
