#!/usr/bin/env python3
"""Mechanical NAN -> node-addon-api substitutions for the gdal-async migration.

Only context-free rewrites live here. Anything that involves control flow
(info.GetReturnValue(), Nan::ThrowError(), `->` member access on a handle,
Nan::Callback, ...) is deliberately left to the caller.

The value-constructor rules rely on a property of this code base: a NAN value
that needs .ToLocalChecked() is always created from a string (Nan::New(const char*)
returns MaybeLocal<String>), while a bare Nan::New(x) is always a number.

Usage:
    nan2napi.py [--env EXPR] file.cpp [file2.cpp ...]

--env selects the expression used for the Napi::Env: "info.Env()" (default, for
method bodies) or "env" (for module-init style code).
"""

import argparse
import re
import sys

ap = argparse.ArgumentParser()
ap.add_argument("files", nargs="+")
ap.add_argument("--env", default="info.Env()", help="Napi::Env expression")
args = ap.parse_args()
E = args.env

RULES = [
    # private properties - these must run BEFORE the value constructors below,
    # otherwise the inner Nan::New("name") is rewritten first
    (r'Nan::GetPrivate\(([^,]+),\s*Nan::New\("([^"]+)"\)\.ToLocalChecked\(\)\)', r'GDAL_GET_PRIVATE(\1, "\2")'),
    (r'Nan::SetPrivate\(([^,]+),\s*Nan::New\("([^"]+)"\)\.ToLocalChecked\(\),\s*', r'GDAL_SET_PRIVATE(\1, "\2", '),
    # value constructors
    (r"Nan::New<v8::String>\((.*?)\)\.ToLocalChecked\(\)", "Napi::String::New(%s, \\1)" % E),
    (r"Nan::New<Integer>\((.*?)\)", "Napi::Number::New(%s, \\1)" % E),
    (r"Nan::New<Number>\((.*?)\)", "Napi::Number::New(%s, \\1)" % E),
    (r"Nan::New<Boolean>\((.*?)\)", "Napi::Boolean::New(%s, \\1)" % E),
    (r"Nan::New<Object>\(\)", "Napi::Object::New(%s)" % E),
    (r"Nan::New\((true|false)\)", "Napi::Boolean::New(%s, \\1)" % E),
    (r"Nan::New\((.*?)\)\.ToLocalChecked\(\)", "Napi::String::New(%s, \\1)" % E),
    (r"Nan::New\((.*?)\)", "Napi::Number::New(%s, \\1)" % E),
    (r"Nan::Undefined\(\)", "%s.Undefined()" % E),
    (r"Nan::Null\(\)", "%s.Null()" % E),
    # registration
    (r"Nan::Set\(\s*([A-Za-z_]\w*)\s*,", "\\1.Set("),
    (r"Nan__SetAsyncableMethod\(target\s*,", "GDAL_SetAsyncableMethod(%s, target," % E),
    (r"Nan::SetMethod\(([A-Za-z_]\w*)\s*,", "GDAL_SetMethod(%s, \\1," % E),
    # a NAN job published its result through info.GetReturnValue(); a
    # node-addon-api job returns it
    (r"(?m)^(\s*)(?!return )([A-Za-z_]\w*)\.run\(", "\\1return \\2.run("),
    # arrays
    (r"Nan::New<Array>\(\)", "Napi::Array::New(%s)" % E),
    (r"Nan::New<Array>\((.*?)\)", "Napi::Array::New(%s, \\1)" % E),
    # unwrapping wrapped objects
    (r"Nan::ObjectWrap::Unwrap<(\w+)>\(info\.This\(\)\)", "node_gdal::UnwrapWrapped<\\1>(info.This().As<Napi::Object>())"),
    (r"Nan::ObjectWrap::Unwrap<(\w+)>\(([^;]*)\)", "node_gdal::UnwrapWrapped<\\1>(\\2)"),
    # member access on handles
    (r"(\w+)\[(.*?)\]->Is(\w+)\(\)", "\\1[\\2].Is\\3()"),
    (r"(\w+(?:\[[^\]]*\])?)->IsConstructCall\(\)", "\\1.IsConstructCall()"),
    (r"(\w+)->handle\(\)", "\\1->Value()"),
    # scopes - N-API manages them
    (r"(?m)^\s*Nan::EscapableHandleScope scope;\n", ""),
    (r"(?m)^\s*Nan::HandleScope scope;\n", ""),
    (r"scope\.Escape\((.*)\)", "\\1"),
    # errors and results
    (r"Nan::ThrowError\((.*?)\);", "Napi::Error::New(%s, \\1).ThrowAsJavaScriptException();" % E),
    (r"(?m)^(\s*)info\.GetReturnValue\(\)\.Set\((.*)\);$", "\\1return \\2;"),
    (r"(?m)^(\s*)return;(\s*//.*)?$", "\\1return %s.Undefined();\\2" % E),
    (r"Nan::AdjustExternalMemory\((.*?)\)", "Napi::MemoryManagement::AdjustExternalMemory(%s, \\1)" % E),
    # handle types
    (r"v8::Local<v8::Value>", "Napi::Value"),
    (r"Local<Object>", "Napi::Object"),
    (r"Local<Value>", "Napi::Value"),
    (r"Local<Array>", "Napi::Array"),
    (r"Nan::Callback", "Napi::FunctionReference"),
    # scalar conversions
    (r"Nan::To<double>\((.*?)\)\.FromJust\(\)", r"\1.As<Napi::Number>().DoubleValue()"),
    (r"Nan::To<int>\((.*?)\)\.FromJust\(\)", r"\1.As<Napi::Number>().Int32Value()"),
    (r"Nan::To<double>\((.*?)\)", r"\1.As<Napi::Number>().DoubleValue()"),
    (r"Nan::To<int>\((.*?)\)", r"\1.As<Napi::Number>().Int32Value()"),
    (r"\*Nan::Utf8String\((.*?)\)", r"\1.As<Napi::String>().Utf8Value()"),
    (r"Nan::Utf8String\((.*?)\)", r"\1.As<Napi::String>().Utf8Value()"),
    # typed errors
    (r"Nan::ThrowTypeError\((.*?)\);", "Napi::TypeError::New(%s, \\1).ThrowAsJavaScriptException();" % E),
    (r"Nan::ThrowRangeError\((.*?)\);", "Napi::RangeError::New(%s, \\1).ThrowAsJavaScriptException();" % E),
    # dates
    (r"Nan::New<Date>\((.*?)\)\.ToLocalChecked\(\)", "Napi::Date::New(%s, \\1)" % E),
    # the static constructor reference, when it has leading whitespace
    (r"(?m)^(\s*)Nan::Persistent<FunctionTemplate> (\w+)::constructor;", r"\1Napi::FunctionReference \2::constructor;"),
]

total = 0
for path in args.files:
    with open(path, encoding="utf-8") as f:
        src = f.read()
    out = src
    n = 0
    for pat, rep in RULES:
        out, k = re.subn(pat, rep, out)
        n += k
    if out != src:
        with open(path, "w", encoding="utf-8") as f:
            f.write(out)
    print("%-45s %4d substitutions" % (path, n))
    total += n
print("total: %d" % total)
