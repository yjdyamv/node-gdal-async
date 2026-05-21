#include "gdal_utils_napi.hpp"
#include "gdal_dataset_napi.hpp"
#include "gdal_common_napi.hpp"
#include "async_napi.hpp"
#include <gdal_utils.h>

namespace node_gdal {

Napi::Object UtilsNapi::Init(Napi::Env env, Napi::Object exports) {
  auto obj = Napi::Object::New(env);
  // Helper: register function on both obj and exports
  auto reg = [&](const char *name, Napi::Function f) {
    obj.Set(name, f);
    exports.Set(name, f);  // also at gdal.{name} level (NAN compat)
  };

  reg("info", Napi::Function::New(env, [](const Napi::CallbackInfo &i) {
    return UtilsNapi::gdalInfo_do(i, false);
  }));
  reg("infoAsync", Napi::Function::New(env, [](const Napi::CallbackInfo &i) {
    return UtilsNapi::gdalInfo_do(i, true);
  }));
  reg("translate", Napi::Function::New(env, [](const Napi::CallbackInfo &i) {
    return UtilsNapi::translate_do(i, false);
  }));
  reg("translateAsync", Napi::Function::New(env, [](const Napi::CallbackInfo &i) {
    return UtilsNapi::translate_do(i, true);
  }));
  reg("vectorTranslate", Napi::Function::New(env, [](const Napi::CallbackInfo &i) {
    return UtilsNapi::vectorTranslate_do(i, false);
  }));
  reg("vectorTranslateAsync", Napi::Function::New(env, [](const Napi::CallbackInfo &i) {
    return UtilsNapi::vectorTranslate_do(i, true);
  }));
  reg("warp", Napi::Function::New(env, [](const Napi::CallbackInfo &i) {
    return UtilsNapi::warp_do(i, false);
  }));
  reg("warpAsync", Napi::Function::New(env, [](const Napi::CallbackInfo &i) {
    return UtilsNapi::warp_do(i, true);
  }));
  reg("buildVRT", Napi::Function::New(env, [](const Napi::CallbackInfo &i) {
    return UtilsNapi::buildVRT_do(i, false);
  }));
  reg("buildVRTAsync", Napi::Function::New(env, [](const Napi::CallbackInfo &i) {
    return UtilsNapi::buildVRT_do(i, true);
  }));
  reg("rasterize", Napi::Function::New(env, [](const Napi::CallbackInfo &i) {
    return UtilsNapi::rasterize_do(i, false);
  }));
  reg("rasterizeAsync", Napi::Function::New(env, [](const Napi::CallbackInfo &i) {
    return UtilsNapi::rasterize_do(i, true);
  }));
  reg("dem", Napi::Function::New(env, [](const Napi::CallbackInfo &i) {
    return UtilsNapi::dem_do(i, false);
  }));
  reg("demAsync", Napi::Function::New(env, [](const Napi::CallbackInfo &i) {
    return UtilsNapi::dem_do(i, true);
  }));

  exports.Set("Utils", obj);
  return exports;
}

// =========================================================================
// gdalInfo
// =========================================================================

Napi::Value UtilsNapi::gdalInfo(const Napi::CallbackInfo &info) { return gdalInfo_do(info, false); }
Napi::Value UtilsNapi::gdalInfoAsync(const Napi::CallbackInfo &info) { return gdalInfo_do(info, true); }
Napi::Value UtilsNapi::gdalInfo_do(const Napi::CallbackInfo &info, bool async) {
  auto aosOptions = std::make_shared<CPLStringList>();

  Napi::Object src;
  NAPI_ARG_OBJECT(0, "src", src);
  DatasetNapi *ds = nullptr;
  NAPI_ARG_WRAPPED(0, "src", DatasetNapi, ds);
  GDALDataset *raw = ds->get();

  Napi::Array args;
  NAPI_ARG_ARRAY_OPT(1, "args", args);
  if (info.Length() > 1 && args.Length() > 0) {
    for (uint32_t i = 0; i < args.Length(); i++) {
      aosOptions->AddString(args.Get(i).As<Napi::String>().Utf8Value().c_str());
    }
  }

  GDALAsyncableJobNapi<std::string> job;
  job.main = [raw, aosOptions]() -> std::string {
    CPLErrorReset();
    auto psOptions = GDALInfoOptionsNew(aosOptions->List(), nullptr);
    if (psOptions == nullptr) throw CPLGetLastErrorMsg();
    char *r = GDALInfo(GDALDataset::ToHandle(raw), psOptions);
    GDALInfoOptionsFree(psOptions);
    if (r == nullptr) throw CPLGetLastErrorMsg();
    std::string s(r);
    CPLFree(r);
    return s;
  };
  job.rval = [](Napi::Env env, const std::string &s) {
    return SafeStringNapi(env, s.c_str());
  };

  return job.run(info, async, 2);
}

// =========================================================================
// translate
// =========================================================================

Napi::Value UtilsNapi::translate(const Napi::CallbackInfo &info) { return translate_do(info, false); }
Napi::Value UtilsNapi::translateAsync(const Napi::CallbackInfo &info) { return translate_do(info, true); }
Napi::Value UtilsNapi::translate_do(const Napi::CallbackInfo &info, bool async) {
  auto aosOptions = std::make_shared<CPLStringList>();

  std::string dst;
  NAPI_ARG_STR(0, "dst", dst);

  Napi::Object src;
  NAPI_ARG_OBJECT(1, "src", src);
  DatasetNapi *ds = nullptr;
  NAPI_ARG_WRAPPED(1, "src", DatasetNapi, ds);
  GDALDataset *raw = ds->get();

  Napi::Array args;
  NAPI_ARG_ARRAY_OPT(2, "args", args);
  if (info.Length() > 2 && args.Length() > 0) {
    for (uint32_t i = 0; i < args.Length(); i++) {
      aosOptions->AddString(args.Get(i).As<Napi::String>().Utf8Value().c_str());
    }
  }

  GDALAsyncableJobNapi<GDALDataset *> job;
  job.main = [raw, dst, aosOptions]() -> GDALDataset * {
    CPLErrorReset();
    auto psOptions = GDALTranslateOptionsNew(aosOptions->List(), nullptr);
    if (psOptions == nullptr) throw CPLGetLastErrorMsg();
    GDALDataset *r = GDALDataset::FromHandle(
      GDALTranslate(dst.c_str(), GDALDataset::ToHandle(raw), psOptions, nullptr));
    GDALTranslateOptionsFree(psOptions);
    if (r == nullptr) throw CPLGetLastErrorMsg();
    return r;
  };
  job.rval = [](Napi::Env env, GDALDataset *ds) {
    return DatasetNapi::New(env, ds);
  };

  return job.run(info, async, 4);
}

// =========================================================================
// vectorTranslate
// =========================================================================

Napi::Value UtilsNapi::vectorTranslate(const Napi::CallbackInfo &info) { return vectorTranslate_do(info, false); }
Napi::Value UtilsNapi::vectorTranslateAsync(const Napi::CallbackInfo &info) { return vectorTranslate_do(info, true); }
Napi::Value UtilsNapi::vectorTranslate_do(const Napi::CallbackInfo &info, bool async) {
  auto aosOptions = std::make_shared<CPLStringList>();

  if (info.Length() < 1) {
    Napi::Error::New(info.Env(), "\"dst\" must be given").ThrowAsJavaScriptException();
    return info.Env().Undefined();
  }

  std::string dst_filename("");
  DatasetNapi *dst_ds = nullptr;
  GDALDataset *dst_raw = nullptr;

  if (info[0].IsString()) {
    NAPI_ARG_STR(0, "dst", dst_filename);
  } else if (info[0].IsObject()) {
    NAPI_ARG_WRAPPED(0, "dst", DatasetNapi, dst_ds);
    dst_raw = dst_ds->get();
  } else {
    Napi::Error::New(info.Env(), "\"dst\" must be an object or a string")
      .ThrowAsJavaScriptException();
    return info.Env().Undefined();
  }

  Napi::Object src;
  NAPI_ARG_OBJECT(1, "src", src);
  DatasetNapi *ds = nullptr;
  NAPI_ARG_WRAPPED(1, "src", DatasetNapi, ds);
  GDALDataset *src_raw = ds->get();

  Napi::Array args;
  NAPI_ARG_ARRAY_OPT(2, "args", args);
  if (info.Length() > 2 && args.Length() > 0) {
    for (uint32_t i = 0; i < args.Length(); i++) {
      aosOptions->AddString(args.Get(i).As<Napi::String>().Utf8Value().c_str());
    }
  }

  GDALAsyncableJobNapi<GDALDataset *> job;
  job.main = [src_raw, dst_filename, dst_raw, aosOptions]() -> GDALDataset * {
    CPLErrorReset();
    auto psOptions = GDALVectorTranslateOptionsNew(aosOptions->List(), nullptr);
    if (psOptions == nullptr) throw CPLGetLastErrorMsg();

    auto srcH = GDALDataset::ToHandle(src_raw);
    GDALDataset *r = GDALDataset::FromHandle(
      GDALVectorTranslate(dst_filename.c_str(), GDALDataset::ToHandle(dst_raw), 1, &srcH,
                          psOptions, nullptr));

    GDALVectorTranslateOptionsFree(psOptions);
    if (r == nullptr) throw CPLGetLastErrorMsg();
    return r;
  };
  job.rval = [](Napi::Env env, GDALDataset *ds) {
    return DatasetNapi::New(env, ds);
  };

  return job.run(info, async, 4);
}

// =========================================================================
// warp
// =========================================================================

Napi::Value UtilsNapi::warp(const Napi::CallbackInfo &info) { return warp_do(info, false); }
Napi::Value UtilsNapi::warpAsync(const Napi::CallbackInfo &info) { return warp_do(info, true); }
Napi::Value UtilsNapi::warp_do(const Napi::CallbackInfo &info, bool async) {
  auto aosOptions = std::make_shared<CPLStringList>();

  std::string dst_path("");
  NAPI_ARG_OPT_STR(0, "dst_path", dst_path);

  DatasetNapi *dst_ds = nullptr;
  GDALDataset *gdal_dst_ds = nullptr;
  if (info.Length() > 1 && info[1].IsObject() && !info[1].IsNull()) {
    NAPI_ARG_WRAPPED(1, "dst_ds", DatasetNapi, dst_ds);
    gdal_dst_ds = dst_ds->get();
  }

  if (dst_path.empty() && gdal_dst_ds == nullptr) {
    Napi::Error::New(info.Env(), "Either \"dst_path\" or \"dst_ds\" must be given")
      .ThrowAsJavaScriptException();
    return info.Env().Undefined();
  }

  Napi::Array src_ds_arr;
  NAPI_ARG_ARRAY(2, "src_ds", src_ds_arr);
  if (src_ds_arr.Length() < 1) {
    Napi::Error::New(info.Env(), "\"src_ds\" must contain at least one element")
      .ThrowAsJavaScriptException();
    return info.Env().Undefined();
  }

  uint32_t src_count = src_ds_arr.Length();
  auto gdal_src_ds = std::shared_ptr<GDALDatasetH[]>(new GDALDatasetH[src_count]);
  for (uint32_t i = 0; i < src_count; i++) {
    Napi::Object obj = src_ds_arr.Get(i).As<Napi::Object>();
    DatasetNapi *_ds = nullptr;
    NAPI_ARG_WRAPPED(static_cast<int>(2), "src_ds element", DatasetNapi, _ds);
    gdal_src_ds.get()[i] = GDALDataset::ToHandle(_ds->get());
  }

  Napi::Array args;
  NAPI_ARG_ARRAY_OPT(3, "args", args);
  if (info.Length() > 3 && args.Length() > 0) {
    for (uint32_t i = 0; i < args.Length(); i++) {
      aosOptions->AddString(args.Get(i).As<Napi::String>().Utf8Value().c_str());
    }
  }

  GDALAsyncableJobNapi<GDALDataset *> job;
  job.main = [dst_path, gdal_dst_ds, src_count, gdal_src_ds, aosOptions]() -> GDALDataset * {
    CPLErrorReset();
    auto psOptions = GDALWarpAppOptionsNew(aosOptions->List(), nullptr);
    if (psOptions == nullptr) throw CPLGetLastErrorMsg();
    GDALDatasetH r = GDALWarp(
      dst_path.length() > 0 ? dst_path.c_str() : nullptr, gdal_dst_ds,
      src_count, gdal_src_ds.get(), psOptions, nullptr);
    GDALWarpAppOptionsFree(psOptions);
    if (r == nullptr) throw CPLGetLastErrorMsg();
    return GDALDataset::FromHandle(r);
  };
  job.rval = [](Napi::Env env, GDALDataset *ds) {
    return DatasetNapi::New(env, ds);
  };

  return job.run(info, async, 5);
}

// =========================================================================
// buildVRT
// =========================================================================

Napi::Value UtilsNapi::buildVRT(const Napi::CallbackInfo &info) { return buildVRT_do(info, false); }
Napi::Value UtilsNapi::buildVRTAsync(const Napi::CallbackInfo &info) { return buildVRT_do(info, true); }
Napi::Value UtilsNapi::buildVRT_do(const Napi::CallbackInfo &info, bool async) {
  auto aosOptions = std::make_shared<CPLStringList>();

  std::string dst_path;
  NAPI_ARG_STR(0, "dst_path", dst_path);

  Napi::Array src_ds_arr;
  NAPI_ARG_ARRAY(1, "src_ds", src_ds_arr);
  if (src_ds_arr.Length() < 1) {
    Napi::Error::New(info.Env(), "\"src_ds\" must contain at least one element")
      .ThrowAsJavaScriptException();
    return info.Env().Undefined();
  }

  uint32_t src_count = src_ds_arr.Length();
  std::shared_ptr<CPLStringList> aosSrcDs = nullptr;
  std::shared_ptr<GDALDatasetH[]> gdalSrcDs = nullptr;

  if (src_ds_arr.Get(uint32_t(0)).IsString()) {
    aosSrcDs = std::make_shared<CPLStringList>();
    for (uint32_t i = 0; i < src_count; i++) {
      if (!src_ds_arr.Get(i).IsString()) {
        Napi::Error::New(info.Env(), "All \"src_ds\" elements must have the same type")
          .ThrowAsJavaScriptException();
        return info.Env().Undefined();
      }
      aosSrcDs->AddString(src_ds_arr.Get(i).As<Napi::String>().Utf8Value().c_str());
    }
  } else {
    gdalSrcDs = std::shared_ptr<GDALDatasetH[]>(new GDALDatasetH[src_count]);
    for (uint32_t i = 0; i < src_count; i++) {
      Napi::Object obj = src_ds_arr.Get(i).As<Napi::Object>();
      DatasetNapi *_ds = nullptr;
      NAPI_ARG_WRAPPED(static_cast<int>(1), "src_ds element", DatasetNapi, _ds);
      gdalSrcDs.get()[i] = GDALDataset::ToHandle(_ds->get());
    }
  }

  Napi::Array args;
  NAPI_ARG_ARRAY_OPT(2, "args", args);
  if (info.Length() > 2 && args.Length() > 0) {
    for (uint32_t i = 0; i < args.Length(); i++) {
      aosOptions->AddString(args.Get(i).As<Napi::String>().Utf8Value().c_str());
    }
  }

  GDALAsyncableJobNapi<GDALDataset *> job;
  job.main = [dst_path, src_count, gdalSrcDs, aosSrcDs, aosOptions]() -> GDALDataset * {
    CPLErrorReset();
    auto psOptions = GDALBuildVRTOptionsNew(aosOptions->List(), nullptr);
    if (psOptions == nullptr) throw CPLGetLastErrorMsg();

    GDALDatasetH r = GDALBuildVRT(dst_path.c_str(), src_count, gdalSrcDs.get(),
                                  aosSrcDs.get() != nullptr ? aosSrcDs->List() : nullptr,
                                  psOptions, nullptr);

    GDALBuildVRTOptionsFree(psOptions);
    if (r == nullptr) throw CPLGetLastErrorMsg();
    return GDALDataset::FromHandle(r);
  };
  job.rval = [](Napi::Env env, GDALDataset *ds) {
    return DatasetNapi::New(env, ds);
  };

  return job.run(info, async, 4);
}

// =========================================================================
// rasterize
// =========================================================================

Napi::Value UtilsNapi::rasterize(const Napi::CallbackInfo &info) { return rasterize_do(info, false); }
Napi::Value UtilsNapi::rasterizeAsync(const Napi::CallbackInfo &info) { return rasterize_do(info, true); }
Napi::Value UtilsNapi::rasterize_do(const Napi::CallbackInfo &info, bool async) {
  auto aosOptions = std::make_shared<CPLStringList>();

  std::string dst_path("");
  DatasetNapi *dst_ds = nullptr;
  GDALDataset *dst_raw = nullptr;

  if (info.Length() > 1 && info[0].IsString()) {
    NAPI_ARG_STR(0, "dst", dst_path);
  } else if (info.Length() > 1 && info[0].IsObject()) {
    NAPI_ARG_WRAPPED(0, "dst", DatasetNapi, dst_ds);
    dst_raw = dst_ds->get();
  } else {
    Napi::Error::New(info.Env(), "dst must be given").ThrowAsJavaScriptException();
    return info.Env().Undefined();
  }

  Napi::Object src;
  NAPI_ARG_OBJECT(1, "src", src);
  DatasetNapi *ds = nullptr;
  NAPI_ARG_WRAPPED(1, "src", DatasetNapi, ds);
  GDALDataset *src_raw = ds->get();

  Napi::Array args;
  NAPI_ARG_ARRAY_OPT(2, "args", args);
  if (info.Length() > 2 && args.Length() > 0) {
    for (uint32_t i = 0; i < args.Length(); i++) {
      aosOptions->AddString(args.Get(i).As<Napi::String>().Utf8Value().c_str());
    }
  }

  GDALAsyncableJobNapi<GDALDataset *> job;
  job.main = [dst_path, dst_raw, src_raw, aosOptions]() -> GDALDataset * {
    CPLErrorReset();
    auto psOptions = GDALRasterizeOptionsNew(aosOptions->List(), nullptr);
    if (psOptions == nullptr) throw CPLGetLastErrorMsg();

    GDALDatasetH r = GDALRasterize(
      dst_path.length() > 0 ? dst_path.c_str() : nullptr, dst_raw, src_raw,
      psOptions, nullptr);

    GDALRasterizeOptionsFree(psOptions);
    if (r == nullptr) throw CPLGetLastErrorMsg();
    return GDALDataset::FromHandle(r);
  };
  job.rval = [](Napi::Env env, GDALDataset *ds) {
    return DatasetNapi::New(env, ds);
  };

  return job.run(info, async, 4);
}

// =========================================================================
// dem
// =========================================================================

Napi::Value UtilsNapi::dem(const Napi::CallbackInfo &info) { return dem_do(info, false); }
Napi::Value UtilsNapi::demAsync(const Napi::CallbackInfo &info) { return dem_do(info, true); }
Napi::Value UtilsNapi::dem_do(const Napi::CallbackInfo &info, bool async) {
  auto aosOptions = std::make_shared<CPLStringList>();

  std::string dst_path;
  NAPI_ARG_STR(0, "dst_path", dst_path);

  Napi::Object src;
  NAPI_ARG_OBJECT(1, "src", src);
  DatasetNapi *ds = nullptr;
  NAPI_ARG_WRAPPED(1, "src", DatasetNapi, ds);
  GDALDataset *raw = ds->get();

  std::string mode;
  NAPI_ARG_STR(2, "mode", mode);

  Napi::Array args;
  NAPI_ARG_ARRAY_OPT(3, "args", args);
  if (info.Length() > 3 && args.Length() > 0) {
    for (uint32_t i = 0; i < args.Length(); i++) {
      aosOptions->AddString(args.Get(i).As<Napi::String>().Utf8Value().c_str());
    }
  }

  std::string colorFilename = "";
  NAPI_ARG_OPT_STR(4, "colorFilename", colorFilename);

  GDALAsyncableJobNapi<GDALDataset *> job;
  job.main = [dst_path, mode, raw, colorFilename, aosOptions]() -> GDALDataset * {
    CPLErrorReset();
    auto psOptions = GDALDEMProcessingOptionsNew(aosOptions->List(), nullptr);
    if (psOptions == nullptr) throw CPLGetLastErrorMsg();
    GDALDataset *r = GDALDataset::FromHandle(GDALDEMProcessing(
      dst_path.c_str(), GDALDataset::ToHandle(raw), mode.c_str(),
      colorFilename.size() > 0 ? colorFilename.c_str() : nullptr, psOptions, nullptr));

    GDALDEMProcessingOptionsFree(psOptions);
    if (r == nullptr) throw CPLGetLastErrorMsg();
    return r;
  };
  job.rval = [](Napi::Env env, GDALDataset *ds) {
    return DatasetNapi::New(env, ds);
  };

  return job.run(info, async, 6);
}

// =========================================================================
// reprojectImage / suggestedWarpOutput (TODO)
// =========================================================================

Napi::Value UtilsNapi::reprojectImage(const Napi::CallbackInfo &info) {
  Napi::Error::New(info.Env(), "reprojectImage not yet ported to N-API")
    .ThrowAsJavaScriptException();
  return info.Env().Undefined();
}

Napi::Value UtilsNapi::suggestedWarpOutput(const Napi::CallbackInfo &info) {
  Napi::Error::New(info.Env(), "suggestedWarpOutput not yet ported to N-API")
    .ThrowAsJavaScriptException();
  return info.Env().Undefined();
}

// =========================================================================
// WarperNapi / AlgorithmsNapi (placeholders)
// =========================================================================

namespace WarperNapi {
Napi::Object Init(Napi::Env env, Napi::Object exports) {
  auto obj = Napi::Object::New(env);
  // TODO: port reprojectImage, suggestedWarpOutput
  exports.Set("Warper", obj);
  return exports;
}
} // namespace WarperNapi

namespace AlgorithmsNapi {
Napi::Object Init(Napi::Env env, Napi::Object exports) {
  auto obj = Napi::Object::New(env);
  // TODO: port fillNodata, contourGenerate, sieveFilter, checksumImage, polygonize
  exports.Set("Algorithms", obj);
  return exports;
}
} // namespace AlgorithmsNapi

} // namespace node_gdal
