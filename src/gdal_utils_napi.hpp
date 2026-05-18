#ifndef __GDAL_UTILS_NAPI_H__
#define __GDAL_UTILS_NAPI_H__

#include <napi.h>
#include <gdal_priv.h>
#include "gdal_common_napi.hpp"

namespace node_gdal {

namespace WarperNapi {
  Napi::Object Init(Napi::Env env, Napi::Object exports);
}
namespace AlgorithmsNapi {
  Napi::Object Init(Napi::Env env, Napi::Object exports);
}
namespace UtilsNapi {
  Napi::Object Init(Napi::Env env, Napi::Object exports);
}

} // namespace node_gdal

#endif
