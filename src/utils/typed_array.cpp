#include "typed_array.hpp"

#if GDAL_VERSION_MAJOR > 3 || (GDAL_VERSION_MAJOR == 3 && GDAL_VERSION_MINOR >= 11)
#include <cpl_float.h>
#endif

#include <climits>
#include <sstream>

const double max_safe_integer = std::numeric_limits<double>::radix / std::numeric_limits<double>::epsilon();

namespace node_gdal {

// https://github.com/joyent/node/issues/4201#issuecomment-9837340

Napi::Value TypedArray::New(GDALDataType type, int64_t length) {

  Napi::Value val;
  Local<Function> constructor;
  Napi::Object global = Nan::GetCurrentContext()->Global();

  const char *name;
  switch (type) {
    case GDT_Byte: name = "Uint8Array"; break;
    case GDT_Int16: name = "Int16Array"; break;
    case GDT_UInt16: name = "Uint16Array"; break;
    case GDT_Int32: name = "Int32Array"; break;
    case GDT_UInt32: name = "Uint32Array"; break;
#if GDAL_VERSION_MAJOR > 3 || (GDAL_VERSION_MAJOR == 3 && GDAL_VERSION_MINOR >= 5)
    case GDT_Int64: name = "BigInt64Array"; break;
    case GDT_UInt64: name = "BigUint64Array"; break;
#endif
#if GDAL_VERSION_MAJOR > 3 || (GDAL_VERSION_MAJOR == 3 && GDAL_VERSION_MINOR >= 11)
    case GDT_Float16: name = "Float16Array"; break;
#endif
    case GDT_Float32: name = "Float32Array"; break;
    case GDT_Float64: name = "Float64Array"; break;
    default: Napi::Error::New(node_gdal::napi_env(), "Unsupported array type").ThrowAsJavaScriptException(); return node_gdal::napi_env().Undefined();
  }

  // make ArrayBuffer
  val = global.As<Napi::Object>().Get(Napi::String::New(node_gdal::napi_env(), "ArrayBuffer"));
  if (!val.IsFunction()) {
    Napi::Error::New(node_gdal::napi_env(), "Error getting ArrayBuffer constructor").ThrowAsJavaScriptException();
    return Napi::Value();
  }

  constructor = val.As<Napi::Function>();
  int64_t size = length * GDALGetDataTypeSizeBytes(type);
  if (size == 0) {
    Napi::Error::New(node_gdal::napi_env(), "Invalid GDAL data type").ThrowAsJavaScriptException();
    return Napi::Value();
  }
  if (size > max_safe_integer) {
    Napi::Error::New(node_gdal::napi_env(), "Buffer size exceeds maximum safe JS integer").ThrowAsJavaScriptException();
    return Napi::Value();
  }
  Napi::Value v8_size = Nan::New<v8::Number>(size);
  MaybeNapi::Object array_buffer_maybe = Nan::NewInstance(constructor, 1, &v8_size);
  if (array_buffer_maybe.IsEmpty()) { return Napi::Value(); }
  Napi::Value array_buffer = array_buffer_maybe;
  if (!array_buffer->IsObject()) {
    Napi::Error::New(node_gdal::napi_env(), "Error allocating ArrayBuffer").ThrowAsJavaScriptException();
    return Napi::Value();
  }

  // make TypedArray
  val = global.As<Napi::Object>().Get(Napi::String::New(node_gdal::napi_env(), name));
  if (val.IsEmpty()) { return Napi::Value(); }
  if (!val->IsFunction()) {
    Napi::Error::New(node_gdal::napi_env(), "Error getting typed array constructor").ThrowAsJavaScriptException();
    return Napi::Value();
  }
  constructor = val.As<Napi::Function>();
  MaybeNapi::Object array_maybe = Nan::NewInstance(constructor, 1, &array_buffer);
  if (array_maybe.IsEmpty()) {
    Napi::RangeError::New(node_gdal::napi_env(), "Failed constructing a TypedArray, data is probably over the 4G elements limit").ThrowAsJavaScriptException();
    return Napi::Value();
  }
  Napi::Object array = array_maybe;

  array.Set( Napi::String::New(node_gdal::napi_env(), "_gdal_type"), Napi::Number::New(node_gdal::napi_env(), type));

  return array;
}

// Create a new TypedArray view over an existing memory buffer
// This function throws because it is meant to be used inside a pixel function
Napi::Value TypedArray::New(GDALDataType type, void *data, int64_t length) {

  Napi::Object global = Nan::GetCurrentContext()->Global();

  const char *name;
  switch (type) {
    case GDT_Byte: name = "Uint8Array"; break;
    case GDT_Int16: name = "Int16Array"; break;
    case GDT_UInt16: name = "Uint16Array"; break;
    case GDT_Int32: name = "Int32Array"; break;
    case GDT_UInt32: name = "Uint32Array"; break;
#if GDAL_VERSION_MAJOR > 3 || (GDAL_VERSION_MAJOR == 3 && GDAL_VERSION_MINOR >= 5)
    case GDT_Int64: name = "BigInt64Array"; break;
    case GDT_UInt64: name = "BigUint64Array"; break;
#endif
#if GDAL_VERSION_MAJOR > 3 || (GDAL_VERSION_MAJOR == 3 && GDAL_VERSION_MINOR >= 11)
    case GDT_Float16: name = "Float16Array"; break;
#endif
    case GDT_Float32: name = "Float32Array"; break;
    case GDT_Float64: name = "Float64Array"; break;
    default: throw "Unsupported array type";
  }

  size_t size = GDALGetDataTypeSizeBytes(type);
  if (size == 0) { throw "Invalid GDAL data type"; }

  // make ArrayBuffer with external storage by creating a Node.js Buffer w/ an empty free callback
  Napi::Object buffer =
    Nan::NewBuffer(reinterpret_cast<char *>(data), length * size, [](char *, void *) {}, nullptr);

  if (buffer.IsEmpty() || !buffer->IsObject()) { throw "Error getting creating Node.js Buffer"; }

  // get the underlying ArrayBuffer
  Napi::Value underlyingAB = buffer.As<Napi::Object>().Get(Napi::String::New(node_gdal::napi_env(), "buffer"));

  // make TypedArray
  Napi::Value val = global.As<Napi::Object>().Get(Napi::String::New(node_gdal::napi_env(), name));
  if (val.IsEmpty() || !val->IsFunction()) { throw "Error getting typed array constructor"; }
  Local<Function> constructor = val.As<Napi::Function>();

  Napi::Object array = Nan::NewInstance(constructor, 1, &underlyingAB);

  if (array.IsEmpty() || !array->IsObject()) { throw "Error creating TypedArray"; }

  array.Set( Napi::String::New(node_gdal::napi_env(), "_gdal_type"), Napi::Number::New(node_gdal::napi_env(), type));

  return array;
}

GDALDataType TypedArray::Identify(Napi::Object obj) {

  Local<String> sym = Napi::String::New(node_gdal::napi_env(), "_gdal_type");
  if (!obj.As<Napi::Object>().HasOwnProperty(sym)) return GDT_Unknown;
  Napi::Value val = obj.As<Napi::Object>().Get(sym);
  if (!val->IsNumber()) return GDT_Unknown;

  return (GDALDataType)val.As<Napi::Number>().Int32Value();
}

void *TypedArray::Validate(Napi::Object obj, GDALDataType type, int64_t min_length) {
  // validate array

  GDALDataType src_type = TypedArray::Identify(obj);
  if (src_type == GDT_Unknown) {
    Napi::TypeError::New(node_gdal::napi_env(), "Unable to identify GDAL datatype of passed array object").ThrowAsJavaScriptException();
    return NULL;
  }
  if (src_type != type) {
    std::ostringstream ss;
    ss << "Array type does not match band data type ("
       << "input: " << GDALGetDataTypeName(src_type) << ", target: " << GDALGetDataTypeName(type) << ")";

    Napi::TypeError::New(node_gdal::napi_env(), ss.str().c_str()).ThrowAsJavaScriptException();
    return NULL;
  }
  switch (type) {
    case GDT_Byte: {
      Nan::TypedArrayContents<GByte> contents(obj);
      if (ValidateLength(contents.length(), min_length)) return NULL;
      return *contents;
    }
    case GDT_Int16: {
      Nan::TypedArrayContents<GInt16> contents(obj);
      if (ValidateLength(contents.length(), min_length)) return NULL;
      return *contents;
    }
    case GDT_UInt16: {
      Nan::TypedArrayContents<GUInt16> contents(obj);
      if (ValidateLength(contents.length(), min_length)) return NULL;
      return *contents;
    }
    case GDT_Int32: {
      Nan::TypedArrayContents<GInt32> contents(obj);
      if (ValidateLength(contents.length(), min_length)) return NULL;
      return *contents;
    }
    case GDT_UInt32: {
      Nan::TypedArrayContents<GUInt32> contents(obj);
      if (ValidateLength(contents.length(), min_length)) return NULL;
      return *contents;
    }
#if GDAL_VERSION_MAJOR > 3 || (GDAL_VERSION_MAJOR == 3 && GDAL_VERSION_MINOR >= 5)
    case GDT_Int64: {
      Nan::TypedArrayContents<GInt64> contents(obj);
      if (ValidateLength(contents.length(), min_length)) return NULL;
      return *contents;
    }
    case GDT_UInt64: {
      Nan::TypedArrayContents<GUInt64> contents(obj);
      if (ValidateLength(contents.length(), min_length)) return NULL;
      return *contents;
    }
#endif
#if GDAL_VERSION_MAJOR > 3 || (GDAL_VERSION_MAJOR == 3 && GDAL_VERSION_MINOR >= 11)
    case GDT_Float16: {
      Nan::TypedArrayContents<GFloat16> contents(obj);
      if (ValidateLength(contents.length(), min_length)) return NULL;
      return *contents;
    }
#endif
    case GDT_Float32: {
      Nan::TypedArrayContents<float> contents(obj);
      if (ValidateLength(contents.length(), min_length)) return NULL;
      return *contents;
    }
    case GDT_Float64: {
      Nan::TypedArrayContents<double> contents(obj);
      if (ValidateLength(contents.length(), min_length)) return NULL;
      return *contents;
    }
    default: Napi::Error::New(node_gdal::napi_env(), "Unsupported array type").ThrowAsJavaScriptException(); return NULL;
  }
}
bool TypedArray::ValidateLength(size_t length, int64_t min_length) {
  if (static_cast<int64_t>(length) < min_length) {
    std::ostringstream ss;
    ss << "Array length must be greater than or equal to " << min_length;

    Napi::Error::New(node_gdal::napi_env(), ss.str().c_str()).ThrowAsJavaScriptException();
    return true;
  }
  return false;
}

} // namespace node_gdal
