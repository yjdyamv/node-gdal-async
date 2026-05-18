#ifndef __NODE_GDAL_COLLECTIONS_NAPI_H__
#define __NODE_GDAL_COLLECTIONS_NAPI_H__

#include <napi.h>
#include <gdal_priv.h>
#include "../gdal_common_napi.hpp"
#include "../async_napi.hpp"

namespace node_gdal {

class DatasetBandsNapi : public Napi::ObjectWrap<DatasetBandsNapi> {
    public:
  static Napi::FunctionReference constructor;
  static Napi::Object Init(Napi::Env env, Napi::Object exports);
  DatasetBandsNapi(const Napi::CallbackInfo &info);
  Napi::Value toString(const Napi::CallbackInfo &info);
  GDAL_ASYNCABLE_DECLARE_NAPI(get);
  GDAL_ASYNCABLE_DECLARE_NAPI(count);
};

class DatasetLayersNapi : public Napi::ObjectWrap<DatasetLayersNapi> {
    public:
  static Napi::FunctionReference constructor;
  static Napi::Object Init(Napi::Env env, Napi::Object exports);
  DatasetLayersNapi(const Napi::CallbackInfo &info);
  Napi::Value toString(const Napi::CallbackInfo &info);
  GDAL_ASYNCABLE_DECLARE_NAPI(count);
};

class LayerFeaturesNapi : public Napi::ObjectWrap<LayerFeaturesNapi> {
    public:
  static Napi::FunctionReference constructor;
  static Napi::Object Init(Napi::Env env, Napi::Object exports);
  LayerFeaturesNapi(const Napi::CallbackInfo &info);
  Napi::Value toString(const Napi::CallbackInfo &info);
  GDAL_ASYNCABLE_DECLARE_NAPI(count);
  GDAL_ASYNCABLE_DECLARE_NAPI(next);
};

} // namespace node_gdal

#endif
