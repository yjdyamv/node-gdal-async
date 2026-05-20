#ifndef __GDAL_ALGORITHMS_NAPI_H__
#define __GDAL_ALGORITHMS_NAPI_H__

#include <napi.h>
#include "gdal_common_napi.hpp"

namespace node_gdal {

class AlgorithmsNapi {
    public:
  static Napi::Object Init(Napi::Env env, Napi::Object exports);

  static Napi::Value fillNodata(const Napi::CallbackInfo &info);
  static Napi::Value fillNodataAsync(const Napi::CallbackInfo &info);
  static Napi::Value fillNodata_do(const Napi::CallbackInfo &info, bool async);

  static Napi::Value contourGenerate(const Napi::CallbackInfo &info);
  static Napi::Value contourGenerateAsync(const Napi::CallbackInfo &info);
  static Napi::Value contourGenerate_do(const Napi::CallbackInfo &info, bool async);

  static Napi::Value sieveFilter(const Napi::CallbackInfo &info);
  static Napi::Value sieveFilterAsync(const Napi::CallbackInfo &info);
  static Napi::Value sieveFilter_do(const Napi::CallbackInfo &info, bool async);

  static Napi::Value checksumImage(const Napi::CallbackInfo &info);
  static Napi::Value checksumImageAsync(const Napi::CallbackInfo &info);
  static Napi::Value checksumImage_do(const Napi::CallbackInfo &info, bool async);

  static Napi::Value polygonize(const Napi::CallbackInfo &info);
  static Napi::Value polygonizeAsync(const Napi::CallbackInfo &info);
  static Napi::Value polygonize_do(const Napi::CallbackInfo &info, bool async);
};

} // namespace node_gdal

#endif
