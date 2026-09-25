#!/usr/bin/env python3
"""Port the private-collection setup the constructors lost."""

n = 0

edits = [
    (
        "src/gdal_rasterband.cpp",
        """  if (info.Length() > 0 && info[0].IsExternal()) {
    this_ = static_cast<GDALRasterBand *>(info[0].As<Napi::External<void>>().Data());
    LOG("Created band [%p] (dataset = %p)", this_, this_->GetDataset());
    return;
  }
  Napi::Error::New(info.Env(), "Cannot create RasterBand directly").ThrowAsJavaScriptException();
}""",
        """  if (info.Length() > 0 && info[0].IsExternal()) {
    this_ = static_cast<GDALRasterBand *>(info[0].As<Napi::External<void>>().Data());
    LOG("Created band [%p] (dataset = %p)", this_, this_->GetDataset());
  } else {
    Napi::Error::New(info.Env(), "Cannot create RasterBand directly").ThrowAsJavaScriptException();
    return;
  }

  GDAL_SET_PRIVATE(info.This(), "overviews_", RasterBandOverviews::New(info.This()));
  GDAL_SET_PRIVATE(info.This(), "pixels_", RasterBandPixels::New(info.This()));
}""",
    ),
    (
        "src/gdal_layer.cpp",
        """  if (info.Length() > 0 && info[0].IsExternal()) {
    this_ = static_cast<OGRLayer *>(info[0].As<Napi::External<void>>().Data());
    LOG("Created layer [%p]", this_);
    return;
  }
  Napi::Error::New(info.Env(), "Cannot create Layer directly").ThrowAsJavaScriptException();
}""",
        """  if (info.Length() > 0 && info[0].IsExternal()) {
    this_ = static_cast<OGRLayer *>(info[0].As<Napi::External<void>>().Data());
    LOG("Created layer [%p]", this_);
  } else {
    Napi::Error::New(info.Env(), "Cannot create Layer directly").ThrowAsJavaScriptException();
    return;
  }

  GDAL_SET_PRIVATE(info.This(), "features_", LayerFeatures::New(info.This()));
  GDAL_SET_PRIVATE(info.This(), "fields_", LayerFields::New(info.This()));
}""",
    ),
    (
        "src/gdal_group.cpp",
        """  if (info.Length() > 0 && info[0].IsExternal()) {
    this_ = node_gdal::ImportShared<GDALGroup>(info);
    LOG("Created group [%p]", this_.get());
    return;
  }
  Napi::Error::New(info.Env(), "Cannot create Group directly").ThrowAsJavaScriptException();
}""",
        """  if (info.Length() > 0 && info[0].IsExternal()) {
    this_ = node_gdal::ImportShared<GDALGroup>(info);
    LOG("Created group [%p]", this_.get());
  } else {
    Napi::Error::New(info.Env(), "Cannot create Group directly").ThrowAsJavaScriptException();
    return;
  }

  // the parent dataset travels as the second argument, for the sub-collections
  Napi::Value parent_ds = info.Length() > 1 ? info[1] : info.Env().Undefined();
  GDAL_SET_PRIVATE(info.This(), "groups_", GroupGroups::New(info.This(), parent_ds));
  GDAL_SET_PRIVATE(info.This(), "arrays_", GroupArrays::New(info.This(), parent_ds));
  GDAL_SET_PRIVATE(info.This(), "dims_", GroupDimensions::New(info.This(), parent_ds));
  GDAL_SET_PRIVATE(info.This(), "attrs_", GroupAttributes::New(info.This(), parent_ds));
}""",
    ),
    (
        "src/gdal_mdarray.cpp",
        """  if (info.Length() > 0 && info[0].IsExternal()) {
    this_ = node_gdal::ImportShared<GDALMDArray>(info);
    LOG("Created MDArray [%p]", this_.get());
    return;
  }
  Napi::Error::New(info.Env(), "Cannot create MDArray directly").ThrowAsJavaScriptException();
}""",
        """  if (info.Length() > 0 && info[0].IsExternal()) {
    this_ = node_gdal::ImportShared<GDALMDArray>(info);
    LOG("Created MDArray [%p]", this_.get());
  } else {
    Napi::Error::New(info.Env(), "Cannot create MDArray directly").ThrowAsJavaScriptException();
    return;
  }

  Napi::Value parent_ds = info.Length() > 1 ? info[1] : info.Env().Undefined();
  GDAL_SET_PRIVATE(info.This(), "dims_", ArrayDimensions::New(info.This(), parent_ds));
  GDAL_SET_PRIVATE(info.This(), "attrs_", ArrayAttributes::New(info.This(), parent_ds));
}""",
    ),
]

for path, old, new in edits:
    s = open(path, encoding="utf-8").read()
    if old in s:
        open(path, "w", encoding="utf-8").write(s.replace(old, new, 1))
        n += 1
        print("patched", path)
    else:
        print("ANCHOR MISSING in", path)

print("total", n)
