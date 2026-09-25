#!/usr/bin/env python3
"""Temporary: enable the CPLE constants but keep the cleanup hook off."""

p = "src/node_gdal.cpp"
s = open(p, encoding="utf-8").read()

s = s.replace("""#if 0
  TRY_STMT("CPLE_OpenFailed", NODE_DEFINE_CONSTANT(target, CPLE_OpenFailed);)""",
              """  TRY_STMT("CPLE_OpenFailed", NODE_DEFINE_CONSTANT(target, CPLE_OpenFailed);)""", 1)

s = s.replace('  TRY_STMT("cleanup_hook", napi_add_env_cleanup_hook(env, Cleanup, nullptr);)\n#endif',
              '#if 0\n  TRY_STMT("cleanup_hook", napi_add_env_cleanup_hook(env, Cleanup, nullptr);)\n#endif', 1)

open(p, "w", encoding="utf-8").write(s)
print("cple on, hook off")
