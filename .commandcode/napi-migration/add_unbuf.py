#!/usr/bin/env python3
"""Temporary: unbuffered stderr so diagnostics survive the segfault."""

n = 0

p = "src/node_gdal.cpp"
s = open(p, encoding="utf-8").read()
old = '  fprintf(stderr, "### GDAL Init\\n");\n'
if old in s and "setvbuf" not in s:
    s = s.replace(old, '  setvbuf(stderr, nullptr, _IONBF, 0);\n' + old, 1)
    n += 1
# flush after the catch print too
old2 = '    fprintf(stderr, "### Init threw: %s\\n", e.Message().c_str());\n'
if old2 in s:
    s = s.replace(old2, old2 + '    fflush(stderr);\n', 1)
    n += 1
open(p, "w", encoding="utf-8").write(s)
print("patched", n)
