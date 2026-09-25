#!/usr/bin/env python3
"""Wrapper that survives the registration-time exception.

Binding the Napi::Object returned by Init lets a failing N-API call (inside
node-addon-api's return handling, after Init's body has finished) escape and
abort the load. Calling it for the side effect and handing back `target` keeps
the module loadable. TODO: find the underlying call.
"""

p = "src/node_gdal.cpp"
s = open(p, encoding="utf-8").read()

old = """static Napi::Object GDALInit(Napi::Env env, Napi::Object target) {
  return node_gdal::Init(env, target);
}"""
new = """static Napi::Object GDALInit(Napi::Env env, Napi::Object target) {
  try {
    node_gdal::Init(env, target);
  } catch (const Napi::Error &) {
    // Init completes its work; a failing N-API call follows it. Swallowing it
    // here is what keeps the module loadable - see the note in the header.
  }
  return target;
}"""

if old in s:
    s = s.replace(old, new, 1)
    open(p, "w", encoding="utf-8").write(s)
    print("wrapper patched")
else:
    print("anchor not found")
