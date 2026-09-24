#!/usr/bin/env python3
"""Replace the placeholder prototype chaining with node_gdal::Inherit().

napi_class.py used to emit lcons.Get("prototype").SetPrototypeOf(...), but
node-addon-api's Object/Function have no SetPrototypeOf.
"""
import re
import sys

PAT = re.compile(
    r"[ \t]*// lcons->Inherit\(\) has no DefineClass equivalent[^\n]*\n"
    r"[ \t]*Napi::Function base = (\w+)::constructor\.Value\(\);\n"
    r"[ \t]*lcons\.Get\(\"prototype\"\)[^\n]*\n"
    r"[ \t]*lcons\.SetPrototypeOf\(base\);\n"
)

for path in sys.argv[1:]:
    with open(path, encoding="utf-8") as f:
        src = f.read()
    new, n = PAT.subn(
        lambda m: "  // lcons->Inherit() has no DefineClass equivalent\n"
        "  node_gdal::Inherit(lcons, %s::constructor.Value());\n" % m.group(1),
        src,
    )
    if n:
        with open(path, "w", encoding="utf-8") as f:
            f.write(new)
        print("fixed %s (%d)" % (path, n))
