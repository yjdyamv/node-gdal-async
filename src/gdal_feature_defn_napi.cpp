#include "gdal_feature_defn_napi.hpp"
#include "gdal_stubs_napi.hpp"

namespace node_gdal {

Napi::FunctionReference FeatureDefnNapi::constructor;

Napi::Object FeatureDefnNapi::Init(Napi::Env env, Napi::Object exports) {
  Napi::Function func = DefineClass(
    env,
    "FeatureDefn",
    {
      InstanceMethod("toString", &FeatureDefnNapi::toString),
      InstanceMethod("clone", &FeatureDefnNapi::clone),
      InstanceAccessor<&FeatureDefnNapi::nameGetter>("name"),
      InstanceAccessor<&FeatureDefnNapi::geomTypeGetter,
                        &FeatureDefnNapi::geomTypeSetter>("geomType"),
      InstanceAccessor<&FeatureDefnNapi::geomIgnoredGetter,
                        &FeatureDefnNapi::geomIgnoredSetter>("geomIgnored"),
      InstanceAccessor<&FeatureDefnNapi::fieldsGetter>("fields"),
      InstanceAccessor<&FeatureDefnNapi::styleIgnoredGetter,
                        &FeatureDefnNapi::styleIgnoredSetter>("styleIgnored"),
    });

  constructor = Napi::Persistent(func);
  constructor.SuppressDestruct();

  exports.Set("FeatureDefn", func);
  return exports;
}

FeatureDefnNapi::FeatureDefnNapi(const Napi::CallbackInfo &info)
  : Napi::ObjectWrap<FeatureDefnNapi>(info), this_(nullptr), owned_(true) {

  if (!info.IsConstructCall()) {
    Napi::Error::New(info.Env(), "Cannot call constructor as function, use 'new' keyword")
      .ThrowAsJavaScriptException();
    return;
  }

  if (info.Length() > 0 && info[0].IsExternal()) {
    this_ = info[0].As<Napi::External<OGRFeatureDefn>>().Data();
    owned_ = true;
  } else {
    if (info.Length() != 0) {
      Napi::Error::New(info.Env(), "FeatureDefnNapi constructor doesn't take any arguments")
        .ThrowAsJavaScriptException();
      return;
    }
    this_ = new OGRFeatureDefn();
    this_->Reference();
  }
}

FeatureDefnNapi::~FeatureDefnNapi() {
  if (this_ && owned_) {
    this_->Release();
  }
  this_ = nullptr;
}

Napi::Value FeatureDefnNapi::New(Napi::Env env, const OGRFeatureDefn *def) {
  if (!def) return env.Null();
  OGRFeatureDefn *copy = def->Clone();
  return FeatureDefnNapi::New(env, copy, true);
}

Napi::Value FeatureDefnNapi::New(Napi::Env env, OGRFeatureDefn *def, bool owned) {
  Napi::EscapableHandleScope scope(env);
  if (!def) return scope.Escape(env.Null());
  if (!owned) { def = def->Clone(); }
  def->Reference();

  return scope.Escape(
    constructor.New({Napi::External<OGRFeatureDefn>::New(env, def)}));
}

Napi::Value FeatureDefnNapi::toString(const Napi::CallbackInfo &info) {
  return Napi::String::New(info.Env(), "FeatureDefn");
}

Napi::Value FeatureDefnNapi::clone(const Napi::CallbackInfo &info) {
  FeatureDefnNapi *def = FeatureDefnNapi::Unwrap(info.This().As<Napi::Object>());
  if (!def || !def->isAlive()) {
    Napi::Error::New(info.Env(), "FeatureDefnNapi object has already been destroyed")
      .ThrowAsJavaScriptException();
    return info.Env().Undefined();
  }
  return FeatureDefnNapi::New(info.Env(), def->this_->Clone());
}

Napi::Value FeatureDefnNapi::nameGetter(const Napi::CallbackInfo &info) {
  FeatureDefnNapi *def = FeatureDefnNapi::Unwrap(info.This().As<Napi::Object>());
  if (!def || !def->isAlive()) {
    Napi::Error::New(info.Env(), "FeatureDefnNapi object has already been destroyed")
      .ThrowAsJavaScriptException();
    return info.Env().Undefined();
  }
  return SafeStringNapi(info.Env(), def->this_->GetName());
}

Napi::Value FeatureDefnNapi::geomTypeGetter(const Napi::CallbackInfo &info) {
  FeatureDefnNapi *def = FeatureDefnNapi::Unwrap(info.This().As<Napi::Object>());
  if (!def || !def->isAlive()) {
    Napi::Error::New(info.Env(), "FeatureDefnNapi object has already been destroyed")
      .ThrowAsJavaScriptException();
    return info.Env().Undefined();
  }
  return Napi::Number::New(info.Env(), def->this_->GetGeomType());
}

Napi::Value FeatureDefnNapi::geomIgnoredGetter(const Napi::CallbackInfo &info) {
  FeatureDefnNapi *def = FeatureDefnNapi::Unwrap(info.This().As<Napi::Object>());
  if (!def || !def->isAlive()) {
    Napi::Error::New(info.Env(), "FeatureDefnNapi object has already been destroyed")
      .ThrowAsJavaScriptException();
    return info.Env().Undefined();
  }
  return Napi::Boolean::New(info.Env(), def->this_->IsGeometryIgnored());
}

Napi::Value FeatureDefnNapi::styleIgnoredGetter(const Napi::CallbackInfo &info) {
  FeatureDefnNapi *def = FeatureDefnNapi::Unwrap(info.This().As<Napi::Object>());
  if (!def || !def->isAlive()) {
    Napi::Error::New(info.Env(), "FeatureDefnNapi object has already been destroyed")
      .ThrowAsJavaScriptException();
    return info.Env().Undefined();
  }
  return Napi::Boolean::New(info.Env(), def->this_->IsStyleIgnored());
}

void FeatureDefnNapi::geomTypeSetter(const Napi::CallbackInfo &info, const Napi::Value &value) {
  FeatureDefnNapi *def = FeatureDefnNapi::Unwrap(info.This().As<Napi::Object>());
  if (!def || !def->isAlive()) {
    Napi::Error::New(info.Env(), "FeatureDefnNapi object has already been destroyed")
      .ThrowAsJavaScriptException();
    return;
  }
  if (!value.IsNumber()) {
    Napi::Error::New(info.Env(), "geomType must be an integer").ThrowAsJavaScriptException();
    return;
  }
  def->this_->SetGeomType(static_cast<OGRwkbGeometryType>(value.As<Napi::Number>().Int32Value()));
}

void FeatureDefnNapi::geomIgnoredSetter(const Napi::CallbackInfo &info, const Napi::Value &value) {
  FeatureDefnNapi *def = FeatureDefnNapi::Unwrap(info.This().As<Napi::Object>());
  if (!def || !def->isAlive()) {
    Napi::Error::New(info.Env(), "FeatureDefnNapi object has already been destroyed")
      .ThrowAsJavaScriptException();
    return;
  }
  if (!value.IsBoolean()) {
    Napi::Error::New(info.Env(), "geomIgnored must be a boolean").ThrowAsJavaScriptException();
    return;
  }
  def->this_->SetGeometryIgnored(value.As<Napi::Boolean>().Value());
}

void FeatureDefnNapi::styleIgnoredSetter(const Napi::CallbackInfo &info, const Napi::Value &value) {
  FeatureDefnNapi *def = FeatureDefnNapi::Unwrap(info.This().As<Napi::Object>());
  if (!def || !def->isAlive()) {
    Napi::Error::New(info.Env(), "FeatureDefnNapi object has already been destroyed")
      .ThrowAsJavaScriptException();
    return;
  }
  if (!value.IsBoolean()) {
    Napi::Error::New(info.Env(), "styleIgnored must be a boolean").ThrowAsJavaScriptException();
    return;
  }
  def->this_->SetStyleIgnored(value.As<Napi::Boolean>().Value());
}

Napi::Value FeatureDefnNapi::fieldsGetter(const Napi::CallbackInfo &info) {
  NAPI_UNWRAP_THIS(FeatureDefnNapi, self);
  Napi::Object thiz = info.This().As<Napi::Object>();
  if (thiz.Has("__fields")) { Napi::Value c = thiz.Get("__fields"); if (!c.IsNull() && !c.IsUndefined()) return c; }
  Napi::Object f = FeatureDefnFieldsNapi::constructor.New({Napi::External<OGRFeatureDefn>::New(info.Env(), self->this_)});
  thiz.Set("__fields", f); return f;
}

} // namespace node_gdal
