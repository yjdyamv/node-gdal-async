#ifndef __GDAL_COMMON_NAPI_H__
#define __GDAL_COMMON_NAPI_H__

#pragma warning(push)
#pragma warning(disable: 4100)
#include <napi.h>
#pragma warning(pop)

#include <cpl_error.h>
#include <gdal_version.h>
#include <string>

// ---------------------------------------------------------------------------
// SafeString equivalent – returns null for nullptr strings
// ---------------------------------------------------------------------------
inline Napi::Value SafeStringNapi(Napi::Env env, const char *data) {
  if (!data) return env.Null();
  return Napi::String::New(env, data);
}

// ---------------------------------------------------------------------------
// NAPI_ARG_* – required argument extraction
// ---------------------------------------------------------------------------

#define NAPI_ARG_INT(num, name, var)                                                          \
  do {                                                                                         \
    if (info.Length() < (num) + 1) {                                                            \
      Napi::Error::New(info.Env(), name " must be given").ThrowAsJavaScriptException();         \
      return info.Env().Undefined();                                                            \
    }                                                                                           \
    if (!info[num].IsNumber()) {                                                                \
      Napi::TypeError::New(info.Env(), name " must be an integer").ThrowAsJavaScriptException();\
      return info.Env().Undefined();                                                            \
    }                                                                                           \
    var = info[num].As<Napi::Number>().Int32Value();                                           \
  } while (0)

#define NAPI_ARG_DOUBLE(num, name, var)                                                       \
  do {                                                                                         \
    if (info.Length() < (num) + 1) {                                                            \
      Napi::Error::New(info.Env(), name " must be given").ThrowAsJavaScriptException();         \
      return info.Env().Undefined();                                                            \
    }                                                                                           \
    if (!info[num].IsNumber()) {                                                                \
      Napi::TypeError::New(info.Env(), name " must be a number").ThrowAsJavaScriptException();  \
      return info.Env().Undefined();                                                            \
    }                                                                                           \
    var = info[num].As<Napi::Number>().DoubleValue();                                          \
  } while (0)

#define NAPI_ARG_STR(num, name, var)                                                          \
  do {                                                                                         \
    if (info.Length() < (num) + 1) {                                                            \
      Napi::Error::New(info.Env(), name " must be given").ThrowAsJavaScriptException();         \
      return info.Env().Undefined();                                                            \
    }                                                                                           \
    if (!info[num].IsString()) {                                                                \
      Napi::TypeError::New(info.Env(), name " must be a string").ThrowAsJavaScriptException();  \
      return info.Env().Undefined();                                                            \
    }                                                                                           \
    var = info[num].As<Napi::String>().Utf8Value();                                            \
  } while (0)

#define NAPI_ARG_BOOL(num, name, var)                                                         \
  do {                                                                                         \
    if (info.Length() < (num) + 1) {                                                            \
      Napi::Error::New(info.Env(), name " must be given").ThrowAsJavaScriptException();         \
      return info.Env().Undefined();                                                            \
    }                                                                                           \
    if (!info[num].IsBoolean()) {                                                               \
      Napi::TypeError::New(info.Env(), name " must be a boolean").ThrowAsJavaScriptException(); \
      return info.Env().Undefined();                                                            \
    }                                                                                           \
    var = info[num].As<Napi::Boolean>().Value();                                               \
  } while (0)

#define NAPI_ARG_ENUM(num, name, enum_type, var)                                              \
  do {                                                                                         \
    if (info.Length() < (num) + 1) {                                                            \
      Napi::Error::New(info.Env(), name " must be given").ThrowAsJavaScriptException();         \
      return info.Env().Undefined();                                                            \
    }                                                                                           \
    if (!info[num].IsNumber()) {                                                                \
      Napi::TypeError::New(info.Env(), name " must be an integer").ThrowAsJavaScriptException();\
      return info.Env().Undefined();                                                            \
    }                                                                                           \
    var = static_cast<enum_type>(info[num].As<Napi::Number>().Uint32Value());                   \
  } while (0)

