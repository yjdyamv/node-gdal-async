#!/usr/bin/env python3
"""Temporary: test the CPLE constant with the function's env instead of target.Env()."""

p = "src/node_gdal.cpp"
s = open(p, encoding="utf-8").read()
old = '  TRY_STMT("CPLE_OpenFailed", NODE_DEFINE_CONSTANT(target, CPLE_OpenFailed);)'
new = '  TRY_STMT("CPLE_OpenFailed", target.Set("CPLE_OpenFailed", Napi::Number::New(env, 4));)'
if old in s:
    s = s.replace(old, new, 1)
    open(p, "w", encoding="utf-8").write(s)
    print("swapped to explicit env")
else:
    print("no change")
