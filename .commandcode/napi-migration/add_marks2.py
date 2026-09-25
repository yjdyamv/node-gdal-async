#!/usr/bin/env python3
"""Temporary: pinpoint the GDALDrivers construction."""

n = 0

p = "src/node_gdal.cpp"
s = open(p, encoding="utf-8").read()
anchor = "  GDALDrivers::Initialize(target); // calls GDALRegisterAll()\n"
if anchor in s and "### D" not in s:
    s = s.replace(anchor, '  fprintf(stderr, "### D\\n");\n' + anchor, 1)
    n += 1
anchor2 = 'target.Set( Napi::String::New(env, "drivers"), GDALDrivers::New());\n'
if anchor2 in s and "### E" not in s:
    s = s.replace(anchor2, anchor2 + '  fprintf(stderr, "### E\\n");\n', 1)
    n += 1
open(p, "w", encoding="utf-8").write(s)

p = "src/collections/gdal_drivers.cpp"
s = open(p, encoding="utf-8").read()
if "### ctor" not in s:
    old = "GDALDrivers::GDALDrivers(const Napi::CallbackInfo &info) : GDALObject<GDALDrivers>(info) {\n"
    if old in s:
        s = s.replace(old, old + '  fprintf(stderr, "### ctor len=%d\\n", (int)info.Length());\n', 1)
        n += 1
open(p, "w", encoding="utf-8").write(s)

print("patched", n)
