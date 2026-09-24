#!/bin/bash
# Per-header syntax check: compiles a throwaway translation unit that includes
# exactly one header. Used to verify the header phase of the N-API migration
# independently of the (still unconverted) .cpp bodies.
#
# A header whose own include closure still pulls in nan.h will FAIL here - that
# is the expected signal that a dependency has not been converted yet.
#
# Usage:  .commandcode/wsl/check_headers.sh src/foo.hpp [src/bar.hpp ...]
set -e

HERE=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
REPO=$(cd "$HERE/../.." && pwd)
TC=${TC:-$HOME/toolchain}
WS=${WS:-$HOME/ws}

export PATH="$TC/node/bin:$PATH"

rsync -a --delete "$REPO/src/" "$WS/src/"
rsync -a --delete "$REPO/include/" "$WS/include/"
cp -f "$REPO/binding.gyp" "$REPO/package.json" "$WS/" 2>/dev/null || true
mkdir -p "$WS/obj"

GDAL_INC="$TC/gdal-sys/usr/include/gdal"
NODE_INC="$TC/node/include/node"

INCS="-I$NODE_INC -I$GDAL_INC -I$TC/gdal-sys/usr/include -I$WS/include -I$WS/src -I$WS/src/utils -I$WS/node_modules/nan -I$WS/node_modules/node-addon-api"
DEFS="-DNODE_GYP_MODULE_NAME=gdal -DBUILDING_NODE_EXTENSION -DPLATFORM=linux -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64 -DNOGDI=1 -DHAVE_LIBZ=1 -DLINUX -DBUNDLED_GDAL=1"
FLAGS="-std=gnu++20 -fPIC -O1 -w -Wreturn-type -Werror=return-type -frtti -fexceptions"

FAIL=0
for h in "$@"; do
  tu="$WS/obj/hc.cpp"
  err="$WS/obj/hc.err"
  printf '#include "%s"\n' "${h#src/}" > "$tu"
  if g++ $FLAGS $DEFS $INCS -fsyntax-only "$tu" 2> "$err"; then
    echo "OK:   $h"
  else
    echo "FAIL: $h"
    grep -E ': error' "$err" | head -4 | sed 's/^/        /'
    FAIL=1
  fi
done
exit $FAIL
