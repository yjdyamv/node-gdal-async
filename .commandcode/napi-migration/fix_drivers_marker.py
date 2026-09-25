#!/usr/bin/env python3
"""Use the same External marker as the sibling classes for GDALDrivers."""

p = "src/collections/gdal_drivers.cpp"
s = open(p, encoding="utf-8").read()
n = 0

old = '  if (info.Length() > 0 && info[0].IsBoolean()) return;'
new = '  if (info.Length() > 0 && info[0].IsExternal()) return;'
if old in s:
    s = s.replace(old, new, 1)
    n += 1

old2 = '  std::vector<napi_value> args = {Napi::Boolean::New(node_gdal::napi_env(), true)};'
new2 = '  std::vector<napi_value> args = {Napi::External<void>::New(node_gdal::napi_env(), nullptr)};'
if old2 in s:
    s = s.replace(old2, new2, 1)
    n += 1

open(p, "w", encoding="utf-8").write(s)
print("patched", n)
