#!/usr/bin/env python3
import os
import re

LOG = os.path.expanduser("~/ws/chk.log")
txt = open(LOG, encoding="utf-8", errors="replace").read()
print("log chars:", len(txt), " non-pointer count:", txt.count("non-pointer type"))
SITE = re.compile(r"^/home/yuan/ws/(\S+?):(\d+):\d+: error: .base operand of .->. has non-pointer type", re.M)
print("matches:", len(SITE.findall(txt)))
for line in txt.split("\n"):
    if "non-pointer type" in line:
        print("RAW:", repr(line[:130]))
        break
