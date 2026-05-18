#ifndef __NODE_OGR_LINESTRING_NAPI_H__
#define __NODE_OGR_LINESTRING_NAPI_H__

#include "gdal_curvebase_napi.hpp"

namespace node_gdal {

class LineStringNapi
  : public Napi::ObjectWrap<LineStringNapi>,
    public CurveBaseNapi<LineStringNapi, OGRLineString, void> {
    public:
  static Napi::FunctionReference constructor;
  using GeometryBaseNapi<LineStringNapi, OGRLineString>::New;

  static Napi::Object Init(Napi::Env env, Napi::Object exports);
  LineStringNapi(const Napi::CallbackInfo &info);
  Napi::Value toString(const Napi::CallbackInfo &info);
};

} // namespace node_gdal

#endif
