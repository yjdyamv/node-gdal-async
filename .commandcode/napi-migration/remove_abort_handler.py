#!/usr/bin/env python3
"""Remove the temporary SIGABRT backtrace handler."""

import re

p = "src/node_gdal.cpp"
s = open(p, encoding="utf-8").read()

s = re.sub(r"\nstatic void gdal_abort_handler\(int\) \{.*?\n\}\n", "\n", s, flags=re.S)
s = s.replace("  signal(SIGABRT, gdal_abort_handler);\n", "")
s = s.replace("#include <execinfo.h>\n#include <signal.h>\n", "")

open(p, "w", encoding="utf-8").write(s)
print("handler removed; leftovers:", s.count("gdal_abort_handler"), s.count("execinfo"))
