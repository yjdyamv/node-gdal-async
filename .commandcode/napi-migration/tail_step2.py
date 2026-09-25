#!/usr/bin/env python3
"""Temporary: keep only the first CPLE constant enabled."""

p = "src/node_gdal.cpp"
s = open(p, encoding="utf-8").read()

old = """  TRY_STMT("CPLE_IllegalArg", NODE_DEFINE_CONSTANT(target, CPLE_IllegalArg);)"""
if old in s:
    s = s.replace(old, "#if 0\n" + old, 1)
    s = s.replace('  TRY_STMT("CPLE_UserInterrupt", NODE_DEFINE_CONSTANT(target, CPLE_UserInterrupt);)',
                  '  TRY_STMT("CPLE_UserInterrupt", NODE_DEFINE_CONSTANT(target, CPLE_UserInterrupt);)\n#endif', 1)
    open(p, "w", encoding="utf-8").write(s)
    print("only CPLE_OpenFailed enabled")
else:
    print("no change")
