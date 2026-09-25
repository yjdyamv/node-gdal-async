#!/usr/bin/env python3
"""Keep the `_VOID` macro spelling only inside void functions.

`NODE_UNWRAP_CHECK`/`GDAL_LOCK_PARENT` return a value on error, which a setter
cannot do, so the setters use the `_VOID` variants. A plain textual swap is too
broad - it also hits the value-returning methods - so this walks back to the
enclosing function and reverts the ones that are not setters.

Usage: fix_void_macros.py file.cpp [...]
"""
import re
import sys

VOID_USE = re.compile(r"\b(NODE_UNWRAP_CHECK|GDAL_LOCK_PARENT)_VOID\(")
PLAIN_USE = re.compile(r"\b(NODE_UNWRAP_CHECK|GDAL_LOCK_PARENT)\(")
FUNC = re.compile(r"^\s*(?:static\s+)?(NAN_METHOD|NAN_GETTER|NAN_SETTER)\(|^GDAL_ASYNCABLE_|^Napi::Value\s|^void\s")


def enclosing_is_setter(lines, index):
    for i in range(index, -1, -1):
        if FUNC.search(lines[i]):
            return "NAN_SETTER" in lines[i]
    return False


for path in sys.argv[1:]:
    with open(path, encoding="utf-8") as f:
        lines = f.read().split("\n")
    n = 0
    for i, line in enumerate(lines):
        setter = enclosing_is_setter(lines, i)
        if VOID_USE.search(line) and not setter:
            lines[i] = VOID_USE.sub(lambda m: m.group(1) + "(", line)
            n += 1
        elif PLAIN_USE.search(line) and setter:
            lines[i] = PLAIN_USE.sub(lambda m: m.group(1) + "_VOID(", line)
            n += 1
    if n:
        with open(path, "w", encoding="utf-8") as f:
            f.write("\n".join(lines))
    print("%-46s %d reverted" % (path, n))
