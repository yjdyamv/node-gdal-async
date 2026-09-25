#!/bin/bash
# AddressSanitizer variant of run.sh, used to chase the heap corruption in the
# pixel-function path. The output replaces the normal build, so rebuild with
# run.sh afterwards.
set -e

HERE=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
REPO=$(cd "$HERE/../.." && pwd)
TC=${TC:-$HOME/toolchain}
WS=${WS:-$HOME/ws}
MODE=${1:-build}

export PATH="$TC/node/bin:$PATH"
_LIBDIRS=$(find "$TC/gdal-sys" -name '*.so*' -printf '%h\n' 2>/dev/null | sort -u | tr '\n' ':')
export LD_LIBRARY_PATH="${_LIBDIRS}${LD_LIBRARY_PATH}"

GDAL_INC="$TC/gdal-sys/usr/include/gdal"
GDAL_LIB="$TC/gdal-sys/usr/lib/x86_64-linux-gnu"
NODE_INC="$TC/node/include/node"

INCS="-I$NODE_INC -I$GDAL_INC -I$TC/gdal-sys/usr/include -I$WS/include -I$WS/src -I$WS/src/utils -I$WS/node_modules/nan -I$WS/node_modules/node-addon-api"
DEFS="-DNODE_GYP_MODULE_NAME=gdal -DBUILDING_NODE_EXTENSION -DPLATFORM=linux -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64 -DNOGDI=1 -DHAVE_LIBZ=1 -DLINUX -DBUNDLED_GDAL=1"
FLAGS="-std=gnu++20 -fPIC -O1 -g -w -frtti -fexceptions -fsanitize=address -fno-omit-frame-pointer"

mkdir -p "$WS"
rsync -a --delete "$REPO/src/" "$WS/src/"
rsync -a --delete "$REPO/include/" "$WS/include/"
cp -f "$REPO/binding.gyp" "$REPO/package.json" "$REPO/tsconfig.json" "$WS/" 2>/dev/null || true
cp -f "$REPO/.mocharc.json" "$WS/" 2>/dev/null || true
mkdir -p "$WS/obj"

SOURCES=$(grep -oE '"src/[^"]+\.cpp"' "$WS/binding.gyp" | tr -d '"')
echo "$SOURCES" | tr ' ' '\n' | grep . > "$WS/obj/sources.txt"

if [ "$MODE" = "build" ]; then
  compile_one() {
    s="$1"
    o="$WS/obj/$(echo "$s" | tr '/' '_' | sed 's/\.cpp$/.o/')"
    g++ -c $FLAGS $DEFS $INCS -o "$o" "$WS/$s" 2> "$o.err" \
      || { echo "COMPILE FAIL: $s"; head -40 "$o.err"; return 1; }
  }
  export -f compile_one; export WS FLAGS DEFS INCS
  xargs -a "$WS/obj/sources.txt" -P "$(nproc)" -I{} bash -c 'compile_one "$@"' _ {}
  OBJS=$(sed 's#\.cpp$#.o#' "$WS/obj/sources.txt" | tr '/' '_' | sed "s#^#$WS/obj/#" | tr '\n' ' ')
  BINDIR="$WS/lib/binding/node-v137-linux-x64"
  mkdir -p "$BINDIR"
  g++ -shared -fsanitize=address -o "$BINDIR/gdal.node" $OBJS -L"$GDAL_LIB" -lgdal -Wl,-rpath,"$GDAL_LIB"
  echo "ASAN BUILD OK: $BINDIR/gdal.node"
  exit 0
fi

if [ "$MODE" = "test" ]; then
  cd "$WS"
  unset GDAL_DATA PROJ_LIB
  shift || true
  ASAN_LIB=$(gcc -print-file-name=libasan.so)
  exec env LD_PRELOAD="$ASAN_LIB" \
    ASAN_OPTIONS="detect_leaks=0:abort_on_error=1:fast_unwind_on_malloc=0:symbolize=1" \
    node_modules/.bin/mocha --reporter spec --timeout 20000 "$@"
fi

echo "usage: run_asan.sh [build|test] [args]"
exit 1
