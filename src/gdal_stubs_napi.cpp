#include "gdal_stubs_napi.hpp"
#include "gdal_rasterband_napi.hpp"
#include "gdal_multi_napi.hpp"
#include "geometry/gdal_geometry_napi.hpp"
#include "geometry/gdal_linearring_napi.hpp"
#include "geometry/gdal_point_napi.hpp"

namespace node_gdal {

// ===================== RasterBandPixelsNapi =====================
Napi::FunctionReference RasterBandPixelsNapi::constructor;
Napi::Object RasterBandPixelsNapi::Init(Napi::Env env, Napi::Object exports) {
  Napi::Function f = DefineClass(env, "RasterBandPixels", {
    InstanceMethod("get", &RasterBandPixelsNapi::getPixel),
    InstanceMethod("set", &RasterBandPixelsNapi::setPixel),
    InstanceMethod("read", &RasterBandPixelsNapi::read),
    InstanceMethod("readAsync", &RasterBandPixelsNapi::readAsync),
    InstanceMethod("write", &RasterBandPixelsNapi::write),
    InstanceMethod("writeAsync", &RasterBandPixelsNapi::writeAsync),
  });
  constructor = Napi::Persistent(f); constructor.SuppressDestruct();
  exports.Set("RasterBandPixels", f); return exports;
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
  Napi::Function f = DefineClass(env, "FeatureFields", {
    InstanceMethod("get", &FeatureFieldsNapi::get),
    InstanceMethod("set", &FeatureFieldsNapi::set),
    InstanceMethod("count", &FeatureFieldsNapi::count),
    InstanceMethod("getNames", &FeatureFieldsNapi::getNames),
  });
  constructor = Napi::Persistent(f); constructor.SuppressDestruct();
  exports.Set("FeatureFields", f); return exports;
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
  Napi::Function f = DefineClass(env, "LayerFields", {
    InstanceMethod("get", &LayerFieldsNapi::get),
    InstanceMethod("count", &LayerFieldsNapi::count),
    InstanceMethod("getNames", &LayerFieldsNapi::getNames),
  });
  constructor = Napi::Persistent(f); constructor.SuppressDestruct();
  exports.Set("LayerFields", f); return exports;
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
  Napi::Function f = DefineClass(env, "FeatureDefnFields", {
    InstanceMethod("get", &FeatureDefnFieldsNapi::get),
    InstanceMethod("count", &FeatureDefnFieldsNapi::count),
    InstanceMethod("getNames", &FeatureDefnFieldsNapi::getNames),
  });
  constructor = Napi::Persistent(f); constructor.SuppressDestruct();
  exports.Set("FeatureDefnFields", f); return exports;
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
  Napi::Function f = DefineClass(env, "RasterBandOverviews", {
    InstanceMethod("get", &RasterBandOverviewsNapi::get),
    InstanceMethod("getAsync", &RasterBandOverviewsNapi::getAsync),
    InstanceMethod("count", &RasterBandOverviewsNapi::count),
    InstanceMethod("countAsync", &RasterBandOverviewsNapi::countAsync),
  });
  constructor = Napi::Persistent(f); constructor.SuppressDestruct();
  exports.Set("RasterBandOverviews", f); return exports;
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
  Napi::Function f = DefineClass(env, "GeometryCollectionChildren", {
    InstanceMethod("get", &GeometryCollectionChildrenNapi::get),
    InstanceMethod("count", &GeometryCollectionChildrenNapi::count),
    InstanceMethod("add", &GeometryCollectionChildrenNapi::add),
  });
  constructor = Napi::Persistent(f); constructor.SuppressDestruct();
  exports.Set("GeometryCollectionChildren", f); return exports;
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

// ===================== PolygonRingsNapi =====================
Napi::FunctionReference PolygonRingsNapi::constructor;
Napi::Object PolygonRingsNapi::Init(Napi::Env env, Napi::Object exports) {
  Napi::Function f = DefineClass(env, "PolygonRings", {
    InstanceMethod("get", &PolygonRingsNapi::get),
    InstanceMethod("count", &PolygonRingsNapi::count),
    InstanceMethod("add", &PolygonRingsNapi::add),
  });
  constructor = Napi::Persistent(f); constructor.SuppressDestruct();
  exports.Set("PolygonRings", f); return exports;
}
PolygonRingsNapi::PolygonRingsNapi(const Napi::CallbackInfo &info)
  : Napi::ObjectWrap<PolygonRingsNapi>(info), geom_(nullptr) {
  if (info.Length() > 0 && info[0].IsExternal()) geom_ = info[0].As<Napi::External<OGRPolygon>>().Data();
}
Napi::Value PolygonRingsNapi::get(const Napi::CallbackInfo &info) {
  if (!geom_) return info.Env().Null();
  int i; NAPI_ARG_INT(0, "index", i);
  OGRLinearRing *r = (i == 0) ? geom_->getExteriorRing() : geom_->getInteriorRing(i - 1);
  if (!r) NAPI_THROW_LAST_CPLERR;
  return LinearRingNapi::New(info.Env(), r, false);
}
Napi::Value PolygonRingsNapi::count(const Napi::CallbackInfo &info) {
  if (!geom_) return Napi::Number::New(info.Env(), 0);
  int n = geom_->getExteriorRing() ? 1 : 0;
  n += geom_->getNumInteriorRings();
  return Napi::Number::New(info.Env(), n);
}
Napi::Value PolygonRingsNapi::add(const Napi::CallbackInfo &info) {
  if (!geom_) return info.Env().Null();
  // Accept LinearRingNapi object OR Array of rings OR (x, y, ...) coordinate pairs
  if (info.Length() >= 1 && info[0].IsArray()) {
    Napi::Array rings = info[0].As<Napi::Array>();
    for (uint32_t i = 0; i < rings.Length(); i++) {
      Napi::Value item = rings.Get(i);
      if (!item.IsObject()) { Napi::Error::New(info.Env(), "Array elements must be LinearRing objects").ThrowAsJavaScriptException(); return info.Env().Undefined(); }
      LinearRingNapi *ring = LinearRingNapi::Unwrap(item.As<Napi::Object>());
      if (!ring || !ring->isAlive()) { Napi::Error::New(info.Env(), "LinearRing object has been destroyed").ThrowAsJavaScriptException(); return info.Env().Undefined(); }
      OGRErr err = geom_->addRing(ring->get());
      if (err) NAPI_THROW_OGRERR(err);
    }
  } else if (info.Length() >= 1 && info[0].IsObject() && !info[0].IsNumber()) {
    LinearRingNapi *ring = nullptr;
    NAPI_ARG_WRAPPED(0, "ring", LinearRingNapi, ring);
    OGRErr err = geom_->addRing(ring->get());
    if (err) NAPI_THROW_OGRERR(err);
  } else if (info.Length() >= 4 && info[0].IsNumber()) {
    OGRLinearRing ring;
    for (int i = 0; i + 1 < (int)info.Length(); i += 2) {
      double x = info[i].As<Napi::Number>().DoubleValue();
      double y = info[i+1].As<Napi::Number>().DoubleValue();
      ring.addPoint(x, y);
    }
    ring.closeRings();
    OGRErr err = geom_->addRing(&ring);
    if (err) NAPI_THROW_OGRERR(err);
  } else {
    Napi::Error::New(info.Env(), "add() requires a LinearRing or (x, y, ...) coordinate pairs").ThrowAsJavaScriptException();
  }
  return info.Env().Undefined();
}

// ===================== LineStringPointsNapi =====================
Napi::FunctionReference LineStringPointsNapi::constructor;
Napi::Object LineStringPointsNapi::Init(Napi::Env env, Napi::Object exports) {
  Napi::Function f = DefineClass(env, "LineStringPoints", {
    InstanceMethod("get", &LineStringPointsNapi::get),
    InstanceMethod("count", &LineStringPointsNapi::count),
    InstanceMethod("add", &LineStringPointsNapi::add),
    InstanceMethod("set", &LineStringPointsNapi::setPi),
  });
  constructor = Napi::Persistent(f); constructor.SuppressDestruct();
  exports.Set("LineStringPoints", f); return exports;
}
LineStringPointsNapi::LineStringPointsNapi(const Napi::CallbackInfo &info)
  : Napi::ObjectWrap<LineStringPointsNapi>(info), geom_(nullptr) {
  if (info.Length() > 0 && info[0].IsExternal()) geom_ = info[0].As<Napi::External<OGRLineString>>().Data();
}
Napi::Value LineStringPointsNapi::get(const Napi::CallbackInfo &info) {
  if (!geom_) return info.Env().Null();
  int i; NAPI_ARG_INT(0, "index", i);
  OGRPoint pt; geom_->getPoint(i, &pt);
  return PointNapi::New(info.Env(), new OGRPoint(pt));
}
Napi::Value LineStringPointsNapi::count(const Napi::CallbackInfo &info) {
  return geom_ ? Napi::Number::New(info.Env(), geom_->getNumPoints()) : Napi::Number::New(info.Env(), 0);
}
Napi::Value LineStringPointsNapi::add(const Napi::CallbackInfo &info) {
  if (!geom_) return info.Env().Null();
  // Accept PointNapi object OR (x, y, z) coordinates
  if (info.Length() >= 1 && info[0].IsObject() && !info[0].IsNumber()) {
    PointNapi *pt = nullptr;
    NAPI_ARG_WRAPPED(0, "point", PointNapi, pt);
    geom_->addPoint(pt->get());
  } else if (info.Length() >= 2 && info[0].IsNumber()) {
    double x, y, z = 0;
    NAPI_ARG_DOUBLE(0, "x", x);
    NAPI_ARG_DOUBLE(1, "y", y);
    NAPI_ARG_DOUBLE_OPT(2, "z", z);
    geom_->addPoint(x, y, z);
  } else {
    Napi::Error::New(info.Env(), "add() requires a Point or (x, y[, z]) coordinates").ThrowAsJavaScriptException();
  }
  return info.Env().Undefined();
}
Napi::Value LineStringPointsNapi::setPi(const Napi::CallbackInfo &info) {
  if (!geom_) return info.Env().Null();
  int i; NAPI_ARG_INT(0, "index", i);
  PointNapi *pt; NAPI_ARG_WRAPPED(1, "point", PointNapi, pt);
  geom_->setPoint(i, pt->get());
  return info.Env().Undefined();
}

// ===================== CompoundCurveCurvesNapi =====================
Napi::FunctionReference CompoundCurveCurvesNapi::constructor;
Napi::Object CompoundCurveCurvesNapi::Init(Napi::Env env, Napi::Object exports) {
  Napi::Function f = DefineClass(env, "CompoundCurveCurves", {
    InstanceMethod("get", &CompoundCurveCurvesNapi::get),
    InstanceMethod("count", &CompoundCurveCurvesNapi::count),
  });
  constructor = Napi::Persistent(f); constructor.SuppressDestruct();
  exports.Set("CompoundCurveCurves", f); return exports;
}
CompoundCurveCurvesNapi::CompoundCurveCurvesNapi(const Napi::CallbackInfo &info)
  : Napi::ObjectWrap<CompoundCurveCurvesNapi>(info), geom_(nullptr) {
  if (info.Length() > 0 && info[0].IsExternal()) geom_ = info[0].As<Napi::External<OGRCompoundCurve>>().Data();
}
Napi::Value CompoundCurveCurvesNapi::get(const Napi::CallbackInfo &info) {
  if (!geom_) return info.Env().Null();
  int i; NAPI_ARG_INT(0, "index", i);
  if (i < 0 || i >= geom_->getNumCurves()) { NAPI_THROW_LAST_CPLERR; }
  return GeometryNapi::New(info.Env(), geom_->getCurve(i), false);
}
Napi::Value CompoundCurveCurvesNapi::count(const Napi::CallbackInfo &info) {
  return geom_ ? Napi::Number::New(info.Env(), geom_->getNumCurves()) : Napi::Number::New(info.Env(), 0);
}

// ===================== Multi-dimensional collections =====================
#define MD_COLL_INIT(klass, jsname) \
  Napi::FunctionReference klass::constructor; \
  Napi::Object klass::Init(Napi::Env env, Napi::Object exports) { \
    Napi::Function f = DefineClass(env, jsname, { \
      InstanceMethod("get", &klass::get), \
      InstanceMethod("count", &klass::count), \
      InstanceMethod("getNames", &klass::getNames), \
    }); \
    constructor = Napi::Persistent(f); constructor.SuppressDestruct(); \
    exports.Set(jsname, f); return exports; \
  }

#define MD_GROUP_CTOR(klass) \
  klass::klass(const Napi::CallbackInfo &info) : Napi::ObjectWrap<klass>(info), group_(nullptr) { \
    if (info.Length() > 0 && info[0].IsExternal()) group_ = info[0].As<Napi::External<GDALGroup>>().Data(); \
  }

#define MD_ARRAY_CTOR(klass) \
  klass::klass(const Napi::CallbackInfo &info) : Napi::ObjectWrap<klass>(info), array_(nullptr) { \
    if (info.Length() > 0 && info[0].IsExternal()) array_ = info[0].As<Napi::External<GDALMDArray>>().Data(); \
  }

// --- GroupGroups ---
MD_COLL_INIT(GroupGroupsNapi, "GroupGroups") MD_GROUP_CTOR(GroupGroupsNapi)
Napi::Value GroupGroupsNapi::get(const Napi::CallbackInfo &info) {
  if (!group_) return info.Env().Null();
  std::shared_ptr<GDALGroup> r;
  if (info[0].IsString()) r = group_->OpenGroup(info[0].As<Napi::String>().Utf8Value());
  else if (info[0].IsNumber()) { auto names = group_->GetGroupNames(); int i = info[0].As<Napi::Number>().Int32Value(); if (i>=0 && i<(int)names.size()) r = group_->OpenGroup(names[i]); }
  return r ? GroupNapi::New(info.Env(), r.get()) : info.Env().Null();
}
Napi::Value GroupGroupsNapi::count(const Napi::CallbackInfo &info) {
  return group_ ? Napi::Number::New(info.Env(), (int)group_->GetGroupNames().size()) : Napi::Number::New(info.Env(), 0);
}
Napi::Value GroupGroupsNapi::getNames(const Napi::CallbackInfo &info) {
  if (!group_) return Napi::Array::New(info.Env(), 0);
  auto names = group_->GetGroupNames(); Napi::Array r = Napi::Array::New(info.Env(), names.size());
  for (size_t i = 0; i < names.size(); i++) r.Set(i, Napi::String::New(info.Env(), names[i]));
  return r;
}

// --- GroupArrays ---
MD_COLL_INIT(GroupArraysNapi, "GroupArrays") MD_GROUP_CTOR(GroupArraysNapi)
Napi::Value GroupArraysNapi::get(const Napi::CallbackInfo &info) {
  if (!group_) return info.Env().Null();
  std::shared_ptr<GDALMDArray> r;
  if (info[0].IsString()) r = group_->OpenMDArray(info[0].As<Napi::String>().Utf8Value());
  else if (info[0].IsNumber()) { auto ns = group_->GetMDArrayNames(); int i = info[0].As<Napi::Number>().Int32Value(); if (i>=0&&i<(int)ns.size()) r = group_->OpenMDArray(ns[i]); }
  return r ? MDArrayNapi::New(info.Env(), r.get()) : info.Env().Null();
}
Napi::Value GroupArraysNapi::count(const Napi::CallbackInfo &info) {
  return group_ ? Napi::Number::New(info.Env(), (int)group_->GetMDArrayNames().size()) : Napi::Number::New(info.Env(), 0);
}
Napi::Value GroupArraysNapi::getNames(const Napi::CallbackInfo &info) {
  if (!group_) return Napi::Array::New(info.Env(), 0);
  auto ns = group_->GetMDArrayNames(); Napi::Array r = Napi::Array::New(info.Env(), ns.size());
  for (size_t i = 0; i < ns.size(); i++) r.Set(i, Napi::String::New(info.Env(), ns[i]));
  return r;
}

// --- GroupAttributes ---
MD_COLL_INIT(GroupAttributesNapi, "GroupAttributes") MD_GROUP_CTOR(GroupAttributesNapi)
Napi::Value GroupAttributesNapi::get(const Napi::CallbackInfo &info) {
  if (!group_) return info.Env().Null();
  std::shared_ptr<GDALAttribute> r;
  if (info[0].IsString()) r = group_->GetAttribute(info[0].As<Napi::String>().Utf8Value());
  else if (info[0].IsNumber()) { auto attrs = group_->GetAttributes(); int i = info[0].As<Napi::Number>().Int32Value(); if (i>=0&&i<(int)attrs.size()) r = attrs[i]; }
  return r ? AttributeNapi::New(info.Env(), r.get()) : info.Env().Null();
}
Napi::Value GroupAttributesNapi::count(const Napi::CallbackInfo &info) {
  return group_ ? Napi::Number::New(info.Env(), (int)group_->GetAttributes().size()) : Napi::Number::New(info.Env(), 0);
}
Napi::Value GroupAttributesNapi::getNames(const Napi::CallbackInfo &info) {
  if (!group_) return Napi::Array::New(info.Env(), 0);
  auto attrs = group_->GetAttributes(); Napi::Array r = Napi::Array::New(info.Env(), attrs.size());
  for (size_t i = 0; i < attrs.size(); i++) r.Set(i, Napi::String::New(info.Env(), attrs[i]->GetName()));
  return r;
}

// --- GroupDimensions ---
MD_COLL_INIT(GroupDimensionsNapi, "GroupDimensions") MD_GROUP_CTOR(GroupDimensionsNapi)
Napi::Value GroupDimensionsNapi::get(const Napi::CallbackInfo &info) {
  if (!group_) return info.Env().Null();
  auto dims = group_->GetDimensions();
  if (info[0].IsString()) {
    for (auto &d : dims) if (d->GetName() == info[0].As<Napi::String>().Utf8Value())
      return DimensionNapi::New(info.Env(), d.get());
  } else if (info[0].IsNumber()) {
    int i = info[0].As<Napi::Number>().Int32Value();
    if (i>=0 && i<(int)dims.size()) return DimensionNapi::New(info.Env(), dims[i].get());
  }
  return info.Env().Null();
}
Napi::Value GroupDimensionsNapi::count(const Napi::CallbackInfo &info) {
  return group_ ? Napi::Number::New(info.Env(), (int)group_->GetDimensions().size()) : Napi::Number::New(info.Env(), 0);
}
Napi::Value GroupDimensionsNapi::getNames(const Napi::CallbackInfo &info) {
  if (!group_) return Napi::Array::New(info.Env(), 0);
  auto dims = group_->GetDimensions(); Napi::Array r = Napi::Array::New(info.Env(), dims.size());
  for (size_t i = 0; i < dims.size(); i++) r.Set(i, Napi::String::New(info.Env(), dims[i]->GetName()));
  return r;
}

// --- ArrayDimensions ---
MD_COLL_INIT(ArrayDimensionsNapi, "ArrayDimensions") MD_ARRAY_CTOR(ArrayDimensionsNapi)
Napi::Value ArrayDimensionsNapi::get(const Napi::CallbackInfo &info) {
  if (!array_) return info.Env().Null();
  if (info[0].IsNumber()) { auto dims = array_->GetDimensions(); int i = info[0].As<Napi::Number>().Int32Value(); if (i>=0&&i<(int)dims.size()) return DimensionNapi::New(info.Env(), dims[i].get()); }
  return info.Env().Null();
}
Napi::Value ArrayDimensionsNapi::count(const Napi::CallbackInfo &info) {
  return array_ ? Napi::Number::New(info.Env(), (int)array_->GetDimensions().size()) : Napi::Number::New(info.Env(), 0);
}
Napi::Value ArrayDimensionsNapi::getNames(const Napi::CallbackInfo &info) {
  if (!array_) return Napi::Array::New(info.Env(), 0);
  auto dims = array_->GetDimensions(); Napi::Array r = Napi::Array::New(info.Env(), dims.size());
  for (size_t i = 0; i < dims.size(); i++) r.Set(i, Napi::String::New(info.Env(), dims[i]->GetName()));
  return r;
}

// --- ArrayAttributes ---
MD_COLL_INIT(ArrayAttributesNapi, "ArrayAttributes") MD_ARRAY_CTOR(ArrayAttributesNapi)
Napi::Value ArrayAttributesNapi::get(const Napi::CallbackInfo &info) {
  if (!array_) return info.Env().Null();
  std::shared_ptr<GDALAttribute> r;
  if (info[0].IsString()) r = array_->GetAttribute(info[0].As<Napi::String>().Utf8Value());
  else if (info[0].IsNumber()) { auto attrs = array_->GetAttributes(); int i = info[0].As<Napi::Number>().Int32Value(); if (i>=0&&i<(int)attrs.size()) r = attrs[i]; }
  return r ? AttributeNapi::New(info.Env(), r.get()) : info.Env().Null();
}
Napi::Value ArrayAttributesNapi::count(const Napi::CallbackInfo &info) {
  return array_ ? Napi::Number::New(info.Env(), (int)array_->GetAttributes().size()) : Napi::Number::New(info.Env(), 0);
}
Napi::Value ArrayAttributesNapi::getNames(const Napi::CallbackInfo &info) {
  if (!array_) return Napi::Array::New(info.Env(), 0);
  auto attrs = array_->GetAttributes(); Napi::Array r = Napi::Array::New(info.Env(), attrs.size());
  for (size_t i = 0; i < attrs.size(); i++) r.Set(i, Napi::String::New(info.Env(), attrs[i]->GetName()));
  return r;
}

} // namespace node_gdal
