#ifndef __GDAL_ALGEBRA_NAPI_H__
#define __GDAL_ALGEBRA_NAPI_H__

#include <napi.h>
#include <gdal_priv.h>
#include "gdal_common_napi.hpp"
#include "async_napi.hpp"

namespace node_gdal {
namespace AlgebraNapi {
  Napi::Object Init(Napi::Env env, Napi::Object exports);
}
}
#endif
