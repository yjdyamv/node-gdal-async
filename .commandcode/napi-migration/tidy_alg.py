#!/usr/bin/env python3
"""Tidy the whitespace the diagnostics left behind."""

import re

for p in ["src/gdal_algorithms.cpp", "src/node_gdal.cpp"]:
    s = open(p, encoding="utf-8").read()
    s = "\n".join(l.rstrip() for l in s.split("\n"))
    s = re.sub(r"\n{3,}", "\n\n", s)
    open(p, "w", encoding="utf-8").write(s)
    print("tidied", p)
