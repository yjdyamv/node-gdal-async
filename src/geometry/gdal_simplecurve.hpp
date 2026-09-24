#ifndef __NODE_OGR_CURVE_H__
#define __NODE_OGR_CURVE_H__

// node

// nan
#include "../gdal_common.hpp"

// ogr
#include <ogrsf_frmts.h>

#include "gdal_curvebase.hpp"
#include "../collections/linestring_points.hpp"


namespace node_gdal {

class SimpleCurve : public CurveBase<SimpleCurve, OGRSimpleCurve, LineStringPoints> {

    public:
  static Napi::FunctionReference constructor;
  // Abstract in JS
  SimpleCurve(const Napi::CallbackInfo &info);

  static void Initialize(Napi::Object target);
  static NAN_METHOD(toString);
  static NAN_METHOD(value);
  static NAN_METHOD(getLength);
  static NAN_METHOD(addSubLineString);

  static NAN_GETTER(pointsGetter);
};

} // namespace node_gdal

#endif
