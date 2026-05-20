#include "gdal_drivers_napi.hpp"
#include "../gdal_driver_napi.hpp"

namespace node_gdal {

Napi::FunctionReference GDALDriversNapi::constructor;

Napi::Object GDALDriversNapi::Init(Napi::Env env, Napi::Object exports) {
  Napi::Function func = DefineClass(
    env, "GDALDrivers",
    {
      InstanceMethod("toString", &GDALDriversNapi::toString),
      InstanceMethod("count", &GDALDriversNapi::count),
      InstanceMethod("get", &GDALDriversNapi::get),
      InstanceMethod("getNames", &GDALDriversNapi::getNames),
    });
  constructor = Napi::Persistent(func);
  constructor.SuppressDestruct();

  GDALAllRegister();
  exports.Set("GDALDrivers", func);
  return exports;
}

GDALDriversNapi::GDALDriversNapi(const Napi::CallbackInfo &info)
  : Napi::ObjectWrap<GDALDriversNapi>(info) {
  if (!info.IsConstructCall()) {
    Napi::Error::New(info.Env(), "Cannot call constructor as function, use 'new' keyword")
      .ThrowAsJavaScriptException();
    return;
  }
  if (info.Length() == 0 || !info[0].IsExternal()) {
    Napi::Error::New(info.Env(), "Cannot create GDALDrivers directly")
      .ThrowAsJavaScriptException();
  }
}

Napi::Value GDALDriversNapi::New(Napi::Env env) {
  return constructor.New({});
}

Napi::Value GDALDriversNapi::toString(const Napi::CallbackInfo &info) {
  return Napi::String::New(info.Env(), "GDALDrivers");
}

Napi::Value GDALDriversNapi::get(const Napi::CallbackInfo &info) {
  if (info.Length() == 0) {
    Napi::Error::New(info.Env(), "Either driver name or index must be provided")
      .ThrowAsJavaScriptException();
    return info.Env().Undefined();
  }

  GDALDriver *gdal_driver = nullptr;

  if (info[0].IsString()) {
    std::string name = info[0].As<Napi::String>().Utf8Value();
    if (name == "VRT:vector" || name == "VRT:raster") name = "VRT";
    gdal_driver = GetGDALDriverManager()->GetDriverByName(name.c_str());
  } else if (info[0].IsNumber()) {
    int i = info[0].As<Napi::Number>().Int32Value();
    gdal_driver = GetGDALDriverManager()->GetDriver(i);
  } else {
    Napi::Error::New(info.Env(), "Argument must be string or integer")
      .ThrowAsJavaScriptException();
    return info.Env().Undefined();
  }

  if (gdal_driver) return DriverNapi::New(info.Env(), gdal_driver);
  NAPI_THROW_LAST_CPLERR;
}

Napi::Value GDALDriversNapi::getNames(const Napi::CallbackInfo &info) {
  int count = GetGDALDriverManager()->GetDriverCount();
  Napi::Array names = Napi::Array::New(info.Env(), count);
  for (int i = 0; i < count; i++) {
    GDALDriver *driver = GetGDALDriverManager()->GetDriver(i);
    names.Set(i, SafeStringNapi(info.Env(), driver->GetDescription()));
  }
  return names;
}

Napi::Value GDALDriversNapi::count(const Napi::CallbackInfo &info) {
  return Napi::Number::New(info.Env(), GetGDALDriverManager()->GetDriverCount());
}

} // namespace node_gdal
