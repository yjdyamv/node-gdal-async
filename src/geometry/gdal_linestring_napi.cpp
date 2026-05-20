#include "gdal_linestring_napi.hpp"
#include "../gdal_stubs_napi.hpp"

namespace node_gdal {

Napi::FunctionReference LineStringNapi::constructor;

Napi::Object LineStringNapi::Init(Napi::Env env, Napi::Object exports) {
  Napi::Function func = DefineClass(
    env,
    "LineString",
    {
      InstanceMethod("toString", &LineStringNapi::toString),
      InstanceAccessor<&LineStringNapi::pointsGetter>("points"),
    });

  constructor = Napi::Persistent(func);
  constructor.SuppressDestruct();
  exports.Set("LineString", func);
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

Napi::Value LineStringNapi::pointsGetter(const Napi::CallbackInfo &info) {
  NAPI_UNWRAP_THIS(LineStringNapi, self);
  Napi::Object thiz = info.This().As<Napi::Object>();
  if (thiz.Has("__points")) {
    Napi::Value cached = thiz.Get("__points");
    if (!cached.IsNull() && !cached.IsUndefined()) return cached;
  }
  Napi::Object pts = LineStringPointsNapi::constructor.New({
    Napi::External<OGRLineString>::New(info.Env(), self->this_)
  });
  thiz.Set("__points", pts);
  return pts;
}

} // namespace node_gdal
