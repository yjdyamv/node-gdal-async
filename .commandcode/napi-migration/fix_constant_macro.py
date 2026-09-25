#!/usr/bin/env python3
"""Drop the probes, restore the tail, and make NODE_DEFINE_CONSTANT use the ambient env."""

import re

n = 0

# --- gdal_common.hpp: the ambient env is the one Init registered -------
p = "src/gdal_common.hpp"
s = open(p, encoding="utf-8").read()
old = "#define NODE_DEFINE_CONSTANT(target, constant) target.Set(#constant, Napi::Number::New(target.Env(), constant))"
new = "#define NODE_DEFINE_CONSTANT(target, constant) target.Set(#constant, Napi::Number::New(node_gdal::napi_env(), constant))"
if old in s:
    s = s.replace(old, new, 1)
    n += 1
old2 = """#define NODE_DEFINE_CONSTANT_HEX(target, constant)                                                                     \\
  target.Set(#constant, Napi::Number::New(target.Env(), constant))"""
new2 = """#define NODE_DEFINE_CONSTANT_HEX(target, constant)                                                                     \\
  target.Set(#constant, Napi::Number::New(node_gdal::napi_env(), constant))"""
if old2 in s:
    s = s.replace(old2, new2, 1)
    n += 1
open(p, "w", encoding="utf-8").write(s)

# --- node_gdal.cpp: drop the probes, restore the full tail -------------
p = "src/node_gdal.cpp"
s = open(p, encoding="utf-8").read()

s = re.sub(r'  \{\n    napi_value k = nullptr;.*?\n  \}\n', '', s, flags=re.S)
s = re.sub(r'  \{\n    fprintf\(stderr, "### before Number.*?\n  \}\n', '', s, flags=re.S)
s = re.sub(r'^ *fprintf\(stderr, "### [^"]*\\n"\);\n', '', s, flags=re.M)
s = s.replace('  fflush(stderr);\n', '')

if 'CPLE_OpenFailed);' not in s:
    anchor = """  Napi::Object supports = Napi::Object::New(env);
  target.Set( Napi::String::New(env, "supports"), supports);
"""
    tail = """  NODE_DEFINE_CONSTANT(target, CPLE_OpenFailed);
  NODE_DEFINE_CONSTANT(target, CPLE_IllegalArg);
  NODE_DEFINE_CONSTANT(target, CPLE_NotSupported);
  NODE_DEFINE_CONSTANT(target, CPLE_AssertionFailed);
  NODE_DEFINE_CONSTANT(target, CPLE_NoWriteAccess);
  NODE_DEFINE_CONSTANT(target, CPLE_UserInterrupt);

  napi_add_env_cleanup_hook(env, Cleanup, nullptr);
"""
    if anchor in s:
        s = s.replace(anchor, anchor + "\n" + tail, 1)
        n += 1

if 'napi_add_env_cleanup_hook' not in s:
    s = s.replace("""  NODE_DEFINE_CONSTANT(target, CPLE_UserInterrupt);
""", """  NODE_DEFINE_CONSTANT(target, CPLE_UserInterrupt);

  napi_add_env_cleanup_hook(env, Cleanup, nullptr);
""", 1)
    n += 1

open(p, "w", encoding="utf-8").write(s)
print("patched", n, "; ### left:", s.count("###"))
