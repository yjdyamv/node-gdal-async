#ifndef __NODE_OGR_FEATURE_NAPI_H__
#define __NODE_OGR_FEATURE_NAPI_H__

#include <napi.h>
#include <ogrsf_frmts.h>
#include <ogr_featurestyle.h>

#include "gdal_common_napi.hpp"

namespace node_gdal {

class FeatureNapi : public Napi::ObjectWrap<FeatureNapi> {
    public:
  static Napi::FunctionReference constructor;
  static Napi::Object Init(Napi::Env env, Napi::Object exports);
  static Napi::Value New(Napi::Env env, OGRFeature *feature);
  static Napi::Value New(Napi::Env env, OGRFeature *feature, bool owned);

  FeatureNapi(const Napi::CallbackInfo &info);
  ~FeatureNapi();

  Napi::Value toString(const Napi::CallbackInfo &info);
  Napi::Value clone(const Napi::CallbackInfo &info);
  Napi::Value setFrom(const Napi::CallbackInfo &info);
  Napi::Value destroy(const Napi::CallbackInfo &info);
  Napi::Value getStyleString(const Napi::CallbackInfo &info);
  Napi::Value setStyleString(const Napi::CallbackInfo &info);
  Napi::Value fieldsGetter(const Napi::CallbackInfo &info);
  Napi::Value getGeometry(const Napi::CallbackInfo &info);
  Napi::Value setGeometry(const Napi::CallbackInfo &info);

  Napi::Value fidGetter(const Napi::CallbackInfo &info);
  Napi::Value defnGetter(const Napi::CallbackInfo &info);

  void fidSetter(const Napi::CallbackInfo &info, const Napi::Value &value);

  OGRFeature *get() {
    return this_;
  }
  bool isAlive() {
    return this_ != nullptr;
  }

    private:
  OGRFeature *this_;
  bool owned_;
};

} // namespace node_gdal

#endif
