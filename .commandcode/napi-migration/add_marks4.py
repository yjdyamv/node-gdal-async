#!/usr/bin/env python3
"""Temporary: pin the tail statement that leaves a pending exception."""

p = "src/node_gdal.cpp"
s = open(p, encoding="utf-8").read()

marks = [
    ('  GDAL_DEFINE_ACCESSOR(target, "lastError", LastErrorGetter, LastErrorSetter);\n', "### T1"),
    ('  GDAL_DEFINE_ACCESSOR(target, "eventLoopWarning", EventLoopWarningGetter, EventLoopWarningSetter);\n', "### T2"),
    ('  GDAL_SetMethod(env, target, "log", Log);\n', "### T3"),
    ('  target.Set( Napi::String::New(env, "supports"), supports);\n', "### T4"),
    ('  NODE_DEFINE_CONSTANT(target, CPLE_OpenFailed);\n', "### T5"),
]

for anchor, tag in marks:
    if anchor in s and tag not in s:
        s = s.replace(anchor, anchor + '  fprintf(stderr, "%s\\n");\n' % tag, 1)

open(p, "w", encoding="utf-8").write(s)
print("ok")
