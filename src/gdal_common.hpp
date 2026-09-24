#ifndef __GDAL_COMMON_H__
#define __GDAL_COMMON_H__

#include <cpl_error.h>
#include <gdal_version.h>
#include <thread>
#include <stdio.h>
#include <string>
#include <memory>

// napi
#include "napi-wrapper.h"

#include "utils/ptr_manager.hpp"

#if GDAL_VERSION_MAJOR < 2 || (GDAL_VERSION_MAJOR == 2 && GDAL_VERSION_MINOR < 2)
#error gdal-async now requires GDAL >= 2.2, downgrade to gdal-async@3.6.x for earlier versions
#endif

namespace node_gdal {
extern FILE *log_file;
extern ObjectStore object_store;
extern bool eventLoopWarn;
extern Napi::Env napi_env;
} // namespace node_gdal

#ifdef ENABLE_LOGGING
#define LOG(fmt, ...)                                                                                                  \
  if (node_gdal::log_file) {                                                                                           \
    fprintf(node_gdal::log_file, fmt "\n", __VA_ARGS__);                                                               \
    fflush(node_gdal::log_file);                                                                                       \
  }
#else
#define LOG(fmt, ...)
#endif

//
// Method signatures
//
// The N-API world uses `Napi::Value (const Napi::CallbackInfo &)`. Methods stay
// static so that both the declaration sites and the descriptor builders below
// can reference them as plain function pointers.
//
#define NAN_METHOD(name) Napi::Value name(const Napi::CallbackInfo &info)
#define NAN_GETTER(name) Napi::Value name(const Napi::CallbackInfo &info)
#define NAN_SETTER(name) void name(const Napi::CallbackInfo &info, const Napi::Value &value)

// NAN_GETTER_ARGS_TYPE / NAN_GETTER_RETURN_TYPE / NAN_SETTER_ARGS_TYPE must NOT
// be redefined here: nan.h uses those very names for typedefs of its own, so
// redefining them breaks nan.h itself. They are unused by this code base.

// Nan::New(null) used to segfault, hence this helper
class SafeString {
    public:
  static Napi::Value New(Napi::Env env, const char *data) {
    if (!data) return env.Null();
    return Napi::String::New(env, data);
  }
  // Convenience for the (many) call sites that run on the main thread and can
  // use the ambient environment - same approach as v8_undefined()/v8_null()
  static Napi::Value New(const char *data) {
    return New(node_gdal::napi_env, data);
  }
};

inline const char *getOGRErrMsg(int err) {
  if (err == 6) {
    // get more descriptive error
    return CPLGetLastErrorMsg();
  }
  switch (err) {
    case 0: return "No error";
    case 1: return "Not enough data";
    case 2: return "Not enough memory";
    case 3: return "Unsupported geometry type";
    case 4: return "Unsupported operation";
    case 5: return "Corrupt Data";
    case 6: return "Failure";
    case 7: return "Unsupported SRS";
    default: return "Invalid Error";
  }
};

//
// Property descriptor builders
//
// node-addon-api's `InstanceMethod` only accepts *member* function pointers,
// but every method in this code base is a static function. `ClassPropertyDescriptor`
// has a public constructor from a raw `napi_property_descriptor`, so we can
// build descriptors around plain function pointers and still get instance
// methods (no `napi_static` flag).
//
namespace node_gdal {

template <Napi::Value (*FN)(const Napi::CallbackInfo &)> napi_value MethodTrampoline(::napi_env env, napi_callback_info info) {
  Napi::CallbackInfo ci(env, info);
  return FN(ci);
}

template <void (*FN)(const Napi::CallbackInfo &, const Napi::Value &)>
napi_value SetterTrampoline(::napi_env env, napi_callback_info info) {
  Napi::CallbackInfo ci(env, info);
  FN(ci, ci.Length() > 0 ? ci[0] : ci.Env().Undefined());
  return nullptr;
}

inline napi_value UndefinedTrampoline(::napi_env env, napi_callback_info) {
  return Napi::Env(env).Undefined();
}

} // namespace node_gdal

template <typename T, Napi::Value (*FN)(const Napi::CallbackInfo &)>
inline Napi::ClassPropertyDescriptor<T> GDALInstanceMethodT(const char *name) {
  napi_property_descriptor d = {};
  d.utf8name = name;
  d.method = &node_gdal::MethodTrampoline<FN>;
  d.attributes = static_cast<napi_property_attributes>(napi_writable | napi_enumerable | napi_configurable);
  return Napi::ClassPropertyDescriptor<T>(d);
}

// Same, but non-enumerable (used for the hidden async accessors)
template <typename T, Napi::Value (*FN)(const Napi::CallbackInfo &)>
inline Napi::ClassPropertyDescriptor<T> GDALInstanceMethodHiddenT(const char *name) {
  napi_property_descriptor d = {};
  d.utf8name = name;
  d.method = &node_gdal::MethodTrampoline<FN>;
  d.attributes = static_cast<napi_property_attributes>(napi_writable | napi_configurable);
  return Napi::ClassPropertyDescriptor<T>(d);
}

template <typename T, Napi::Value (*GET)(const Napi::CallbackInfo &)>
inline Napi::ClassPropertyDescriptor<T> GDALInstanceAccessorT(const char *name) {
  napi_property_descriptor d = {};
  d.utf8name = name;
  d.getter = &node_gdal::MethodTrampoline<GET>;
  d.data = (void *)name;
  d.attributes = static_cast<napi_property_attributes>(napi_writable | napi_enumerable | napi_configurable);
  return Napi::ClassPropertyDescriptor<T>(d);
}

template <typename T, Napi::Value (*GET)(const Napi::CallbackInfo &)>
inline Napi::ClassPropertyDescriptor<T> GDALInstanceAccessorHiddenT(const char *name) {
  napi_property_descriptor d = {};
  d.utf8name = name;
  d.getter = &node_gdal::MethodTrampoline<GET>;
  d.data = (void *)name;
  d.attributes = static_cast<napi_property_attributes>(napi_writable | napi_configurable);
  return Napi::ClassPropertyDescriptor<T>(d);
}

//
// Scalar -> Napi::Value conversions used by the generated method bodies
//
namespace node_gdal {
inline Napi::Value ToNapi(Napi::Env env, int v) {
  return Napi::Number::New(env, v);
}
inline Napi::Value ToNapi(Napi::Env env, unsigned int v) {
  return Napi::Number::New(env, v);
}
inline Napi::Value ToNapi(Napi::Env env, long v) {
  return Napi::Number::New(env, (double)v);
}
inline Napi::Value ToNapi(Napi::Env env, unsigned long v) {
  return Napi::Number::New(env, (double)v);
}
inline Napi::Value ToNapi(Napi::Env env, long long v) {
  return Napi::Number::New(env, (double)v);
}
inline Napi::Value ToNapi(Napi::Env env, unsigned long long v) {
  return Napi::Number::New(env, (double)v);
}
inline Napi::Value ToNapi(Napi::Env env, double v) {
  return Napi::Number::New(env, v);
}
inline Napi::Value ToNapi(Napi::Env env, float v) {
  return Napi::Number::New(env, v);
}
inline Napi::Value ToNapi(Napi::Env env, bool v) {
  return Napi::Boolean::New(env, v);
}
inline Napi::Value ToNapi(Napi::Env env, const std::string &v) {
  return Napi::String::New(env, v);
}
inline Napi::Value ToNapi(Napi::Env env, const char *v) {
  return SafeString::New(env, v);
}
inline Napi::Value ToNapi(Napi::Env, Napi::Value v) {
  return v;
}
inline Napi::Value ToNapi(Napi::Env, Napi::Object v) {
  return v;
}
inline Napi::Value ToNapi(Napi::Env, Napi::Array v) {
  return v;
}
inline Napi::Value ToNapi(Napi::Env, Napi::String v) {
  return v;
}
inline Napi::Value ToNapi(Napi::Env, Napi::Number v) {
  return v;
}
} // namespace node_gdal


template <
  typename T,
  Napi::Value (*GET)(const Napi::CallbackInfo &),
  void (*SET)(const Napi::CallbackInfo &, const Napi::Value &)>
inline Napi::ClassPropertyDescriptor<T> GDALInstanceAccessorT(const char *name) {
  napi_property_descriptor d = {};
  d.utf8name = name;
  d.getter = &node_gdal::MethodTrampoline<GET>;
  d.setter = &node_gdal::SetterTrampoline<SET>;
  d.data = (void *)name;
  d.attributes = static_cast<napi_property_attributes>(napi_writable | napi_enumerable | napi_configurable);
  return Napi::ClassPropertyDescriptor<T>(d);
}

// Module-level (free) functions
inline Napi::Function GDALFunction(Napi::Env env, Napi::Value (*fn)(const Napi::CallbackInfo &)) {
  return Napi::Function::New(env, fn);
}

// ----- throwing / rejecting helpers -------

// These are referenced from the macros below as node_gdal::..., so they must
// live in the node_gdal namespace (node_gdal::ToNapi and the trampolines
// already do)
namespace node_gdal {

inline void ThrowError(Napi::Env env, const char *msg) {
  Napi::Error::New(env, msg).ThrowAsJavaScriptException();
}

inline Napi::Value RejectPromise(const Napi::CallbackInfo &info, const char *msg) {
  auto deferred = Napi::Promise::Deferred::New(info.Env());
  deferred.Reject(Napi::Error::New(info.Env(), msg).Value());
  return deferred.Promise();
}

} // namespace node_gdal

