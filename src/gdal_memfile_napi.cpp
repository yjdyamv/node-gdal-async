#include "gdal_memfile_napi.hpp"

namespace node_gdal {

Napi::Object MemfileNapi::Init(Napi::Env env, Napi::Object exports) {
  // Memfile uses Nan::WeakCallback for GC lifecycle management.
  // Full N-API port requires Napi::ObjectReference with weak refs.
  // Keep nan version as primary for now.
  if (!exports.Has("vsimem")) {
    Napi::Object vsimem = Napi::Object::New(env);
    // TODO: implement vsimem.set, release, copy with Napi::Buffer
    exports.Set("vsimem", vsimem);
  }
  return exports;
}

} // namespace node_gdal
