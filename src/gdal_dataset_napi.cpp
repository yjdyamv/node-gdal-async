#include <memory>
#include "gdal_dataset_napi.hpp"
#include "gdal_driver_napi.hpp"
#include "gdal_spatial_reference_napi.hpp"
#include "gdal_layer_napi.hpp"
#include "utils/napi_object_store.hpp"
#include "collections/collections_napi.hpp"

namespace node_gdal {

Napi::FunctionReference DatasetNapi::constructor;

Napi::Object DatasetNapi::Init(Napi::Env env, Napi::Object exports) {
  Napi::Function func = DefineClass(
    env,
    "Dataset",
    {
      InstanceMethod("toString", &DatasetNapi::toString),
      InstanceMethod("close", &DatasetNapi::close),
      InstanceMethod("flush", &DatasetNapi::flush),
      InstanceMethod("flushAsync", &DatasetNapi::flushAsync),
      InstanceMethod("getMetadata", &DatasetNapi::getMetadata),
      InstanceMethod("getMetadataAsync", &DatasetNapi::getMetadataAsync),
      InstanceMethod("setMetadata", &DatasetNapi::setMetadata),
      InstanceMethod("setMetadataAsync", &DatasetNapi::setMetadataAsync),
      InstanceMethod("buildOverviews", &DatasetNapi::buildOverviews),
      InstanceMethod("buildOverviewsAsync", &DatasetNapi::buildOverviewsAsync),
      InstanceMethod("executeSQL", &DatasetNapi::executeSQL),
      InstanceMethod("executeSQLAsync", &DatasetNapi::executeSQLAsync),
      InstanceMethod("getFileList", &DatasetNapi::getFileList),
      InstanceMethod("testCapability", &DatasetNapi::testCapability),
      InstanceAccessor<&DatasetNapi::descriptionGetter>("description"),
      InstanceAccessor<&DatasetNapi::driverGetter>("driver"),
      InstanceAccessor<&DatasetNapi::threadSafeGetter>("threadSafe"),
      InstanceAccessor<&DatasetNapi::rasterSizeGetter>("rasterSize"),
      InstanceAccessor<&DatasetNapi::rasterSizeGetterAsync>("rasterSizeAsync"),
      InstanceAccessor<&DatasetNapi::geoTransformGetter>("geoTransform"),
      InstanceAccessor<&DatasetNapi::geoTransformGetterAsync>("geoTransformAsync"),
      InstanceAccessor<&DatasetNapi::srsGetter>("srs"),
      InstanceAccessor<&DatasetNapi::srsGetterAsync>("srsAsync"),
      InstanceAccessor<&DatasetNapi::bandsGetter>("bands"),
      InstanceAccessor<&DatasetNapi::layersGetter>("layers"),
    });

  constructor = Napi::Persistent(func);
  constructor.SuppressDestruct();

  exports.Set("Dataset", func);
  return exports;
}

DatasetNapi::DatasetNapi(const Napi::CallbackInfo &info)
  : Napi::ObjectWrap<DatasetNapi>(info), this_dataset(nullptr) {

  if (!info.IsConstructCall()) {
    Napi::Error::New(info.Env(), "Cannot call constructor as function, use 'new' keyword")
      .ThrowAsJavaScriptException();
    return;
  }

  if (info.Length() > 0 && info[0].IsExternal()) {
    this_dataset = info[0].As<Napi::External<GDALDataset>>().Data();
  }
}

DatasetNapi::~DatasetNapi() {
  if (this_dataset && owned_) {
    GDALClose(this_dataset);
  }
  this_dataset = nullptr;
}

Napi::Value DatasetNapi::New(Napi::Env env, GDALDataset *ds, bool owned) {
  Napi::EscapableHandleScope scope(env);
  if (!ds) return scope.Escape(env.Null());
  // Return existing wrapper if already registered
  if (napi_obj_store_has<GDALDataset *>(ds)) {
    Napi::Object existing = napi_obj_store_get<GDALDataset *>(env, ds);
    if (!existing.IsEmpty()) return scope.Escape(existing);
  }
  Napi::Object obj = constructor.New({Napi::External<GDALDataset>::New(env, ds)});
  DatasetNapi *w = DatasetNapi::Unwrap(obj);
  if (w) w->owned_ = owned;
  napi_obj_store_add<GDALDataset *>(ds, obj);
  return scope.Escape(obj);
}

// ---------------------------------------------------------------------------
// Simple methods
// ---------------------------------------------------------------------------

Napi::Value DatasetNapi::toString(const Napi::CallbackInfo &info) {
  return Napi::String::New(info.Env(), "Dataset");
}

Napi::Value DatasetNapi::close(const Napi::CallbackInfo &info) {
  NAPI_UNWRAP_THIS(DatasetNapi, ds);
  if (ds->this_dataset && ds->owned_) {
    GDALClose(ds->this_dataset);
  }
  ds->this_dataset = nullptr;
  return info.Env().Undefined();
}

Napi::Value DatasetNapi::getFileList(const Napi::CallbackInfo &info) {
  NAPI_UNWRAP_THIS(DatasetNapi, ds);
  char **files = ds->this_dataset->GetFileList();
  Napi::Array result = Napi::Array::New(info.Env());
  if (files) {
    for (int i = 0; files[i]; i++) {
      result.Set(i, Napi::String::New(info.Env(), files[i]));
    }
    CSLDestroy(files);
  }
  return result;
}

Napi::Value DatasetNapi::testCapability(const Napi::CallbackInfo &info) {
  NAPI_UNWRAP_THIS(DatasetNapi, ds);
  std::string cap;
  NAPI_ARG_STR(0, "capability", cap);
  return Napi::Boolean::New(info.Env(), ds->this_dataset->TestCapability(cap.c_str()));
}

Napi::Value DatasetNapi::descriptionGetter(const Napi::CallbackInfo &info) {
  NAPI_UNWRAP_THIS(DatasetNapi, ds);
  char **files = ds->this_dataset->GetFileList();
  std::string desc;
  if (files && files[0]) {
    desc = CPLGetBasename(files[0]);
    CSLDestroy(files);
  }
  return Napi::String::New(info.Env(), desc);
}

Napi::Value DatasetNapi::driverGetter(const Napi::CallbackInfo &info) {
  NAPI_UNWRAP_THIS(DatasetNapi, ds);
  GDALDriver *driver = ds->this_dataset->GetDriver();
  if (!driver) return info.Env().Null();
  return DriverNapi::New(info.Env(), driver);
}

Napi::Value DatasetNapi::threadSafeGetter(const Napi::CallbackInfo &info) {
  // GDAL datasets are not thread-safe, this returns 1 (FALSE in GDAL terms)
  return Napi::Number::New(info.Env(), 1);
}

// ---------------------------------------------------------------------------
// getMetadata (async)
// ---------------------------------------------------------------------------

GDAL_ASYNCABLE_DEFINE_NAPI(DatasetNapi, getMetadata) {
  NAPI_UNWRAP_THIS(DatasetNapi, ds);
  std::string domain;
  NAPI_ARG_OPT_STR(0, "domain", domain);

  GDALDataset *raw = ds->this_dataset;

  GDALAsyncableJobNapi<char **> job;
  job.main = [raw, domain]() {
    return raw->GetMetadata(domain.empty() ? nullptr : domain.c_str());
  };
  job.rval = [](Napi::Env env, char **md) -> Napi::Value {
    return MajorObjectNapiGetMetadata(env, md);
  };

  return job.run(info, async, 1);
}

// ---------------------------------------------------------------------------
// flush (async)
// ---------------------------------------------------------------------------

GDAL_ASYNCABLE_DEFINE_NAPI(DatasetNapi, flush) {
  NAPI_UNWRAP_THIS(DatasetNapi, ds);
  GDALDataset *raw = ds->this_dataset;

  GDALAsyncableJobNapi<int> job;
  job.main = [raw]() {
    raw->FlushCache();
    return 0;
  };
  job.rval = [](Napi::Env env, int) -> Napi::Value { return env.Undefined(); };

  return job.run(info, async, 0);
}

// ---------------------------------------------------------------------------
// rasterSize getter (async)
// ---------------------------------------------------------------------------

GDAL_ASYNCABLE_GETTER_DEFINE_NAPI(DatasetNapi, rasterSizeGetter) {
  NAPI_UNWRAP_THIS(DatasetNapi, ds);
  GDALDataset *raw = ds->this_dataset;

  struct RasterSize {
    int x, y;
    bool null;
  };

  GDALAsyncableJobNapi<RasterSize> job;
  job.main = [raw]() -> RasterSize {
    RasterSize sz;
    if (!raw->GetDriver() || !raw->GetDriver()->GetMetadataItem(GDAL_DCAP_RASTER)) {
      sz.null = true;
      return sz;
    }
    sz.x = raw->GetRasterXSize();
    sz.y = raw->GetRasterYSize();
    sz.null = false;
    return sz;
  };
  job.rval = [](Napi::Env env, RasterSize sz) -> Napi::Value {
    if (sz.null) return env.Null();
    Napi::Object result = Napi::Object::New(env);
    result.Set("x", Napi::Number::New(env, sz.x));
    result.Set("y", Napi::Number::New(env, sz.y));
    return result;
  };

  return job.run(info, async, 0);
}

// ---------------------------------------------------------------------------
// srs getter (async)
// ---------------------------------------------------------------------------

GDAL_ASYNCABLE_GETTER_DEFINE_NAPI(DatasetNapi, srsGetter) {
  NAPI_UNWRAP_THIS(DatasetNapi, ds);
  GDALDataset *raw = ds->this_dataset;

  GDALAsyncableJobNapi<OGRSpatialReference *> job;
  job.main = [raw]() -> OGRSpatialReference * {
    const char *wkt = raw->GetProjectionRef();
    if (!wkt || *wkt == '\0') return nullptr;
    OGRSpatialReference *srs = new OGRSpatialReference();
    if (srs->importFromWkt(&wkt) != OGRERR_NONE) {
      delete srs;
      throw "Failed to import projection";
    }
    return srs;
  };
  job.rval = [](Napi::Env env, OGRSpatialReference *srs) -> Napi::Value {
    if (!srs) return env.Null();
    return SpatialReferenceNapi::New(env, srs, true);
  };

  return job.run(info, async, 0);
}

// ---------------------------------------------------------------------------
// geoTransform getter (async)
// ---------------------------------------------------------------------------

GDAL_ASYNCABLE_GETTER_DEFINE_NAPI(DatasetNapi, geoTransformGetter) {
  NAPI_UNWRAP_THIS(DatasetNapi, ds);
  GDALDataset *raw = ds->this_dataset;

  struct GeoTransformResult {
    double coefs[6];
    bool valid;
  };

  GDALAsyncableJobNapi<GeoTransformResult> job;
  job.main = [raw]() -> GeoTransformResult {
    GeoTransformResult gt;
    gt.valid = (raw->GetGeoTransform(gt.coefs) == CE_None);
    return gt;
  };
  job.rval = [](Napi::Env env, GeoTransformResult gt) -> Napi::Value {
    if (!gt.valid) return env.Null();
    Napi::Array result = Napi::Array::New(env, 6);
    for (int i = 0; i < 6; i++) {
      result.Set(i, Napi::Number::New(env, gt.coefs[i]));
    }
    return result;
  };

  return job.run(info, async, 0);
}

// ---------------------------------------------------------------------------
// Setters
// ---------------------------------------------------------------------------

void DatasetNapi::srsSetter(const Napi::CallbackInfo &info, const Napi::Value &value) {
  NAPI_UNWRAP_THIS_VOID(DatasetNapi, ds);
  if (value.IsNull() || value.IsUndefined()) {
    ds->this_dataset->SetProjection("");
    return;
  }
  if (value.IsObject() && value.As<Napi::Object>().InstanceOf(SpatialReferenceNapi::constructor.Value())) {
    SpatialReferenceNapi *srs = SpatialReferenceNapi::Unwrap(value.As<Napi::Object>());
    if (!srs || !srs->isAlive()) {
      Napi::Error::New(info.Env(), "SpatialReferenceNapi object has already been destroyed")
        .ThrowAsJavaScriptException();
      return;
    }
    char *wkt;
    if (srs->get()->exportToWkt(&wkt) != OGRERR_NONE) {
      Napi::Error::New(info.Env(), "Failed to export SRS to WKT").ThrowAsJavaScriptException();
      return;
    }
    ds->this_dataset->SetProjection(wkt);
    CPLFree(wkt);
    return;
  }
  Napi::Error::New(info.Env(), "srs must be a SpatialReferenceNapi object")
    .ThrowAsJavaScriptException();
}

void DatasetNapi::geoTransformSetter(const Napi::CallbackInfo &info, const Napi::Value &value) {
  NAPI_UNWRAP_THIS_VOID(DatasetNapi, ds);
  if (!value.IsArray()) {
    Napi::Error::New(info.Env(), "geoTransform must be an array of 6 numbers")
      .ThrowAsJavaScriptException();
    return;
  }
  Napi::Array arr = value.As<Napi::Array>();
  if (arr.Length() < 6) {
    Napi::Error::New(info.Env(), "geoTransform must have 6 elements")
      .ThrowAsJavaScriptException();
    return;
  }
  double coefs[6];
  for (uint32_t i = 0; i < 6; i++) {
    coefs[i] = arr.Get(i).As<Napi::Number>().DoubleValue();
  }
  CPLErr err = ds->this_dataset->SetGeoTransform(coefs);
  if (err != CE_None) {
    Napi::Error::New(info.Env(), CPLGetLastErrorMsg()).ThrowAsJavaScriptException();
  }
}

Napi::Value DatasetNapi::bandsGetter(const Napi::CallbackInfo &info) {
  NAPI_UNWRAP_THIS(DatasetNapi, ds);
  Napi::Object thiz = info.This().As<Napi::Object>();
  if (thiz.Has("__bands")) { Napi::Value c = thiz.Get("__bands"); if (!c.IsNull() && !c.IsUndefined()) return c; }
  Napi::Object bands = DatasetBandsNapi::constructor.New({Napi::External<GDALDataset>::New(info.Env(), ds->this_dataset)});
  bands.Set("_parent", thiz);  // parent reference for collection operations
  thiz.Set("__bands", bands); return bands;
}

Napi::Value DatasetNapi::layersGetter(const Napi::CallbackInfo &info) {
  NAPI_UNWRAP_THIS(DatasetNapi, ds);
  Napi::Object thiz = info.This().As<Napi::Object>();
  if (thiz.Has("__layers")) { Napi::Value c = thiz.Get("__layers"); if (!c.IsNull() && !c.IsUndefined()) return c; }
  Napi::Object layers = DatasetLayersNapi::constructor.New({Napi::External<GDALDataset>::New(info.Env(), ds->this_dataset)});
  layers.Set("_parent", thiz);  // parent reference for collection operations
  thiz.Set("__layers", layers); return layers;
}

// =========================================================================
// buildOverviews
// =========================================================================
GDAL_ASYNCABLE_DEFINE_NAPI(DatasetNapi, buildOverviews) {
  NAPI_UNWRAP_THIS(DatasetNapi, ds);
  std::string resampling;
  NAPI_ARG_STR(0, "resampling", resampling);
  Napi::Array overview_arr;
  NAPI_ARG_ARRAY_OPT(1, "overviews", overview_arr);
  std::vector<int> overviews;
  if (info.Length() > 1 && overview_arr.Length() > 0) {
    for (uint32_t i = 0; i < overview_arr.Length(); i++)
      overviews.push_back(overview_arr.Get(i).As<Napi::Number>().Int32Value());
  }
  Napi::Array bands_arr;
  NAPI_ARG_ARRAY_OPT(2, "bands", bands_arr);
  std::vector<int> band_list;
  if (info.Length() > 2 && bands_arr.Length() > 0) {
    for (uint32_t i = 0; i < bands_arr.Length(); i++)
      band_list.push_back(bands_arr.Get(i).As<Napi::Number>().Int32Value());
  }

  GDALDataset *raw = ds->this_dataset;
  GDALAsyncableJobNapi<CPLErr> job;
  if (info.Length() > 3 && info[3].IsObject()) {
    NAPI_CB_FROM_OBJ_OPT(info[3].As<Napi::Object>(), "progress_cb", job.progress_cb_);
  }
  job.main = [raw, resampling, overviews, band_list, &job]() -> CPLErr {
    CPLErrorReset();
    CPLErr err = raw->BuildOverviews(resampling.c_str(),
      (int)overviews.size(), overviews.empty() ? nullptr : overviews.data(),
      (int)band_list.size(), band_list.empty() ? nullptr : band_list.data(),
      job.progressFunc(), job.progressArg());
    if (err != CE_None) throw CPLGetLastErrorMsg();
    return err;
  };
  job.rval = [](Napi::Env env, CPLErr) { return env.Undefined(); };
  return job.run(info, async, 4);
}

// =========================================================================
// executeSQL
// =========================================================================
GDAL_ASYNCABLE_DEFINE_NAPI(DatasetNapi, executeSQL) {
  NAPI_UNWRAP_THIS(DatasetNapi, ds);
  std::string sql;
  NAPI_ARG_STR(0, "sql", sql);
  std::string dialect;
  NAPI_ARG_OPT_STR(1, "dialect", dialect);
  GDALDataset *raw = ds->this_dataset;
  GDALAsyncableJobNapi<OGRLayer *> job;
  job.main = [raw, sql, dialect]() -> OGRLayer * {
    CPLErrorReset();
    OGRLayer *layer = raw->ExecuteSQL(sql.c_str(), nullptr, dialect.empty() ? nullptr : dialect.c_str());
    if (!layer) throw CPLGetLastErrorMsg();
    return layer;
  };
  job.rval = [](Napi::Env env, OGRLayer *l) { return LayerNapi::New(env, l); };
  return job.run(info, async, 2);
}

// =========================================================================
// setMetadata
// =========================================================================
GDAL_ASYNCABLE_DEFINE_NAPI(DatasetNapi, setMetadata) {
  NAPI_UNWRAP_THIS(DatasetNapi, ds);
  Napi::Object meta;
  NAPI_ARG_OBJECT(0, "metadata", meta);
  std::string domain;
  NAPI_ARG_OPT_STR(1, "domain", domain);
  GDALDataset *raw = ds->this_dataset;
  GDALAsyncableJobNapi<CPLErr> job;
  Napi::Object m = meta;
  std::string d = domain;
  job.main = [raw, m, d]() -> CPLErr {
    Napi::Array keys = m.GetPropertyNames();
    std::vector<std::string> strs; std::vector<char *> list;
    for (uint32_t i = 0; i < keys.Length(); i++) {
      std::string k = keys.Get(i).As<Napi::String>().Utf8Value();
      std::string v = m.Get(k).As<Napi::String>().Utf8Value();
      strs.push_back(k + "=" + v); list.push_back((char *)strs.back().c_str());
    }
    list.push_back(nullptr);
    CPLErr err = raw->SetMetadata(list.data(), d.empty() ? nullptr : d.c_str());
    if (err != CE_None) throw CPLGetLastErrorMsg();
    return err;
  };
  job.rval = [](Napi::Env, CPLErr) { return Napi::Value(); };
  return job.run(info, async, 2);
}

} // namespace node_gdal
