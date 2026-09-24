#include "gdal_attribute.hpp"
#include "gdal_common.hpp"
#include "gdal_driver.hpp"
#include "gdal_group.hpp"
#include "collections/group_groups.hpp"
#include "collections/group_arrays.hpp"
#include "geometry/gdal_geometry.hpp"
#include "gdal_layer.hpp"
#include "gdal_majorobject.hpp"
#include "gdal_rasterband.hpp"
#include "gdal_spatial_reference.hpp"

namespace node_gdal {

#if GDAL_VERSION_MAJOR > 3 || (GDAL_VERSION_MAJOR == 3 && GDAL_VERSION_MINOR >= 1)

Napi::FunctionReference Attribute::constructor;

void Attribute::Initialize(Napi::Object target) {
  Napi::Env env = target.Env();
  SELF_CLASS(Attribute);

  // NOTE: the descriptor macros carry their own trailing comma
  Napi::Function lcons = DefineClass(env, "Attribute",
    {
        METHOD(toString)
        ATTR_DONT_ENUM(lcons, "_uid", uidGetter, READ_ONLY_SETTER)
        ATTR(lcons, "dataType", typeGetter, READ_ONLY_SETTER)
        ATTR(lcons, "value", valueGetter, READ_ONLY_SETTER)
    });

  target.Set("Attribute", lcons);

  constructor = Napi::Persistent(lcons);
  constructor.SuppressDestruct();
}

Attribute::Attribute(std::shared_ptr<GDALAttribute> attribute)
  : Nan::ObjectWrap(), uid(0), this_(attribute), parent_ds(0) {
  LOG("Created attribute [%p]", attribute.get());
}

Attribute::Attribute() : Nan::ObjectWrap(), uid(0), this_(0), parent_ds(0) {
}

Attribute::~Attribute() {
  dispose();
}

void Attribute::dispose() {
  if (this_) {

    LOG("Disposing attribute [%p]", this_.get());

    object_store.dispose(uid);

    LOG("Disposed attribute [%p]", this_.get());
  }
};

/**
 * A representation of a group with access methods.
 *
 * @class Attribute
 */
NAN_METHOD(Attribute::New) {
  if (!info.IsConstructCall()) {
    Napi::Error::New(node_gdal::napi_env, "Cannot call constructor as function, you need to use 'new' keyword").ThrowAsJavaScriptException();
    return node_gdal::napi_env.Undefined();
  }

  if (info.Length() == 1 && info[0].IsExternal()) {
    Local<External> ext = info[0].As<External>();
    void *ptr = ext->Value(V8_TYPE_TAG);
    Attribute *f = static_cast<Attribute *>(ptr);
    f->Wrap(info.This());

    return info.This();
    return node_gdal::napi_env.Undefined();
  } else {
    Napi::Error::New(node_gdal::napi_env, "Cannot create attribute directly. Create with dataset instead.").ThrowAsJavaScriptException();
    return node_gdal::napi_env.Undefined();
  }

  return info.This();
}

Napi::Value Attribute::New(std::shared_ptr<GDALAttribute> raw, GDALDataset *parent_ds) {

  if (!raw) { return node_gdal::napi_env.Null(); }
  if (object_store.has(raw)) { return object_store.get(raw); }

  Attribute *wrapped = new Attribute(raw);

  Napi::Object ds;
  if (object_store.has(parent_ds)) {
    ds = object_store.get(parent_ds);
  } else {
    LOG("Attribute's parent dataset disappeared from cache (array = %p, dataset = %p)", raw.get(), parent_ds);
    Napi::Error::New(node_gdal::napi_env, "Attribute's parent dataset disappeared from cache").ThrowAsJavaScriptException();
    return node_gdal::napi_env.Undefined();
  }

  Napi::Value ext = Nan::New<External>(wrapped);
  Napi::Object obj =
    Nan::NewInstance(Nan::GetFunction(Napi::String::New(node_gdal::napi_env, Attribute::constructor)), 1, &ext).ToLocalChecked();

  Dataset *unwrapped_ds = node_gdal::UnwrapWrapped<Dataset>(ds);
  long parent_uid = unwrapped_ds->uid;

  wrapped->uid = object_store.add(raw, wrapped->persistent(), parent_uid);
  wrapped->parent_ds = parent_ds;
  wrapped->parent_uid = parent_uid;

  Nan::SetPrivate(obj, Napi::String::New(node_gdal::napi_env, "ds_"), ds);

  return obj;
}

NAN_METHOD(Attribute::toString) {
  return Napi::String::New(node_gdal::napi_env, "Attribute");
}

/**
 * Complex GDAL data types introduced in 3.1 are not yet supported
 * @readonly
 * @kind member
 * @name value
 * @instance
 * @memberof Attribute
 * @throws {Error}
 * @type {string|number}
 */
NAN_GETTER(Attribute::valueGetter) {
  NODE_UNWRAP_CHECK(Attribute, info.This(), attribute);
  GDAL_RAW_CHECK(std::shared_ptr<GDALAttribute>, attribute, raw);
  GDAL_LOCK_PARENT(attribute);
  GDALExtendedDataType type = raw->GetDataType();
  Napi::Value r;
  switch (type.GetClass()) {
    case GEDTC_NUMERIC: r = Napi::Number::New(node_gdal::napi_env, raw->ReadAsDouble()); break;
    case GEDTC_STRING: r = SafeString::New(raw->ReadAsString()); break;
    default: Napi::Error::New(node_gdal::napi_env, "Compound attributes are not supported yet").ThrowAsJavaScriptException(); return;
  }

  return r;
}

/**
 * @readonly
 * @kind member
 * @name dataType
 * @instance
 * @memberof Attribute
 * @type {string}
 */
NAN_GETTER(Attribute::typeGetter) {
  NODE_UNWRAP_CHECK(Attribute, info.This(), attribute);
  GDAL_RAW_CHECK(std::shared_ptr<GDALAttribute>, attribute, raw);
  GDAL_LOCK_PARENT(attribute);
  GDALExtendedDataType type = raw->GetDataType();
  const char *r;
  switch (type.GetClass()) {
    case GEDTC_NUMERIC: r = GDALGetDataTypeName(type.GetNumericDataType()); break;
    case GEDTC_STRING: r = "String"; break;
    case GEDTC_COMPOUND: r = "Compound"; break;
    default: Napi::Error::New(node_gdal::napi_env, "Invalid attribute type").ThrowAsJavaScriptException(); return;
  }

  return SafeString::New(r);
}

NAN_GETTER(Attribute::uidGetter) {
  Attribute *group = node_gdal::UnwrapWrapped<Attribute>(info.This().As<Napi::Object>());
  return Napi::Number::New(node_gdal::napi_env, (int)group->uid);
}

#endif

} // namespace node_gdal
