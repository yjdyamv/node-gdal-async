#!/usr/bin/env python3
"""Fold a class' NAN constructors into the single node-addon-api one.

Upstream had `X()` and `X(GDALX *)`, plus a `NAN_METHOD(X::New)` that adopted the
wrapper the factory had already built. node-addon-api allocates the instance
itself and napi_factory.py hands the GDAL object over through an untyped
External<void>, so all of it collapses into:

    X::X(const Napi::CallbackInfo &info)
      : GDALObject<X>(info), <old init list, payload null-initialised> {
      <old constructor body>
      if (info.Length() > 0 && info[0].IsExternal()) {
        <member> = static_cast<TYPE *>(info[0].As<Napi::External<void>>().Data());
        return;
      }
      <how the old JS constructor refused a direct construction>
    }

Usage: napi_ctor.py file.cpp [...]
"""
import re
import sys

HEAD = re.compile(r"^(\w+)::(\w+)\(")
REFUSE = re.compile(r"^(\s*Napi::Error::New\([^;]*\"Cannot create [^\"]*\"\)[^;]*;)$", re.M)
PARAM = re.compile(r"^([\w:]+(?:\s*<[^>]*>)?)\s*\*\s*(\w+)$")


def block_end(lines, start):
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


def convert(path):
    with open(path, encoding="utf-8") as f:
        lines = f.read().split("\n")

    ctors = []
    for i, line in enumerate(lines):
        m = HEAD.match(line)
        if not m or m.group(1) != m.group(2):
            continue
        j = i
        while j < len(lines) and not lines[j].rstrip().endswith("{"):
            j += 1
            if j - i > 6:
                break
        if j >= len(lines) or not lines[j].rstrip().endswith("{"):
            continue
        sig = "\n".join(lines[i : j + 1])
        if "Nan::ObjectWrap()" in sig:
            ctors.append((i, j, sig))
    if not ctors:
        print("skip   %s" % path)
        return

    cls = HEAD.match(lines[ctors[0][0]]).group(1)

    # the payload constructor: `X(TYPE *name) : Nan::ObjectWrap(), ..., member(name), ...`
    pick = None
    for i, j, sig in ctors:
        params = sig.split("(", 1)[1].split(")", 1)[0]
        pm = PARAM.match(params.split(",")[0].strip())
        if pm:
            pick = (i, j, sig, pm.group(1), pm.group(2))
            break
    if pick is None:
        print("MANUAL %s: no `X(TYPE *name)` constructor" % path)
        return
    i, j, sig, ptype, pname = pick

    inits = sig.split("Nan::ObjectWrap()", 1)[1].rstrip()
    if inits.endswith("{"):
        inits = inits[:-1].rstrip()
    member = re.search(r"(\w+)\(\s*%s\s*\)" % pname, inits)
    if not member:
        print("MANUAL %s: cannot tell which member takes the payload" % path)
        return
    member = member.group(1)
    inits = re.sub(r"\(\s*%s\s*\)" % pname, "(nullptr)", inits)

    end = block_end(lines, i)
    body = lines[i + 1 : end]

    # how the old JS constructor refused direct construction
    js = re.search(r"^\s*NAN_METHOD\(%s::New\) \{" % cls, "\n".join(lines), re.M)
    refuse = None
    js_range = ()
    if js:
        js_start = next(k for k in range(len(lines)) if re.match(r"^\s*NAN_METHOD\(%s::New\) \{" % cls, lines[k]))
        js_end = block_end(lines, js_start)
        js_range = (js_start, js_end)
        rm = REFUSE.search("\n".join(lines[js_start : js_end + 1]))
        if rm:
            refuse = rm.group(1).strip()
    if refuse is None:
        if js:
            print("MANUAL %s: the JS constructor does more than refuse" % path)
            return
        refuse = 'Napi::Error::New(info.Env(), "Cannot create %s directly").ThrowAsJavaScriptException();' % cls

    drop = set(range(i, end + 1)) | set(range(js_range[0], js_range[1] + 1)) if js_range else set(range(i, end + 1))
    keep = [l for k, l in enumerate(lines) if k not in drop]

    at = next(k for k, l in enumerate(keep) if l.startswith("%s::~%s(" % (cls, cls)))
    new = ["%s::%s(const Napi::CallbackInfo &info) : GDALObject<%s>(info)%s {" % (cls, cls, cls, inits)]
    new += body
    new += [
        "  if (info.Length() > 0 && info[0].IsExternal()) {",
        "    %s = static_cast<%s *>(info[0].As<Napi::External<void>>().Data());" % (member, ptype),
        "    return;",
        "  }",
        "  %s" % refuse,
        "}",
        "",
    ]
    keep[at:at] = new

    with open(path, "w", encoding="utf-8") as f:
        f.write("\n".join(keep))
    print("ok     %s (%s: %s <- %s)" % (path, cls, member, pname))


for p in sys.argv[1:]:
    convert(p)
