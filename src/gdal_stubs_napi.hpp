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

// Multi-dimensional collection stubs with real get/count/getNames
class GroupGroupsNapi : public Napi::ObjectWrap<GroupGroupsNapi> {
    public:
  static Napi::FunctionReference constructor;
  static Napi::Object Init(Napi::Env e, Napi::Object o);
  GroupGroupsNapi(const Napi::CallbackInfo &i);
  GDAL_ASYNCABLE_DECLARE_NAPI(get);
  GDAL_ASYNCABLE_DECLARE_NAPI(count);
  Napi::Value getNames(const Napi::CallbackInfo &i);
    private: GDALGroup *group_;
};
class GroupArraysNapi : public Napi::ObjectWrap<GroupArraysNapi> {
    public:
  static Napi::FunctionReference constructor;
  static Napi::Object Init(Napi::Env e, Napi::Object o);
  GroupArraysNapi(const Napi::CallbackInfo &i);
  GDAL_ASYNCABLE_DECLARE_NAPI(get);
  GDAL_ASYNCABLE_DECLARE_NAPI(count);
  Napi::Value getNames(const Napi::CallbackInfo &i);
    private: GDALGroup *group_;
};
class GroupAttributesNapi : public Napi::ObjectWrap<GroupAttributesNapi> {
    public:
  static Napi::FunctionReference constructor;
  static Napi::Object Init(Napi::Env e, Napi::Object o);
  GroupAttributesNapi(const Napi::CallbackInfo &i);
  GDAL_ASYNCABLE_DECLARE_NAPI(get);
  GDAL_ASYNCABLE_DECLARE_NAPI(count);
  Napi::Value getNames(const Napi::CallbackInfo &i);
    private: GDALGroup *group_;
};
class GroupDimensionsNapi : public Napi::ObjectWrap<GroupDimensionsNapi> {
    public:
  static Napi::FunctionReference constructor;
  static Napi::Object Init(Napi::Env e, Napi::Object o);
  GroupDimensionsNapi(const Napi::CallbackInfo &i);
  GDAL_ASYNCABLE_DECLARE_NAPI(get);
  GDAL_ASYNCABLE_DECLARE_NAPI(count);
  Napi::Value getNames(const Napi::CallbackInfo &i);
    private: GDALGroup *group_;
};
class ArrayDimensionsNapi : public Napi::ObjectWrap<ArrayDimensionsNapi> {
    public:
  static Napi::FunctionReference constructor;
  static Napi::Object Init(Napi::Env e, Napi::Object o);
  ArrayDimensionsNapi(const Napi::CallbackInfo &i);
  GDAL_ASYNCABLE_DECLARE_NAPI(get);
  GDAL_ASYNCABLE_DECLARE_NAPI(count);
  Napi::Value getNames(const Napi::CallbackInfo &i);
    private: GDALMDArray *array_;
};
class ArrayAttributesNapi : public Napi::ObjectWrap<ArrayAttributesNapi> {
    public:
  static Napi::FunctionReference constructor;
  static Napi::Object Init(Napi::Env e, Napi::Object o);
  ArrayAttributesNapi(const Napi::CallbackInfo &i);
  GDAL_ASYNCABLE_DECLARE_NAPI(get);
  GDAL_ASYNCABLE_DECLARE_NAPI(count);
  Napi::Value getNames(const Napi::CallbackInfo &i);
    private: GDALMDArray *array_;
};

} // namespace node_gdal
#endif
