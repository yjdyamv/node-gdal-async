#ifndef __NODE_GDAL_POLYGON_RINGS_H__
#define __NODE_GDAL_POLYGON_RINGS_H__

// node

// nan
#include "../gdal_common.hpp"

// gdal
#include <gdal_priv.h>

// Polygon.rings

namespace node_gdal {

class PolygonRings : public GDALObject<PolygonRings> {
    public:
  static Napi::FunctionReference constructor;

  static void Initialize(Napi::Object target);
  static Napi::Value New(Napi::Value geom);
  static NAN_METHOD(toString);

  static NAN_METHOD(get);
  static NAN_METHOD(count);
  static NAN_METHOD(add);
  static NAN_METHOD(remove);

  PolygonRings(const Napi::CallbackInfo &info);

  // Must be accessible: ObjectWrap's finalizer deletes the instance itself
  ~PolygonRings();
    private:
};

} // namespace node_gdal
#endif
