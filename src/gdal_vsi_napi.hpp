#ifndef __GDAL_VSI_NAPI_H__
#define __GDAL_VSI_NAPI_H__

#include <napi.h>
#include "gdal_common_napi.hpp"

namespace node_gdal {
namespace VsiNapi {
  Napi::Object Init(Napi::Env env, Napi::Object exports);
}
}

#endif
