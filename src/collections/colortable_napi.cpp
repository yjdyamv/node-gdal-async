#include "colortable_napi.hpp"

namespace node_gdal {

// ---------------------------------------------------------------------------
// Color entry extraction helper
// ---------------------------------------------------------------------------
static bool extractColorEntry(Napi::Object obj, GDALColorEntry &color) {
  if (!obj.Has("c1") || !obj.Has("c2") || !obj.Has("c3") || !obj.Has("c4")) {
    return false;
  }
  Napi::Value c1 = obj.Get("c1");
  Napi::Value c2 = obj.Get("c2");
  Napi::Value c3 = obj.Get("c3");
  Napi::Value c4 = obj.Get("c4");
  if (!c1.IsNumber() || !c2.IsNumber() || !c3.IsNumber() || !c4.IsNumber()) {
    return false;
  }
  color.c1 = (short)c1.As<Napi::Number>().Int32Value();
  color.c2 = (short)c2.As<Napi::Number>().Int32Value();
  color.c3 = (short)c3.As<Napi::Number>().Int32Value();
  color.c4 = (short)c4.As<Napi::Number>().Int32Value();
  return true;
}

static Napi::Object createColorResult(Napi::Env env, const GDALColorEntry *color) {
  Napi::Object result = Napi::Object::New(env);
  result.Set("c1", Napi::Number::New(env, color->c1));
  result.Set("c2", Napi::Number::New(env, color->c2));
  result.Set("c3", Napi::Number::New(env, color->c3));
  result.Set("c4", Napi::Number::New(env, color->c4));
  return result;
}

// ---------------------------------------------------------------------------
// Static members
// ---------------------------------------------------------------------------
Napi::FunctionReference ColorTableNapi::constructor;

Napi::Object ColorTableNapi::Init(Napi::Env env, Napi::Object exports) {
  Napi::Function func = DefineClass(
    env,
    "ColorTable",
    {
      InstanceMethod("toString", &ColorTableNapi::toString),
      InstanceMethod("isSame", &ColorTableNapi::isSame),
      InstanceMethod("clone", &ColorTableNapi::clone),
      InstanceMethod("get", &ColorTableNapi::get),
      InstanceMethod("set", &ColorTableNapi::set),
      InstanceMethod("count", &ColorTableNapi::count),
      InstanceMethod("ramp", &ColorTableNapi::ramp),
      InstanceAccessor<&ColorTableNapi::interpretationGetter>("interpretation"),
    });

  constructor = Napi::Persistent(func);
  constructor.SuppressDestruct();

  exports.Set("ColorTable", func);
  return exports;
}

ColorTableNapi::ColorTableNapi(const Napi::CallbackInfo &info)
  : Napi::ObjectWrap<ColorTableNapi>(info), this_(nullptr) {

  if (!info.IsConstructCall()) {
    Napi::Error::New(info.Env(), "Cannot call constructor as function, use 'new' keyword")
      .ThrowAsJavaScriptException();
    return;
  }

  if (info.Length() > 0 && info[0].IsExternal()) {
    this_ = info[0].As<Napi::External<GDALColorTable>>().Data();
  } else {
    std::string pi;
    if (info.Length() < 1 || !info[0].IsString()) {
      Napi::Error::New(info.Env(), "palette interpretation must be a string")
        .ThrowAsJavaScriptException();
      return;
    }
    pi = info[0].As<Napi::String>().Utf8Value();

    GDALPaletteInterp gpi;
    if (pi == "Gray") gpi = GPI_Gray;
    else if (pi == "RGB") gpi = GPI_RGB;
    else if (pi == "CMYK") gpi = GPI_CMYK;
    else if (pi == "HLS") gpi = GPI_HLS;
    else {
      Napi::RangeError::New(info.Env(), "Invalid palette interpretation")
        .ThrowAsJavaScriptException();
      return;
    }
    this_ = new GDALColorTable(gpi);
  }
}

ColorTableNapi::~ColorTableNapi() {
  if (this_) {
    delete this_;
    this_ = nullptr;
  }
}

Napi::Value ColorTableNapi::New(Napi::Env env, GDALColorTable *raw) {
  Napi::EscapableHandleScope scope(env);
  if (!raw) return scope.Escape(env.Null());
  return scope.Escape(
    constructor.New({Napi::External<GDALColorTable>::New(env, raw)}));
}

Napi::Value ColorTableNapi::toString(const Napi::CallbackInfo &info) {
  return Napi::String::New(info.Env(), "ColorTable");
}

Napi::Value ColorTableNapi::clone(const Napi::CallbackInfo &info) {
  ColorTableNapi *ct = ColorTableNapi::Unwrap(info.This().As<Napi::Object>());
  if (!ct || !ct->isAlive()) {
    Napi::Error::New(info.Env(), "ColorTableNapi object has already been destroyed")
      .ThrowAsJavaScriptException();
    return info.Env().Undefined();
  }
  return ColorTableNapi::New(info.Env(), ct->this_->Clone());
}

Napi::Value ColorTableNapi::isSame(const Napi::CallbackInfo &info) {
  ColorTableNapi *self = ColorTableNapi::Unwrap(info.This().As<Napi::Object>());
  if (!self || !self->isAlive()) {
    Napi::Error::New(info.Env(), "ColorTableNapi object has already been destroyed")
      .ThrowAsJavaScriptException();
    return info.Env().Undefined();
  }

  ColorTableNapi *other;
  NAPI_ARG_WRAPPED(0, "other", ColorTableNapi, other);

  return Napi::Boolean::New(info.Env(), self->this_->IsSame(other->this_));
}

Napi::Value ColorTableNapi::get(const Napi::CallbackInfo &info) {
  int index;
  NAPI_ARG_INT(0, "index", index);

  ColorTableNapi *self = ColorTableNapi::Unwrap(info.This().As<Napi::Object>());
  if (!self || !self->isAlive()) {
    Napi::Error::New(info.Env(), "ColorTableNapi object has already been destroyed")
      .ThrowAsJavaScriptException();
    return info.Env().Undefined();
  }

  const GDALColorEntry *color = self->this_->GetColorEntry(index);
  if (!color) {
    NAPI_THROW_LAST_CPLERR;
  }

  return createColorResult(info.Env(), color);
}

Napi::Value ColorTableNapi::set(const Napi::CallbackInfo &info) {
  int index;
  NAPI_ARG_INT(0, "index", index);

  if (info.Length() < 2 || !info[1].IsObject()) {
    Napi::TypeError::New(info.Env(), "color must be an object").ThrowAsJavaScriptException();
    return info.Env().Undefined();
  }
  Napi::Object color_obj = info[1].As<Napi::Object>();

  ColorTableNapi *self = ColorTableNapi::Unwrap(info.This().As<Napi::Object>());
  if (!self || !self->isAlive()) {
    Napi::Error::New(info.Env(), "ColorTableNapi object has already been destroyed")
      .ThrowAsJavaScriptException();
    return info.Env().Undefined();
  }

  GDALColorEntry color;
  if (!extractColorEntry(color_obj, color)) {
    Napi::Error::New(info.Env(), "Object must contain numerical properties c1, c2, c3, c4")
      .ThrowAsJavaScriptException();
    return info.Env().Undefined();
  }

  self->this_->SetColorEntry(index, &color);
  return info.Env().Undefined();
}

Napi::Value ColorTableNapi::count(const Napi::CallbackInfo &info) {
  ColorTableNapi *self = ColorTableNapi::Unwrap(info.This().As<Napi::Object>());
  if (!self || !self->isAlive()) {
    Napi::Error::New(info.Env(), "ColorTableNapi object has already been destroyed")
      .ThrowAsJavaScriptException();
    return info.Env().Undefined();
  }
  return Napi::Number::New(info.Env(), self->this_->GetColorEntryCount());
}

Napi::Value ColorTableNapi::ramp(const Napi::CallbackInfo &info) {
  int start_index, end_index;
  NAPI_ARG_INT(0, "start_index", start_index);
  NAPI_ARG_INT(2, "end_index", end_index);

  if (start_index < 0 || end_index < 0 || end_index < start_index) {
    Napi::RangeError::New(info.Env(), "Invalid color interval").ThrowAsJavaScriptException();
    return info.Env().Undefined();
  }

  if (info.Length() < 4 || !info[1].IsObject() || !info[3].IsObject()) {
    Napi::TypeError::New(info.Env(), "start_color and end_color must be objects")
      .ThrowAsJavaScriptException();
    return info.Env().Undefined();
  }

  Napi::Object start_color_obj = info[1].As<Napi::Object>();
  Napi::Object end_color_obj = info[3].As<Napi::Object>();

  ColorTableNapi *self = ColorTableNapi::Unwrap(info.This().As<Napi::Object>());
  if (!self || !self->isAlive()) {
    Napi::Error::New(info.Env(), "ColorTableNapi object has already been destroyed")
      .ThrowAsJavaScriptException();
    return info.Env().Undefined();
  }

  GDALColorEntry start_color, end_color;
  if (!extractColorEntry(start_color_obj, start_color) ||
      !extractColorEntry(end_color_obj, end_color)) {
    Napi::Error::New(info.Env(), "Color objects must contain numerical c1, c2, c3, c4")
      .ThrowAsJavaScriptException();
    return info.Env().Undefined();
  }

  int r = self->this_->CreateColorRamp(start_index, &start_color, end_index, &end_color);
  if (r == -1) NAPI_THROW_LAST_CPLERR;
  return Napi::Number::New(info.Env(), r);
}

Napi::Value ColorTableNapi::interpretationGetter(const Napi::CallbackInfo &info) {
  ColorTableNapi *self = ColorTableNapi::Unwrap(info.This().As<Napi::Object>());
  if (!self || !self->isAlive()) {
    Napi::Error::New(info.Env(), "ColorTableNapi object has already been destroyed")
      .ThrowAsJavaScriptException();
    return info.Env().Undefined();
  }

  GDALPaletteInterp interp = self->this_->GetPaletteInterpretation();
  const char *r = "invalid";
  switch (interp) {
    case GPI_Gray: r = "Gray"; break;
    case GPI_RGB: r = "RGB"; break;
    case GPI_CMYK: r = "CMYK"; break;
    case GPI_HLS: r = "HLS"; break;
    default: break;
  }
  return Napi::String::New(info.Env(), r);
}

} // namespace node_gdal
