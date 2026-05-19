#include "gdal_algebra_napi.hpp"

namespace node_gdal {
namespace AlgebraNapi {

Napi::Object Init(Napi::Env env, Napi::Object exports) {
  Napi::Object a = Napi::Object::New(env);
  // TODO: full algebra implementation (gdal::add, sub, abs, sqrt, etc.)
  // The GDAL algebra functions (gdal::add, etc.) require GDAL >= 3.12
  // with specific GDALComputedRasterBand support.
  // Only register if not already set by nan version
  if (!exports.Has("algebra")) exports.Set("algebra", a);
  return exports;
}

} // AlgebraNapi
} // node_gdal
