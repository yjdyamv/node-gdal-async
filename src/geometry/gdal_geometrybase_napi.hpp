#ifndef __NODE_OGR_GEOMETRYBASE_NAPI_H__
#define __NODE_OGR_GEOMETRYBASE_NAPI_H__

#include <napi.h>
#include <ogrsf_frmts.h>
#include <uv.h>
#include "../gdal_common_napi.hpp"

namespace node_gdal {

// CRTP data/utility base for geometry wrappers.
// Does NOT inherit from ObjectWrap – the derived class inherits ObjectWrap.
template <class T, class OGRT>
class GeometryBaseNapi {
    public:
  static Napi::Value New(Napi::Env env, OGRT *geom);
  static Napi::Value New(Napi::Env env, OGRT *geom, bool owned);

  OGRT *get() {
    return this_;
  }
  bool isAlive() {
    return this_ != nullptr;
  }

    protected:
  GeometryBaseNapi();
  ~GeometryBaseNapi();

  OGRT *this_;
  bool owned_;
  int size_;
  uv_sem_t *async_lock;
};

// ---------------------------------------------------------------------------
// Template implementations
// ---------------------------------------------------------------------------

template <class T, class OGRT>
Napi::Value GeometryBaseNapi<T, OGRT>::New(Napi::Env env, OGRT *geom) {
  return New(env, geom, true);
}

template <class T, class OGRT>
Napi::Value GeometryBaseNapi<T, OGRT>::New(Napi::Env env, OGRT *geom, bool owned) {
  Napi::EscapableHandleScope scope(env);

  if (!geom) { return scope.Escape(env.Null()); }
  if (!owned) { geom = static_cast<OGRT *>(geom->clone()); }

  Napi::Object obj = T::constructor.New({Napi::External<OGRT>::New(env, geom)});
  return scope.Escape(obj);
}

template <class T, class OGRT>
GeometryBaseNapi<T, OGRT>::GeometryBaseNapi()
  : this_(nullptr), owned_(false), size_(0), async_lock(nullptr) {
  async_lock = new uv_sem_t;
  uv_sem_init(async_lock, 1);
}

template <class T, class OGRT>
GeometryBaseNapi<T, OGRT>::~GeometryBaseNapi() {
  if (this_) {
    if (owned_) {
      OGRGeometryFactory::destroyGeometry(this_);
    }
    this_ = nullptr;
  }
  if (async_lock) {
    uv_sem_destroy(async_lock);
    delete async_lock;
    async_lock = nullptr;
  }
}

} // namespace node_gdal

#endif
