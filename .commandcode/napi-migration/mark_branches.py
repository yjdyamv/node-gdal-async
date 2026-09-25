#!/usr/bin/env python3
"""Temporary: disambiguate the error branch from the normal return."""

p = "src/gdal_algorithms.cpp"
s = open(p, encoding="utf-8").read()

old = """  if (err != CE_None) {
    fprintf(stderr, "### addPixelFunc: gdal err=%d msg=[%s]\\n### metadata=[%s]\\n", (int)err,
            CPLGetLastErrorMsg() ? CPLGetLastErrorMsg() : "(null)", desc->metadata);
    fflush(stderr);
    NODE_THROW_LAST_CPLERR;
  }"""

new = """  if (err != CE_None) {
    MK("E1 ")
    const char *m_ = CPLGetLastErrorMsg();
    if (m_ != nullptr) {
      ssize_t ig_ = write(2, m_, strlen(m_));
      (void)ig_;
    }
    MK("E2 ")
    NODE_THROW_LAST_CPLERR;
    MK("E3 ")
  } else {
    MK("OK ")
  }
  MK("M9 ")"""

if old in s:
    s = s.replace(old, new, 1)
    open(p, "w", encoding="utf-8").write(s)
    print("branch markers added")
else:
    print("anchor not found")
