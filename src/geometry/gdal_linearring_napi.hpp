#ifndef __NODE_OGR_LINEARRING_NAPI_H__
#define __NODE_OGR_LINEARRING_NAPI_H__

#include "gdal_curvebase_napi.hpp"

namespace node_gdal {

class LinearRingNapi
  : public Napi::ObjectWrap<LinearRingNapi>,
    public CurveBaseNapi<LinearRingNapi, OGRLinearRing, void> {
    public:
  static Napi::FunctionReference constructor;
  using GeometryBaseNapi<LinearRingNapi, OGRLinearRing>::New;

  static Napi::Object Init(Napi::Env env, Napi::Object exports);
  LinearRingNapi(const Napi::CallbackInfo &info);
  Napi::Value toString(const Napi::CallbackInfo &info);
  Napi::Value getArea(const Napi::CallbackInfo &info);
};

} // namespace node_gdal

#endif
