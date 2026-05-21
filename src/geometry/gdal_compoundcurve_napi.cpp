#include "gdal_simplecurve_napi.hpp"
#include "gdal_compoundcurve_napi.hpp"
#include "../gdal_stubs_napi.hpp"

namespace node_gdal {

Napi::FunctionReference CompoundCurveNapi::constructor;

Napi::Object CompoundCurveNapi::Init(Napi::Env env, Napi::Object exports) {
  Napi::Function func = DefineClass(
    env, "CompoundCurve",
    {
      InstanceMethod("toString", &CompoundCurveNapi::toString),
      InstanceAccessor<&CompoundCurveNapi::pointsGetter>("points"),
    });
  constructor = Napi::Persistent(func);
  NapiSetPrototypeChain(env, func, SimpleCurveNapi::constructor.Value());
  constructor.SuppressDestruct();
  exports.Set("CompoundCurve", func);
  return exports;
}

CompoundCurveNapi::CompoundCurveNapi(const Napi::CallbackInfo &info)
  : Napi::ObjectWrap<CompoundCurveNapi>(info) {

  if (!info.IsConstructCall()) {
    Napi::Error::New(info.Env(), "Cannot call constructor as function, use 'new' keyword")
      .ThrowAsJavaScriptException();
    return;
  }

  if (info.Length() > 0 && info[0].IsExternal()) {
    this_ = info[0].As<Napi::External<OGRCompoundCurve>>().Data();
    owned_ = true;
  } else {
    if (info.Length() != 0) {
      Napi::Error::New(info.Env(), "CompoundCurve constructor doesn't take any arguments")
        .ThrowAsJavaScriptException();
      return;
    }
    this_ = new OGRCompoundCurve();
    owned_ = true;
  }
}

Napi::Value CompoundCurveNapi::toString(const Napi::CallbackInfo &info) {
  return Napi::String::New(info.Env(), "CompoundCurve");
}

Napi::Value CompoundCurveNapi::pointsGetter(const Napi::CallbackInfo &info) {
  NAPI_UNWRAP_THIS(CompoundCurveNapi, self);
  Napi::Object thiz = info.This().As<Napi::Object>();
  if (thiz.Has("__points")) { Napi::Value c = thiz.Get("__points"); if (!c.IsNull() && !c.IsUndefined()) return c; }
  Napi::Object pts = LineStringPointsNapi::constructor.New({
    Napi::External<OGRLineString>::New(info.Env(), reinterpret_cast<OGRLineString *>(self->this_))
  });
  thiz.Set("__points", pts); return pts;
}

} // namespace node_gdal
