#include "gdal_group.hpp"
#include "gdal_common.hpp"
#include "gdal_driver.hpp"
#include "collections/group_groups.hpp"
#include "collections/group_arrays.hpp"
#include "collections/group_dimensions.hpp"
#include "collections/group_attributes.hpp"
#include "geometry/gdal_geometry.hpp"
#include "gdal_layer.hpp"
#include "gdal_majorobject.hpp"
#include "gdal_rasterband.hpp"
#include "gdal_spatial_reference.hpp"

namespace node_gdal {

#if GDAL_VERSION_MAJOR > 3 || (GDAL_VERSION_MAJOR == 3 && GDAL_VERSION_MINOR >= 1)

Napi::FunctionReference Group::constructor;

void Group::Initialize(Napi::Object target) {
  Napi::Env env = target.Env();
  SELF_CLASS(Group);

  // NOTE: the descriptor macros carry their own trailing comma
  Napi::Function lcons = DefineClass(env, "Group",
    {
        METHOD(toString)
        ATTR_DONT_ENUM(lcons, "_uid", uidGetter, READ_ONLY_SETTER)
        ATTR(lcons, "description", descriptionGetter, READ_ONLY_SETTER)
        ATTR(lcons, "groups", groupsGetter, READ_ONLY_SETTER)
        ATTR(lcons, "arrays", arraysGetter, READ_ONLY_SETTER)
        ATTR(lcons, "dimensions", dimensionsGetter, READ_ONLY_SETTER)
        ATTR(lcons, "attributes", attributesGetter, READ_ONLY_SETTER)
    });

  target.Set("Group", lcons);

  constructor = Napi::Persistent(lcons);
  constructor.SuppressDestruct();
}

Group::Group(std::shared_ptr<GDALGroup> group) : Nan::ObjectWrap(), uid(0), this_(group), parent_ds(0) {
  LOG("Created group [%p]", group.get());
}

Group::Group(const Napi::CallbackInfo &info) : GDALObject<Group>(info), uid(0), this_(0), parent_ds(0) {
}

Group::~Group() {
  dispose();
}

void Group::dispose() {
  if (this_) {

    LOG("Disposing group [%p]", this_.get());

    object_store.dispose(uid);

    LOG("Disposed group [%p]", this_.get());
  }
};

/**
 * A representation of a group with access methods.
 *
 * @class Group
 */
NAN_METHOD(Group::New) {

  if (!info.IsConstructCall()) {
    Napi::Error::New(node_gdal::napi_env, "Cannot call constructor as function, you need to use 'new' keyword").ThrowAsJavaScriptException();
    return node_gdal::napi_env.Undefined();
  }

  if (info.Length() > 1 && info[0].IsExternal() && info[1].IsObject()) {
    Local<External> ext = info[0].As<Napi::External<void>>();
    void *ptr = ext->Value();
    Group *f = static_cast<Group *>(ptr);
    f->Wrap(info.This());

    Napi::Value groups = GroupGroups::New(info.This(), info[1]);
    GDAL_SET_PRIVATE(info.This(), "groups_", groups);
    Napi::Value arrays = GroupArrays::New(info.This(), info[1]);
    GDAL_SET_PRIVATE(info.This(), "arrays_", arrays);
    Napi::Value dims = GroupDimensions::New(info.This(), info[1]);
    GDAL_SET_PRIVATE(info.This(), "dims_", dims);
    Napi::Value attrs = GroupAttributes::New(info.This(), info[1]);
    GDAL_SET_PRIVATE(info.This(), "attrs_", attrs);

    return info.This();
    return node_gdal::napi_env.Undefined();
  } else {
    Napi::Error::New(node_gdal::napi_env, "Cannot create group directly. Create with dataset instead.").ThrowAsJavaScriptException();
    return node_gdal::napi_env.Undefined();
  }

  return info.This();
}

Napi::Value Group::New(std::shared_ptr<GDALGroup> raw, GDALDataset *parent_ds) {

  if (object_store.has(parent_ds)) {
    Napi::Object ds = object_store.get(parent_ds);
    return Group::New(raw, ds);
  } else {
    LOG("Group's parent dataset disappeared from cache (group = %p, dataset = %p)", raw.get(), parent_ds);
    Napi::Error::New(node_gdal::napi_env, "Group's parent dataset disappeared from cache").ThrowAsJavaScriptException();
    return node_gdal::napi_env.Undefined();
  }
}

Napi::Value Group::New(std::shared_ptr<GDALGroup> raw, Napi::Object parent_ds) {

  if (!raw) { return node_gdal::napi_env.Null(); }
  if (object_store.has(raw)) { return object_store.get(raw); }

  Group *wrapped = new Group(raw);

  long parent_group_uid = 0;
  Napi::Object parent;

  Napi::Value ext = Nan::New<External>(wrapped);
  Napi::Value argv[] = {ext, parent_ds};
  Napi::Object obj =
    Nan::NewInstance(Group::constructor.Value(), 2, argv);

  Dataset *unwrapped_ds = node_gdal::UnwrapWrapped<Dataset>(parent_ds);
  long parent_uid = unwrapped_ds->uid;

  wrapped->uid = object_store.add(raw, wrapped->persistent(), parent_uid);
  wrapped->parent_ds = unwrapped_ds->get();
  wrapped->parent_uid = parent_uid;
  GDAL_SET_PRIVATE(obj, "ds_", parent_ds);
  if (parent_group_uid != 0) GDAL_SET_PRIVATE(obj, "parent_", parent);

  return obj;
}

NAN_METHOD(Group::toString) {
  return Napi::String::New(node_gdal::napi_env, "Group");
}

/**
 * @readonly
 * @kind member
 * @name description
 * @instance
 * @memberof Group
 * @type {string}
 */
NODE_WRAPPED_GETTER_WITH_STRING_LOCKED(Group, descriptionGetter, GetFullName);

/**
 * @readonly
 * @kind member
 * @name groups
 * @instance
 * @memberof Group
 * @type {GroupGroups}
 */
NAN_GETTER(Group::groupsGetter) {
  return GDAL_GET_PRIVATE(info.This(), "groups_");
}

/**
 * @readonly
 * @kind member
 * @name arrays
 * @instance
 * @memberof Group
 * @type {GroupArrays}
 */
NAN_GETTER(Group::arraysGetter) {
  return GDAL_GET_PRIVATE(info.This(), "arrays_");
}

/**
 * @readonly
 * @kind member
 * @name dimensions
 * @instance
 * @memberof Group
 * @type {GroupDimensions}
 */
NAN_GETTER(Group::dimensionsGetter) {
  return GDAL_GET_PRIVATE(info.This(), "dims_");
}

/**
 * @readonly
 * @kind member
 * @name attributes
 * @instance
 * @memberof Group
 * @type {GroupAttributes}
 */
NAN_GETTER(Group::attributesGetter) {
  return GDAL_GET_PRIVATE(info.This(), "attrs_");
}

NAN_GETTER(Group::uidGetter) {
  Group *group = node_gdal::UnwrapWrapped<Group>(info.This().As<Napi::Object>());
  return Napi::Number::New(node_gdal::napi_env, (int)group->uid);
}

#endif

} // namespace node_gdal
