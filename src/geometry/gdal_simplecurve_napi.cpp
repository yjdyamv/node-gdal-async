#include "gdal_geometry_napi.hpp"
#include "gdal_simplecurve_napi.hpp"
#include "gdal_point_napi.hpp"
#include "../gdal_stubs_napi.hpp"

namespace node_gdal {

Napi::FunctionReference SimpleCurveNapi::constructor;

Napi::Object SimpleCurveNapi::Init(Napi::Env env, Napi::Object exports) {
  Napi::Function func = DefineClass(
    env,
    "SimpleCurve",
    {
      InstanceMethod("toString", &SimpleCurveNapi::toString),
      InstanceMethod("value", &SimpleCurveNapi::value),
      InstanceMethod("getLength", &SimpleCurveNapi::getLength),
      InstanceAccessor<&SimpleCurveNapi::pointsGetter>("points"),
    });

  constructor = Napi::Persistent(func);
  NapiSetPrototypeChain(env, func, GeometryNapi::constructor.Value());
  GeometryNapi::AddInheritedMethods(env, func);
  constructor.SuppressDestruct();
  exports.Set("SimpleCurve", func);
  return exports;
}

SimpleCurveNapi::SimpleCurveNapi(const Napi::CallbackInfo &info)
  : Napi::ObjectWrap<SimpleCurveNapi>(info) {
  Napi::Error::New(info.Env(), "SimpleCurve is an abstract class and cannot be instantiated")
    .ThrowAsJavaScriptException();
}

Napi::Value SimpleCurveNapi::toString(const Napi::CallbackInfo &info) {
  return Napi::String::New(info.Env(), "SimpleCurve");
}

Napi::Value SimpleCurveNapi::value(const Napi::CallbackInfo &info) {
  NAPI_UNWRAP_THIS(SimpleCurveNapi, geom);
  double dist;
  NAPI_ARG_DOUBLE(0, "distance", dist);
  OGRPoint *pt = new OGRPoint();
  geom->this_->Value(dist, pt);
  return PointNapi::New(info.Env(), pt);
}

Napi::Value SimpleCurveNapi::getLength(const Napi::CallbackInfo &info) {
  NAPI_UNWRAP_THIS(SimpleCurveNapi, geom);
  return Napi::Number::New(info.Env(), geom->this_->get_Length());
}

Napi::Value SimpleCurveNapi::pointsGetter(const Napi::CallbackInfo &info) {
  NAPI_UNWRAP_THIS(SimpleCurveNapi, self);
  Napi::Object thiz = info.This().As<Napi::Object>();
  // Return cached wrapper if present
  if (thiz.Has("__points")) {
    Napi::Value cached = thiz.Get("__points");
    if (!cached.IsNull() && !cached.IsUndefined()) return cached;
  }
  // Create collection wrapper
  Napi::Object pts = LineStringPointsNapi::constructor.New({
    Napi::External<OGRLineString>::New(info.Env(),
      reinterpret_cast<OGRLineString *>(self->this_))
  });
  thiz.Set("__points", pts);
  return pts;
}

} // namespace node_gdal
