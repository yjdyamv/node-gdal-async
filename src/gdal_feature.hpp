#ifndef __NODE_OGR_FEATURE_H__
#define __NODE_OGR_FEATURE_H__

// node

// nan
#include "gdal_common.hpp"

// ogr
#include <ogrsf_frmts.h>

#include <ogr_featurestyle.h>

namespace node_gdal {

class Feature : public GDALObject<Feature> {
    public:
  static Napi::FunctionReference constructor;
  static void Initialize(Napi::Object target);
  static Napi::Value New(OGRFeature *feature);
  static Napi::Value New(OGRFeature *feature, bool owned);
  static NAN_METHOD(toString);
  static NAN_METHOD(getGeometry);
  //	static NAN_METHOD(setGeometryDirectly);
  static NAN_METHOD(setGeometry);
  //  static NAN_METHOD(stealGeometry);
  static NAN_METHOD(clone);
  static NAN_METHOD(equals);
  static NAN_METHOD(getFieldDefn);
  static NAN_METHOD(setFrom);
  static NAN_METHOD(destroy);
  static NAN_METHOD(getStyleString);
  static NAN_METHOD(setStyleString);

  static NAN_GETTER(fieldsGetter);
  static NAN_GETTER(fidGetter);
  static NAN_GETTER(defnGetter);

  static NAN_SETTER(fidSetter);

  Feature(const Napi::CallbackInfo &info);

  // Must be accessible: ObjectWrap's finalizer deletes the instance itself
  ~Feature();
  inline OGRFeature *get() {
    return this_;
  }
  inline bool isAlive() {
    return this_;
  }
  void dispose();

    private:
  OGRFeature *this_;
  bool owned_;
  // int size_;
};

} // namespace node_gdal
#endif
