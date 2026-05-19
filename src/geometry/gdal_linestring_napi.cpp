#include "gdal_linestring_napi.hpp"

namespace node_gdal {

Napi::FunctionReference LineStringNapi::constructor;

Napi::Object LineStringNapi::Init(Napi::Env env, Napi::Object exports) {
  Napi::Function func = DefineClass(
    env,
    "LineStringNapi",
    {
      InstanceMethod("toString", &LineStringNapi::toString),
    });

  constructor = Napi::Persistent(func);
  constructor.SuppressDestruct();
  exports.Set("LineStringNapi", func);
  return exports;
}

LineStringNapi::LineStringNapi(const Napi::CallbackInfo &info)
  : Napi::ObjectWrap<LineStringNapi>(info) {

  if (!info.IsConstructCall()) {
    Napi::Error::New(info.Env(), "Cannot call constructor as function, use 'new' keyword")
      .ThrowAsJavaScriptException();
    return;
  }

  if (info.Length() > 0 && info[0].IsExternal()) {
    this_ = info[0].As<Napi::External<OGRLineString>>().Data();
    owned_ = true;
  } else {
    if (info.Length() != 0) {
      Napi::Error::New(info.Env(), "LineString constructor doesn't take any arguments")
        .ThrowAsJavaScriptException();
      return;
    }
    this_ = new OGRLineString();
    owned_ = true;
  }
}

Napi::Value LineStringNapi::toString(const Napi::CallbackInfo &info) {
  return Napi::String::New(info.Env(), "LineString");
}

} // namespace node_gdal
