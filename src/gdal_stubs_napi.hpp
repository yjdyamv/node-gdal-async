#ifndef __NODE_GDAL_STUBS_NAPI_H__
#define __NODE_GDAL_STUBS_NAPI_H__

#include <napi.h>
#include <gdal_priv.h>
#include <ogrsf_frmts.h>
#include "gdal_common_napi.hpp"
#include "async_napi.hpp"

namespace node_gdal {

// ---- RasterBandPixelsNapi ----
class RasterBandPixelsNapi : public Napi::ObjectWrap<RasterBandPixelsNapi> {
    public:
  static Napi::FunctionReference constructor;
  static Napi::Object Init(Napi::Env e, Napi::Object o);
  RasterBandPixelsNapi(const Napi::CallbackInfo &i);
  Napi::Value getPixel(const Napi::CallbackInfo &i);
  Napi::Value setPixel(const Napi::CallbackInfo &i);
  GDAL_ASYNCABLE_DECLARE_NAPI(read);
  GDAL_ASYNCABLE_DECLARE_NAPI(write);
    private:
  GDALRasterBand *band_;
};

// ---- FeatureFieldsNapi ----
class FeatureFieldsNapi : public Napi::ObjectWrap<FeatureFieldsNapi> {
    public:
  static Napi::FunctionReference constructor;
  static Napi::Object Init(Napi::Env e, Napi::Object o);
  FeatureFieldsNapi(const Napi::CallbackInfo &i);
  Napi::Value get(const Napi::CallbackInfo &i);
  Napi::Value set(const Napi::CallbackInfo &i);
  Napi::Value count(const Napi::CallbackInfo &i);
  Napi::Value getNames(const Napi::CallbackInfo &i);
    private:
  OGRFeature *feat_;
};

// ---- LayerFieldsNapi ----
class LayerFieldsNapi : public Napi::ObjectWrap<LayerFieldsNapi> {
    public:
  static Napi::FunctionReference constructor;
  static Napi::Object Init(Napi::Env e, Napi::Object o);
  LayerFieldsNapi(const Napi::CallbackInfo &i);
  Napi::Value get(const Napi::CallbackInfo &i);
  Napi::Value count(const Napi::CallbackInfo &i);
  Napi::Value getNames(const Napi::CallbackInfo &i);
    private:
  OGRLayer *layer_;
};

// ---- Remaining lightweight stubs ----
#define DECLARE_STUB_NAPI(klass)                     \
  class klass : public Napi::ObjectWrap<klass> {     \
    public:                                          \
    static Napi::FunctionReference constructor;      \
    static Napi::Object Init(Napi::Env e, Napi::Object o); \
    klass(const Napi::CallbackInfo &i) : Napi::ObjectWrap<klass>(i) {} \
  }

DECLARE_STUB_NAPI(RasterBandOverviewsNapi);
DECLARE_STUB_NAPI(FeatureDefnFieldsNapi);
DECLARE_STUB_NAPI(GeometryCollectionChildrenNapi);
DECLARE_STUB_NAPI(PolygonRingsNapi);
DECLARE_STUB_NAPI(LineStringPointsNapi);
DECLARE_STUB_NAPI(CompoundCurveCurvesNapi);
DECLARE_STUB_NAPI(GroupGroupsNapi);
DECLARE_STUB_NAPI(GroupArraysNapi);
DECLARE_STUB_NAPI(GroupAttributesNapi);
DECLARE_STUB_NAPI(GroupDimensionsNapi);
DECLARE_STUB_NAPI(ArrayDimensionsNapi);
DECLARE_STUB_NAPI(ArrayAttributesNapi);

namespace VsiNapi { Napi::Object Init(Napi::Env, Napi::Object); }

} // namespace node_gdal
#endif
