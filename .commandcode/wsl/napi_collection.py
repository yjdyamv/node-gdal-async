#!/usr/bin/env python3
"""Convert a collection's JS constructor.

A collection is created by its parent (`dataset.bands`) and is not constructible
directly - test/api_classes.test.ts asserts that `new gdal.DatasetBands()`
throws. NAN built the wrapper first and handed it over through an External; now
the parent is passed as an ordinary constructor argument, which doubles as the
guard.

Per file:

  * the old `NAN_METHOD(X::New)` body goes away (the constructor replaces it),
  * the constructor validates its single argument and records the parent,
  * the factory passes the parent instead of setting the property itself.

Usage: napi_collection.py file.cpp [...]
"""
import re
import sys


def find_block(lines, start):
    depth, opened = 0, False
    for i in range(start, len(lines)):
        for ch in lines[i]:
            if ch == "{":
                depth += 1
                opened = True
            elif ch == "}":
                depth -= 1
                if opened and depth == 0:
                    return i
    raise SystemExit("unbalanced braces")


for path in sys.argv[1:]:
    with open(path, encoding="utf-8") as f:
        src = f.read()
    lines = src.split("\n")

    m = re.search(r"^NAN_METHOD\((\w+)::New\) \{", src, re.M)
    if not m:
        print("skip   %s" % path)
        continue
    cls = m.group(1)

    # the private property the factory used to attach
    pm = re.search(r'GDAL_SET_PRIVATE\(obj,\s*"([^"]+)",\s*([^)]+)\);', src)
    if not pm:
        print("warn   %s: no GDAL_SET_PRIVATE in the factory" % path)
        continue

    # only the members whose factory builds them from nothing; the ones carrying
    # a GDAL object through an External keep the object as their first argument
    if "External<void>" in src or "Nan::NewInstance" in src:
        print("skip   %s: takes a payload" % path)
        continue

    key, parent = pm.group(1), pm.group(2).strip()

    # 1. drop the old JS constructor
    start = next(i for i, l in enumerate(lines) if re.match(r"^NAN_METHOD\(%s::New\) \{" % cls, l))
    end = find_block(lines, start)
    lines[start : end + 1] = []
    src = "\n".join(lines)

    # 2. the factory passes the parent, and no longer sets the property
    src = src.replace("  GDAL_SET_PRIVATE(obj, \"%s\", %s);\n" % (key, parent), "")
    src = src.replace("  std::vector<napi_value> args;", "  std::vector<napi_value> args = {%s};" % parent)

    # 3. the constructor validates and records it
    src = re.sub(
        r"(%s::%s\(const Napi::CallbackInfo &info\) : GDALObject<%s>\(info\) \{\n)\}"
        % (cls, cls, cls),
        r"""\1  if (info.Length() < 1 || !info[0].IsObject()) {
    Napi::Error::New(info.Env(), "Cannot create %s directly").ThrowAsJavaScriptException();
    return;
  }
  GDAL_SET_PRIVATE(info.This(), "%s", info[0]);
}""" % (cls, key),
        src,
    )

    with open(path, "w", encoding="utf-8") as f:
        f.write(src)
    print("ok     %s (%s, parent=%s)" % (path, cls, parent))