#define NAPI_ARG_ARRAY(num, name, var)                                                        \
  do {                                                                                         \
    if (info.Length() < (num) + 1) {                                                            \
      Napi::Error::New(info.Env(), name " must be given").ThrowAsJavaScriptException();         \
      return info.Env().Undefined();                                                            \
    }                                                                                           \
    if (!info[num].IsArray()) {                                                                 \
      Napi::TypeError::New(info.Env(), name " must be an array").ThrowAsJavaScriptException();  \
      return info.Env().Undefined();                                                            \
    }                                                                                           \
    var = info[num].As<Napi::Array>();                                                          \
  } while (0)

#define NAPI_ARG_OBJECT(num, name, var)                                                       \
  do {                                                                                         \
    if (info.Length() < (num) + 1) {                                                            \
      Napi::Error::New(info.Env(), name " must be given").ThrowAsJavaScriptException();         \
      return info.Env().Undefined();                                                            \
    }                                                                                           \
    if (!info[num].IsObject()) {                                                                \
      Napi::TypeError::New(info.Env(), name " must be an object").ThrowAsJavaScriptException(); \
      return info.Env().Undefined();                                                            \
    }                                                                                           \
    var = info[num].As<Napi::Object>();                                                         \
  } while (0)

// ---------------------------------------------------------------------------
// NAPI_ARG_*_OPT – optional argument extraction
// ---------------------------------------------------------------------------

#define NAPI_ARG_INT_OPT(num, name, var)                                                      \
  do {                                                                                         \
    var = 0;                                                                                    \
    if (info.Length() > (num)) {                                                                \
      if (info[num].IsNumber()) {                                                               \
        var = info[num].As<Napi::Number>().Int32Value();                                       \
      } else if (!info[num].IsNull() && !info[num].IsUndefined()) {                             \
        Napi::TypeError::New(info.Env(), name " must be an integer").ThrowAsJavaScriptException();\
        return info.Env().Undefined();                                                          \
      }                                                                                         \
    }                                                                                           \
  } while (0)

#define NAPI_ARG_DOUBLE_OPT(num, name, var)                                                   \
  do {                                                                                         \
    var = 0.0;                                                                                  \
    if (info.Length() > (num)) {                                                                \
      if (info[num].IsNumber()) {                                                               \
        var = info[num].As<Napi::Number>().DoubleValue();                                      \
      } else if (!info[num].IsNull() && !info[num].IsUndefined()) {                             \
        Napi::TypeError::New(info.Env(), name " must be a number").ThrowAsJavaScriptException();\
        return info.Env().Undefined();                                                          \
      }                                                                                         \
    }                                                                                           \
  } while (0)

#define NAPI_ARG_OPT_STR(num, name, var)                                                      \
  do {                                                                                         \
    var = "";                                                                                   \
    if (info.Length() > (num)) {                                                                \
      if (info[num].IsString()) {                                                               \
        var = info[num].As<Napi::String>().Utf8Value();                                        \
      } else if (!info[num].IsNull() && !info[num].IsUndefined()) {                             \
        Napi::TypeError::New(info.Env(), name " must be a string").ThrowAsJavaScriptException();\
        return info.Env().Undefined();                                                          \
      }                                                                                         \
    }                                                                                           \
  } while (0)

#define NAPI_ARG_BOOL_OPT(num, name, var)                                                     \
  do {                                                                                         \
    var = false;                                                                                \
    if (info.Length() > (num)) {                                                                \
      if (info[num].IsBoolean()) {                                                              \
        var = info[num].As<Napi::Boolean>().Value();                                           \
      } else if (!info[num].IsNull() && !info[num].IsUndefined()) {                             \
        Napi::TypeError::New(info.Env(), name " must be a boolean").ThrowAsJavaScriptException();\
        return info.Env().Undefined();                                                          \
      }                                                                                         \
    }                                                                                           \
  } while (0)

