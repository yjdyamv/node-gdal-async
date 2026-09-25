#!/usr/bin/env python3
"""Temporary: phase markers through Init."""

p = "src/node_gdal.cpp"
s = open(p, encoding="utf-8").read()

marks = [
    ('  GDAL_SetMethod(env, target, "_isAlive", isAlive);                    // for tests\n', "A"),
    ('  Layer::Initialize(target);\n', "B"),
    ('  DatasetBands::Initialize(target);\n', "C"),
    ('  GDALDrivers::Initialize(target); // calls GDALRegisterAll()\n', "D"),
    ('  Napi::Object supports = Napi::Object::New(env);\n', "E"),
    ('  napi_add_env_cleanup_hook(env, Cleanup, nullptr);\n', "F"),
]

found = []
for anchor, tag in marks:
    if anchor in s and ('### %s\\n' % tag) not in s:
        s = s.replace(anchor, anchor + '  fprintf(stderr, "### %s\\n");\n  fflush(stderr);\n' % tag, 1)
        found.append(tag)

open(p, "w", encoding="utf-8").write(s)
print("markers:", found)
