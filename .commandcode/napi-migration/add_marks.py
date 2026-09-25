#!/usr/bin/env python3
"""Temporary: bisect the Init-time pending exception."""

p = "src/node_gdal.cpp"
s = open(p, encoding="utf-8").read()

marks = [
    ('  GDAL_SetMethod(env, target, "_isAlive", isAlive);                    // for tests\n',
     '### A'),
    ('  Layer::Initialize(target);\n', '### B'),
    ('  DatasetBands::Initialize(target);\n', '### C'),
]

for anchor, tag in marks:
    if anchor in s and tag not in s:
        s = s.replace(anchor, anchor + '  fprintf(stderr, "%s\\n");\n' % tag, 1)

open(p, "w", encoding="utf-8").write(s)
print("markers:", [t for _, t in marks if t in s])
