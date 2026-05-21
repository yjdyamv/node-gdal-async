#include "gdal_geometry_napi.hpp"
#include "gdal_point_napi.hpp"

namespace node_gdal {

Napi::FunctionReference PointNapi::constructor;

Napi::Object PointNapi::Init(Napi::Env env, Napi::Object exports) {
  Napi::Function func = DefineClass(
    env,
    "Point",
    {
      InstanceMethod("toString", &PointNapi::toString),
      InstanceMethod("toJSON", &PointNapi::toJSON),
      InstanceAccessor<&PointNapi::xGetter, &PointNapi::xSetter>("x"),
      InstanceAccessor<&PointNapi::yGetter, &PointNapi::ySetter>("y"),
      InstanceAccessor<&PointNapi::zGetter, &PointNapi::zSetter>("z"),
    });

  constructor = Napi::Persistent(func);
  NapiSetPrototypeChain(env, func, GeometryNapi::constructor.Value());
  constructor.SuppressDestruct();

  exports.Set("Point", func);
  return exports;
}

PointNapi::PointNapi(const Napi::CallbackInfo &info)
  : Napi::ObjectWrap<PointNapi>(info) {

  if (!info.IsConstructCall()) {
    Napi::Error::New(info.Env(), "Cannot call constructor as function, use 'new' keyword")
      .ThrowAsJavaScriptException();
    return;
  }

  if (info.Length() > 0 && info[0].IsExternal()) {
    // Factory path: External(OGRPoint*)
    this_ = info[0].As<Napi::External<OGRPoint>>().Data();
    owned_ = true;
  } else {
    double x = 0, y = 0, z = 0;

    if (info.Length() == 1) {
      Napi::Error::New(info.Env(), "Point constructor must be given 0, 2, or 3 arguments")
        .ThrowAsJavaScriptException();
      return;
    }

    if (info.Length() > 0 && info[0].IsNumber()) {
      x = info[0].As<Napi::Number>().DoubleValue();
    }
    if (info.Length() > 1 && info[1].IsNumber()) {
      y = info[1].As<Napi::Number>().DoubleValue();
    }
    if (info.Length() > 2 && info[2].IsNumber()) {
      z = info[2].As<Napi::Number>().DoubleValue();
    }

    OGRPoint *geom = info.Length() >= 3 ? new OGRPoint(x, y, z) : new OGRPoint(x, y);
    this_ = geom;
    owned_ = true;
  }
}

Napi::Value PointNapi::toString(const Napi::CallbackInfo &info) {
  return Napi::String::New(info.Env(), "Point");
}

Napi::Value PointNapi::xGetter(const Napi::CallbackInfo &info) {
  PointNapi *self = PointNapi::Unwrap(info.This().As<Napi::Object>());
  if (!self || !self->isAlive()) {
    Napi::Error::New(info.Env(), "PointNapi object already destroyed")
      .ThrowAsJavaScriptException();
    return info.Env().Undefined();
  }
  return Napi::Number::New(info.Env(), self->this_->getX());
}

void PointNapi::xSetter(const Napi::CallbackInfo &info, const Napi::Value &value) {
  PointNapi *self = PointNapi::Unwrap(info.This().As<Napi::Object>());
  if (!self || !self->isAlive()) {
    Napi::Error::New(info.Env(), "PointNapi object already destroyed")
      .ThrowAsJavaScriptException();
    return;
  }
  if (!value.IsNumber()) {
    Napi::Error::New(info.Env(), "x must be a number").ThrowAsJavaScriptException();
    return;
  }
  self->this_->setX(value.As<Napi::Number>().DoubleValue());
}

Napi::Value PointNapi::yGetter(const Napi::CallbackInfo &info) {
  PointNapi *self = PointNapi::Unwrap(info.This().As<Napi::Object>());
  if (!self || !self->isAlive()) {
    Napi::Error::New(info.Env(), "PointNapi object already destroyed")
      .ThrowAsJavaScriptException();
    return info.Env().Undefined();
  }
  return Napi::Number::New(info.Env(), self->this_->getY());
}

void PointNapi::ySetter(const Napi::CallbackInfo &info, const Napi::Value &value) {
  PointNapi *self = PointNapi::Unwrap(info.This().As<Napi::Object>());
  if (!self || !self->isAlive()) {
    Napi::Error::New(info.Env(), "PointNapi object already destroyed")
      .ThrowAsJavaScriptException();
    return;
  }
  if (!value.IsNumber()) {
    Napi::Error::New(info.Env(), "y must be a number").ThrowAsJavaScriptException();
    return;
  }
  self->this_->setY(value.As<Napi::Number>().DoubleValue());
}

Napi::Value PointNapi::zGetter(const Napi::CallbackInfo &info) {
  PointNapi *self = PointNapi::Unwrap(info.This().As<Napi::Object>());
  if (!self || !self->isAlive()) {
    Napi::Error::New(info.Env(), "PointNapi object already destroyed")
      .ThrowAsJavaScriptException();
    return info.Env().Undefined();
  }
  return Napi::Number::New(info.Env(), self->this_->getZ());
}

void PointNapi::zSetter(const Napi::CallbackInfo &info, const Napi::Value &value) {
  PointNapi *self = PointNapi::Unwrap(info.This().As<Napi::Object>());
  if (!self || !self->isAlive()) {
    Napi::Error::New(info.Env(), "PointNapi object already destroyed")
      .ThrowAsJavaScriptException();
    return;
  }
  if (!value.IsNumber()) {
    Napi::Error::New(info.Env(), "z must be a number").ThrowAsJavaScriptException();
    return;
  }
  self->this_->setZ(value.As<Napi::Number>().DoubleValue());
}

Napi::Value PointNapi::toJSON(const Napi::CallbackInfo &info) {
  NAPI_UNWRAP_THIS(PointNapi, self);
  Napi::Object obj = Napi::Object::New(info.Env());
  obj.Set("type", Napi::String::New(info.Env(), "Point"));
  Napi::Array coords = Napi::Array::New(info.Env());
  coords.Set(uint32_t(0), Napi::Number::New(info.Env(), self->this_->getX()));
  coords.Set(uint32_t(1), Napi::Number::New(info.Env(), self->this_->getY()));
  coords.Set(uint32_t(2), Napi::Number::New(info.Env(), self->this_->getZ()));
  obj.Set("coordinates", coords);
  return obj;
}

} // namespace node_gdal
