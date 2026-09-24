#!/usr/bin/env python3
"""Print every error site in a compiler log together with its source line."""
import sys

LOG = sys.argv[1]
seen = set()
for line in open(LOG, encoding="utf-8", errors="replace").read().split("\n"):
    if ": error:" not in line:
        continue
    head, msg = line.split(": error:", 1)
    parts = head.split(":")
    if len(parts) < 3:
        continue
    path = ":".join(parts[:-2]).replace("/home/yuan/ws/", "")
    try:
        ln = int(parts[-2])
    except ValueError:
        continue
    if (path, ln) in seen:
        continue
    seen.add((path, ln))
    try:
        src = open(path, encoding="utf-8", errors="replace").read().split("\n")[ln - 1]
    except OSError:
        src = "<unreadable>"
    print("%s:%d\n    %s\n    -> %s" % (path, ln, src.strip()[:110], msg.strip()[:80]))
