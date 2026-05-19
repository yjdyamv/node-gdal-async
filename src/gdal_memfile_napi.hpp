#ifndef __NODE_GDAL_MEMFILE_NAPI_H__
#define __NODE_GDAL_MEMFILE_NAPI_H__

#include <napi.h>
#include <gdal_priv.h>

namespace node_gdal {
namespace MemfileNapi {
  Napi::Object Init(Napi::Env env, Napi::Object exports);
}
} // namespace node_gdal

#endif
