#ifndef __NODE_OGR_FEATURE_DEFN_H__
#define __NODE_OGR_FEATURE_DEFN_H__

// node

// nan
#include "gdal_common.hpp"

// ogr
#include <ogrsf_frmts.h>

namespace node_gdal {

class FeatureDefn : public GDALObject<FeatureDefn> {
    public:
  static Napi::FunctionReference constructor;
  static void Initialize(Napi::Object target);
  static Napi::Value New(const OGRFeatureDefn *def);
  static Napi::Value New(OGRFeatureDefn *def, bool owned);
  static NAN_METHOD(toString);
  static NAN_METHOD(clone);

  static NAN_GETTER(fieldsGetter);
  static NAN_GETTER(nameGetter);
  static NAN_GETTER(geomTypeGetter);
  static NAN_GETTER(geomIgnoredGetter);
  static NAN_GETTER(styleIgnoredGetter);

  static NAN_SETTER(geomTypeSetter);
  static NAN_SETTER(geomIgnoredSetter);
  static NAN_SETTER(styleIgnoredSetter);

  FeatureDefn(const Napi::CallbackInfo &info);

  // Must be accessible: ObjectWrap's finalizer deletes the instance itself
  ~FeatureDefn();
  inline OGRFeatureDefn *get() {
    return this_;
  }
  inline bool isAlive() {
    return this_;
  }

    private:
  OGRFeatureDefn *this_;
  bool owned_;
};

} // namespace node_gdal
#endif
