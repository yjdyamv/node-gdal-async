#ifndef __NODE_OGR_COMPOUNDCURVE_NAPI_H__
#define __NODE_OGR_COMPOUNDCURVE_NAPI_H__

#include "gdal_curvebase_napi.hpp"

namespace node_gdal {

class CompoundCurveNapi
  : public Napi::ObjectWrap<CompoundCurveNapi>,
    public CurveBaseNapi<CompoundCurveNapi, OGRCompoundCurve, void> {
    public:
  static Napi::FunctionReference constructor;
  using GeometryBaseNapi<CompoundCurveNapi, OGRCompoundCurve>::New;

  static Napi::Object Init(Napi::Env env, Napi::Object exports);
  CompoundCurveNapi(const Napi::CallbackInfo &info);
  Napi::Value toString(const Napi::CallbackInfo &info);
  Napi::Value pointsGetter(const Napi::CallbackInfo &info);
  Napi::Value curvesGetter(const Napi::CallbackInfo &info);
};

} // namespace node_gdal

#endif
