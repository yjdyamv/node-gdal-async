#!/usr/bin/env python3
"""Temporary: disable the Init tail to see whether it is the crasher."""

p = "src/node_gdal.cpp"
s = open(p, encoding="utf-8").read()

old = """  TRY_STMT("CPLE_OpenFailed", NODE_DEFINE_CONSTANT(target, CPLE_OpenFailed);)"""
new = """#if 0
  TRY_STMT("CPLE_OpenFailed", NODE_DEFINE_CONSTANT(target, CPLE_OpenFailed);)"""
if old in s and "#if 0" not in s:
    s = s.replace(old, new, 1)
    s = s.replace('  TRY_STMT("cleanup_hook", napi_add_env_cleanup_hook(env, Cleanup, nullptr);)',
                  '  TRY_STMT("cleanup_hook", napi_add_env_cleanup_hook(env, Cleanup, nullptr);)\n#endif', 1)
    open(p, "w", encoding="utf-8").write(s)
    print("disabled tail")
else:
    print("no change")
