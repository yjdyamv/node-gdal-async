#include "gdal_spatial_reference_napi.hpp"

namespace node_gdal {

Napi::FunctionReference SpatialReferenceNapi::constructor;

// ---------------------------------------------------------------------------
// Helper macros for repetitive SRS patterns
// ---------------------------------------------------------------------------

// OGRERR-throwing methods
#define SRS_WRAPPED_OGRERR(method, wrapped)                                                    \
  Napi::Value SpatialReferenceNapi::method(const Napi::CallbackInfo &info) {                    \
    NAPI_UNWRAP_THIS(SpatialReferenceNapi, srs);                                                 \
    int err = srs->this_->wrapped();                                                             \
    if (err) NAPI_THROW_OGRERR(err);                                                            \
    return info.Env().Undefined();                                                              \
  }

// OGRERR with 1 string param
#define SRS_WRAPPED_OGRERR_1STR(method, wrapped)                                                \
  Napi::Value SpatialReferenceNapi::method(const Napi::CallbackInfo &info) {                    \
    NAPI_UNWRAP_THIS(SpatialReferenceNapi, srs);                                                 \
    std::string arg;                                                                             \
    NAPI_ARG_STR(0, "input", arg);                                                                \
    int err = srs->this_->wrapped(arg.c_str());                                                  \
    if (err) NAPI_THROW_OGRERR(err);                                                            \
    return info.Env().Undefined();                                                              \
  }

// Boolean return
#define SRS_WRAPPED_BOOL(method, wrapped)                                                       \
  Napi::Value SpatialReferenceNapi::method(const Napi::CallbackInfo &info) {                    \
    NAPI_UNWRAP_THIS(SpatialReferenceNapi, srs);                                                 \
    return Napi::Boolean::New(info.Env(), srs->this_->wrapped());                               \
  }

// String return
#define SRS_WRAPPED_STR(method, wrapped)                                                        \
  Napi::Value SpatialReferenceNapi::method(const Napi::CallbackInfo &info) {                    \
    NAPI_UNWRAP_THIS(SpatialReferenceNapi, srs);                                                 \
    return SafeStringNapi(info.Env(), srs->this_->wrapped());                                   \
  }

// 1 Wrapped param returning bool
#define SRS_WRAPPED_1W_BOOL(method, wrapped)                                                    \
  Napi::Value SpatialReferenceNapi::method(const Napi::CallbackInfo &info) {                    \
    NAPI_UNWRAP_THIS(SpatialReferenceNapi, srs);                                                 \
    SpatialReferenceNapi *other;                                                                 \
    NAPI_ARG_WRAPPED(0, "srs", SpatialReferenceNapi, other);                                    \
    return Napi::Boolean::New(info.Env(), srs->this_->wrapped(other->this_));                   \
  }

// 1 String param returning string
#define SRS_WRAPPED_1STR_STR(method, wrapped)                                                   \
  Napi::Value SpatialReferenceNapi::method(const Napi::CallbackInfo &info) {                    \
    NAPI_UNWRAP_THIS(SpatialReferenceNapi, srs);                                                 \
    std::string arg;                                                                             \
    NAPI_ARG_STR(0, "input", arg);                                                                \
    return SafeStringNapi(info.Env(), srs->this_->wrapped(arg.c_str()));                        \
  }

// Export (string return)
#define SRS_WRAPPED_EXPORT(method, wrapped)                                                     \
  Napi::Value SpatialReferenceNapi::method(const Napi::CallbackInfo &info) {                    \
    NAPI_UNWRAP_THIS(SpatialReferenceNapi, srs);                                                 \
    char *result = nullptr;                                                                      \
    int err = srs->this_->wrapped(&result);                                                      \
    if (err) NAPI_THROW_OGRERR(err);                                                            \
    Napi::Value ret = Napi::String::New(info.Env(), result ? result : "");                       \
    CPLFree(result);                                                                             \
    return ret;                                                                                  \
  }

// ---------------------------------------------------------------------------
// Init
// ---------------------------------------------------------------------------
Napi::Object SpatialReferenceNapi::Init(Napi::Env env, Napi::Object exports) {
  Napi::Function func = DefineClass(
    env, "SpatialReferenceNapi",
    {
      InstanceMethod("toString", &SpatialReferenceNapi::toString),
      InstanceMethod("clone", &SpatialReferenceNapi::clone),
      InstanceMethod("cloneGeogCS", &SpatialReferenceNapi::cloneGeogCS),
      InstanceMethod("toWKT", &SpatialReferenceNapi::exportToWKT),
      InstanceMethod("toPrettyWKT", &SpatialReferenceNapi::exportToPrettyWKT),
      InstanceMethod("toProj4", &SpatialReferenceNapi::exportToProj4),
      InstanceMethod("toXML", &SpatialReferenceNapi::exportToXML),
      InstanceMethod("setWellKnownGeogCS", &SpatialReferenceNapi::setWellKnownGeogCS),
      InstanceMethod("morphToESRI", &SpatialReferenceNapi::morphToESRI),
      InstanceMethod("morphFromESRI", &SpatialReferenceNapi::morphFromESRI),
      InstanceMethod("EPSGTreatsAsLatLong", &SpatialReferenceNapi::EPSGTreatsAsLatLong),
      InstanceMethod("EPSGTreatsAsNorthingEasting", &SpatialReferenceNapi::EPSGTreatsAsNorthingEasting),
      InstanceMethod("getLinearUnits", &SpatialReferenceNapi::getLinearUnits),
      InstanceMethod("getAngularUnits", &SpatialReferenceNapi::getAngularUnits),
      InstanceMethod("isGeocentric", &SpatialReferenceNapi::isGeocentric),
      InstanceMethod("isGeographic", &SpatialReferenceNapi::isGeographic),
      InstanceMethod("isProjected", &SpatialReferenceNapi::isProjected),
      InstanceMethod("isLocal", &SpatialReferenceNapi::isLocal),
      InstanceMethod("isVertical", &SpatialReferenceNapi::isVertical),
      InstanceMethod("isCompound", &SpatialReferenceNapi::isCompound),
      InstanceMethod("isSameGeogCS", &SpatialReferenceNapi::isSameGeogCS),
      InstanceMethod("isSameVertCS", &SpatialReferenceNapi::isSameVertCS),
      InstanceMethod("isSame", &SpatialReferenceNapi::isSame),
      InstanceMethod("autoIdentifyEPSG", &SpatialReferenceNapi::autoIdentifyEPSG),
      InstanceMethod("getAuthorityName", &SpatialReferenceNapi::getAuthorityName),
      InstanceMethod("getAuthorityCode", &SpatialReferenceNapi::getAuthorityCode),
      InstanceMethod("getAttrValue", &SpatialReferenceNapi::getAttrValue),
      InstanceMethod("validate", &SpatialReferenceNapi::validate),
    });
  constructor = Napi::Persistent(func);
  constructor.SuppressDestruct();
  exports.Set("SpatialReferenceNapi", func);
  return exports;
}

// ---------------------------------------------------------------------------
// Constructor
// ---------------------------------------------------------------------------
SpatialReferenceNapi::SpatialReferenceNapi(const Napi::CallbackInfo &info)
  : Napi::ObjectWrap<SpatialReferenceNapi>(info), this_(nullptr), owned_(false) {

  if (!info.IsConstructCall()) {
    Napi::Error::New(info.Env(), "Cannot call constructor as function, use 'new' keyword")
      .ThrowAsJavaScriptException();
    return;
  }

  if (info.Length() > 0 && info[0].IsExternal()) {
    this_ = info[0].As<Napi::External<OGRSpatialReference>>().Data();
    owned_ = true;
  } else {
    std::string wkt;
    if (info.Length() > 0 && info[0].IsString()) {
      wkt = info[0].As<Napi::String>().Utf8Value();
    }
    this_ = new OGRSpatialReference(wkt.empty() ? nullptr : wkt.c_str());
    if (!wkt.empty()) {
      const char *wkt_c = wkt.c_str();
      int err = this_->importFromWkt(&wkt_c);
      if (err) {
        delete this_;
        this_ = nullptr;
        const char *msg = (err == 6) ? CPLGetLastErrorMsg() : "Failed to import WKT";
        Napi::Error::New(info.Env(), msg).ThrowAsJavaScriptException();
        return;
      }
    }
    owned_ = true;
  }
}

SpatialReferenceNapi::~SpatialReferenceNapi() {
  if (this_ && owned_) {
    this_->Release();
  }
  this_ = nullptr;
}

// ---------------------------------------------------------------------------
// Static factories
// ---------------------------------------------------------------------------
Napi::Value SpatialReferenceNapi::New(Napi::Env env, const OGRSpatialReference *srs) {
  return SpatialReferenceNapi::New(env, srs->Clone(), true);
}
Napi::Value SpatialReferenceNapi::New(Napi::Env env, OGRSpatialReference *srs, bool owned) {
  Napi::EscapableHandleScope scope(env);
  if (!srs) return scope.Escape(env.Null());
  if (!owned) { srs = srs->Clone(); }
  return scope.Escape(
    constructor.New({Napi::External<OGRSpatialReference>::New(env, srs)}));
}

// ---------------------------------------------------------------------------
// toString
// ---------------------------------------------------------------------------
Napi::Value SpatialReferenceNapi::toString(const Napi::CallbackInfo &info) {
  return Napi::String::New(info.Env(), "SpatialReference");
}

// ---------------------------------------------------------------------------
// clone / cloneGeogCS
// ---------------------------------------------------------------------------
Napi::Value SpatialReferenceNapi::clone(const Napi::CallbackInfo &info) {
  NAPI_UNWRAP_THIS(SpatialReferenceNapi, srs);
  return SpatialReferenceNapi::New(info.Env(), srs->this_->Clone());
}
Napi::Value SpatialReferenceNapi::cloneGeogCS(const Napi::CallbackInfo &info) {
  NAPI_UNWRAP_THIS(SpatialReferenceNapi, srs);
  return SpatialReferenceNapi::New(info.Env(), srs->this_->CloneGeogCS());
}

// ---------------------------------------------------------------------------
// Export methods
// ---------------------------------------------------------------------------
SRS_WRAPPED_EXPORT(exportToWKT, exportToWkt)
SRS_WRAPPED_EXPORT(exportToPrettyWKT, exportToPrettyWkt)
SRS_WRAPPED_EXPORT(exportToProj4, exportToProj4)
SRS_WRAPPED_EXPORT(exportToXML, exportToXML)

// ---------------------------------------------------------------------------
// OGRERR methods
// ---------------------------------------------------------------------------
SRS_WRAPPED_OGRERR(morphToESRI, morphToESRI)
SRS_WRAPPED_OGRERR(morphFromESRI, morphFromESRI)
SRS_WRAPPED_OGRERR_1STR(setWellKnownGeogCS, SetWellKnownGeogCS)
SRS_WRAPPED_OGRERR(autoIdentifyEPSG, AutoIdentifyEPSG)

// ---------------------------------------------------------------------------
// Boolean getters
// ---------------------------------------------------------------------------
SRS_WRAPPED_BOOL(isGeocentric, IsGeocentric)
SRS_WRAPPED_BOOL(isGeographic, IsGeographic)
SRS_WRAPPED_BOOL(isProjected, IsProjected)
SRS_WRAPPED_BOOL(isLocal, IsLocal)
SRS_WRAPPED_BOOL(isVertical, IsVertical)
SRS_WRAPPED_BOOL(isCompound, IsCompound)
SRS_WRAPPED_BOOL(EPSGTreatsAsLatLong, EPSGTreatsAsLatLong)
SRS_WRAPPED_BOOL(EPSGTreatsAsNorthingEasting, EPSGTreatsAsNorthingEasting)

// ---------------------------------------------------------------------------
// 1 Wrapped param bool
// ---------------------------------------------------------------------------
SRS_WRAPPED_1W_BOOL(isSameGeogCS, IsSameGeogCS)
SRS_WRAPPED_1W_BOOL(isSameVertCS, IsSameVertCS)
SRS_WRAPPED_1W_BOOL(isSame, IsSame)

// ---------------------------------------------------------------------------
// 1 String param returns string
// ---------------------------------------------------------------------------
SRS_WRAPPED_1STR_STR(getAttrValue, GetAttrValue)
SRS_WRAPPED_1STR_STR(getAuthorityName, GetAuthorityName)
SRS_WRAPPED_1STR_STR(getAuthorityCode, GetAuthorityCode)

// ---------------------------------------------------------------------------
// getLinearUnits / getAngularUnits (return {value, units})
// ---------------------------------------------------------------------------
Napi::Value SpatialReferenceNapi::getLinearUnits(const Napi::CallbackInfo &info) {
  NAPI_UNWRAP_THIS(SpatialReferenceNapi, srs);
  const char *unit_name = nullptr;
  double units = srs->this_->GetLinearUnits(&unit_name);
  Napi::Object result = Napi::Object::New(info.Env());
  result.Set("value", Napi::Number::New(info.Env(), units));
  result.Set("units", SafeStringNapi(info.Env(), unit_name));
  return result;
}
Napi::Value SpatialReferenceNapi::getAngularUnits(const Napi::CallbackInfo &info) {
  NAPI_UNWRAP_THIS(SpatialReferenceNapi, srs);
  const char *unit_name = nullptr;
  double units = srs->this_->GetAngularUnits(&unit_name);
  Napi::Object result = Napi::Object::New(info.Env());
  result.Set("value", Napi::Number::New(info.Env(), units));
  result.Set("units", SafeStringNapi(info.Env(), unit_name));
  return result;
}

// ---------------------------------------------------------------------------
// validate
// ---------------------------------------------------------------------------
Napi::Value SpatialReferenceNapi::validate(const Napi::CallbackInfo &info) {
  NAPI_UNWRAP_THIS(SpatialReferenceNapi, srs);
  OGRErr err = srs->this_->Validate();
  if (err == OGRERR_NONE) return info.Env().Null();
  if (err == OGRERR_CORRUPT_DATA) return Napi::String::New(info.Env(), "corrupt");
  if (err == OGRERR_UNSUPPORTED_SRS) return Napi::String::New(info.Env(), "unsupported");
  NAPI_THROW_OGRERR(err);
  return info.Env().Undefined();
}

// ---------------------------------------------------------------------------
// Static factories
// ---------------------------------------------------------------------------
#define SRS_FACTORY(name, method)                                                              \
  Napi::Value SpatialReferenceNapi::name(const Napi::CallbackInfo &info) {                     \
    std::string arg;                                                                            \
    NAPI_ARG_STR(0, "input", arg);                                                               \
    OGRSpatialReference *srs = new OGRSpatialReference();                                       \
    int err = srs->method(arg.c_str());                                                         \
    if (err) {                                                                                  \
      delete srs;                                                                               \
      NAPI_THROW_OGRERR(err);                                                                  \
    }                                                                                           \
    return SpatialReferenceNapi::New(info.Env(), srs, true);                                    \
  }

SRS_FACTORY(fromWKT, importFromWkt)
SRS_FACTORY(fromProj4, importFromProj4)
SRS_FACTORY(fromXML, importFromXML)
SRS_FACTORY(fromURN, importFromURN)

// Integer EPSG factories
Napi::Value SpatialReferenceNapi::fromEPSG(const Napi::CallbackInfo &info) {
  int epsg;
  NAPI_ARG_INT(0, "epsg", epsg);
  OGRSpatialReference *srs = new OGRSpatialReference();
  int err = srs->importFromEPSG(epsg);
  if (err) { delete srs; NAPI_THROW_OGRERR(err); }
  return SpatialReferenceNapi::New(info.Env(), srs, true);
}
Napi::Value SpatialReferenceNapi::fromEPSGA(const Napi::CallbackInfo &info) {
  int epsg;
  NAPI_ARG_INT(0, "epsg", epsg);
  OGRSpatialReference *srs = new OGRSpatialReference();
  int err = srs->importFromEPSGA(epsg);
  if (err) { delete srs; NAPI_THROW_OGRERR(err); }
  return SpatialReferenceNapi::New(info.Env(), srs, true);
}

// fromESRI uses StringList
Napi::Value SpatialReferenceNapi::fromESRI(const Napi::CallbackInfo &info) {
  auto list = std::make_shared<NapiStringList>();
  if (info.Length() < 1 || !list->parse(info[0])) {
    Napi::Error::New(info.Env(), "Failed parsing input string list").ThrowAsJavaScriptException();
    return info.Env().Undefined();
  }
  OGRSpatialReference *srs = new OGRSpatialReference();
  int err = srs->importFromESRI(list->get());
  if (err) { delete srs; NAPI_THROW_OGRERR(err); }
  return SpatialReferenceNapi::New(info.Env(), srs, true);
}

// fromWMSAUTO
Napi::Value SpatialReferenceNapi::fromWMSAUTO(const Napi::CallbackInfo &info) {
  std::string arg;
  NAPI_ARG_STR(0, "input", arg);
  OGRSpatialReference *srs = new OGRSpatialReference();
  int err = srs->importFromWMSAUTO(arg.c_str());
  if (err) { delete srs; NAPI_THROW_OGRERR(err); }
  return SpatialReferenceNapi::New(info.Env(), srs, true);
}

// fromMICoordSys
Napi::Value SpatialReferenceNapi::fromMICoordSys(const Napi::CallbackInfo &info) {
  std::string arg;
  NAPI_ARG_STR(0, "input", arg);
  OGRSpatialReference *srs = new OGRSpatialReference();
  int err = srs->importFromMICoordSys(arg.c_str());
  if (err) { delete srs; NAPI_THROW_OGRERR(err); }
  return SpatialReferenceNapi::New(info.Env(), srs, true);
}

// ---------------------------------------------------------------------------
// fromUserInput (async)
// ---------------------------------------------------------------------------
GDAL_ASYNCABLE_DEFINE_NAPI(SpatialReferenceNapi, fromUserInput) {
  std::string input;
  NAPI_ARG_STR(0, "input", input);

  GDALAsyncableJobNapi<OGRSpatialReference *> job;
  job.main = [input]() -> OGRSpatialReference * {
    OGRSpatialReference *srs = new OGRSpatialReference();
    int err = srs->SetFromUserInput(input.c_str());
    if (err) { delete srs; throw "Failed to parse SRS from user input"; }
    return srs;
  };
  job.rval = [](Napi::Env env, OGRSpatialReference *srs) -> Napi::Value {
    return SpatialReferenceNapi::New(env, srs, true);
  };
  return job.run(info, async, 1);
}

} // namespace node_gdal
