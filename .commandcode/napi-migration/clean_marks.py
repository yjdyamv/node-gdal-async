#!/usr/bin/env python3
"""Remove the temporary Init diagnostics, restoring a clean tail."""

import re

p = "src/node_gdal.cpp"
s = open(p, encoding="utf-8").read()

# 1. the TRY_STMT machinery + the disabled blocks
s = re.sub(r'#define TRY_STMT.*?\n\n', '', s, flags=re.S)
s = re.sub(r'  TRY_STMT\("(\w+)", (.*?);\)\n', r'  \2;\n', s)
s = s.replace("#if 0\n", "").replace("#endif\n  fprintf(stderr, \"### C7\\n\");\n", "")

# 2. stray markers
s = re.sub(r'^ *fprintf\(stderr, "### [^"]*\\n"\);\n', '', s, flags=re.M)
s = s.replace('  fflush(stderr);\n', '')

open(p, "w", encoding="utf-8").write(s)
print("cleaned; remaining ### lines:", s.count("###"))
