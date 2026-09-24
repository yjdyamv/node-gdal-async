#include "layer_fields.hpp"
#include "../gdal_common.hpp"
#include "../gdal_field_defn.hpp"
#include "../gdal_layer.hpp"

namespace node_gdal {

Napi::FunctionReference LayerFields::constructor;

void LayerFields::Initialize(Napi::Object target) {
  Napi::Env env = target.Env();
  SELF_CLASS(LayerFields);

  // NOTE: the descriptor macros carry their own trailing comma
  Napi::Function lcons = DefineClass(env, "LayerFields",
    {
        METHOD(toString)
        METHOD(count)
        METHOD(get)
        METHOD(remove)
        METHOD(getNames)
        METHOD(indexOf)
        METHOD(reorder)
        METHOD(add)
        ATTR_DONT_ENUM(lcons, "layer", layerGetter, READ_ONLY_SETTER)
    });

  // Nan::SetPrototypeMethod(lcons, "alter", alter);

  target.Set("LayerFields", lcons);

  constructor = Napi::Persistent(lcons);
  constructor.SuppressDestruct();
}

LayerFields::LayerFields() : Nan::ObjectWrap() {
}

LayerFields::~LayerFields() {
}

/**
 * @class LayerFields
 */
NAN_METHOD(LayerFields::New) {

  if (!info.IsConstructCall()) {
    Napi::Error::New(node_gdal::napi_env, "Cannot call constructor as function, you need to use 'new' keyword").ThrowAsJavaScriptException();
    return node_gdal::napi_env.Undefined();
  }
  if (info[0].IsExternal()) {
    Local<External> ext = info[0].As<External>();
    void *ptr = ext->Value(V8_TYPE_TAG);
    LayerFields *layer = static_cast<LayerFields *>(ptr);
    layer->Wrap(info.This());
    return info.This();
    return node_gdal::napi_env.Undefined();
  } else {
    Napi::Error::New(node_gdal::napi_env, "Cannot create LayerFields directly").ThrowAsJavaScriptException();
    return node_gdal::napi_env.Undefined();
  }
}

Napi::Value LayerFields::New(Napi::Value layer_obj) {

  LayerFields *wrapped = new LayerFields();

  Napi::Value ext = Nan::New<External>(wrapped);
  v8::Local<v8::Object> obj =
    Nan::NewInstance(Nan::GetFunction(Napi::String::New(node_gdal::napi_env, LayerFields::constructor)), 1, &ext).ToLocalChecked();
  Nan::SetPrivate(obj, Napi::String::New(node_gdal::napi_env, "parent_"), layer_obj);

  return obj;
}

NAN_METHOD(LayerFields::toString) {
  return Napi::String::New(node_gdal::napi_env, "LayerFields");
}

/**
 * Returns the number of fields.
 *
 * @method count
 * @instance
 * @memberof LayerFields
 * @return {number}
 */
NAN_METHOD(LayerFields::count) {

  Napi::Object parent =
    Nan::GetPrivate(info.This(), Napi::String::New(node_gdal::napi_env, "parent_")).ToLocalChecked().As<Object>();
  Layer *layer = node_gdal::UnwrapWrapped<Layer>(parent);
  if (!layer->isAlive()) {
    Napi::Error::New(node_gdal::napi_env, "Layer object already destroyed").ThrowAsJavaScriptException();
    return node_gdal::napi_env.Undefined();
  }

  OGRFeatureDefn *def = layer->get()->GetLayerDefn();
  if (!def) {
    Napi::Error::New(node_gdal::napi_env, "Layer has no layer definition set").ThrowAsJavaScriptException();
    return node_gdal::napi_env.Undefined();
  }

  return Napi::Number::New(node_gdal::napi_env, def->GetFieldCount());
}

/**
 * Find the index of field in the layer.
 *
 * @method indexOf
 * @instance
 * @memberof LayerFields
 * @param {string} field
 * @return {number} Field index, or -1 if the field doesn't exist
 */
NAN_METHOD(LayerFields::indexOf) {

  Napi::Object parent =
    Nan::GetPrivate(info.This(), Napi::String::New(node_gdal::napi_env, "parent_")).ToLocalChecked().As<Object>();
  Layer *layer = node_gdal::UnwrapWrapped<Layer>(parent);
  if (!layer->isAlive()) {
    Napi::Error::New(node_gdal::napi_env, "Layer object already destroyed").ThrowAsJavaScriptException();
    return node_gdal::napi_env.Undefined();
  }

  OGRFeatureDefn *def = layer->get()->GetLayerDefn();
  if (!def) {
    Napi::Error::New(node_gdal::napi_env, "Layer has no layer definition set").ThrowAsJavaScriptException();
    return node_gdal::napi_env.Undefined();
  }

  std::string name("");
  NODE_ARG_STR(0, "field name", name);

  return Napi::Number::New(node_gdal::napi_env, def->GetFieldIndex(name.c_str()));
}

/**
 * Returns a field definition.
 *
 * @throws {Error}
 * @method get
 * @instance
 * @memberof LayerFields
 * @param {string|number} field Field name or index (0-based)
 * @return {FieldDefn}
 */
NAN_METHOD(LayerFields::get) {

  Napi::Object parent =
    Nan::GetPrivate(info.This(), Napi::String::New(node_gdal::napi_env, "parent_")).ToLocalChecked().As<Object>();
  Layer *layer = node_gdal::UnwrapWrapped<Layer>(parent);
  if (!layer->isAlive()) {
    Napi::Error::New(node_gdal::napi_env, "Layer object already destroyed").ThrowAsJavaScriptException();
    return node_gdal::napi_env.Undefined();
  }

  if (info.Length() < 1) {
    Napi::Error::New(node_gdal::napi_env, "Field index or name must be given").ThrowAsJavaScriptException();
    return node_gdal::napi_env.Undefined();
  }

  OGRFeatureDefn *def = layer->get()->GetLayerDefn();
  if (!def) {
    Napi::Error::New(node_gdal::napi_env, "Layer has no layer definition set").ThrowAsJavaScriptException();
    return node_gdal::napi_env.Undefined();
  }

  int field_index;
  ARG_FIELD_ID(0, def, field_index);

  auto r = def->GetFieldDefn(field_index);
  if (r == nullptr) {
    NODE_THROW_LAST_CPLERR;
    return node_gdal::napi_env.Undefined();
  }
  return FieldDefn::New(r);
}

/**
 * Returns a list of field names.
 *
 * @throws {Error}
 * @method getNames
 * @instance
 * @memberof LayerFields
 * @return {string[]} List of strings.
 */
NAN_METHOD(LayerFields::getNames) {

  Napi::Object parent =
    Nan::GetPrivate(info.This(), Napi::String::New(node_gdal::napi_env, "parent_")).ToLocalChecked().As<Object>();
  Layer *layer = node_gdal::UnwrapWrapped<Layer>(parent);
  if (!layer->isAlive()) {
    Napi::Error::New(node_gdal::napi_env, "Layer object already destroyed").ThrowAsJavaScriptException();
    return node_gdal::napi_env.Undefined();
  }

  OGRFeatureDefn *def = layer->get()->GetLayerDefn();
  if (!def) {
    Napi::Error::New(node_gdal::napi_env, "Layer has no layer definition set").ThrowAsJavaScriptException();
    return node_gdal::napi_env.Undefined();
  }

  int n = def->GetFieldCount();
  Napi::Array result = Napi::Array::New(node_gdal::napi_env, n);

  for (int i = 0; i < n; i++) {
    OGRFieldDefn *field_def = def->GetFieldDefn(i);
    result.Set( i, SafeString::New(field_def->GetNameRef()));
  }

  return result;
}

/**
 * Removes a field.
 *
 * @throws {Error}
 * @method remove
 * @instance
 * @memberof LayerFields
 * @param {string|number} field Field name or index (0-based)
 */
NAN_METHOD(LayerFields::remove) {

  Napi::Object parent =
    Nan::GetPrivate(info.This(), Napi::String::New(node_gdal::napi_env, "parent_")).ToLocalChecked().As<Object>();
  Layer *layer = node_gdal::UnwrapWrapped<Layer>(parent);
  if (!layer->isAlive()) {
    Napi::Error::New(node_gdal::napi_env, "Layer object already destroyed").ThrowAsJavaScriptException();
    return node_gdal::napi_env.Undefined();
  }

  if (info.Length() < 1) {
    Napi::Error::New(node_gdal::napi_env, "Field index or name must be given").ThrowAsJavaScriptException();
    return node_gdal::napi_env.Undefined();
  }

  OGRFeatureDefn *def = layer->get()->GetLayerDefn();
  if (!def) {
    Napi::Error::New(node_gdal::napi_env, "Layer has no layer definition set").ThrowAsJavaScriptException();
    return node_gdal::napi_env.Undefined();
  }

  int field_index;
  ARG_FIELD_ID(0, def, field_index);

  int err = layer->get()->DeleteField(field_index);
  if (err) {
    NODE_THROW_OGRERR(err);
    return node_gdal::napi_env.Undefined();
  }

  return node_gdal::napi_env.Undefined();
}

/**
 * Adds field(s).
 *
 * @throws {Error}
 * @method add
 * @instance
 * @memberof LayerFields
 * @param {FieldDefn|FieldDefn[]} defs A field definition, or array of field
 * definitions.
 * @param {boolean} [approx=true]
 */
NAN_METHOD(LayerFields::add) {

  Napi::Object parent =
    Nan::GetPrivate(info.This(), Napi::String::New(node_gdal::napi_env, "parent_")).ToLocalChecked().As<Object>();
  Layer *layer = node_gdal::UnwrapWrapped<Layer>(parent);
  if (!layer->isAlive()) {
    Napi::Error::New(node_gdal::napi_env, "Layer object already destroyed").ThrowAsJavaScriptException();
    return node_gdal::napi_env.Undefined();
  }
  if (info.Length() < 1) {
    Napi::Error::New(node_gdal::napi_env, "field definition(s) must be given").ThrowAsJavaScriptException();
    return node_gdal::napi_env.Undefined();
  }

  FieldDefn *field_def;
  int err;
  int approx = 1;
  NODE_ARG_BOOL_OPT(1, "approx", approx);

  if (info[0].IsArray()) {
    Napi::Array array = info[0].As<Array>();
    int n = array->Length();
    for (int i = 0; i < n; i++) {
      Napi::Value element = Nan::Get(array, i).ToLocalChecked();
      if (IS_WRAPPED(element, FieldDefn)) {
        field_def = node_gdal::UnwrapWrapped<FieldDefn>(element.As<Object>());
        err = layer->get()->CreateField(field_def->get(), approx);
        if (err) {
          NODE_THROW_OGRERR(err);
          return node_gdal::napi_env.Undefined();
        }
      } else {
        Napi::Error::New(node_gdal::napi_env, "All array elements must be FieldDefn objects").ThrowAsJavaScriptException();
        return node_gdal::napi_env.Undefined();
      }
    }
  } else if (IS_WRAPPED(info[0], FieldDefn)) {
    field_def = node_gdal::UnwrapWrapped<FieldDefn>(info[0].As<Object>());
    err = layer->get()->CreateField(field_def->get(), approx);
    if (err) {
      NODE_THROW_OGRERR(err);
      return node_gdal::napi_env.Undefined();
    }
  } else {
    Napi::Error::New(node_gdal::napi_env, "field definition(s) must be a FieldDefn object or array of FieldDefn objects").ThrowAsJavaScriptException();
    return node_gdal::napi_env.Undefined();
  }

  return node_gdal::napi_env.Undefined();
}

/**
 * Reorders fields.
 *
 * @example
 *
 * // reverse field order
 * layer.fields.reorder([2,1,0]);
 *
 * @throws {Error}
 * @method reorder
 * @instance
 * @memberof LayerFields
 * @param {number[]} map An array of new indexes (integers)
 */
NAN_METHOD(LayerFields::reorder) {

  Napi::Object parent =
    Nan::GetPrivate(info.This(), Napi::String::New(node_gdal::napi_env, "parent_")).ToLocalChecked().As<Object>();
  Layer *layer = node_gdal::UnwrapWrapped<Layer>(parent);
  if (!layer->isAlive()) {
    Napi::Error::New(node_gdal::napi_env, "Layer object already destroyed").ThrowAsJavaScriptException();
    return node_gdal::napi_env.Undefined();
  }

  OGRFeatureDefn *def = layer->get()->GetLayerDefn();
  if (!def) {
    Napi::Error::New(node_gdal::napi_env, "Layer has no layer definition set").ThrowAsJavaScriptException();
    return node_gdal::napi_env.Undefined();
  }

  Napi::Array field_map = Napi::Array::New(node_gdal::napi_env, 0);
  NODE_ARG_ARRAY(0, "field map", field_map);

  int n = def->GetFieldCount();
  OGRErr err = 0;

  if ((int)field_map->Length() != n) {
    Napi::Error::New(node_gdal::napi_env, "Array length must match field count").ThrowAsJavaScriptException();
    return node_gdal::napi_env.Undefined();
  }

  int *field_map_array = new int[n];

  for (int i = 0; i < n; i++) {
    Napi::Value val = Nan::Get(field_map, i).ToLocalChecked();
    if (!val->IsNumber()) {
      delete[] field_map_array;
      Napi::Error::New(node_gdal::napi_env, "Array must only contain integers").ThrowAsJavaScriptException();
      return node_gdal::napi_env.Undefined();
    }

    int key = Nan::To<int64_t>(val).ToChecked();
    if (key < 0 || key >= n) {
      delete[] field_map_array;
      Napi::Error::New(node_gdal::napi_env, "Values must be between 0 and field count - 1").ThrowAsJavaScriptException();
      return node_gdal::napi_env.Undefined();
    }

    field_map_array[i] = key;
  }

  err = layer->get()->ReorderFields(field_map_array);

  delete[] field_map_array;

  if (err) {
    NODE_THROW_OGRERR(err);
    return node_gdal::napi_env.Undefined();
  }
  return node_gdal::napi_env.Undefined();
}

/**
 * Returns the parent layer.
 *
 * @readonly
 * @kind member
 * @name layer
 * @instance
 * @memberof LayerFields
 * @type {Layer}
 */
NAN_GETTER(LayerFields::layerGetter) {
  return Nan::GetPrivate(info.This(), Napi::String::New(node_gdal::napi_env, "parent_")).ToLocalChecked();
}

} // namespace node_gdal
