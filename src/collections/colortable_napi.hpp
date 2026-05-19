#ifndef __NODE_GDAL_COLORTABLE_NAPI_H__
#define __NODE_GDAL_COLORTABLE_NAPI_H__

#include <napi.h>
#include <gdal_priv.h>
#include "../gdal_common_napi.hpp"

namespace node_gdal {

class ColorTableNapi : public Napi::ObjectWrap<ColorTableNapi> {
    public:
  static Napi::FunctionReference constructor;
  static Napi::Object Init(Napi::Env env, Napi::Object exports);
  static Napi::Value New(Napi::Env env, GDALColorTable *raw);

  ColorTableNapi(const Napi::CallbackInfo &info);
  ~ColorTableNapi();

  Napi::Value toString(const Napi::CallbackInfo &info);
  Napi::Value isSame(const Napi::CallbackInfo &info);
  Napi::Value clone(const Napi::CallbackInfo &info);
  Napi::Value get(const Napi::CallbackInfo &info);
  Napi::Value set(const Napi::CallbackInfo &info);
  Napi::Value count(const Napi::CallbackInfo &info);
  Napi::Value ramp(const Napi::CallbackInfo &info);
  Napi::Value interpretationGetter(const Napi::CallbackInfo &info);

  GDALColorTable *get() {
    return this_;
  }
  bool isAlive() {
    return this_ != nullptr;
  }

    private:
  GDALColorTable *this_;
};

} // namespace node_gdal

#endif
