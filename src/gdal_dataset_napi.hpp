#ifndef __NODE_GDAL_DATASET_NAPI_H__
#define __NODE_GDAL_DATASET_NAPI_H__

#include <napi.h>
#include <gdal_priv.h>
#include <ogrsf_frmts.h>

#include "async_napi.hpp"

namespace node_gdal {

class DatasetNapi : public Napi::ObjectWrap<DatasetNapi> {
    public:
  static Napi::FunctionReference constructor;
  static Napi::Object Init(Napi::Env env, Napi::Object exports);
  static Napi::Value New(Napi::Env env, GDALDataset *ds);

  DatasetNapi(const Napi::CallbackInfo &info);
  ~DatasetNapi();

  Napi::Value toString(const Napi::CallbackInfo &info);
  Napi::Value close(const Napi::CallbackInfo &info);

  GDAL_ASYNCABLE_DECLARE_NAPI(flush);
  GDAL_ASYNCABLE_DECLARE_NAPI(getMetadata);

  Napi::Value getFileList(const Napi::CallbackInfo &info);
  Napi::Value testCapability(const Napi::CallbackInfo &info);

  Napi::Value descriptionGetter(const Napi::CallbackInfo &info);
  Napi::Value driverGetter(const Napi::CallbackInfo &info);
  Napi::Value threadSafeGetter(const Napi::CallbackInfo &info);
  GDAL_ASYNCABLE_GETTER_DECLARE_NAPI(rasterSizeGetter);
  GDAL_ASYNCABLE_GETTER_DECLARE_NAPI(srsGetter);
  GDAL_ASYNCABLE_GETTER_DECLARE_NAPI(geoTransformGetter);

  void srsSetter(const Napi::CallbackInfo &info, const Napi::Value &value);
  void geoTransformSetter(const Napi::CallbackInfo &info, const Napi::Value &value);

  GDALDataset *get() {
    return this_dataset;
  }
  bool isAlive() {
    return this_dataset != nullptr;
  }

    private:
  GDALDataset *this_dataset;
};

} // namespace node_gdal

#endif
