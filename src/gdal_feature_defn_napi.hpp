#ifndef __NODE_OGR_FEATURE_DEFN_NAPI_H__
#define __NODE_OGR_FEATURE_DEFN_NAPI_H__

#include <napi.h>
#include <ogrsf_frmts.h>

#include "gdal_common_napi.hpp"

namespace node_gdal {

class FeatureDefnNapi : public Napi::ObjectWrap<FeatureDefnNapi> {
    public:
  static Napi::FunctionReference constructor;
  static Napi::Object Init(Napi::Env env, Napi::Object exports);
  static Napi::Value New(Napi::Env env, const OGRFeatureDefn *def);
  static Napi::Value New(Napi::Env env, OGRFeatureDefn *def, bool owned);

  FeatureDefnNapi(const Napi::CallbackInfo &info);
  ~FeatureDefnNapi();

  Napi::Value toString(const Napi::CallbackInfo &info);
  Napi::Value clone(const Napi::CallbackInfo &info);

  Napi::Value nameGetter(const Napi::CallbackInfo &info);
  Napi::Value geomTypeGetter(const Napi::CallbackInfo &info);
  Napi::Value geomIgnoredGetter(const Napi::CallbackInfo &info);
  Napi::Value styleIgnoredGetter(const Napi::CallbackInfo &info);

  void geomTypeSetter(const Napi::CallbackInfo &info, const Napi::Value &value);
  void geomIgnoredSetter(const Napi::CallbackInfo &info, const Napi::Value &value);
  void styleIgnoredSetter(const Napi::CallbackInfo &info, const Napi::Value &value);

  OGRFeatureDefn *get() {
    return this_;
  }
  bool isAlive() {
    return this_ != nullptr;
  }

    private:
  OGRFeatureDefn *this_;
  Napi::Value fieldsGetter(const Napi::CallbackInfo &info);
  bool owned_;
};

} // namespace node_gdal

#endif
