#ifndef __NAPI_OBJ_STORE_H__
#define __NAPI_OBJ_STORE_H__

#include <napi.h>
#include <map>
#include <memory>
#include <mutex>

namespace node_gdal {

// Lightweight object identity tracker for N-API ObjectWrap classes.
// Mirrors ObjectStore semantics but uses Napi::ObjectReference instead
// of Nan::Persistent, ensuring the same JS wrapper object is returned
// for the same GDAL pointer (e.g., ds.bands.get(1) === ds.bands.get(1)).

template <typename GDALPTR>
class NapiObjectStore {
    public:
  using Ref = Napi::Reference<Napi::Object>;

  // Register a JS wrapper for a GDAL pointer. Returns true if first registration.
  bool add(GDALPTR ptr, const Napi::Object &obj) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (ptrMap_.count(ptr)) return false;
    ptrMap_[ptr] = std::make_shared<Ref>(Napi::Persistent(obj));
    auto item = ptrMap_[ptr];
    return true;
  }

  // Check if a GDAL pointer has a registered wrapper
  bool has(GDALPTR ptr) {
    std::lock_guard<std::mutex> lock(mutex_);
    return ptrMap_.count(ptr) > 0;
  }

  // Retrieve the JS wrapper for a GDAL pointer (main thread only)
  Napi::Object get(Napi::Env env, GDALPTR ptr) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = ptrMap_.find(ptr);
    if (it != ptrMap_.end()) return it->second->Value();
    return Napi::Object::New(env);
  }

  // Remove a GDAL pointer from the store (called from Finalize)
  void remove(GDALPTR ptr) {
    std::lock_guard<std::mutex> lock(mutex_);
    ptrMap_.erase(ptr);
  }

  static NapiObjectStore<GDALPTR> &instance() {
    static NapiObjectStore<GDALPTR> store;
    return store;
  }

    private:
  std::mutex mutex_;
  std::map<GDALPTR, std::shared_ptr<Ref>> ptrMap_;
};

// Convenience free functions
template <typename GDALPTR>
inline bool napi_obj_store_add(GDALPTR ptr, const Napi::Object &obj) {
  return NapiObjectStore<GDALPTR>::instance().add(ptr, obj);
}

template <typename GDALPTR>
inline bool napi_obj_store_has(GDALPTR ptr) {
  return NapiObjectStore<GDALPTR>::instance().has(ptr);
}

template <typename GDALPTR>
inline void napi_obj_store_remove(GDALPTR ptr) {
  NapiObjectStore<GDALPTR>::instance().remove(ptr);
}

template <typename GDALPTR>
inline Napi::Object napi_obj_store_get(Napi::Env env, GDALPTR ptr) {
  return NapiObjectStore<GDALPTR>::instance().get(env, ptr);
}

} // namespace node_gdal

#endif
