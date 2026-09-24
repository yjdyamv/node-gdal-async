
#include "gdal_multicurve.hpp"
#include "../collections/geometry_collection_children.hpp"
#include "../gdal_common.hpp"
#include "gdal_geometry.hpp"
#include "gdal_geometrycollection.hpp"

#include <stdlib.h>

namespace node_gdal {

Napi::FunctionReference MultiCurve::constructor;

void MultiCurve::Initialize(Napi::Object target) {
  Napi::Env env = target.Env();
  SELF_CLASS(MultiCurve);

  // NOTE: the descriptor macros carry their own trailing comma
  Napi::Function lcons = DefineClass(env, "MultiCurve",
    {
        METHOD(toString)
        METHOD(polygonize)
    });

  // lcons->Inherit() has no DefineClass equivalent, chain the prototypes by hand
  Napi::Function base = GeometryCollection::constructor.Value();
  lcons.Get("prototype").As<Napi::Object>().SetPrototypeOf(base.Get("prototype").As<Napi::Object>());
  lcons.SetPrototypeOf(base);

  target.Set("MultiCurve", lcons);

  constructor = Napi::Persistent(lcons);
  constructor.SuppressDestruct();
}

/**
 * @constructor
 * @class MultiCurve
 * @extends GeometryCollection
 */

NAN_METHOD(MultiCurve::toString) {
  return Napi::String::New(node_gdal::napi_env, "MultiCurve");
}

/**
 * Converts it to a polygon.
 *
 * @method polygonize
 * @instance
 * @memberof MultiCurve
 * @return {Polygon}
 */
NAN_METHOD(MultiCurve::polygonize) {

  MultiCurve *geom = node_gdal::UnwrapWrapped<MultiCurve>(info.This().As<Napi::Object>());

  return Geometry::New(geom->this_->Polygonize());
}

} // namespace node_gdal
