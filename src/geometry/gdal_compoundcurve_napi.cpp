#include "gdal_compoundcurve_napi.hpp"

namespace node_gdal {

Napi::FunctionReference CompoundCurveNapi::constructor;

Napi::Object CompoundCurveNapi::Init(Napi::Env env, Napi::Object exports) {
  Napi::Function func = DefineClass(
    env, "CompoundCurveNapi",
    {InstanceMethod("toString", &CompoundCurveNapi::toString)});
  constructor = Napi::Persistent(func);
  constructor.SuppressDestruct();
  exports.Set("CompoundCurveNapi", func);
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

} // namespace node_gdal