#define NODE_THROW_LAST_CPLERR Napi::Error::New(info.Env(), CPLGetLastErrorMsg()).ThrowAsJavaScriptException()

#define NODE_THROW_OGRERR(err) Napi::Error::New(info.Env(), getOGRErrMsg(err)).ThrowAsJavaScriptException()

//
// Object handle helpers
//
namespace node_gdal {

template <typename T> inline T *UnwrapWrapped(const Napi::Object &obj) {
  return Napi::ObjectWrap<T>::Unwrap(obj);
}

template <typename T> inline bool IsInstanceOf(const Napi::Value &obj) {
  return obj.IsObject() && obj.As<Napi::Object>().InstanceOf(T::constructor.Value());
}

} // namespace node_gdal

// ----- object base class -------

//
// Every JS-visible class derives from this instead of Napi::ObjectWrap<T>
// directly, for two reasons:
//
//  * the message thrown by a constructor called without `new` is asserted by
//    the test suite (test/api_classes.test.ts), and node-addon-api's own
//    wording ("Class constructors cannot be invoked without 'new'") does not
//    match what NAN used to throw;
//  * it gives the classes a single place for the shared constructor plumbing.
//
template <typename T> class GDALObject : public Napi::ObjectWrap<T> {
    public:
  explicit GDALObject(const Napi::CallbackInfo &info) : Napi::ObjectWrap<T>(info) {
  }

  static Napi::Value OnCalledAsFunction(const Napi::CallbackInfo &info) {
    Napi::Error::New(info.Env(), "Cannot call constructor as function, you need to use 'new' keyword")
      .ThrowAsJavaScriptException();
    return info.Env().Undefined();
  }
};

// ----- private property keys -------

//
// NAN had Nan::SetPrivate/GetPrivate, keyed by a *name* and interned by V8.
// N-API only has napi_create_symbol, which yields a fresh symbol on every call,
// so the keys have to be registered to keep behaving like the NAN originals.
// gdal-async supports a single instance per V8 isolate (enforced in Init), so a
// process-wide registry is safe; the references are never released because
// static destruction happens after the environment is gone.
//
namespace node_gdal {

inline Napi::Symbol PrivateKey(Napi::Env env, const char *name) {
  static std::map<std::string, Napi::Reference<Napi::Symbol>> keys;
  auto i = keys.find(name);
  if (i != keys.end()) return i->second.Value();
  Napi::Symbol s = Napi::Symbol::New(env, name);
  Napi::Reference<Napi::Symbol> ref = Napi::Persistent(s);
  ref.SuppressDestruct();
  keys.emplace(name, std::move(ref));
  return s;
}

} // namespace node_gdal

// `.As<Napi::Object>()` keeps these usable with either an Object or a Value
// (call sites pass info.This() and plain locals alike)
#define GDAL_SET_PRIVATE(obj, name, value)                                                                             \
  (obj).As<Napi::Object>().Set(node_gdal::PrivateKey((obj).Env(), name), value)
#define GDAL_GET_PRIVATE(obj, name) (obj).As<Napi::Object>().Get(node_gdal::PrivateKey((obj).Env(), name))

// ----- inheritance -------

//
// node-addon-api has no equivalent of v8::FunctionTemplate::Inherit, and the
// stable N-API has no prototype setter (node_api_set_prototype is behind
// NAPI_EXPERIMENTAL), so the chain is established from JS.
//
// Both the constructor and its `prototype` object have to be relinked:
// `x instanceof Base` walks `Derived.prototype`, while `Derived.staticMember`
// needs the constructor chain.
//
namespace node_gdal {

inline void Inherit(Napi::Function derived, Napi::Function base) {
  Napi::Env env = derived.Env();
  Napi::Function setProtoOf =
    env.Global().Get("Object").As<Napi::Object>().Get("setPrototypeOf").As<Napi::Function>();
  setProtoOf.Call({derived, base});
  setProtoOf.Call({derived.Get("prototype"), base.Get("prototype")});
}

} // namespace node_gdal

// ----- object property conversion -------

#define GDAL_HAS_PROP(obj, key) obj.HasOwnProperty(Napi::String::New(info.Env(), key))

#define NODE_DOUBLE_FROM_OBJ(obj, key, var)                                                                            \
  {                                                                                                                    \
    if (!obj.HasOwnProperty(Napi::String::New(info.Env(), key))) {                                                      \
      Napi::Error::New(info.Env(), "Object must contain property \"" key "\"").ThrowAsJavaScriptException();            \
      return info.Env().Undefined();                                                                                    \
    }                                                                                                                  \
    Napi::Value val = obj.Get(Napi::String::New(info.Env(), key));                                                      \
    if (!val.IsNumber()) {                                                                                              \
      Napi::TypeError::New(info.Env(), "Property \"" key "\" must be a number").ThrowAsJavaScriptException();          \
      return info.Env().Undefined();                                                                                    \
    }                                                                                                                  \
    var = val.As<Napi::Number>().DoubleValue();                                                                         \
  }

#define NODE_INT_FROM_OBJ(obj, key, var)                                                                               \
  {                                                                                                                    \
    if (!obj.HasOwnProperty(Napi::String::New(info.Env(), key))) {                                                      \
      Napi::Error::New(info.Env(), "Object must contain property \"" key "\"").ThrowAsJavaScriptException();            \
      return info.Env().Undefined();                                                                                    \
    }                                                                                                                  \
    Napi::Value val = obj.Get(Napi::String::New(info.Env(), key));                                                      \
    if (!val.IsNumber()) {                                                                                              \
      Napi::TypeError::New(info.Env(), "Property \"" key "\" must be a number").ThrowAsJavaScriptException();          \
      return info.Env().Undefined();                                                                                    \
    }                                                                                                                  \
    var = val.As<Napi::Number>().Int32Value();                                                                          \
  }

#define NODE_STR_FROM_OBJ(obj, key, var)                                                                               \
  {                                                                                                                    \
    if (!obj.HasOwnProperty(Napi::String::New(info.Env(), key))) {                                                      \
      Napi::Error::New(info.Env(), "Object must contain property \"" key "\"").ThrowAsJavaScriptException();            \
      return info.Env().Undefined();                                                                                    \
    }                                                                                                                  \
    Napi::Value val = obj.Get(Napi::String::New(info.Env(), key));                                                      \
    if (!val.IsString()) {                                                                                              \
      Napi::TypeError::New(info.Env(), "Property \"" key "\" must be a string").ThrowAsJavaScriptException();          \
      return info.Env().Undefined();                                                                                    \
    }                                                                                                                  \
    var = val.As<Napi::String>().Utf8Value();                                                                           \
  }

#define NODE_ARRAY_FROM_OBJ(obj, key, var)                                                                             \
  {                                                                                                                    \
    if (!obj.HasOwnProperty(Napi::String::New(info.Env(), key))) {                                                      \
      Napi::Error::New(info.Env(), "Object must contain property \"" key "\"").ThrowAsJavaScriptException();            \
      return info.Env().Undefined();                                                                                    \
    }                                                                                                                  \
    Napi::Value val = obj.Get(Napi::String::New(info.Env(), key));                                                      \
    if (!val.IsArray()) {                                                                                               \
      Napi::TypeError::New(info.Env(), "Property \"" key "\" must be an array").ThrowAsJavaScriptException();          \
      return info.Env().Undefined();                                                                                    \
    }                                                                                                                  \
    var = val.As<Napi::Array>();                                                                                        \
  }

#define NODE_ARRAY_FROM_OBJ_OPT(obj, key, var)                                                                         \
  {                                                                                                                    \
    if (obj.HasOwnProperty(Napi::String::New(info.Env(), key))) {                                                       \
      Napi::Value val = obj.Get(Napi::String::New(info.Env(), key));                                                    \
      if (!val.IsUndefined() && !val.IsNull()) {                                                                        \
        if (!val.IsArray()) {                                                                                           \
          Napi::TypeError::New(info.Env(), "Property \"" key "\" must be an array").ThrowAsJavaScriptException();      \
          return info.Env().Undefined();                                                                                 \
        }                                                                                                              \
        var = val.As<Napi::Array>();                                                                                    \
      }                                                                                                                \
    }                                                                                                                  \
  }

