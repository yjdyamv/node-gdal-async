#ifndef __NODE_GDAL_DRIVERS_NAPI_H__
#define __NODE_GDAL_DRIVERS_NAPI_H__

#include <napi.h>
#include <gdal_priv.h>
#include "../gdal_common_napi.hpp"

namespace node_gdal {

class GDALDriversNapi : public Napi::ObjectWrap<GDALDriversNapi> {
    public:
  static Napi::FunctionReference constructor;
  static Napi::Object Init(Napi::Env env, Napi::Object exports);
  static Napi::Value New(Napi::Env env);

  GDALDriversNapi(const Napi::CallbackInfo &info);

  Napi::Value toString(const Napi::CallbackInfo &info);
  Napi::Value get(const Napi::CallbackInfo &info);
  Napi::Value getNames(const Napi::CallbackInfo &info);
  Napi::Value count(const Napi::CallbackInfo &info);
};

} // namespace node_gdal

#endif
