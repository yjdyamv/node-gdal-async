#!/usr/bin/env python3
"""Drop the diagnostics, keep the real fixes."""

import re

n = 0

p = "src/node_gdal.cpp"
s = open(p, encoding="utf-8").read()

# the peek instrumentation
old = """static Napi::Object GDALInit(Napi::Env env, Napi::Object target) {
  Napi::Object r = node_gdal::Init(env, target);
  bool pending = false;
  if (napi_is_exception_pending(env, &pending) == napi_ok && pending) {
    napi_value ex = nullptr;
    napi_get_and_clear_last_exception(env, &ex);
    napi_value str = nullptr;
    napi_coerce_to_string(env, ex, &str);
    std::string s = Napi::String(env, str).Utf8Value();
    fprintf(stderr, "### pending: %s\\n", s.c_str());
    fflush(stderr);
  }
  return r;
}"""
new = """static Napi::Object GDALInit(Napi::Env env, Napi::Object target) {
  return node_gdal::Init(env, target);
}"""
if old in s:
    s = s.replace(old, new, 1)
    n += 1

s = re.sub(r'^ *fprintf\(stderr, "### [^"]*\\n"\);\n', '', s, flags=re.M)
s = s.replace('  fflush(stderr);\n', '')
s = s.replace('  setvbuf(stderr, nullptr, _IONBF, 0);\n', '')
open(p, "w", encoding="utf-8").write(s)

p = "src/collections/gdal_drivers.cpp"
s = open(p, encoding="utf-8").read()
s = re.sub(r'^ *fprintf\(stderr, "### [^"]*\\n"\);\n', '', s, flags=re.M)
s = s.replace('  fflush(stderr);\n', '')
open(p, "w", encoding="utf-8").write(s)

print("cleaned", n, "; ### left:", open(p).read().count("###"))
