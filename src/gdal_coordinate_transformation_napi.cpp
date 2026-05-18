#include "gdal_coordinate_transformation_napi.hpp"

namespace node_gdal {

Napi::FunctionReference CoordinateTransformationNapi::constructor;

Napi::Object CoordinateTransformationNapi::Init(Napi::Env env, Napi::Object exports) {
  Napi::Function func = DefineClass(
    env,
    "CoordinateTransformationNapi",
    {
      InstanceMethod("toString", &CoordinateTransformationNapi::toString),
      InstanceMethod("transformPoint", &CoordinateTransformationNapi::transformPoint),
    });

  constructor = Napi::Persistent(func);
  constructor.SuppressDestruct();

  exports.Set("CoordinateTransformationNapi", func);
  return exports;
}

CoordinateTransformationNapi::CoordinateTransformationNapi(const Napi::CallbackInfo &info)
  : Napi::ObjectWrap<CoordinateTransformationNapi>(info), this_(nullptr) {

  if (!info.IsConstructCall()) {
    Napi::Error::New(info.Env(), "Cannot call constructor as function, use 'new' keyword")
      .ThrowAsJavaScriptException();
    return;
  }

  if (info.Length() > 0 && info[0].IsExternal()) {
    this_ = info[0].As<Napi::External<OGRCoordinateTransformation>>().Data();
  } else {
    Napi::Error::New(info.Env(), "CoordinateTransformationNapi must be created via factory")
      .ThrowAsJavaScriptException();
    return;
  }
}

CoordinateTransformationNapi::~CoordinateTransformationNapi() {
  if (this_) {
    OGRCoordinateTransformation::DestroyCT(this_);
    this_ = nullptr;
  }
}

Napi::Value CoordinateTransformationNapi::New(Napi::Env env, OGRCoordinateTransformation *transform) {
  Napi::EscapableHandleScope scope(env);
  if (!transform) return scope.Escape(env.Null());

  return scope.Escape(
    constructor.New({Napi::External<OGRCoordinateTransformation>::New(env, transform)}));
}

Napi::Value CoordinateTransformationNapi::toString(const Napi::CallbackInfo &info) {
  return Napi::String::New(info.Env(), "CoordinateTransformation");
}

Napi::Value CoordinateTransformationNapi::transformPoint(const Napi::CallbackInfo &info) {
  CoordinateTransformationNapi *transform =
    CoordinateTransformationNapi::Unwrap(info.This().As<Napi::Object>());
  if (!transform || !transform->isAlive()) {
    Napi::Error::New(info.Env(), "CoordinateTransformationNapi object has already been destroyed")
      .ThrowAsJavaScriptException();
    return info.Env().Undefined();
  }

  double x, y, z = 0;

  if (info.Length() == 1 && info[0].IsObject()) {
    Napi::Object obj = info[0].As<Napi::Object>();
    Napi::Value arg_x = obj.Get("x");
    Napi::Value arg_y = obj.Get("y");
    Napi::Value arg_z = obj.Get("z");
    if (!arg_x.IsNumber() || !arg_y.IsNumber()) {
      Napi::Error::New(info.Env(), "point must contain numerical properties x and y")
        .ThrowAsJavaScriptException();
      return info.Env().Undefined();
    }
    x = arg_x.As<Napi::Number>().DoubleValue();
    y = arg_y.As<Napi::Number>().DoubleValue();
    if (arg_z.IsNumber()) { z = arg_z.As<Napi::Number>().DoubleValue(); }
  } else {
    NAPI_ARG_DOUBLE(0, "x", x);
    NAPI_ARG_DOUBLE(1, "y", y);
    NAPI_ARG_DOUBLE_OPT(2, "z", z);
  }

  if (!transform->this_->Transform(1, &x, &y, &z)) {
    Napi::Error::New(info.Env(), "Error transforming point").ThrowAsJavaScriptException();
    return info.Env().Undefined();
  }

  Napi::Object result = Napi::Object::New(info.Env());
  result.Set("x", Napi::Number::New(info.Env(), x));
  result.Set("y", Napi::Number::New(info.Env(), y));
  result.Set("z", Napi::Number::New(info.Env(), z));

  return result;
}

} // namespace node_gdal
