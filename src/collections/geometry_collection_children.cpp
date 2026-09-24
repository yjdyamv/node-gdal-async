#include "geometry_collection_children.hpp"
#include "../gdal_common.hpp"
#include "../geometry/gdal_geometry.hpp"
#include "../geometry/gdal_geometrycollection.hpp"

namespace node_gdal {

Napi::FunctionReference GeometryCollectionChildren::constructor;

void GeometryCollectionChildren::Initialize(Napi::Object target) {
  Napi::Env env = target.Env();
  SELF_CLASS(GeometryCollectionChildren);

  // NOTE: the descriptor macros carry their own trailing comma
  Napi::Function lcons = DefineClass(env, "GeometryCollectionChildren",
    {
        METHOD(toString)
        METHOD(count)
        METHOD(get)
        METHOD(remove)
        METHOD(add)
    });

  target.Set("GeometryCollectionChildren", lcons);

  constructor = Napi::Persistent(lcons);
  constructor.SuppressDestruct();
}

GeometryCollectionChildren::GeometryCollectionChildren(const Napi::CallbackInfo &info) : GDALObject<GeometryCollectionChildren>(info) {
}

GeometryCollectionChildren::~GeometryCollectionChildren() {
}

/**
 * A collection of Geometries, used by {@link GeometryCollection}.
 *
 * @class GeometryCollectionChildren
 */
NAN_METHOD(GeometryCollectionChildren::New) {

  if (!info.IsConstructCall()) {
    Napi::Error::New(node_gdal::napi_env, "Cannot call constructor as function, you need to use 'new' keyword").ThrowAsJavaScriptException();
    return node_gdal::napi_env.Undefined();
  }
  if (info[0].IsExternal()) {
    Local<External> ext = info[0].As<Napi::External<void>>();
    void *ptr = ext->Value();
    GeometryCollectionChildren *geom = static_cast<GeometryCollectionChildren *>(ptr);
    geom->Wrap(info.This());
    return info.This();
    return node_gdal::napi_env.Undefined();
  } else {
    Napi::Error::New(node_gdal::napi_env, "Cannot create GeometryCollectionChildren directly").ThrowAsJavaScriptException();
    return node_gdal::napi_env.Undefined();
  }
}

Napi::Value GeometryCollectionChildren::New(Napi::Value geom) {

  std::vector<napi_value> args;
  Napi::Object obj = GeometryCollectionChildren::constructor.Value().New(args);
  GDAL_SET_PRIVATE(obj, "parent_", geom);

  return obj;
}

NAN_METHOD(GeometryCollectionChildren::toString) {
  return Napi::String::New(node_gdal::napi_env, "GeometryCollectionChildren");
}

/**
 * Returns the number of items.
 *
 * @method count
 * @instance
 * @memberof GeometryCollectionChildren
 * @return {number}
 */
NAN_METHOD(GeometryCollectionChildren::count) {

  Napi::Object parent =
    GDAL_GET_PRIVATE(info.This(), "parent_").As<Napi::Object>();
  GeometryCollection *geom = node_gdal::UnwrapWrapped<GeometryCollection>(parent);

  return Napi::Number::New(node_gdal::napi_env, geom->get()->getNumGeometries());
}

/**
 * Returns the geometry at the specified index.
 *
 * @method get
 * @instance
 * @memberof GeometryCollectionChildren
 * @param {number} index 0-based index
 * @throws {Error}
 * @return {Geometry}
 */
NAN_METHOD(GeometryCollectionChildren::get) {

  Napi::Object parent =
    GDAL_GET_PRIVATE(info.This(), "parent_").As<Napi::Object>();
  GeometryCollection *geom = node_gdal::UnwrapWrapped<GeometryCollection>(parent);

  int i;
  NODE_ARG_INT(0, "index", i);

  auto r = geom->get()->getGeometryRef(i);
  if (r == nullptr) {
    NODE_THROW_LAST_CPLERR;
    return node_gdal::napi_env.Undefined();
  }
  return Geometry::New(r, false);
}

/**
 * Removes the geometry at the specified index.
 *
 * @method remove
 * @instance
 * @memberof GeometryCollectionChildren
 * @param {number} index 0-based index, -1 for all geometries
 */
NAN_METHOD(GeometryCollectionChildren::remove) {

  Napi::Object parent =
    GDAL_GET_PRIVATE(info.This(), "parent_").As<Napi::Object>();
  GeometryCollection *geom = node_gdal::UnwrapWrapped<GeometryCollection>(parent);

  int i;
  NODE_ARG_INT(0, "index", i);

  OGRErr err = geom->get()->removeGeometry(i);
  if (err) {
    NODE_THROW_OGRERR(err);
    return node_gdal::napi_env.Undefined();
  }

  return node_gdal::napi_env.Undefined();
}

/**
 * Adds geometry(s) to the collection.
 *
 * @example
 *
 * // one at a time:
 * geometryCollection.children.add(new Point(0,0,0));
 *
 * // add many at once:
 * geometryCollection.children.add([
 *     new Point(1,0,0),
 *     new Point(1,0,0)
 * ]);
 *
 * @method add
 * @instance
 * @memberof GeometryCollectionChildren
 * @param {Geometry|Geometry[]} geometry
 */
NAN_METHOD(GeometryCollectionChildren::add) {

  Napi::Object parent =
    GDAL_GET_PRIVATE(info.This(), "parent_").As<Napi::Object>();
  GeometryCollection *geom = node_gdal::UnwrapWrapped<GeometryCollection>(parent);

  Geometry *child;

  if (info.Length() < 1) {
    Napi::Error::New(node_gdal::napi_env, "child(ren) must be given").ThrowAsJavaScriptException();
    return node_gdal::napi_env.Undefined();
  }
  if (info[0].IsArray()) {
    // set from array of geometry objects
    Napi::Array array = info[0].As<Napi::Array>();
    int length = array.Length();
    for (int i = 0; i < length; i++) {
      Napi::Value element = array.As<Napi::Object>().Get(i);
      if (IS_WRAPPED(element, Geometry)) {
        child = node_gdal::UnwrapWrapped<Geometry>(element.As<Napi::Object>());
        OGRErr err = geom->get()->addGeometry(child->get());
        if (err) {
          NODE_THROW_OGRERR(err);
          return node_gdal::napi_env.Undefined();
        }
      } else {
        Napi::Error::New(node_gdal::napi_env, "All array elements must be geometry objects").ThrowAsJavaScriptException();
        return node_gdal::napi_env.Undefined();
      }
    }
  } else if (IS_WRAPPED(info[0], Geometry)) {
    child = node_gdal::UnwrapWrapped<Geometry>(info[0].As<Napi::Object>());
    OGRErr err = geom->get()->addGeometry(child->get());
    if (err) {
      NODE_THROW_OGRERR(err);
      return node_gdal::napi_env.Undefined();
    }
  } else {
    Napi::Error::New(node_gdal::napi_env, "child must be a geometry object or array of geometry objects").ThrowAsJavaScriptException();
    return node_gdal::napi_env.Undefined();
  }

  return node_gdal::napi_env.Undefined();
}

} // namespace node_gdal
