#!/usr/bin/env python3
"""Temporary: keep only the cleanup hook in the Init tail."""

p = "src/node_gdal.cpp"
s = open(p, encoding="utf-8").read()

old = """  NODE_DEFINE_CONSTANT(target, CPLE_OpenFailed);
  NODE_DEFINE_CONSTANT(target, CPLE_IllegalArg);
  NODE_DEFINE_CONSTANT(target, CPLE_NotSupported);
  NODE_DEFINE_CONSTANT(target, CPLE_AssertionFailed);
  NODE_DEFINE_CONSTANT(target, CPLE_NoWriteAccess);
  NODE_DEFINE_CONSTANT(target, CPLE_UserInterrupt);

  napi_add_env_cleanup_hook(env, Cleanup, nullptr);"""

new = """  napi_add_env_cleanup_hook(env, Cleanup, nullptr);"""

if old in s:
    s = s.replace(old, new, 1)
    open(p, "w", encoding="utf-8").write(s)
    print("CPLE block removed, hook kept")
else:
    print("anchor not found")
