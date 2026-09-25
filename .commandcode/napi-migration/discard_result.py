#!/usr/bin/env python3
"""Step: call Init without binding its returned Napi::Object."""

p = "src/node_gdal.cpp"
s = open(p, encoding="utf-8").read()

old = """    Napi::Object r = node_gdal::Init(env, target);
    fprintf(stderr, "W: init returned\\n");
    fflush(stderr);
    return r;"""
new = """    node_gdal::Init(env, target);
    fprintf(stderr, "W: init returned\\n");
    fflush(stderr);
    return target;"""

if old in s:
    s = s.replace(old, new, 1)
    open(p, "w", encoding="utf-8").write(s)
    print("result discarded")
else:
    print("anchor not found")
