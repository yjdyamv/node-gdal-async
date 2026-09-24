#include "polygon_rings.hpp"
#include "../gdal_common.hpp"
#include "../geometry/gdal_geometry.hpp"
#include "../geometry/gdal_linearring.hpp"
#include "../geometry/gdal_polygon.hpp"

namespace node_gdal {

Napi::FunctionReference PolygonRings::constructor;

void PolygonRings::Initialize(Napi::Object target) {
  Napi::Env env = target.Env();
  SELF_CLASS(PolygonRings);

  // NOTE: the descriptor macros carry their own trailing comma
  Napi::Function lcons = DefineClass(env, "PolygonRings",
    {
        METHOD(toString)
        METHOD(count)
        METHOD(get)
        METHOD(add)
    });

  target.Set("PolygonRings", lcons);

  constructor = Napi::Persistent(lcons);
  constructor.SuppressDestruct();
}

PolygonRings::PolygonRings() : Nan::ObjectWrap() {
}

PolygonRings::~PolygonRings() {
}

/**
 * A collection of polygon rings, used by {@link Polygon}.
 *
 * @class PolygonRings
 */
NAN_METHOD(PolygonRings::New) {

  if (!info.IsConstructCall()) {
    Napi::Error::New(node_gdal::napi_env, "Cannot call constructor as function, you need to use 'new' keyword").ThrowAsJavaScriptException();
    return node_gdal::napi_env.Undefined();
  }
  if (info[0].IsExternal()) {
    Local<External> ext = info[0].As<External>();
    void *ptr = ext->Value(V8_TYPE_TAG);
    PolygonRings *geom = static_cast<PolygonRings *>(ptr);
    geom->Wrap(info.This());
    return info.This();
    return node_gdal::napi_env.Undefined();
  } else {
    Napi::Error::New(node_gdal::napi_env, "Cannot create PolygonRings directly").ThrowAsJavaScriptException();
    return node_gdal::napi_env.Undefined();
  }
}

Napi::Value PolygonRings::New(Napi::Value geom) {

  PolygonRings *wrapped = new PolygonRings();

  Napi::Value ext = Nan::New<External>(wrapped);
  v8::Local<v8::Object> obj =
    Nan::NewInstance(Nan::GetFunction(Napi::String::New(node_gdal::napi_env, PolygonRings::constructor)), 1, &ext).ToLocalChecked();
  Nan::SetPrivate(obj, Napi::String::New(node_gdal::napi_env, "parent_"), geom);

  return obj;
}

NAN_METHOD(PolygonRings::toString) {
  return Napi::String::New(node_gdal::napi_env, "PolygonRings");
}

/**
 * Returns the number of rings that exist in the collection.
 *
 * @method count
 * @instance
 * @memberof PolygonRings
 * @return {number}
 */
NAN_METHOD(PolygonRings::count) {

  Napi::Object parent =
    Nan::GetPrivate(info.This(), Napi::String::New(node_gdal::napi_env, "parent_")).ToLocalChecked().As<Object>();
  Polygon *geom = node_gdal::UnwrapWrapped<Polygon>(parent);

  int i = geom->get()->getExteriorRing() ? 1 : 0;
  i += geom->get()->getNumInteriorRings();

  return Napi::Number::New(node_gdal::napi_env, i);
}

/**
 * Returns the ring at the specified index. The ring
 * at index `0` will always be the polygon's exterior ring.
 *
 * @example
 *
 * var exterior = polygon.rings.get(0);
 * var interior = polygon.rings.get(1);
 *
 * @method get
 * @instance
 * @memberof PolygonRings
 * @param {number} index
 * @throws {Error}
 * @return {LinearRing}
 */
NAN_METHOD(PolygonRings::get) {

  Napi::Object parent =
    Nan::GetPrivate(info.This(), Napi::String::New(node_gdal::napi_env, "parent_")).ToLocalChecked().As<Object>();
  Polygon *geom = node_gdal::UnwrapWrapped<Polygon>(parent);

  int i;
  NODE_ARG_INT(0, "index", i);

  OGRLinearRing *r;
  if (i == 0) {
    r = geom->get()->getExteriorRing();
  } else {
    r = geom->get()->getInteriorRing(i - 1);
  }
  if (r == nullptr) {
    NODE_THROW_LAST_CPLERR;
    return node_gdal::napi_env.Undefined();
  }
  return LinearRing::New(r, false);
}

/**
 * Adds a ring to the collection.
 *
 * @example
 *
 * var ring1 = new gdal.LinearRing();
 * ring1.points.add(0,0);
 * ring1.points.add(1,0);
 * ring1.points.add(1,1);
 * ring1.points.add(0,1);
 * ring1.points.add(0,0);
 *
 * // one at a time:
 * polygon.rings.add(ring1);
 *
 * // many at once:
 * polygon.rings.add([ring1, ...]);
 *
 * @method add
 * @instance
 * @memberof PolygonRings
 * @param {LinearRing|LinearRing[]} rings
 */
NAN_METHOD(PolygonRings::add) {

  Napi::Object parent =
    Nan::GetPrivate(info.This(), Napi::String::New(node_gdal::napi_env, "parent_")).ToLocalChecked().As<Object>();
  Polygon *geom = node_gdal::UnwrapWrapped<Polygon>(parent);

  LinearRing *ring;

  if (info.Length() < 1) {
    Napi::Error::New(node_gdal::napi_env, "ring(s) must be given").ThrowAsJavaScriptException();
    return node_gdal::napi_env.Undefined();
  }
  if (info[0].IsArray()) {
    // set from array of geometry objects
    Napi::Array array = info[0].As<Array>();
    int length = array->Length();
    for (int i = 0; i < length; i++) {
      Napi::Value element = Nan::Get(array, i).ToLocalChecked();
      if (IS_WRAPPED(element, LinearRing)) {
        ring = node_gdal::UnwrapWrapped<LinearRing>(element.As<Object>());
        OGRErr err = geom->get()->addRing(ring->get());
        if (err) {
          NODE_THROW_OGRERR(err);
          return node_gdal::napi_env.Undefined();
        }
      } else {
        Napi::Error::New(node_gdal::napi_env, "All array elements must be LinearRings").ThrowAsJavaScriptException();
        return node_gdal::napi_env.Undefined();
      }
    }
  } else if (IS_WRAPPED(info[0], LinearRing)) {
    ring = node_gdal::UnwrapWrapped<LinearRing>(info[0].As<Object>());
    OGRErr err = geom->get()->addRing(ring->get());
    if (err) {
      NODE_THROW_OGRERR(err);
      return node_gdal::napi_env.Undefined();
    }
  } else {
    Napi::Error::New(node_gdal::napi_env, "ring(s) must be a LinearRing or array of LinearRings").ThrowAsJavaScriptException();
    return node_gdal::napi_env.Undefined();
  }

  return node_gdal::napi_env.Undefined();
}

} // namespace node_gdal
