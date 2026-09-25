#!/usr/bin/env python3
"""Revert the macro experiment: target.Env() is the correct form."""

p = "src/gdal_common.hpp"
s = open(p, encoding="utf-8").read()
n = 0
if "Napi::Number::New(node_gdal::napi_env(), constant)" in s:
    s = s.replace("Napi::Number::New(node_gdal::napi_env(), constant)", "Napi::Number::New(target.Env(), constant)")
    n += 1
open(p, "w", encoding="utf-8").write(s)
print("reverted", n)
