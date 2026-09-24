#!/usr/bin/env python3
"""Throwaway: show why the factory regex does not match."""
import re

path = "src/collections/colortable.cpp"
src = open(path, encoding="utf-8").read()
BLOCK = re.compile(
    r"[ \t]*Napi::Value ext = Nan::New<External>\((\w+)\);\s*\n"
    r"[ \t]*[^\n]*?Local<[^>]*Object>\s+(\w+)\s*=\s*\n?"
    r"[ \t]*Nan::NewInstance\([^\n]*?(\w+)::constructor\)\)[^\n]*\n"
    r"(?:[ \t]*\.ToLocalChecked\(\);\n)?"
)
print("ext occurrences:", src.count("Napi::Value ext = Nan::New<External>"))
m = BLOCK.search(src)
print("match:", bool(m))
i = src.find("Napi::Value ext")
print("--- actual bytes ---")
print(repr(src[i - 140 : i + 300]))
