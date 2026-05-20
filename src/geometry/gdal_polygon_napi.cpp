#include "gdal_polygon_napi.hpp"
#include "../gdal_stubs_napi.hpp"

namespace node_gdal {

Napi::FunctionReference PolygonNapi::constructor;

Napi::Object PolygonNapi::Init(Napi::Env env, Napi::Object exports) {
  Napi::Function func = DefineClass(
    env,
    "Polygon",
    {
      InstanceMethod("toString", &PolygonNapi::toString),
      InstanceMethod("getArea", &PolygonNapi::getArea),
      InstanceAccessor<&PolygonNapi::ringsGetter>("rings"),
    });

  constructor = Napi::Persistent(func);
  constructor.SuppressDestruct();
  exports.Set("Polygon", func);
  return exports;
}

PolygonNapi::PolygonNapi(const Napi::CallbackInfo &info)
  : Napi::ObjectWrap<PolygonNapi>(info) {

  if (!info.IsConstructCall()) {
    Napi::Error::New(info.Env(), "Cannot call constructor as function, use 'new' keyword")
      .ThrowAsJavaScriptException();
    return;
  }

  if (info.Length() > 0 && info[0].IsExternal()) {
    this_ = info[0].As<Napi::External<OGRPolygon>>().Data();
    owned_ = true;
  } else {
    if (info.Length() != 0) {
      Napi::Error::New(info.Env(), "Polygon constructor doesn't take any arguments")
        .ThrowAsJavaScriptException();
      return;
    }
    this_ = new OGRPolygon();
    owned_ = true;
  }
}

Napi::Value PolygonNapi::toString(const Napi::CallbackInfo &info) {
  return Napi::String::New(info.Env(), "Polygon");
}

Napi::Value PolygonNapi::getArea(const Napi::CallbackInfo &info) {
  NAPI_UNWRAP_THIS(PolygonNapi, geom);
  return Napi::Number::New(info.Env(), geom->this_->get_Area());
}

Napi::Value PolygonNapi::ringsGetter(const Napi::CallbackInfo &info) {
  NAPI_UNWRAP_THIS(PolygonNapi, self);
  Napi::Object thiz = info.This().As<Napi::Object>();
  if (thiz.Has("__rings")) {
    Napi::Value cached = thiz.Get("__rings");
    if (!cached.IsNull() && !cached.IsUndefined()) return cached;
  }
  Napi::Object rings = PolygonRingsNapi::constructor.New({
    Napi::External<OGRPolygon>::New(info.Env(), self->this_)
  });
  thiz.Set("__rings", rings);
  return rings;
}

} // namespace node_gdal
