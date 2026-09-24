#!/usr/bin/env python3
"""Rewrite the NAN object factories (new T(x) + Nan::NewInstance) for node-addon-api.

    X *wrapped = new X(PAYLOAD);
    Napi::Value ext = Nan::New<External>(wrapped);
    Local<Object> obj = Nan::NewInstance(Nan::GetFunction(...X::constructor...), 1, &ext).ToLocalChecked();

becomes

    std::vector<napi_value> args = {Napi::External<void>::New(node_gdal::napi_env, PAYLOAD)};
    Napi::Object obj = X::constructor.Value().New(args);
    X *wrapped = node_gdal::UnwrapWrapped<X>(obj);

The External is untyped (void *) so that the factory does not have to name the
GDAL type - the class' constructor casts it back.

Usage: napi_factory.py file.cpp [...]
"""
import re
import sys

BLOCK = re.compile(
    r"[ \t]*Napi::Value ext = Nan::New<External>\((\w+)\);\s*\n"
    r"[ \t]*(?:[^\n]*?Local<[^>]*Object>|Napi::Object)\s+(\w+)\s*=\s*\n?"
    r"[ \t]*Nan::NewInstance\([^\n]*?(\w+)::constructor\)\)[^\n]*\n"
    r"(?:[ \t]*\.ToLocalChecked\(\);\n)?"
)


def fix(path):
    with open(path, encoding="utf-8") as f:
        src = f.read()

    n = 0
    for _ in range(40):
        m = BLOCK.search(src)
        if not m:
            break
        ext_var, _, cls = m.group(1), m.group(2), m.group(3)

        # the payload is what the wrapper was constructed from, just above
        ctor = re.search(r"[ \t]*%s\s*\*\s*%s\s*=\s*new\s+%s\(([^;]*)\);" % (cls, ext_var, cls), src[: m.start()])
        if not ctor:
            print("warn   %s: no `new %s(...)` for %s" % (path, cls, ext_var))
            break
        payload = ctor.group(1).strip()
        args = [a.strip() for a in re.split(r",(?![^()]*\))", payload)] if payload else []

        if len(args) == 0:
            # a collection or view: the constructor takes nothing, the factory
            # attaches the parent afterwards
            new = (
                "  std::vector<napi_value> args;\n"
                "  Napi::Object obj = %s::constructor.Value().New(args);\n" % cls
            )
        elif len(args) == 1:
            new = (
                "  std::vector<napi_value> args = {Napi::External<void>::New(node_gdal::napi_env, %s)};\n"
                "  Napi::Object obj = %s::constructor.Value().New(args);\n"
                "  %s *%s = node_gdal::UnwrapWrapped<%s>(obj);\n" % (args[0], cls, cls, ext_var, cls)
            )
        else:
            print("warn   %s: %s takes %d constructor arguments - fix by hand" % (path, cls, len(args)))
            break

        # drop everything from the `new X(...)` line through the end of the
        # ext/obj block and put the new construction in its place
        src = src[: ctor.start()] + new + src[m.end() :]
        n += 1

    if n:
        with open(path, "w", encoding="utf-8") as f:
            f.write(src)
    print("%-50s %d factories" % (path, n))


for p in sys.argv[1:]:
    fix(p)
