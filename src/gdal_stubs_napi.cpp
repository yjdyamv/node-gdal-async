#include "gdal_stubs_napi.hpp"
#include "gdal_rasterband_napi.hpp"
#include "geometry/gdal_geometry_napi.hpp"

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
  if (info.Length() > 0 && info[0].IsExternal()) band_ = info[0].As<Napi::External<GDALRasterBand>>().Data();
}
Napi::Value RasterBandPixelsNapi::getPixel(const Napi::CallbackInfo &info) {
  if (!band_) return info.Env().Null();
  int x, y; NAPI_ARG_INT(0, "x", x); NAPI_ARG_INT(1, "y", y);
  double val;
  CPLErr err = band_->RasterIO(GF_Read, x, y, 1, 1, &val, 1, 1, GDT_Float64, 0, 0);
  if (err) NAPI_THROW_LAST_CPLERR;
  return Napi::Number::New(info.Env(), val);
}
Napi::Value RasterBandPixelsNapi::setPixel(const Napi::CallbackInfo &info) {
  if (!band_) return info.Env().Null();
  int x, y; double val;
  NAPI_ARG_INT(0, "x", x); NAPI_ARG_INT(1, "y", y); NAPI_ARG_DOUBLE(2, "val", val);
  CPLErr err = band_->RasterIO(GF_Write, x, y, 1, 1, (void *)&val, 1, 1, GDT_Float64, 0, 0);
  if (err) NAPI_THROW_LAST_CPLERR;
  return info.Env().Undefined();
}
GDAL_ASYNCABLE_DEFINE_NAPI(RasterBandPixelsNapi, read) {
  if (!band_) return info.Env().Null();
  int x, y, w, h;
  NAPI_ARG_INT(0, "x", x); NAPI_ARG_INT(1, "y", y); NAPI_ARG_INT(2, "w", w); NAPI_ARG_INT(3, "h", h);
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
  job.rval = [](Napi::Env env, CPLErr) { return env.Null(); };
  return job.run(info, async, 5);
}
GDAL_ASYNCABLE_DEFINE_NAPI(RasterBandPixelsNapi, write) {
  if (!band_) return info.Env().Null();
  int x, y, w, h;
  NAPI_ARG_INT(0, "x", x); NAPI_ARG_INT(1, "y", y); NAPI_ARG_INT(2, "w", w); NAPI_ARG_INT(3, "h", h);
  if (info.Length() < 5 || !info[4].IsBuffer()) return info.Env().Undefined();
  Napi::Buffer<uint8_t> buf = info[4].As<Napi::Buffer<uint8_t>>();
  GDALRasterBand *raw = band_;
  GDALAsyncableJobNapi<CPLErr> job;
  job.main = [raw, x, y, w, h, d = buf.Data(), l = buf.Length()]() {
    CPLErr err = raw->RasterIO(GF_Write, x, y, w, h, (void *)d, w, h, GDT_Byte, 0, 0);
    if (err) throw CPLGetLastErrorMsg();
    return err;
  };
  job.rval = [](Napi::Env env, CPLErr) { return env.Undefined(); };
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
  if (info.Length() > 0 && info[0].IsExternal()) feat_ = info[0].As<Napi::External<OGRFeature>>().Data();
}
Napi::Value FeatureFieldsNapi::get(const Napi::CallbackInfo &info) {
  if (!feat_) return info.Env().Null();
  int idx;
  if (info[0].IsString()) { idx = feat_->GetFieldIndex(info[0].As<Napi::String>().Utf8Value().c_str()); if (idx < 0) return info.Env().Null(); }
  else NAPI_ARG_INT(0, "index", idx);
  if (idx < 0 || idx >= feat_->GetFieldCount()) { Napi::RangeError::New(info.Env(), "Invalid field index").ThrowAsJavaScriptException(); return info.Env().Undefined(); }
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
  if (info[0].IsString()) { idx = feat_->GetFieldIndex(info[0].As<Napi::String>().Utf8Value().c_str()); if (idx < 0) { Napi::Error::New(info.Env(), "Field not found").ThrowAsJavaScriptException(); return info.Env().Undefined(); } }
  else NAPI_ARG_INT(0, "index", idx);
  if (idx < 0 || idx >= feat_->GetFieldCount()) { Napi::RangeError::New(info.Env(), "Invalid field index").ThrowAsJavaScriptException(); return info.Env().Undefined(); }
  Napi::Value val = info[1];
  OGRFieldType type = feat_->GetFieldDefnRef(idx)->GetType();
  if (val.IsNull() || val.IsUndefined()) feat_->UnsetField(idx);
  else if (val.IsNumber()) { double d = val.As<Napi::Number>().DoubleValue(); if (type == OFTInteger || type == OFTInteger64) feat_->SetField(idx, static_cast<GIntBig>(d)); else feat_->SetField(idx, d); }
  else if (val.IsString()) feat_->SetField(idx, val.As<Napi::String>().Utf8Value().c_str());
  return info.Env().Undefined();
}
Napi::Value FeatureFieldsNapi::count(const Napi::CallbackInfo &info) { return feat_ ? Napi::Number::New(info.Env(), feat_->GetFieldCount()) : Napi::Number::New(info.Env(), 0); }
Napi::Value FeatureFieldsNapi::getNames(const Napi::CallbackInfo &info) {
  if (!feat_) return Napi::Array::New(info.Env(), 0);
  int n = feat_->GetFieldCount(); Napi::Array names = Napi::Array::New(info.Env(), n);
  for (int i = 0; i < n; i++) names.Set(i, SafeStringNapi(info.Env(), feat_->GetFieldDefnRef(i)->GetNameRef()));
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
  if (info.Length() > 0 && info[0].IsExternal()) layer_ = info[0].As<Napi::External<OGRLayer>>().Data();
}
Napi::Value LayerFieldsNapi::get(const Napi::CallbackInfo &info) {
  if (!layer_) return info.Env().Null();
  OGRFeatureDefn *defn = layer_->GetLayerDefn(); if (!defn) return info.Env().Null();
  int idx;
  if (info[0].IsString()) { idx = defn->GetFieldIndex(info[0].As<Napi::String>().Utf8Value().c_str()); if (idx < 0) return info.Env().Null(); }
  else NAPI_ARG_INT(0, "index", idx);
  if (idx < 0 || idx >= defn->GetFieldCount()) { Napi::RangeError::New(info.Env(), "Invalid field index").ThrowAsJavaScriptException(); return info.Env().Undefined(); }
  OGRFieldDefn *field = defn->GetFieldDefn(idx);
  Napi::Object r = Napi::Object::New(info.Env());
  r.Set("name", SafeStringNapi(info.Env(), field->GetNameRef()));
  r.Set("type", Napi::Number::New(info.Env(), static_cast<int>(field->GetType())));
  return r;
}
Napi::Value LayerFieldsNapi::count(const Napi::CallbackInfo &info) {
  if (!layer_) return Napi::Number::New(info.Env(), 0);
  return Napi::Number::New(info.Env(), static_cast<double>(layer_->GetLayerDefn()->GetFieldCount()));
}
Napi::Value LayerFieldsNapi::getNames(const Napi::CallbackInfo &info) {
  if (!layer_) return Napi::Array::New(info.Env(), 0);
  OGRFeatureDefn *defn = layer_->GetLayerDefn();
  int n = defn->GetFieldCount(); Napi::Array names = Napi::Array::New(info.Env(), n);
  for (int i = 0; i < n; i++) names.Set(i, SafeStringNapi(info.Env(), defn->GetFieldDefn(i)->GetNameRef()));
  return names;
}

