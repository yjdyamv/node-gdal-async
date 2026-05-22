#ifndef __NODE_OGR_POLY_NAPI_H__
#define __NODE_OGR_POLY_NAPI_H__

#include "gdal_curvebase_napi.hpp"

namespace node_gdal {

class PolygonNapi
  : public Napi::ObjectWrap<PolygonNapi>,
    public CurveBaseNapi<PolygonNapi, OGRPolygon, void> {
    public:
  static Napi::FunctionReference constructor;
  using GeometryBaseNapi<PolygonNapi, OGRPolygon>::New;

  static Napi::Object Init(Napi::Env env, Napi::Object exports);
  PolygonNapi(const Napi::CallbackInfo &info);
  Napi::Value toString(const Napi::CallbackInfo &info);
  Napi::Value getArea(const Napi::CallbackInfo &info);
  Napi::Value ringsGetter(const Napi::CallbackInfo &info);
};

} // namespace node_gdal

#endif