#define NODE_WRAPPED_FROM_OBJ(obj, key, type, var)                                                                     \
  {                                                                                                                    \
    if (!obj.HasOwnProperty(Napi::String::New(info.Env(), key))) {                                                      \
      Napi::Error::New(info.Env(), "Object must contain property \"" key "\"").ThrowAsJavaScriptException();            \
      return info.Env().Undefined();                                                                                    \
    }                                                                                                                  \
    Napi::Value val = obj.Get(Napi::String::New(info.Env(), key));                                                      \
    if (!node_gdal::IsInstanceOf<type>(val)) {                                                                          \
      Napi::TypeError::New(info.Env(), "Property \"" key "\" must be a " #type " object").ThrowAsJavaScriptException(); \
      return info.Env().Undefined();                                                                                    \
    }                                                                                                                  \
    var = node_gdal::UnwrapWrapped<type>(val.As<Napi::Object>());                                                       \
    if (!var->isAlive()) {                                                                                              \
      Napi::Error::New(info.Env(), key ": " #type " object has already been destroyed").ThrowAsJavaScriptException();   \
      return info.Env().Undefined();                                                                                    \
    }                                                                                                                  \
  }

#define NODE_WRAPPED_FROM_OBJ_OPT(obj, key, type, var)                                                                 \
  {                                                                                                                    \
    if (obj.HasOwnProperty(Napi::String::New(info.Env(), key))) {                                                       \
      Napi::Value val = obj.Get(Napi::String::New(info.Env(), key));                                                    \
      if (node_gdal::IsInstanceOf<type>(val)) {                                                                         \
        var = node_gdal::UnwrapWrapped<type>(val.As<Napi::Object>());                                                   \
        if (!var->isAlive()) {                                                                                          \
          Napi::Error::New(info.Env(), key ": " #type " object has already been destroyed")                             \
            .ThrowAsJavaScriptException();                                                                              \
          return info.Env().Undefined();                                                                                \
        }                                                                                                              \
      } else if (!val.IsNull() && !val.IsUndefined()) {                                                                 \
        Napi::TypeError::New(info.Env(), key "property must be a " #type " object").ThrowAsJavaScriptException();       \
        return info.Env().Undefined();                                                                                  \
      }                                                                                                                \
    }                                                                                                                  \
  }

#define NODE_DOUBLE_FROM_OBJ_OPT(obj, key, var)                                                                        \
  {                                                                                                                    \
    if (obj.HasOwnProperty(Napi::String::New(info.Env(), key))) {                                                       \
      Napi::Value val = obj.Get(Napi::String::New(info.Env(), key));                                                    \
      if (!val.IsNumber()) {                                                                                            \
        Napi::TypeError::New(info.Env(), "Property \"" key "\" must be a number").ThrowAsJavaScriptException();        \
        return info.Env().Undefined();                                                                                  \
      }                                                                                                                \
      var = val.As<Napi::Number>().DoubleValue();                                                                       \
    }                                                                                                                  \
  }

#define NODE_INT_FROM_OBJ_OPT(obj, key, var)                                                                           \
  {                                                                                                                    \
    if (obj.HasOwnProperty(Napi::String::New(info.Env(), key))) {                                                       \
      Napi::Value val = obj.Get(Napi::String::New(info.Env(), key));                                                    \
      if (!val.IsNumber()) {                                                                                            \
        Napi::TypeError::New(info.Env(), "Property \"" key "\" must be a number").ThrowAsJavaScriptException();        \
        return info.Env().Undefined();                                                                                  \
      }                                                                                                                \
      var = val.As<Napi::Number>().Int32Value();                                                                        \
    }                                                                                                                  \
  }

#define NODE_INT64_FROM_OBJ_OPT(obj, key, var)                                                                         \
  {                                                                                                                    \
    if (obj.HasOwnProperty(Napi::String::New(info.Env(), key))) {                                                       \
      Napi::Value val = obj.Get(Napi::String::New(info.Env(), key));                                                    \
      if (!val.IsNumber()) {                                                                                            \
        Napi::TypeError::New(info.Env(), "Property \"" key "\" must be a number").ThrowAsJavaScriptException();        \
        return info.Env().Undefined();                                                                                  \
      }                                                                                                                \
      var = val.As<Napi::Number>().Int64Value();                                                                        \
    }                                                                                                                  \
  }

#define NODE_STR_FROM_OBJ_OPT(obj, key, var)                                                                           \
  {                                                                                                                    \
    if (obj.HasOwnProperty(Napi::String::New(info.Env(), key))) {                                                       \
      Napi::Value val = obj.Get(Napi::String::New(info.Env(), key));                                                    \
      if (!val.IsString()) {                                                                                            \
        Napi::TypeError::New(info.Env(), "Property \"" key "\" must be a string").ThrowAsJavaScriptException();        \
        return info.Env().Undefined();                                                                                  \
      }                                                                                                                \
      var = val.As<Napi::String>().Utf8Value();                                                                         \
    }                                                                                                                  \
  }

#define NODE_CB_FROM_OBJ_OPT(obj, key, var)                                                                            \
  {                                                                                                                    \
    var = nullptr;                                                                                                     \
    if (obj.HasOwnProperty(Napi::String::New(info.Env(), key))) {                                                       \
      Napi::Value val = obj.Get(Napi::String::New(info.Env(), key));                                                    \
      if (!val.IsFunction()) {                                                                                          \
        Napi::TypeError::New(info.Env(), "Property \"" key "\" must be a function").ThrowAsJavaScriptException();      \
        return info.Env().Undefined();                                                                                  \
      } else {                                                                                                         \
        var = new Napi::FunctionReference(Napi::Persistent(val.As<Napi::Function>()));                                 \
        var->SuppressDestruct();                                                                                       \
      }                                                                                                                \
    }                                                                                                                  \
  }

// ----- argument conversion -------

#define ARG_FIELD_ID(num, f, var)                                                                                      \
  {                                                                                                                    \
    if (info[num].IsString()) {                                                                                        \
      std::string field_name = info[num].As<Napi::String>().Utf8Value();                                               \
      var = f->GetFieldIndex(field_name.c_str());                                                                      \
      if (var == -1) {                                                                                                 \
        Napi::Error::New(info.Env(), "Specified field name does not exist").ThrowAsJavaScriptException();              \
        return info.Env().Undefined();                                                                                  \
      }                                                                                                                \
    } else if (info[num].IsNumber()) {                                                                                 \
      var = info[num].As<Napi::Number>().Int32Value();                                                                 \
      if (var < 0 || var >= f->GetFieldCount()) {                                                                      \
        Napi::RangeError::New(info.Env(), "Invalid field index").ThrowAsJavaScriptException();                         \
        return info.Env().Undefined();                                                                                  \
      }                                                                                                                \
    } else {                                                                                                           \
      Napi::TypeError::New(info.Env(), "Field index must be integer or string").ThrowAsJavaScriptException();          \
      return info.Env().Undefined();                                                                                    \
    }                                                                                                                  \
  }

#define NODE_ARG_INT(num, name, var)                                                                                   \
  if (info.Length() < num + 1) {                                                                                       \
    Napi::Error::New(info.Env(), name " must be given").ThrowAsJavaScriptException();                                  \
    return info.Env().Undefined();                                                                                      \
  }                                                                                                                    \
  if (!info[num].IsNumber()) {                                                                                         \
    Napi::TypeError::New(info.Env(), name " must be an integer").ThrowAsJavaScriptException();                         \
    return info.Env().Undefined();                                                                                      \
  }                                                                                                                    \
  var = static_cast<int>(info[num].As<Napi::Number>().Int64Value());

#define NODE_ARG_ENUM(num, name, enum_type, var)                                                                       \
  if (info.Length() < num + 1) {                                                                                       \
    Napi::Error::New(info.Env(), name " must be given").ThrowAsJavaScriptException();                                  \
    return info.Env().Undefined();                                                                                      \
  }                                                                                                                    \
  if (!info[num].IsNumber()) {                                                                                         \
    Napi::TypeError::New(info.Env(), name " must be of type " #enum_type).ThrowAsJavaScriptException();                \
    return info.Env().Undefined();                                                                                      \
  }                                                                                                                    \
  var = enum_type(info[num].As<Napi::Number>().Uint32Value());

#define NODE_ARG_BOOL(num, name, var)                                                                                  \
  if (info.Length() < num + 1) {                                                                                       \
    Napi::Error::New(info.Env(), name " must be given").ThrowAsJavaScriptException();                                  \
    return info.Env().Undefined();                                                                                      \
  }                                                                                                                    \
  if (!info[num].IsBoolean()) {                                                                                        \
    Napi::TypeError::New(info.Env(), name " must be an boolean").ThrowAsJavaScriptException();                         \
    return info.Env().Undefined();                                                                                      \
  }                                                                                                                    \
  var = info[num].As<Napi::Boolean>().Value();

#define NODE_ARG_DOUBLE(num, name, var)                                                                                \
  if (info.Length() < num + 1) {                                                                                       \
    Napi::Error::New(info.Env(), name " must be given").ThrowAsJavaScriptException();                                  \
    return info.Env().Undefined();                                                                                      \
  }                                                                                                                    \
  if (!info[num].IsNumber()) {                                                                                         \
    Napi::TypeError::New(info.Env(), name " must be a number").ThrowAsJavaScriptException();                           \
    return info.Env().Undefined();                                                                                      \
  }                                                                                                                    \
  var = info[num].As<Napi::Number>().DoubleValue();

#define NODE_ARG_ARRAY(num, name, var)                                                                                 \
  if (info.Length() < num + 1) {                                                                                       \
    Napi::Error::New(info.Env(), name " must be given").ThrowAsJavaScriptException();                                  \
    return info.Env().Undefined();                                                                                      \
  }                                                                                                                    \
  if (!info[num].IsArray()) {                                                                                          \
    Napi::TypeError::New(info.Env(), name " must be an array").ThrowAsJavaScriptException();                           \
    return info.Env().Undefined();                                                                                      \
  }                                                                                                                    \
  var = info[num].As<Napi::Array>();

#define NODE_ARG_OBJECT(num, name, var)                                                                                \
  if (info.Length() < num + 1) {                                                                                       \
    Napi::Error::New(info.Env(), name " must be given").ThrowAsJavaScriptException();                                  \
    return info.Env().Undefined();                                                                                      \
  }                                                                                                                    \
  if (!info[num].IsObject()) {                                                                                         \
    Napi::TypeError::New(info.Env(), name " must be an object").ThrowAsJavaScriptException();                          \
    return info.Env().Undefined();                                                                                      \
  }                                                                                                                    \
  var = info[num].As<Napi::Object>();

#define NODE_ARG_WRAPPED(num, name, type, var)                                                                         \
  if (info.Length() < num + 1) {                                                                                       \
    Napi::Error::New(info.Env(), name " must be given").ThrowAsJavaScriptException();                                  \
    return info.Env().Undefined();                                                                                      \
  }                                                                                                                    \
  if (info[num].IsNull() || info[num].IsUndefined() || !node_gdal::IsInstanceOf<type>(info[num])) {                     \
    Napi::TypeError::New(info.Env(), name " must be an instance of " #type).ThrowAsJavaScriptException();              \
    return info.Env().Undefined();                                                                                      \
  }                                                                                                                    \
  var = node_gdal::UnwrapWrapped<type>(info[num].As<Napi::Object>());                                                  \
  if (!var->isAlive()) {                                                                                               \
    Napi::Error::New(info.Env(), #type " parameter already destroyed").ThrowAsJavaScriptException();                  \
    return info.Env().Undefined();                                                                                      \
  }

#define NODE_ARG_STR(num, name, var)                                                                                   \
  if (info.Length() < num + 1) {                                                                                       \
    Napi::Error::New(info.Env(), name " must be given").ThrowAsJavaScriptException();                                  \
    return info.Env().Undefined();                                                                                      \
  }                                                                                                                    \
  if (!info[num].IsString()) {                                                                                         \
    Napi::TypeError::New(info.Env(), name " must be a string").ThrowAsJavaScriptException();                           \
    return info.Env().Undefined();                                                                                      \
  }                                                                                                                    \
  var = info[num].As<Napi::String>().Utf8Value()

#define NODE_ARG_STR_INT(num, name, varString, varInt, isString)                                                       \
  bool isString = false;                                                                                               \
  if (info.Length() < num + 1) {                                                                                       \
    Napi::Error::New(info.Env(), name " must be given").ThrowAsJavaScriptException();                                  \
    return info.Env().Undefined();                                                                                      \
  }                                                                                                                    \
  if (info[num].IsString()) {                                                                                          \
    varString = info[num].As<Napi::String>().Utf8Value();                                                              \
    isString = true;                                                                                                   \
  } else if (info[num].IsNumber()) {                                                                                   \
    varInt = static_cast<int>(info[num].As<Napi::Number>().Int64Value());                                              \
    isString = false;                                                                                                  \
  } else {                                                                                                             \
    Napi::TypeError::New(info.Env(), name " must be a string or a number").ThrowAsJavaScriptException();               \
    return info.Env().Undefined();                                                                                      \
  }

#define NODE_ARG_BUFFER(num, name, var)                                                                                \
  if (info.Length() < num + 1) {                                                                                       \
    Napi::Error::New(info.Env(), name " must be given").ThrowAsJavaScriptException();                                  \
    return info.Env().Undefined();                                                                                      \
  }                                                                                                                    \
  if (!info[num].IsBuffer() && !info[num].IsArrayBuffer()) {                                                           \
    Napi::TypeError::New(info.Env(), name " must be a buffer").ThrowAsJavaScriptException();                           \
    return info.Env().Undefined();                                                                                      \
  }                                                                                                                    \
  var = info[num]

#define NODE_ARG_CB(num, name, var)                                                                                    \
  if (info.Length() < num + 1) {                                                                                       \
    Napi::Error::New(info.Env(), name " must be given").ThrowAsJavaScriptException();                                  \
    return info.Env().Undefined();                                                                                      \
  }                                                                                                                    \
  if (!info[num].IsFunction()) {                                                                                       \
    Napi::TypeError::New(info.Env(), name " must be a function").ThrowAsJavaScriptException();                         \
    return info.Env().Undefined();                                                                                      \
  }                                                                                                                    \
  var = new Napi::FunctionReference(Napi::Persistent(info[num].As<Napi::Function>()));                                 \
  var->SuppressDestruct()

// ----- optional argument conversion -------

#define NODE_ARG_INT_OPT(num, name, var)                                                                               \
  if (info.Length() > num) {                                                                                           \
    if (info[num].IsNumber()) {                                                                                        \
      var = static_cast<int>(info[num].As<Napi::Number>().Int64Value());                                               \
    } else if (!info[num].IsNull() && !info[num].IsUndefined()) {                                                      \
      Napi::TypeError::New(info.Env(), name " must be an integer").ThrowAsJavaScriptException();                       \
      return info.Env().Undefined();                                                                                    \
    }                                                                                                                  \
  }

#define NODE_ARG_ENUM_OPT(num, name, enum_type, var)                                                                   \
  if (info.Length() > num) {                                                                                           \
    if (info[num].IsNumber()) {                                                                                        \
      var = static_cast<enum_type>(info[num].As<Napi::Number>().Uint32Value());                                         \
    } else if (!info[num].IsNull() && !info[num].IsUndefined()) {                                                      \
      Napi::TypeError::New(info.Env(), name " must be an integer").ThrowAsJavaScriptException();                       \
      return info.Env().Undefined();                                                                                    \
    }                                                                                                                  \
  }

#define NODE_ARG_BOOL_OPT(num, name, var)                                                                              \
  if (info.Length() > num) {                                                                                           \
    if (info[num].IsBoolean()) {                                                                                       \
      var = info[num].As<Napi::Boolean>().Value();                                                                     \
    } else if (!info[num].IsNull() && !info[num].IsUndefined()) {                                                      \
      Napi::TypeError::New(info.Env(), name " must be an boolean").ThrowAsJavaScriptException();                       \
      return info.Env().Undefined();                                                                                    \
    }                                                                                                                  \
  }

#define NODE_ARG_OPT_STR(num, name, var)                                                                               \
  if (info.Length() > num) {                                                                                           \
    if (info[num].IsString()) {                                                                                        \
      var = info[num].As<Napi::String>().Utf8Value();                                                                  \
    } else if (!info[num].IsNull() && !info[num].IsUndefined()) {                                                      \
      Napi::TypeError::New(info.Env(), name " must be a string").ThrowAsJavaScriptException();                         \
      return info.Env().Undefined();                                                                                    \
    }                                                                                                                  \
  }

#define NODE_ARG_DOUBLE_OPT(num, name, var)                                                                            \
  if (info.Length() > num) {                                                                                           \
    if (info[num].IsNumber()) {                                                                                        \
      var = info[num].As<Napi::Number>().DoubleValue();                                                                \
    } else if (!info[num].IsNull() && !info[num].IsUndefined()) {                                                      \
      Napi::TypeError::New(info.Env(), name " must be a number").ThrowAsJavaScriptException();                         \
      return info.Env().Undefined();                                                                                    \
    }                                                                                                                  \
  }

#define NODE_ARG_WRAPPED_OPT(num, name, type, var)                                                                     \
  if (info.Length() > num && !info[num].IsNull() && !info[num].IsUndefined()) {                                        \
    if (!node_gdal::IsInstanceOf<type>(info[num])) {                                                                   \
      Napi::TypeError::New(info.Env(), name " must be an instance of " #type).ThrowAsJavaScriptException();            \
      return info.Env().Undefined();                                                                                    \
    }                                                                                                                  \
    var = node_gdal::UnwrapWrapped<type>(info[num].As<Napi::Object>());                                                \
    if (!var->isAlive()) {                                                                                             \
      Napi::Error::New(info.Env(), #type " parameter already destroyed").ThrowAsJavaScriptException();                \
      return info.Env().Undefined();                                                                                    \
    }                                                                                                                  \
  }

#define NODE_ARG_ARRAY_OPT(num, name, var)                                                                             \
  if (info.Length() > num) {                                                                                           \
    if (info[num].IsArray()) {                                                                                         \
      var = info[num].As<Napi::Array>();                                                                               \
    } else if (!info[num].IsNull() && !info[num].IsUndefined()) {                                                      \
      Napi::TypeError::New(info.Env(), name " must be an array").ThrowAsJavaScriptException();                         \
      return info.Env().Undefined();                                                                                    \
    }                                                                                                                  \
  }

#define NODE_ARG_OBJECT_OPT(num, name, var)                                                                            \
  if (info.Length() > num && !info[num].IsNull() && !info[num].IsUndefined()) {                                        \
    if (!info[num].IsObject()) {                                                                                       \
      Napi::TypeError::New(info.Env(), name " must be an object").ThrowAsJavaScriptException();                        \
      return info.Env().Undefined();                                                                                    \
    }                                                                                                                  \
    var = info[num].As<Napi::Object>();                                                                                \
  }

#define NODE_ARG_CB_OPT(num, name, var)                                                                                \
  if (info.Length() > num && !info[num].IsNull() && !info[num].IsUndefined()) {                                        \
    if (info[num].IsFunction()) {                                                                                      \
      var = new Napi::FunctionReference(Napi::Persistent(info[num].As<Napi::Function>()));                             \
      var->SuppressDestruct();                                                                                         \
    } else {                                                                                                           \
      Napi::TypeError::New(info.Env(), name " must be a function").ThrowAsJavaScriptException();                       \
      return info.Env().Undefined();                                                                                    \
    }                                                                                                                  \
  }

// ----- special case: progress callback in [options] argument -------

#define NODE_PROGRESS_CB_OPT(num, progress_cb, job)                                                                    \
  {                                                                                                                    \
    Napi::Object progress_obj;                                                                                         \
    progress_cb = nullptr;                                                                                             \
    if (info.Length() > num && !info[num].IsNull() && !info[num].IsUndefined()) {                                      \
      if (!info[num].IsObject()) {                                                                                     \
        Napi::TypeError::New(info.Env(), "options must be an object").ThrowAsJavaScriptException();                    \
        return info.Env().Undefined();                                                                                  \
      }                                                                                                                \
      progress_obj = info[num].As<Napi::Object>();                                                                     \
      NODE_CB_FROM_OBJ_OPT(progress_obj, "progress_cb", progress_cb);                                                  \
    }                                                                                                                  \
    if (progress_cb) { job.progress = progress_cb; }                                                                   \
  }

// ----- wrapped methods w/ results-------

#define NODE_WRAPPED_METHOD_WITH_RESULT(klass, method, result_type, wrapped_method)                                    \
  NAN_METHOD(klass::method) {                                                                                          \
    klass *obj = node_gdal::UnwrapWrapped<klass>(info.This().As<Napi::Object>());                                      \
    if (!obj->isAlive()) {                                                                                             \
      Napi::Error::New(info.Env(), #klass " object has already been destroyed").ThrowAsJavaScriptException();          \
      return info.Env().Undefined();                                                                                    \
    }                                                                                                                  \
    return node_gdal::ToNapi(info.Env(), obj->this_->wrapped_method());                                    \
  }

#define NODE_WRAPPED_METHOD_WITH_RESULT_1_ENUM_PARAM(                                                                  \
  klass, method, result_type, wrapped_method, enum_type, param_name)                                                   \
  NAN_METHOD(klass::method) {                                                                                          \
    enum_type param;                                                                                                   \
    NODE_ARG_ENUM(0, #param_name, enum_type, param);                                                                   \
    klass *obj = node_gdal::UnwrapWrapped<klass>(info.This().As<Napi::Object>());                                      \
    if (!obj->isAlive()) {                                                                                             \
      Napi::Error::New(info.Env(), #klass " object has already been destroyed").ThrowAsJavaScriptException();          \
      return info.Env().Undefined();                                                                                    \
    }                                                                                                                  \
    return node_gdal::ToNapi(info.Env(), obj->this_->wrapped_method(param));                               \
  }

#define NODE_WRAPPED_METHOD_WITH_RESULT_1_INTEGER_PARAM(klass, method, result_type, wrapped_method, param_name)        \
  NAN_METHOD(klass::method) {                                                                                          \
    int param;                                                                                                         \
    NODE_ARG_INT(0, #param_name, param);                                                                               \
    klass *obj = node_gdal::UnwrapWrapped<klass>(info.This().As<Napi::Object>());                                      \
    if (!obj->isAlive()) {                                                                                             \
      Napi::Error::New(info.Env(), #klass " object has already been destroyed").ThrowAsJavaScriptException();          \
      return info.Env().Undefined();                                                                                    \
    }                                                                                                                  \
    return node_gdal::ToNapi(info.Env(), obj->this_->wrapped_method(param));                               \
  }

#define NODE_WRAPPED_METHOD_WITH_RESULT_1_DOUBLE_PARAM(klass, method, result_type, wrapped_method, param_name)         \
  NAN_METHOD(klass::method) {                                                                                          \
    double param;                                                                                                      \
    NODE_ARG_DOUBLE(0, #param_name, param);                                                                            \
    klass *obj = node_gdal::UnwrapWrapped<klass>(info.This().As<Napi::Object>());                                      \
    if (!obj->isAlive()) {                                                                                             \
      Napi::Error::New(info.Env(), #klass " object has already been destroyed").ThrowAsJavaScriptException();          \
      return info.Env().Undefined();                                                                                    \
    }                                                                                                                  \
    return node_gdal::ToNapi(info.Env(), obj->this_->wrapped_method(param));                               \
  }

// ----- wrapped methods w/ lock -------

#define NODE_WRAPPED_METHOD_WITH_RESULT_LOCKED(klass, method, result_type, wrapped_method)                             \
  NAN_METHOD(klass::method) {                                                                                          \
    klass *obj = node_gdal::UnwrapWrapped<klass>(info.This().As<Napi::Object>());                                      \
    if (!obj->isAlive()) {                                                                                             \
      Napi::Error::New(info.Env(), #klass " object has already been destroyed").ThrowAsJavaScriptException();          \
      return info.Env().Undefined();                                                                                    \
    }                                                                                                                  \
    GDAL_LOCK_PARENT(obj);                                                                                             \
    return node_gdal::ToNapi(info.Env(), obj->this_->wrapped_method());                                    \
  }

#define NODE_WRAPPED_GETTER_WITH_STRING_LOCKED(klass, method, wrapped_method)                                          \
  NAN_GETTER(klass::method) {                                                                                          \
    klass *obj = node_gdal::UnwrapWrapped<klass>(info.This().As<Napi::Object>());                                      \
    if (!obj->isAlive()) {                                                                                             \
      Napi::Error::New(info.Env(), #klass " object has already been destroyed").ThrowAsJavaScriptException();          \
      return info.Env().Undefined();                                                                                    \
    }                                                                                                                  \
    GDAL_LOCK_PARENT(obj);                                                                                             \
    auto r = obj->this_->wrapped_method();                                                                             \
    return SafeString::New(info.Env(), r.c_str());                                                                     \
  }

#define NODE_WRAPPED_GETTER_WITH_RESULT_LOCKED(klass, method, result_type, wrapped_method)                             \
  NAN_GETTER(klass::method) {                                                                                          \
    klass *obj = node_gdal::UnwrapWrapped<klass>(info.This().As<Napi::Object>());                                      \
    if (!obj->isAlive()) {                                                                                             \
      Napi::Error::New(info.Env(), #klass " object has already been destroyed").ThrowAsJavaScriptException();          \
      return info.Env().Undefined();                                                                                    \
    }                                                                                                                  \
    GDAL_LOCK_PARENT(obj);                                                                                             \
    return node_gdal::ToNapi(info.Env(), obj->this_->wrapped_method());                                    \
  }

// ----- wrapped methods -------

#define NODE_WRAPPED_METHOD(klass, method, wrapped_method)                                                             \
  NAN_METHOD(klass::method) {                                                                                          \
    klass *obj = node_gdal::UnwrapWrapped<klass>(info.This().As<Napi::Object>());                                      \
    if (!obj->isAlive()) {                                                                                             \
      Napi::Error::New(info.Env(), #klass " object has already been destroyed").ThrowAsJavaScriptException();          \
      return info.Env().Undefined();                                                                                    \
    }                                                                                                                  \
    obj->this_->wrapped_method();                                                                                      \
    return info.Env().Undefined();                                                                                     \
  }

#define NODE_WRAPPED_METHOD_WITH_1_INTEGER_PARAM(klass, method, wrapped_method, param_name)                            \
  NAN_METHOD(klass::method) {                                                                                          \
    int param;                                                                                                         \
    NODE_ARG_INT(0, #param_name, param);                                                                               \
    klass *obj = node_gdal::UnwrapWrapped<klass>(info.This().As<Napi::Object>());                                      \
    if (!obj->isAlive()) {                                                                                             \
      Napi::Error::New(info.Env(), #klass " object has already been destroyed").ThrowAsJavaScriptException();          \
      return info.Env().Undefined();                                                                                    \
    }                                                                                                                  \
    obj->this_->wrapped_method(param);                                                                                 \
    return info.Env().Undefined();                                                                                     \
  }

#define NODE_WRAPPED_METHOD_WITH_1_DOUBLE_PARAM(klass, method, wrapped_method, param_name)                             \
  NAN_METHOD(klass::method) {                                                                                          \
    double param;                                                                                                      \
    NODE_ARG_DOUBLE(0, #param_name, param);                                                                            \
    klass *obj = node_gdal::UnwrapWrapped<klass>(info.This().As<Napi::Object>());                                      \
    if (!obj->isAlive()) {                                                                                             \
      Napi::Error::New(info.Env(), #klass " object has already been destroyed").ThrowAsJavaScriptException();          \
      return info.Env().Undefined();                                                                                    \
    }                                                                                                                  \
    obj->this_->wrapped_method(param);                                                                                 \
    return info.Env().Undefined();                                                                                     \
  }

#define NODE_WRAPPED_METHOD_WITH_1_BOOLEAN_PARAM(klass, method, wrapped_method, param_name)                            \
  NAN_METHOD(klass::method) {                                                                                          \
    bool param;                                                                                                        \
    NODE_ARG_BOOL(0, #param_name, param);                                                                              \
    klass *obj = node_gdal::UnwrapWrapped<klass>(info.This().As<Napi::Object>());                                      \
    if (!obj->isAlive()) {                                                                                             \
      Napi::Error::New(info.Env(), #klass " object has already been destroyed").ThrowAsJavaScriptException();          \
      return info.Env().Undefined();                                                                                    \
    }                                                                                                                  \
    obj->this_->wrapped_method(param);                                                                                 \
    return info.Env().Undefined();                                                                                     \
  }

#define NODE_WRAPPED_METHOD_WITH_1_ENUM_PARAM(klass, method, wrapped_method, enum_type, param_name)                    \
  NAN_METHOD(klass::method) {                                                                                          \
    enum_type param;                                                                                                   \
    NODE_ARG_ENUM(0, #param_name, enum_type, param);                                                                   \
    klass *obj = node_gdal::UnwrapWrapped<klass>(info.This().As<Napi::Object>());                                      \
    if (!obj->isAlive()) {                                                                                             \
      Napi::Error::New(info.Env(), #klass " object has already been destroyed").ThrowAsJavaScriptException();          \
      return info.Env().Undefined();                                                                                    \
    }                                                                                                                  \
    obj->this_->wrapped_method(param);                                                                                 \
    return info.Env().Undefined();                                                                                     \
  }

#define NODE_WRAPPED_METHOD_WITH_1_STRING_PARAM(klass, method, wrapped_method, param_name)                             \
  NAN_METHOD(klass::method) {                                                                                          \
    std::string param;                                                                                                 \
    NODE_ARG_STR(0, #param_name, param);                                                                               \
    klass *obj = node_gdal::UnwrapWrapped<klass>(info.This().As<Napi::Object>());                                      \
    if (!obj->isAlive()) {                                                                                             \
      Napi::Error::New(info.Env(), #klass " object has already been destroyed").ThrowAsJavaScriptException();          \
      return info.Env().Undefined();                                                                                    \
    }                                                                                                                  \
    obj->this_->wrapped_method(param.c_str());                                                                         \
    return info.Env().Undefined();                                                                                     \
  }

#define MEASURE_EXECUTION_TIME(msg, op)                                                                                \
  {                                                                                                                    \
    auto start = std::chrono::high_resolution_clock::now();                                                            \
    if (msg != nullptr) fprintf(stderr, "%s", msg);                                                                    \
    op;                                                                                                                \
    auto elapsed = std::chrono::high_resolution_clock::now() - start;                                                  \
    if (msg != nullptr)                                                                                                \
      fprintf(                                                                                                         \
        stderr,                                                                                                        \
        "%ld µs\n",                                                                                                    \
        static_cast<long>(std::chrono::duration_cast<std::chrono::microseconds>(elapsed).count()));                    \
  }

// gdal-async supports a single instance per V8 isolate (enforced in Init),
// so the environment is kept reachable for the few places (async result
// lambdas) where NAN did not require threading it through.
// The environment is declared near the top of this file (the trampolines above
// need the ::napi_env type unshadowed), it is defined in node_gdal.cpp
inline Napi::Value v8_undefined() { return node_gdal::napi_env.Undefined(); }
inline Napi::Value v8_null() { return node_gdal::napi_env.Null(); }

// ----- array helpers -------

template <typename INPUT, typename RETURN>
std::shared_ptr<RETURN[]> NumberArrayToSharedPtr(Napi::Env env, Napi::Array array, size_t count = 0) {
  if (array.IsEmpty()) return nullptr;
  if (count != 0 && array.Length() != count) throw "Array size must match the number of dimensions";
  std::shared_ptr<RETURN[]> ptr(new RETURN[array.Length()]);
  for (unsigned i = 0; i < array.Length(); i++) {
    Napi::Value val = array.Get(i);
    if (!val.IsNumber()) throw "Array must contain only numbers";
    ptr.get()[i] = static_cast<RETURN>(val.As<Napi::Number>().DoubleValue());
  }
  return ptr;
}

// ----- throwing / rejecting -------

#define THROW_OR_REJECT(msg)                                                                                           \
  if (async) {                                                                                                         \
    return node_gdal::RejectPromise(info, msg);                                                                        \
  } else {                                                                                                             \
    Napi::Error::New(info.Env(), msg).ThrowAsJavaScriptException();                                                    \
    return info.Env().Undefined();                                                                                     \
  }

#define IS_WRAPPED(obj, type) node_gdal::IsInstanceOf<type>(obj)

#define NODE_UNWRAP_CHECK(type, obj, var)                                                                              \
  if (!node_gdal::IsInstanceOf<type>(obj)) {                                                                           \
    Napi::TypeError::New(info.Env(), "Object must be a " #type " object").ThrowAsJavaScriptException();                \
    return info.Env().Undefined();                                                                                     \
  }                                                                                                                    \
  type *var = node_gdal::UnwrapWrapped<type>(obj.As<Napi::Object>());                                                  \
  if (!var->isAlive()) {                                                                                               \
    Napi::Error::New(info.Env(), #type " object has already been destroyed").ThrowAsJavaScriptException();             \
    return info.Env().Undefined();                                                                                     \
  }

#define NODE_UNWRAP_CHECK_ASYNC(type, obj, var)                                                                        \
  if (!node_gdal::IsInstanceOf<type>(obj)) {                                                                           \
    THROW_OR_REJECT("Object must be a " #type " object");                                                              \
  }                                                                                                                    \
  type *var = node_gdal::UnwrapWrapped<type>(obj.As<Napi::Object>());                                                  \
  if (!var->isAlive()) { THROW_OR_REJECT(#type " object has already been destroyed"); }

#define GDAL_RAW_CHECK(type, obj, var)                                                                                 \
  type var = obj->get();                                                                                               \
  if (!obj) {                                                                                                          \
    Napi::Error::New(info.Env(), #type " object has already been destroyed").ThrowAsJavaScriptException();             \
    return info.Env().Undefined();                                                                                     \
  }

#define GDAL_RAW_CHECK_ASYNC(type, obj, var)                                                                           \
  type var = obj->get();                                                                                               \
  if (!obj) { THROW_OR_REJECT(#type " object has already been destroyed"); }

// ----- wrapped methods w/ results + wrapped params -------

#define NODE_WRAPPED_METHOD_WITH_RESULT_1_WRAPPED_PARAM(                                                               \
  klass, method, result_type, wrapped_method, param_type, param_name)                                                  \
  NAN_METHOD(klass::method) {                                                                                          \
    param_type *param;                                                                                                 \
    NODE_ARG_WRAPPED(0, #param_name, param_type, param);                                                               \
    klass *obj = node_gdal::UnwrapWrapped<klass>(info.This().As<Napi::Object>());                                      \
    if (!obj->isAlive()) {                                                                                             \
      Napi::Error::New(info.Env(), #klass " object has already been destroyed").ThrowAsJavaScriptException();          \
      return info.Env().Undefined();                                                                                    \
    }                                                                                                                  \
    return node_gdal::ToNapi(info.Env(), obj->this_->wrapped_method(param->get()));                                     \
  }

#define NODE_WRAPPED_METHOD_WITH_RESULT_1_STRING_PARAM(klass, method, result_type, wrapped_method, param_name)         \
  NAN_METHOD(klass::method) {                                                                                          \
    std::string param;                                                                                                 \
    NODE_ARG_STR(0, #param_name, param);                                                                               \
    klass *obj = node_gdal::UnwrapWrapped<klass>(info.This().As<Napi::Object>());                                      \
    if (!obj->isAlive()) {                                                                                             \
      Napi::Error::New(info.Env(), #klass " object has already been destroyed").ThrowAsJavaScriptException();          \
      return info.Env().Undefined();                                                                                    \
    }                                                                                                                  \
    return node_gdal::ToNapi(info.Env(), obj->this_->wrapped_method(param.c_str()));                                    \
  }

#define NODE_WRAPPED_METHOD_WITH_RESULT_1_STRING_PARAM_LOCKED(klass, method, result_type, wrapped_method, param_name)  \
  NAN_METHOD(klass::method) {                                                                                          \
    std::string param;                                                                                                 \
    NODE_ARG_STR(0, #param_name, param);                                                                               \
    klass *obj = node_gdal::UnwrapWrapped<klass>(info.This().As<Napi::Object>());                                      \
    if (!obj->isAlive()) {                                                                                             \
      Napi::Error::New(info.Env(), #klass " object has already been destroyed").ThrowAsJavaScriptException();          \
      return info.Env().Undefined();                                                                                    \
    }                                                                                                                  \
    GDAL_LOCK_PARENT(obj);                                                                                             \
    return node_gdal::ToNapi(info.Env(), obj->this_->wrapped_method(param.c_str()));                                    \
  }

#define NODE_WRAPPED_METHOD_WITH_1_WRAPPED_PARAM(klass, method, wrapped_method, param_type, param_name)                \
  NAN_METHOD(klass::method) {                                                                                          \
    param_type *param;                                                                                                 \
    NODE_ARG_WRAPPED(0, #param_name, param_type, param);                                                               \
    klass *obj = node_gdal::UnwrapWrapped<klass>(info.This().As<Napi::Object>());                                      \
    if (!obj->isAlive()) {                                                                                             \
      Napi::Error::New(info.Env(), #klass " object has already been destroyed").ThrowAsJavaScriptException();          \
      return info.Env().Undefined();                                                                                    \
    }                                                                                                                  \
    obj->this_->wrapped_method(param->get());                                                                          \
    return info.Env().Undefined();                                                                                     \
  }

// ----- wrapped methods w/ CPLErr result (throws) -------

#define NODE_WRAPPED_METHOD_WITH_CPLERR_RESULT(klass, method, wrapped_method)                                          \
  NAN_METHOD(klass::method) {                                                                                          \
    klass *obj = node_gdal::UnwrapWrapped<klass>(info.This().As<Napi::Object>());                                      \
    if (!obj->isAlive()) {                                                                                             \
      Napi::Error::New(info.Env(), #klass " object has already been destroyed").ThrowAsJavaScriptException();          \
      return info.Env().Undefined();                                                                                    \
    }                                                                                                                  \
    int err = obj->this_->wrapped_method();                                                                            \
    if (err) { NODE_THROW_LAST_CPLERR; }                                                                               \
    return info.Env().Undefined();                                                                                     \
  }

#define NODE_WRAPPED_METHOD_WITH_CPLERR_RESULT_1_WRAPPED_PARAM(klass, method, wrapped_method, param_type, param_name)  \
  NAN_METHOD(klass::method) {                                                                                          \
    param_type *param;                                                                                                 \
    NODE_ARG_WRAPPED(0, #param_name, param_type, param);                                                               \
    klass *obj = node_gdal::UnwrapWrapped<klass>(info.This().As<Napi::Object>());                                      \
    if (!obj->isAlive()) {                                                                                             \
      Napi::Error::New(info.Env(), #klass " object has already been destroyed").ThrowAsJavaScriptException();          \
      return info.Env().Undefined();                                                                                    \
    }                                                                                                                  \
    int err = obj->this_->wrapped_method(param->get());                                                                \
    if (err) { NODE_THROW_LAST_CPLERR; }                                                                               \
    return info.Env().Undefined();                                                                                     \
  }

#define NODE_WRAPPED_METHOD_WITH_CPLERR_RESULT_1_STRING_PARAM(klass, method, wrapped_method, param_name)               \
  NAN_METHOD(klass::method) {                                                                                          \
    std::string param;                                                                                                 \
    NODE_ARG_STR(0, #param_name, param);                                                                               \
    klass *obj = node_gdal::UnwrapWrapped<klass>(info.This().As<Napi::Object>());                                      \
    if (!obj->isAlive()) {                                                                                             \
      Napi::Error::New(info.Env(), #klass " object has already been destroyed").ThrowAsJavaScriptException();          \
      return info.Env().Undefined();                                                                                    \
    }                                                                                                                  \
    int err = obj->this_->wrapped_method(param.c_str());                                                               \
    if (err) { NODE_THROW_LAST_CPLERR; }                                                                               \
    return info.Env().Undefined();                                                                                     \
  }

#define NODE_WRAPPED_METHOD_WITH_CPLERR_RESULT_1_INTEGER_PARAM(klass, method, wrapped_method, param_name)              \
  NAN_METHOD(klass::method) {                                                                                          \
    int param;                                                                                                         \
    NODE_ARG_INT(0, #param_name, param);                                                                               \
    klass *obj = node_gdal::UnwrapWrapped<klass>(info.This().As<Napi::Object>());                                      \
    if (!obj->isAlive()) {                                                                                             \
      Napi::Error::New(info.Env(), #klass " object has already been destroyed").ThrowAsJavaScriptException();          \
      return info.Env().Undefined();                                                                                    \
    }                                                                                                                  \
    int err = obj->this_->wrapped_method(param);                                                                       \
    if (err) { NODE_THROW_LAST_CPLERR; }                                                                               \
    return info.Env().Undefined();                                                                                     \
  }

#define NODE_WRAPPED_METHOD_WITH_CPLERR_RESULT_1_INTEGER_PARAM_LOCKED(klass, method, wrapped_method, param_name)       \
  NAN_METHOD(klass::method) {                                                                                          \
    int param;                                                                                                         \
    NODE_ARG_INT(0, #param_name, param);                                                                               \
    klass *obj = node_gdal::UnwrapWrapped<klass>(info.This().As<Napi::Object>());                                      \
    if (!obj->isAlive()) {                                                                                             \
      Napi::Error::New(info.Env(), #klass " object has already been destroyed").ThrowAsJavaScriptException();          \
      return info.Env().Undefined();                                                                                    \
    }                                                                                                                  \
    int err = obj->this_->wrapped_method(param);                                                                       \
    if (err) { NODE_THROW_LAST_CPLERR; }                                                                               \
    return info.Env().Undefined();                                                                                     \
  }

#define NODE_WRAPPED_METHOD_WITH_CPLERR_RESULT_1_DOUBLE_PARAM(klass, method, wrapped_method, param_name)               \
  NAN_METHOD(klass::method) {                                                                                          \
    double param;                                                                                                      \
    NODE_ARG_DOUBLE(0, #param_name, param);                                                                            \
    klass *obj = node_gdal::UnwrapWrapped<klass>(info.This().As<Napi::Object>());                                      \
    if (!obj->isAlive()) {                                                                                             \
      Napi::Error::New(info.Env(), #klass " object has already been destroyed").ThrowAsJavaScriptException();          \
      return info.Env().Undefined();                                                                                    \
    }                                                                                                                  \
    GDAL_LOCK_PARENT(obj);                                                                                             \
    int err = obj->this_->wrapped_method(param);                                                                       \
    if (err) { NODE_THROW_LAST_CPLERR; }                                                                               \
    return info.Env().Undefined();                                                                                     \
  }

// ----- wrapped methods w/ OGRErr result (throws) -------

#define NODE_WRAPPED_METHOD_WITH_OGRERR_RESULT(klass, method, wrapped_method)                                          \
  NAN_METHOD(klass::method) {                                                                                          \
    klass *obj = node_gdal::UnwrapWrapped<klass>(info.This().As<Napi::Object>());                                      \
    if (!obj->isAlive()) {                                                                                             \
      Napi::Error::New(info.Env(), #klass " object has already been destroyed").ThrowAsJavaScriptException();          \
      return info.Env().Undefined();                                                                                    \
    }                                                                                                                  \
    int err = obj->this_->wrapped_method();                                                                            \
    if (err) { NODE_THROW_OGRERR(err); }                                                                               \
    return info.Env().Undefined();                                                                                     \
  }

#define NODE_WRAPPED_METHOD_WITH_OGRERR_RESULT_1_WRAPPED_PARAM(klass, method, wrapped_method, param_type, param_name)  \
  NAN_METHOD(klass::method) {                                                                                          \
    param_type *param;                                                                                                 \
    NODE_ARG_WRAPPED(0, #param_name, param_type, param);                                                               \
    klass *obj = node_gdal::UnwrapWrapped<klass>(info.This().As<Napi::Object>());                                      \
    if (!obj->isAlive()) {                                                                                             \
      Napi::Error::New(info.Env(), #klass " object has already been destroyed").ThrowAsJavaScriptException();          \
      return info.Env().Undefined();                                                                                    \
    }                                                                                                                  \
    int err = obj->this_->wrapped_method(param->get());                                                                \
    if (err) { NODE_THROW_OGRERR(err); }                                                                               \
    return info.Env().Undefined();                                                                                     \
  }

#define NODE_WRAPPED_METHOD_WITH_OGRERR_RESULT_1_STRING_PARAM(klass, method, wrapped_method, param_name)               \
  NAN_METHOD(klass::method) {                                                                                          \
    std::string param;                                                                                                 \
    NODE_ARG_STR(0, #param_name, param);                                                                               \
    klass *obj = node_gdal::UnwrapWrapped<klass>(info.This().As<Napi::Object>());                                      \
    if (!obj->isAlive()) {                                                                                             \
      Napi::Error::New(info.Env(), #klass " object has already been destroyed").ThrowAsJavaScriptException();          \
      return info.Env().Undefined();                                                                                    \
    }                                                                                                                  \
    int err = obj->this_->wrapped_method(param.c_str());                                                               \
    if (err) { NODE_THROW_OGRERR(err); }                                                                               \
    return info.Env().Undefined();                                                                                     \
  }

#define NODE_WRAPPED_METHOD_WITH_OGRERR_RESULT_1_INTEGER_PARAM(klass, method, wrapped_method, param_name)              \
  NAN_METHOD(klass::method) {                                                                                          \
    int param;                                                                                                         \
    NODE_ARG_INT(0, #param_name, param);                                                                               \
    klass *obj = node_gdal::UnwrapWrapped<klass>(info.This().As<Napi::Object>());                                      \
    if (!obj->isAlive()) {                                                                                             \
      Napi::Error::New(info.Env(), #klass " object has already been destroyed").ThrowAsJavaScriptException();          \
      return info.Env().Undefined();                                                                                    \
    }                                                                                                                  \
    int err = obj->this_->wrapped_method(param);                                                                       \
    if (err) { NODE_THROW_OGRERR(err); }                                                                               \
    return info.Env().Undefined();                                                                                     \
  }

#define NODE_WRAPPED_METHOD_WITH_OGRERR_RESULT_1_DOUBLE_PARAM(klass, method, wrapped_method, param_name)               \
  NAN_METHOD(klass::method) {                                                                                          \
    double param;                                                                                                      \
    NODE_ARG_DOUBLE(0, #param_name, param);                                                                            \
    klass *obj = node_gdal::UnwrapWrapped<klass>(info.This().As<Napi::Object>());                                      \
    if (!obj->isAlive()) {                                                                                             \
      Napi::Error::New(info.Env(), #klass " object has already been destroyed").ThrowAsJavaScriptException();          \
      return info.Env().Undefined();                                                                                    \
    }                                                                                                                  \
    int err = obj->this_->wrapped_method(param);                                                                       \
    if (err) { NODE_THROW_OGRERR(err); }                                                                               \
    return info.Env().Undefined();                                                                                     \
  }

#define NODE_WRAPPED_METHOD_WITH_OGRERR_RESULT_LOCKED(klass, method, wrapped_method)                                   \
  NAN_METHOD(klass::method) {                                                                                          \
    klass *obj = node_gdal::UnwrapWrapped<klass>(info.This().As<Napi::Object>());                                      \
    if (!obj->isAlive()) {                                                                                             \
      Napi::Error::New(info.Env(), #klass " object has already been destroyed").ThrowAsJavaScriptException();          \
      return info.Env().Undefined();                                                                                    \
    }                                                                                                                  \
    GDAL_LOCK_PARENT(obj);                                                                                             \
    int err = obj->this_->wrapped_method();                                                                            \
    if (err) { NODE_THROW_OGRERR(err); }                                                                               \
    return info.Env().Undefined();                                                                                     \
  }

// ----- wrapped asyncable methods -------

#define NODE_WRAPPED_ASYNC_METHOD(klass, method, wrapped_method)                                                       \
  GDAL_ASYNCABLE_DEFINE(klass::method) {                                                                               \
    klass *obj = node_gdal::UnwrapWrapped<klass>(info.This().As<Napi::Object>());                                      \
    if (!obj->isAlive()) {                                                                                             \
      Napi::Error::New(info.Env(), #klass " object has already been destroyed").ThrowAsJavaScriptException();          \
      return info.Env().Undefined();                                                                                    \
    }                                                                                                                  \
    auto *gdal_obj = obj->this_;                                                                                       \
    GDALAsyncableJob<int> job(0);                                                                                      \
    job.main = [gdal_obj](const GDALExecutionProgress &) {                                                             \
      gdal_obj->wrapped_method();                                                                                      \
      return 0;                                                                                                        \
    };                                                                                                                 \
    job.rval = [](int, const GetFromPersistentFunc &) { return v8_undefined(); };                                      \
    return job.run(info, async, 0);                                                                                    \
  }

#define NODE_WRAPPED_ASYNC_METHOD_WITH_RESULT(klass, async_type, method, result_type, wrapped_method)                  \
  GDAL_ASYNCABLE_DEFINE(klass::method) {                                                                               \
    klass *obj = node_gdal::UnwrapWrapped<klass>(info.This().As<Napi::Object>());                                      \
    if (!obj->isAlive()) {                                                                                             \
      Napi::Error::New(info.Env(), #klass " object has already been destroyed").ThrowAsJavaScriptException();          \
      return info.Env().Undefined();                                                                                    \
    }                                                                                                                  \
    auto *gdal_obj = obj->this_;                                                                                       \
    GDALAsyncableJob<async_type> job(0);                                                                               \
    job.main = [gdal_obj](const GDALExecutionProgress &) { return gdal_obj->wrapped_method(); };                       \
    job.rval = [](async_type r, const GetFromPersistentFunc &) { return node_gdal::ToNapi(node_gdal::napi_env, r); };              \
    return job.run(info, async, 0);                                                                                    \
  }

#define NODE_WRAPPED_ASYNC_METHOD_WITH_RESULT_1_WRAPPED_PARAM(                                                         \
  klass, async_type, method, result_type, wrapped_method, param_type, param_name)                                      \
  GDAL_ASYNCABLE_DEFINE(klass::method) {                                                                               \
    param_type *param;                                                                                                 \
    NODE_ARG_WRAPPED(0, #param_name, param_type, param);                                                               \
    klass *obj = node_gdal::UnwrapWrapped<klass>(info.This().As<Napi::Object>());                                      \
    if (!obj->isAlive()) {                                                                                             \
      Napi::Error::New(info.Env(), #klass " object has already been destroyed").ThrowAsJavaScriptException();          \
      return info.Env().Undefined();                                                                                    \
    }                                                                                                                  \
    auto *gdal_obj = obj->this_;                                                                                       \
    auto *gdal_param = param->get();                                                                                   \
    GDALAsyncableJob<async_type> job(0);                                                                               \
    job.persist(info[0].As<Napi::Object>());                                                                           \
    job.main = [gdal_obj, gdal_param](const GDALExecutionProgress &) {                                                 \
      return gdal_obj->wrapped_method(gdal_param);                                                                     \
    };                                                                                                                 \
    job.rval = [](async_type r, const GetFromPersistentFunc &) { return node_gdal::ToNapi(node_gdal::napi_env, r); };              \
    return job.run(info, async, 1);                                                                                    \
  }

#define NODE_WRAPPED_ASYNC_METHOD_WITH_RESULT_1_ENUM_PARAM(                                                            \
  klass, async_type, method, result_type, wrapped_method, enum_type, param_name)                                       \
  GDAL_ASYNCABLE_DEFINE(klass::method) {                                                                               \
    enum_type param;                                                                                                   \
    NODE_ARG_ENUM(0, #param_name, enum_type, param);                                                                   \
    klass *obj = node_gdal::UnwrapWrapped<klass>(info.This().As<Napi::Object>());                                      \
    if (!obj->isAlive()) {                                                                                             \
      Napi::Error::New(info.Env(), #klass " object has already been destroyed").ThrowAsJavaScriptException();          \
      return info.Env().Undefined();                                                                                    \
    }                                                                                                                  \
    auto *gdal_obj = obj->this_;                                                                                       \
    GDALAsyncableJob<async_type> job(0);                                                                               \
    job.main = [gdal_obj, param](const GDALExecutionProgress &) { return gdal_obj->wrapped_method(param); };           \
    job.rval = [](async_type r, const GetFromPersistentFunc &) { return node_gdal::ToNapi(node_gdal::napi_env, r); };              \
    return job.run(info, async, 1);                                                                                    \
  }

#define NODE_WRAPPED_ASYNC_METHOD_WITH_OGRERR_RESULT_LOCKED(klass, method, wrapped_method)                             \
  GDAL_ASYNCABLE_DEFINE(klass::method) {                                                                               \
    klass *obj = node_gdal::UnwrapWrapped<klass>(info.This().As<Napi::Object>());                                      \
    if (!obj->isAlive()) {                                                                                             \
      Napi::Error::New(info.Env(), #klass " object has already been destroyed").ThrowAsJavaScriptException();          \
      return info.Env().Undefined();                                                                                    \
    }                                                                                                                  \
    auto gdal_obj = obj->this_;                                                                                        \
    GDALAsyncableJob<OGRErr> job(obj->parent_uid);                                                                     \
    job.main = [gdal_obj](const GDALExecutionProgress &) {                                                             \
      int err = gdal_obj->wrapped_method();                                                                            \
      if (err) throw getOGRErrMsg(err);                                                                                \
      return err;                                                                                                      \
    };                                                                                                                 \
    job.rval = [](OGRErr, const GetFromPersistentFunc &) { return v8_undefined(); };                                   \
    return job.run(info, async, 0);                                                                                    \
  }

#define NODE_WRAPPED_ASYNC_METHOD_WITH_OGRERR_RESULT_1_WRAPPED_PARAM(                                                  \
  klass, async_type, method, wrapped_method, param_type, param_name)                                                   \
  GDAL_ASYNCABLE_DEFINE(klass::method) {                                                                               \
    param_type *param;                                                                                                 \
    NODE_ARG_WRAPPED(0, #param_name, param_type, param);                                                               \
    klass *obj = node_gdal::UnwrapWrapped<klass>(info.This().As<Napi::Object>());                                      \
    if (!obj->isAlive()) {                                                                                             \
      Napi::Error::New(info.Env(), #klass " object has already been destroyed").ThrowAsJavaScriptException();          \
      return info.Env().Undefined();                                                                                    \
    }                                                                                                                  \
    auto gdal_obj = obj->this_;                                                                                        \
    auto gdal_param = param->get();                                                                                    \
    GDALAsyncableJob<async_type> job(0);                                                                               \
    job.persist(info[0].As<Napi::Object>());                                                                           \
    job.main = [gdal_obj, gdal_param](const GDALExecutionProgress &) {                                                 \
      int err = gdal_obj->wrapped_method(gdal_param);                                                                  \
      if (err) throw getOGRErrMsg(err);                                                                                \
      return err;                                                                                                      \
    };                                                                                                                 \
    job.rval = [](async_type, const GetFromPersistentFunc &) { return v8_undefined(); };                               \
    return job.run(info, async, 1);                                                                                    \
  }

// ----- constants -------

#define NODE_DEFINE_CONSTANT(target, constant) target.Set(#constant, Napi::Number::New(target.Env(), constant))
#define NODE_DEFINE_CONSTANT_HEX(target, constant)                                                                     \
  target.Set(#constant, Napi::Number::New(target.Env(), constant))

// ----- class registration helpers -------
//
// Used inside a class' `Initialize`, where `SELF` must name the class:
//
//   Napi::Function func = DefineClass(env, "Driver", {
//     METHOD(toString), METHOD_ASYNCABLE(open), ATTR(lcons, "description", descriptionGetter, READ_ONLY_SETTER),
//   });
//
#define SELF_CLASS(S) using SELF = S;

#define METHOD(name) GDALInstanceMethodT<SELF, &SELF::name>(#name),
#define METHOD_ASYNCABLE(name)                                                                                         \
  GDALInstanceMethodT<SELF, &SELF::name>(#name), GDALInstanceMethodT<SELF, &SELF::name##Async>(#name "Async"),
#define METHOD_HIDDEN(name) GDALInstanceMethodHiddenT<SELF, &SELF::name>(#name),

// Same, but for the ~20 members whose JS name differs from the C++ name
// (eg. the JS "toWKT" is Geometry::exportToWKT, and "isVectical" really is
// spelled that way in the public API)
#define METHOD_AS(name, method) GDALInstanceMethodT<SELF, &SELF::method>(name),
#define METHOD_ASYNCABLE_AS(name, method)                                                                              \
  GDALInstanceMethodT<SELF, &SELF::method>(name), GDALInstanceMethodT<SELF, &SELF::method##Async>(name "Async"),

#define ATTR(t, name, get, set) GDALInstanceAccessorT<SELF, &SELF::get, &set>(name),
#define ATTR_DONT_ENUM(t, name, get, set) GDALInstanceAccessorHiddenT<SELF, &SELF::get>(name),
#define ATTR_ASYNCABLE(t, name, get, set)                                                                              \
  GDALInstanceAccessorT<SELF, &SELF::get, &set>(name),                                                                 \
    GDALInstanceAccessorHiddenT<SELF, &SELF::get##Async>(name "Async"),

NAN_SETTER(READ_ONLY_SETTER);

// module-level asyncable free functions
#define GDAL_SetAsyncableMethod(env, target, name, method)                                                             \
  target.Set(name, Napi::Function::New(env, method));                                                                  \
  target.Set(name "Async", Napi::Function::New(env, method##Async));

// module-level plain free function
#define GDAL_SetMethod(env, target, name, method) target.Set(name, Napi::Function::New(env, method));
#endif
