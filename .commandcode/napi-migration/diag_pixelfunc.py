#!/usr/bin/env python3
"""Temporary: print what GDAL says about our pixel function metadata."""

p = "src/gdal_algorithms.cpp"
s = open(p, encoding="utf-8").read()

old = """  if (err != CE_None) { NODE_THROW_LAST_CPLERR; }"""
new = """  if (err != CE_None) {
    fprintf(stderr, "### addPixelFunc: gdal err=%d msg=[%s]\\n### metadata=[%s]\\n", (int)err,
            CPLGetLastErrorMsg() ? CPLGetLastErrorMsg() : "(null)", desc->metadata);
    fflush(stderr);
    NODE_THROW_LAST_CPLERR;
  }"""

if old in s and "addPixelFunc: gdal err" not in s:
    s = s.replace(old, new, 1)
    open(p, "w", encoding="utf-8").write(s)
    print("diagnostic added")
else:
    print("no change")
