#ifndef __NODE_GDAL_ASYNC_WORKER_NAPI_H__
#define __NODE_GDAL_ASYNC_WORKER_NAPI_H__

#include <functional>
#include <memory>
#include <string>
#include "gdal_common_napi.hpp"

namespace node_gdal {

// ---------------------------------------------------------------------------
// N-API async worker – callback-based
// ---------------------------------------------------------------------------
template <class GDALType>
class GDALAsyncWorkerNapi : public Napi::AsyncWorker {
    public:
  using MainFunc = std::function<GDALType()>;
  using RValFunc = std::function<Napi::Value(Napi::Env, const GDALType &)>;

  GDALAsyncWorkerNapi(Napi::Function callback, MainFunc main, RValFunc rval)
    : Napi::AsyncWorker(callback),
      main_(std::move(main)),
      rval_(std::move(rval)),
      result_() {
  }

  void Execute() override {
    try {
      result_ = main_();
    } catch (const std::exception &e) {
      SetError(e.what());
    } catch (const char *e) {
      SetError(e);
    }
  }

  void OnOK() override {
    Napi::HandleScope scope(Env());
    Napi::Value jsResult = rval_(Env(), result_);
    Callback().Call({Env().Null(), jsResult});
  }

    private:
  MainFunc main_;
  RValFunc rval_;
  GDALType result_;
};

// Promise-based async worker (no callback)
template <class GDALType>
class GDALAsyncWorkerNapiPromise : public Napi::AsyncWorker {
    public:
  using MainFunc = std::function<GDALType()>;
  using RValFunc = std::function<Napi::Value(Napi::Env, const GDALType &)>;

  GDALAsyncWorkerNapiPromise(Napi::Env env, Napi::Promise::Deferred deferred, MainFunc main, RValFunc rval)
    : Napi::AsyncWorker(env),
      deferred_(std::move(deferred)),
      main_(std::move(main)),
      rval_(std::move(rval)) {}

  void Execute() override {
    try { result_ = main_(); }
    catch (const std::exception &e) { SetError(e.what()); }
    catch (const char *e) { SetError(e); }
  }

  void OnOK() override {
    Napi::HandleScope scope(Env());
    deferred_.Resolve(rval_(Env(), result_));
  }

  void OnError(const Napi::Error &e) override {
    deferred_.Reject(e.Value());
  }

    private:
  Napi::Promise::Deferred deferred_;
  MainFunc main_;
  RValFunc rval_;
  GDALType result_;
};

// ---------------------------------------------------------------------------
// Progress callback support
// ---------------------------------------------------------------------------

// Non-template base for GDALAsyncableJobNapi so the progress trampoline
// can access progress_error_ without knowing the template parameter.
class GDALAsyncableJobNapiBase {
    public:
  std::string progress_error_;
};

// Context passed to main() so algorithms can wire up GDAL progress callbacks.
class NapiProgressContext {
    public:
  NapiProgressContext() : cb_(nullptr), env_(nullptr), job_(nullptr) {}
  void setCallback(Napi::FunctionReference *cb, napi_env env) { cb_ = cb; env_ = env; }
  void setJob(GDALAsyncableJobNapiBase *job) { job_ = job; }

  GDALProgressFunc getFunc() const;
  void *getArg() const { return (void *)this; }  // pass context itself as user data

  // Accessed by trampoline
  Napi::FunctionReference *cb_;
  GDALAsyncableJobNapiBase *job_;
    private:
  napi_env env_;
};

// C-linkage trampoline: called by GDAL during long-running operations.
// Only supports sync mode (runs on main V8 thread).
// The pProgressArg points to a NapiProgressContext (which holds cb and job ptr).
static int ProgressTrampolineNapi(double dfComplete, const char *pszMessage, void *pProgressArg) {
  NapiProgressContext *ctx = static_cast<NapiProgressContext *>(pProgressArg);
  if (!ctx) return true;
  Napi::FunctionReference *cb = ctx->cb_;
  if (!cb || cb->IsEmpty()) return true;
  Napi::Env env = cb->Env();
  try {
    Napi::HandleScope scope(env);
    Napi::Value result = cb->Call({
      Napi::Number::New(env, dfComplete),
      SafeStringNapi(env, pszMessage)
    });
    if (result.IsBoolean()) return result.As<Napi::Boolean>().Value();
  } catch (const Napi::Error &e) {
    if (ctx->job_) ctx->job_->progress_error_ = std::string("sync progress callback exception: ") + e.Message();
    return false;  // stop GDAL operation
  } catch (const std::exception &e) {
    if (ctx->job_) ctx->job_->progress_error_ = std::string("sync progress callback exception: ") + e.what();
    return false;
  } catch (...) {
    if (ctx->job_) ctx->job_->progress_error_ = "sync progress callback exception: Unknown error";
    return false;
  }
  return true; // continue
}

inline GDALProgressFunc NapiProgressContext::getFunc() const {
  return (cb_ && !cb_->IsEmpty()) ? ProgressTrampolineNapi : nullptr;
}

// ---------------------------------------------------------------------------
// Simplified async dispatch (callback or Promise)
// ---------------------------------------------------------------------------
template <class GDALType>
class GDALAsyncableJobNapi : public GDALAsyncableJobNapiBase {
    public:
  using MainFunc = typename GDALAsyncWorkerNapi<GDALType>::MainFunc;
  using RValFunc = typename GDALAsyncWorkerNapi<GDALType>::RValFunc;

  MainFunc main;
  RValFunc rval;
  Napi::FunctionReference progress_cb_;

  GDALAsyncableJobNapi() : main(), rval() {
  }

