#!/usr/bin/env python3
"""Give every NAN_METHOD that used to be void an explicit return.

In NAN a NAN_METHOD returned void, so a body that ended with a plain statement
was fine. These are Napi::Value functions now: falling off the end hands a
garbage handle back to node-addon-api. The most visible one is Init itself.
"""

RET = "  return node_gdal::napi_env().Undefined();"

edits = [
    ("src/collections/colortable.cpp",
     "  CPLErrorReset();\n  raw->SetColorEntry(index, &color);\n}",
     "  CPLErrorReset();\n  raw->SetColorEntry(index, &color);\n" + RET + "\n}"),
    ("src/collections/colortable.cpp",
     "  Napi::Value parentMaybe = GDAL_GET_PRIVATE(info.This(), \"parent_\");\n"
     "  if (!parentMaybe.IsEmpty() && !parentMaybe.IsNull() || parentMaybe.IsUndefined()) {\n"
     "    return parentMaybe;\n  }\n}",
     "  Napi::Value parentMaybe = GDAL_GET_PRIVATE(info.This(), \"parent_\");\n"
     "  if (!parentMaybe.IsEmpty() && !parentMaybe.IsNull() || parentMaybe.IsUndefined()) {\n"
     "    return parentMaybe;\n  }\n" + RET + "\n}"),
    ("src/collections/compound_curves.cpp",
     "  if (i >= 0 && i < geom->get()->getNumCurves())\n"
     "    return Geometry::New(geom->get()->getCurve(i), false);\n"
     "  else\n"
     "    Napi::RangeError::New(node_gdal::napi_env(), \"Invalid curve requested\").ThrowAsJavaScriptException();\n}",
     "  if (i >= 0 && i < geom->get()->getNumCurves())\n"
     "    return Geometry::New(geom->get()->getCurve(i), false);\n"
     "  Napi::RangeError::New(node_gdal::napi_env(), \"Invalid curve requested\").ThrowAsJavaScriptException();\n"
     + RET + "\n}"),
    ("src/collections/feature_fields.cpp",
     "  } catch (const char *err) { Napi::Error::New(node_gdal::napi_env(), err).ThrowAsJavaScriptException(); }\n}",
     "  } catch (const char *err) { Napi::Error::New(node_gdal::napi_env(), err).ThrowAsJavaScriptException(); }\n"
     + RET + "\n}"),
    ("src/gdal_algorithms.cpp",
     "  Napi::Error::New(node_gdal::napi_env(), \"Custom pixel functions require GDAL >= 3.5\").ThrowAsJavaScriptException();\n#endif\n}",
     "  Napi::Error::New(node_gdal::napi_env(), \"Custom pixel functions require GDAL >= 3.5\").ThrowAsJavaScriptException();\n#endif\n"
     + RET + "\n}"),
    ("src/gdal_feature.cpp",
     "  std::string utf8 = info[0].As<Napi::String>().Utf8Value();\n  feature->this_->SetStyleString(utf8.c_str());\n}",
     "  std::string utf8 = info[0].As<Napi::String>().Utf8Value();\n  feature->this_->SetStyleString(utf8.c_str());\n"
     + RET + "\n}"),
    ("src/gdal_memfile.cpp",
     "  if (memfile == nullptr) Napi::Error::New(node_gdal::napi_env(), \"Failed creating in-memory file\").ThrowAsJavaScriptException();\n}",
     "  if (memfile == nullptr) Napi::Error::New(node_gdal::napi_env(), \"Failed creating in-memory file\").ThrowAsJavaScriptException();\n"
     + RET + "\n}"),
    ("src/gdal_memfile.cpp",
     "  if (!Memfile::copy(buffer, filename)) Napi::Error::New(node_gdal::napi_env(), \"Failed creating in-memory file\").ThrowAsJavaScriptException();\n}",
     "  if (!Memfile::copy(buffer, filename)) Napi::Error::New(node_gdal::napi_env(), \"Failed creating in-memory file\").ThrowAsJavaScriptException();\n"
     + RET + "\n}"),
    ("src/gdal_memfile.cpp",
     "  Memfile *memfile = Memfile::get(buffer);\n"
     "  if (memfile == nullptr)\n"
     "    Napi::Error::New(node_gdal::napi_env(), \"Failed creating in-memory file\").ThrowAsJavaScriptException();\n"
     "  else\n"
     "    return Napi::String::New(node_gdal::napi_env(), memfile->filename);",
     "  Memfile *memfile = Memfile::get(buffer);\n"
     "  if (memfile == nullptr)\n"
     "    Napi::Error::New(node_gdal::napi_env(), \"Failed creating in-memory file\").ThrowAsJavaScriptException();\n"
     "  else\n"
     "    return Napi::String::New(node_gdal::napi_env(), memfile->filename);\n"
     + RET),
    ("src/node_gdal.cpp",
     "  napi_add_env_cleanup_hook(env, Cleanup, nullptr);\n\n}\n}",
     "  napi_add_env_cleanup_hook(env, Cleanup, nullptr);\n\n  return target;\n}\n}"),
]

for path, old, new in edits:
    s = open(path, encoding="utf-8").read()
    if old in s:
        open(path, "w", encoding="utf-8").write(s.replace(old, new, 1))
        print("patched", path)
    else:
        print("ANCHOR MISSING in", path, "->", old.split("\n")[-2][:60])
