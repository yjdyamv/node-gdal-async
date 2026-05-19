#include "gdal_layer_napi.hpp"
#include "gdal_spatial_reference_napi.hpp"

namespace node_gdal {

Napi::FunctionReference LayerNapi::constructor;

Napi::Object LayerNapi::Init(Napi::Env env, Napi::Object exports) {
  Napi::Function func = DefineClass(
    env, "LayerNapi",
    {
      InstanceMethod("toString", &LayerNapi::toString),
      InstanceMethod("getExtent", &LayerNapi::getExtent),
      InstanceMethod("setAttributeFilter", &LayerNapi::setAttributeFilter),
      InstanceMethod("setSpatialFilter", &LayerNapi::setSpatialFilter),
      InstanceMethod("getSpatialFilter", &LayerNapi::getSpatialFilter),
      InstanceMethod("testCapability", &LayerNapi::testCapability),
      InstanceMethod("flush", &LayerNapi::syncToDisk),
      InstanceMethod("flushAsync", &LayerNapi::syncToDiskAsync),
      InstanceAccessor<&LayerNapi::srsGetter>("srs"),
      InstanceAccessor<&LayerNapi::nameGetter>("name"),
      InstanceAccessor<&LayerNapi::geomTypeGetter>("geomType"),
      InstanceAccessor<&LayerNapi::geomColumnGetter>("geomColumn"),
      InstanceAccessor<&LayerNapi::fidColumnGetter>("fidColumn"),
    });
  constructor = Napi::Persistent(func);
  constructor.SuppressDestruct();
  exports.Set("LayerNapi", func);
  return exports;
}

LayerNapi::LayerNapi(const Napi::CallbackInfo &info)
  : Napi::ObjectWrap<LayerNapi>(info), this_(nullptr) {
  if (!info.IsConstructCall()) {
    Napi::Error::New(info.Env(), "Cannot call constructor as function, use 'new' keyword")
      .ThrowAsJavaScriptException();
    return;
  }
  if (info.Length() > 0 && info[0].IsExternal()) {
    this_ = info[0].As<Napi::External<OGRLayer>>().Data();
  } else {
    Napi::Error::New(info.Env(), "Cannot create layer directly. Create with dataset instead.")
      .ThrowAsJavaScriptException();
  }
}

LayerNapi::~LayerNapi() {
  this_ = nullptr;
}

Napi::Value LayerNapi::New(Napi::Env env, OGRLayer *layer) {
  Napi::EscapableHandleScope scope(env);
  if (!layer) return scope.Escape(env.Null());
  return scope.Escape(constructor.New({Napi::External<OGRLayer>::New(env, layer)}));
}

Napi::Value LayerNapi::toString(const Napi::CallbackInfo &info) {
  NAPI_UNWRAP_THIS(LayerNapi, layer);
  if (!layer->this_) return Napi::String::New(info.Env(), "Null layer");
  std::ostringstream ss;
  ss << "Layer (" << layer->this_->GetName() << ")";
  return Napi::String::New(info.Env(), ss.str());
}

Napi::Value LayerNapi::getExtent(const Napi::CallbackInfo &info) {
  NAPI_UNWRAP_THIS(LayerNapi, layer);
  int force = 1;
  NAPI_ARG_BOOL_OPT(0, "force", force);
  OGREnvelope env;
  OGRErr err = layer->this_->GetExtent(&env, force);
  if (err) {
    Napi::Error::New(info.Env(), "Can't get layer extent without computing it")
      .ThrowAsJavaScriptException();
    return info.Env().Undefined();
  }
  Napi::Object result = Napi::Object::New(info.Env());
  result.Set("minX", Napi::Number::New(info.Env(), env.MinX));
  result.Set("maxX", Napi::Number::New(info.Env(), env.MaxX));
  result.Set("minY", Napi::Number::New(info.Env(), env.MinY));
  result.Set("maxY", Napi::Number::New(info.Env(), env.MaxY));
  return result;
}

Napi::Value LayerNapi::setAttributeFilter(const Napi::CallbackInfo &info) {
  NAPI_UNWRAP_THIS(LayerNapi, layer);
  std::string filter;
  if (info.Length() > 0 && info[0].IsString()) {
    filter = info[0].As<Napi::String>().Utf8Value();
  }
  OGRErr err = filter.empty() ? layer->this_->SetAttributeFilter(nullptr)
                              : layer->this_->SetAttributeFilter(filter.c_str());
  if (err) NAPI_THROW_OGRERR(err);
  return info.Env().Undefined();
}

Napi::Value LayerNapi::setSpatialFilter(const Napi::CallbackInfo &info) {
  NAPI_UNWRAP_THIS(LayerNapi, layer);
  if (info.Length() == 4 && info[0].IsNumber()) {
    double minX, minY, maxX, maxY;
    NAPI_ARG_DOUBLE(0, "minX", minX);
    NAPI_ARG_DOUBLE(1, "minY", minY);
    NAPI_ARG_DOUBLE(2, "maxX", maxX);
    NAPI_ARG_DOUBLE(3, "maxY", maxY);
    layer->this_->SetSpatialFilterRect(minX, minY, maxX, maxY);
  } else if (info.Length() == 0 || (info.Length() == 1 && info[0].IsNull())) {
    layer->this_->SetSpatialFilter(nullptr);
  } else if (info.Length() >= 1 && info[0].IsObject()) {
    // Accept GeometryNapi (placeholder - GeometryNapi not yet fully wired)
    layer->this_->SetSpatialFilter(nullptr);
  } else {
    Napi::Error::New(info.Env(), "Invalid number of arguments").ThrowAsJavaScriptException();
  }
  return info.Env().Undefined();
}

Napi::Value LayerNapi::getSpatialFilter(const Napi::CallbackInfo &info) {
  NAPI_UNWRAP_THIS(LayerNapi, layer);
  OGRGeometry *filter = layer->this_->GetSpatialFilter();
  if (!filter) return info.Env().Null();
  // TODO: return GeometryNapi when fully wired
  return info.Env().Null();
}

Napi::Value LayerNapi::testCapability(const Napi::CallbackInfo &info) {
  NAPI_UNWRAP_THIS(LayerNapi, layer);
  std::string cap;
  NAPI_ARG_STR(0, "capability", cap);
  return Napi::Boolean::New(info.Env(), layer->this_->TestCapability(cap.c_str()));
}

GDAL_ASYNCABLE_DEFINE_NAPI(LayerNapi, syncToDisk) {
  NAPI_UNWRAP_THIS(LayerNapi, layer);
  GDALAsyncableJobNapi<int> job;
  job.main = [layer]() { return layer->this_->SyncToDisk(); };
  job.rval = [](Napi::Env env, int) { return env.Undefined(); };
  return job.run(info, async, 0);
}

Napi::Value LayerNapi::srsGetter(const Napi::CallbackInfo &info) {
  NAPI_UNWRAP_THIS(LayerNapi, layer);
  const OGRSpatialReference *srs = layer->this_->GetSpatialRef();
  if (!srs) return info.Env().Null();
  return SpatialReferenceNapi::New(info.Env(), srs);
}

Napi::Value LayerNapi::nameGetter(const Napi::CallbackInfo &info) {
  NAPI_UNWRAP_THIS(LayerNapi, layer);
  return SafeStringNapi(info.Env(), layer->this_->GetName());
}
Napi::Value LayerNapi::geomTypeGetter(const Napi::CallbackInfo &info) {
  NAPI_UNWRAP_THIS(LayerNapi, layer);
  return Napi::Number::New(info.Env(), layer->this_->GetGeomType());
}
Napi::Value LayerNapi::geomColumnGetter(const Napi::CallbackInfo &info) {
  NAPI_UNWRAP_THIS(LayerNapi, layer);
  return SafeStringNapi(info.Env(), layer->this_->GetGeometryColumn());
}
Napi::Value LayerNapi::fidColumnGetter(const Napi::CallbackInfo &info) {
  NAPI_UNWRAP_THIS(LayerNapi, layer);
  return SafeStringNapi(info.Env(), layer->this_->GetFIDColumn());
}

} // namespace node_gdal
