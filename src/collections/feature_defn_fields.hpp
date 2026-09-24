#ifndef __NODE_GDAL_FIELD_DEFN_COLLECTION_H__
#define __NODE_GDAL_FIELD_DEFN_COLLECTION_H__

// node

// nan
#include "../gdal_common.hpp"

// gdal
#include <gdal_priv.h>

// FeatureDefn.fields : FeatureDefnFields

namespace node_gdal {

class FeatureDefnFields : public GDALObject<FeatureDefnFields> {
    public:
  static Napi::FunctionReference constructor;

  static void Initialize(Napi::Object target);
  static Napi::Value New(Napi::Value layer_obj);
  static NAN_METHOD(toString);

  static NAN_METHOD(get);
  static NAN_METHOD(getNames);
  static NAN_METHOD(count);
  static NAN_METHOD(add);
  static NAN_METHOD(remove);
  static NAN_METHOD(indexOf);
  static NAN_METHOD(reorder);

  // - implement in the future -
  // static NAN_METHOD(alter);

  static NAN_GETTER(featureDefnGetter);

  FeatureDefnFields(const Napi::CallbackInfo &info);

  // Must be accessible: ObjectWrap's finalizer deletes the instance itself
  ~FeatureDefnFields();
    private:
};

} // namespace node_gdal
#endif
