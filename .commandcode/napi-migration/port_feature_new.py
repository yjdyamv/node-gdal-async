#!/usr/bin/env python3
"""Port Feature::New, then report any other declared-but-undefined New factory."""

import glob
import os
import re

p = "src/gdal_feature.cpp"
s = open(p, encoding="utf-8").read()

anchor = """NAN_METHOD(Feature::toString) {
  return Napi::String::New(node_gdal::napi_env(), "Feature");
}"""
factory = """Napi::Value Feature::New(OGRFeature *feature) {
  return Feature::New(feature, true);
}

// Features are not tracked in the object store: the same OGRFeature can be
// handed to JS more than once
Napi::Value Feature::New(OGRFeature *feature, bool owned) {
  if (!feature) { return node_gdal::napi_env().Null(); }

  std::vector<napi_value> args = {Napi::External<void>::New(node_gdal::napi_env(), feature)};
  Napi::Object obj = Feature::constructor.Value().New(args);
  node_gdal::UnwrapWrapped<Feature>(obj)->owned_ = owned;
  return obj;
}

""" + anchor

if "Napi::Value Feature::New" not in s and anchor in s:
    s = s.replace(anchor, factory, 1)
    open(p, "w", encoding="utf-8").write(s)
    print("Feature::New ported")

# ---- any other class with a declared but undefined New? ----
missing = []
for hpp in glob.glob("src/*.hpp"):
    cpp = hpp[:-4] + ".cpp"
    if not os.path.exists(cpp):
        continue
    if not os.path.exists(cpp):
        continue
    body = open(cpp, encoding="utf-8").read()
    for m in re.finditer(r"static\s+Napi::Value\s+New\(([^)]*)\)\s*;", open(hpp, encoding="utf-8").read()):
        args = m.group(1)
        cls = os.path.basename(hpp)[:-4]
        # a definition looks like: Napi::Value <Something>::New(<args>) {
        count = len(re.findall(r"::\s*New\(", body))
        if count == 0:
            missing.append((hpp, cls, args))

print("files with no ::New definition at all:", missing)
