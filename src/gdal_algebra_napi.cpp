#include "gdal_algebra_napi.hpp"
#include "gdal_rasterband_napi.hpp"
#include "gdal_dataset_napi.hpp"
#include "async_napi.hpp"
#include <gdal_computedrasterband.h>
#include <cmath>

#if GDAL_VERSION_MAJOR > 3 || (GDAL_VERSION_MAJOR == 3 && GDAL_VERSION_MINOR >= 12)

namespace node_gdal {

// === small helpers (avoid macros, avoid "info" param conflict) ===

static inline Napi::Value AlgebraRVal(Napi::Env env, GDALRasterBand *r) {
  if (!r) return env.Null();
  DatasetNapi::New(env, r->GetDataset(), false); // computed datasets should not be closed
  return RasterBandNapi::New(env, r);
}

static inline GDALRasterBand *unwrapBand(Napi::Value v) {
  if (!v.IsObject()) return nullptr;
  auto obj = v.As<Napi::Object>();
  if (!obj.InstanceOf(RasterBandNapi::constructor.Value())) return nullptr;
  auto *b = RasterBandNapi::Unwrap(obj);
  return (b && b->isAlive()) ? b->get() : nullptr;
}

static inline bool isBand(Napi::Value v) {
  return v.IsObject() && v.As<Napi::Object>().InstanceOf(RasterBandNapi::constructor.Value());
}

using AlgebraMain = std::function<GDALRasterBand *()>;

static Napi::Value dispatch(const Napi::CallbackInfo &cb, bool async, int cbIdx, AlgebraMain m) {
  GDALAsyncableJobNapi<GDALRasterBand *> job;
  job.main = std::move(m);
  job.rval = AlgebraRVal;
  return job.run(cb, async, cbIdx);
}

// === unary ops (abs, sqrt, log, log10, not) ===

template <typename F>
static Napi::Value unary(const Napi::CallbackInfo &cb, bool async, F op) {
  GDALRasterBand *r = unwrapBand(cb[0]);
  if (!r) { Napi::TypeError::New(cb.Env(), "Argument must be an instance of RasterBand").ThrowAsJavaScriptException(); return cb.Env().Undefined(); }
  return dispatch(cb, async, 1, [r, op]() { return new GDALComputedRasterBand(op(*r)); });
}

static Napi::Value algebra_abs(const Napi::CallbackInfo &cb)   { return unary(cb, false, [](auto &b){ return gdal::abs(b); }); }
static Napi::Value algebra_absAsync(const Napi::CallbackInfo &cb){ return unary(cb, true,  [](auto &b){ return gdal::abs(b); }); }
static Napi::Value algebra_sqrt(const Napi::CallbackInfo &cb)   { return unary(cb, false, [](auto &b){ return gdal::sqrt(b); }); }
static Napi::Value algebra_sqrtAsync(const Napi::CallbackInfo &cb){ return unary(cb, true,  [](auto &b){ return gdal::sqrt(b); }); }
static Napi::Value algebra_log(const Napi::CallbackInfo &cb)   { return unary(cb, false, [](auto &b){ return gdal::log(b); }); }
static Napi::Value algebra_logAsync(const Napi::CallbackInfo &cb){ return unary(cb, true,  [](auto &b){ return gdal::log(b); }); }
static Napi::Value algebra_log10(const Napi::CallbackInfo &cb)   { return unary(cb, false, [](auto &b){ return gdal::log10(b); }); }
static Napi::Value algebra_log10Async(const Napi::CallbackInfo &cb){ return unary(cb, true,  [](auto &b){ return gdal::log10(b); }); }
static Napi::Value algebra_not(const Napi::CallbackInfo &cb)   { return unary(cb, false, [](auto &b){ return !b; }); }
static Napi::Value algebra_notAsync(const Napi::CallbackInfo &cb){ return unary(cb, true,  [](auto &b){ return !b; }); }

// === binary ops (add, sub, mul, div, lt, lte, gt, gte, eq, notEq, and, or, pow) ===

template <typename F>
static Napi::Value binary(const Napi::CallbackInfo &cb, bool async, F op) {
  double n1=NAN, n2=NAN;
  GDALRasterBand *r1=nullptr, *r2=nullptr;
  if (isBand(cb[0])) r1=unwrapBand(cb[0]); else if (cb[0].IsNumber()) n1=cb[0].As<Napi::Number>().DoubleValue();
  else { Napi::TypeError::New(cb.Env(),"Argument must be either a number or a RasterBand").ThrowAsJavaScriptException(); return cb.Env().Undefined(); }
  if (isBand(cb[1])) r2=unwrapBand(cb[1]); else if (cb[1].IsNumber()) n2=cb[1].As<Napi::Number>().DoubleValue();
  else { Napi::TypeError::New(cb.Env(),"Argument must be either a number or a RasterBand").ThrowAsJavaScriptException(); return cb.Env().Undefined(); }
  if (!r1 && !r2) { Napi::Error::New(cb.Env(),"At least one RasterBand must be given").ThrowAsJavaScriptException(); return cb.Env().Undefined(); }

  AlgebraMain m;
  if (r1 && r2) { m = [r1,r2,op]() { return new GDALComputedRasterBand(op(*r1, *r2)); }; }
  else if (r1)  { m = [r1,n2,op]() { return new GDALComputedRasterBand(op(*r1, n2)); }; }
  else           { m = [n1,r2,op]() { return new GDALComputedRasterBand(op(n1, *r2)); }; }
  return dispatch(cb, async, 2, m);
}

static Napi::Value algebra_add(const Napi::CallbackInfo &cb){return binary(cb,false,[](const auto &a, const auto &b){return a+b;});}
static Napi::Value algebra_addAsync(const Napi::CallbackInfo &cb){return binary(cb,true,[](const auto &a, const auto &b){return a+b;});}
static Napi::Value algebra_sub(const Napi::CallbackInfo &cb){return binary(cb,false,[](const auto &a, const auto &b){return a-b;});}
static Napi::Value algebra_subAsync(const Napi::CallbackInfo &cb){return binary(cb,true,[](const auto &a, const auto &b){return a-b;});}
static Napi::Value algebra_mul(const Napi::CallbackInfo &cb){return binary(cb,false,[](const auto &a, const auto &b){return a*b;});}
static Napi::Value algebra_mulAsync(const Napi::CallbackInfo &cb){return binary(cb,true,[](const auto &a, const auto &b){return a*b;});}
static Napi::Value algebra_div(const Napi::CallbackInfo &cb){return binary(cb,false,[](const auto &a, const auto &b){return a/b;});}
static Napi::Value algebra_divAsync(const Napi::CallbackInfo &cb){return binary(cb,true,[](const auto &a, const auto &b){return a/b;});}
static Napi::Value algebra_lt(const Napi::CallbackInfo &cb){return binary(cb,false,[](const auto &a, const auto &b){return a<b;});}
static Napi::Value algebra_ltAsync(const Napi::CallbackInfo &cb){return binary(cb,true,[](const auto &a, const auto &b){return a<b;});}
static Napi::Value algebra_lte(const Napi::CallbackInfo &cb){return binary(cb,false,[](const auto &a, const auto &b){return a<=b;});}
static Napi::Value algebra_lteAsync(const Napi::CallbackInfo &cb){return binary(cb,true,[](const auto &a, const auto &b){return a<=b;});}
static Napi::Value algebra_gt(const Napi::CallbackInfo &cb){return binary(cb,false,[](const auto &a, const auto &b){return a>b;});}
static Napi::Value algebra_gtAsync(const Napi::CallbackInfo &cb){return binary(cb,true,[](const auto &a, const auto &b){return a>b;});}
static Napi::Value algebra_gte(const Napi::CallbackInfo &cb){return binary(cb,false,[](const auto &a, const auto &b){return a>=b;});}
static Napi::Value algebra_gteAsync(const Napi::CallbackInfo &cb){return binary(cb,true,[](const auto &a, const auto &b){return a>=b;});}
static Napi::Value algebra_eq(const Napi::CallbackInfo &cb){return binary(cb,false,[](const auto &a, const auto &b){return a==b;});}
static Napi::Value algebra_eqAsync(const Napi::CallbackInfo &cb){return binary(cb,true,[](const auto &a, const auto &b){return a==b;});}
static Napi::Value algebra_notEq(const Napi::CallbackInfo &cb){return binary(cb,false,[](const auto &a, const auto &b){return a!=b;});}
static Napi::Value algebra_notEqAsync(const Napi::CallbackInfo &cb){return binary(cb,true,[](const auto &a, const auto &b){return a!=b;});}
static Napi::Value algebra_and(const Napi::CallbackInfo &cb){return binary(cb,false,[](const auto &a, const auto &b){return a&&b;});}
static Napi::Value algebra_andAsync(const Napi::CallbackInfo &cb){return binary(cb,true,[](const auto &a, const auto &b){return a&&b;});}
static Napi::Value algebra_or(const Napi::CallbackInfo &cb){return binary(cb,false,[](const auto &a, const auto &b){return a||b;});}
static Napi::Value algebra_orAsync(const Napi::CallbackInfo &cb){return binary(cb,true,[](const auto &a, const auto &b){return a||b;});}
static Napi::Value algebra_pow(const Napi::CallbackInfo &cb){return binary(cb,false,[](const auto &a, const auto &b){return gdal::pow(a,b);});}
static Napi::Value algebra_powAsync(const Napi::CallbackInfo &cb){return binary(cb,true,[](const auto &a, const auto &b){return gdal::pow(a,b);});}

// === variadic ops (min, max, mean) ===

template <typename F>
static Napi::Value variadic(const Napi::CallbackInfo &cb, bool async, F fn) {
  std::vector<GDALRasterBandH> handles;
  for (int i = 0; i < (int)cb.Length() && !cb[i].IsFunction(); i++) {
    GDALRasterBand *r = unwrapBand(cb[i]);
    if (!r) { Napi::TypeError::New(cb.Env(), "Argument must be an instance of RasterBand").ThrowAsJavaScriptException(); return cb.Env().Undefined(); }
    handles.push_back(GDALRasterBand::ToHandle(r));
  }
  if (handles.size() < 2) { Napi::Error::New(cb.Env(),"At least two arguments must be given").ThrowAsJavaScriptException(); return cb.Env().Undefined(); }
  int n = (int)handles.size();
  auto *h = new GDALRasterBandH[n]; std::copy(handles.begin(), handles.end(), h);
  GDALAsyncableJobNapi<GDALRasterBand *> job;
  job.main = [h, n, fn]() { auto *r = fn(n, h); delete[] h; return r; };
  job.rval = AlgebraRVal;
  return job.run(cb, async, n);
}

static Napi::Value algebra_min(const Napi::CallbackInfo &cb){return variadic(cb,false,[](int n,GDALRasterBandH*h){return GDALRasterBand::FromHandle(GDALMinimumOfNBands(n,h));});}
static Napi::Value algebra_minAsync(const Napi::CallbackInfo &cb){return variadic(cb,true,[](int n,GDALRasterBandH*h){return GDALRasterBand::FromHandle(GDALMinimumOfNBands(n,h));});}
static Napi::Value algebra_max(const Napi::CallbackInfo &cb){return variadic(cb,false,[](int n,GDALRasterBandH*h){return GDALRasterBand::FromHandle(GDALMaximumOfNBands(n,h));});}
static Napi::Value algebra_maxAsync(const Napi::CallbackInfo &cb){return variadic(cb,true,[](int n,GDALRasterBandH*h){return GDALRasterBand::FromHandle(GDALMaximumOfNBands(n,h));});}
static Napi::Value algebra_mean(const Napi::CallbackInfo &cb){return variadic(cb,false,[](int n,GDALRasterBandH*h){return GDALRasterBand::FromHandle(GDALMeanOfNBands(n,h));});}
static Napi::Value algebra_meanAsync(const Napi::CallbackInfo &cb){return variadic(cb,true,[](int n,GDALRasterBandH*h){return GDALRasterBand::FromHandle(GDALMeanOfNBands(n,h));});}

// === ifThenElse ===

static Napi::Value algebra_ifThenElse(const Napi::CallbackInfo &cb) {
  GDALRasterBand *r1=nullptr,*r2=nullptr,*r3=nullptr; double n1=NAN,n2=NAN,n3=NAN;
  if (isBand(cb[0])) r1=unwrapBand(cb[0]); else if (cb[0].IsNumber()) n1=cb[0].As<Napi::Number>().DoubleValue();
  else { Napi::Error::New(cb.Env(),"Argument must be either a number or a RasterBand").ThrowAsJavaScriptException(); return cb.Env().Undefined(); }
  if (isBand(cb[1])) r2=unwrapBand(cb[1]); else if (cb[1].IsNumber()) n2=cb[1].As<Napi::Number>().DoubleValue();
  else { Napi::Error::New(cb.Env(),"Argument must be either a number or a RasterBand").ThrowAsJavaScriptException(); return cb.Env().Undefined(); }
  if (isBand(cb[2])) r3=unwrapBand(cb[2]); else if (cb[2].IsNumber()) n3=cb[2].As<Napi::Number>().DoubleValue();
  else { Napi::Error::New(cb.Env(),"Argument must be either a number or a RasterBand").ThrowAsJavaScriptException(); return cb.Env().Undefined(); }

  AlgebraMain m;
  if (r1&&r2&&r3) { m=[r1,r2,r3](){return new GDALComputedRasterBand(gdal::IfThenElse(*r1,*r2,*r3));}; }
  else if(r1&&r2)  { m=[r1,r2,n3](){return new GDALComputedRasterBand(gdal::IfThenElse(*r1,*r2,n3));}; }
  else if(r1&&r3)  { m=[r1,n2,r3](){return new GDALComputedRasterBand(gdal::IfThenElse(*r1,n2,*r3));}; }
  else { Napi::Error::New(cb.Env(),"Need at least 2 RasterBands").ThrowAsJavaScriptException(); return cb.Env().Undefined(); }
  return dispatch(cb, false, 3, m);
}
static Napi::Value algebra_ifThenElseAsync(const Napi::CallbackInfo &cb){ return algebra_ifThenElse(cb); }

// === asType ===

static Napi::Value algebra_asType(const Napi::CallbackInfo &cb) {
  GDALRasterBand *r = unwrapBand(cb[0]);
  if (!r) { Napi::TypeError::New(cb.Env(),"Argument must be an instance of RasterBand").ThrowAsJavaScriptException(); return cb.Env().Undefined(); }
  std::string tn = cb[1].As<Napi::String>().Utf8Value();
  GDALDataType t = GDALGetDataTypeByName(tn.c_str());
  if (t == GDT_Unknown) { Napi::Error::New(cb.Env(),"Invalid data type").ThrowAsJavaScriptException(); return cb.Env().Undefined(); }
  return dispatch(cb, false, 2, [r,t](){ return new GDALComputedRasterBand(r->AsType(t)); });
}
static Napi::Value algebra_asTypeAsync(const Napi::CallbackInfo &cb){ return algebra_asType(cb); }

// === Init ===

Napi::Object AlgebraNapi::Init(Napi::Env env, Napi::Object exports) {
  Napi::Object a = Napi::Object::New(env);
  #define SET(fn) a.Set(#fn, Napi::Function::New(env, algebra_##fn)); a.Set(#fn "Async", Napi::Function::New(env, algebra_##fn##Async));
  SET(abs); SET(sqrt); SET(log); SET(log10); SET(not);
  SET(add); SET(sub); SET(mul); SET(div); SET(pow);
  SET(lt); SET(lte); SET(gt); SET(gte); SET(eq); SET(notEq); SET(and); SET(or);
  SET(min); SET(max); SET(mean);
  SET(ifThenElse); SET(asType);
  #undef SET
  if (!exports.Has("algebra")) exports.Set("algebra", a);
  return exports;
}

} // namespace node_gdal

#else
namespace node_gdal {
Napi::Object AlgebraNapi::Init(Napi::Env env, Napi::Object exports) {
  Napi::Object a = Napi::Object::New(env);
  if (!exports.Has("algebra")) exports.Set("algebra", a);
  return exports;
}
} // namespace node_gdal
#endif
