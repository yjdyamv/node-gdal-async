#include "gdal_geometry_napi.hpp"
#include "gdal_geometrycollection_napi.hpp"
#include "../gdal_stubs_napi.hpp"

namespace node_gdal {

// ===================== GeometryCollectionNapi =====================
Napi::FunctionReference GeometryCollectionNapi::constructor;

Napi::Object GeometryCollectionNapi::Init(Napi::Env env, Napi::Object exports) {
  Napi::Function func = DefineClass(
    env, "GeometryCollection",
    {
      InstanceMethod("toString", &GeometryCollectionNapi::toString),
      InstanceMethod("getArea", &GeometryCollectionNapi::getArea),
      InstanceMethod("getLength", &GeometryCollectionNapi::getLength),
      InstanceAccessor<&GeometryCollectionNapi::childrenGetter>("children"),
    });
  constructor = Napi::Persistent(func);
  NapiSetPrototypeChain(env, func, GeometryNapi::constructor.Value());
  constructor.SuppressDestruct();
  exports.Set("GeometryCollection", func);
  return exports;
}

GeometryCollectionNapi::GeometryCollectionNapi(const Napi::CallbackInfo &info)
  : Napi::ObjectWrap<GeometryCollectionNapi>(info) {

  if (!info.IsConstructCall()) {
    Napi::Error::New(info.Env(), "Cannot call constructor as function, use 'new' keyword")
      .ThrowAsJavaScriptException();
    return;
  }

  if (info.Length() > 0 && info[0].IsExternal()) {
    this_ = info[0].As<Napi::External<OGRGeometryCollection>>().Data();
    owned_ = true;
  } else {
    if (info.Length() != 0) {
      Napi::Error::New(info.Env(), "GeometryCollection constructor doesn't take any arguments")
        .ThrowAsJavaScriptException();
      return;
    }
    this_ = new OGRGeometryCollection();
    owned_ = true;
  }
}

Napi::Value GeometryCollectionNapi::toString(const Napi::CallbackInfo &info) {
  return Napi::String::New(info.Env(), "GeometryCollection");
}
Napi::Value GeometryCollectionNapi::getArea(const Napi::CallbackInfo &info) {
  NAPI_UNWRAP_THIS(GeometryCollectionNapi, self);
  return Napi::Number::New(info.Env(), self->this_->get_Area());
}
Napi::Value GeometryCollectionNapi::getLength(const Napi::CallbackInfo &info) {
  NAPI_UNWRAP_THIS(GeometryCollectionNapi, self);
  return Napi::Number::New(info.Env(), self->this_->get_Length());
}

// ===================== MultiPointNapi =====================
Napi::FunctionReference MultiPointNapi::constructor;

Napi::Object MultiPointNapi::Init(Napi::Env env, Napi::Object exports) {
  Napi::Function func = DefineClass(
    env, "MultiPoint",
    {InstanceMethod("toString", &MultiPointNapi::toString)});
  constructor = Napi::Persistent(func);
  NapiSetPrototypeChain(env, func, GeometryCollectionNapi::constructor.Value());
  constructor.SuppressDestruct();
  exports.Set("MultiPoint", func); return exports;
}

MultiPointNapi::MultiPointNapi(const Napi::CallbackInfo &info)
  : Napi::ObjectWrap<MultiPointNapi>(info) {
  if (!info.IsConstructCall()) {
    Napi::Error::New(info.Env(), "Cannot call constructor as function, use 'new' keyword")
      .ThrowAsJavaScriptException(); return;
  }
  if (info.Length() > 0 && info[0].IsExternal()) {
    this_ = info[0].As<Napi::External<OGRMultiPoint>>().Data(); owned_ = true;
  } else {
    if (info.Length() != 0) {
      Napi::Error::New(info.Env(), "constructor doesn't take any arguments")
        .ThrowAsJavaScriptException(); return;
    }
    this_ = new OGRMultiPoint(); owned_ = true;
  }
}
Napi::Value MultiPointNapi::toString(const Napi::CallbackInfo &info) {
  return Napi::String::New(info.Env(), "MultiPoint");
}

// ===================== MultiLineStringNapi =====================
Napi::FunctionReference MultiLineStringNapi::constructor;

Napi::Object MultiLineStringNapi::Init(Napi::Env env, Napi::Object exports) {
  Napi::Function func = DefineClass(
    env, "MultiLineString",
    {InstanceMethod("toString", &MultiLineStringNapi::toString)});
  constructor = Napi::Persistent(func);
  NapiSetPrototypeChain(env, func, GeometryCollectionNapi::constructor.Value());
  constructor.SuppressDestruct();
  exports.Set("MultiLineString", func); return exports;
}

MultiLineStringNapi::MultiLineStringNapi(const Napi::CallbackInfo &info)
  : Napi::ObjectWrap<MultiLineStringNapi>(info) {
  if (!info.IsConstructCall()) {
    Napi::Error::New(info.Env(), "Cannot call constructor as function, use 'new' keyword")
      .ThrowAsJavaScriptException(); return;
  }
  if (info.Length() > 0 && info[0].IsExternal()) {
    this_ = info[0].As<Napi::External<OGRMultiLineString>>().Data(); owned_ = true;
  } else {
    if (info.Length() != 0) {
      Napi::Error::New(info.Env(), "constructor doesn't take any arguments")
        .ThrowAsJavaScriptException(); return;
    }
    this_ = new OGRMultiLineString(); owned_ = true;
  }
}
Napi::Value MultiLineStringNapi::toString(const Napi::CallbackInfo &info) {
  return Napi::String::New(info.Env(), "MultiLineString");
}

// ===================== MultiPolygonNapi =====================
Napi::FunctionReference MultiPolygonNapi::constructor;

Napi::Object MultiPolygonNapi::Init(Napi::Env env, Napi::Object exports) {
  Napi::Function func = DefineClass(
    env, "MultiPolygon",
    {InstanceMethod("toString", &MultiPolygonNapi::toString)});
  constructor = Napi::Persistent(func);
  NapiSetPrototypeChain(env, func, GeometryCollectionNapi::constructor.Value());
  constructor.SuppressDestruct();
  exports.Set("MultiPolygon", func); return exports;
}

MultiPolygonNapi::MultiPolygonNapi(const Napi::CallbackInfo &info)
  : Napi::ObjectWrap<MultiPolygonNapi>(info) {
  if (!info.IsConstructCall()) {
    Napi::Error::New(info.Env(), "Cannot call constructor as function, use 'new' keyword")
      .ThrowAsJavaScriptException(); return;
  }
  if (info.Length() > 0 && info[0].IsExternal()) {
    this_ = info[0].As<Napi::External<OGRMultiPolygon>>().Data(); owned_ = true;
  } else {
    if (info.Length() != 0) {
      Napi::Error::New(info.Env(), "constructor doesn't take any arguments")
        .ThrowAsJavaScriptException(); return;
    }
    this_ = new OGRMultiPolygon(); owned_ = true;
  }
}
Napi::Value MultiPolygonNapi::toString(const Napi::CallbackInfo &info) {
  return Napi::String::New(info.Env(), "MultiPolygon");
}

// ===================== MultiCurveNapi =====================
Napi::FunctionReference MultiCurveNapi::constructor;

Napi::Object MultiCurveNapi::Init(Napi::Env env, Napi::Object exports) {
  Napi::Function func = DefineClass(
    env, "MultiCurve",
    {InstanceMethod("toString", &MultiCurveNapi::toString)});
  constructor = Napi::Persistent(func);
  NapiSetPrototypeChain(env, func, GeometryCollectionNapi::constructor.Value());
  constructor.SuppressDestruct();
  exports.Set("MultiCurve", func); return exports;
}

MultiCurveNapi::MultiCurveNapi(const Napi::CallbackInfo &info)
  : Napi::ObjectWrap<MultiCurveNapi>(info) {
  if (!info.IsConstructCall()) {
    Napi::Error::New(info.Env(), "Cannot call constructor as function, use 'new' keyword")
      .ThrowAsJavaScriptException(); return;
  }
  if (info.Length() > 0 && info[0].IsExternal()) {
    this_ = info[0].As<Napi::External<OGRMultiCurve>>().Data(); owned_ = true;
  } else {
    if (info.Length() != 0) {
      Napi::Error::New(info.Env(), "constructor doesn't take any arguments")
        .ThrowAsJavaScriptException(); return;
    }
    this_ = new OGRMultiCurve(); owned_ = true;
  }
}
Napi::Value MultiCurveNapi::toString(const Napi::CallbackInfo &info) {
  return Napi::String::New(info.Env(), "MultiCurve");
}

Napi::Value GeometryCollectionNapi::childrenGetter(const Napi::CallbackInfo &info) {
  NAPI_UNWRAP_THIS(GeometryCollectionNapi, self);
  Napi::Object thiz = info.This().As<Napi::Object>();
  if (thiz.Has("__children")) {
    Napi::Value cached = thiz.Get("__children");
    if (!cached.IsNull() && !cached.IsUndefined()) return cached;
  }
  Napi::Object ch = GeometryCollectionChildrenNapi::constructor.New({
    Napi::External<OGRGeometryCollection>::New(info.Env(), self->this_)
  });
  thiz.Set("__children", ch);
  return ch;
}

} // namespace node_gdal
