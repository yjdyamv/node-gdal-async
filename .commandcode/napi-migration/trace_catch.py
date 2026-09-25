#!/usr/bin/env python3
"""Step: print a C++ backtrace from the catch around Init."""

p = "src/node_gdal.cpp"
s = open(p, encoding="utf-8").read()

old = """static Napi::Object GDALInit(Napi::Env env, Napi::Object target) {
  fprintf(stderr, "W: enter\\n");
  fflush(stderr);
  Napi::Object r = node_gdal::Init(env, target);
  fprintf(stderr, "W: init returned\\n");
  fflush(stderr);
  return r;
}"""
new = """static Napi::Object GDALInit(Napi::Env env, Napi::Object target) {
  fprintf(stderr, "W: enter\\n");
  fflush(stderr);
  try {
    Napi::Object r = node_gdal::Init(env, target);
    fprintf(stderr, "W: init returned\\n");
    fflush(stderr);
    return r;
  } catch (const Napi::Error &e) {
    void *frames[48];
    int n = backtrace(frames, 48);
    fprintf(stderr, "### Init threw: %s\\n", e.Message().c_str());
    backtrace_symbols_fd(frames, n, 2);
    fflush(stderr);
    return target;
  }
}"""

if old in s:
    s = s.replace(old, new, 1)

if "#include <execinfo.h>" not in s:
    s = s.replace('#include "napi-wrapper.h"', '#include <execinfo.h>\n#include "napi-wrapper.h"', 1)

open(p, "w", encoding="utf-8").write(s)
print("instrumented with backtrace")
