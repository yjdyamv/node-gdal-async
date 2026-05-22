#include "gdal_rasterband_napi.hpp"
#include "gdal_dataset_napi.hpp"
#include "utils/napi_object_store.hpp"
#include "gdal_stubs_napi.hpp"
#include <array>

namespace node_gdal {

Napi::FunctionReference RasterBandNapi::constructor;

// Macro helpers
#define RB_SIMPLE_GETTER(name, wrapped, ret_type)                                              \
  GDAL_ASYNCABLE_GETTER_DEFINE_NAPI(RasterBandNapi, name) {                                    \
    NAPI_UNWRAP_THIS(RasterBandNapi, band);                                                     \
    return ret_type::New(info.Env(), band->this_->wrapped());                                   \
  }

#define RB_STR_GETTER(name, wrapped)                                                           \
  GDAL_ASYNCABLE_GETTER_DEFINE_NAPI(RasterBandNapi, name) {                                    \
    NAPI_UNWRAP_THIS(RasterBandNapi, band);                                                     \
    return SafeStringNapi(info.Env(), band->this_->wrapped());                                  \
  }

#define RB_DOUBLE_GETTER(name, wrapped)                                                        \
  GDAL_ASYNCABLE_GETTER_DEFINE_NAPI(RasterBandNapi, name) {                                    \
    NAPI_UNWRAP_THIS(RasterBandNapi, band);                                                     \
    double val = band->this_->wrapped(nullptr);                                                 \
    return Napi::Number::New(info.Env(), val);                                                  \
  }

// ---------------------------------------------------------------------------
Napi::Object RasterBandNapi::Init(Napi::Env env, Napi::Object exports) {
  Napi::Function func = DefineClass(
    env, "RasterBand",
    {
      InstanceMethod("toString", &RasterBandNapi::toString),
      InstanceMethod("flush", &RasterBandNapi::flush),
      InstanceMethod("flushAsync", &RasterBandNapi::flushAsync),
      InstanceMethod("fill", &RasterBandNapi::fill),
      InstanceMethod("fillAsync", &RasterBandNapi::fillAsync),
      InstanceMethod("getStatistics", &RasterBandNapi::getStatistics),
      InstanceMethod("setStatistics", &RasterBandNapi::setStatistics),
      InstanceMethod("computeStatistics", &RasterBandNapi::computeStatistics),
      InstanceMethod("computeStatisticsAsync", &RasterBandNapi::computeStatisticsAsync),
      InstanceMethod("getMaskBand", &RasterBandNapi::getMaskBand),
      InstanceMethod("getMaskFlags", &RasterBandNapi::getMaskFlags),
      InstanceMethod("createMaskBand", &RasterBandNapi::createMaskBand),
      // Simple accessors
      InstanceAccessor<&RasterBandNapi::sizeGetter>("size"),
      InstanceAccessor<&RasterBandNapi::sizeGetterAsync>("sizeAsync"),
      InstanceAccessor<&RasterBandNapi::idGetter>("id"),
      InstanceAccessor<&RasterBandNapi::idGetterAsync>("idAsync"),
      InstanceAccessor<&RasterBandNapi::descriptionGetter>("description"),
      InstanceAccessor<&RasterBandNapi::descriptionGetterAsync>("descriptionAsync"),
      InstanceAccessor<&RasterBandNapi::blockSizeGetter>("blockSize"),
      InstanceAccessor<&RasterBandNapi::blockSizeGetterAsync>("blockSizeAsync"),
      InstanceAccessor<&RasterBandNapi::minimumGetter>("minimum"),
      InstanceAccessor<&RasterBandNapi::minimumGetterAsync>("minimumAsync"),
      InstanceAccessor<&RasterBandNapi::maximumGetter>("maximum"),
      InstanceAccessor<&RasterBandNapi::maximumGetterAsync>("maximumAsync"),
      InstanceAccessor<&RasterBandNapi::readOnlyGetter>("readOnly"),
      InstanceAccessor<&RasterBandNapi::readOnlyGetterAsync>("readOnlyAsync"),
      InstanceAccessor<&RasterBandNapi::dataTypeGetter>("dataType"),
      InstanceAccessor<&RasterBandNapi::dataTypeGetterAsync>("dataTypeAsync"),
      InstanceAccessor<&RasterBandNapi::unitTypeGetter, &RasterBandNapi::unitTypeSetter>("unitType"),
      InstanceAccessor<&RasterBandNapi::unitTypeGetterAsync>("unitTypeAsync"),
      InstanceAccessor<&RasterBandNapi::scaleGetter, &RasterBandNapi::scaleSetter>("scale"),
      InstanceAccessor<&RasterBandNapi::scaleGetterAsync>("scaleAsync"),
      InstanceAccessor<&RasterBandNapi::offsetGetter, &RasterBandNapi::offsetSetter>("offset"),
      InstanceAccessor<&RasterBandNapi::offsetGetterAsync>("offsetAsync"),
      InstanceAccessor<&RasterBandNapi::noDataValueGetter,
                        &RasterBandNapi::noDataValueSetter>("noDataValue"),
      InstanceAccessor<&RasterBandNapi::noDataValueGetterAsync>("noDataValueAsync"),
      InstanceAccessor<&RasterBandNapi::categoryNamesGetter,
                        &RasterBandNapi::categoryNamesSetter>("categoryNames"),
      InstanceAccessor<&RasterBandNapi::categoryNamesGetterAsync>("categoryNamesAsync"),
      InstanceAccessor<&RasterBandNapi::colorInterpretationGetter>("colorInterpretation"),
      InstanceAccessor<&RasterBandNapi::colorInterpretationGetterAsync>("colorInterpretationAsync"),
      InstanceAccessor<&RasterBandNapi::hasArbitraryOverviewsGetter>("hasArbitraryOverviews"),
      InstanceAccessor<&RasterBandNapi::hasArbitraryOverviewsGetterAsync>(
        "hasArbitraryOverviewsAsync"),
      InstanceAccessor<&RasterBandNapi::dsGetter>("ds"),
      InstanceAccessor<&RasterBandNapi::pixelsGetter>("pixels"),
      InstanceAccessor<&RasterBandNapi::overviewsGetter>("overviews"),
    });
  constructor = Napi::Persistent(func);
  constructor.SuppressDestruct();
  exports.Set("RasterBand", func);
  return exports;
}

// ---------------------------------------------------------------------------
RasterBandNapi::RasterBandNapi(const Napi::CallbackInfo &info)
  : Napi::ObjectWrap<RasterBandNapi>(info), this_(nullptr) {
  if (!info.IsConstructCall()) {
    Napi::Error::New(info.Env(), "Cannot call constructor as function, use 'new' keyword")
      .ThrowAsJavaScriptException();
    return;
  }
  if (info.Length() > 0 && info[0].IsExternal()) {
    this_ = info[0].As<Napi::External<GDALRasterBand>>().Data();
  } else {
    Napi::Error::New(info.Env(), "Cannot create RasterBand directly")
      .ThrowAsJavaScriptException();
  }
}
RasterBandNapi::~RasterBandNapi() { this_ = nullptr; }

Napi::Value RasterBandNapi::New(Napi::Env env, GDALRasterBand *band) {
  Napi::EscapableHandleScope scope(env);
  if (!band) return scope.Escape(env.Null());
  if (napi_obj_store_has<GDALRasterBand *>(band)) {
    Napi::Object existing = napi_obj_store_get<GDALRasterBand *>(env, band);
    if (!existing.IsEmpty()) return scope.Escape(existing);
  }
  Napi::Object obj = constructor.New({Napi::External<GDALRasterBand>::New(env, band)});
  napi_obj_store_add<GDALRasterBand *>(band, obj);
  return scope.Escape(obj);
}

Napi::Value RasterBandNapi::toString(const Napi::CallbackInfo &info) {
  return Napi::String::New(info.Env(), "RasterBand");
}

// ---------------------------------------------------------------------------
// Async methods
// ---------------------------------------------------------------------------
GDAL_ASYNCABLE_DEFINE_NAPI(RasterBandNapi, flush) {
  NAPI_UNWRAP_THIS(RasterBandNapi, band);
  GDALAsyncableJobNapi<int> job;
  job.main = [band]() { band->this_->FlushCache(); return 0; };
  job.rval = [](Napi::Env env, int) { return env.Undefined(); };
  return job.run(info, async, 0);
}
GDAL_ASYNCABLE_DEFINE_NAPI(RasterBandNapi, fill) {
  NAPI_UNWRAP_THIS(RasterBandNapi, band);
  double real, imag = 0;
  NAPI_ARG_DOUBLE(0, "real", real);
  NAPI_ARG_DOUBLE_OPT(1, "imag", imag);
  GDALAsyncableJobNapi<int> job;
  job.main = [band, real, imag]() { return band->this_->Fill(real, imag); };
  job.rval = [](Napi::Env env, int) { return env.Undefined(); };
  return job.run(info, async, 2);
}

// ---------------------------------------------------------------------------
// getStatistics / setStatistics / computeStatistics
// ---------------------------------------------------------------------------
Napi::Value RasterBandNapi::getStatistics(const Napi::CallbackInfo &info) {
  NAPI_UNWRAP_THIS(RasterBandNapi, band);
  int approx_ok = 0, force = 0;
  NAPI_ARG_BOOL_OPT(0, "approx_ok", approx_ok);
  NAPI_ARG_BOOL_OPT(1, "force", force);
  double min, max, mean, stddev;
  CPLErr err = band->this_->GetStatistics(approx_ok, force, &min, &max, &mean, &stddev);
  if (err) {
    Napi::Error::New(info.Env(), CPLGetLastErrorMsg()).ThrowAsJavaScriptException();
    return info.Env().Undefined();
  }
  Napi::Object result = Napi::Object::New(info.Env());
  result.Set("min", Napi::Number::New(info.Env(), min));
  result.Set("max", Napi::Number::New(info.Env(), max));
  result.Set("mean", Napi::Number::New(info.Env(), mean));
  result.Set("stddev", Napi::Number::New(info.Env(), stddev));
  return result;
}
GDAL_ASYNCABLE_DEFINE_NAPI(RasterBandNapi, computeStatistics) {
  NAPI_UNWRAP_THIS(RasterBandNapi, band);
  int approx_ok = 0;
  NAPI_ARG_BOOL_OPT(0, "approx_ok", approx_ok);
  GDALAsyncableJobNapi<std::array<double, 4>> job;
  job.main = [band, approx_ok]() -> std::array<double, 4> {
    double min, max, mean, stddev;
    band->this_->ComputeStatistics(approx_ok, &min, &max, &mean, &stddev, nullptr, nullptr);
    return {min, max, mean, stddev};
  };
  job.rval = [](Napi::Env env, std::array<double, 4> s) -> Napi::Value {
    Napi::Object r = Napi::Object::New(env);
    r.Set("min", Napi::Number::New(env, s[0]));
    r.Set("max", Napi::Number::New(env, s[1]));
    r.Set("mean", Napi::Number::New(env, s[2]));
    r.Set("stddev", Napi::Number::New(env, s[3]));
    return r;
  };
  return job.run(info, async, 1);
}
Napi::Value RasterBandNapi::setStatistics(const Napi::CallbackInfo &info) {
  NAPI_UNWRAP_THIS(RasterBandNapi, band);
  double min, max, mean, stddev;
  NAPI_ARG_DOUBLE(0, "min", min);
  NAPI_ARG_DOUBLE(1, "max", max);
  NAPI_ARG_DOUBLE(2, "mean", mean);
  NAPI_ARG_DOUBLE(3, "stddev", stddev);
  band->this_->SetStatistics(min, max, mean, stddev);
  return info.Env().Undefined();
}
Napi::Value RasterBandNapi::getMaskBand(const Napi::CallbackInfo &info) {
  NAPI_UNWRAP_THIS(RasterBandNapi, band);
  GDALRasterBand *mask = band->this_->GetMaskBand();
  if (!mask) return info.Env().Null();
  return RasterBandNapi::New(info.Env(), mask);
}
Napi::Value RasterBandNapi::getMaskFlags(const Napi::CallbackInfo &info) {
  NAPI_UNWRAP_THIS(RasterBandNapi, band);
  return Napi::Number::New(info.Env(), band->this_->GetMaskFlags());
}
Napi::Value RasterBandNapi::createMaskBand(const Napi::CallbackInfo &info) {
  NAPI_UNWRAP_THIS(RasterBandNapi, band);
  int flags;
  NAPI_ARG_INT(0, "flags", flags);
  CPLErr err = band->this_->CreateMaskBand(flags);
  if (err) NAPI_THROW_LAST_CPLERR;
  return info.Env().Undefined();
}

// ---------------------------------------------------------------------------
// Simple getters
// ---------------------------------------------------------------------------
GDAL_ASYNCABLE_GETTER_DEFINE_NAPI(RasterBandNapi, sizeGetter) {
  NAPI_UNWRAP_THIS(RasterBandNapi, band);
  Napi::Object r = Napi::Object::New(info.Env());
  r.Set("x", Napi::Number::New(info.Env(), band->this_->GetXSize()));
  r.Set("y", Napi::Number::New(info.Env(), band->this_->GetYSize()));
  return r;
}
RB_SIMPLE_GETTER(idGetter, GetBand, Napi::Number)
RB_STR_GETTER(descriptionGetter, GetDescription)
GDAL_ASYNCABLE_GETTER_DEFINE_NAPI(RasterBandNapi, blockSizeGetter) {
  NAPI_UNWRAP_THIS(RasterBandNapi, band);
  int x, y;
  band->this_->GetBlockSize(&x, &y);
  Napi::Object r = Napi::Object::New(info.Env());
  r.Set("x", Napi::Number::New(info.Env(), x));
  r.Set("y", Napi::Number::New(info.Env(), y));
  return r;
}
RB_DOUBLE_GETTER(minimumGetter, GetMinimum)
RB_DOUBLE_GETTER(maximumGetter, GetMaximum)
RB_SIMPLE_GETTER(readOnlyGetter, GetAccess, Napi::Number) // 0=readOnly
GDAL_ASYNCABLE_GETTER_DEFINE_NAPI(RasterBandNapi, dataTypeGetter) {
  NAPI_UNWRAP_THIS(RasterBandNapi, band);
  GDALDataType type = band->this_->GetRasterDataType();
  return SafeStringNapi(info.Env(), GDALGetDataTypeName(type));
}
RB_SIMPLE_GETTER(hasArbitraryOverviewsGetter, HasArbitraryOverviews, Napi::Boolean)
RB_STR_GETTER(unitTypeGetter, GetUnitType)
RB_DOUBLE_GETTER(scaleGetter, GetScale)
RB_DOUBLE_GETTER(offsetGetter, GetOffset)
GDAL_ASYNCABLE_GETTER_DEFINE_NAPI(RasterBandNapi, noDataValueGetter) {
  NAPI_UNWRAP_THIS(RasterBandNapi, band);
  int hasVal;
  double val = band->this_->GetNoDataValue(&hasVal);
  if (!hasVal) return info.Env().Null();
  return Napi::Number::New(info.Env(), val);
}
GDAL_ASYNCABLE_GETTER_DEFINE_NAPI(RasterBandNapi, categoryNamesGetter) {
  NAPI_UNWRAP_THIS(RasterBandNapi, band);
  char **names = band->this_->GetCategoryNames();
  Napi::Array r = Napi::Array::New(info.Env());
  if (names) {
    for (int i = 0; names[i]; i++) r.Set(i, Napi::String::New(info.Env(), names[i]));
  }
  return r;
}
RB_SIMPLE_GETTER(colorInterpretationGetter, GetColorInterpretation, Napi::Number)

// ---------------------------------------------------------------------------
// Setters
// ---------------------------------------------------------------------------
void RasterBandNapi::unitTypeSetter(const Napi::CallbackInfo &info, const Napi::Value &value) {
  NAPI_UNWRAP_THIS_VOID(RasterBandNapi, band);
  if (!value.IsString()) {
    Napi::Error::New(info.Env(), "unitType must be a string").ThrowAsJavaScriptException();
    return;
  }
  band->this_->SetUnitType(value.As<Napi::String>().Utf8Value().c_str());
}
void RasterBandNapi::scaleSetter(const Napi::CallbackInfo &info, const Napi::Value &value) {
  NAPI_UNWRAP_THIS_VOID(RasterBandNapi, band);
  if (!value.IsNumber()) {
    Napi::Error::New(info.Env(), "scale must be a number").ThrowAsJavaScriptException();
    return;
  }
  band->this_->SetScale(value.As<Napi::Number>().DoubleValue());
}
void RasterBandNapi::offsetSetter(const Napi::CallbackInfo &info, const Napi::Value &value) {
  NAPI_UNWRAP_THIS_VOID(RasterBandNapi, band);
  if (!value.IsNumber()) {
    Napi::Error::New(info.Env(), "offset must be a number").ThrowAsJavaScriptException();
    return;
  }
  band->this_->SetOffset(value.As<Napi::Number>().DoubleValue());
}
void RasterBandNapi::noDataValueSetter(const Napi::CallbackInfo &info, const Napi::Value &value) {
  NAPI_UNWRAP_THIS_VOID(RasterBandNapi, band);
  if (value.IsNull() || value.IsUndefined()) {
    band->this_->DeleteNoDataValue();
  } else if (value.IsNumber()) {
    band->this_->SetNoDataValue(value.As<Napi::Number>().DoubleValue());
  } else {
    Napi::Error::New(info.Env(), "noDataValue must be a number or null").ThrowAsJavaScriptException();
  }
}
void RasterBandNapi::categoryNamesSetter(const Napi::CallbackInfo &info, const Napi::Value &value) {
  NAPI_UNWRAP_THIS_VOID(RasterBandNapi, band);
  if (!value.IsArray()) {
    Napi::Error::New(info.Env(), "categoryNames must be an array of strings").ThrowAsJavaScriptException();
    return;
  }
  Napi::Array arr = value.As<Napi::Array>();
  char **names = new char *[arr.Length() + 1];
  for (uint32_t i = 0; i < arr.Length(); i++) {
    names[i] = (char *)arr.Get(i).As<Napi::String>().Utf8Value().c_str();
  }
  names[arr.Length()] = nullptr;
  band->this_->SetCategoryNames(names);
  delete[] names;
}

Napi::Value RasterBandNapi::dsGetter(const Napi::CallbackInfo &info) {
  NAPI_UNWRAP_THIS(RasterBandNapi, band);
  GDALDataset *ds = band->this_->GetDataset();
  return ds ? DatasetNapi::New(info.Env(), ds) : info.Env().Null();
}

Napi::Value RasterBandNapi::pixelsGetter(const Napi::CallbackInfo &info) {
  NAPI_UNWRAP_THIS(RasterBandNapi, band);
  Napi::Object thiz = info.This().As<Napi::Object>();
  if (thiz.Has("__pixels")) { Napi::Value c = thiz.Get("__pixels"); if (!c.IsNull() && !c.IsUndefined()) return c; }
  Napi::Object px = RasterBandPixelsNapi::constructor.New({
    Napi::External<GDALRasterBand>::New(info.Env(), band->this_)
  });
  thiz.Set("__pixels", px); return px;
}

Napi::Value RasterBandNapi::overviewsGetter(const Napi::CallbackInfo &info) {
  NAPI_UNWRAP_THIS(RasterBandNapi, band);
  Napi::Object thiz = info.This().As<Napi::Object>();
  if (thiz.Has("__overviews")) { Napi::Value c = thiz.Get("__overviews"); if (!c.IsNull() && !c.IsUndefined()) return c; }
  Napi::Object ov = RasterBandOverviewsNapi::constructor.New({
    Napi::External<GDALRasterBand>::New(info.Env(), band->this_)
  });
  thiz.Set("__overviews", ov); return ov;
}

} // namespace node_gdal
