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
    # same, for files where the inner Nan::New was rewritten by an earlier run
    (r'Nan::GetPrivate\(([^,]+),\s*Napi::String::New\([^,]+,\s*"([^"]+)"\)\)', r'GDAL_GET_PRIVATE(\1, "\2")'),
    (r'Nan::SetPrivate\(([^,]+),\s*Napi::String::New\([^,]+,\s*"([^"]+)"\),\s*', r'GDAL_SET_PRIVATE(\1, "\2", '),
    # value constructors
    (r"Nan::New<v8::String>\((.*?)\)\.ToLocalChecked\(\)", "Napi::String::New(%s, \\1)" % E),
    (r"Nan::New<Integer>\((.*?)\)", "Napi::Number::New(%s, \\1)" % E),
    (r"Nan::New<String>\((.*?)\)", "Napi::String::New(%s, \\1)" % E),
    (r"Nan::New<Number>\((.*?)\)", "Napi::Number::New(%s, \\1)" % E),
    (r"Nan::New<Boolean>\((.*?)\)", "Napi::Boolean::New(%s, \\1)" % E),
    (r"Nan::New<Object>\(\)", "Napi::Object::New(%s)" % E),
    (r"Nan::New\((true|false)\)", "Napi::Boolean::New(%s, \\1)" % E),
    (r"Nan::True\(\)", "Napi::Boolean::New(%s, true)" % E),
    (r"Nan::False\(\)", "Napi::Boolean::New(%s, false)" % E),
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
    # Nan::ObjectWrap::persistent() was the wrapper's own handle; ObjectWrap *is*
    # a Reference<Object> in node-addon-api
    (r"(\w+)->persistent\(\)", r"*\1"),
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
    (r"v8::Local<v8::Object>", "Napi::Object"),
    (r"v8::Local<v8::Array>", "Napi::Array"),
    (r"Local<Object>", "Napi::Object"),
    (r"Local<Value>", "Napi::Value"),
    (r"Local<Array>", "Napi::Array"),
    (r"Local<String>", "Napi::String"),
    (r"Local<Number>", "Napi::Number"),
    (r"Local<Boolean>", "Napi::Boolean"),
    (r"Local<Function>", "Napi::Function"),
    (r"Nan::Callback", "Napi::FunctionReference"),
    # scalar conversions
    (r"Nan::To<double>\((.*?)\)\.FromJust\(\)", r"\1.As<Napi::Number>().DoubleValue()"),
    (r"Nan::To<int>\((.*?)\)\.FromJust\(\)", r"\1.As<Napi::Number>().Int32Value()"),
    (r"Nan::To<double>\((.*?)\)", r"\1.As<Napi::Number>().DoubleValue()"),
    (r"Nan::To<int>\((.*?)\)", r"\1.As<Napi::Number>().Int32Value()"),
    (r"\*Nan::Utf8String\((.*?)\)", r"\1.As<Napi::String>().Utf8Value()"),
    (r"Nan::Utf8String\((.*?)\)", r"\1.As<Napi::String>().Utf8Value()"),
    # `Nan::Utf8String name(arg);` is a local holding the string
    (r"Nan::Utf8String (\w+)\(([^)]*)\);", r"std::string \1 = \2.As<Napi::String>().Utf8Value();"),
    # typed errors
    (r"Nan::ThrowTypeError\((.*?)\);", "Napi::TypeError::New(%s, \\1).ThrowAsJavaScriptException();" % E),
    (r"Nan::ThrowRangeError\((.*?)\);", "Napi::RangeError::New(%s, \\1).ThrowAsJavaScriptException();" % E),
    # dates
    (r"Nan::New<Date>\((.*?)\)\.ToLocalChecked\(\)", "Napi::Date::New(%s, \\1)" % E),
    # the static constructor reference, when it has leading whitespace
    (r"(?m)^(\s*)Nan::Persistent<FunctionTemplate> (\w+)::constructor;", r"\1Napi::FunctionReference \2::constructor;"),
    # members with no payload (the collections/views): the constructor takes no
    # argument, the factory attaches the parent afterwards
    (r"(?m)^(\w+)::\1\(\) : Nan::ObjectWrap\(\)", r"\1::\1(const Napi::CallbackInfo &info) : GDALObject<\1>(info)"),
    # object property access
    (r"Nan::Get\((.*?),\s*(.*?)\)\.ToLocalChecked\(\)", r"\1.As<Napi::Object>().Get(\2)"),
    (r"Nan::Get\((.*?),\s*(.*?)\)", r"\1.As<Napi::Object>().Get(\2)"),
    (r"Nan::HasOwnProperty\((.*?),\s*(.*?)\)", r"\1.As<Napi::Object>().HasOwnProperty(\2)"),
    (r"Nan::Set\((.*?),\s*(.*?),\s*(.*?)\)", r"\1.As<Napi::Object>().Set(\2, \3)"),
    # remaining scalar conversions
    (r"Nan::To<int64_t>\((.*?)\)\.ToChecked\(\)", r"\1.As<Napi::Number>().Int64Value()"),
    (r"Nan::To<uint32_t>\((.*?)\)\.ToChecked\(\)", r"\1.As<Napi::Number>().Uint32Value()"),
    (r"Nan::To<int32_t>\((.*?)\)\.ToChecked\(\)", r"\1.As<Napi::Number>().Int32Value()"),
    (r"Nan::To<bool>\((.*?)\)\.ToChecked\(\)", r"\1.As<Napi::Boolean>().Value()"),
    (r"Nan::To<double>\((.*?)\)\.ToChecked\(\)", r"\1.As<Napi::Number>().DoubleValue()"),
    (r"Nan::ADDON_REGISTER_FUNCTION_ARGS_TYPE", "Napi::Object"),
    # bare v8 type names in casts - upstream had `using namespace v8`
    (r"As<Object>", "As<Napi::Object>"),
    (r"As<Value>", "As<Napi::Value>"),
    (r"As<Array>", "As<Napi::Array>"),
    (r"As<Function>", "As<Napi::Function>"),
    (r"As<String>", "As<Napi::String>"),
    (r"As<Number>", "As<Napi::Number>"),
    (r"As<Boolean>", "As<Napi::Boolean>"),
    (r"As<External>", "As<Napi::External<void>>"),
    # the constructor function behind Nan::GetFunction
    (r"Nan::GetFunction\(Napi::String::New\([^,]+, (\w+)::constructor\)\)\.ToLocalChecked\(\)", r"\1::constructor.Value()"),
    (r"Nan::GetFunction\(Napi::String::New\([^,]+, (\w+)::constructor\)\)", r"\1::constructor.Value()"),
    # V8's MaybeLocal API has no N-API counterpart, so whatever is left of these
    # is a leftover of a converted expression - drop them. Must be last.
    (r"\.ToLocalChecked\(\)", ""),
    (r"\.ToChecked\(\)", ""),
    (r"\.FromJust\(\)", ""),
    # NAN had Value::IsInt32(); N-API only tells numbers apart
    (r"\.IsInt32\(\)", ".IsNumber()"),
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
