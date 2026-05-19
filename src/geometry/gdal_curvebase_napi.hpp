#ifndef __NODE_OGR_CURVEBASE_NAPI_H__
#define __NODE_OGR_CURVEBASE_NAPI_H__

#include "gdal_geometrybase_napi.hpp"

namespace node_gdal {

// CRTP base for curve-type geometries (SimpleCurve, LineString, etc.)
// COLLECTIONT is the nan collection type (unused in N-API yet)
template <class T, class OGRT, class COLLECTIONT>
class CurveBaseNapi : public GeometryBaseNapi<T, OGRT> {
    protected:
  using GeometryBaseNapi<T, OGRT>::GeometryBaseNapi;
};

} // namespace node_gdal

#endif
