#!/usr/bin/env python3
"""Step: mark the module-registration wrapper to see where the throw lands."""

p = "src/node_gdal.cpp"
s = open(p, encoding="utf-8").read()

old = """static Napi::Object GDALInit(Napi::Env env, Napi::Object target) {
  return node_gdal::Init(env, target);
}"""
new = """static Napi::Object GDALInit(Napi::Env env, Napi::Object target) {
  fprintf(stderr, "W: enter\\n");
  fflush(stderr);
  Napi::Object r = node_gdal::Init(env, target);
  fprintf(stderr, "W: init returned\\n");
  fflush(stderr);
  return r;
}"""

if old in s:
    s = s.replace(old, new, 1)
    open(p, "w", encoding="utf-8").write(s)
    print("wrapper instrumented")
else:
    print("anchor not found")
