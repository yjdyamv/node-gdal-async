#!/usr/bin/env python3
"""Temporary: raw napi statuses for the CPLE constants."""

p = "src/node_gdal.cpp"
s = open(p, encoding="utf-8").read()

old = """  NODE_DEFINE_CONSTANT(target, CPLE_OpenFailed);"""
new = """  {
    napi_value k = nullptr;
    napi_value v = nullptr;
    napi_status s1 = napi_create_string_utf8(env, "CPLE_OpenFailed", NAPI_AUTO_LENGTH, &k);
    napi_status s2 = napi_create_int32(env, 4, &v);
    napi_status s3 = napi_set_named_property(env, s1 == napi_ok ? target : target, "CPLE_OpenFailed", v);
    bool pend = false;
    napi_status s4 = napi_is_exception_pending(env, &pend);
    fprintf(stderr, "### st %d %d %d %d pend=%d\\n", (int)s1, (int)s2, (int)s3, (int)s4, (int)pend);
    fflush(stderr);
  }
  NODE_DEFINE_CONSTANT(target, CPLE_OpenFailed);"""

if old in s and "### st" not in s:
    s = s.replace(old, new, 1)
    open(p, "w", encoding="utf-8").write(s)
    print("probe inserted")
else:
    print("anchor missing or already present")
