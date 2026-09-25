#!/usr/bin/env python3
"""Temporary: is it the Number or the Set that throws?"""

p = "src/node_gdal.cpp"
s = open(p, encoding="utf-8").read()

old = """  NODE_DEFINE_CONSTANT(target, CPLE_OpenFailed);
  NODE_DEFINE_CONSTANT(target, CPLE_IllegalArg);"""

new = """  {
    fprintf(stderr, "### before Number\\n");
    fflush(stderr);
    Napi::Number n = Napi::Number::New(env, 4);
    fprintf(stderr, "### before Set\\n");
    fflush(stderr);
    bool ok = target.Set("CPLE_OpenFailed", n);
    fprintf(stderr, "### after Set ok=%d\\n", (int)ok);
    fflush(stderr);
  }
  NODE_DEFINE_CONSTANT(target, CPLE_IllegalArg);"""

if old in s:
    s = s.replace(old, new, 1)
    open(p, "w", encoding="utf-8").write(s)
    print("probe2 inserted")
else:
    print("anchor not found")
