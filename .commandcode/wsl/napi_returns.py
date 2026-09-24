#!/usr/bin/env python3
"""Make `return` statements match the enclosing function's signature.

The flat substitution cannot tell the two apart, so this pass walks the bodies:

  * inside NAN_METHOD / NAN_GETTER, a bare `return;` has to return the undefined
    value (a NAN method used to publish its result via info.GetReturnValue())
  * inside a constructor or a NAN_SETTER, `return;` must stay bare - those
    return void

Safe to re-run.

Usage: napi_returns.py file.cpp [...]
"""
import re
import sys

E = "node_gdal::napi_env()"
CTOR_RE = re.compile(r"^(\w+)::\1\(")
METH_RE = re.compile(r"^\s*(?:static\s+)?(NAN_METHOD|NAN_GETTER|NAN_SETTER)\(")
# the sync/async shared bodies: written as GDAL_ASYNCABLE_DEFINE(...) and
# expanding to `Napi::Value X_do(const CallbackInfo&, bool)`, so they return a value
DO_RE = re.compile(r"^\s*(?:static\s+)?(?:Napi::Value\s+\w+_do|GDAL_ASYNCABLE_(?:DEFINE|GETTER_DEFINE|TEMPLATE))\(")


def main(path):
    with open(path, encoding="utf-8") as f:
        lines = f.read().split("\n")

    out, i, changed = [], 0, 0
    while i < len(lines):
        line = lines[i]
        ctor = bool(CTOR_RE.match(line))
        setter = bool(METH_RE.match(line)) and "NAN_SETTER" in line
        method = (bool(METH_RE.match(line)) and not setter) or bool(DO_RE.match(line))

        if ctor or setter or method:
            # the signature may span lines before the body opens
            open_at = i
            while open_at < len(lines) and not lines[open_at].rstrip().endswith("{"):
                open_at += 1
                if open_at - i > 5:
                    break
            if open_at < len(lines) and lines[open_at].rstrip().endswith("{"):
                out.extend(lines[i : open_at + 1])
                depth, i = 1, open_at + 1
                while i < len(lines) and depth > 0:
                    l = lines[i]
                    depth += l.count("{") - l.count("}")
                    if depth > 0:
                        if ctor or setter:
                            new = re.sub(r"(\breturn )%s\.Undefined\(\);(\s*//.*)?$" % re.escape(E), r"return;\2", l)
                        else:
                            new = re.sub(r"(\breturn;)(\s*//.*)?$", r"return %s.Undefined();\2" % E, l)
                        changed += new != l
                        l = new
                    out.append(l)
                    i += 1
                continue
        out.append(line)
        i += 1

    with open(path, "w", encoding="utf-8") as f:
        f.write("\n".join(out))
    return changed


for p in sys.argv[1:]:
    n = main(p)
    if n:
        print("%-50s %d fixed" % (p, n))
