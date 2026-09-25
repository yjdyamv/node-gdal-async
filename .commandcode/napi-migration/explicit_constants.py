#!/usr/bin/env python3
"""Step: explicit Set() for the CPLE constants, one marker each, plus the hook."""

p = "src/node_gdal.cpp"
s = open(p, encoding="utf-8").read()

old = """  NODE_DEFINE_CONSTANT(target, CPLE_OpenFailed);
  NODE_DEFINE_CONSTANT(target, CPLE_IllegalArg);
  NODE_DEFINE_CONSTANT(target, CPLE_NotSupported);
  NODE_DEFINE_CONSTANT(target, CPLE_AssertionFailed);
  NODE_DEFINE_CONSTANT(target, CPLE_NoWriteAccess);
  NODE_DEFINE_CONSTANT(target, CPLE_UserInterrupt);

  napi_add_env_cleanup_hook(env, Cleanup, nullptr);"""

names = [
    "CPLE_OpenFailed",
    "CPLE_IllegalArg",
    "CPLE_NotSupported",
    "CPLE_AssertionFailed",
    "CPLE_NoWriteAccess",
    "CPLE_UserInterrupt",
]

lines = []
for name in names:
    lines.append('  fprintf(stderr, "T: %s ++\\n"); fflush(stderr);' % name)
    lines.append('  target.Set(Napi::String::New(env, "%s"), Napi::Number::New(env, %s));' % (name, name))
lines.append('  fprintf(stderr, "T: hook ++\\n"); fflush(stderr);')
lines.append('  napi_add_env_cleanup_hook(env, Cleanup, nullptr);')
lines.append('  fprintf(stderr, "T: done\\n"); fflush(stderr);')
new = "\n".join(lines)

if old in s:
    s = s.replace(old, new, 1)
    open(p, "w", encoding="utf-8").write(s)
    print("tail replaced with explicit form + markers")
else:
    print("anchor not found")