// ===================== FeatureDefnFieldsNapi =====================
Napi::FunctionReference FeatureDefnFieldsNapi::constructor;
Napi::Object FeatureDefnFieldsNapi::Init(Napi::Env env, Napi::Object exports) {
  Napi::Function f = DefineClass(env, "FeatureDefnFieldsNapi", {
    InstanceMethod("get", &FeatureDefnFieldsNapi::get),
    InstanceMethod("count", &FeatureDefnFieldsNapi::count),
    InstanceMethod("getNames", &FeatureDefnFieldsNapi::getNames),
  });
  constructor = Napi::Persistent(f); constructor.SuppressDestruct();
  exports.Set("FeatureDefnFieldsNapi", f); return exports;
}
FeatureDefnFieldsNapi::FeatureDefnFieldsNapi(const Napi::CallbackInfo &info)
  : Napi::ObjectWrap<FeatureDefnFieldsNapi>(info), defn_(nullptr) {
  if (info.Length() > 0 && info[0].IsExternal()) defn_ = info[0].As<Napi::External<OGRFeatureDefn>>().Data();
}
Napi::Value FeatureDefnFieldsNapi::get(const Napi::CallbackInfo &info) {
  if (!defn_) return info.Env().Null();
  int idx;
  if (info[0].IsString()) { idx = defn_->GetFieldIndex(info[0].As<Napi::String>().Utf8Value().c_str()); if (idx < 0) return info.Env().Null(); }
  else NAPI_ARG_INT(0, "index", idx);
  if (idx < 0 || idx >= defn_->GetFieldCount()) { Napi::RangeError::New(info.Env(), "Invalid field index").ThrowAsJavaScriptException(); return info.Env().Undefined(); }
  OGRFieldDefn *field = defn_->GetFieldDefn(idx);
  Napi::Object r = Napi::Object::New(info.Env());
  r.Set("name", SafeStringNapi(info.Env(), field->GetNameRef()));
  r.Set("type", Napi::Number::New(info.Env(), static_cast<int>(field->GetType())));
  return r;
}
Napi::Value FeatureDefnFieldsNapi::count(const Napi::CallbackInfo &info) {
  return defn_ ? Napi::Number::New(info.Env(), defn_->GetFieldCount()) : Napi::Number::New(info.Env(), 0);
}
Napi::Value FeatureDefnFieldsNapi::getNames(const Napi::CallbackInfo &info) {
  if (!defn_) return Napi::Array::New(info.Env(), 0);
  int n = defn_->GetFieldCount(); Napi::Array names = Napi::Array::New(info.Env(), n);
  for (int i = 0; i < n; i++) names.Set(i, SafeStringNapi(info.Env(), defn_->GetFieldDefn(i)->GetNameRef()));
  return names;
}

