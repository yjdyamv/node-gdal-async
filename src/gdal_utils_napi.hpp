#ifndef __GDAL_UTILS_NAPI_H__
#define __GDAL_UTILS_NAPI_H__

#include <napi.h>
#include <gdal_priv.h>
#include "gdal_common_napi.hpp"
#include "async_napi.hpp"

namespace node_gdal {

class UtilsNapi {
    public:
  static Napi::Object Init(Napi::Env env, Napi::Object exports);

  static Napi::Value gdalInfo(const Napi::CallbackInfo &info);
  static Napi::Value gdalInfoAsync(const Napi::CallbackInfo &info);
  static Napi::Value gdalInfo_do(const Napi::CallbackInfo &info, bool async);

  static Napi::Value translate(const Napi::CallbackInfo &info);
  static Napi::Value translateAsync(const Napi::CallbackInfo &info);
  static Napi::Value translate_do(const Napi::CallbackInfo &info, bool async);

  static Napi::Value vectorTranslate(const Napi::CallbackInfo &info);
  static Napi::Value vectorTranslateAsync(const Napi::CallbackInfo &info);
  static Napi::Value vectorTranslate_do(const Napi::CallbackInfo &info, bool async);

  static Napi::Value warp(const Napi::CallbackInfo &info);
  static Napi::Value warpAsync(const Napi::CallbackInfo &info);
  static Napi::Value warp_do(const Napi::CallbackInfo &info, bool async);

  static Napi::Value buildVRT(const Napi::CallbackInfo &info);
  static Napi::Value buildVRTAsync(const Napi::CallbackInfo &info);
  static Napi::Value buildVRT_do(const Napi::CallbackInfo &info, bool async);

  static Napi::Value rasterize(const Napi::CallbackInfo &info);
  static Napi::Value rasterizeAsync(const Napi::CallbackInfo &info);
  static Napi::Value rasterize_do(const Napi::CallbackInfo &info, bool async);

  static Napi::Value dem(const Napi::CallbackInfo &info);
  static Napi::Value demAsync(const Napi::CallbackInfo &info);
  static Napi::Value dem_do(const Napi::CallbackInfo &info, bool async);

  static Napi::Value reprojectImage(const Napi::CallbackInfo &info);
  static Napi::Value suggestedWarpOutput(const Napi::CallbackInfo &info);
};

namespace WarperNapi {
  Napi::Object Init(Napi::Env env, Napi::Object exports);
}

} // namespace node_gdal

#endif