  // Set up progress context before calling main() in sync mode
  void setupProgress(Napi::Env env) {
    if (!progress_cb_.IsEmpty()) {
      ctx_.setCallback(&progress_cb_, env);
      ctx_.setJob(static_cast<GDALAsyncableJobNapiBase *>(this));
    }
  }

  GDALProgressFunc progressFunc() { return ctx_.getFunc(); }
  void *progressArg() { return ctx_.getArg(); }

  // Dispatch: sync if !async, else queue an async worker.
  // If async and last arg is a Function, use callback mode.
  // Otherwise return a Promise.
  Napi::Value run(const Napi::CallbackInfo &info, bool async, int cb_index) {
    if (async) {
      if (info.Length() > cb_index && info[cb_index].IsFunction()) {
        Napi::Function callback = info[cb_index].As<Napi::Function>();
        auto *worker = new GDALAsyncWorkerNapi<GDALType>(callback, main, rval);
        worker->Queue();
        return info.Env().Undefined();
      }
      // Promise mode
      Napi::Env env = info.Env();
      Napi::Promise::Deferred deferred = Napi::Promise::Deferred::New(env);
      auto *worker = new GDALAsyncWorkerNapiPromise<GDALType>(env, deferred, main, rval);
      worker->Queue();
      return deferred.Promise();
    }

    // Sync mode: set up progress context, then call main
    progress_error_.clear();
    setupProgress(info.Env());
    try {
      GDALType result = main();
      if (!progress_error_.empty()) {
        Napi::Error::New(info.Env(), progress_error_).ThrowAsJavaScriptException();
        return info.Env().Undefined();
      }
      return rval(info.Env(), result);
    } catch (const std::exception &e) {
      Napi::Error::New(info.Env(), e.what()).ThrowAsJavaScriptException();
    } catch (const char *e) {
      Napi::Error::New(info.Env(), e).ThrowAsJavaScriptException();
    }
    return info.Env().Undefined();
  }

    private:
  NapiProgressContext ctx_;
};

} // namespace node_gdal

// ---------------------------------------------------------------------------
// Macros for declaring and defining sync/async method pairs
// ---------------------------------------------------------------------------

// In the class header:
#define GDAL_ASYNCABLE_DECLARE_NAPI(method)                                                    \
  Napi::Value method(const Napi::CallbackInfo &info);                                           \
  Napi::Value method##Async(const Napi::CallbackInfo &info);                                    \
  Napi::Value method##_do(const Napi::CallbackInfo &info, bool async)

// In the class implementation:
#define GDAL_ASYNCABLE_DEFINE_NAPI(cls, method)                                                 \
  Napi::Value cls::method(const Napi::CallbackInfo &info) {                                     \
    return method##_do(info, false);                                                            \
  }                                                                                              \
  Napi::Value cls::method##Async(const Napi::CallbackInfo &info) {                              \
    return method##_do(info, true);                                                             \
  }                                                                                              \
  Napi::Value cls::method##_do(const Napi::CallbackInfo &info, bool async)

// In the class header (getters use same signature as methods in N-API):
#define GDAL_ASYNCABLE_GETTER_DECLARE_NAPI(getter)                                             \
  Napi::Value getter(const Napi::CallbackInfo &info);                                           \
  Napi::Value getter##Async(const Napi::CallbackInfo &info);                                    \
  Napi::Value getter##_do(const Napi::CallbackInfo &info, bool async)

// In the class implementation:
#define GDAL_ASYNCABLE_GETTER_DEFINE_NAPI(cls, getter)                                         \
  Napi::Value cls::getter(const Napi::CallbackInfo &info) {                                     \
    return getter##_do(info, false);                                                            \
  }                                                                                              \
  Napi::Value cls::getter##Async(const Napi::CallbackInfo &info) {                              \
    return getter##_do(info, true);                                                             \
  }                                                                                              \
  Napi::Value cls::getter##_do(const Napi::CallbackInfo &info, bool async)

// ---------------------------------------------------------------------------
// NapiStringList – lightweight string list parser for GDAL options
// ---------------------------------------------------------------------------
namespace node_gdal {

class NapiStringList {
    public:
  NapiStringList() : list_(nullptr), strs_(nullptr), count_(0) {
  }
  ~NapiStringList() {
    free();
  }

  bool parse(Napi::Value value) {
    if (value.IsNull() || value.IsUndefined()) return true;

    if (value.IsArray()) {
      Napi::Array arr = value.As<Napi::Array>();
      count_ = arr.Length();
      if (count_ == 0) return true;

      strs_ = new std::string[count_];
      list_ = new char *[count_ + 1];
      for (uint32_t i = 0; i < count_; i++) {
        strs_[i] = arr.Get(i).As<Napi::String>().Utf8Value();
        list_[i] = (char *)strs_[i].c_str();
      }
      list_[count_] = nullptr;
      return true;
    }

    if (value.IsObject()) {
      Napi::Object obj = value.As<Napi::Object>();
      Napi::Array keys = obj.GetPropertyNames();
      count_ = keys.Length();
      if (count_ == 0) return true;

      strs_ = new std::string[count_];
      list_ = new char *[count_ + 1];
      for (uint32_t i = 0; i < count_; i++) {
        std::string key = keys.Get(i).As<Napi::String>().Utf8Value();
        std::string val = obj.Get(key).As<Napi::String>().Utf8Value();
        strs_[i] = key + "=" + val;
        list_[i] = (char *)strs_[i].c_str();
      }
      list_[count_] = nullptr;
      return true;
    }

    return false;
  }

  char **get() {
    return list_;
  }

    private:
  void free() {
    if (list_) delete[] list_;
    if (strs_) delete[] strs_;
    list_ = nullptr;
    strs_ = nullptr;
    count_ = 0;
  }

  char **list_;
  std::string *strs_;
  uint32_t count_;
};

} // namespace node_gdal

#endif
