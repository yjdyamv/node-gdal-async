#ifndef __GDAL_WARPER_NAPI_H__
#define __GDAL_WARPER_NAPI_H__

#include <napi.h>
#include "gdal_common_napi.hpp"

namespace node_gdal {

class WarperNapi {
    public:
  static Napi::Object Init(Napi::Env env, Napi::Object exports);

  static Napi::Value reprojectImage(const Napi::CallbackInfo &info);
  static Napi::Value reprojectImageAsync(const Napi::CallbackInfo &info);
  static Napi::Value reprojectImage_do(const Napi::CallbackInfo &info, bool async);

  static Napi::Value suggestedWarpOutput(const Napi::CallbackInfo &info);
  static Napi::Value suggestedWarpOutputAsync(const Napi::CallbackInfo &info);
  static Napi::Value suggestedWarpOutput_do(const Napi::CallbackInfo &info, bool async);
};

} // namespace node_gdal

#endif
