#!/usr/bin/env python3
"""Temporary: allocation-free markers through addPixelFunc."""

p = "src/gdal_algorithms.cpp"
s = open(p, encoding="utf-8").read()

if "MK(" not in s:
    s = s.replace(
        "NAN_METHOD(Algorithms::addPixelFunc) {",
        '#define MK(t) do { const char *m_ = t; ssize_t ig_ = write(2, m_, strlen(m_)); (void)ig_; } while (0)\n'
        "NAN_METHOD(Algorithms::addPixelFunc) {\n  MK(\"M0 \")\n",
        1)
    s = s.replace('  NODE_ARG_STR(0, "name", name);', '  NODE_ARG_STR(0, "name", name);\n  MK("M1 ")', 1)
    s = s.replace('  NODE_ARG_OBJECT(1, "pixelFn", arg);', '  NODE_ARG_OBJECT(1, "pixelFn", arg);\n  MK("M2 ")', 1)
    s = s.replace("  Napi::TypedArrayOf<uint64_t> magic = arg.As<Napi::TypedArrayOf<uint64_t>>();",
                  '  Napi::TypedArrayOf<uint64_t> magic = arg.As<Napi::TypedArrayOf<uint64_t>>();\n  MK("M3 ")', 1)
    s = s.replace("  if (magic.ElementLength() < 1 || *magic.Data() != NODE_GDAL_CAPI_MAGIC) {",
                  '  MK("M4 ")\n  if (magic.ElementLength() < 1 || *magic.Data() != NODE_GDAL_CAPI_MAGIC) {', 1)
    s = s.replace("  Napi::TypedArrayOf<uint8_t> data = arg.As<Napi::TypedArrayOf<uint8_t>>();",
                  '  MK("M5 ")\n  Napi::TypedArrayOf<uint8_t> data = arg.As<Napi::TypedArrayOf<uint8_t>>();\n  MK("M6 ")', 1)
    s = s.replace("  CPLErr err = GDALAddDerivedBandPixelFuncWithArgs(name.c_str(), desc->fn, desc->metadata);",
                  '  MK("M7 ")\n  CPLErr err = GDALAddDerivedBandPixelFuncWithArgs(name.c_str(), desc->fn, desc->metadata);\n  MK("M8 ")', 1)
    s = s.replace("#include <climits>", "#include <unistd.h>\n#include <climits>", 1)
    open(p, "w", encoding="utf-8").write(s)
    print("markers added")
else:
    print("already instrumented")
