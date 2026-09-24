#ifndef __NODE_GDAL_MEMFILE_H__
#define __NODE_GDAL_MEMFILE_H__

// gdal
#include <gdal_priv.h>

#include "gdal_common.hpp"

// A vsimem file
//
// Memfile is not a JS class: it is a plain C++ helper that ties a Node Buffer to
// a /vsimem/ file. Anonymous files are deleted by the GC, which in NAN was a
// weak callback on a Persistent<Object> and in N-API is napi_add_finalizer.

namespace node_gdal {

class Memfile {
  void *data;
  Napi::Reference<Napi::Object> *persistent;
  // napi_finalize callback: :: because node_gdal::napi_env shadows the type
  static void finalize(::napi_env env, void *data, void *hint);

    public:
  std::string filename;
  Memfile(void *);
  Memfile(void *, const std::string &filename);
  ~Memfile();
  static Memfile *get(Napi::Object);
  static Memfile *get(Napi::Object, const std::string &filename);
  static bool copy(Napi::Object, const std::string &filename);
  static std::map<void *, Memfile *> memfile_collection;
  static std::map<void *, size_t> tracked_buffers;

  static void Initialize(Napi::Object target);
  static NAN_METHOD(vsimemSet);
  static NAN_METHOD(vsimemAnonymous);
  static NAN_METHOD(vsimemRelease);
  static NAN_METHOD(vsimemCopy);
};
} // namespace node_gdal
#endif
