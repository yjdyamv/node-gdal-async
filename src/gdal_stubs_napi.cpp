#include "gdal_stubs_napi.hpp"

namespace node_gdal {

// ===================== RasterBandPixelsNapi =====================
Napi::FunctionReference RasterBandPixelsNapi::constructor;

Napi::Object RasterBandPixelsNapi::Init(Napi::Env env, Napi::Object exports) {
  Napi::Function f = DefineClass(env, "RasterBandPixelsNapi", {
    InstanceMethod("get", &RasterBandPixelsNapi::getPixel),
    InstanceMethod("set", &RasterBandPixelsNapi::setPixel),
    InstanceMethod("read", &RasterBandPixelsNapi::read),
    InstanceMethod("readAsync", &RasterBandPixelsNapi::readAsync),
    InstanceMethod("write", &RasterBandPixelsNapi::write),
    InstanceMethod("writeAsync", &RasterBandPixelsNapi::writeAsync),
  });
  constructor = Napi::Persistent(f); constructor.SuppressDestruct();
  exports.Set("RasterBandPixelsNapi", f); return exports;
}

RasterBandPixelsNapi::RasterBandPixelsNapi(const Napi::CallbackInfo &info)
  : Napi::ObjectWrap<RasterBandPixelsNapi>(info), band_(nullptr) {
  if (info.Length() > 0 && info[0].IsExternal()) {
    band_ = info[0].As<Napi::External<GDALRasterBand>>().Data();
  }
}

Napi::Value RasterBandPixelsNapi::getPixel(const Napi::CallbackInfo &info) {
  if (!band_) return info.Env().Null();
  int x, y;
  NAPI_ARG_INT(0, "x", x);
  NAPI_ARG_INT(1, "y", y);
  double val;
  CPLErr err = band_->RasterIO(GF_Read, x, y, 1, 1, &val, 1, 1, GDT_Float64, 0, 0);
  if (err) NAPI_THROW_LAST_CPLERR;
  return Napi::Number::New(info.Env(), val);
}

Napi::Value RasterBandPixelsNapi::setPixel(const Napi::CallbackInfo &info) {
  if (!band_) return info.Env().Null();
  int x, y;
  double val;
  NAPI_ARG_INT(0, "x", x);
  NAPI_ARG_INT(1, "y", y);
  NAPI_ARG_DOUBLE(2, "val", val);
  CPLErr err = band_->RasterIO(GF_Write, x, y, 1, 1, (void *)&val, 1, 1, GDT_Float64, 0, 0);
  if (err) NAPI_THROW_LAST_CPLERR;
  return info.Env().Undefined();
}

GDAL_ASYNCABLE_DEFINE_NAPI(RasterBandPixelsNapi, read) {
  if (!band_) return info.Env().Null();
  int x, y, w, h;
  NAPI_ARG_INT(0, "x", x);
  NAPI_ARG_INT(1, "y", y);
  NAPI_ARG_INT(2, "w", w);
  NAPI_ARG_INT(3, "h", h);

  GDALDataType type = band_->GetRasterDataType();
  int size = w * h * GDALGetDataTypeSizeBytes(type);
  std::vector<uint8_t> buf(size);

  GDALRasterBand *raw = band_;
  GDALAsyncableJobNapi<CPLErr> job;
  job.main = [raw, x, y, w, h, type, buf = std::move(buf)]() mutable {
    CPLErr err = raw->RasterIO(GF_Read, x, y, w, h, buf.data(), w, h, type, 0, 0);
    if (err) throw CPLGetLastErrorMsg();
    return err;
  };
  job.rval = [type](Napi::Env env, CPLErr) -> Napi::Value {
    // return null for now - TypedArray creation needs better support
    return env.Null();
  };
  return job.run(info, async, 5);
}

GDAL_ASYNCABLE_DEFINE_NAPI(RasterBandPixelsNapi, write) {
  if (!band_) return info.Env().Null();
  int x, y, w, h;
  NAPI_ARG_INT(0, "x", x);
  NAPI_ARG_INT(1, "y", y);
  NAPI_ARG_INT(2, "w", w);
  NAPI_ARG_INT(3, "h", h);
  // data argument at index 4 (Buffer/TypedArray) - validate and use
  if (info.Length() < 5 || !info[4].IsBuffer()) {
    return info.Env().Undefined();
  }
  Napi::Buffer<uint8_t> buf = info[4].As<Napi::Buffer<uint8_t>>();

  GDALRasterBand *raw = band_;
  GDALAsyncableJobNapi<CPLErr> job;
  job.main = [raw, x, y, w, h, buf_data = buf.Data(), buf_len = buf.Length()]() {
    CPLErr err = raw->RasterIO(GF_Write, x, y, w, h, (void *)buf_data, w, h, GDT_Byte, 0, 0);
    if (err) throw CPLGetLastErrorMsg();
    return err;
  };
  job.rval = [](Napi::Env env, CPLErr) -> Napi::Value { return env.Undefined(); };
  return job.run(info, async, 5);
}

// ===================== FeatureFieldsNapi =====================
Napi::FunctionReference FeatureFieldsNapi::constructor;

Napi::Object FeatureFieldsNapi::Init(Napi::Env env, Napi::Object exports) {
  Napi::Function f = DefineClass(env, "FeatureFieldsNapi", {
    InstanceMethod("get", &FeatureFieldsNapi::get),
    InstanceMethod("set", &FeatureFieldsNapi::set),
    InstanceMethod("count", &FeatureFieldsNapi::count),
    InstanceMethod("getNames", &FeatureFieldsNapi::getNames),
  });
  constructor = Napi::Persistent(f); constructor.SuppressDestruct();
  exports.Set("FeatureFieldsNapi", f); return exports;
}

FeatureFieldsNapi::FeatureFieldsNapi(const Napi::CallbackInfo &info)
  : Napi::ObjectWrap<FeatureFieldsNapi>(info), feat_(nullptr) {
  if (info.Length() > 0 && info[0].IsExternal()) {
    feat_ = info[0].As<Napi::External<OGRFeature>>().Data();
  }
}

Napi::Value FeatureFieldsNapi::get(const Napi::CallbackInfo &info) {
  if (!feat_) return info.Env().Null();
  int idx;
  if (info[0].IsString()) {
    std::string name = info[0].As<Napi::String>().Utf8Value();
    idx = feat_->GetFieldIndex(name.c_str());
    if (idx < 0) { return info.Env().Null(); }
  } else {
    NAPI_ARG_INT(0, "index", idx);
  }
  if (idx < 0 || idx >= feat_->GetFieldCount()) {
    Napi::RangeError::New(info.Env(), "Invalid field index").ThrowAsJavaScriptException();
    return info.Env().Undefined();
  }
  if (!feat_->IsFieldSetAndNotNull(idx)) return info.Env().Null();

  OGRFieldType type = feat_->GetFieldDefnRef(idx)->GetType();
  switch (type) {
    case OFTInteger:   return Napi::Number::New(info.Env(), feat_->GetFieldAsInteger(idx));
    case OFTReal:      return Napi::Number::New(info.Env(), feat_->GetFieldAsDouble(idx));
    case OFTString:    return SafeStringNapi(info.Env(), feat_->GetFieldAsString(idx));
    case OFTInteger64: return Napi::Number::New(info.Env(), static_cast<double>(feat_->GetFieldAsInteger64(idx)));
    default:           return info.Env().Null();
  }
}

Napi::Value FeatureFieldsNapi::set(const Napi::CallbackInfo &info) {
  if (!feat_) return info.Env().Null();
  int idx;
  if (info[0].IsString()) {
    std::string name = info[0].As<Napi::String>().Utf8Value();
    idx = feat_->GetFieldIndex(name.c_str());
    if (idx < 0) {
      Napi::Error::New(info.Env(), "Field not found").ThrowAsJavaScriptException();
      return info.Env().Undefined();
    }
  } else {
    NAPI_ARG_INT(0, "index", idx);
  }
  if (idx < 0 || idx >= feat_->GetFieldCount()) {
    Napi::RangeError::New(info.Env(), "Invalid field index").ThrowAsJavaScriptException();
    return info.Env().Undefined();
  }

  Napi::Value val = info[1];
  OGRFieldType type = feat_->GetFieldDefnRef(idx)->GetType();
  if (val.IsNull() || val.IsUndefined()) {
    feat_->UnsetField(idx);
  } else if (val.IsNumber()) {
    double d = val.As<Napi::Number>().DoubleValue();
    if (type == OFTInteger || type == OFTInteger64) feat_->SetField(idx, static_cast<GIntBig>(d));
    else feat_->SetField(idx, d);
  } else if (val.IsString()) {
    feat_->SetField(idx, val.As<Napi::String>().Utf8Value().c_str());
  }
  return info.Env().Undefined();
}

Napi::Value FeatureFieldsNapi::count(const Napi::CallbackInfo &info) {
  if (!feat_) return Napi::Number::New(info.Env(), 0);
  return Napi::Number::New(info.Env(), feat_->GetFieldCount());
}

Napi::Value FeatureFieldsNapi::getNames(const Napi::CallbackInfo &info) {
  if (!feat_) return Napi::Array::New(info.Env(), 0);
  int n = feat_->GetFieldCount();
  Napi::Array names = Napi::Array::New(info.Env(), n);
  for (int i = 0; i < n; i++) {
    names.Set(i, SafeStringNapi(info.Env(), feat_->GetFieldDefnRef(i)->GetNameRef()));
  }
  return names;
}

// ===================== LayerFieldsNapi =====================
Napi::FunctionReference LayerFieldsNapi::constructor;

Napi::Object LayerFieldsNapi::Init(Napi::Env env, Napi::Object exports) {
  Napi::Function f = DefineClass(env, "LayerFieldsNapi", {
    InstanceMethod("get", &LayerFieldsNapi::get),
    InstanceMethod("count", &LayerFieldsNapi::count),
    InstanceMethod("getNames", &LayerFieldsNapi::getNames),
  });
  constructor = Napi::Persistent(f); constructor.SuppressDestruct();
  exports.Set("LayerFieldsNapi", f); return exports;
}

LayerFieldsNapi::LayerFieldsNapi(const Napi::CallbackInfo &info)
  : Napi::ObjectWrap<LayerFieldsNapi>(info), layer_(nullptr) {
  if (info.Length() > 0 && info[0].IsExternal()) {
    layer_ = info[0].As<Napi::External<OGRLayer>>().Data();
  }
}

Napi::Value LayerFieldsNapi::get(const Napi::CallbackInfo &info) {
  if (!layer_) return info.Env().Null();
  int idx;
  if (info[0].IsString()) {
    std::string name = info[0].As<Napi::String>().Utf8Value();
    idx = layer_->GetLayerDefn()->GetFieldIndex(name.c_str());
    if (idx < 0) { return info.Env().Null(); }
  } else {
    NAPI_ARG_INT(0, "index", idx);
  }
  OGRFeatureDefn *defn = layer_->GetLayerDefn();
  if (idx < 0 || idx >= defn->GetFieldCount()) {
    Napi::RangeError::New(info.Env(), "Invalid field index").ThrowAsJavaScriptException();
    return info.Env().Undefined();
  }
  OGRFieldDefn *field = defn->GetFieldDefn(idx);
  Napi::Object r = Napi::Object::New(info.Env());
  r.Set("name", SafeStringNapi(info.Env(), field->GetNameRef()));
  r.Set("type", Napi::Number::New(info.Env(), field->GetType()));
  return r;
}

Napi::Value LayerFieldsNapi::count(const Napi::CallbackInfo &info) {
  if (!layer_) return Napi::Number::New(info.Env(), 0);
  return Napi::Number::New(info.Env(), static_cast<double>(layer_->GetLayerDefn()->GetFieldCount()));
}

Napi::Value LayerFieldsNapi::getNames(const Napi::CallbackInfo &info) {
  if (!layer_) return Napi::Array::New(info.Env(), 0);
  OGRFeatureDefn *defn = layer_->GetLayerDefn();
  int n = defn->GetFieldCount();
  Napi::Array names = Napi::Array::New(info.Env(), n);
  for (int i = 0; i < n; i++) {
    names.Set(i, SafeStringNapi(info.Env(), defn->GetFieldDefn(i)->GetNameRef()));
  }
  return names;
}

// ---- Remaining stubs ----
#define IMPL_STUB(klass) \
  Napi::FunctionReference klass::constructor; \
  Napi::Object klass::Init(Napi::Env env, Napi::Object exports) { \
    Napi::Function f = DefineClass(env, #klass, {}); \
    constructor = Napi::Persistent(f); constructor.SuppressDestruct(); \
    exports.Set(#klass, f); return exports; \
  }

IMPL_STUB(RasterBandOverviewsNapi)
IMPL_STUB(FeatureDefnFieldsNapi)
IMPL_STUB(GeometryCollectionChildrenNapi)
IMPL_STUB(PolygonRingsNapi)
IMPL_STUB(LineStringPointsNapi)
IMPL_STUB(CompoundCurveCurvesNapi)
IMPL_STUB(GroupGroupsNapi)
IMPL_STUB(GroupArraysNapi)
IMPL_STUB(GroupAttributesNapi)
IMPL_STUB(GroupDimensionsNapi)
IMPL_STUB(ArrayDimensionsNapi)
IMPL_STUB(ArrayAttributesNapi)

namespace VsiNapi {
Napi::Object Init(Napi::Env env, Napi::Object exports) {
  exports.Set("VsiNapi", Napi::Object::New(env));
  return exports;
}
}

} // namespace node_gdal
