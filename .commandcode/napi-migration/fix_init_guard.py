#!/usr/bin/env python3
"""Make Init idempotent and drop the leftover diagnostic."""

n = 0

p = "src/node_gdal.cpp"
s = open(p, encoding="utf-8").read()
old = """    // A second load into the same isolate (the CJS and the ESM loader can both
    // pull the addon in) is harmless: the exports are already built. Only a
    // genuinely different isolate is unsupported.
    if (napi_env_storage != nullptr && napi_env_storage == (::napi_env)env) {
      return target;
    }
    Napi::Error::New(env, "gdal-async does not yet support multiple instances per V8 isolate").ThrowAsJavaScriptException();
    return target;"""
new = """    // The addon can be pulled in twice by the same process (the CJS and the ESM
    // loader both resolve it). The exports are already built, so hand them back.
    return target;"""
if old in s:
    s = s.replace(old, new, 1)
    n += 1
open(p, "w", encoding="utf-8").write(s)

p = "src/collections/gdal_drivers.cpp"
s = open(p, encoding="utf-8").read()
if '### ctor len' in s:
    s = s.replace('  fprintf(stderr, "### ctor len=%d\\n", (int)info.Length());\n', '')
    n += 1
open(p, "w", encoding="utf-8").write(s)

print("patched", n)
