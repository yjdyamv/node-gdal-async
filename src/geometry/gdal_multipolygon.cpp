#include "gdal_multipolygon.hpp"
#include "../collections/geometry_collection_children.hpp"
#include "../gdal_common.hpp"
#include "gdal_geometry.hpp"
#include "gdal_geometrycollection.hpp"
#include "gdal_polygon.hpp"

#include <stdlib.h>

namespace node_gdal {

Napi::FunctionReference MultiPolygon::constructor;

void MultiPolygon::Initialize(Napi::Object target) {
  Napi::Env env = target.Env();
  SELF_CLASS(MultiPolygon);

  // NOTE: the descriptor macros carry their own trailing comma
  Napi::Function lcons = DefineClass(env, "MultiPolygon",
    {
        METHOD(toString)
        METHOD(unionCascaded)
        METHOD(getArea)
    });

  // lcons->Inherit() has no DefineClass equivalent, chain the prototypes by hand
  Napi::Function base = GeometryCollection::constructor.Value();
  lcons.Get("prototype").As<Napi::Object>().SetPrototypeOf(base.Get("prototype").As<Napi::Object>());
  lcons.SetPrototypeOf(base);

  target.Set("MultiPolygon", lcons);

  constructor = Napi::Persistent(lcons);
  constructor.SuppressDestruct();
}

/**
 * @constructor
 * @class MultiPolygon
 * @extends GeometryCollection
 */

NAN_METHOD(MultiPolygon::toString) {
  return Napi::String::New(node_gdal::napi_env, "MultiPolygon");
}

/**
 * Unions all the geometries and returns the result.
 *
 * @method unionCascaded
 * @instance
 * @memberof MultiPolygon
 * @return {Geometry}
 */
NAN_METHOD(MultiPolygon::unionCascaded) {

  MultiPolygon *geom = node_gdal::UnwrapWrapped<MultiPolygon>(info.This().As<Napi::Object>());
  auto r = geom->this_->UnionCascaded();
  if (r == nullptr) {
    NODE_THROW_LAST_CPLERR;
    return node_gdal::napi_env.Undefined();
  }

  return Geometry::New(r);
}

/**
 * Computes the combined area of the collection.
 *
 * @method getArea
 * @instance
 * @memberof MultiPolygon
 * @return {number}
 */
NODE_WRAPPED_METHOD_WITH_RESULT(MultiPolygon, getArea, Number, get_Area);

} // namespace node_gdal
