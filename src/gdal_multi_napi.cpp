#include "gdal_multi_napi.hpp"

namespace node_gdal {

// ===================== GroupNapi =====================
Napi::FunctionReference GroupNapi::constructor;

Napi::Object GroupNapi::Init(Napi::Env env, Napi::Object exports) {
  Napi::Function func = DefineClass(
    env, "GroupNapi",
    {
      InstanceMethod("toString", &GroupNapi::toString),
      InstanceAccessor<&GroupNapi::descriptionGetter>("description"),
    });
  constructor = Napi::Persistent(func); constructor.SuppressDestruct();
  exports.Set("GroupNapi", func); return exports;
}

GroupNapi::GroupNapi(const Napi::CallbackInfo &info)
  : Napi::ObjectWrap<GroupNapi>(info), this_(nullptr) {
  if (!info.IsConstructCall()) {
    Napi::Error::New(info.Env(), "Cannot call constructor as function, use 'new' keyword")
      .ThrowAsJavaScriptException(); return;
  }
  if (info.Length() > 0 && info[0].IsExternal()) {
    this_ = info[0].As<Napi::External<GDALGroup>>().Data();
  }
}
Napi::Value GroupNapi::toString(const Napi::CallbackInfo &info) {
  return Napi::String::New(info.Env(), "Group");
}
Napi::Value GroupNapi::descriptionGetter(const Napi::CallbackInfo &info) {
  NAPI_UNWRAP_THIS(GroupNapi, self);
  return SafeStringNapi(info.Env(), self->this_->GetFullName().c_str());
}

// ===================== MDArrayNapi =====================
Napi::FunctionReference MDArrayNapi::constructor;

Napi::Object MDArrayNapi::Init(Napi::Env env, Napi::Object exports) {
  Napi::Function func = DefineClass(
    env, "MDArrayNapi",
    {
      InstanceMethod("toString", &MDArrayNapi::toString),
      InstanceAccessor<&MDArrayNapi::dataTypeGetter>("dataType"),
      InstanceAccessor<&MDArrayNapi::dimensionCountGetter>("dimensionCount"),
    });
  constructor = Napi::Persistent(func); constructor.SuppressDestruct();
  exports.Set("MDArrayNapi", func); return exports;
}

MDArrayNapi::MDArrayNapi(const Napi::CallbackInfo &info)
  : Napi::ObjectWrap<MDArrayNapi>(info), this_(nullptr) {
  if (!info.IsConstructCall()) {
    Napi::Error::New(info.Env(), "Cannot call constructor as function, use 'new' keyword")
      .ThrowAsJavaScriptException(); return;
  }
  if (info.Length() > 0 && info[0].IsExternal()) {
    this_ = info[0].As<Napi::External<GDALMDArray>>().Data();
  }
}
Napi::Value MDArrayNapi::toString(const Napi::CallbackInfo &info) {
  return Napi::String::New(info.Env(), "MDArray");
}
Napi::Value MDArrayNapi::dataTypeGetter(const Napi::CallbackInfo &info) {
  NAPI_UNWRAP_THIS(MDArrayNapi, self);
  return Napi::String::New(info.Env(), self->this_->GetDataType().GetName().c_str());
}
Napi::Value MDArrayNapi::dimensionCountGetter(const Napi::CallbackInfo &info) {
  NAPI_UNWRAP_THIS(MDArrayNapi, self);
  return Napi::Number::New(info.Env(), self->this_->GetDimensionCount());
}

// ===================== DimensionNapi =====================
Napi::FunctionReference DimensionNapi::constructor;

Napi::Object DimensionNapi::Init(Napi::Env env, Napi::Object exports) {
  Napi::Function func = DefineClass(
    env, "DimensionNapi",
    {
      InstanceMethod("toString", &DimensionNapi::toString),
      InstanceAccessor<&DimensionNapi::nameGetter>("name"),
      InstanceAccessor<&DimensionNapi::sizeGetter>("size"),
      InstanceAccessor<&DimensionNapi::typeGetter>("type"),
    });
  constructor = Napi::Persistent(func); constructor.SuppressDestruct();
  exports.Set("DimensionNapi", func); return exports;
}

DimensionNapi::DimensionNapi(const Napi::CallbackInfo &info)
  : Napi::ObjectWrap<DimensionNapi>(info), this_(nullptr) {
  if (!info.IsConstructCall()) {
    Napi::Error::New(info.Env(), "Cannot call constructor as function, use 'new' keyword")
      .ThrowAsJavaScriptException(); return;
  }
  if (info.Length() > 0 && info[0].IsExternal()) {
    this_ = info[0].As<Napi::External<GDALDimension>>().Data();
  }
}
Napi::Value DimensionNapi::toString(const Napi::CallbackInfo &info) {
  return Napi::String::New(info.Env(), "Dimension");
}
Napi::Value DimensionNapi::nameGetter(const Napi::CallbackInfo &info) {
  NAPI_UNWRAP_THIS(DimensionNapi, self);
  return SafeStringNapi(info.Env(), self->this_->GetName().c_str());
}
Napi::Value DimensionNapi::sizeGetter(const Napi::CallbackInfo &info) {
  NAPI_UNWRAP_THIS(DimensionNapi, self);
  return Napi::Number::New(info.Env(), self->this_->GetSize());
}
Napi::Value DimensionNapi::typeGetter(const Napi::CallbackInfo &info) {
  NAPI_UNWRAP_THIS(DimensionNapi, self);
  return SafeStringNapi(info.Env(), self->this_->GetType().c_str());
}

// ===================== AttributeNapi =====================
Napi::FunctionReference AttributeNapi::constructor;

Napi::Object AttributeNapi::Init(Napi::Env env, Napi::Object exports) {
  Napi::Function func = DefineClass(
    env, "AttributeNapi",
    {
      InstanceMethod("toString", &AttributeNapi::toString),
      InstanceAccessor<&AttributeNapi::nameGetter>("name"),
      InstanceAccessor<&AttributeNapi::valueGetter>("value"),
    });
  constructor = Napi::Persistent(func); constructor.SuppressDestruct();
  exports.Set("AttributeNapi", func); return exports;
}

AttributeNapi::AttributeNapi(const Napi::CallbackInfo &info)
  : Napi::ObjectWrap<AttributeNapi>(info), this_(nullptr) {
  if (!info.IsConstructCall()) {
    Napi::Error::New(info.Env(), "Cannot call constructor as function, use 'new' keyword")
      .ThrowAsJavaScriptException(); return;
  }
  if (info.Length() > 0 && info[0].IsExternal()) {
    this_ = info[0].As<Napi::External<GDALAttribute>>().Data();
  }
}
Napi::Value AttributeNapi::toString(const Napi::CallbackInfo &info) {
  return Napi::String::New(info.Env(), "Attribute");
}
Napi::Value AttributeNapi::nameGetter(const Napi::CallbackInfo &info) {
  NAPI_UNWRAP_THIS(AttributeNapi, self);
  return SafeStringNapi(info.Env(), self->this_->GetName().c_str());
}
Napi::Value AttributeNapi::valueGetter(const Napi::CallbackInfo &info) {
  NAPI_UNWRAP_THIS(AttributeNapi, self);
  auto type = self->this_->GetDataType().GetClass();
  if (type == GEDTC_NUMERIC) return Napi::Number::New(info.Env(), self->this_->ReadAsDouble());
  if (type == GEDTC_STRING) return SafeStringNapi(info.Env(), self->this_->ReadAsString());
  return info.Env().Null();
}

} // namespace node_gdal
