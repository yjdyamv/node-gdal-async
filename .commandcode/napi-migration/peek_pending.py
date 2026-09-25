#!/usr/bin/env python3
"""Temporary: surface the real pending exception at module load."""

p = "src/node_gdal.cpp"
s = open(p, encoding="utf-8").read()

old = """static Napi::Object GDALInit(Napi::Env env, Napi::Object target) {
  return node_gdal::Init(env, target);
}"""

new = """static Napi::Object GDALInit(Napi::Env env, Napi::Object target) {
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

if old in s:
    s = s.replace(old, new, 1)
    open(p, "w", encoding="utf-8").write(s)
    print("instrumented")
else:
    print("anchor not found")
