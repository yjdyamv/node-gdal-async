#!/usr/bin/env python3
"""Napi::Env has no default constructor, so the ambient environment is held as a
raw napi_env and wrapped on demand:

    node_gdal::napi_env      ->  node_gdal::napi_env()
    extern Napi::Env napi_env;   ->  extern ::napi_env napi_env_storage; + inline Napi::Env napi_env()

Run once over the worktree.
"""
import glob
import re
import sys

paths = []
for a in sys.argv[1:]:
    if a == "--all":
        paths += glob.glob("src/**/*.cpp", recursive=True) + glob.glob("src/**/*.hpp", recursive=True)
    else:
        paths.append(a)

n = 0
for path in sorted(set(paths)):
    with open(path, encoding="utf-8") as f:
        src = f.read()
    out = src
    # declaration in gdal_common.hpp
    out = out.replace(
        "extern Napi::Env napi_env;",
        "// Napi::Env has no default constructor, so the environment is stored raw and\n"
        "// wrapped on demand (gdal-async is a single-instance addon, it is set once in Init)\n"
        "extern ::napi_env napi_env_storage;\n"
        "inline Napi::Env napi_env() {\n"
        "  return Napi::Env(napi_env_storage);\n"
        "}",
    )
    # definition in node_gdal.cpp
    out = out.replace("Napi::Env napi_env;\n", "::napi_env napi_env_storage = nullptr;\n")
    out = out.replace("  napi_env = env;\n", "  napi_env_storage = env;\n")
    # uses (guarded so a second run is a no-op)
    out = re.sub(r"node_gdal::napi_env(?!\(|_)", "node_gdal::napi_env()", out)
    out = re.sub(r"(?<![:\w])napi_env(?!\()\.Undefined\(\)", "napi_env().Undefined()", out)
    if out != src:
        with open(path, "w", encoding="utf-8") as f:
            f.write(out)
        n += 1
        print("env    %s" % path)
print("%d files" % n)
