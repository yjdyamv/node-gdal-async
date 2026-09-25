#!/usr/bin/env python3
"""Leave one coherent constant block plus the cleanup hook at the end of Init."""

p = "src/node_gdal.cpp"
s = open(p, encoding="utf-8").read()

messy = """  napi_add_env_cleanup_hook(env, Cleanup, nullptr);

  NODE_DEFINE_CONSTANT(target, CPLE_IllegalArg);
  NODE_DEFINE_CONSTANT(target, CPLE_NotSupported);
  NODE_DEFINE_CONSTANT(target, CPLE_AssertionFailed);
  NODE_DEFINE_CONSTANT(target, CPLE_NoWriteAccess);
  NODE_DEFINE_CONSTANT(target, CPLE_UserInterrupt);
"""
clean = """  napi_add_env_cleanup_hook(env, Cleanup, nullptr);
"""
if messy in s:
    s = s.replace(messy, clean, 1)
    open(p, "w", encoding="utf-8").write(s)
    print("tidied")
else:
    print("already tidy")
