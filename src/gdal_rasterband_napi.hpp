#ifndef __NODE_GDAL_RASTERBAND_NAPI_H__
#define __NODE_GDAL_RASTERBAND_NAPI_H__

#include <napi.h>
#include <gdal_priv.h>
#include "async_napi.hpp"

namespace node_gdal {

class RasterBandNapi : public Napi::ObjectWrap<RasterBandNapi> {
    public:
  static Napi::FunctionReference constructor;
  static Napi::Object Init(Napi::Env env, Napi::Object exports);
  static Napi::Value New(Napi::Env env, GDALRasterBand *band);

  RasterBandNapi(const Napi::CallbackInfo &info);
  ~RasterBandNapi();

  Napi::Value toString(const Napi::CallbackInfo &info);
  GDAL_ASYNCABLE_DECLARE_NAPI(flush);
  GDAL_ASYNCABLE_DECLARE_NAPI(fill);
  Napi::Value getStatistics(const Napi::CallbackInfo &info);
  GDAL_ASYNCABLE_DECLARE_NAPI(computeStatistics);
  Napi::Value setStatistics(const Napi::CallbackInfo &info);
  Napi::Value getMaskBand(const Napi::CallbackInfo &info);
  Napi::Value getMaskFlags(const Napi::CallbackInfo &info);
  Napi::Value createMaskBand(const Napi::CallbackInfo &info);

  GDAL_ASYNCABLE_GETTER_DECLARE_NAPI(sizeGetter);
  GDAL_ASYNCABLE_GETTER_DECLARE_NAPI(idGetter);
  GDAL_ASYNCABLE_GETTER_DECLARE_NAPI(descriptionGetter);
  GDAL_ASYNCABLE_GETTER_DECLARE_NAPI(blockSizeGetter);
  GDAL_ASYNCABLE_GETTER_DECLARE_NAPI(minimumGetter);
  GDAL_ASYNCABLE_GETTER_DECLARE_NAPI(maximumGetter);
  GDAL_ASYNCABLE_GETTER_DECLARE_NAPI(readOnlyGetter);
  GDAL_ASYNCABLE_GETTER_DECLARE_NAPI(dataTypeGetter);
  GDAL_ASYNCABLE_GETTER_DECLARE_NAPI(hasArbitraryOverviewsGetter);
  GDAL_ASYNCABLE_GETTER_DECLARE_NAPI(unitTypeGetter);
  GDAL_ASYNCABLE_GETTER_DECLARE_NAPI(scaleGetter);
  GDAL_ASYNCABLE_GETTER_DECLARE_NAPI(offsetGetter);
  GDAL_ASYNCABLE_GETTER_DECLARE_NAPI(noDataValueGetter);
  GDAL_ASYNCABLE_GETTER_DECLARE_NAPI(categoryNamesGetter);
  GDAL_ASYNCABLE_GETTER_DECLARE_NAPI(colorInterpretationGetter);

  void unitTypeSetter(const Napi::CallbackInfo &info, const Napi::Value &value);
  void scaleSetter(const Napi::CallbackInfo &info, const Napi::Value &value);
  void offsetSetter(const Napi::CallbackInfo &info, const Napi::Value &value);
  void noDataValueSetter(const Napi::CallbackInfo &info, const Napi::Value &value);
  void categoryNamesSetter(const Napi::CallbackInfo &info, const Napi::Value &value);

  GDALRasterBand *get() { return this_; }
  bool isAlive() { return this_ != nullptr; }

    private:
  GDALRasterBand *this_;
};

} // namespace node_gdal

#endif
