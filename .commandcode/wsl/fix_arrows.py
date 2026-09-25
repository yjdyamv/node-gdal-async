#!/usr/bin/env python3
"""Fix `->` on node-addon-api handles, using the plain (file, line) sites from a
compiler log. No regex parsing of the log - just string splitting.

A line flagged with "base operand of '->' has non-pointer type" has at least one
arrow whose operand is a Napi value. Single-arrow lines are rewritten
unconditionally; lines with several arrows are only touched for the arrows whose
operand was declared as a Napi type.

Usage: fix_arrows.py <compiler log>
"""
import re
import sys

LOG = sys.argv[1]

sites = {}
for line in open(LOG, encoding="utf-8", errors="replace").read().split("\n"):
    if "non-pointer type" not in line or "error:" not in line:
        continue
    head = line.split(": error:")[0]
    parts = head.split(":")
    if len(parts) < 3:
        continue
    try:
        ln = int(parts[-2])
    except ValueError:
        continue
    path = ":".join(parts[:-2]).replace("/home/yuan/ws/", "")
    sites.setdefault(path, set()).add(ln)

print("sites: %d lines in %d files" % (sum(len(v) for v in sites.values()), len(sites)))

NAPI_T = r"Napi::(?:Value|Object|Array|Function|String|Number|Boolean|BigInt|Symbol|Date|Promise|Buffer|External<[^>]*>)"

for path, line_numbers in sorted(sites.items()):
    with open(path, encoding="utf-8") as f:
        lines = f.read().split("\n")
    napi_vars = set(re.findall(r"%s\s*[\*&]*\s*(\w+)" % NAPI_T, "\n".join(lines)))
    if "NAN_SETTER(" in "\n".join(lines):
        # NAN_SETTER expands to (const CallbackInfo&, const Napi::Value& value)
        napi_vars.add("value")
    changed = 0
    for ln in sorted(line_numbers):
        text = lines[ln - 1]
        arrows = text.count("->")
        if arrows == 1:
            lines[ln - 1] = text.replace("->", ".")
            changed += 1
            continue

        def repl(m):
            global changed
            if m.group(1) in napi_vars:
                changed += 1
                return m.group(1) + "."
            return m.group(0)

        lines[ln - 1] = re.sub(r"(\w+)->", repl, text)
        if lines[ln - 1] == text:
            print("MANUAL %s:%d  %s" % (path, ln, text.strip()[:100]))
    if changed:
        with open(path, "w", encoding="utf-8") as f:
            f.write("\n".join(lines))
    print("%-46s %d arrows" % (path, changed))
