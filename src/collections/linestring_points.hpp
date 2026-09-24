#ifndef __NODE_GDAL_LINESTRING_POINTS_H__
#define __NODE_GDAL_LINESTRING_POINTS_H__

// node

// nan
#include "../gdal_common.hpp"

// gdal
#include <gdal_priv.h>

// LineString.children

namespace node_gdal {

class LineStringPoints : public GDALObject<LineStringPoints> {
    public:
  static Napi::FunctionReference constructor;

  static void Initialize(Napi::Object target);
  static Napi::Value New(Napi::Value geom);
  static NAN_METHOD(toString);

  static NAN_METHOD(add);
  static NAN_METHOD(get);
  static NAN_METHOD(set);
  static NAN_METHOD(count);
  static NAN_METHOD(reverse);
  static NAN_METHOD(resize);

  LineStringPoints(const Napi::CallbackInfo &info);

  // Must be accessible: ObjectWrap's finalizer deletes the instance itself
  ~LineStringPoints();
    private:
};

} // namespace node_gdal
#endif
