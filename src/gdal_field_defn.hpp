#ifndef __NODE_OGR_FIELD_DEFN_H__
#define __NODE_OGR_FIELD_DEFN_H__

// node

// nan
#include "gdal_common.hpp"

// ogr
#include <ogrsf_frmts.h>

namespace node_gdal {

class FieldDefn : public GDALObject<FieldDefn> {
    public:
  static Napi::FunctionReference constructor;
  static void Initialize(Napi::Object target);
  static Napi::Value New(const OGRFieldDefn *def);
  static Napi::Value New(OGRFieldDefn *def, bool owned);
  static NAN_METHOD(toString);

  static NAN_GETTER(nameGetter);
  static NAN_GETTER(typeGetter);
  static NAN_GETTER(justificationGetter);
  static NAN_GETTER(precisionGetter);
  static NAN_GETTER(widthGetter);
  static NAN_GETTER(ignoredGetter);

  static NAN_SETTER(nameSetter);
  static NAN_SETTER(typeSetter);
  static NAN_SETTER(justificationSetter);
  static NAN_SETTER(precisionSetter);
  static NAN_SETTER(widthSetter);
  static NAN_SETTER(ignoredSetter);

  FieldDefn(const Napi::CallbackInfo &info);

  // Must be accessible: ObjectWrap's finalizer deletes the instance itself
  ~FieldDefn();
  inline OGRFieldDefn *get() {
    return this_;
  }
  inline bool isAlive() {
    return this_;
  }

    private:
  OGRFieldDefn *this_;
  bool owned_;
};

} // namespace node_gdal
#endif
