#!/usr/bin/env python3
"""Count the dot-reporter results captured before the abort."""

s = open("/tmp/full2.log", encoding="utf-8", errors="ignore").read()
body = s[s.find("  "):]
print("passing(dots):", body.count("."), " failing(!):", body.count("!"))
