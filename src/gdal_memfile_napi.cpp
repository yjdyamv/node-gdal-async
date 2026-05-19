#include "gdal_memfile_napi.hpp"
#include "gdal_common_napi.hpp"
#include <map>
#include <string>

namespace node_gdal {
namespace MemfileNapi {

struct MemFileEntry {
  Napi::ObjectReference ref;
  std::string filename;
  bool is_anonymous;
};

static std::map<void *, MemFileEntry *> memfile_collection;
static std::map<void *, size_t> tracked_buffers;

// ---------------------------------------------------------------------------
// vsimem.set – create in-memory file from Buffer (zero-copy)
// ---------------------------------------------------------------------------
static Napi::Value vsimemSet(const Napi::CallbackInfo &info) {
  Napi::Env env = info.Env();
  if (info.Length() < 2 || !info[0].IsBuffer() || !info[1].IsString()) {
    Napi::Error::New(env, "Expected (buffer, filename)").ThrowAsJavaScriptException();
    return env.Undefined();
  }
  Napi::Buffer<uint8_t> buf = info[0].As<Napi::Buffer<uint8_t>>();
  std::string filename = info[1].As<Napi::String>().Utf8Value();

  void *data = buf.Data();
  size_t len = buf.Length();

  // Check if already registered
  if (memfile_collection.count(data)) {
    return env.Undefined(); // already exists
  }

  VSILFILE *vsi = VSIFileFromMemBuffer(filename.c_str(), (GByte *)data, len, 0);
  if (!vsi) {
    Napi::Error::New(env, "Failed creating in-memory file").ThrowAsJavaScriptException();
    return env.Undefined();
  }
  VSIFCloseL(vsi);

  auto *entry = new MemFileEntry();
  entry->filename = filename;
  entry->is_anonymous = false;
  entry->ref = Napi::Persistent(info[0].As<Napi::Object>()); // strong ref to prevent GC
  memfile_collection[data] = entry;

  return env.Undefined();
}

// ---------------------------------------------------------------------------
// vsimem.release – delete and optionally return Buffer content
// ---------------------------------------------------------------------------
static Napi::Value vsimemRelease(const Napi::CallbackInfo &info) {
  Napi::Env env = info.Env();
  if (info.Length() < 1 || !info[0].IsString()) {
    Napi::Error::New(env, "Expected filename").ThrowAsJavaScriptException();
    return env.Undefined();
  }
  std::string filename = info[0].As<Napi::String>().Utf8Value();

  vsi_l_offset len = 0;
  CPLErrorReset();
  void *data = VSIGetMemFileBuffer(filename.c_str(), &len, false);
  if (!data) {
    NAPI_THROW_LAST_CPLERR;
  }

  if (memfile_collection.count(data)) {
    // Named buffer: return reference to original Buffer
    MemFileEntry *entry = memfile_collection[data];
    memfile_collection.erase(data);
    VSIUnlink(entry->filename.c_str());
    Napi::Value result = entry->ref.Value();
    delete entry;
    return result;
  } else {
    // GDAL-owned buffer: create new Buffer with finalizer
    VSIGetMemFileBuffer(filename.c_str(), &len, true);
    return Napi::Buffer<uint8_t>::New(env, (uint8_t *)data, (size_t)len,
      [](Napi::Env, uint8_t *data) {
        // Free GDAL buffer when Buffer is GC'd
        auto it = tracked_buffers.find(data);
        if (it != tracked_buffers.end()) {
          tracked_buffers.erase(it);
        }
        CPLFree(data);
      });
  }
}

// ---------------------------------------------------------------------------
// vsimem.copy – create extendable copy in GDAL heap
// ---------------------------------------------------------------------------
static Napi::Value vsimemCopy(const Napi::CallbackInfo &info) {
  Napi::Env env = info.Env();
  if (info.Length() < 2 || !info[0].IsBuffer() || !info[1].IsString()) {
    Napi::Error::New(env, "Expected (buffer, filename)").ThrowAsJavaScriptException();
    return env.Undefined();
  }
  Napi::Buffer<uint8_t> buf = info[0].As<Napi::Buffer<uint8_t>>();
  std::string filename = info[1].As<Napi::String>().Utf8Value();

  size_t len = buf.Length();
  void *dataCopy = CPLMalloc(len);
  if (!dataCopy) {
    Napi::Error::New(env, "Failed allocating memory").ThrowAsJavaScriptException();
    return env.Undefined();
  }
  memcpy(dataCopy, buf.Data(), len);

  VSILFILE *vsi = VSIFileFromMemBuffer(filename.c_str(), (GByte *)dataCopy, len, 1);
  if (!vsi) {
    CPLFree(dataCopy);
    Napi::Error::New(env, CPLGetLastErrorMsg()).ThrowAsJavaScriptException();
    return env.Undefined();
  }
  VSIFCloseL(vsi);

  tracked_buffers[dataCopy] = len;
  return env.Undefined();
}

// ===================================================================
// _anonymous – create anonymous vsimem; auto-deleted on GC
// ===================================================================
static Napi::Value vsimemAnonymous(const Napi::CallbackInfo &info) {
  Napi::Env env = info.Env();
  if (info.Length() < 1 || !info[0].IsBuffer()) {
    Napi::Error::New(env, "Expected a Buffer").ThrowAsJavaScriptException();
    return env.Undefined();
  }
  Napi::Buffer<uint8_t> buf = info[0].As<Napi::Buffer<uint8_t>>();
  void *data = buf.Data();
  size_t len = buf.Length();

  // Already exists?
  if (memfile_collection.count(data)) {
    return Napi::String::New(env, memfile_collection[data]->filename);
  }

  char _filename[32];
  snprintf(_filename, sizeof(_filename), "/vsimem/%p", data);

  VSILFILE *vsi = VSIFileFromMemBuffer(_filename, (GByte *)data, len, 0);
  if (!vsi) {
    Napi::Error::New(env, "Failed creating in-memory file").ThrowAsJavaScriptException();
    return env.Undefined();
  }
  VSIFCloseL(vsi);

  auto *entry = new MemFileEntry();
  entry->filename = _filename;
  entry->is_anonymous = true;
  entry->ref = Napi::Persistent(info[0].As<Napi::Object>()); // keep buffer alive
  memfile_collection[data] = entry;

  return Napi::String::New(env, _filename);
}

// ===================================================================
// Init
// ===================================================================
Napi::Object Init(Napi::Env env, Napi::Object exports) {
  Napi::Object vsimem = Napi::Object::New(env);
  vsimem.Set("set", Napi::Function::New(env, vsimemSet, "set"));
  vsimem.Set("release", Napi::Function::New(env, vsimemRelease, "release"));
  vsimem.Set("copy", Napi::Function::New(env, vsimemCopy, "copy"));
  vsimem.Set("_anonymous", Napi::Function::New(env, vsimemAnonymous, "_anonymous"));
  exports.Set("vsimem", vsimem);
  return exports;
}

} // namespace MemfileNapi
} // namespace node_gdal
