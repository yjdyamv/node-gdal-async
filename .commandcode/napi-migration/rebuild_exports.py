#!/usr/bin/env python3
"""Let a repeated registration rebuild the exports instead of returning an empty object."""

p = "src/node_gdal.cpp"
s = open(p, encoding="utf-8").read()

old = """// The addon can be pulled in twice by the same process: the CJS and the ESM
// loader both resolve it. The second registration gets a fresh, empty exports
// object, so the one built by the first call is what has to be handed back.
static Napi::ObjectReference *g_exports = nullptr;

Napi::Object Init(Napi::Env env, Napi::Object target) {
  if (g_exports != nullptr) {
    return g_exports->Value().As<Napi::Object>();
  }"""

new = """Napi::Object Init(Napi::Env env, Napi::Object target) {
  // Note: the CJS and the ESM loader can both register the addon in the same
  // process. Each call gets its own empty exports object, so the registration
  // has to run every time - a guard here would hand the second one back empty."""

if old in s:
    s = s.replace(old, new, 1)
    n = 1
else:
    n = 0

s = s.replace("""  g_exports = new Napi::ObjectReference(Napi::Persistent(target));
""", "")

open(p, "w", encoding="utf-8").write(s)
print("patched", n)
