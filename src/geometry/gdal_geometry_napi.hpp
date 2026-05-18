#ifndef __NODE_OGR_GEOMETRY_NAPI_H__
#define __NODE_OGR_GEOMETRY_NAPI_H__

#include <napi.h>
#include <ogrsf_frmts.h>

#include "../async_napi.hpp"
#include "gdal_geometrybase_napi.hpp"

namespace node_gdal {

class GeometryNapi : public Napi::ObjectWrap<GeometryNapi>,
                     public GeometryBaseNapi<GeometryNapi, OGRGeometry> {
    public:
  static Napi::FunctionReference constructor;
  using GeometryBaseNapi<GeometryNapi, OGRGeometry>::New;
  static Napi::Value New(Napi::Env env, OGRGeometry *geom, bool owned);

  static Napi::Object Init(Napi::Env env, Napi::Object exports);
  GeometryNapi(const Napi::CallbackInfo &info);

  Napi::Value toString(const Napi::CallbackInfo &info);

  // Async methods
  GDAL_ASYNCABLE_DECLARE_NAPI(isEmpty);
  GDAL_ASYNCABLE_DECLARE_NAPI(isValid);
  GDAL_ASYNCABLE_DECLARE_NAPI(isSimple);
  GDAL_ASYNCABLE_DECLARE_NAPI(isRing);
  Napi::Value clone(const Napi::CallbackInfo &info);
  GDAL_ASYNCABLE_DECLARE_NAPI(empty);
  GDAL_ASYNCABLE_DECLARE_NAPI(exportToKML);
  GDAL_ASYNCABLE_DECLARE_NAPI(exportToGML);
  GDAL_ASYNCABLE_DECLARE_NAPI(exportToJSON);
  GDAL_ASYNCABLE_DECLARE_NAPI(exportToWKT);
  GDAL_ASYNCABLE_DECLARE_NAPI(exportToWKB);
  GDAL_ASYNCABLE_DECLARE_NAPI(closeRings);
  GDAL_ASYNCABLE_DECLARE_NAPI(segmentize);
  GDAL_ASYNCABLE_DECLARE_NAPI(intersects);
  GDAL_ASYNCABLE_DECLARE_NAPI(equals);
  GDAL_ASYNCABLE_DECLARE_NAPI(disjoint);
  GDAL_ASYNCABLE_DECLARE_NAPI(touches);
  GDAL_ASYNCABLE_DECLARE_NAPI(crosses);
  GDAL_ASYNCABLE_DECLARE_NAPI(within);
  GDAL_ASYNCABLE_DECLARE_NAPI(contains);
  GDAL_ASYNCABLE_DECLARE_NAPI(overlaps);
  GDAL_ASYNCABLE_DECLARE_NAPI(boundary);
  GDAL_ASYNCABLE_DECLARE_NAPI(distance);
  GDAL_ASYNCABLE_DECLARE_NAPI(convexHull);
  GDAL_ASYNCABLE_DECLARE_NAPI(buffer);
  GDAL_ASYNCABLE_DECLARE_NAPI(intersection);
  GDAL_ASYNCABLE_DECLARE_NAPI(unionGeometry);
  GDAL_ASYNCABLE_DECLARE_NAPI(difference);
  GDAL_ASYNCABLE_DECLARE_NAPI(symDifference);
  GDAL_ASYNCABLE_DECLARE_NAPI(simplify);
  GDAL_ASYNCABLE_DECLARE_NAPI(simplifyPreserveTopology);
  GDAL_ASYNCABLE_DECLARE_NAPI(swapXY);
  GDAL_ASYNCABLE_DECLARE_NAPI(getEnvelope);
  GDAL_ASYNCABLE_DECLARE_NAPI(getEnvelope3D);
  GDAL_ASYNCABLE_DECLARE_NAPI(flattenTo2D);

  // Static async constructors
  GDAL_ASYNCABLE_DECLARE_NAPI(create);
  GDAL_ASYNCABLE_DECLARE_NAPI(createFromWkt);
  GDAL_ASYNCABLE_DECLARE_NAPI(createFromWkb);
  GDAL_ASYNCABLE_DECLARE_NAPI(createFromGeoJson);

  Napi::Value getName(const Napi::CallbackInfo &info);
  Napi::Value getConstructor(const Napi::CallbackInfo &info);

  // Getters
  Napi::Value srsGetter(const Napi::CallbackInfo &info);
  Napi::Value typeGetter(const Napi::CallbackInfo &info);
  Napi::Value nameGetter(const Napi::CallbackInfo &info);
  Napi::Value wkbSizeGetter(const Napi::CallbackInfo &info);
  Napi::Value dimensionGetter(const Napi::CallbackInfo &info);
  Napi::Value coordinateDimensionGetter(const Napi::CallbackInfo &info);

  // Setters
  void srsSetter(const Napi::CallbackInfo &info, const Napi::Value &value);
  void coordinateDimensionSetter(const Napi::CallbackInfo &info, const Napi::Value &value);
};

} // namespace node_gdal

#endif
