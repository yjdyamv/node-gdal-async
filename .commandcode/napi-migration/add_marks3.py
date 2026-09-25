#!/usr/bin/env python3
"""Temporary: quarter markers through the long constant table in Init."""

p = "src/node_gdal.cpp"
lines = open(p, encoding="utf-8").read().split("\n")

for ln, tag in [(1700, "### Q4"), (1400, "### Q3"), (1100, "### Q2"), (800, "### Q1")]:
    idx = ln - 1
    if tag not in "\n".join(lines[idx - 2:idx + 2]):
        lines.insert(idx, '  fprintf(stderr, "%s\\n");' % tag)

open(p, "w", encoding="utf-8").write("\n".join(lines))
print("ok")
