#include "gdal_geometry_napi.hpp"
#include "gdal_simplecurve_napi.hpp"
#include "gdal_circularstring_napi.hpp"
#include "../gdal_stubs_napi.hpp"

namespace node_gdal {

Napi::FunctionReference CircularStringNapi::constructor;

Napi::Object CircularStringNapi::Init(Napi::Env env, Napi::Object exports) {
  Napi::Function func = DefineClass(
    env, "CircularString",
    {
      InstanceMethod("toString", &CircularStringNapi::toString),
      InstanceAccessor<&CircularStringNapi::pointsGetter>("points"),
    });
  constructor = Napi::Persistent(func);
  NapiSetPrototypeChain(env, func, SimpleCurveNapi::constructor.Value());
  GeometryNapi::AddInheritedMethods(env, func);
  constructor.SuppressDestruct();
  exports.Set("CircularString", func);
  return exports;
}

CircularStringNapi::CircularStringNapi(const Napi::CallbackInfo &info)
  : Napi::ObjectWrap<CircularStringNapi>(info) {

  if (!info.IsConstructCall()) {
    Napi::Error::New(info.Env(), "Cannot call constructor as function, use 'new' keyword")
      .ThrowAsJavaScriptException();
    return;
  }

  if (info.Length() > 0 && info[0].IsExternal()) {
    this_ = info[0].As<Napi::External<OGRCircularString>>().Data();
    owned_ = true;
  } else {
    if (info.Length() != 0) {
      Napi::Error::New(info.Env(), "CircularString constructor doesn't take any arguments")
        .ThrowAsJavaScriptException();
      return;
    }
    this_ = new OGRCircularString();
    owned_ = true;
  }
}

Napi::Value CircularStringNapi::toString(const Napi::CallbackInfo &info) {
  return Napi::String::New(info.Env(), "CircularString");
}

Napi::Value CircularStringNapi::pointsGetter(const Napi::CallbackInfo &info) {
  NAPI_UNWRAP_THIS(CircularStringNapi, self);
  Napi::Object thiz = info.This().As<Napi::Object>();
  if (thiz.Has("__points")) { Napi::Value c = thiz.Get("__points"); if (!c.IsNull() && !c.IsUndefined()) return c; }
  Napi::Object pts = LineStringPointsNapi::constructor.New({
    Napi::External<OGRLineString>::New(info.Env(), reinterpret_cast<OGRLineString *>(self->this_))
  });
  thiz.Set("__points", pts); return pts;
}

} // namespace node_gdal
