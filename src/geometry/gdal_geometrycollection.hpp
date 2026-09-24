#ifndef __NODE_OGR_GEOMETRYCOLLECTION_H__
#define __NODE_OGR_GEOMETRYCOLLECTION_H__

// node

// nan
#include "../gdal_common.hpp"

// ogr
#include <ogrsf_frmts.h>


#include "gdal_geometrybase.hpp"
#include "gdal_geometrycollectionbase.hpp"
#include "../collections/geometry_collection_children.hpp"

namespace node_gdal {

class GeometryCollection : public GeometryCollectionBase<GeometryCollection, OGRGeometryCollection> {

    public:
  static Napi::FunctionReference constructor;
  using GeometryCollectionBase<GeometryCollection, OGRGeometryCollection>::GeometryCollectionBase;

  static void Initialize(Napi::Object target);
  using GeometryCollectionBase<GeometryCollection, OGRGeometryCollection>::New;
  static NAN_METHOD(toString);
  static NAN_METHOD(getArea);
  static NAN_METHOD(getLength);

  static NAN_GETTER(childrenGetter);
};

} // namespace node_gdal
#endif
