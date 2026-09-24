
#include "gdal_feature_defn.hpp"
#include "collections/feature_defn_fields.hpp"
#include "gdal_common.hpp"
#include "gdal_field_defn.hpp"

namespace node_gdal {

Napi::FunctionReference FeatureDefn::constructor;

void FeatureDefn::Initialize(Napi::Object target) {
  Napi::Env env = target.Env();
  SELF_CLASS(FeatureDefn);

  // NOTE: the descriptor macros carry their own trailing comma
  Napi::Function lcons = DefineClass(env, "FeatureDefn",
    {
        METHOD(toString)
        METHOD(clone)
        ATTR(lcons, "name", nameGetter, READ_ONLY_SETTER)
        ATTR(lcons, "fields", fieldsGetter, READ_ONLY_SETTER)
        ATTR(lcons, "styleIgnored", styleIgnoredGetter, styleIgnoredSetter)
        ATTR(lcons, "geomIgnored", geomIgnoredGetter, geomIgnoredSetter)
        ATTR(lcons, "geomType", geomTypeGetter, geomTypeSetter)
    });

  target.Set("FeatureDefn", lcons);

  constructor = Napi::Persistent(lcons);
  constructor.SuppressDestruct();
}

FeatureDefn::FeatureDefn(OGRFeatureDefn *def) : Nan::ObjectWrap(), this_(def), owned_(true) {
  LOG("Created FeatureDefn [%p]", def);
}

FeatureDefn::FeatureDefn(const Napi::CallbackInfo &info) : GDALObject<FeatureDefn>(info), this_(0), owned_(true) {
}

FeatureDefn::~FeatureDefn() {
  if (this_) {
    LOG("Disposing FeatureDefn [%p] (%s)", this_, owned_ ? "owned" : "unowned");
    if (owned_) this_->Release();
    this_ = NULL;
    LOG("Disposed FeatureDefn [%p]", this_);
  }
}

/**
 * Definition of a feature class or feature layer.
 *
 * @constructor
 * @class FeatureDefn
 */
NAN_METHOD(FeatureDefn::New) {
  FeatureDefn *f;

  if (!info.IsConstructCall()) {
    Napi::Error::New(node_gdal::napi_env(), "Cannot call constructor as function, you need to use 'new' keyword").ThrowAsJavaScriptException();
    return node_gdal::napi_env().Undefined();
  }

  if (info[0].IsExternal()) {
    Local<External> ext = info[0].As<Napi::External<void>>();
    void *ptr = ext->Value();
    f = static_cast<FeatureDefn *>(ptr);
  } else {
    if (info.Length() != 0) {
      Napi::Error::New(node_gdal::napi_env(), "FeatureDefn constructor doesn't take any arguments").ThrowAsJavaScriptException();
      return node_gdal::napi_env().Undefined();
    }
    f = new FeatureDefn(new OGRFeatureDefn());
    f->this_->Reference();
  }

  Napi::Value fields = FeatureDefnFields::New(info.This());
  GDAL_SET_PRIVATE(info.This(), "fields_", fields);

  f->Wrap(info.This());
  return info.This();
}

// Currently read-only feature definitions are copied.
// Modifying a feature definition that should have been
// read-only modifies a shadow copy without any real effect.
// TODO: Implement proper read-only objects that throw

Napi::Value FeatureDefn::New(const OGRFeatureDefn *def) {
  if (!def) { return node_gdal::napi_env().Null(); }
  OGRFeatureDefn *copy = def->Clone();
  return FeatureDefn::New(copy, true);
}

Napi::Value FeatureDefn::New(OGRFeatureDefn *def, bool owned) {

  if (!def) { return node_gdal::napi_env().Null(); }
  if (!owned) { def = def->Clone(); }

  std::vector<napi_value> args = {Napi::External<void>::New(node_gdal::napi_env(), def)};
  Napi::Object obj = FeatureDefn::constructor.Value().New(args);
  FeatureDefn *wrapped = node_gdal::UnwrapWrapped<FeatureDefn>(obj);

  return obj;
}

NAN_METHOD(FeatureDefn::toString) {
  return Napi::String::New(node_gdal::napi_env(), "FeatureDefn");
}

/**
 * Clones the feature definition.
 *
 * @method clone
 * @instance
 * @memberof FeatureDefn
 * @return {FeatureDefn}
 */
NAN_METHOD(FeatureDefn::clone) {
  FeatureDefn *def = node_gdal::UnwrapWrapped<FeatureDefn>(info.This().As<Napi::Object>());
  return FeatureDefn::New(def->this_->Clone());
}

/**
 * @readonly
 * @kind member
 * @name name
 * @instance
 * @memberof FeatureDefn
 * @type {string}
 */
NAN_GETTER(FeatureDefn::nameGetter) {
  FeatureDefn *def = node_gdal::UnwrapWrapped<FeatureDefn>(info.This().As<Napi::Object>());
  return SafeString::New(def->this_->GetName());
}

/**
 * WKB geometry type ({@link wkbGeometryType|see table}
 *
 * @kind member
 * @name geomType
 * @instance
 * @memberof FeatureDefn
 * @type {number}
 */
NAN_GETTER(FeatureDefn::geomTypeGetter) {
  FeatureDefn *def = node_gdal::UnwrapWrapped<FeatureDefn>(info.This().As<Napi::Object>());
  return Napi::Number::New(node_gdal::napi_env(), def->this_->GetGeomType());
}

/**
 * @kind member
 * @name geomIgnored
 * @instance
 * @memberof FeatureDefn
 * @type {boolean}
 */
NAN_GETTER(FeatureDefn::geomIgnoredGetter) {
  FeatureDefn *def = node_gdal::UnwrapWrapped<FeatureDefn>(info.This().As<Napi::Object>());
  return Napi::Boolean::New(node_gdal::napi_env(), def->this_->IsGeometryIgnored());
}

/**
 * @kind member
 * @name styleIgnored
 * @instance
 * @memberof FeatureDefn
 * @type {boolean}
 */
NAN_GETTER(FeatureDefn::styleIgnoredGetter) {
  FeatureDefn *def = node_gdal::UnwrapWrapped<FeatureDefn>(info.This().As<Napi::Object>());
  return Napi::Boolean::New(node_gdal::napi_env(), def->this_->IsStyleIgnored());
}

/**
 * @readonly
 * @kind member
 * @name fields
 * @instance
 * @memberof FeatureDefn
 * @type {FeatureDefnFields}
 */
NAN_GETTER(FeatureDefn::fieldsGetter) {
  return GDAL_GET_PRIVATE(info.This(), "fields_");
}

NAN_SETTER(FeatureDefn::geomTypeSetter) {
  FeatureDefn *def = node_gdal::UnwrapWrapped<FeatureDefn>(info.This().As<Napi::Object>());
  if (!value->IsInt32()) {
    Napi::Error::New(node_gdal::napi_env(), "geomType must be an integer").ThrowAsJavaScriptException();
    return;
  }
  def->this_->SetGeomType(OGRwkbGeometryType(value.As<Napi::Number>().Int64Value()));
}

NAN_SETTER(FeatureDefn::geomIgnoredSetter) {
  FeatureDefn *def = node_gdal::UnwrapWrapped<FeatureDefn>(info.This().As<Napi::Object>());
  if (!value->IsBoolean()) {
    Napi::Error::New(node_gdal::napi_env(), "geomIgnored must be a boolean").ThrowAsJavaScriptException();
    return;
  }
  def->this_->SetGeometryIgnored(value.As<Napi::Number>().Int64Value());
}

NAN_SETTER(FeatureDefn::styleIgnoredSetter) {
  FeatureDefn *def = node_gdal::UnwrapWrapped<FeatureDefn>(info.This().As<Napi::Object>());
  if (!value->IsBoolean()) {
    Napi::Error::New(node_gdal::napi_env(), "styleIgnored must be a boolean").ThrowAsJavaScriptException();
    return;
  }
  def->this_->SetStyleIgnored(value.As<Napi::Number>().Int64Value());
}

} // namespace node_gdal
