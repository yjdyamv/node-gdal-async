#ifndef __NODE_OGR_CIRCULARSTRING_NAPI_H__
#define __NODE_OGR_CIRCULARSTRING_NAPI_H__

#include "gdal_curvebase_napi.hpp"

namespace node_gdal {

class CircularStringNapi
  : public Napi::ObjectWrap<CircularStringNapi>,
    public CurveBaseNapi<CircularStringNapi, OGRCircularString, void> {
    public:
  static Napi::FunctionReference constructor;
  using GeometryBaseNapi<CircularStringNapi, OGRCircularString>::New;

  static Napi::Object Init(Napi::Env env, Napi::Object exports);
  CircularStringNapi(const Napi::CallbackInfo &info);
  Napi::Value toString(const Napi::CallbackInfo &info);
  Napi::Value pointsGetter(const Napi::CallbackInfo &info);
};

} // namespace node_gdal

#endif
