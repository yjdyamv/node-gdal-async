#include "gdal_polygon_napi.hpp"

namespace node_gdal {

Napi::FunctionReference PolygonNapi::constructor;

Napi::Object PolygonNapi::Init(Napi::Env env, Napi::Object exports) {
  Napi::Function func = DefineClass(
    env,
    "PolygonNapi",
    {
      InstanceMethod("toString", &PolygonNapi::toString),
      InstanceMethod("getArea", &PolygonNapi::getArea),
    });

  constructor = Napi::Persistent(func);
  constructor.SuppressDestruct();
  exports.Set("PolygonNapi", func);
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

} // namespace node_gdal
