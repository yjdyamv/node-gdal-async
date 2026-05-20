#include "gdal_linearring_napi.hpp"
#include "../gdal_stubs_napi.hpp"

namespace node_gdal {

Napi::FunctionReference LinearRingNapi::constructor;

Napi::Object LinearRingNapi::Init(Napi::Env env, Napi::Object exports) {
  Napi::Function func = DefineClass(
    env,
    "LinearRing",
    {
      InstanceMethod("toString", &LinearRingNapi::toString),
      InstanceMethod("getArea", &LinearRingNapi::getArea),
      InstanceAccessor<&LinearRingNapi::pointsGetter>("points"),
    });

  constructor = Napi::Persistent(func);
  constructor.SuppressDestruct();
  exports.Set("LinearRing", func);
  return exports;
}

LinearRingNapi::LinearRingNapi(const Napi::CallbackInfo &info)
  : Napi::ObjectWrap<LinearRingNapi>(info) {

  if (!info.IsConstructCall()) {
    Napi::Error::New(info.Env(), "Cannot call constructor as function, use 'new' keyword")
      .ThrowAsJavaScriptException();
    return;
  }

  if (info.Length() > 0 && info[0].IsExternal()) {
    this_ = info[0].As<Napi::External<OGRLinearRing>>().Data();
    owned_ = true;
  } else {
    if (info.Length() != 0) {
      Napi::Error::New(info.Env(), "LinearRing constructor doesn't take any arguments")
        .ThrowAsJavaScriptException();
      return;
    }
    this_ = new OGRLinearRing();
    owned_ = true;
  }
}

Napi::Value LinearRingNapi::toString(const Napi::CallbackInfo &info) {
  return Napi::String::New(info.Env(), "LinearRing");
}

Napi::Value LinearRingNapi::getArea(const Napi::CallbackInfo &info) {
  NAPI_UNWRAP_THIS(LinearRingNapi, geom);
  return Napi::Number::New(info.Env(), geom->this_->get_Area());
}

Napi::Value LinearRingNapi::pointsGetter(const Napi::CallbackInfo &info) {
  NAPI_UNWRAP_THIS(LinearRingNapi, self);
  Napi::Object thiz = info.This().As<Napi::Object>();
  if (thiz.Has("__points")) {
    Napi::Value cached = thiz.Get("__points");
    if (!cached.IsNull() && !cached.IsUndefined()) return cached;
  }
  Napi::Object pts = LineStringPointsNapi::constructor.New({
    Napi::External<OGRLineString>::New(info.Env(),
      static_cast<OGRLineString *>(self->this_))
  });
  thiz.Set("__points", pts);
  return pts;
}

} // namespace node_gdal
