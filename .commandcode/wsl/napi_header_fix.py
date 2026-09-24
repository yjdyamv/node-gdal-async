#!/usr/bin/env python3
"""Repair pass for napi_header.py output.

A class that declared both `X();` and `X(SomePtr *);` ended up with two identical
`X(const Napi::CallbackInfo &info);` lines, because both forms map onto the same
N-API signature. Keep one. Also tidies the blank lines the destructor insertion
used to leave behind.

Usage: napi_header_fix.py file.hpp [...]
"""
import re
import sys

for path in sys.argv[1:]:
    with open(path, encoding="utf-8") as f:
        src = f.read()

    m = re.search(r"^class\s+(\w+)\s*:\s*public\s+Napi::ObjectWrap<", src, re.M)
    if not m:
        print("skip   %s" % path)
        continue
    name = m.group(1)
    ctor = "%s(const Napi::CallbackInfo &info);" % name

    out, seen = [], False
    for line in src.split("\n"):
        if line.strip() == ctor:
            if seen:
                continue
            seen = True
        out.append(line)
    src = "\n".join(out)

    src = re.sub(
        r"(\n[ \t]*// Must be accessible: ObjectWrap's finalizer deletes the instance itself)\n\n+", r"\1\n", src
    )
    src = re.sub(r"\n{3,}", "\n\n", src)

    with open(path, "w", encoding="utf-8") as f:
        f.write(src)
    print("fix    %s %s" % (path, name))
