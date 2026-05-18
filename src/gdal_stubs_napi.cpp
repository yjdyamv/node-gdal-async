#include "gdal_stubs_napi.hpp"

namespace node_gdal {

#define IMPL_STUB(klass) \
  Napi::FunctionReference klass::constructor; \
  Napi::Object klass::Init(Napi::Env env, Napi::Object exports) { \
    Napi::Function f = DefineClass(env, #klass, {}); \
    constructor = Napi::Persistent(f); constructor.SuppressDestruct(); \
    exports.Set(#klass, f); return exports; \
  }

IMPL_STUB(RasterBandPixelsNapi)
IMPL_STUB(RasterBandOverviewsNapi)
IMPL_STUB(LayerFieldsNapi)
IMPL_STUB(FeatureFieldsNapi)
IMPL_STUB(FeatureDefnFieldsNapi)
IMPL_STUB(GeometryCollectionChildrenNapi)
IMPL_STUB(PolygonRingsNapi)
IMPL_STUB(LineStringPointsNapi)
IMPL_STUB(CompoundCurveCurvesNapi)
IMPL_STUB(GroupGroupsNapi)
IMPL_STUB(GroupArraysNapi)
IMPL_STUB(GroupAttributesNapi)
IMPL_STUB(GroupDimensionsNapi)
IMPL_STUB(ArrayDimensionsNapi)
IMPL_STUB(ArrayAttributesNapi)

namespace VsiNapi {
Napi::Object Init(Napi::Env env, Napi::Object exports) {
  exports.Set("VsiNapi", Napi::Object::New(env));
  return exports;
}
}

} // namespace node_gdal
