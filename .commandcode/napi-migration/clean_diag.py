#!/usr/bin/env python3
"""Remove the load-time diagnostics, keep the real code."""

import re

p = "src/node_gdal.cpp"
s = open(p, encoding="utf-8").read()

# wrapper: back to the plain form (the try/catch only made the failure silent)
s = re.sub(r'static Napi::Object GDALInit\(Napi::Env env, Napi::Object target\) \{.*?\n\}\n',
           'static Napi::Object GDALInit(Napi::Env env, Napi::Object target) {\n  return node_gdal::Init(env, target);\n}\n',
           s, count=1, flags=re.S)

# the per-statement markers of the constants block: keep the calls, drop the prints
s = re.sub(r'^ *fprintf\(stderr, "T: [^"]*\\n"\); fflush\(stderr\);\n', '', s, flags=re.M)
s = re.sub(r'^ *fprintf\(stderr, "### [^"]*\\n"\);\n', '', s, flags=re.M)
s = s.replace('  setvbuf(stderr, nullptr, _IONBF, 0);\n', '')
s = s.replace('#include <execinfo.h>\n', '')

open(p, "w", encoding="utf-8").write(s)
print("cleaned; T: left =", s.count('"T: '), " W: left =", s.count('"W: '))
