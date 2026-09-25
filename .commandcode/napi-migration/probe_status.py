#!/usr/bin/env python3
"""Temporary: probe the raw napi_status of the last Init step; drop the ctor print."""

n = 0

p = "src/collections/gdal_drivers.cpp"
s = open(p, encoding="utf-8").read()
line = '  fprintf(stderr, "### ctor len=%d t=%d\\n", (int)info.Length(), info.Length() > 0 ? (int)info[0].Type() : -1);\n'
if line in s:
    s = s.replace(line, '', 1)
    s = s.replace('  fflush(stderr);\n', '')
    n += 1
open(p, "w", encoding="utf-8").write(s)

p = "src/node_gdal.cpp"
s = open(p, encoding="utf-8").read()
old = """  napi_add_env_cleanup_hook(env, Cleanup, nullptr);
}
}"""
new = """  {
    napi_value k = nullptr;
    napi_value v = nullptr;
    napi_status s1 = napi_create_string_utf8(env, "CPLE_OpenFailed", NAPI_AUTO_LENGTH, &k);
    napi_status s2 = napi_create_int32(env, 4, &v);
    napi_status s3 = napi_set_named_property(env, target, "CPLE_OpenFailed", v);
    napi_status s4 = napi_add_env_cleanup_hook(env, Cleanup, nullptr);
    fprintf(stderr, "### st %d %d %d %d\\n", (int)s1, (int)s2, (int)s3, (int)s4);
    fflush(stderr);
  }
}
}"""
if old in s:
    s = s.replace(old, new, 1)
    n += 1
open(p, "w", encoding="utf-8").write(s)

print("patched", n)
