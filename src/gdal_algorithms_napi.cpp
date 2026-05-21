#include "gdal_algorithms_napi.hpp"
#include "gdal_rasterband_napi.hpp"
#include "gdal_layer_napi.hpp"
#include "gdal_common_napi.hpp"
#include "async_napi.hpp"
#include <gdal_alg.h>

namespace node_gdal {

Napi::Object AlgorithmsNapi::Init(Napi::Env env, Napi::Object exports) {
  auto obj = Napi::Object::New(env);

  obj.Set("fillNodata", Napi::Function::New(env, AlgorithmsNapi::fillNodata));
  obj.Set("fillNodataAsync", Napi::Function::New(env, AlgorithmsNapi::fillNodataAsync));
  obj.Set("contourGenerate", Napi::Function::New(env, AlgorithmsNapi::contourGenerate));
  obj.Set("contourGenerateAsync", Napi::Function::New(env, AlgorithmsNapi::contourGenerateAsync));
  obj.Set("sieveFilter", Napi::Function::New(env, AlgorithmsNapi::sieveFilter));
  obj.Set("sieveFilterAsync", Napi::Function::New(env, AlgorithmsNapi::sieveFilterAsync));
  obj.Set("checksumImage", Napi::Function::New(env, AlgorithmsNapi::checksumImage));
  obj.Set("checksumImageAsync", Napi::Function::New(env, AlgorithmsNapi::checksumImageAsync));
  obj.Set("polygonize", Napi::Function::New(env, AlgorithmsNapi::polygonize));
  obj.Set("polygonizeAsync", Napi::Function::New(env, AlgorithmsNapi::polygonizeAsync));

  exports.Set("Algorithms", obj);
  // Also register at top level for backward compatibility
  exports.Set("fillNodata", obj.Get("fillNodata"));
  exports.Set("fillNodataAsync", obj.Get("fillNodataAsync"));
  exports.Set("contourGenerate", obj.Get("contourGenerate"));
  exports.Set("contourGenerateAsync", obj.Get("contourGenerateAsync"));
  exports.Set("sieveFilter", obj.Get("sieveFilter"));
  exports.Set("sieveFilterAsync", obj.Get("sieveFilterAsync"));
  exports.Set("checksumImage", obj.Get("checksumImage"));
  exports.Set("checksumImageAsync", obj.Get("checksumImageAsync"));
  exports.Set("polygonize", obj.Get("polygonize"));
  exports.Set("polygonizeAsync", obj.Get("polygonizeAsync"));
  return exports;
}

// =========================================================================
// fillNodata
// =========================================================================

Napi::Value AlgorithmsNapi::fillNodata(const Napi::CallbackInfo &info) { return fillNodata_do(info, false); }
Napi::Value AlgorithmsNapi::fillNodataAsync(const Napi::CallbackInfo &info) { return fillNodata_do(info, true); }
Napi::Value AlgorithmsNapi::fillNodata_do(const Napi::CallbackInfo &info, bool async) {
  Napi::Env env = info.Env();

  Napi::Object options;
  NAPI_ARG_OBJECT(0, "options", options);

  auto getObjProp = [&](const char *key, Napi::Value &val) -> bool {
    if (!options.Has(key)) {
      Napi::Error::New(env, "options must contain property \"" + std::string(key) + "\"")
        .ThrowAsJavaScriptException();
      return false;
    }
    val = options.Get(key);
    return true;
  };

  Napi::Value srcVal;
  if (!getObjProp("src", srcVal)) return env.Undefined();
  RasterBandNapi *src = nullptr;
  NAPI_ARG_WRAPPED(0, "options.src", RasterBandNapi, src);

  RasterBandNapi *mask = nullptr;
  if (options.Has("mask") && !options.Get("mask").IsNull() && !options.Get("mask").IsUndefined()) {
    NAPI_ARG_WRAPPED(0, "options.mask", RasterBandNapi, mask);
  }

  Napi::Value searchDistVal;
  if (!getObjProp("searchDist", searchDistVal)) return env.Undefined();
  double search_dist = searchDistVal.As<Napi::Number>().DoubleValue();

  int smooth_iterations = 0;
  if (options.Has("smoothIterations")) {
    smooth_iterations = options.Get("smoothIterations").As<Napi::Number>().Int32Value();
  }

  GDALRasterBand *gdal_src = src->get();
  GDALRasterBand *gdal_mask = mask ? mask->get() : nullptr;

  GDALAsyncableJobNapi<CPLErr> job;
  job.main = [gdal_src, gdal_mask, search_dist, smooth_iterations]() -> CPLErr {
    CPLErrorReset();
    CPLErr err = GDALFillNodata(gdal_src, gdal_mask, search_dist, 0, smooth_iterations,
                                NULL, NULL, NULL);
    if (err) { throw CPLGetLastErrorMsg(); }
    return err;
  };
  job.rval = [](Napi::Env env, CPLErr r) {
    return env.Undefined();
  };

  return job.run(info, async, 1);
}

// =========================================================================
// contourGenerate
// =========================================================================

Napi::Value AlgorithmsNapi::contourGenerate(const Napi::CallbackInfo &info) { return contourGenerate_do(info, false); }
Napi::Value AlgorithmsNapi::contourGenerateAsync(const Napi::CallbackInfo &info) { return contourGenerate_do(info, true); }
Napi::Value AlgorithmsNapi::contourGenerate_do(const Napi::CallbackInfo &info, bool async) {
  Napi::Env env = info.Env();

  Napi::Object options;
  NAPI_ARG_OBJECT(0, "options", options);

  RasterBandNapi *src = nullptr;
  LayerNapi *dst = nullptr;

  NAPI_ARG_WRAPPED(0, "options.src", RasterBandNapi, src);
  NAPI_ARG_WRAPPED(0, "options.dst", LayerNapi, dst);

  int id_field = -1, elev_field = -1;
  NAPI_INT_FROM_OBJ(options, "idField", id_field);
  NAPI_INT_FROM_OBJ(options, "elevField", elev_field);

  double interval = 100;
  if (options.Has("interval")) {
    interval = options.Get("interval").As<Napi::Number>().DoubleValue();
  }
  double base = 0;
  if (options.Has("offset")) {
    base = options.Get("offset").As<Napi::Number>().DoubleValue();
  }

  double *fixed_levels = nullptr;
  unsigned int n_fixed_levels = 0;
  std::vector<double> fixed_levels_vec;
  if (options.Has("fixedLevels")) {
    Napi::Value flVal = options.Get("fixedLevels");
    if (flVal.IsArray()) {
      Napi::Array flArr = flVal.As<Napi::Array>();
      n_fixed_levels = flArr.Length();
      fixed_levels_vec.reserve(n_fixed_levels);
      for (uint32_t i = 0; i < n_fixed_levels; i++) {
        fixed_levels_vec.push_back(flArr.Get(i).As<Napi::Number>().DoubleValue());
      }
      fixed_levels = fixed_levels_vec.data();
    }
  }

  bool use_nodata = false;
  double nodata = 0;
  if (options.Has("nodata") && !options.Get("nodata").IsNull() && !options.Get("nodata").IsUndefined()) {
    use_nodata = true;
    nodata = options.Get("nodata").As<Napi::Number>().DoubleValue();
  }

  GDALRasterBand *gdal_src = src->get();
  OGRLayer *gdal_dst = dst->get();

  GDALAsyncableJobNapi<CPLErr> job;
  job.main = [gdal_src, interval, base, n_fixed_levels, fixed_levels, use_nodata, nodata,
              gdal_dst, id_field, elev_field]() -> CPLErr {
    CPLErrorReset();
    CPLErr err = GDALContourGenerate(gdal_src, interval, base, n_fixed_levels, fixed_levels,
                                     use_nodata, nodata, gdal_dst, id_field, elev_field,
                                     nullptr, nullptr);
    if (err) { throw CPLGetLastErrorMsg(); }
    return err;
  };
  job.rval = [](Napi::Env env, CPLErr r) {
    return env.Undefined();
  };

  return job.run(info, async, 1);
}

// =========================================================================
// sieveFilter
// =========================================================================

Napi::Value AlgorithmsNapi::sieveFilter(const Napi::CallbackInfo &info) { return sieveFilter_do(info, false); }
Napi::Value AlgorithmsNapi::sieveFilterAsync(const Napi::CallbackInfo &info) { return sieveFilter_do(info, true); }
Napi::Value AlgorithmsNapi::sieveFilter_do(const Napi::CallbackInfo &info, bool async) {
  Napi::Env env = info.Env();

  Napi::Object options;
  NAPI_ARG_OBJECT(0, "options", options);

  RasterBandNapi *src = nullptr;
  RasterBandNapi *dst = nullptr;
  RasterBandNapi *mask = nullptr;

  NAPI_ARG_WRAPPED(0, "options.src", RasterBandNapi, src);
  NAPI_ARG_WRAPPED(0, "options.dst", RasterBandNapi, dst);

  if (options.Has("mask") && !options.Get("mask").IsNull() && !options.Get("mask").IsUndefined()) {
    NAPI_ARG_WRAPPED(0, "options.mask", RasterBandNapi, mask);
  }

  int threshold;
  NAPI_INT_FROM_OBJ(options, "threshold", threshold);

  int connectedness = 4;
  NAPI_INT_FROM_OBJ(options, "connectedness", connectedness);

  if (connectedness != 4 && connectedness != 8) {
    Napi::Error::New(env, "connectedness option must be 4 or 8").ThrowAsJavaScriptException();
    return env.Undefined();
  }

  GDALRasterBand *gdal_src = src->get();
  GDALRasterBand *gdal_dst = dst->get();
  GDALRasterBand *gdal_mask = mask ? mask->get() : nullptr;

  GDALAsyncableJobNapi<CPLErr> job;
  job.main = [gdal_src, gdal_dst, gdal_mask, threshold, connectedness]() -> CPLErr {
    CPLErrorReset();
    CPLErr err = GDALSieveFilter(gdal_src, gdal_mask, gdal_dst, threshold, connectedness,
                                 NULL, nullptr, nullptr);
    if (err) { throw CPLGetLastErrorMsg(); }
    return err;
  };
  job.rval = [](Napi::Env env, CPLErr r) {
    return env.Undefined();
  };

  return job.run(info, async, 1);
}

// =========================================================================
// checksumImage
// =========================================================================

Napi::Value AlgorithmsNapi::checksumImage(const Napi::CallbackInfo &info) { return checksumImage_do(info, false); }
Napi::Value AlgorithmsNapi::checksumImageAsync(const Napi::CallbackInfo &info) { return checksumImage_do(info, true); }
Napi::Value AlgorithmsNapi::checksumImage_do(const Napi::CallbackInfo &info, bool async) {
  Napi::Env env = info.Env();

  RasterBandNapi *src = nullptr;
  NAPI_ARG_WRAPPED(0, "src", RasterBandNapi, src);

  GDALRasterBand *gdal_src = src->get();
  int bandw = gdal_src->GetXSize();
  int bandh = gdal_src->GetYSize();

  int x = 0, y = 0, w = bandw, h = bandh;
  NAPI_ARG_INT_OPT(1, "x", x);
  NAPI_ARG_INT_OPT(2, "y", y);
  NAPI_ARG_INT_OPT(3, "xSize", w);
  NAPI_ARG_INT_OPT(4, "ySize", h);

  if (x < 0 || y < 0 || x >= bandw || y >= bandh) {
    Napi::RangeError::New(env, "offset invalid for given band").ThrowAsJavaScriptException();
    return env.Undefined();
  }
  if (w < 0 || h < 0 || w > bandw || h > bandh) {
    Napi::RangeError::New(env, "x and y size must be smaller than band dimensions and greater than 0")
      .ThrowAsJavaScriptException();
    return env.Undefined();
  }
  if (x + w - 1 >= bandw || y + h - 1 >= bandh) {
    Napi::RangeError::New(env, "given range is outside bounds of given band")
      .ThrowAsJavaScriptException();
    return env.Undefined();
  }

  GDALAsyncableJobNapi<int> job;
  job.main = [gdal_src, x, y, w, h]() -> int {
    CPLErrorReset();
    int r = GDALChecksumImage(gdal_src, x, y, w, h);
    if (r == -1) throw CPLGetLastErrorMsg();
    return r;
  };
  job.rval = [](Napi::Env env, int r) {
    return Napi::Number::New(env, r);
  };

  return job.run(info, async, 5);
}

// =========================================================================
// polygonize
// =========================================================================

Napi::Value AlgorithmsNapi::polygonize(const Napi::CallbackInfo &info) { return polygonize_do(info, false); }
Napi::Value AlgorithmsNapi::polygonizeAsync(const Napi::CallbackInfo &info) { return polygonize_do(info, true); }
Napi::Value AlgorithmsNapi::polygonize_do(const Napi::CallbackInfo &info, bool async) {
  Napi::Env env = info.Env();

  Napi::Object options;
  NAPI_ARG_OBJECT(0, "options", options);

  RasterBandNapi *src = nullptr;
  LayerNapi *dst = nullptr;
  RasterBandNapi *mask = nullptr;

  NAPI_ARG_WRAPPED(0, "options.src", RasterBandNapi, src);
  NAPI_ARG_WRAPPED(0, "options.dst", LayerNapi, dst);

  if (options.Has("mask") && !options.Get("mask").IsNull() && !options.Get("mask").IsUndefined()) {
    NAPI_ARG_WRAPPED(0, "options.mask", RasterBandNapi, mask);
  }

  int connectedness = 4;
  if (options.Has("connectedness")) {
    connectedness = options.Get("connectedness").As<Napi::Number>().Int32Value();
  }

  int pix_val_field;
  NAPI_INT_FROM_OBJ(options, "pixValField", pix_val_field);

  if (connectedness != 4 && connectedness != 8) {
    Napi::Error::New(env, "connectedness must be 4 or 8").ThrowAsJavaScriptException();
    return env.Undefined();
  }

  char **papszOptions = nullptr;
  if (connectedness == 8) {
    papszOptions = CSLSetNameValue(papszOptions, "8CONNECTED", "8");
  }

  bool useFloats = false;
  if (options.Has("useFloats") && !options.Get("useFloats").IsNull() && !options.Get("useFloats").IsUndefined()) {
    useFloats = options.Get("useFloats").As<Napi::Boolean>().Value();
  }

  GDALRasterBand *gdal_src = src->get();
  OGRLayer *gdal_dst = dst->get();
  GDALRasterBand *gdal_mask = mask ? mask->get() : nullptr;

  GDALAsyncableJobNapi<CPLErr> job;
  if (useFloats) {
    job.main = [gdal_src, gdal_mask, gdal_dst, pix_val_field, papszOptions]() -> CPLErr {
      CPLErrorReset();
      CPLErr err = GDALFPolygonize(gdal_src, gdal_mask, reinterpret_cast<OGRLayerH>(gdal_dst),
                                    pix_val_field, papszOptions, nullptr, nullptr);
      CSLDestroy(papszOptions);
      if (err) throw CPLGetLastErrorMsg();
      return err;
    };
  } else {
    job.main = [gdal_src, gdal_mask, gdal_dst, pix_val_field, papszOptions]() -> CPLErr {
      CPLErrorReset();
      CPLErr err = GDALPolygonize(gdal_src, gdal_mask, reinterpret_cast<OGRLayerH>(gdal_dst),
                                   pix_val_field, papszOptions, nullptr, nullptr);
      CSLDestroy(papszOptions);
      if (err) throw CPLGetLastErrorMsg();
      return err;
    };
  }
  job.rval = [](Napi::Env env, CPLErr r) {
    return env.Undefined();
  };

  return job.run(info, async, 1);
}

} // namespace node_gdal
