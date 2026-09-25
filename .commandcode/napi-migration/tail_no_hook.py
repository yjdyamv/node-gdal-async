#!/usr/bin/env python3
"""Temporary: restore the CPLE constants, drop the cleanup hook."""

p = "src/node_gdal.cpp"
s = open(p, encoding="utf-8").read()

old = """  napi_add_env_cleanup_hook(env, Cleanup, nullptr);
}
}"""
new = """  NODE_DEFINE_CONSTANT(target, CPLE_OpenFailed);
  NODE_DEFINE_CONSTANT(target, CPLE_IllegalArg);
  NODE_DEFINE_CONSTANT(target, CPLE_NotSupported);
  NODE_DEFINE_CONSTANT(target, CPLE_AssertionFailed);
  NODE_DEFINE_CONSTANT(target, CPLE_NoWriteAccess);
  NODE_DEFINE_CONSTANT(target, CPLE_UserInterrupt);
}
}"""

if old in s:
    s = s.replace(old, new, 1)
    open(p, "w", encoding="utf-8").write(s)
    print("hook removed, CPLE restored")
else:
    print("anchor not found")
