#ifndef __NODE_GDAL_DRIVER_NAPI_H__
#define __NODE_GDAL_DRIVER_NAPI_H__

// napi
#pragma warning(push)
#pragma warning(disable: 4100)
#include <napi.h>
#pragma warning(pop)

// gdal
#include <gdal_priv.h>
#include <ogrsf_frmts.h>

#include "async_napi.hpp"

namespace node_gdal {

class DriverNapi : public Napi::ObjectWrap<DriverNapi> {
    public:
  static Napi::FunctionReference constructor;
  static Napi::Object Init(Napi::Env env, Napi::Object exports);
  static Napi::Value New(Napi::Env env, GDALDriver *driver);

  DriverNapi(const Napi::CallbackInfo &info);
  ~DriverNapi();

  // Async methods (declared via macro to generate sync/async/_do trio)
  GDAL_ASYNCABLE_DECLARE_NAPI(open);
  GDAL_ASYNCABLE_DECLARE_NAPI(create);
  GDAL_ASYNCABLE_DECLARE_NAPI(createCopy);

  // Sync-only methods
  Napi::Value toString(const Napi::CallbackInfo &info);
  Napi::Value deleteDataset(const Napi::CallbackInfo &info);
  Napi::Value rename(const Napi::CallbackInfo &info);
  Napi::Value copyFiles(const Napi::CallbackInfo &info);
  Napi::Value getMetadata(const Napi::CallbackInfo &info);

  // Property accessors
  Napi::Value descriptionGetter(const Napi::CallbackInfo &info);

  GDALDriver *getGDALDriver() {
    return this_gdaldriver;
  }
  bool isAlive() {
    return this_gdaldriver != nullptr;
  }
  long uid;

    private:
  GDALDriver *this_gdaldriver;
};

} // namespace node_gdal

#endif
