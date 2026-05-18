#ifndef __NODE_GDAL_MULTI_NAPI_H__
#define __NODE_GDAL_MULTI_NAPI_H__

#include <napi.h>
#include <gdal_priv.h>
#include "gdal_common_napi.hpp"

namespace node_gdal {

class GroupNapi : public Napi::ObjectWrap<GroupNapi> {
    public:
  static Napi::FunctionReference constructor;
  static Napi::Object Init(Napi::Env env, Napi::Object exports);
  GroupNapi(const Napi::CallbackInfo &info);
  Napi::Value toString(const Napi::CallbackInfo &info);
  Napi::Value descriptionGetter(const Napi::CallbackInfo &info);
  GDALGroup *get() { return this_; }
  bool isAlive() { return this_ != nullptr; }
    private:
  GDALGroup *this_;
};

class MDArrayNapi : public Napi::ObjectWrap<MDArrayNapi> {
    public:
  static Napi::FunctionReference constructor;
  static Napi::Object Init(Napi::Env env, Napi::Object exports);
  MDArrayNapi(const Napi::CallbackInfo &info);
  Napi::Value toString(const Napi::CallbackInfo &info);
  Napi::Value dataTypeGetter(const Napi::CallbackInfo &info);
  Napi::Value dimensionCountGetter(const Napi::CallbackInfo &info);
  GDALMDArray *get() { return this_; }
  bool isAlive() { return this_ != nullptr; }
    private:
  GDALMDArray *this_;
};

class DimensionNapi : public Napi::ObjectWrap<DimensionNapi> {
    public:
  static Napi::FunctionReference constructor;
  static Napi::Object Init(Napi::Env env, Napi::Object exports);
  DimensionNapi(const Napi::CallbackInfo &info);
  Napi::Value toString(const Napi::CallbackInfo &info);
  Napi::Value nameGetter(const Napi::CallbackInfo &info);
  Napi::Value sizeGetter(const Napi::CallbackInfo &info);
  Napi::Value typeGetter(const Napi::CallbackInfo &info);
  GDALDimension *get() { return this_; }
  bool isAlive() { return this_ != nullptr; }
    private:
  GDALDimension *this_;
};

class AttributeNapi : public Napi::ObjectWrap<AttributeNapi> {
    public:
  static Napi::FunctionReference constructor;
  static Napi::Object Init(Napi::Env env, Napi::Object exports);
  AttributeNapi(const Napi::CallbackInfo &info);
  Napi::Value toString(const Napi::CallbackInfo &info);
  Napi::Value nameGetter(const Napi::CallbackInfo &info);
  Napi::Value valueGetter(const Napi::CallbackInfo &info);
  GDALAttribute *get() { return this_; }
  bool isAlive() { return this_ != nullptr; }
    private:
  GDALAttribute *this_;
};

} // namespace node_gdal

#endif
