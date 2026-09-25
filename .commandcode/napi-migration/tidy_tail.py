#!/usr/bin/env python3
"""Tidy the Init tail: drop the duplicated constant block, restore the cleanup hook."""

import re

p = "src/node_gdal.cpp"
s = open(p, encoding="utf-8").read()

# the appended duplicate used the macro form; the original block is below it
dup = """  NODE_DEFINE_CONSTANT(target, CPLE_OpenFailed);
  NODE_DEFINE_CONSTANT(target, CPLE_IllegalArg);
  NODE_DEFINE_CONSTANT(target, CPLE_NotSupported);
  NODE_DEFINE_CONSTANT(target, CPLE_AssertionFailed);
  NODE_DEFINE_CONSTANT(target, CPLE_NoWriteAccess);
  NODE_DEFINE_CONSTANT(target, CPLE_UserInterrupt);

"""
if s.count(dup) > 1:
    s = s.replace(dup, "", 1)
    print("dropped the duplicate")

if 'napi_add_env_cleanup_hook' not in s:
    marker = """  target.Set( Napi::String::New(env, "CPLE_UserInterrupt"), Napi::Number::New(env, CPLE_UserInterrupt));
"""
    if marker in s:
        s = s.replace(marker, marker + "\n  napi_add_env_cleanup_hook(env, Cleanup, nullptr);\n", 1)
        print("cleanup hook restored")

open(p, "w", encoding="utf-8").write(s)

tail = s.split("  Napi::Object supports")[-1]
print("---- tail ----")
print(tail[:1400])
