#include "gdal_circularstring_napi.hpp"

namespace node_gdal {

Napi::FunctionReference CircularStringNapi::constructor;

Napi::Object CircularStringNapi::Init(Napi::Env env, Napi::Object exports) {
  Napi::Function func = DefineClass(
    env, "CircularStringNapi",
    {InstanceMethod("toString", &CircularStringNapi::toString)});
  constructor = Napi::Persistent(func);
  constructor.SuppressDestruct();
  exports.Set("CircularStringNapi", func);
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

} // namespace node_gdal
