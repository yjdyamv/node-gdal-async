#!/usr/bin/env python3
"""One-off fixups for node_gdal.cpp after nan2napi.py.

Handles the control-flow and signature rewrites that nan2napi.py deliberately
leaves alone. Safe to re-run: every rule is idempotent.
"""
import re
import sys

path = sys.argv[1]
with open(path, encoding="utf-8") as f:
    src = f.read()

# the v8 namespaces are gone
src = src.replace("using namespace node;\n", "")
src = src.replace("using namespace v8;\n", "")

# globals: define the ambient environment used by the macro layer
src = src.replace(
    "bool eventLoopWarn = true;",
    "bool eventLoopWarn = true;\nNapi::Env napi_env;",
)

# module accessors
src = src.replace(
    'Nan::SetAccessor(target, Napi::String::New(info.Env(), "lastError"), LastErrorGetter, LastErrorSetter);',
    'target.DefineProperty(Napi::PropertyDescriptor::Accessor("lastError", LastErrorGetter, LastErrorSetter));',
)
src = re.sub(
    r'Nan::SetAccessor\(\s*target,\s*Napi::String::New\(info\.Env\(\), "eventLoopWarning"\),\s*'
    r"EventLoopWarningGetter,\s*EventLoopWarningSetter\);",
    'target.DefineProperty(\n    Napi::PropertyDescriptor::Accessor("eventLoopWarning", EventLoopWarningGetter, '
    "EventLoopWarningSetter));",
    src,
)

# handle member access and conversions
src = src.replace("value->IsNull()", "value.IsNull()")
src = src.replace("value->IsBoolean()", "value.IsBoolean()")
src = src.replace("info[1]->IsString()", "info[1].IsString()")
src = src.replace("info[1]->IsNull()", "info[1].IsNull()")
src = src.replace("info[1]->IsUndefined()", "info[1].IsUndefined()")
src = src.replace("Nan::To<bool>(value).ToChecked()", "value.As<Napi::Boolean>().Value()")
src = src.replace("*Nan::Utf8String(info[1])", "info[1].As<Napi::String>().Utf8Value()")

# handle types
src = src.replace("Local<Object> result = ", "Napi::Object result = ")
src = src.replace("Local<Object> supports = ", "Napi::Object supports = ")

# module entry point
src = src.replace(
    "static void Init(Local<Object> target, Local<v8::Value>, void *) {",
    "Napi::Object Init(Napi::Env env, Napi::Object target) {",
)
src = src.replace(
    "NODE_MODULE(NODE_GYP_MODULE_NAME, node_gdal::Init);",
    "NODE_API_MODULE(NODE_GYP_MODULE_NAME, node_gdal::Init);",
)
src = src.replace(
    "  auto *env = GetCurrentEnvironment(Nan::GetCurrentContext());\n  AtExit(env, Cleanup, nullptr);",
    "  napi_add_env_cleanup_hook(env, Cleanup, nullptr);",
)

# line oriented rules
out = []
for line in src.split("\n"):
    line = re.sub(r"info\.GetReturnValue\(\)\.Set\((.*)\);", r"return \1;", line)
    line = re.sub(r"Nan::ThrowError\((.*?)\);", r"Napi::Error::New(info.Env(), \1).ThrowAsJavaScriptException();", line)
    line = re.sub(r"^(\s*)return;$", r"\1return info.Env().Undefined();", line)
    out.append(line)
src = "\n".join(out)

# NAN_SETTER produces a void function - it must not return a value
src = src.replace(
    """    Napi::Error::New(info.Env(), "'lastError' only supports being set to null").ThrowAsJavaScriptException();
    return info.Env().Undefined();""",
    """    Napi::Error::New(info.Env(), "'lastError' only supports being set to null").ThrowAsJavaScriptException();
    return;""",
)
src = src.replace(
    """    Napi::Error::New(info.Env(), "'eventLoopWarning' must be a boolean value").ThrowAsJavaScriptException();
    return info.Env().Undefined();""",
    """    Napi::Error::New(info.Env(), "'eventLoopWarning' must be a boolean value").ThrowAsJavaScriptException();
    return;""",
)

# inside Init the ambient environment is the parameter
marker = "Napi::Object Init(Napi::Env env, Napi::Object target) {"
pos = src.index(marker)
src = src[:pos] + src[pos:].replace("info.Env()", "env")

with open(path, "w", encoding="utf-8") as f:
    f.write(src)
print("fixups applied to %s" % path)
