
#include "gdal_polygon.hpp"
#include "../collections/polygon_rings.hpp"
#include "../gdal_common.hpp"
#include "gdal_geometry.hpp"

#include <stdlib.h>

namespace node_gdal {

Napi::FunctionReference Polygon::constructor;

void Polygon::Initialize(Napi::Object target) {
  Napi::Env env = target.Env();
  SELF_CLASS(Polygon);

  // NOTE: the descriptor macros carry their own trailing comma
  Napi::Function lcons = DefineClass(env, "Polygon",
    {
        METHOD(toString)
        METHOD(getArea)
        ATTR(lcons, "rings", ringsGetter, READ_ONLY_SETTER)
    });

  // lcons->Inherit() has no DefineClass equivalent
  node_gdal::Inherit(lcons, Geometry::constructor.Value());

  target.Set("Polygon", lcons);

  constructor = Napi::Persistent(lcons);
  constructor.SuppressDestruct();
}

void Polygon::SetPrivate(Nan::ADDON_REGISTER_FUNCTION_ARGS_TYPE _this, Napi::Value value) {
  Nan::SetPrivate(_this, Napi::String::New(node_gdal::napi_env, "rings_"), value);
};

/**
 * Concrete class representing polygons.
 *
 * @constructor
 * @class Polygon
 * @extends Geometry
 */

NAN_METHOD(Polygon::toString) {
  return Napi::String::New(node_gdal::napi_env, "Polygon");
}

/**
 * Computes the area of the polygon.
 *
 * @method getArea
 * @instance
 * @memberof Polygon
 * @return {number}
 */
NODE_WRAPPED_METHOD_WITH_RESULT(Polygon, getArea, Number, get_Area);

/**
 * The rings that make up the polygon geometry.
 *
 * @kind member
 * @name rings
 * @instance
 * @memberof Polygon
 * @type {PolygonRings}
 */
NAN_GETTER(Polygon::ringsGetter) {
  return Nan::GetPrivate(info.This(), Napi::String::New(node_gdal::napi_env, "rings_")).ToLocalChecked();
}

} // namespace node_gdal
