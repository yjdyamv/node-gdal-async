#ifndef __NODE_OGR_COORDINATETRANSFORMATION_NAPI_H__
#define __NODE_OGR_COORDINATETRANSFORMATION_NAPI_H__

#include <napi.h>
#include <ogrsf_frmts.h>
#include <gdalwarper.h>

#include "gdal_common_napi.hpp"
// GeoTransformTransformer is defined in the nan header
#include "gdal_coordinate_transformation.hpp"

namespace node_gdal {

class CoordinateTransformationNapi : public Napi::ObjectWrap<CoordinateTransformationNapi> {
    public:
  static Napi::FunctionReference constructor;
  static Napi::Object Init(Napi::Env env, Napi::Object exports);
  static Napi::Value New(Napi::Env env, OGRCoordinateTransformation *transform);

  CoordinateTransformationNapi(const Napi::CallbackInfo &info);
  ~CoordinateTransformationNapi();

  Napi::Value toString(const Napi::CallbackInfo &info);
  Napi::Value transformPoint(const Napi::CallbackInfo &info);

  OGRCoordinateTransformation *get() {
    return this_;
  }
  bool isAlive() {
    return this_ != nullptr;
  }

    private:
  OGRCoordinateTransformation *this_;
};

} // namespace node_gdal

#endif
