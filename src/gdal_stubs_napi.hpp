#ifndef __NODE_GDAL_STUBS_NAPI_H__
#define __NODE_GDAL_STUBS_NAPI_H__

#include <napi.h>
#include <gdal_priv.h>
#include "gdal_common_napi.hpp"

namespace node_gdal {

// Remaining collection/utility stubs for module init registration chain
#define DECLARE_STUB_NAPI(klass)                     \
  class klass : public Napi::ObjectWrap<klass> {     \
    public:                                          \
    static Napi::FunctionReference constructor;      \
    static Napi::Object Init(Napi::Env e, Napi::Object o); \
    klass(const Napi::CallbackInfo &i) : Napi::ObjectWrap<klass>(i) {} \
  }

DECLARE_STUB_NAPI(RasterBandPixelsNapi);
DECLARE_STUB_NAPI(RasterBandOverviewsNapi);
DECLARE_STUB_NAPI(LayerFieldsNapi);
DECLARE_STUB_NAPI(FeatureFieldsNapi);
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
