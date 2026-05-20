#ifndef __NODE_OGR_LAYER_NAPI_H__
#define __NODE_OGR_LAYER_NAPI_H__

#include <napi.h>
#include <ogrsf_frmts.h>
#include "async_napi.hpp"

namespace node_gdal {

class LayerNapi : public Napi::ObjectWrap<LayerNapi> {
    public:
  static Napi::FunctionReference constructor;
  static Napi::Object Init(Napi::Env env, Napi::Object exports);
  static Napi::Value New(Napi::Env env, OGRLayer *layer);

  LayerNapi(const Napi::CallbackInfo &info);
  ~LayerNapi();

  Napi::Value toString(const Napi::CallbackInfo &info);
  Napi::Value getExtent(const Napi::CallbackInfo &info);
  Napi::Value setAttributeFilter(const Napi::CallbackInfo &info);
  Napi::Value setSpatialFilter(const Napi::CallbackInfo &info);
  Napi::Value getSpatialFilter(const Napi::CallbackInfo &info);
  Napi::Value testCapability(const Napi::CallbackInfo &info);
  GDAL_ASYNCABLE_DECLARE_NAPI(syncToDisk);

  Napi::Value srsGetter(const Napi::CallbackInfo &info);
  Napi::Value nameGetter(const Napi::CallbackInfo &info);
  Napi::Value geomTypeGetter(const Napi::CallbackInfo &info);
  Napi::Value geomColumnGetter(const Napi::CallbackInfo &info);
  Napi::Value fidColumnGetter(const Napi::CallbackInfo &info);
  Napi::Value featuresGetter(const Napi::CallbackInfo &info);
  Napi::Value fieldsGetter(const Napi::CallbackInfo &info);

  OGRLayer *get() {
    return this_;
  }
  bool isAlive() {
    return this_ != nullptr;
  }

    private:
  OGRLayer *this_;
};

} // namespace node_gdal

#endif
