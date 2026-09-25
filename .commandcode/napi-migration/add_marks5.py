#!/usr/bin/env python3
"""Temporary: wrap the tail statements of Init so the throwing one names itself."""

p = "src/node_gdal.cpp"
s = open(p, encoding="utf-8").read()

old = """  NODE_DEFINE_CONSTANT(target, CPLE_OpenFailed);
  fprintf(stderr, "### T5\\n");
  NODE_DEFINE_CONSTANT(target, CPLE_IllegalArg);
  NODE_DEFINE_CONSTANT(target, CPLE_NotSupported);
  NODE_DEFINE_CONSTANT(target, CPLE_AssertionFailed);
  NODE_DEFINE_CONSTANT(target, CPLE_NoWriteAccess);
  NODE_DEFINE_CONSTANT(target, CPLE_UserInterrupt);

  napi_add_env_cleanup_hook(env, Cleanup, nullptr);"""

new = """  fprintf(stderr, "### T5\\n");
#define TRY_STMT(tag, stmt)                                                                                            \\
  try {                                                                                                                \\
    stmt                                                                                                               \\
  } catch (const Napi::Error &e) {                                                                                     \\
    fprintf(stderr, "### %s threw: %s\\n", tag, e.Message().c_str());                                                 \\
  }
  TRY_STMT("CPLE_OpenFailed", NODE_DEFINE_CONSTANT(target, CPLE_OpenFailed);)
  TRY_STMT("CPLE_IllegalArg", NODE_DEFINE_CONSTANT(target, CPLE_IllegalArg);)
  TRY_STMT("CPLE_NotSupported", NODE_DEFINE_CONSTANT(target, CPLE_NotSupported);)
  TRY_STMT("CPLE_AssertionFailed", NODE_DEFINE_CONSTANT(target, CPLE_AssertionFailed);)
  TRY_STMT("CPLE_NoWriteAccess", NODE_DEFINE_CONSTANT(target, CPLE_NoWriteAccess);)
  TRY_STMT("CPLE_UserInterrupt", NODE_DEFINE_CONSTANT(target, CPLE_UserInterrupt);)
  TRY_STMT("cleanup_hook", napi_add_env_cleanup_hook(env, Cleanup, nullptr);)"""

if old in s:
    s = s.replace(old, new, 1)
    open(p, "w", encoding="utf-8").write(s)
    print("wrapped")
else:
    print("anchor not found")
