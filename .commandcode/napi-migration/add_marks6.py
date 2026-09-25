#!/usr/bin/env python3
"""Temporary: markers between the CPLE constants to find the crashing one."""

p = "src/node_gdal.cpp"
s = open(p, encoding="utf-8").read()

pairs = [
    ('  TRY_STMT("CPLE_OpenFailed", NODE_DEFINE_CONSTANT(target, CPLE_OpenFailed);)\n', "### C1"),
    ('  TRY_STMT("CPLE_IllegalArg", NODE_DEFINE_CONSTANT(target, CPLE_IllegalArg);)\n', "### C2"),
    ('  TRY_STMT("CPLE_NotSupported", NODE_DEFINE_CONSTANT(target, CPLE_NotSupported);)\n', "### C3"),
    ('  TRY_STMT("CPLE_AssertionFailed", NODE_DEFINE_CONSTANT(target, CPLE_AssertionFailed);)\n', "### C4"),
    ('  TRY_STMT("CPLE_NoWriteAccess", NODE_DEFINE_CONSTANT(target, CPLE_NoWriteAccess);)\n', "### C5"),
    ('  TRY_STMT("CPLE_UserInterrupt", NODE_DEFINE_CONSTANT(target, CPLE_UserInterrupt);)\n', "### C6"),
    ('  TRY_STMT("cleanup_hook", napi_add_env_cleanup_hook(env, Cleanup, nullptr);)\n', "### C7"),
]

for anchor, tag in pairs:
    if anchor in s and tag not in s:
        s = s.replace(anchor, anchor + '  fprintf(stderr, "%s\\n");\n' % tag, 1)

open(p, "w", encoding="utf-8").write(s)
print("ok")
