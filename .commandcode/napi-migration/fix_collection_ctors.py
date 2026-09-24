#!/usr/bin/env python3
"""Add `using GroupCollection<...>::GroupCollection;` to the concrete group/array
collection headers.

The concrete classes do not declare a constructor of their own, and
GroupCollection's constructor now has to be `GroupCollection(const Napi::CallbackInfo&)`
for node-addon-api (it does `new T(callbackInfo)`). Without the inherited
constructor the concrete class has no such constructor at all.
"""
import glob
import re
import sys

for path in glob.glob("src/collections/*.hpp"):
    with open(path, encoding="utf-8") as f:
        src = f.read()
    if "GroupCollection<" not in src or "using GroupCollection<" in src:
        continue
    new, n = re.subn(
        r"^(class\s+\w+\s*:\s*public\s+(GroupCollection<[^>]*>)\s*\{\n\s+public:)\n",
        r"\1\n  using \2::GroupCollection;\n",
        src,
        flags=re.M,
    )
    if n:
        with open(path, "w", encoding="utf-8") as f:
            f.write(new)
        print("patched %s" % path)
    else:
        print("no match %s" % path)
