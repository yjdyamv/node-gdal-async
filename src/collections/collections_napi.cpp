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
      InstanceMethod("create", &DatasetBandsNapi::create),
      InstanceMethod("createAsync", &DatasetBandsNapi::createAsync),
      InstanceMethod("getEnvelope", &DatasetBandsNapi::getEnvelope),
    });
  constructor = Napi::Persistent(func); constructor.SuppressDestruct();
  exports.Set("DatasetBands", func); return exports;
}

DatasetBandsNapi::DatasetBandsNapi(const Napi::CallbackInfo &info)
  : Napi::ObjectWrap<DatasetBandsNapi>(info) {
  if (!info.IsConstructCall()) {
    Napi::Error::New(info.Env(), "Cannot call constructor as function, use 'new' keyword")
      .ThrowAsJavaScriptException(); return;
  }
  if (info.Length() == 0 || !info[0].IsExternal()) {
    Napi::Error::New(info.Env(), "Cannot create directly").ThrowAsJavaScriptException(); return;
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

GDAL_ASYNCABLE_DEFINE_NAPI(DatasetBandsNapi, create) {
  auto priv = info.This().As<Napi::Object>().Get("_parent");
  if (priv.IsObject()) {
    auto *ds = DatasetNapi::Unwrap(priv.As<Napi::Object>());
    if (ds && ds->isAlive()) {
      int type = GDT_Byte;
      if (info.Length() > 0 && info[0].IsNumber()) type = info[0].As<Napi::Number>().Int32Value();
      GDALDataset *raw = ds->get();
      GDALAsyncableJobNapi<GDALRasterBand *> job;
      job.main = [raw, type]() -> GDALRasterBand * {
        CPLErr err = raw->AddBand(static_cast<GDALDataType>(type), nullptr);
        if (err != CE_None) throw CPLGetLastErrorMsg();
        return raw->GetRasterBand(raw->GetRasterCount());
      };
      job.rval = [](Napi::Env env, GDALRasterBand *b) { return RasterBandNapi::New(env, b); };
      return job.run(info, async, 1);
    }
  }
  return info.Env().Null();
}

Napi::Value DatasetBandsNapi::getEnvelope(const Napi::CallbackInfo &info) {
  auto priv = info.This().As<Napi::Object>().Get("_parent");
  if (priv.IsObject()) {
    auto *ds = DatasetNapi::Unwrap(priv.As<Napi::Object>());
    if (ds && ds->isAlive()) {
      GDALDataset *raw = ds->get();
      double gt[6]; if (raw->GetGeoTransform(gt) != CE_None) return info.Env().Null();
      double minX = gt[0], maxY = gt[3];
      double maxX = minX + gt[1] * raw->GetRasterXSize();
      double minY = maxY + gt[5] * raw->GetRasterYSize();
      Napi::Object result = Napi::Object::New(info.Env());
      result.Set("minX", Napi::Number::New(info.Env(), minX));
      result.Set("maxX", Napi::Number::New(info.Env(), maxX));
      result.Set("minY", Napi::Number::New(info.Env(), minY));
      result.Set("maxY", Napi::Number::New(info.Env(), maxY));
      return result;
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
      InstanceMethod("getAsync", &DatasetLayersNapi::getAsync),
      InstanceMethod("count", &DatasetLayersNapi::count),
      InstanceMethod("countAsync", &DatasetLayersNapi::countAsync),
      InstanceMethod("create", &DatasetLayersNapi::create),
      InstanceMethod("createAsync", &DatasetLayersNapi::createAsync),
    });
  constructor = Napi::Persistent(func); constructor.SuppressDestruct();
  exports.Set("DatasetLayers", func); return exports;
}

DatasetLayersNapi::DatasetLayersNapi(const Napi::CallbackInfo &info)
  : Napi::ObjectWrap<DatasetLayersNapi>(info) {
  if (!info.IsConstructCall()) {
    Napi::Error::New(info.Env(), "Cannot call constructor as function, use 'new' keyword")
      .ThrowAsJavaScriptException(); return;
  }
  if (info.Length() == 0 || !info[0].IsExternal()) {
    Napi::Error::New(info.Env(), "Cannot create directly").ThrowAsJavaScriptException(); return;
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

GDAL_ASYNCABLE_DEFINE_NAPI(DatasetLayersNapi, get) {
  auto priv = info.This().As<Napi::Object>().Get("_parent");
  if (priv.IsObject()) {
    auto *ds = DatasetNapi::Unwrap(priv.As<Napi::Object>());
    if (ds && ds->isAlive()) {
      int layer_id;
      NAPI_ARG_INT(0, "layer id", layer_id);
      GDALDataset *raw = ds->get();
      GDALAsyncableJobNapi<OGRLayer *> job;
      job.main = [raw, layer_id]() -> OGRLayer * {
        OGRLayer *layer = raw->GetLayer(layer_id);
        if (!layer) throw "Invalid layer index";
        return layer;
      };
      job.rval = [priv](Napi::Env env, OGRLayer *l) {
        Napi::Value result = LayerNapi::New(env, l);
        if (result.IsObject()) {
          result.As<Napi::Object>().Set("_ds", priv);
          auto *dsWrap = DatasetNapi::Unwrap(priv.As<Napi::Object>());
          if (dsWrap) dsWrap->addLayerRef(result.As<Napi::Object>());
        }
        return result;
      };
      return job.run(info, async, 1);
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
      InstanceMethod("first", &LayerFeaturesNapi::first),
      InstanceMethod("firstAsync", &LayerFeaturesNapi::firstAsync),
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
      .ThrowAsJavaScriptException(); return;
  }
  if (info.Length() == 0 || !info[0].IsExternal()) {
    Napi::Error::New(info.Env(), "Cannot create directly").ThrowAsJavaScriptException(); return;
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

GDAL_ASYNCABLE_DEFINE_NAPI(DatasetLayersNapi, create) {
  auto priv = info.This().As<Napi::Object>().Get("_parent");
  if (priv.IsObject()) {
    auto *ds = DatasetNapi::Unwrap(priv.As<Napi::Object>());
    if (ds && ds->isAlive()) {
      std::string name;
      NAPI_ARG_STR(0, "layer name", name);
      GDALDataset *raw = ds->get();
      GDALAsyncableJobNapi<OGRLayer *> job;
      job.main = [raw, name]() -> OGRLayer * {
        OGRLayer *layer = raw->CreateLayer(name.c_str());
        if (!layer) throw CPLGetLastErrorMsg();
        return layer;
      };
      job.rval = [priv](Napi::Env env, OGRLayer *l) {
        Napi::Value result = LayerNapi::New(env, l);
        if (result.IsObject()) result.As<Napi::Object>().Set("_ds", priv);
        return result;
      };
      return job.run(info, async, 1);
    }
  }
  return info.Env().Null();
}

// Helper: check if parent dataset of a layer features object is still alive
static bool isLayerParentDsAlive(const Napi::CallbackInfo &info) {
  auto priv = info.This().As<Napi::Object>().Get("_parent");
  if (!priv.IsObject()) return false;
  auto *layer = LayerNapi::Unwrap(priv.As<Napi::Object>());
  if (!layer || !layer->isAlive()) return false;
  Napi::Value dsVal = priv.As<Napi::Object>().Get("_ds");
  if (dsVal.IsObject()) {
    auto *ds = DatasetNapi::Unwrap(dsVal.As<Napi::Object>());
    if (!ds || !ds->isAlive()) return false;
  }
  return true;
}

GDAL_ASYNCABLE_DEFINE_NAPI(LayerFeaturesNapi, next) {
  if (!isLayerParentDsAlive(info)) {
    Napi::Error::New(info.Env(), "Layer object has already been destroyed").ThrowAsJavaScriptException();
    return info.Env().Undefined();
  }
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

GDAL_ASYNCABLE_DEFINE_NAPI(LayerFeaturesNapi, first) {
  if (!isLayerParentDsAlive(info)) {
    Napi::Error::New(info.Env(), "Layer object has already been destroyed").ThrowAsJavaScriptException();
    return info.Env().Undefined();
  }
  auto priv = info.This().As<Napi::Object>().Get("_parent");
  auto *layer = LayerNapi::Unwrap(priv.As<Napi::Object>());
  GDALAsyncableJobNapi<OGRFeature *> job;
  OGRLayer *raw = layer->get();
  job.main = [raw]() -> OGRFeature * {
    raw->ResetReading();
    return raw->GetNextFeature();
  };
  job.rval = [](Napi::Env env, OGRFeature *f) -> Napi::Value {
    if (!f) return env.Null();
    return FeatureNapi::New(env, f);
  };
  return job.run(info, async, 0);
}

} // namespace node_gdal
