#include "gdal_utils_napi.hpp"

namespace node_gdal {

namespace WarperNapi {
Napi::Object Init(Napi::Env env, Napi::Object exports) {
  auto obj = Napi::Object::New(env);
  // TODO: port reprojectImage, suggestedWarpOutput
  exports.Set("WarperNapi", obj);
  return exports;
}
}

namespace AlgorithmsNapi {
Napi::Object Init(Napi::Env env, Napi::Object exports) {
  auto obj = Napi::Object::New(env);
  // TODO: port fillNodata, contourGenerate, sieveFilter, checksumImage, polygonize
  exports.Set("AlgorithmsNapi", obj);
  return exports;
}
}

namespace UtilsNapi {
Napi::Object Init(Napi::Env env, Napi::Object exports) {
  auto obj = Napi::Object::New(env);
  // TODO: port info, translate, warp, buildVRT, rasterize, dem
  exports.Set("UtilsNapi", obj);
  return exports;
}
}

} // namespace node_gdal
