#!/usr/bin/env python3
"""Hand back the exports object the first Init actually populated."""

p = "src/node_gdal.cpp"
s = open(p, encoding="utf-8").read()

old = """Napi::Object Init(Napi::Env env, Napi::Object target) {
  static bool initialized = false;
  if (initialized) {
    // The addon can be pulled in twice by the same process (the CJS and the ESM
    // loader both resolve it). The exports are already built, so hand them back.
    return target;
  }
  initialized = true;"""

new = """// The addon can be pulled in twice by the same process: the CJS and the ESM
// loader both resolve it. The second registration gets a fresh, empty exports
// object, so the one built by the first call is what has to be handed back.
static Napi::ObjectReference *g_exports = nullptr;

Napi::Object Init(Napi::Env env, Napi::Object target) {
  if (g_exports != nullptr) {
    return g_exports->Value().As<Napi::Object>();
  }"""

if old in s:
    s = s.replace(old, new, 1)
    n = 1
else:
    n = 0

# record the built exports at the end of the body
old2 = """  napi_add_env_cleanup_hook(env, Cleanup, nullptr);
}
}"""
new2 = """  napi_add_env_cleanup_hook(env, Cleanup, nullptr);

  g_exports = new Napi::ObjectReference(Napi::Persistent(target));
}
}"""
if old2 in s:
    s = s.replace(old2, new2, 1)
    n += 1

open(p, "w", encoding="utf-8").write(s)
print("patched", n)
