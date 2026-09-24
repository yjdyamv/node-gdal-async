
#include "gdal_compoundcurve.hpp"
#include "../collections/linestring_points.hpp"
#include "../gdal_common.hpp"
#include "gdal_geometry.hpp"
#include "gdal_point.hpp"

#include <stdlib.h>

namespace node_gdal {

Napi::FunctionReference CompoundCurve::constructor;

void CompoundCurve::Initialize(Napi::Object target) {
  Napi::Env env = target.Env();
  SELF_CLASS(CompoundCurve);

  // NOTE: the descriptor macros carry their own trailing comma
  Napi::Function lcons = DefineClass(env, "CompoundCurve",
    {
        METHOD(toString)
        ATTR(lcons, "curves", curvesGetter, READ_ONLY_SETTER)
    });

  // lcons->Inherit() has no DefineClass equivalent
  node_gdal::Inherit(lcons, Geometry::constructor.Value());

  target.Set("CompoundCurve", lcons);

  constructor = Napi::Persistent(lcons);
  constructor.SuppressDestruct();
}

void CompoundCurve::SetPrivate(Nan::ADDON_REGISTER_FUNCTION_ARGS_TYPE _this, Napi::Value value) {
  Nan::SetPrivate(_this, Napi::String::New(node_gdal::napi_env, "curves_"), value);
};

/**
 * Concrete representation of a compound contionuos curve.
 *
 * @example
 *
 * var CompoundCurve = new gdal.CompoundCurve();
 * CompoundCurve.points.add(new gdal.Point(0,0));
 * CompoundCurve.points.add(new gdal.Point(0,10));
 *
 * @constructor
 * @class CompoundCurve
 * @extends Geometry
 */

NAN_METHOD(CompoundCurve::toString) {
  return Napi::String::New(node_gdal::napi_env, "CompoundCurve");
}

/**
 * Points that make up the compound curve.
 *
 * @kind member
 * @name curves
 * @instance
 * @memberof CompoundCurve
 * @type {CompoundCurveCurves}
 */
NAN_GETTER(CompoundCurve::curvesGetter) {
  return Nan::GetPrivate(info.This(), Napi::String::New(node_gdal::napi_env, "curves_")).ToLocalChecked();
}

} // namespace node_gdal
