
#include "gdal_linestring.hpp"
#include "../collections/linestring_points.hpp"
#include "../gdal_common.hpp"
#include "gdal_geometry.hpp"
#include "gdal_simplecurve.hpp"
#include "gdal_point.hpp"

#include <stdlib.h>

namespace node_gdal {

Napi::FunctionReference LineString::constructor;

void LineString::Initialize(Napi::Object target) {
  Napi::Env env = target.Env();
  SELF_CLASS(LineString);

  // NOTE: the descriptor macros carry their own trailing comma
  Napi::Function lcons = DefineClass(env, "LineString",
    {
        METHOD(toString)
    });

  // lcons->Inherit() has no DefineClass equivalent, chain the prototypes by hand
  Napi::Function base = SimpleCurve::constructor.Value();
  lcons.Get("prototype").As<Napi::Object>().SetPrototypeOf(base.Get("prototype").As<Napi::Object>());
  lcons.SetPrototypeOf(base);

  target.Set("LineString", lcons);

  constructor = Napi::Persistent(lcons);
  constructor.SuppressDestruct();
}

/**
 * Concrete representation of a multi-vertex line.
 *
 * @example
 *
 * var lineString = new gdal.LineString();
 * lineString.points.add(new gdal.Point(0,0));
 * lineString.points.add(new gdal.Point(0,10));
 *
 * @constructor
 * @class LineString
 * @extends SimpleCurve
 */

NAN_METHOD(LineString::toString) {
  return Napi::String::New(node_gdal::napi_env, "LineString");
}

} // namespace node_gdal
