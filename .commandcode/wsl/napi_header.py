#!/usr/bin/env python3
"""Header conversion for the N-API migration (plain ObjectWrap classes).

Handles the mechanical shape of a class header:

  * drops <node.h> / <node_object_wrap.h> includes and the `using namespace v8/node`
  * `#include "nan-wrapper.h"` -> `#include "napi-wrapper.h"`
  * `class X : public Nan::ObjectWrap` -> `class X : public Napi::ObjectWrap<X>`
  * `static Nan::Persistent<FunctionTemplate> constructor;` -> `static Napi::FunctionReference constructor;`
  * `static void Initialize(Local<Object> target);` -> `static void Initialize(Napi::Object target);`
  * drops `static NAN_METHOD(New);` - ObjectWrap's own constructor is the JS constructor
  * `Local<Value>`/`Local<Object>` -> `Napi::Value`/`Napi::Object`
  * `Nan::Callback` -> `Napi::FunctionReference`
  * `X(); X(Y *);` -> `X(const Napi::CallbackInfo &info);`
  * `~X();` moves into the public section, right after the constructor: ObjectWrap's
    finalizer does `delete static_cast<T *>(instance)`, so it cannot be private

Deliberately NOT covered (handled by hand): the geometry/ CRTP hierarchy,
collections/group_collection.hpp, and inline method bodies that still use NAN.

Usage: napi_header.py file.hpp [...]
"""
import re
import sys

DROP = ("#include <node.h>", "#include <node_object_wrap.h>", "using namespace v8;", "using namespace node;")

for path in sys.argv[1:]:
    with open(path, encoding="utf-8") as f:
        src = f.read()

    m = re.search(r"^class\s+(\w+)\s*:\s*public\s+Nan::ObjectWrap", src, re.M)
    name = m.group(1) if m else None

    # The class headers got NAN_METHOD/NAN_GETTER from nan.h; in the N-API layer
    # those come from gdal_common.hpp (which in turn includes napi-wrapper.h).
    # Pure type-only headers (utils/ptr_manager.hpp, utils/string_list.hpp) keep
    # napi-wrapper.h - they are not processed by this script.
    src = src.replace('#include "nan-wrapper.h"', '#include "gdal_common.hpp"')
    src = src.replace('#include "../nan-wrapper.h"', '#include "../gdal_common.hpp"')
    src = "\n".join(l for l in src.split("\n") if l.strip() not in DROP)

    if name:
        src = re.sub(
            r"class\s+%s\s*:\s*public\s+Nan::ObjectWrap\b" % name,
            "class %s : public GDALObject<%s>" % (name, name),
            src,
        )
        src = src.replace(
            "static void Initialize(Local<Object> target);", "static void Initialize(Napi::Object target);"
        )

    # the JS constructor is ObjectWrap's own constructor now
    src = re.sub(r"^\s*static NAN_METHOD\(New\);\s*\n", "", src, flags=re.M)

    src = src.replace("static Nan::Persistent<FunctionTemplate> constructor;", "static Napi::FunctionReference constructor;")
    src = src.replace("Local<Value>", "Napi::Value").replace("Local<Object>", "Napi::Object")
    src = src.replace("v8::Local<v8::Value>", "Napi::Value").replace("v8::Local<v8::Object>", "Napi::Object")
    src = src.replace("Nan::ADDON_REGISTER_FUNCTION_ARGS_TYPE", "Napi::Object")
    src = src.replace("Nan::Callback", "Napi::FunctionReference")

    if name:
        had_dtor = bool(re.search(r"^\s*~%s\(\);\s*$" % name, src, re.M))
        if had_dtor:
            src = re.sub(r"^\s*~%s\(\);\s*\n" % name, "", src, flags=re.M)

        def ctor(mo):
            return "%s%s(const Napi::CallbackInfo &info);" % (mo.group(1), name)

        src = re.sub(r"^(\s*)%s\(\);\s*$" % name, ctor, src, flags=re.M)
        src = re.sub(r"^(\s*)%s\((?!const Napi::CallbackInfo)[^)]*\);\s*$" % name, ctor, src, flags=re.M)

        # a class may declare both X() and X(SomePtr *) - both map onto the same
        # N-API signature, keep a single one
        out, seen = [], False
        for line in src.split("\n"):
            if line.strip() == "%s(const Napi::CallbackInfo &info);" % name:
                if seen:
                    continue
                seen = True
            out.append(line)
        src = "\n".join(out)

        if had_dtor:
            mo = re.search(r"^(\s*)%s\(const Napi::CallbackInfo &info\);$" % name, src, re.M)
            if mo:
                indent, end = mo.group(1), mo.end()
                ins = (
                    "\n%s// Must be accessible: ObjectWrap's finalizer deletes the instance itself"
                    "\n%s~%s();" % (indent, indent, name)
                )
                src = src[:end] + ins + src[end:]

    with open(path, "w", encoding="utf-8") as f:
        f.write(src)
    print("%-50s %s" % (path, name or "(includes only)"))
