#include "gdal_stubs_napi.hpp"

namespace node_gdal {

// RasterBandPixelsNapi
Napi::FunctionReference RasterBandPixelsNapi::constructor;
Napi::Object RasterBandPixelsNapi::Init(Napi::Env env, Napi::Object exports) {
  Napi::Function f = DefineClass(env, "RasterBandPixelsNapi", {});
  constructor = Napi::Persistent(f); constructor.SuppressDestruct();
  exports.Set("RasterBandPixelsNapi", f); return exports;
}

#define STUB_TOSTRING(klass) \
  Napi::FunctionReference klass::constructor; \
  Napi::Object klass::Init(Napi::Env env, Napi::Object exports) { \
    Napi::Function f = DefineClass(env, #klass, {}); \
    constructor = Napi::Persistent(f); constructor.SuppressDestruct(); \
    exports.Set(#klass, f); return exports; \
  }

STUB_TOSTRING(RasterBandOverviewsNapi)
STUB_TOSTRING(LayerFieldsNapi)
STUB_TOSTRING(FeatureFieldsNapi)
STUB_TOSTRING(FeatureDefnFieldsNapi)
STUB_TOSTRING(GeometryCollectionChildrenNapi)
STUB_TOSTRING(PolygonRingsNapi)
STUB_TOSTRING(LineStringPointsNapi)
STUB_TOSTRING(CompoundCurveCurvesNapi)
STUB_TOSTRING(GroupGroupsNapi)
STUB_TOSTRING(GroupArraysNapi)
STUB_TOSTRING(GroupAttributesNapi)
STUB_TOSTRING(GroupDimensionsNapi)
STUB_TOSTRING(ArrayDimensionsNapi)
STUB_TOSTRING(ArrayAttributesNapi)

namespace VsiNapi {
Napi::Object Init(Napi::Env env, Napi::Object exports) {
  exports.Set("VsiNapi", Napi::Object::New(env));
  return exports;
}
}

} // namespace node_gdal
