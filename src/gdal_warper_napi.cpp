#include "gdal_warper_napi.hpp"
#include "gdal_dataset_napi.hpp"
#include "gdal_spatial_reference_napi.hpp"
#include "gdal_common_napi.hpp"
#include "async_napi.hpp"
#include <gdalwarper.h>

namespace node_gdal {

// =========================================================================
// Init
// =========================================================================

Napi::Object WarperNapi::Init(Napi::Env env, Napi::Object exports) {
  auto obj = Napi::Object::New(env);

  obj.Set("reprojectImage", Napi::Function::New(env, [](const Napi::CallbackInfo &i) {
    return WarperNapi::reprojectImage_do(i, false);
  }));
  obj.Set("reprojectImageAsync", Napi::Function::New(env, [](const Napi::CallbackInfo &i) {
    return WarperNapi::reprojectImage_do(i, true);
  }));
  obj.Set("suggestedWarpOutput", Napi::Function::New(env, [](const Napi::CallbackInfo &i) {
    return WarperNapi::suggestedWarpOutput_do(i, false);
  }));
  obj.Set("suggestedWarpOutputAsync", Napi::Function::New(env, [](const Napi::CallbackInfo &i) {
    return WarperNapi::suggestedWarpOutput_do(i, true);
  }));

  exports.Set("Warper", obj);
  return exports;
}

// =========================================================================
// suggestedWarpOutput
// =========================================================================

Napi::Value WarperNapi::suggestedWarpOutput(const Napi::CallbackInfo &info) { return suggestedWarpOutput_do(info, false); }
Napi::Value WarperNapi::suggestedWarpOutputAsync(const Napi::CallbackInfo &info) { return suggestedWarpOutput_do(info, true); }
Napi::Value WarperNapi::suggestedWarpOutput_do(const Napi::CallbackInfo &info, bool async) {
  Napi::Env env = info.Env();

  Napi::Object obj;
  NAPI_ARG_OBJECT(0, "options", obj);

  // src Dataset
  Napi::Value srcVal = obj.Get("src");
  if (!srcVal.IsObject() || srcVal.IsNull()) {
    Napi::Error::New(env, "src dataset must be provided").ThrowAsJavaScriptException();
    return env.Undefined();
  }
  DatasetNapi *ds = nullptr;
  NAPI_ARG_WRAPPED(0, "options.src", DatasetNapi, ds);
  GDALDataset *gdal_ds = ds->get();

  // s_srs / t_srs
  if (!obj.Has("s_srs") || !obj.Has("t_srs")) {
    Napi::Error::New(env, "s_srs and t_srs must be provided").ThrowAsJavaScriptException();
    return env.Undefined();
  }
  Napi::Value sVal = obj.Get("s_srs"), tVal = obj.Get("t_srs");
  if (!sVal.IsObject() || !tVal.IsObject()) {
    Napi::Error::New(env, "s_srs and t_srs must be SpatialReferenceNapi objects").ThrowAsJavaScriptException();
    return env.Undefined();
  }
  SpatialReferenceNapi *s_srs = nullptr, *t_srs = nullptr;
  NAPI_ARG_WRAPPED(0, "options.s_srs", SpatialReferenceNapi, s_srs);
  NAPI_ARG_WRAPPED(0, "options.t_srs", SpatialReferenceNapi, t_srs);

  double maxError = 0;
  if (obj.Has("maxError")) maxError = obj.Get("maxError").As<Napi::Number>().DoubleValue();

  // Export SRS to WKT
  char *s_wkt = nullptr, *t_wkt = nullptr;
  if (s_srs->get()->exportToWkt(&s_wkt) != OGRERR_NONE) {
    Napi::Error::New(env, "Error converting s_srs to WKT").ThrowAsJavaScriptException();
    return env.Undefined();
  }
  if (t_srs->get()->exportToWkt(&t_wkt) != OGRERR_NONE) {
    CPLFree(s_wkt);
    Napi::Error::New(env, "Error converting t_srs to WKT").ThrowAsJavaScriptException();
    return env.Undefined();
  }
  std::string s_str(s_wkt), t_str(t_wkt);
  CPLFree(s_wkt); CPLFree(t_wkt);

  struct WarpOutputResult { double gt[6]; int w, h; };

  GDALDatasetH hDS = GDALDataset::ToHandle(gdal_ds);

  GDALAsyncableJobNapi<WarpOutputResult> job;
  job.main = [hDS, s_str, t_str, maxError]() -> WarpOutputResult {
    WarpOutputResult r{};
    CPLErrorReset();

    void *hGen = GDALCreateGenImgProjTransformer(hDS, s_str.c_str(), nullptr, t_str.c_str(), TRUE, 1000.0, 0);
    if (!hGen) throw CPLGetLastErrorMsg();

    void *hTransformArg;
    GDALTransformerFunc transformer;
    if (maxError > 0.0) {
      hTransformArg = GDALCreateApproxTransformer(GDALGenImgProjTransform, hGen, maxError);
      transformer = GDALApproxTransform;
      if (!hTransformArg) { GDALDestroyGenImgProjTransformer(hGen); throw CPLGetLastErrorMsg(); }
    } else {
      hTransformArg = hGen;
      transformer = GDALGenImgProjTransform;
    }

    CPLErr err = GDALSuggestedWarpOutput(hDS, transformer, hTransformArg, r.gt, &r.w, &r.h);
    GDALDestroyGenImgProjTransformer(hGen);
    if (maxError > 0.0) GDALDestroyApproxTransformer(hTransformArg);
    if (err) throw CPLGetLastErrorMsg();
    return r;
  };

  job.rval = [](Napi::Env env, WarpOutputResult r) -> Napi::Value {
    Napi::Object size = Napi::Object::New(env);
    size.Set("x", Napi::Number::New(env, r.w));
    size.Set("y", Napi::Number::New(env, r.h));
    Napi::Array gt = Napi::Array::New(env, 6);
    for (int i = 0; i < 6; i++) gt.Set(i, Napi::Number::New(env, r.gt[i]));
    Napi::Object result = Napi::Object::New(env);
    result.Set("rasterSize", size);
    result.Set("geoTransform", gt);
    return result;
  };

  return job.run(info, async, 1);
}

// =========================================================================
// reprojectImage
// =========================================================================

Napi::Value WarperNapi::reprojectImage(const Napi::CallbackInfo &info) { return reprojectImage_do(info, false); }
Napi::Value WarperNapi::reprojectImageAsync(const Napi::CallbackInfo &info) { return reprojectImage_do(info, true); }
Napi::Value WarperNapi::reprojectImage_do(const Napi::CallbackInfo &info, bool async) {
  Napi::Env env = info.Env();

  Napi::Object obj;
  NAPI_ARG_OBJECT(0, "options", obj);

  // src + dst Datasets
  if (!obj.Has("src") || !obj.Has("dst")) {
    Napi::Error::New(env, "src and dst must be provided").ThrowAsJavaScriptException();
    return env.Undefined();
  }

  DatasetNapi *ds = nullptr;
  auto srcVal = obj.Get("src");
  if (!srcVal.IsObject() || !srcVal.As<Napi::Object>().InstanceOf(DatasetNapi::constructor.Value())) {
    Napi::Error::New(env, "src must be a DatasetNapi").ThrowAsJavaScriptException();
    return env.Undefined();
  }
  ds = DatasetNapi::Unwrap(srcVal.As<Napi::Object>());
  GDALDataset *src_raw = ds->get();

  auto dstVal = obj.Get("dst");
  if (!dstVal.IsObject() || !dstVal.As<Napi::Object>().InstanceOf(DatasetNapi::constructor.Value())) {
    Napi::Error::New(env, "dst must be a DatasetNapi").ThrowAsJavaScriptException();
    return env.Undefined();
  }
  DatasetNapi *dst_ds = DatasetNapi::Unwrap(dstVal.As<Napi::Object>());
  GDALDataset *dst_raw = dst_ds->get();

  // s_srs / t_srs
  SpatialReferenceNapi *s_srs = nullptr, *t_srs = nullptr;
  if (obj.Has("s_srs")) {
    auto v = obj.Get("s_srs");
    if (v.IsObject() && v.As<Napi::Object>().InstanceOf(SpatialReferenceNapi::constructor.Value()))
      s_srs = SpatialReferenceNapi::Unwrap(v.As<Napi::Object>());
  }
  if (obj.Has("t_srs")) {
    auto v = obj.Get("t_srs");
    if (v.IsObject() && v.As<Napi::Object>().InstanceOf(SpatialReferenceNapi::constructor.Value()))
      t_srs = SpatialReferenceNapi::Unwrap(v.As<Napi::Object>());
  }
  if (!s_srs || !t_srs) {
    Napi::Error::New(env, "s_srs and t_srs must be SpatialReferenceNapi objects").ThrowAsJavaScriptException();
    return env.Undefined();
  }

  // Export SRS to WKT
  char *s_wkt = nullptr, *t_wkt = nullptr;
  if (s_srs->get()->exportToWkt(&s_wkt) != OGRERR_NONE || t_srs->get()->exportToWkt(&t_wkt) != OGRERR_NONE) {
    CPLFree(s_wkt); CPLFree(t_wkt);
    Napi::Error::New(env, "Error converting SRS to WKT").ThrowAsJavaScriptException();
    return env.Undefined();
  }
  std::string s_str(s_wkt), t_str(t_wkt);
  CPLFree(s_wkt); CPLFree(t_wkt);

  // Optional: resampling
  GDALResampleAlg resample = GRA_NearestNeighbour;
  if (obj.Has("resampling")) {
    std::string r = obj.Get("resampling").As<Napi::String>().Utf8Value();
    if (r == "Bilinear") resample = GRA_Bilinear;
    else if (r == "Cubic") resample = GRA_Cubic;
    else if (r == "CubicSpline") resample = GRA_CubicSpline;
    else if (r == "Lanczos") resample = GRA_Lanczos;
    else if (r == "Average") resample = GRA_Average;
    else if (r == "Mode") resample = GRA_Mode;
    else if (r == "Min") resample = GRA_Min;
    else if (r == "Max") resample = GRA_Max;
    else if (r == "Med") resample = GRA_Med;
    else if (r == "Q1") resample = GRA_Q1;
    else if (r == "Q3") resample = GRA_Q3;
  }

  double maxError = 0, memoryLimit = 0;
  if (obj.Has("maxError")) maxError = obj.Get("maxError").As<Napi::Number>().DoubleValue();
  if (obj.Has("memoryLimit")) memoryLimit = obj.Get("memoryLimit").As<Napi::Number>().DoubleValue();
  bool useMulti = obj.Has("multi") && obj.Get("multi").As<Napi::Boolean>().Value();

  // Build GDALWarpOptions
  GDALWarpOptions *opts = GDALCreateWarpOptions();
  opts->hSrcDS = GDALDataset::ToHandle(src_raw);
  opts->hDstDS = GDALDataset::ToHandle(dst_raw);
  opts->eResampleAlg = resample;
  opts->dfWarpMemoryLimit = memoryLimit;

  // Handle srcBands / dstBands
  std::vector<int> src_bands, dst_bands;
  if (obj.Has("srcBands")) {
    Napi::Array arr = obj.Get("srcBands").As<Napi::Array>();
    for (uint32_t i = 0; i < arr.Length(); i++) src_bands.push_back(arr.Get(i).As<Napi::Number>().Int32Value());
  }
  if (obj.Has("dstBands")) {
    Napi::Array arr = obj.Get("dstBands").As<Napi::Array>();
    for (uint32_t i = 0; i < arr.Length(); i++) dst_bands.push_back(arr.Get(i).As<Napi::Number>().Int32Value());
  }
  if (!src_bands.empty()) {
    opts->nBandCount = (int)src_bands.size();
    opts->panSrcBands = (int *)CPLMalloc(sizeof(int) * src_bands.size());
    opts->panDstBands = (int *)CPLMalloc(sizeof(int) * src_bands.size());
    for (size_t i = 0; i < src_bands.size(); i++) {
      opts->panSrcBands[i] = src_bands[i];
      opts->panDstBands[i] = i < dst_bands.size() ? dst_bands[i] : src_bands[i];
    }
  }

  GDALDatasetH hSrc = opts->hSrcDS, hDst = opts->hDstDS;

  GDALAsyncableJobNapi<CPLErr> job;
  job.main = [hSrc, hDst, s_str, t_str, resample, memoryLimit, maxError, useMulti]() -> CPLErr {
    CPLErrorReset();
    GDALWarpOptions *wOpts = GDALCreateWarpOptions();
    wOpts->hSrcDS = hSrc;
    wOpts->hDstDS = hDst;
    wOpts->eResampleAlg = resample;
    wOpts->dfWarpMemoryLimit = memoryLimit;

    CPLErr err;
    if (useMulti) {
      err = GDALReprojectImage(hSrc, s_str.c_str(), hDst, t_str.c_str(), resample, memoryLimit, maxError, nullptr, nullptr, wOpts);
    } else {
      err = GDALReprojectImage(hSrc, s_str.c_str(), hDst, t_str.c_str(), resample, memoryLimit, maxError, nullptr, nullptr, wOpts);
    }
    GDALDestroyWarpOptions(wOpts);
    if (err) throw CPLGetLastErrorMsg();
    return err;
  };
  job.rval = [](Napi::Env, CPLErr) { return Napi::Value(); };

  return job.run(info, async, 1);
}

} // namespace node_gdal
