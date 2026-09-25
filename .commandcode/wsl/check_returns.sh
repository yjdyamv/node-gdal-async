#!/bin/bash
# Like run.sh syntax, but with -Wreturn-type re-enabled (run.sh passes -w).
# A NAN_METHOD that falls off its end returns a garbage Napi::Value.
set -e

HERE=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
REPO=$(cd "$HERE/../.." && pwd)
TC=${TC:-$HOME/toolchain}
WS=${WS:-$HOME/ws}

export PATH="$TC/node/bin:$PATH"
_LIBDIRS=$(find "$TC/gdal-sys" -name '*.so*' -printf '%h\n' 2>/dev/null | sort -u | tr '\n' ':')
export LD_LIBRARY_PATH="${_LIBDIRS}${LD_LIBRARY_PATH}"

GDAL_INC="$TC/gdal-sys/usr/include/gdal"
NODE_INC="$TC/node/include/node"

INCS="-I$NODE_INC -I$GDAL_INC -I$TC/gdal-sys/usr/include -I$WS/include -I$WS/src -I$WS/src/utils -I$WS/node_modules/nan -I$WS/node_modules/node-addon-api"
DEFS="-DNODE_GYP_MODULE_NAME=gdal -DBUILDING_NODE_EXTENSION -DPLATFORM=linux -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64 -DNOGDI=1 -DHAVE_LIBZ=1 -DLINUX -DBUNDLED_GDAL=1"
FLAGS="-std=gnu++20 -fPIC -O1 -frtti -fexceptions -Wreturn-type"

rsync -a --delete "$REPO/src/" "$WS/src/"
mkdir -p "$WS/obj"

SOURCES=$(grep -oE '"src/[^"]+\.cpp"' "$WS/binding.gyp" | tr -d '"')
echo "$SOURCES" | tr ' ' '\n' | grep . > "$WS/obj/sources.txt"

while read -r s; do
  g++ $FLAGS $DEFS $INCS -c -o /dev/null "$WS/$s" 2>&1 \
    | grep -E 'warning: (no return statement|control reaches end)|error: (no return statement|control reaches end)' \
    | sed "s#^#${s}: #" || true
done < "$WS/obj/sources.txt"