// ===================== RasterBandOverviewsNapi =====================
Napi::FunctionReference RasterBandOverviewsNapi::constructor;
Napi::Object RasterBandOverviewsNapi::Init(Napi::Env env, Napi::Object exports) {
  Napi::Function f = DefineClass(env, "RasterBandOverviewsNapi", {
    InstanceMethod("get", &RasterBandOverviewsNapi::get),
    InstanceMethod("getAsync", &RasterBandOverviewsNapi::getAsync),
    InstanceMethod("count", &RasterBandOverviewsNapi::count),
    InstanceMethod("countAsync", &RasterBandOverviewsNapi::countAsync),
  });
  constructor = Napi::Persistent(f); constructor.SuppressDestruct();
  exports.Set("RasterBandOverviewsNapi", f); return exports;
}
RasterBandOverviewsNapi::RasterBandOverviewsNapi(const Napi::CallbackInfo &info)
  : Napi::ObjectWrap<RasterBandOverviewsNapi>(info), band_(nullptr) {
  if (info.Length() > 0 && info[0].IsExternal()) band_ = info[0].As<Napi::External<GDALRasterBand>>().Data();
}
GDAL_ASYNCABLE_DEFINE_NAPI(RasterBandOverviewsNapi, get) {
  if (!band_) return info.Env().Null();
  int id; NAPI_ARG_INT(0, "id", id);
  GDALRasterBand *raw = band_;
  GDALAsyncableJobNapi<GDALRasterBand *> job;
  job.main = [raw, id]() -> GDALRasterBand * {
    GDALRasterBand *r = raw->GetOverview(id);
    if (!r) throw "Specified overview not found";
    return r;
  };
  job.rval = [](Napi::Env env, GDALRasterBand *r) { return RasterBandNapi::New(env, r); };
  return job.run(info, async, 1);
}
GDAL_ASYNCABLE_DEFINE_NAPI(RasterBandOverviewsNapi, count) {
  if (!band_) return Napi::Number::New(info.Env(), 0);
  GDALRasterBand *raw = band_;
  GDALAsyncableJobNapi<int> job;
  job.main = [raw]() { return raw->GetOverviewCount(); };
  job.rval = [](Napi::Env env, int c) { return Napi::Number::New(env, c); };
  return job.run(info, async, 0);
}

// ===================== GeometryCollectionChildrenNapi =====================
Napi::FunctionReference GeometryCollectionChildrenNapi::constructor;
Napi::Object GeometryCollectionChildrenNapi::Init(Napi::Env env, Napi::Object exports) {
  Napi::Function f = DefineClass(env, "GeometryCollectionChildrenNapi", {
    InstanceMethod("get", &GeometryCollectionChildrenNapi::get),
    InstanceMethod("count", &GeometryCollectionChildrenNapi::count),
    InstanceMethod("add", &GeometryCollectionChildrenNapi::add),
  });
  constructor = Napi::Persistent(f); constructor.SuppressDestruct();
  exports.Set("GeometryCollectionChildrenNapi", f); return exports;
}
GeometryCollectionChildrenNapi::GeometryCollectionChildrenNapi(const Napi::CallbackInfo &info)
  : Napi::ObjectWrap<GeometryCollectionChildrenNapi>(info), geom_(nullptr) {
  if (info.Length() > 0 && info[0].IsExternal()) geom_ = info[0].As<Napi::External<OGRGeometryCollection>>().Data();
}
Napi::Value GeometryCollectionChildrenNapi::get(const Napi::CallbackInfo &info) {
  if (!geom_) return info.Env().Null();
  int i; NAPI_ARG_INT(0, "index", i);
  OGRGeometry *g = geom_->getGeometryRef(i);
  if (!g) NAPI_THROW_LAST_CPLERR;
  return GeometryNapi::New(info.Env(), g, false);
}
Napi::Value GeometryCollectionChildrenNapi::count(const Napi::CallbackInfo &info) {
  return geom_ ? Napi::Number::New(info.Env(), geom_->getNumGeometries()) : Napi::Number::New(info.Env(), 0);
}
Napi::Value GeometryCollectionChildrenNapi::add(const Napi::CallbackInfo &info) {
  if (!geom_) return info.Env().Null();
  GeometryNapi *child; NAPI_ARG_WRAPPED(0, "geometry", GeometryNapi, child);
  OGRErr err = geom_->addGeometry(child->get());
  if (err) NAPI_THROW_OGRERR(err);
  return info.Env().Undefined();
}

// ===================== Remaining stubs =====================
#define IMPL_STUB(klass) \
  Napi::FunctionReference klass::constructor; \
  Napi::Object klass::Init(Napi::Env env, Napi::Object exports) { \
    Napi::Function f = DefineClass(env, #klass, {}); \
    constructor = Napi::Persistent(f); constructor.SuppressDestruct(); \
    exports.Set(#klass, f); return exports; \
  }

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
