#include "gdal_algorithms_napi.hpp"
#include "gdal_rasterband_napi.hpp"
#include "gdal_layer_napi.hpp"
#include "gdal_common_napi.hpp"
#include "async_napi.hpp"
#include <gdal_alg.h>
#include <uv.h>
#include <thread>
#include "../include/node_gdal.h"

namespace node_gdal {

extern std::thread::id mainV8ThreadId;  // defined in node_gdal.cpp

// Helper: create a TypedArray of the appropriate type for the given GDALDataType
static Napi::TypedArray NewTypedArrayForPixel(Napi::Env env, GDALDataType type, size_t elements) {
  switch (type) {
    case GDT_Byte:
    case GDT_Int8:   return Napi::Int8Array::New(env, elements);
    case GDT_UInt16: return Napi::Uint16Array::New(env, elements);
    case GDT_Int16:  return Napi::Int16Array::New(env, elements);
    case GDT_UInt32: return Napi::Uint32Array::New(env, elements);
    case GDT_Int32:  return Napi::Int32Array::New(env, elements);
    case GDT_Float32:return Napi::Float32Array::New(env, elements);
    case GDT_Float64:return Napi::Float64Array::New(env, elements);
    default:         return Napi::Float32Array::New(env, elements);
  }
}

// =========================================================================
// Pixel Function Infrastructure (GDAL >= 3.5)
// =========================================================================
#if GDAL_VERSION_MAJOR > 3 || (GDAL_VERSION_MAJOR == 3 && GDAL_VERSION_MINOR >= 5)

struct pixelFnCallNapi {
  void **sources;
  size_t num;
  void *destination;
  int width, height;
  GDALDataType inType;
  GDALDataType outType;
  std::map<std::string, std::string> args;
  std::string *err;
};

struct pixelFnNapi {
  Napi::FunctionReference fn;
  pixelFnCallNapi call;
  uv_mutex_t callJS;
  uv_sem_t returnJS;
  uv_async_t *async;
};

std::vector<pixelFnNapi> pixelFuncsNapi;

#define PFN_ID_FIELD "node_gdal_pfn_id"
const char metadataTemplateNapi[] =
  "<PixelFunctionArgumentsList>\n"
  "   <Argument name ='" PFN_ID_FIELD
  "' type='constant' value='%x' />\n"
  "</PixelFunctionArgumentsList>";

static void callJSpfnNapi(uv_async_t *async) {
  pixelFnNapi *fn = reinterpret_cast<pixelFnNapi *>(async->data);
  Napi::Env env = fn->fn.Env();
  Napi::HandleScope scope(env);

  Napi::Array sources = Napi::Array::New(env, fn->call.num);
  size_t len = fn->call.width * fn->call.height;
  for (size_t i = 0; i < fn->call.num; i++) {
    size_t byteLen = len * GDALGetDataTypeSizeBytes(fn->call.inType);
    Napi::TypedArray arr = NewTypedArrayForPixel(env, fn->call.inType, len);
    memcpy(arr.ArrayBuffer().Data(), fn->call.sources[i], byteLen);
    sources.Set(i, arr);
  }

  Napi::TypedArray destination = NewTypedArrayForPixel(env, fn->call.outType, len);
  memcpy(destination.ArrayBuffer().Data(), fn->call.destination,
         len * GDALGetDataTypeSizeBytes(fn->call.outType));

  Napi::Number width = Napi::Number::New(env, fn->call.width);
  Napi::Number height = Napi::Number::New(env, fn->call.height);

  Napi::Object pfArgs = Napi::Object::New(env);
  if (!fn->call.args.empty()) {
    for (auto const &el : fn->call.args) {
      char *end;
      double dval = std::strtod(el.second.c_str(), &end);
      if (*end == 0)
        pfArgs.Set(el.first, Napi::Number::New(env, dval));
      else
        pfArgs.Set(el.first, Napi::String::New(env, el.second));
    }
  }

  Napi::Value args[] = {sources, destination, pfArgs, width, height};

  fn->call.err = nullptr;
  try {
    fn->fn.Call({sources, destination, pfArgs, width, height});
  } catch (Napi::Error &e) {
    fn->call.err = new std::string(e.Message());
  } catch (...) {
    fn->call.err = new std::string("Unknown pixel function error");
  }
  uv_sem_post(&fn->returnJS);
}

static CPLErr pixelFuncNapi(
  void **papoSources, int nSources, void *pData,
  int nBufXSize, int nBufYSize,
  GDALDataType eSrcType, GDALDataType eBufType,
  int nPixelSpace, int nLineSpace,
  CSLConstList papszFunctionArgs) {

  std::map<std::string, std::string> pfArgsMap;
  ParseCSLConstList(papszFunctionArgs, pfArgsMap);

  auto uid = pfArgsMap.find(PFN_ID_FIELD);
  if (uid == pfArgsMap.end()) {
    CPLError(CE_Failure, CPLE_AppDefined, "gdal-async Internal error, pixelFuncs inconsistency, id=NULL");
    return CE_Failure;
  }
  char *end;
  size_t id = std::strtoul(uid->second.c_str(), &end, 16);
  if (end == uid->second.c_str() || id >= pixelFuncsNapi.size()) {
    CPLError(CE_Failure, CPLE_AppDefined, "gdal-async Internal error, pixelFuncs inconsistency");
    return CE_Failure;
  }
  pfArgsMap.erase(PFN_ID_FIELD);

  size_t size = GDALGetDataTypeSizeBytes(eBufType);
  if (size == 0) {
    CPLError(CE_Failure, CPLE_AppDefined, "Invalid GDAL data type");
    return CE_Failure;
  }
  if (size != static_cast<size_t>(nPixelSpace) ||
      size * static_cast<size_t>(nBufXSize) != static_cast<size_t>(nLineSpace)) {
    CPLError(CE_Failure, CPLE_AppDefined, "gdal-async still does not support irregular buffer strides");
    return CE_Failure;
  }

  uv_mutex_lock(&pixelFuncsNapi[id].callJS);

  uv_async_t *async = pixelFuncsNapi[id].async;
  async->data = &pixelFuncsNapi[id];

  pixelFuncsNapi[id].call = {
    papoSources, static_cast<size_t>(nSources), pData,
    nBufXSize, nBufYSize, eSrcType, eBufType,
    std::move(pfArgsMap), nullptr};

  if (std::this_thread::get_id() == mainV8ThreadId) {
    callJSpfnNapi(async);
  } else {
    int s = uv_async_send(async);
    if (s != 0) {
      CPLError(CE_Failure, CPLE_AppDefined, "Pixel function error: failed scheduling async");
      return CE_Failure;
    }
    uv_sem_wait(&pixelFuncsNapi[id].returnJS);
  }

  uv_mutex_unlock(&pixelFuncsNapi[id].callJS);

  if (pixelFuncsNapi[id].call.err != nullptr) {
    CPLError(CE_Failure, CPLE_AppDefined, "Pixel function error: %s", pixelFuncsNapi[id].call.err->c_str());
    delete pixelFuncsNapi[id].call.err;
    pixelFuncsNapi[id].call.err = nullptr;
    return CE_Failure;
  }

  return CE_None;
}
#endif // GDAL >= 3.5

// =========================================================================
// addPixelFunc
// =========================================================================

Napi::Value AlgorithmsNapi::addPixelFunc(const Napi::CallbackInfo &info) {
  Napi::Env env = info.Env();
  std::string name;
  NAPI_ARG_STR(0, "name", name);

  if (info.Length() < 2 || info[1].IsNull() || info[1].IsUndefined() || !info[1].IsTypedArray()) {
    Napi::TypeError::New(env, "pixelFn must be an object").ThrowAsJavaScriptException();
    return env.Undefined();
  }

  Napi::TypedArray arr = info[1].As<Napi::TypedArray>();
  if (arr.ByteLength() < 8) {
    Napi::TypeError::New(env, "pixelFn must be a native code pixel function").ThrowAsJavaScriptException();
    return env.Undefined();
  }
  uint64_t magic = *reinterpret_cast<uint64_t *>(static_cast<char *>(arr.ArrayBuffer().Data()) + arr.ByteOffset());
  if (magic != NODE_GDAL_CAPI_MAGIC) {
    Napi::TypeError::New(env, "pixelFn must be a native code pixel function").ThrowAsJavaScriptException();
    return env.Undefined();
  }

#if GDAL_VERSION_MAJOR > 3 || (GDAL_VERSION_MAJOR == 3 && GDAL_VERSION_MINOR >= 5)
  pixel_func *desc = reinterpret_cast<pixel_func *>(
    static_cast<char *>(arr.ArrayBuffer().Data()) + arr.ByteOffset());
  CPLErr err = GDALAddDerivedBandPixelFuncWithArgs(name.c_str(), desc->fn, desc->metadata);
  if (err != CE_None) NAPI_THROW_LAST_CPLERR;
#else
  Napi::Error::New(env, "Custom pixel functions require GDAL >= 3.5").ThrowAsJavaScriptException();
#endif
  return env.Undefined();
}

// =========================================================================
// toPixelFunc
// =========================================================================

Napi::Value AlgorithmsNapi::toPixelFunc(const Napi::CallbackInfo &info) {
  Napi::Env env = info.Env();

#if GDAL_VERSION_MAJOR > 3 || (GDAL_VERSION_MAJOR == 3 && GDAL_VERSION_MINOR >= 5)
  if (info.Length() < 1 || !info[0].IsFunction()) {
    Napi::TypeError::New(env, "pixelFn must be a function").ThrowAsJavaScriptException();
    return env.Undefined();
  }
  Napi::Function pfn = info[0].As<Napi::Function>();

  size_t uid = pixelFuncsNapi.size();
  pixelFuncsNapi.push_back({});
  pixelFuncsNapi[uid].fn = Napi::Persistent(pfn);
  pixelFuncsNapi[uid].call = {};
  int s = uv_mutex_init(&pixelFuncsNapi[uid].callJS);
  if (s != 0) { Napi::Error::New(env, "Failed creating a mutex").ThrowAsJavaScriptException(); return env.Undefined(); }
  s = uv_sem_init(&pixelFuncsNapi[uid].returnJS, 0);
  if (s != 0) { Napi::Error::New(env, "Failed creating a semaphore").ThrowAsJavaScriptException(); return env.Undefined(); }
  pixelFuncsNapi[uid].async = new uv_async_t;
  s = uv_async_init(uv_default_loop(), pixelFuncsNapi[uid].async, callJSpfnNapi);
  if (s != 0) { Napi::Error::New(env, "Failed creating libuv async").ThrowAsJavaScriptException(); return env.Undefined(); }
  uv_unref(reinterpret_cast<uv_handle_t *>(pixelFuncsNapi[uid].async));

  std::string metadata;
  metadata.reserve(strlen(metadataTemplateNapi) + 32);
  snprintf(&metadata[0], metadata.capacity(), metadataTemplateNapi, static_cast<unsigned>(uid));

  size_t totalSize = sizeof(node_gdal::pixel_func) + strlen(metadata.c_str()) + 1;
  Napi::Buffer<uint8_t> buf = Napi::Buffer<uint8_t>::New(env, totalSize);
  node_gdal::pixel_func *desc = reinterpret_cast<node_gdal::pixel_func *>(buf.Data());

  desc->magic = NODE_GDAL_CAPI_MAGIC;
  desc->fn = pixelFuncNapi;
  char *md = reinterpret_cast<char *>(desc) + sizeof(node_gdal::pixel_func);
  memcpy(md, metadata.data(), strlen(metadata.c_str()));
  desc->metadata = md;

  return buf;
#else
  Napi::Error::New(env, "Custom pixel functions require GDAL >= 3.5").ThrowAsJavaScriptException();
  return env.Undefined();
#endif
}

// =========================================================================
// Init
// =========================================================================
Napi::Object AlgorithmsNapi::Init(Napi::Env env, Napi::Object exports) {
  auto obj = Napi::Object::New(env);

  obj.Set("fillNodata", Napi::Function::New(env, AlgorithmsNapi::fillNodata));
  obj.Set("fillNodataAsync", Napi::Function::New(env, AlgorithmsNapi::fillNodataAsync));
  obj.Set("contourGenerate", Napi::Function::New(env, AlgorithmsNapi::contourGenerate));
  obj.Set("contourGenerateAsync", Napi::Function::New(env, AlgorithmsNapi::contourGenerateAsync));
  obj.Set("sieveFilter", Napi::Function::New(env, AlgorithmsNapi::sieveFilter));
  obj.Set("sieveFilterAsync", Napi::Function::New(env, AlgorithmsNapi::sieveFilterAsync));
  obj.Set("checksumImage", Napi::Function::New(env, AlgorithmsNapi::checksumImage));
  obj.Set("checksumImageAsync", Napi::Function::New(env, AlgorithmsNapi::checksumImageAsync));
  obj.Set("polygonize", Napi::Function::New(env, AlgorithmsNapi::polygonize));
  obj.Set("polygonizeAsync", Napi::Function::New(env, AlgorithmsNapi::polygonizeAsync));

  exports.Set("Algorithms", obj);
  // Also register at top level for backward compatibility
  exports.Set("fillNodata", obj.Get("fillNodata"));
  exports.Set("fillNodataAsync", obj.Get("fillNodataAsync"));
  exports.Set("contourGenerate", obj.Get("contourGenerate"));
  exports.Set("contourGenerateAsync", obj.Get("contourGenerateAsync"));
  exports.Set("sieveFilter", obj.Get("sieveFilter"));
  exports.Set("sieveFilterAsync", obj.Get("sieveFilterAsync"));
  exports.Set("checksumImage", obj.Get("checksumImage"));
  exports.Set("checksumImageAsync", obj.Get("checksumImageAsync"));
  exports.Set("polygonize", obj.Get("polygonize"));
  exports.Set("polygonizeAsync", obj.Get("polygonizeAsync"));
  obj.Set("addPixelFunc", Napi::Function::New(env, AlgorithmsNapi::addPixelFunc));
  obj.Set("toPixelFunc", Napi::Function::New(env, AlgorithmsNapi::toPixelFunc));
  exports.Set("addPixelFunc", obj.Get("addPixelFunc"));
  exports.Set("toPixelFunc", obj.Get("toPixelFunc"));
  return exports;
}

// =========================================================================
// Helper: extract wrapped object from options object
// =========================================================================
template<typename T>
static T *unwrapFromObj(Napi::Object obj, const char *key, Napi::Env env) {
  Napi::Value val = obj.Get(key);
  if (val.IsNull() || val.IsUndefined() || !val.IsObject() ||
      !val.As<Napi::Object>().InstanceOf(T::constructor.Value())) {
    return nullptr;
  }
  T *wrapped = T::Unwrap(val.As<Napi::Object>());
  return (wrapped && wrapped->isAlive()) ? wrapped : nullptr;
}

#define NAPI_ARG_WRAPPED_FROM_OBJ(obj, key_js, type, var) \
  var = unwrapFromObj<type>(obj, key_js, env); \
  if (!var) { \
    Napi::TypeError::New(env, "options." key_js " must be an instance of " #type).ThrowAsJavaScriptException(); \
    return env.Undefined(); \
  }

// =========================================================================
// fillNodata
// =========================================================================

Napi::Value AlgorithmsNapi::fillNodata(const Napi::CallbackInfo &info) { return fillNodata_do(info, false); }
Napi::Value AlgorithmsNapi::fillNodataAsync(const Napi::CallbackInfo &info) { return fillNodata_do(info, true); }
Napi::Value AlgorithmsNapi::fillNodata_do(const Napi::CallbackInfo &info, bool async) {
  Napi::Env env = info.Env();

  Napi::Object options;
  NAPI_ARG_OBJECT(0, "options", options);

  auto getObjProp = [&](const char *key, Napi::Value &val) -> bool {
    if (!options.Has(key)) {
      Napi::Error::New(env, "options must contain property \"" + std::string(key) + "\"")
        .ThrowAsJavaScriptException();
      return false;
    }
    val = options.Get(key);
    return true;
  };

  Napi::Value srcVal;
  if (!getObjProp("src", srcVal)) return env.Undefined();
  RasterBandNapi *src = nullptr;
  NAPI_ARG_WRAPPED_FROM_OBJ(options, "src", RasterBandNapi, src);

  RasterBandNapi *mask = nullptr;
  if (options.Has("mask") && !options.Get("mask").IsNull() && !options.Get("mask").IsUndefined()) {
    NAPI_ARG_WRAPPED_FROM_OBJ(options, "mask", RasterBandNapi, mask);
  }

  Napi::Value searchDistVal;
  if (!getObjProp("searchDist", searchDistVal)) return env.Undefined();
  double search_dist = searchDistVal.As<Napi::Number>().DoubleValue();

  int smooth_iterations = 0;
  if (options.Has("smoothIterations")) {
    smooth_iterations = options.Get("smoothIterations").As<Napi::Number>().Int32Value();
  }

  GDALRasterBand *gdal_src = src->get();
  GDALRasterBand *gdal_mask = mask ? mask->get() : nullptr;

  GDALAsyncableJobNapi<CPLErr> job;
  NAPI_CB_FROM_OBJ_OPT(options, "progress_cb", job.progress_cb_);
  
  job.main = [gdal_src, gdal_mask, search_dist, smooth_iterations, &job]() -> CPLErr {
    CPLErrorReset();
    CPLErr err = GDALFillNodata(gdal_src, gdal_mask, search_dist, 0, smooth_iterations,
                                NULL, job.progressFunc(), job.progressArg());
    if (err) { throw CPLGetLastErrorMsg(); }
    return err;
  };
  job.rval = [](Napi::Env env, CPLErr r) {
    return env.Undefined();
  };

  return job.run(info, async, 1);
}

// =========================================================================
// contourGenerate
// =========================================================================

Napi::Value AlgorithmsNapi::contourGenerate(const Napi::CallbackInfo &info) { return contourGenerate_do(info, false); }
Napi::Value AlgorithmsNapi::contourGenerateAsync(const Napi::CallbackInfo &info) { return contourGenerate_do(info, true); }
Napi::Value AlgorithmsNapi::contourGenerate_do(const Napi::CallbackInfo &info, bool async) {
  Napi::Env env = info.Env();

  Napi::Object options;
  NAPI_ARG_OBJECT(0, "options", options);

  RasterBandNapi *src = nullptr;
  LayerNapi *dst = nullptr;

  NAPI_ARG_WRAPPED_FROM_OBJ(options, "src", RasterBandNapi, src);
  NAPI_ARG_WRAPPED_FROM_OBJ(options, "dst", LayerNapi, dst);

  int id_field = -1, elev_field = -1;
  NAPI_INT_FROM_OBJ(options, "idField", id_field);
  NAPI_INT_FROM_OBJ(options, "elevField", elev_field);

  double interval = 100;
  if (options.Has("interval")) {
    interval = options.Get("interval").As<Napi::Number>().DoubleValue();
  }
  double base = 0;
  if (options.Has("offset")) {
    base = options.Get("offset").As<Napi::Number>().DoubleValue();
  }

  double *fixed_levels = nullptr;
  unsigned int n_fixed_levels = 0;
  std::vector<double> fixed_levels_vec;
  if (options.Has("fixedLevels")) {
    Napi::Value flVal = options.Get("fixedLevels");
    if (flVal.IsArray()) {
      Napi::Array flArr = flVal.As<Napi::Array>();
      n_fixed_levels = flArr.Length();
      fixed_levels_vec.reserve(n_fixed_levels);
      for (uint32_t i = 0; i < n_fixed_levels; i++) {
        fixed_levels_vec.push_back(flArr.Get(i).As<Napi::Number>().DoubleValue());
      }
      fixed_levels = fixed_levels_vec.data();
    }
  }

  bool use_nodata = false;
  double nodata = 0;
  if (options.Has("nodata") && !options.Get("nodata").IsNull() && !options.Get("nodata").IsUndefined()) {
    use_nodata = true;
    nodata = options.Get("nodata").As<Napi::Number>().DoubleValue();
  }

  GDALRasterBand *gdal_src = src->get();
  OGRLayer *gdal_dst = dst->get();

  GDALAsyncableJobNapi<CPLErr> job;
  NAPI_CB_FROM_OBJ_OPT(options, "progress_cb", job.progress_cb_);
  
  job.main = [gdal_src, interval, base, n_fixed_levels, fixed_levels, use_nodata, nodata,
              gdal_dst, id_field, elev_field, &job]() -> CPLErr {
    CPLErrorReset();
    CPLErr err = GDALContourGenerate(gdal_src, interval, base, n_fixed_levels, fixed_levels,
                                     use_nodata, nodata, gdal_dst, id_field, elev_field,
                                     job.progressFunc(), job.progressArg());
    if (err) { throw CPLGetLastErrorMsg(); }
    return err;
  };
  job.rval = [](Napi::Env env, CPLErr r) {
    return env.Undefined();
  };

  return job.run(info, async, 1);
}

// =========================================================================
// sieveFilter
// =========================================================================

Napi::Value AlgorithmsNapi::sieveFilter(const Napi::CallbackInfo &info) { return sieveFilter_do(info, false); }
Napi::Value AlgorithmsNapi::sieveFilterAsync(const Napi::CallbackInfo &info) { return sieveFilter_do(info, true); }
Napi::Value AlgorithmsNapi::sieveFilter_do(const Napi::CallbackInfo &info, bool async) {
  Napi::Env env = info.Env();

  Napi::Object options;
  NAPI_ARG_OBJECT(0, "options", options);

  RasterBandNapi *src = nullptr;
  RasterBandNapi *dst = nullptr;
  RasterBandNapi *mask = nullptr;

  NAPI_ARG_WRAPPED_FROM_OBJ(options, "src", RasterBandNapi, src);
  NAPI_ARG_WRAPPED_FROM_OBJ(options, "dst", RasterBandNapi, dst);

  if (options.Has("mask") && !options.Get("mask").IsNull() && !options.Get("mask").IsUndefined()) {
    NAPI_ARG_WRAPPED_FROM_OBJ(options, "mask", RasterBandNapi, mask);
  }

  int threshold;
  NAPI_INT_FROM_OBJ(options, "threshold", threshold);

  int connectedness = 4;
  NAPI_INT_FROM_OBJ(options, "connectedness", connectedness);

  if (connectedness != 4 && connectedness != 8) {
    Napi::Error::New(env, "connectedness option must be 4 or 8").ThrowAsJavaScriptException();
    return env.Undefined();
  }

  GDALRasterBand *gdal_src = src->get();
  GDALRasterBand *gdal_dst = dst->get();
  GDALRasterBand *gdal_mask = mask ? mask->get() : nullptr;

  GDALAsyncableJobNapi<CPLErr> job;
  NAPI_CB_FROM_OBJ_OPT(options, "progress_cb", job.progress_cb_);
  
  job.main = [gdal_src, gdal_dst, gdal_mask, threshold, connectedness, &job]() -> CPLErr {
    CPLErrorReset();
    CPLErr err = GDALSieveFilter(gdal_src, gdal_mask, gdal_dst, threshold, connectedness,
                                 NULL, job.progressFunc(), job.progressArg());
    if (err) { throw CPLGetLastErrorMsg(); }
    return err;
  };
  job.rval = [](Napi::Env env, CPLErr r) {
    return env.Undefined();
  };

  return job.run(info, async, 1);
}

// =========================================================================
// checksumImage
// =========================================================================

Napi::Value AlgorithmsNapi::checksumImage(const Napi::CallbackInfo &info) { return checksumImage_do(info, false); }
Napi::Value AlgorithmsNapi::checksumImageAsync(const Napi::CallbackInfo &info) { return checksumImage_do(info, true); }
Napi::Value AlgorithmsNapi::checksumImage_do(const Napi::CallbackInfo &info, bool async) {
  Napi::Env env = info.Env();

  RasterBandNapi *src = nullptr;
  NAPI_ARG_WRAPPED(0, "src", RasterBandNapi, src);

  GDALRasterBand *gdal_src = src->get();
  int bandw = gdal_src->GetXSize();
  int bandh = gdal_src->GetYSize();

  int x = 0, y = 0, w = bandw, h = bandh;
  // Don't use NAPI_ARG_INT_OPT — it resets to 0 and loses defaults
  if (info.Length() > 1 && info[1].IsNumber()) x = info[1].As<Napi::Number>().Int32Value();
  if (info.Length() > 2 && info[2].IsNumber()) y = info[2].As<Napi::Number>().Int32Value();
  if (info.Length() > 3 && info[3].IsNumber()) w = info[3].As<Napi::Number>().Int32Value();
  if (info.Length() > 4 && info[4].IsNumber()) h = info[4].As<Napi::Number>().Int32Value();

  if (x < 0 || y < 0 || x >= bandw || y >= bandh) {
    Napi::RangeError::New(env, "offset invalid for given band").ThrowAsJavaScriptException();
    return env.Undefined();
  }
  if (w < 0 || h < 0 || w > bandw || h > bandh) {
    Napi::RangeError::New(env, "x and y size must be smaller than band dimensions and greater than 0")
      .ThrowAsJavaScriptException();
    return env.Undefined();
  }
  if (x + w - 1 >= bandw || y + h - 1 >= bandh) {
    Napi::RangeError::New(env, "given range is outside bounds of given band")
      .ThrowAsJavaScriptException();
    return env.Undefined();
  }

  GDALAsyncableJobNapi<int> job;
  job.main = [gdal_src, x, y, w, h]() -> int {
    CPLErrorReset();
    int r = GDALChecksumImage(gdal_src, x, y, w, h);
    if (r == -1) throw CPLGetLastErrorMsg();
    return r;
  };
  job.rval = [](Napi::Env env, int r) {
    return Napi::Number::New(env, r);
  };

  return job.run(info, async, 5);
}

// =========================================================================
// polygonize
// =========================================================================

Napi::Value AlgorithmsNapi::polygonize(const Napi::CallbackInfo &info) { return polygonize_do(info, false); }
Napi::Value AlgorithmsNapi::polygonizeAsync(const Napi::CallbackInfo &info) { return polygonize_do(info, true); }
Napi::Value AlgorithmsNapi::polygonize_do(const Napi::CallbackInfo &info, bool async) {
  Napi::Env env = info.Env();

  Napi::Object options;
  NAPI_ARG_OBJECT(0, "options", options);

  RasterBandNapi *src = nullptr;
  LayerNapi *dst = nullptr;
  RasterBandNapi *mask = nullptr;

  NAPI_ARG_WRAPPED_FROM_OBJ(options, "src", RasterBandNapi, src);
  NAPI_ARG_WRAPPED_FROM_OBJ(options, "dst", LayerNapi, dst);

  if (options.Has("mask") && !options.Get("mask").IsNull() && !options.Get("mask").IsUndefined()) {
    NAPI_ARG_WRAPPED_FROM_OBJ(options, "mask", RasterBandNapi, mask);
  }

  int connectedness = 4;
  if (options.Has("connectedness")) {
    connectedness = options.Get("connectedness").As<Napi::Number>().Int32Value();
  }

  int pix_val_field;
  NAPI_INT_FROM_OBJ(options, "pixValField", pix_val_field);

  if (connectedness != 4 && connectedness != 8) {
    Napi::Error::New(env, "connectedness must be 4 or 8").ThrowAsJavaScriptException();
    return env.Undefined();
  }

  char **papszOptions = nullptr;
  if (connectedness == 8) {
    papszOptions = CSLSetNameValue(papszOptions, "8CONNECTED", "8");
  }

  bool useFloats = false;
  if (options.Has("useFloats") && !options.Get("useFloats").IsNull() && !options.Get("useFloats").IsUndefined()) {
    useFloats = options.Get("useFloats").As<Napi::Boolean>().Value();
  }

  GDALRasterBand *gdal_src = src->get();
  OGRLayer *gdal_dst = dst->get();
  GDALRasterBand *gdal_mask = mask ? mask->get() : nullptr;

  GDALAsyncableJobNapi<CPLErr> job;
  NAPI_CB_FROM_OBJ_OPT(options, "progress_cb", job.progress_cb_);
  
  if (useFloats) {
    job.main = [gdal_src, gdal_mask, gdal_dst, pix_val_field, papszOptions, &job]() -> CPLErr {
      CPLErrorReset();
      CPLErr err = GDALFPolygonize(gdal_src, gdal_mask, reinterpret_cast<OGRLayerH>(gdal_dst),
                                    pix_val_field, papszOptions, job.progressFunc(), job.progressArg());
      CSLDestroy(papszOptions);
      if (err) throw CPLGetLastErrorMsg();
      return err;
    };
  } else {
    job.main = [gdal_src, gdal_mask, gdal_dst, pix_val_field, papszOptions, &job]() -> CPLErr {
      CPLErrorReset();
      CPLErr err = GDALPolygonize(gdal_src, gdal_mask, reinterpret_cast<OGRLayerH>(gdal_dst),
                                   pix_val_field, papszOptions, job.progressFunc(), job.progressArg());
      CSLDestroy(papszOptions);
      if (err) throw CPLGetLastErrorMsg();
      return err;
    };
  }
  job.rval = [](Napi::Env env, CPLErr r) {
    return env.Undefined();
  };

  return job.run(info, async, 1);
}

} // namespace node_gdal
