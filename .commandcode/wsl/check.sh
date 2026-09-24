#!/bin/bash
# Single translation unit syntax check for the N-API migration.
#
# Same flags as run.sh syntax, but for a named list of files, so that the
# conversion can be iterated file by file while the rest of the tree is still
# in its half-converted state.
#
# Usage:  .commandcode/wsl/check.sh src/async.cpp [src/foo.cpp ...]
set -e

HERE=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
REPO=$(cd "$HERE/../.." && pwd)
TC=${TC:-$HOME/toolchain}
WS=${WS:-$HOME/ws}

export PATH="$TC/node/bin:$PATH"

# sync sources into the ext4 workspace (much faster than building on /mnt/c)
mkdir -p "$WS"
rsync -a --delete "$REPO/src/" "$WS/src/"
rsync -a --delete "$REPO/include/" "$WS/include/"
cp -f "$REPO/binding.gyp" "$REPO/package.json" "$WS/" 2>/dev/null || true
mkdir -p "$WS/obj"

GDAL_INC="$TC/gdal-sys/usr/include/gdal"
NODE_INC="$TC/node/include/node"

INCS="-I$NODE_INC -I$GDAL_INC -I$TC/gdal-sys/usr/include -I$WS/include -I$WS/src -I$WS/src/utils -I$WS/node_modules/nan -I$WS/node_modules/node-addon-api"
DEFS="-DNODE_GYP_MODULE_NAME=gdal -DBUILDING_NODE_EXTENSION -DPLATFORM=linux -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64 -DNOGDI=1 -DHAVE_LIBZ=1 -DLINUX -DBUNDLED_GDAL=1"
# -w, then re-enable the one warning that matters: a NAN method used to set its
# result through info.GetReturnValue(), in node-addon-api it must return it
FLAGS="-std=gnu++20 -fPIC -O1 -w -Wreturn-type -Werror=return-type -frtti -fexceptions"

FAIL=0
for s in "$@"; do
  err="$WS/obj/$(echo "$s" | tr '/' '_').err"
  if g++ $FLAGS $DEFS $INCS -fsyntax-only "$WS/$s" 2> "$err"; then
    echo "OK: $s"
  else
    echo "FAIL: $s"
    head -40 "$err"
    FAIL=1
  fi
done
exit $FAIL