#define NAPI_ARG_ENUM_OPT(num, name, enum_type, var)                                          \
  do {                                                                                         \
    var = static_cast<enum_type>(0);                                                            \
    if (info.Length() > (num)) {                                                                \
      if (info[num].IsNumber()) {                                                               \
        var = static_cast<enum_type>(info[num].As<Napi::Number>().Uint32Value());               \
      } else if (!info[num].IsNull() && !info[num].IsUndefined()) {                             \
        Napi::TypeError::New(info.Env(), name " must be an integer").ThrowAsJavaScriptException();\
        return info.Env().Undefined();                                                          \
      }                                                                                         \
    }                                                                                           \
  } while (0)

#define NAPI_ARG_OBJECT_OPT(num, name, var)                                                   \
  do {                                                                                         \
    var = Napi::Object();                                                                       \
    if (info.Length() > (num) && !info[num].IsNull() && !info[num].IsUndefined()) {             \
      if (!info[num].IsObject()) {                                                              \
        Napi::TypeError::New(info.Env(), name " must be an object").ThrowAsJavaScriptException();\
        return info.Env().Undefined();                                                          \
      }                                                                                         \
      var = info[num].As<Napi::Object>();                                                       \
    }                                                                                           \
  } while (0)

#define NAPI_ARG_ARRAY_OPT(num, name, var)                                                    \
  do {                                                                                         \
    var = Napi::Array();                                                                        \
    if (info.Length() > (num)) {                                                                \
      if (info[num].IsArray()) {                                                                \
        var = info[num].As<Napi::Array>();                                                      \
      } else if (!info[num].IsNull() && !info[num].IsUndefined()) {                             \
        Napi::TypeError::New(info.Env(), name " must be an array").ThrowAsJavaScriptException();\
        return info.Env().Undefined();                                                          \
      }                                                                                         \
    }                                                                                           \
  } while (0)

// ---------------------------------------------------------------------------
// Wrapped argument – for Napi::ObjectWrap<T> instances
// ---------------------------------------------------------------------------

