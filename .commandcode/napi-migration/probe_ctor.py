#!/usr/bin/env python3
"""Temporary: restore the plain tail and print the ctor argument type."""

n = 0

p = "src/node_gdal.cpp"
s = open(p, encoding="utf-8").read()
old = """  try {
    NODE_DEFINE_CONSTANT(target, CPLE_OpenFailed);
    NODE_DEFINE_CONSTANT(target, CPLE_IllegalArg);
    NODE_DEFINE_CONSTANT(target, CPLE_NotSupported);
    NODE_DEFINE_CONSTANT(target, CPLE_AssertionFailed);
    NODE_DEFINE_CONSTANT(target, CPLE_NoWriteAccess);
    NODE_DEFINE_CONSTANT(target, CPLE_UserInterrupt);

    napi_add_env_cleanup_hook(env, Cleanup, nullptr);
  } catch (const Napi::Error &e) {
    fprintf(stderr, "### tail threw: %s\\n", e.Message().c_str());
    fflush(stderr);
  }"""
new = """  NODE_DEFINE_CONSTANT(target, CPLE_OpenFailed);
  NODE_DEFINE_CONSTANT(target, CPLE_IllegalArg);
  NODE_DEFINE_CONSTANT(target, CPLE_NotSupported);
  NODE_DEFINE_CONSTANT(target, CPLE_AssertionFailed);
  NODE_DEFINE_CONSTANT(target, CPLE_NoWriteAccess);
  NODE_DEFINE_CONSTANT(target, CPLE_UserInterrupt);

  napi_add_env_cleanup_hook(env, Cleanup, nullptr);"""
if old in s:
    s = s.replace(old, new, 1)
    n += 1
open(p, "w", encoding="utf-8").write(s)

p = "src/collections/gdal_drivers.cpp"
s = open(p, encoding="utf-8").read()
anchor = "GDALDrivers::GDALDrivers(const Napi::CallbackInfo &info) : GDALObject<GDALDrivers>(info) {\n"
if anchor in s and "### ctor" not in s:
    s = s.replace(anchor, anchor + '  fprintf(stderr, "### ctor len=%d t=%d\\n", (int)info.Length(), info.Length() > 0 ? (int)info[0].Type() : -1);\n  fflush(stderr);\n', 1)
    n += 1
open(p, "w", encoding="utf-8").write(s)

print("patched", n)
