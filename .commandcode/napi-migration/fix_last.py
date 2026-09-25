#!/usr/bin/env python3
"""Module-level accessors and the last geometry return."""

n = 0

p = "src/node_gdal.cpp"
s = open(p, encoding="utf-8").read()

old = '  target.DefineProperty(Napi::PropertyDescriptor::Accessor("lastError", LastErrorGetter, LastErrorSetter));'
new = '  GDAL_DEFINE_ACCESSOR(target, "lastError", LastErrorGetter, LastErrorSetter);'
if old in s:
    s = s.replace(old, new)
    n += 1

old2 = """  target.DefineProperty(
    Napi::PropertyDescriptor::Accessor("eventLoopWarning", EventLoopWarningGetter, EventLoopWarningSetter));"""
new2 = '  GDAL_DEFINE_ACCESSOR(target, "eventLoopWarning", EventLoopWarningGetter, EventLoopWarningSetter);'
if old2 in s:
    s = s.replace(old2, new2)
    n += 1

open(p, "w", encoding="utf-8").write(s)
print("node_gdal.cpp: %d accessors" % n)

p = "src/geometry/gdal_geometry.cpp"
s = open(p, encoding="utf-8").read()
old3 = """  info.GetReturnValue().Set(
    SpatialReference::New(const_cast<OGRSpatialReference *>(geom->this_->getSpatialReference()), false));"""
new3 = """  return SpatialReference::New(const_cast<OGRSpatialReference *>(geom->this_->getSpatialReference()), false);"""
if old3 in s:
    s = s.replace(old3, new3)
    print("gdal_geometry.cpp: return replaced")
open(p, "w", encoding="utf-8").write(s)
