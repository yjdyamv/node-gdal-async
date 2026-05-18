#ifndef __NODE_OGR_GEOMETRYCOLLECTION_NAPI_H__
#define __NODE_OGR_GEOMETRYCOLLECTION_NAPI_H__

#include "gdal_geometrybase_napi.hpp"

namespace node_gdal {

class GeometryCollectionNapi
  : public Napi::ObjectWrap<GeometryCollectionNapi>,
    public GeometryBaseNapi<GeometryCollectionNapi, OGRGeometryCollection> {
    public:
  static Napi::FunctionReference constructor;
  using GeometryBaseNapi<GeometryCollectionNapi, OGRGeometryCollection>::New;

  static Napi::Object Init(Napi::Env env, Napi::Object exports);
  GeometryCollectionNapi(const Napi::CallbackInfo &info);
  Napi::Value toString(const Napi::CallbackInfo &info);
  Napi::Value getArea(const Napi::CallbackInfo &info);
  Napi::Value getLength(const Napi::CallbackInfo &info);
};

class MultiPointNapi
  : public Napi::ObjectWrap<MultiPointNapi>,
    public GeometryBaseNapi<MultiPointNapi, OGRMultiPoint> {
    public:
  static Napi::FunctionReference constructor;
  using GeometryBaseNapi<MultiPointNapi, OGRMultiPoint>::New;

  static Napi::Object Init(Napi::Env env, Napi::Object exports);
  MultiPointNapi(const Napi::CallbackInfo &info);
  Napi::Value toString(const Napi::CallbackInfo &info);
};

class MultiLineStringNapi
  : public Napi::ObjectWrap<MultiLineStringNapi>,
    public GeometryBaseNapi<MultiLineStringNapi, OGRMultiLineString> {
    public:
  static Napi::FunctionReference constructor;
  using GeometryBaseNapi<MultiLineStringNapi, OGRMultiLineString>::New;

  static Napi::Object Init(Napi::Env env, Napi::Object exports);
  MultiLineStringNapi(const Napi::CallbackInfo &info);
  Napi::Value toString(const Napi::CallbackInfo &info);
};

class MultiPolygonNapi
  : public Napi::ObjectWrap<MultiPolygonNapi>,
    public GeometryBaseNapi<MultiPolygonNapi, OGRMultiPolygon> {
    public:
  static Napi::FunctionReference constructor;
  using GeometryBaseNapi<MultiPolygonNapi, OGRMultiPolygon>::New;

  static Napi::Object Init(Napi::Env env, Napi::Object exports);
  MultiPolygonNapi(const Napi::CallbackInfo &info);
  Napi::Value toString(const Napi::CallbackInfo &info);
};

class MultiCurveNapi
  : public Napi::ObjectWrap<MultiCurveNapi>,
    public GeometryBaseNapi<MultiCurveNapi, OGRMultiCurve> {
    public:
  static Napi::FunctionReference constructor;
  using GeometryBaseNapi<MultiCurveNapi, OGRMultiCurve>::New;

  static Napi::Object Init(Napi::Env env, Napi::Object exports);
  MultiCurveNapi(const Napi::CallbackInfo &info);
  Napi::Value toString(const Napi::CallbackInfo &info);
};

} // namespace node_gdal

#endif
