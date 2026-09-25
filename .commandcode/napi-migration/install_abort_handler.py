#!/usr/bin/env python3
"""Temporary: SIGABRT handler that prints a backtrace where glibc caught the corruption."""

p = "src/node_gdal.cpp"
s = open(p, encoding="utf-8").read()

s = s.replace('#include "gdal_common.hpp"', '#include <execinfo.h>\n#include <signal.h>\n#include "gdal_common.hpp"', 1)

handler = '''
static void gdal_abort_handler(int) {
  void *frames[64];
  int n = backtrace(frames, 64);
  const char *msg = "### SIGABRT backtrace\\n";
  ssize_t ignored = write(2, msg, strlen(msg));
  (void)ignored;
  backtrace_symbols_fd(frames, n, 2);
  _exit(134);
}
'''

anchor = "Napi::Object Init(Napi::Env env, Napi::Object target) {"
if "gdal_abort_handler" not in s:
    s = s.replace(anchor, handler + "\n" + anchor, 1)
    s = s.replace("  g_exports = new Napi::ObjectReference(Napi::Persistent(target));\n", "")
    # install once, right after the ambient env is set
    s = s.replace("  napi_env_storage = env;\n",
                  "  napi_env_storage = env;\n  signal(SIGABRT, gdal_abort_handler);\n", 1)

open(p, "w", encoding="utf-8").write(s)
print("handler installed")
