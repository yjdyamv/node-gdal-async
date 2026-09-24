
#include "gdal_linearring.hpp"
#include "../collections/linestring_points.hpp"
#include "../gdal_common.hpp"
#include "gdal_geometry.hpp"
#include "gdal_linestring.hpp"

#include <stdlib.h>

namespace node_gdal {

Napi::FunctionReference LinearRing::constructor;

void LinearRing::Initialize(Napi::Object target) {
  Napi::Env env = target.Env();
  SELF_CLASS(LinearRing);

  // NOTE: the descriptor macros carry their own trailing comma
  Napi::Function lcons = DefineClass(env, "LinearRing",
    {
        METHOD(toString)
        METHOD(getArea)
        METHOD(addSubLineString)
    });

  // lcons->Inherit() has no DefineClass equivalent
  node_gdal::Inherit(lcons, LineString::constructor.Value());

  target.Set("LinearRing", lcons);

  constructor = Napi::Persistent(lcons);
  constructor.SuppressDestruct();
}

Napi::Value LinearRing::New(OGRLinearRing *geom, bool owned) {

  if (!geom) { return node_gdal::napi_env.Null(); }

  // make a copy of geometry owned by a feature
  // + no need to track when a feature is destroyed
  // + no need to throw errors when a method trys to modify an owned read-only
  // geometry
  // - is slower

  if (!owned) { geom = static_cast<OGRLinearRing *>(geom->clone()); }

  LinearRing *wrapped = new LinearRing(geom);
  wrapped->owned_ = true;

  Napi::Value ext = Nan::New<External>(wrapped);
  Napi::Object obj =
    Nan::NewInstance(Nan::GetFunction(Napi::String::New(node_gdal::napi_env, LinearRing::constructor)), 1, &ext).ToLocalChecked();

  return obj;
}

/**
 * Concrete representation of a closed ring.
 *
 * @constructor
 * @class LinearRing
 * @extends LineString
 */

NAN_METHOD(LinearRing::toString) {
  return Napi::String::New(node_gdal::napi_env, "LinearRing");
}

/**
 * Computes the area enclosed by the ring.
 *
 * @method getArea
 * @instance
 * @memberof LinearRing
 * @return {number}
 */
NODE_WRAPPED_METHOD_WITH_RESULT(LinearRing, getArea, Number, get_Area);

NAN_METHOD(LinearRing::addSubLineString) {

  LinearRing *geom = node_gdal::UnwrapWrapped<LinearRing>(info.This().As<Napi::Object>());
  LineString *other;
  int start = 0;
  int end = -1;

  NODE_ARG_WRAPPED(0, "line", LineString, other);
  NODE_ARG_INT_OPT(1, "start", start);
  NODE_ARG_INT_OPT(2, "end", end);

  int n = other->get()->getNumPoints();

  if (start < 0 || end < -1 || start >= n || end >= n) {
    Napi::RangeError::New(node_gdal::napi_env, "Invalid start or end index for LineString").ThrowAsJavaScriptException();
    return node_gdal::napi_env.Undefined();
  }

  geom->this_->addSubLineString(other->get(), start, end);

  return node_gdal::napi_env.Undefined();
}

} // namespace node_gdal