#define NAPI_ARG_WRAPPED(num, name, type, var)                                                \
  do {                                                                                         \
    if (info.Length() < (num) + 1) {                                                            \
      Napi::Error::New(info.Env(), name " must be given").ThrowAsJavaScriptException();         \
      return info.Env().Undefined();                                                            \
    }                                                                                           \
    if (info[num].IsNull() || info[num].IsUndefined() ||                                       \
        !info[num].As<Napi::Object>().InstanceOf(type::constructor.Value())) {                  \
      Napi::TypeError::New(info.Env(), name " must be an instance of " #type)                  \
        .ThrowAsJavaScriptException();                                                          \
      return info.Env().Undefined();                                                            \
    }                                                                                           \
    var = type::Unwrap(info[num].As<Napi::Object>());                                           \
    if (!var || !var->isAlive()) {                                                              \
      Napi::Error::New(info.Env(), #type " parameter already destroyed")                       \
        .ThrowAsJavaScriptException();                                                          \
      return info.Env().Undefined();                                                            \
    }                                                                                           \
  } while (0)

#define NAPI_ARG_WRAPPED_OPT(num, name, type, var)                                            \
  do {                                                                                         \
    var = nullptr;                                                                              \
    if (info.Length() > (num) && !info[num].IsNull() && !info[num].IsUndefined()) {             \
      if (!info[num].As<Napi::Object>().InstanceOf(type::constructor.Value())) {                \
        Napi::TypeError::New(info.Env(), name " must be an instance of " #type)                \
          .ThrowAsJavaScriptException();                                                        \
        return info.Env().Undefined();                                                          \
      }                                                                                         \
      var = type::Unwrap(info[num].As<Napi::Object>());                                         \
      if (!var || !var->isAlive()) {                                                            \
        Napi::Error::New(info.Env(), #type " parameter already destroyed")                     \
          .ThrowAsJavaScriptException();                                                        \
        return info.Env().Undefined();                                                          \
      }                                                                                         \
    }                                                                                           \
  } while (0)

// ---------------------------------------------------------------------------
// Instance-of check
// ---------------------------------------------------------------------------
#define IS_WRAPPED_NAPI(obj, type) (obj.As<Napi::Object>().InstanceOf(type::constructor.Value()))

// ---------------------------------------------------------------------------
// Unwrap with type and alive check
// ---------------------------------------------------------------------------
#define NAPI_UNWRAP_CHECK(type, obj, var)                                                     \
  do {                                                                                         \
    if (!obj.IsObject() || obj.IsNull() ||                                                     \
        !obj.As<Napi::Object>().InstanceOf(type::constructor.Value())) {                       \
      Napi::TypeError::New(obj.Env(), "Object must be a " #type " object")                     \
        .ThrowAsJavaScriptException();                                                         \
      return info.Env().Undefined();                                                            \
    }                                                                                           \
    type *var = type::Unwrap(obj.As<Napi::Object>());                                           \
    if (!var || !var->isAlive()) {                                                              \
      Napi::Error::New(obj.Env(), #type " object has already been destroyed")                  \
        .ThrowAsJavaScriptException();                                                         \
      return info.Env().Undefined();                                                            \
    }                                                                                           \
  } while (0)

// ---------------------------------------------------------------------------
// Object property extraction
// ---------------------------------------------------------------------------

#define NAPI_DOUBLE_FROM_OBJ(obj, key, var)                                                   \
  do {                                                                                         \
    if (!obj.Has(key)) {                                                                        \
      Napi::Error::New(obj.Env(), "Object must contain property \"" key "\"")                   \
        .ThrowAsJavaScriptException();                                                         \
      return info.Env().Undefined();                                                            \
    }                                                                                           \
    Napi::Value val = obj.Get(key);                                                             \
    if (!val.IsNumber()) {                                                                      \
      Napi::TypeError::New(obj.Env(), "Property \"" key "\" must be a number")                  \
        .ThrowAsJavaScriptException();                                                         \
      return info.Env().Undefined();                                                            \
    }                                                                                           \
    var = val.As<Napi::Number>().DoubleValue();                                                 \
  } while (0)

#define NAPI_INT_FROM_OBJ(obj, key, var)                                                      \
  do {                                                                                         \
    if (!obj.Has(key)) {                                                                        \
      Napi::Error::New(obj.Env(), "Object must contain property \"" key "\"")                   \
        .ThrowAsJavaScriptException();                                                         \
      return info.Env().Undefined();                                                            \
    }                                                                                           \
    Napi::Value val = obj.Get(key);                                                             \
    if (!val.IsNumber()) {                                                                      \
      Napi::TypeError::New(obj.Env(), "Property \"" key "\" must be a number")                  \
        .ThrowAsJavaScriptException();                                                         \
      return info.Env().Undefined();                                                            \
    }                                                                                           \
    var = val.As<Napi::Number>().Int32Value();                                                  \
  } while (0)

#define NAPI_STR_FROM_OBJ(obj, key, var)                                                      \
  do {                                                                                         \
    if (!obj.Has(key)) {                                                                        \
      Napi::Error::New(obj.Env(), "Object must contain property \"" key "\"")                   \
        .ThrowAsJavaScriptException();                                                         \
      return info.Env().Undefined();                                                            \
    }                                                                                           \
    Napi::Value val = obj.Get(key);                                                             \
    if (!val.IsString()) {                                                                      \
      Napi::TypeError::New(obj.Env(), "Property \"" key "\" must be a string")                  \
        .ThrowAsJavaScriptException();                                                         \
      return info.Env().Undefined();                                                            \
    }                                                                                           \
    var = val.As<Napi::String>().Utf8Value();                                                   \
  } while (0)

#define NAPI_ARRAY_FROM_OBJ(obj, key, var)                                                    \
  do {                                                                                         \
    if (!obj.Has(key)) {                                                                        \
      Napi::Error::New(obj.Env(), "Object must contain property \"" key "\"")                   \
        .ThrowAsJavaScriptException();                                                         \
      return info.Env().Undefined();                                                            \
    }                                                                                           \
    Napi::Value val = obj.Get(key);                                                             \
    if (!val.IsArray()) {                                                                       \
      Napi::TypeError::New(obj.Env(), "Property \"" key "\" must be an array")                  \
        .ThrowAsJavaScriptException();                                                         \
      return info.Env().Undefined();                                                            \
    }                                                                                           \
    var = val.As<Napi::Array>();                                                                \
  } while (0)

// ---------------------------------------------------------------------------
// Callback extraction
// ---------------------------------------------------------------------------

#define NAPI_ARG_CB(num, name, var)                                                           \
  do {                                                                                         \
    if (info.Length() < (num) + 1) {                                                            \
      Napi::Error::New(info.Env(), name " must be given").ThrowAsJavaScriptException();         \
      return info.Env().Undefined();                                                            \
    }                                                                                           \
    if (!info[num].IsFunction()) {                                                              \
      Napi::TypeError::New(info.Env(), name " must be a function").ThrowAsJavaScriptException();\
      return info.Env().Undefined();                                                            \
    }                                                                                           \
    var.Reset(info[num].As<Napi::Function>(), 1);                                                \
  } while (0)

#define NAPI_ARG_CB_OPT(num, name, var)                                                       \
  do {                                                                                         \
    var.Reset();                                                                                \
    if (info.Length() > (num) && !info[num].IsNull() && !info[num].IsUndefined()) {             \
      if (info[num].IsFunction()) {                                                             \
        var.Reset(info[num].As<Napi::Function>(), 1);                                            \
      } else {                                                                                  \
        Napi::TypeError::New(info.Env(), name " must be a function").ThrowAsJavaScriptException();\
        return info.Env().Undefined();                                                          \
      }                                                                                         \
    }                                                                                           \
  } while (0)

#define NAPI_CB_FROM_OBJ_OPT(obj, key, var)                                                  \
  do {                                                                                         \
    var.Reset();                                                                                \
    if (!obj.IsEmpty() && obj.Has(key)) {                                                       \
      Napi::Value val = obj.Get(key);                                                           \
      if (!val.IsFunction()) {                                                                  \
        Napi::TypeError::New(obj.Env(), "Property \"" key "\" must be a function")              \
          .ThrowAsJavaScriptException();                                                        \
        return info.Env().Undefined();                                                          \
      }                                                                                         \
      var.Reset(val.As<Napi::Function>(), 1);                                                    \
    }                                                                                           \
  } while (0)

// ---------------------------------------------------------------------------
// Error helpers
// ---------------------------------------------------------------------------

#define NAPI_THROW_LAST_CPLERR                                                               \
  do {                                                                                         \
    Napi::Error::New(info.Env(), CPLGetLastErrorMsg()).ThrowAsJavaScriptException();           \
    return info.Env().Undefined();                                                              \
  } while (0)

#define NAPI_THROW_OGRERR(err_code)                                                          \
  do {                                                                                         \
    const char *_msg;                                                                          \
    if ((err_code) == 6) {                                                                     \
      _msg = CPLGetLastErrorMsg();                                                              \
    } else {                                                                                   \
      switch (err_code) {                                                                       \
        case 0: _msg = "No error"; break;                                                       \
        case 1: _msg = "Not enough data"; break;                                                \
        case 2: _msg = "Not enough memory"; break;                                              \
        case 3: _msg = "Unsupported geometry type"; break;                                      \
        case 4: _msg = "Unsupported operation"; break;                                          \
        case 5: _msg = "Corrupt Data"; break;                                                   \
        case 7: _msg = "Unsupported SRS"; break;                                                \
        default: _msg = "Invalid Error"; break;                                                  \
      }                                                                                         \
    }                                                                                           \
    Napi::Error::New(info.Env(), _msg).ThrowAsJavaScriptException();                            \
    return info.Env().Undefined();                                                              \
  } while (0)

// ---------------------------------------------------------------------------
// Property accessor registration
// ---------------------------------------------------------------------------

#define ATTR_NAPI(t, name, get, set)                                                          \
  do {                                                                                         \
    t.InstanceAccessor(name, get, set);                                                        \
  } while (0)

#define ATTR_NAPI_DONT_ENUM(t, name, get, set)                                                \
  do {                                                                                         \
    t.InstanceAccessor(name, get, set, napi_enumerable, nullptr, nullptr, nullptr);             \
  } while (0)

// ---------------------------------------------------------------------------
// Field ID resolution (by name or index)
// ---------------------------------------------------------------------------
#define ARG_FIELD_ID_NAPI(num, f, var)                                                        \
  do {                                                                                         \
    if (info[num].IsString()) {                                                                 \
      std::string field_name = info[num].As<Napi::String>().Utf8Value();                        \
      var = f->GetFieldIndex(field_name.c_str());                                               \
      if (var == -1) {                                                                          \
        Napi::Error::New(info.Env(), "Specified field name does not exist")                     \
          .ThrowAsJavaScriptException();                                                        \
        return info.Env().Undefined();                                                          \
      }                                                                                         \
    } else if (info[num].IsNumber()) {                                                          \
      var = info[num].As<Napi::Number>().Int32Value();                                          \
      if (var < 0 || var >= f->GetFieldCount()) {                                               \
        Napi::RangeError::New(info.Env(), "Invalid field index").ThrowAsJavaScriptException();   \
        return info.Env().Undefined();                                                          \
      }                                                                                         \
    } else {                                                                                    \
      Napi::TypeError::New(info.Env(), "Field index must be integer or string")                  \
        .ThrowAsJavaScriptException();                                                          \
      return info.Env().Undefined();                                                            \
    }                                                                                           \
  } while (0)

// ---------------------------------------------------------------------------
// Progress callback from [options] argument
// ---------------------------------------------------------------------------
#define NAPI_PROGRESS_CB_OPT(num, progress_cb, job)                                           \
  do {                                                                                         \
    progress_cb.Reset();                                                                        \
    if (info.Length() > (num) && !info[num].IsNull() && !info[num].IsUndefined()) {             \
      if (!info[num].IsObject()) {                                                              \
        Napi::TypeError::New(info.Env(), "options must be an object")                           \
          .ThrowAsJavaScriptException();                                                        \
        return info.Env().Undefined();                                                          \
      }                                                                                         \
      Napi::Object progress_obj = info[num].As<Napi::Object>();                                 \
      NAPI_CB_FROM_OBJ_OPT(progress_obj, "progress_cb", progress_cb);                           \
    }                                                                                           \
  } while (0)

// ---------------------------------------------------------------------------
// MajorObject – parse GDAL key=value metadata into a JS object
// ---------------------------------------------------------------------------
inline Napi::Object MajorObjectNapiGetMetadata(Napi::Env env, char **metadata) {
  Napi::Object result = Napi::Object::New(env);
  if (metadata) {
    for (int i = 0; metadata[i]; i++) {
      std::string pair = metadata[i];
      std::size_t eq = pair.find_first_of('=');
      if (eq != std::string::npos) {
        result.Set(pair.substr(0, eq), Napi::String::New(env, pair.substr(eq + 1)));
      }
    }
  }
  return result;
}

#endif // __GDAL_COMMON_NAPI_H__
