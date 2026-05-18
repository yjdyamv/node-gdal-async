#ifndef __NODE_OGR_SPATIALREFERENCE_NAPI_H__
#define __NODE_OGR_SPATIALREFERENCE_NAPI_H__

#include <napi.h>
#include <ogrsf_frmts.h>

#include "async_napi.hpp"

namespace node_gdal {

class SpatialReferenceNapi : public Napi::ObjectWrap<SpatialReferenceNapi> {
    public:
  static Napi::FunctionReference constructor;
  static Napi::Object Init(Napi::Env env, Napi::Object exports);
  static Napi::Value New(Napi::Env env, OGRSpatialReference *srs, bool owned = true);
  static Napi::Value New(Napi::Env env, const OGRSpatialReference *srs);

  SpatialReferenceNapi(const Napi::CallbackInfo &info);
  ~SpatialReferenceNapi();

  Napi::Value toString(const Napi::CallbackInfo &info);
  Napi::Value clone(const Napi::CallbackInfo &info);
  Napi::Value cloneGeogCS(const Napi::CallbackInfo &info);
  Napi::Value exportToWKT(const Napi::CallbackInfo &info);
  Napi::Value exportToPrettyWKT(const Napi::CallbackInfo &info);
  Napi::Value exportToProj4(const Napi::CallbackInfo &info);
  Napi::Value exportToXML(const Napi::CallbackInfo &info);
  Napi::Value setWellKnownGeogCS(const Napi::CallbackInfo &info);
  Napi::Value morphToESRI(const Napi::CallbackInfo &info);
  Napi::Value morphFromESRI(const Napi::CallbackInfo &info);
  Napi::Value EPSGTreatsAsLatLong(const Napi::CallbackInfo &info);
  Napi::Value EPSGTreatsAsNorthingEasting(const Napi::CallbackInfo &info);
  Napi::Value getLinearUnits(const Napi::CallbackInfo &info);
  Napi::Value getAngularUnits(const Napi::CallbackInfo &info);
  Napi::Value isGeocentric(const Napi::CallbackInfo &info);
  Napi::Value isGeographic(const Napi::CallbackInfo &info);
  Napi::Value isProjected(const Napi::CallbackInfo &info);
  Napi::Value isLocal(const Napi::CallbackInfo &info);
  Napi::Value isVertical(const Napi::CallbackInfo &info);
  Napi::Value isCompound(const Napi::CallbackInfo &info);
  Napi::Value isSameGeogCS(const Napi::CallbackInfo &info);
  Napi::Value isSameVertCS(const Napi::CallbackInfo &info);
  Napi::Value isSame(const Napi::CallbackInfo &info);
  Napi::Value autoIdentifyEPSG(const Napi::CallbackInfo &info);
  Napi::Value getAuthorityCode(const Napi::CallbackInfo &info);
  Napi::Value getAuthorityName(const Napi::CallbackInfo &info);
  Napi::Value getAttrValue(const Napi::CallbackInfo &info);
  Napi::Value validate(const Napi::CallbackInfo &info);

  // Async
  GDAL_ASYNCABLE_DECLARE_NAPI(fromUserInput);

  // Static factories
  static Napi::Value fromWKT(const Napi::CallbackInfo &info);
  static Napi::Value fromProj4(const Napi::CallbackInfo &info);
  static Napi::Value fromEPSG(const Napi::CallbackInfo &info);
  static Napi::Value fromEPSGA(const Napi::CallbackInfo &info);
  static Napi::Value fromESRI(const Napi::CallbackInfo &info);
  static Napi::Value fromWMSAUTO(const Napi::CallbackInfo &info);
  static Napi::Value fromXML(const Napi::CallbackInfo &info);
  static Napi::Value fromURN(const Napi::CallbackInfo &info);
  static Napi::Value fromMICoordSys(const Napi::CallbackInfo &info);

  OGRSpatialReference *get() {
    return this_;
  }
  bool isAlive() {
    return this_ != nullptr;
  }

    private:
  OGRSpatialReference *this_;
  bool owned_;
};

} // namespace node_gdal

#endif
