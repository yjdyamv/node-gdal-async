#include "collections_napi.hpp"
#include "../gdal_dataset_napi.hpp"
#include "../gdal_rasterband_napi.hpp"
#include "../gdal_layer_napi.hpp"
#include "../gdal_feature_napi.hpp"

namespace node_gdal {

// ===================== DatasetBandsNapi =====================
Napi::FunctionReference DatasetBandsNapi::constructor;

Napi::Object DatasetBandsNapi::Init(Napi::Env env, Napi::Object exports) {
  Napi::Function func = DefineClass(
    env, "DatasetBands",
    {
      InstanceMethod("toString", &DatasetBandsNapi::toString),
      InstanceMethod("count", &DatasetBandsNapi::count),
      InstanceMethod("countAsync", &DatasetBandsNapi::countAsync),
      InstanceMethod("get", &DatasetBandsNapi::get),
      InstanceMethod("getAsync", &DatasetBandsNapi::getAsync),
    });
  constructor = Napi::Persistent(func); constructor.SuppressDestruct();
  exports.Set("DatasetBands", func); return exports;
}

DatasetBandsNapi::DatasetBandsNapi(const Napi::CallbackInfo &info)
  : Napi::ObjectWrap<DatasetBandsNapi>(info) {
  if (!info.IsConstructCall()) {
    Napi::Error::New(info.Env(), "Cannot call constructor as function, use 'new' keyword")
      .ThrowAsJavaScriptException();
  }
}

Napi::Value DatasetBandsNapi::toString(const Napi::CallbackInfo &info) {
  return Napi::String::New(info.Env(), "DatasetBands");
}

GDAL_ASYNCABLE_DEFINE_NAPI(DatasetBandsNapi, count) {
  // Use parent from private property (set during construction)
  auto priv = info.This().As<Napi::Object>().Get("_parent");
  if (priv.IsObject()) {
    auto *ds = DatasetNapi::Unwrap(priv.As<Napi::Object>());
    if (ds && ds->isAlive()) {
      GDALAsyncableJobNapi<int> job;
      GDALDataset *raw = ds->get();
      job.main = [raw]() { return raw->GetRasterCount(); };
      job.rval = [](Napi::Env env, int c) { return Napi::Number::New(env, c); };
      return job.run(info, async, 0);
    }
  }
  return info.Env().Null();
}

GDAL_ASYNCABLE_DEFINE_NAPI(DatasetBandsNapi, get) {
  auto priv = info.This().As<Napi::Object>().Get("_parent");
  if (priv.IsObject()) {
    auto *ds = DatasetNapi::Unwrap(priv.As<Napi::Object>());
    if (ds && ds->isAlive()) {
      int band_id;
      NAPI_ARG_INT(0, "band id", band_id);
      GDALDataset *raw = ds->get();
      GDALAsyncableJobNapi<GDALRasterBand *> job;
      job.main = [raw, band_id]() -> GDALRasterBand * {
        GDALRasterBand *band = raw->GetRasterBand(band_id);
        if (!band) throw CPLGetLastErrorMsg();
        return band;
      };
      job.rval = [](Napi::Env env, GDALRasterBand *b) {
        return RasterBandNapi::New(env, b);
      };
      return job.run(info, async, 1);
    }
  }
  return info.Env().Null();
}

// ===================== DatasetLayersNapi =====================
Napi::FunctionReference DatasetLayersNapi::constructor;

Napi::Object DatasetLayersNapi::Init(Napi::Env env, Napi::Object exports) {
  Napi::Function func = DefineClass(
    env, "DatasetLayers",
    {
      InstanceMethod("toString", &DatasetLayersNapi::toString),
      InstanceMethod("get", &DatasetLayersNapi::get),
      InstanceMethod("count", &DatasetLayersNapi::count),
      InstanceMethod("countAsync", &DatasetLayersNapi::countAsync),
    });
  constructor = Napi::Persistent(func); constructor.SuppressDestruct();
  exports.Set("DatasetLayers", func); return exports;
}

DatasetLayersNapi::DatasetLayersNapi(const Napi::CallbackInfo &info)
  : Napi::ObjectWrap<DatasetLayersNapi>(info) {
  if (!info.IsConstructCall()) {
    Napi::Error::New(info.Env(), "Cannot call constructor as function, use 'new' keyword")
      .ThrowAsJavaScriptException();
  }
}

Napi::Value DatasetLayersNapi::toString(const Napi::CallbackInfo &info) {
  return Napi::String::New(info.Env(), "DatasetLayers");
}

GDAL_ASYNCABLE_DEFINE_NAPI(DatasetLayersNapi, count) {
  auto priv = info.This().As<Napi::Object>().Get("_parent");
  if (priv.IsObject()) {
    auto *ds = DatasetNapi::Unwrap(priv.As<Napi::Object>());
    if (ds && ds->isAlive()) {
      GDALDataset *raw = ds->get();
      GDALAsyncableJobNapi<int> job;
      job.main = [raw]() { return raw->GetLayerCount(); };
      job.rval = [](Napi::Env env, int c) { return Napi::Number::New(env, c); };
      return job.run(info, async, 0);
    }
  }
  return info.Env().Null();
}

Napi::Value DatasetLayersNapi::get(const Napi::CallbackInfo &info) {
  auto priv = info.This().As<Napi::Object>().Get("_parent");
  if (priv.IsObject()) {
    auto *ds = DatasetNapi::Unwrap(priv.As<Napi::Object>());
    if (ds && ds->isAlive()) {
      int layer_id;
      NAPI_ARG_INT(0, "layer id", layer_id);
      GDALDataset *raw = ds->get();
      OGRLayer *layer = raw->GetLayer(layer_id);
      if (!layer) { Napi::Error::New(info.Env(), "Invalid layer index").ThrowAsJavaScriptException(); return info.Env().Undefined(); }
      return LayerNapi::New(info.Env(), layer);
    }
  }
  return info.Env().Null();
}

// ===================== LayerFeaturesNapi =====================
Napi::FunctionReference LayerFeaturesNapi::constructor;

Napi::Object LayerFeaturesNapi::Init(Napi::Env env, Napi::Object exports) {
  Napi::Function func = DefineClass(
    env, "LayerFeatures",
    {
      InstanceMethod("toString", &LayerFeaturesNapi::toString),
      InstanceMethod("count", &LayerFeaturesNapi::count),
      InstanceMethod("countAsync", &LayerFeaturesNapi::countAsync),
      InstanceMethod("next", &LayerFeaturesNapi::next),
      InstanceMethod("nextAsync", &LayerFeaturesNapi::nextAsync),
    });
  constructor = Napi::Persistent(func); constructor.SuppressDestruct();
  exports.Set("LayerFeatures", func); return exports;
}

LayerFeaturesNapi::LayerFeaturesNapi(const Napi::CallbackInfo &info)
  : Napi::ObjectWrap<LayerFeaturesNapi>(info) {
  if (!info.IsConstructCall()) {
    Napi::Error::New(info.Env(), "Cannot call constructor as function, use 'new' keyword")
      .ThrowAsJavaScriptException();
  }
}

Napi::Value LayerFeaturesNapi::toString(const Napi::CallbackInfo &info) {
  return Napi::String::New(info.Env(), "LayerFeatures");
}

GDAL_ASYNCABLE_DEFINE_NAPI(LayerFeaturesNapi, count) {
  auto priv = info.This().As<Napi::Object>().Get("_parent");
  if (priv.IsObject()) {
    auto *layer = LayerNapi::Unwrap(priv.As<Napi::Object>());
    if (layer && layer->isAlive()) {
      GDALAsyncableJobNapi<GIntBig> job;
      OGRLayer *raw = layer->get();
      job.main = [raw]() { return raw->GetFeatureCount(1); };
      job.rval = [](Napi::Env env, GIntBig c) {
        return Napi::Number::New(env, static_cast<double>(c));
      };
      return job.run(info, async, 0);
    }
  }
  return info.Env().Null();
}

GDAL_ASYNCABLE_DEFINE_NAPI(LayerFeaturesNapi, next) {
  auto priv = info.This().As<Napi::Object>().Get("_parent");
  if (priv.IsObject()) {
    auto *layer = LayerNapi::Unwrap(priv.As<Napi::Object>());
    if (layer && layer->isAlive()) {
      GDALAsyncableJobNapi<OGRFeature *> job;
      OGRLayer *raw = layer->get();
      job.main = [raw]() -> OGRFeature * { return raw->GetNextFeature(); };
      job.rval = [](Napi::Env env, OGRFeature *f) -> Napi::Value {
        if (!f) return env.Null();
        return FeatureNapi::New(env, f);
      };
      return job.run(info, async, 0);
    }
  }
  return info.Env().Null();
}

} // namespace node_gdal
