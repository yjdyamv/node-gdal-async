#!/usr/bin/env python3
"""The last two geometry fixes: the JSON bridge and the second GetConstructorName."""

path = "src/geometry/gdal_geometry.cpp"
src = open(path, encoding="utf-8").read()

old_json = """  Napi::JSON NanJSON;
  Nan::Napi::String result = NanJSON.Stringify(geo_obj);
  if (result.IsEmpty()) {
    Napi::Error::New(node_gdal::napi_env(), "Invalid GeoJSON").ThrowAsJavaScriptException();
    return node_gdal::napi_env().Undefined();
  }
  Napi::String stringified = result;
  std::string *val = new std::string(stringified.As<Napi::String>().Utf8Value());"""

new_json = """  // node-addon-api has no JSON class: reach the global JSON.stringify
  Napi::Value json_stringified = node_gdal::napi_env()
                                   .Global()
                                   .Get("JSON")
                                   .As<Napi::Object>()
                                   .Get("stringify")
                                   .As<Napi::Function>()
                                   .Call({geo_obj});
  if (json_stringified.IsEmpty() || !json_stringified.IsString()) {
    Napi::Error::New(node_gdal::napi_env(), "Invalid GeoJSON").ThrowAsJavaScriptException();
    return node_gdal::napi_env().Undefined();
  }
  std::string *val = new std::string(json_stringified.As<Napi::String>().Utf8Value());"""

if old_json in src:
    src = src.replace(old_json, new_json)
    print("json bridge replaced")

old_check = """  std::string obj_type = geojson_obj.GetConstructorName(.As<Napi::String>().Utf8Value());

  if (obj_type != "Buffer" && obj_type != "Uint8Array") {"""
new_check = """  // N-API has no GetConstructorName
  if (!geojson_obj.IsBuffer() && !geojson_obj.IsTypedArray()) {"""
if old_check in src:
    src = src.replace(old_check, new_check)
    print("constructor name check replaced")

open(path, "w", encoding="utf-8").write(src)
