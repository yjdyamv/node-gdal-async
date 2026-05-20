#include "gdal_algebra_napi.hpp"

namespace node_gdal {
namespace AlgebraNapi {

Napi::Object Init(Napi::Env env, Napi::Object exports) {
  Napi::Object a = Napi::Object::New(env);
  // GDAL >= 3.12 algebra uses GDALComputedRasterBand internal API
  // which differs between GDAL builds. N-API port deferred.
  if (!exports.Has("algebra")) exports.Set("algebra", a);
  return exports;
}

} // namespace AlgebraNapi
} // namespace node_gdal
