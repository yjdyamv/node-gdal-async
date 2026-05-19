#include "gdal_feature_napi.hpp"
#include "gdal_feature_defn_napi.hpp"

namespace node_gdal {

Napi::FunctionReference FeatureNapi::constructor;

Napi::Object FeatureNapi::Init(Napi::Env env, Napi::Object exports) {
  Napi::Function func = DefineClass(
    env,
    "FeatureNapi",
    {
      InstanceMethod("toString", &FeatureNapi::toString),
      InstanceMethod("clone", &FeatureNapi::clone),
      InstanceMethod("setFrom", &FeatureNapi::setFrom),
      InstanceMethod("destroy", &FeatureNapi::destroy),
      InstanceMethod("getStyleString", &FeatureNapi::getStyleString),
      InstanceMethod("setStyleString", &FeatureNapi::setStyleString),
      InstanceAccessor<&FeatureNapi::fidGetter, &FeatureNapi::fidSetter>("fid"),
      InstanceAccessor<&FeatureNapi::defnGetter>("defn"),
    });

  constructor = Napi::Persistent(func);
  constructor.SuppressDestruct();

  exports.Set("FeatureNapi", func);
  return exports;
}

FeatureNapi::FeatureNapi(const Napi::CallbackInfo &info)
  : Napi::ObjectWrap<FeatureNapi>(info), this_(nullptr), owned_(true) {

  if (!info.IsConstructCall()) {
    Napi::Error::New(info.Env(), "Cannot call constructor as function, use 'new' keyword")
      .ThrowAsJavaScriptException();
    return;
  }

  if (info.Length() > 0 && info[0].IsExternal()) {
    // External(OGRFeature*) from factory
    this_ = info[0].As<Napi::External<OGRFeature>>().Data();
    owned_ = true;
  } else {
    if (info.Length() < 1) {
      Napi::Error::New(info.Env(), "Constructor expects FeatureDefnNapi object")
        .ThrowAsJavaScriptException();
      return;
    }

    OGRFeatureDefn *def = nullptr;

    // Accept FeatureDefnNapi
    if (info[0].IsObject() &&
        info[0].As<Napi::Object>().InstanceOf(FeatureDefnNapi::constructor.Value())) {
      FeatureDefnNapi *fdefn = FeatureDefnNapi::Unwrap(info[0].As<Napi::Object>());
      if (!fdefn || !fdefn->isAlive()) {
        Napi::Error::New(info.Env(), "FeatureDefnNapi object already destroyed")
          .ThrowAsJavaScriptException();
        return;
      }
      def = fdefn->get();
    }

    if (!def) {
      Napi::Error::New(info.Env(), "Constructor expects FeatureDefnNapi object")
        .ThrowAsJavaScriptException();
      return;
    }

    this_ = new OGRFeature(def);
    owned_ = true;
  }
}

FeatureNapi::~FeatureNapi() {
  if (this_ && owned_) {
    OGRFeature::DestroyFeature(this_);
  }
  this_ = nullptr;
}

Napi::Value FeatureNapi::New(Napi::Env env, OGRFeature *feature) {
  return FeatureNapi::New(env, feature, true);
}

Napi::Value FeatureNapi::New(Napi::Env env, OGRFeature *feature, bool owned) {
  Napi::EscapableHandleScope scope(env);
  if (!feature) return scope.Escape(env.Null());

  Napi::Object obj =
    constructor.New({Napi::External<OGRFeature>::New(env, feature)});

  // Set owned_ flag on the unwrapped instance
  FeatureNapi *w = FeatureNapi::Unwrap(obj);
  if (w) w->owned_ = owned;

  return scope.Escape(obj);
}

Napi::Value FeatureNapi::toString(const Napi::CallbackInfo &info) {
  return Napi::String::New(info.Env(), "Feature");
}

Napi::Value FeatureNapi::clone(const Napi::CallbackInfo &info) {
  FeatureNapi *feature = FeatureNapi::Unwrap(info.This().As<Napi::Object>());
  if (!feature || !feature->isAlive()) {
    Napi::Error::New(info.Env(), "FeatureNapi object already destroyed")
      .ThrowAsJavaScriptException();
    return info.Env().Undefined();
  }
  return FeatureNapi::New(info.Env(), feature->this_->Clone());
}

Napi::Value FeatureNapi::setFrom(const Napi::CallbackInfo &info) {
  FeatureNapi *other = nullptr;
  NAPI_ARG_WRAPPED(0, "feature", FeatureNapi, other);

  FeatureNapi *self = FeatureNapi::Unwrap(info.This().As<Napi::Object>());
  if (!self || !self->isAlive()) {
    Napi::Error::New(info.Env(), "FeatureNapi object already destroyed")
      .ThrowAsJavaScriptException();
    return info.Env().Undefined();
  }

  int forgiving = 1;

  if (info.Length() > 1 && info[1].IsArray()) {
    Napi::Array index_map = info[1].As<Napi::Array>();
    NAPI_ARG_BOOL_OPT(2, "forgiving", forgiving);

    uint32_t len = index_map.Length();
    if (len < 1) {
      Napi::Error::New(info.Env(), "index map must contain at least 1 index")
        .ThrowAsJavaScriptException();
      return info.Env().Undefined();
    }

    int *map_ptr = new int[len];
    for (uint32_t i = 0; i < len; i++) {
      Napi::Value val = index_map.Get(i);
      if (!val.IsNumber()) {
        delete[] map_ptr;
        Napi::Error::New(info.Env(), "index map must contain only integer values")
          .ThrowAsJavaScriptException();
        return info.Env().Undefined();
      }
      map_ptr[i] = val.As<Napi::Number>().Int32Value();
    }

    OGRErr err = self->this_->SetFrom(other->this_, map_ptr, forgiving ? TRUE : FALSE);
    delete[] map_ptr;
    if (err) NAPI_THROW_OGRERR(err);
  } else {
    NAPI_ARG_BOOL_OPT(1, "forgiving", forgiving);
    OGRErr err = self->this_->SetFrom(other->this_, forgiving ? TRUE : FALSE);
    if (err) NAPI_THROW_OGRERR(err);
  }

  return info.Env().Undefined();
}

Napi::Value FeatureNapi::destroy(const Napi::CallbackInfo &info) {
  FeatureNapi *feature = FeatureNapi::Unwrap(info.This().As<Napi::Object>());
  if (!feature || !feature->isAlive()) {
    Napi::Error::New(info.Env(), "FeatureNapi object already destroyed")
      .ThrowAsJavaScriptException();
    return info.Env().Undefined();
  }
  if (feature->owned_) {
    OGRFeature::DestroyFeature(feature->this_);
    feature->owned_ = false;
  }
  feature->this_ = nullptr;
  return info.Env().Undefined();
}

Napi::Value FeatureNapi::getStyleString(const Napi::CallbackInfo &info) {
  FeatureNapi *feature = FeatureNapi::Unwrap(info.This().As<Napi::Object>());
  if (!feature || !feature->isAlive()) {
    Napi::Error::New(info.Env(), "FeatureNapi object already destroyed")
      .ThrowAsJavaScriptException();
    return info.Env().Undefined();
  }
  const char *psz = feature->this_->GetStyleString();
  if (!psz) return info.Env().Null();
  return Napi::String::New(info.Env(), psz);
}

Napi::Value FeatureNapi::setStyleString(const Napi::CallbackInfo &info) {
  FeatureNapi *feature = FeatureNapi::Unwrap(info.This().As<Napi::Object>());
  if (!feature || !feature->isAlive()) {
    Napi::Error::New(info.Env(), "FeatureNapi object already destroyed")
      .ThrowAsJavaScriptException();
    return info.Env().Undefined();
  }

  if (info.Length() < 1 || info[0].IsNull() || info[0].IsUndefined()) {
    feature->this_->SetStyleString(nullptr);
    return info.Env().Undefined();
  }

  if (!info[0].IsString()) {
    Napi::TypeError::New(info.Env(), "style must be a string, null or undefined")
      .ThrowAsJavaScriptException();
    return info.Env().Undefined();
  }

  feature->this_->SetStyleString(info[0].As<Napi::String>().Utf8Value().c_str());
  return info.Env().Undefined();
}

Napi::Value FeatureNapi::fidGetter(const Napi::CallbackInfo &info) {
  FeatureNapi *feature = FeatureNapi::Unwrap(info.This().As<Napi::Object>());
  if (!feature || !feature->isAlive()) {
    Napi::Error::New(info.Env(), "FeatureNapi object already destroyed")
      .ThrowAsJavaScriptException();
    return info.Env().Undefined();
  }
  return Napi::Number::New(info.Env(), feature->this_->GetFID());
}

Napi::Value FeatureNapi::defnGetter(const Napi::CallbackInfo &info) {
  FeatureNapi *feature = FeatureNapi::Unwrap(info.This().As<Napi::Object>());
  if (!feature || !feature->isAlive()) {
    Napi::Error::New(info.Env(), "FeatureNapi object already destroyed")
      .ThrowAsJavaScriptException();
    return info.Env().Undefined();
  }
  return FeatureDefnNapi::New(info.Env(), feature->this_->GetDefnRef());
}

void FeatureNapi::fidSetter(const Napi::CallbackInfo &info, const Napi::Value &value) {
  FeatureNapi *feature = FeatureNapi::Unwrap(info.This().As<Napi::Object>());
  if (!feature || !feature->isAlive()) {
    Napi::Error::New(info.Env(), "FeatureNapi object already destroyed")
      .ThrowAsJavaScriptException();
    return;
  }
  if (!value.IsNumber()) {
    Napi::Error::New(info.Env(), "fid must be an integer").ThrowAsJavaScriptException();
    return;
  }
  feature->this_->SetFID(value.As<Napi::Number>().Int64Value());
}

} // namespace node_gdal
