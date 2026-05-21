#ifndef __NODE_OGR_POINT_NAPI_H__
#define __NODE_OGR_POINT_NAPI_H__

#include "gdal_geometrybase_napi.hpp"

namespace node_gdal {

class PointNapi : public Napi::ObjectWrap<PointNapi>, public GeometryBaseNapi<PointNapi, OGRPoint> {
    public:
  static Napi::FunctionReference constructor;
  using GeometryBaseNapi<PointNapi, OGRPoint>::New;

  static Napi::Object Init(Napi::Env env, Napi::Object exports);

  PointNapi(const Napi::CallbackInfo &info);

  Napi::Value toString(const Napi::CallbackInfo &info);
  Napi::Value toJSON(const Napi::CallbackInfo &info);
  Napi::Value xGetter(const Napi::CallbackInfo &info);
  Napi::Value yGetter(const Napi::CallbackInfo &info);
  Napi::Value zGetter(const Napi::CallbackInfo &info);
  void xSetter(const Napi::CallbackInfo &info, const Napi::Value &value);
  void ySetter(const Napi::CallbackInfo &info, const Napi::Value &value);
  void zSetter(const Napi::CallbackInfo &info, const Napi::Value &value);
};

} // namespace node_gdal

#endif
