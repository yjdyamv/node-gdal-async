#ifndef __NODE_GDAL_STUBS_NAPI_H__
#define __NODE_GDAL_STUBS_NAPI_H__

#include <napi.h>
#include <gdal_priv.h>
#include <ogrsf_frmts.h>
#include "gdal_common_napi.hpp"
#include "async_napi.hpp"

namespace node_gdal {

class RasterBandPixelsNapi : public Napi::ObjectWrap<RasterBandPixelsNapi> {
    public:
  static Napi::FunctionReference constructor;
  static Napi::Object Init(Napi::Env e, Napi::Object o);
  RasterBandPixelsNapi(const Napi::CallbackInfo &i);
  Napi::Value getPixel(const Napi::CallbackInfo &i);
  Napi::Value setPixel(const Napi::CallbackInfo &i);
  GDAL_ASYNCABLE_DECLARE_NAPI(read);
  GDAL_ASYNCABLE_DECLARE_NAPI(write);
    private: GDALRasterBand *band_;
};

class FeatureFieldsNapi : public Napi::ObjectWrap<FeatureFieldsNapi> {
    public:
  static Napi::FunctionReference constructor;
  static Napi::Object Init(Napi::Env e, Napi::Object o);
  FeatureFieldsNapi(const Napi::CallbackInfo &i);
  Napi::Value get(const Napi::CallbackInfo &i);
  Napi::Value set(const Napi::CallbackInfo &i);
  Napi::Value count(const Napi::CallbackInfo &i);
  Napi::Value getNames(const Napi::CallbackInfo &i);
    private: OGRFeature *feat_;
};

class LayerFieldsNapi : public Napi::ObjectWrap<LayerFieldsNapi> {
    public:
  static Napi::FunctionReference constructor;
  static Napi::Object Init(Napi::Env e, Napi::Object o);
  LayerFieldsNapi(const Napi::CallbackInfo &i);
  Napi::Value get(const Napi::CallbackInfo &i);
  Napi::Value count(const Napi::CallbackInfo &i);
  Napi::Value getNames(const Napi::CallbackInfo &i);
    private: OGRLayer *layer_;
};

class FeatureDefnFieldsNapi : public Napi::ObjectWrap<FeatureDefnFieldsNapi> {
    public:
  static Napi::FunctionReference constructor;
  static Napi::Object Init(Napi::Env e, Napi::Object o);
  FeatureDefnFieldsNapi(const Napi::CallbackInfo &i);
  Napi::Value get(const Napi::CallbackInfo &i);
  Napi::Value count(const Napi::CallbackInfo &i);
  Napi::Value getNames(const Napi::CallbackInfo &i);
    private: OGRFeatureDefn *defn_;
};

class RasterBandOverviewsNapi : public Napi::ObjectWrap<RasterBandOverviewsNapi> {
    public:
  static Napi::FunctionReference constructor;
  static Napi::Object Init(Napi::Env e, Napi::Object o);
  RasterBandOverviewsNapi(const Napi::CallbackInfo &i);
  GDAL_ASYNCABLE_DECLARE_NAPI(get);
  GDAL_ASYNCABLE_DECLARE_NAPI(count);
    private: GDALRasterBand *band_;
};

class GeometryCollectionChildrenNapi : public Napi::ObjectWrap<GeometryCollectionChildrenNapi> {
    public:
  static Napi::FunctionReference constructor;
  static Napi::Object Init(Napi::Env e, Napi::Object o);
  GeometryCollectionChildrenNapi(const Napi::CallbackInfo &i);
  Napi::Value get(const Napi::CallbackInfo &i);
  Napi::Value count(const Napi::CallbackInfo &i);
  Napi::Value add(const Napi::CallbackInfo &i);
    private: OGRGeometryCollection *geom_;
};

class PolygonRingsNapi : public Napi::ObjectWrap<PolygonRingsNapi> {
    public:
  static Napi::FunctionReference constructor;
  static Napi::Object Init(Napi::Env e, Napi::Object o);
  PolygonRingsNapi(const Napi::CallbackInfo &i);
  Napi::Value get(const Napi::CallbackInfo &i);
  Napi::Value count(const Napi::CallbackInfo &i);
  Napi::Value add(const Napi::CallbackInfo &i);
    private: OGRPolygon *geom_;
};

class LineStringPointsNapi : public Napi::ObjectWrap<LineStringPointsNapi> {
    public:
  static Napi::FunctionReference constructor;
  static Napi::Object Init(Napi::Env e, Napi::Object o);
  LineStringPointsNapi(const Napi::CallbackInfo &i);
  Napi::Value get(const Napi::CallbackInfo &i);
  Napi::Value count(const Napi::CallbackInfo &i);
  Napi::Value add(const Napi::CallbackInfo &i);
  Napi::Value setPi(const Napi::CallbackInfo &i);
    private: OGRLineString *geom_;
};

class CompoundCurveCurvesNapi : public Napi::ObjectWrap<CompoundCurveCurvesNapi> {
    public:
  static Napi::FunctionReference constructor;
  static Napi::Object Init(Napi::Env e, Napi::Object o);
  CompoundCurveCurvesNapi(const Napi::CallbackInfo &i);
  Napi::Value get(const Napi::CallbackInfo &i);
  Napi::Value count(const Napi::CallbackInfo &i);
    private: OGRCompoundCurve *geom_;
};

// Multi-dimensional collection stubs with toString
#define DECLARE_STUB_NAPI(klass) \
  class klass : public Napi::ObjectWrap<klass> { \
    public: \
    static Napi::FunctionReference constructor; \
    static Napi::Object Init(Napi::Env e, Napi::Object o); \
    klass(const Napi::CallbackInfo &i) : Napi::ObjectWrap<klass>(i) {} \
  }

DECLARE_STUB_NAPI(GroupGroupsNapi);
DECLARE_STUB_NAPI(GroupArraysNapi);
DECLARE_STUB_NAPI(GroupAttributesNapi);
DECLARE_STUB_NAPI(GroupDimensionsNapi);
DECLARE_STUB_NAPI(ArrayDimensionsNapi);
DECLARE_STUB_NAPI(ArrayAttributesNapi);

namespace VsiNapi { Napi::Object Init(Napi::Env, Napi::Object); }
} // namespace node_gdal
#endif
