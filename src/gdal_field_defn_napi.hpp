#ifndef __NODE_OGR_FIELD_DEFN_NAPI_H__
#define __NODE_OGR_FIELD_DEFN_NAPI_H__

#include <napi.h>
#include <ogrsf_frmts.h>

#include "gdal_common_napi.hpp"

namespace node_gdal {

class FieldDefnNapi : public Napi::ObjectWrap<FieldDefnNapi> {
    public:
  static Napi::FunctionReference constructor;
  static Napi::Object Init(Napi::Env env, Napi::Object exports);
  static Napi::Value New(Napi::Env env, const OGRFieldDefn *def);
  static Napi::Value New(Napi::Env env, OGRFieldDefn *def, bool owned);

  FieldDefnNapi(const Napi::CallbackInfo &info);
  ~FieldDefnNapi();

  // Methods
  Napi::Value toString(const Napi::CallbackInfo &info);

  // Getters
  Napi::Value nameGetter(const Napi::CallbackInfo &info);
  Napi::Value typeGetter(const Napi::CallbackInfo &info);
  Napi::Value justificationGetter(const Napi::CallbackInfo &info);
  Napi::Value precisionGetter(const Napi::CallbackInfo &info);
  Napi::Value widthGetter(const Napi::CallbackInfo &info);
  Napi::Value ignoredGetter(const Napi::CallbackInfo &info);

  // Setters
  void nameSetter(const Napi::CallbackInfo &info, const Napi::Value &value);
  void typeSetter(const Napi::CallbackInfo &info, const Napi::Value &value);
  void justificationSetter(const Napi::CallbackInfo &info, const Napi::Value &value);
  void precisionSetter(const Napi::CallbackInfo &info, const Napi::Value &value);
  void widthSetter(const Napi::CallbackInfo &info, const Napi::Value &value);
  void ignoredSetter(const Napi::CallbackInfo &info, const Napi::Value &value);

  OGRFieldDefn *get() {
    return this_;
  }
  bool isAlive() {
    return this_ != nullptr;
  }

    private:
  OGRFieldDefn *this_;
  bool owned_;
};

} // namespace node_gdal

#endif
