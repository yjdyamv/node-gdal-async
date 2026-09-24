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

Dimension::Dimension(const Napi::CallbackInfo &info) : GDALObject<Dimension>(info), uid(0), this_(0), parent_ds(0) {
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
    Local<External> ext = info[0].As<Napi::External<void>>();
    void *ptr = ext->Value();
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

  std::vector<napi_value> args = {Napi::External<void>::New(node_gdal::napi_env, raw)};
  Napi::Object obj = Dimension::constructor.Value().New(args);
  Dimension *wrapped = node_gdal::UnwrapWrapped<Dimension>(obj);

  Dataset *unwrapped_ds = node_gdal::UnwrapWrapped<Dataset>(ds);
  long parent_uid = unwrapped_ds->uid;

  wrapped->uid = object_store.add(raw, wrapped->persistent(), parent_uid);
  wrapped->parent_ds = parent_ds;
  wrapped->parent_uid = parent_uid;

  GDAL_SET_PRIVATE(obj, "ds_", ds);

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
