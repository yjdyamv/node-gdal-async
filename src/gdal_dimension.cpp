#include "gdal_dimension.hpp"
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

Napi::FunctionReference Dimension::constructor;

void Dimension::Initialize(Napi::Object target) {
  Napi::Env env = target.Env();
  SELF_CLASS(Dimension);

  // NOTE: the descriptor macros carry their own trailing comma
  Napi::Function lcons = DefineClass(env, "Dimension",
    {
        METHOD(toString)
        ATTR_DONT_ENUM(lcons, "_uid", uidGetter, READ_ONLY_SETTER)
        ATTR(lcons, "size", sizeGetter, READ_ONLY_SETTER)
        ATTR(lcons, "description", descriptionGetter, READ_ONLY_SETTER)
        ATTR(lcons, "type", typeGetter, READ_ONLY_SETTER)
        ATTR(lcons, "direction", directionGetter, READ_ONLY_SETTER)
    });

  target.Set("Dimension", lcons);

  constructor = Napi::Persistent(lcons);
  constructor.SuppressDestruct();
}

Dimension::Dimension(std::shared_ptr<GDALDimension> dimension)
  : Nan::ObjectWrap(), uid(0), this_(dimension), parent_ds(0) {
  LOG("Created dimension [%p]", dimension.get());
}

Dimension::Dimension() : Nan::ObjectWrap(), uid(0), this_(0), parent_ds(0) {
}

Dimension::~Dimension() {
  dispose();
}

void Dimension::dispose() {
  if (this_) {

    LOG("Disposing dimension [%p]", this_.get());

    object_store.dispose(uid);

    LOG("Disposed dimension [%p]", this_.get());
  }
};

/**
 * A representation of a group with access methods.
 *
 * @class Dimension
 */
NAN_METHOD(Dimension::New) {

  if (!info.IsConstructCall()) {
    Napi::Error::New(node_gdal::napi_env, "Cannot call constructor as function, you need to use 'new' keyword").ThrowAsJavaScriptException();
    return node_gdal::napi_env.Undefined();
  }

  if (info.Length() == 1 && info[0].IsExternal()) {
    Local<External> ext = info[0].As<External>();
    void *ptr = ext->Value(V8_TYPE_TAG);
    Dimension *f = static_cast<Dimension *>(ptr);
    f->Wrap(info.This());

    return info.This();
    return node_gdal::napi_env.Undefined();
  } else {
    Napi::Error::New(node_gdal::napi_env, "Cannot create dimension directly. Create with dataset instead.").ThrowAsJavaScriptException();
    return node_gdal::napi_env.Undefined();
  }

  return info.This();
}

Napi::Value Dimension::New(std::shared_ptr<GDALDimension> raw, GDALDataset *parent_ds) {

  if (!raw) { return node_gdal::napi_env.Null(); }
  if (object_store.has(raw)) { return object_store.get(raw); }

  Dimension *wrapped = new Dimension(raw);

  Napi::Object ds;
  if (object_store.has(parent_ds)) {
    ds = object_store.get(parent_ds);
  } else {
    LOG("Dimension's parent dataset disappeared from cache (array = %p, dataset = %p)", raw.get(), parent_ds);
    Napi::Error::New(node_gdal::napi_env, "Dimension's parent dataset disappeared from cache").ThrowAsJavaScriptException();
    return node_gdal::napi_env.Undefined();
  }

  Napi::Value ext = Nan::New<External>(wrapped);
  Napi::Object obj =
    Nan::NewInstance(Nan::GetFunction(Napi::String::New(node_gdal::napi_env, Dimension::constructor)), 1, &ext).ToLocalChecked();

  Dataset *unwrapped_ds = node_gdal::UnwrapWrapped<Dataset>(ds);
  long parent_uid = unwrapped_ds->uid;

  wrapped->uid = object_store.add(raw, wrapped->persistent(), parent_uid);
  wrapped->parent_ds = parent_ds;
  wrapped->parent_uid = parent_uid;

  Nan::SetPrivate(obj, Napi::String::New(node_gdal::napi_env, "ds_"), ds);

  return obj;
}

NAN_METHOD(Dimension::toString) {
  return Napi::String::New(node_gdal::napi_env, "Dimension");
}

/**
 * @readonly
 * @kind member
 * @name size
 * @instance
 * @memberof Dimension
 * @type {number}
 */
NODE_WRAPPED_GETTER_WITH_RESULT_LOCKED(Dimension, sizeGetter, Number, GetSize);

/**
 * @readonly
 * @kind member
 * @name description
 * @instance
 * @memberof Dimension
 * @type {string}
 */
NODE_WRAPPED_GETTER_WITH_STRING_LOCKED(Dimension, descriptionGetter, GetFullName);

/**
 * @readonly
 * @kind member
 * @name direction
 * @instance
 * @memberof Dimension
 * @type {string}
 */
NODE_WRAPPED_GETTER_WITH_STRING_LOCKED(Dimension, directionGetter, GetDirection);

/**
 * @readonly
 * @kind member
 * @name type
 * @instance
 * @memberof Dimension
 * @type {string}
 */
NODE_WRAPPED_GETTER_WITH_STRING_LOCKED(Dimension, typeGetter, GetType);

NAN_GETTER(Dimension::uidGetter) {
  Dimension *group = node_gdal::UnwrapWrapped<Dimension>(info.This().As<Napi::Object>());
  return Napi::Number::New(node_gdal::napi_env, (int)group->uid);
}

#endif

} // namespace node_gdal
