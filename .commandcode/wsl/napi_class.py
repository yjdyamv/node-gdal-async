#!/usr/bin/env python3
"""Convert a class' NAN Initialize() into the node-addon-api DefineClass form.

Turns

    void X::Initialize(Local<Object> target) {
      Nan::HandleScope scope;
      Local<FunctionTemplate> lcons = Nan::New<FunctionTemplate>(X::New);
      lcons->InstanceTemplate()->SetInternalFieldCount(1);
      lcons->SetClassName(Nan::New("X").ToLocalChecked());
      lcons->Inherit(Nan::New(Base::constructor));
      Nan::SetPrototypeMethod(lcons, "toString", toString);
      Nan__SetPrototypeAsyncableMethod(lcons, "open", open);
      ATTR(lcons, "description", descriptionGetter, READ_ONLY_SETTER);
      Nan::Set(target, Nan::New("X").ToLocalChecked(), Nan::GetFunction(lcons).ToLocalChecked());
      constructor.Reset(lcons);
    }

into

    void X::Initialize(Napi::Object target) {
      Napi::Env env = target.Env();
      SELF_CLASS(X);
      Napi::Function lcons = DefineClass(env, "X",
        {
          METHOD(toString)
          METHOD_ASYNCABLE(open)
          ATTR(lcons, "description", descriptionGetter, READ_ONLY_SETTER)
        });
      // lcons->Inherit() has no DefineClass equivalent
      Napi::Function base = Base::constructor.Value();
      lcons.Get("prototype").As<Napi::Object>().SetPrototypeOf(base.Get("prototype").As<Napi::Object>());
      lcons.SetPrototypeOf(base);
      target.Set("X", lcons);
      constructor = Napi::Persistent(lcons);
      constructor.SuppressDestruct();
    }

Anything it does not recognise is reported and kept, so it can be reviewed.

Usage: napi_class.py file.cpp [...]
"""
import re
import sys

SCAFFOLD = (
    "Nan::HandleScope scope;",
)

REG_NAME = r'"([^"]+)"\s*,\s*([A-Za-z_][A-Za-z_0-9:]*)'


def entry(line):
    s = line.strip()
    m = re.match(r"Nan::SetPrototypeMethod\(\s*lcons\s*,\s*%s\s*\)\s*;" % REG_NAME, s)
    if m:
        name, meth = m.group(1), m.group(2)
        return 'METHOD(%s)' % meth if name == meth else 'METHOD_AS("%s", %s)' % (name, meth)
    m = re.match(r"Nan__SetPrototypeAsyncableMethod\(\s*lcons\s*,\s*%s\s*\)\s*;" % REG_NAME, s)
    if m:
        name, meth = m.group(1), m.group(2)
        return 'METHOD_ASYNCABLE(%s)' % meth if name == meth else 'METHOD_ASYNCABLE_AS("%s", %s)' % (name, meth)
    if s.startswith(("ATTR(", "ATTR_ASYNCABLE(", "ATTR_DONT_ENUM(")) and s.endswith(";"):
        return s[:-1]
    return None


def find_block(lines, start):
    """Return (open_idx, close_idx) of the brace block starting at/after `start`."""
    depth = 0
    opened = False
    for i in range(start, len(lines)):
        for ch in lines[i]:
            if ch == "{":
                depth += 1
                opened = True
            elif ch == "}":
                depth -= 1
                if opened and depth == 0:
                    return i
    raise SystemExit("unbalanced braces")


for path in sys.argv[1:]:
    with open(path, encoding="utf-8") as f:
        src = f.read()
    lines = src.split("\n")

    idx = next((i for i, l in enumerate(lines) if re.match(r"^void\s+(\w+)::Initialize\(", l)), None)

    # Static members registered on the class itself. Done as a pre-pass so that
    # it also fixes up files whose Initialize is already converted. `env` is in
    # scope inside Initialize.
    src = re.sub(r"Nan::SetMethod\(lcons,\s*", "GDAL_SetMethod(env, lcons, ", src)
    src = re.sub(r"Nan__SetAsyncableMethod\(lcons,\s*", "GDAL_SetAsyncableMethod(env, lcons, ", src)
    # the static constructor reference definition
    src = re.sub(
        r"^Nan::Persistent<FunctionTemplate> (\w+)::constructor;$",
        r"Napi::FunctionReference \1::constructor;",
        src,
        flags=re.M,
    )
    lines = src.split("\n")

    if idx is None:
        print("skip   %s (no Initialize)" % path)
        continue
    cls = re.match(r"^void\s+(\w+)::Initialize\(", lines[idx]).group(1)

    close = find_block(lines, idx)
    body = lines[idx + 1 : close]
    saw_lcons = any("lcons" in l for l in body)
    if any("DefineClass" in l for l in body):
        # still save the pre-pass (static registrations, constructor definition)
        with open(path, "w", encoding="utf-8") as f:
            f.write(src)
        print("done   %s (Initialize already converted)" % path)
        continue

    entries, extra, base, export_name = [], [], None, cls
    for l in body:
        s = l.strip()
        if not s or s in SCAFFOLD:
            continue
        if re.match(r"Local<FunctionTemplate>\s+lcons\s*=", s) or "InstanceTemplate()->SetInternalFieldCount" in s:
            continue
        if "SetClassName(" in s:
            continue
        m = re.match(r"lcons->Inherit\(\s*Nan::New\((\w+)::constructor\)", s)
        if m:
            base = m.group(1)
            continue
        m = re.match(r"Nan::Set\(\s*target\s*,\s*Nan::New\(\"([^\"]+)\"\)", s)
        if m:
            export_name = m.group(1)
            continue
        if "constructor.Reset(lcons)" in s:
            continue
        if s.startswith("#"):
            entries.append(l)
            continue
        e = entry(s)
        if e:
            entries.append("        " + e)
            continue
        # static members registered on the class itself
        m = re.match(r"Nan::SetMethod\(\s*(\w+)\s*,\s*%s\s*\)\s*;" % REG_NAME, s)
        if m:
            extra.append('  GDAL_SetMethod(env, %s, "%s", %s);' % (m.group(1), m.group(2), m.group(3)))
            continue
        m = re.match(r"Nan__SetAsyncableMethod\(\s*(\w+)\s*,\s*%s\s*\)\s*;" % REG_NAME, s)
        if m:
            extra.append('  GDAL_SetAsyncableMethod(env, %s, "%s", %s);' % (m.group(1), m.group(2), m.group(3)))
            continue
        extra.append(l)

    if not entries and not saw_lcons:
        # Module-level only (Utils, VSI, Algorithms, Warper): no class is created
        out = ["void %s::Initialize(Napi::Object target) {" % cls, "  Napi::Env env = target.Env();", ""]
        out += extra
        out += ["}"]
        lines[idx : close + 1] = out
        with open(path, "w", encoding="utf-8") as f:
            f.write("\n".join(lines))
        print("ok     %s (%s, module-level only)" % (path, cls))
        continue

    if not entries:
        print("warn   %s (no registrations found)" % path)

    out = ["void %s::Initialize(Napi::Object target) {" % cls, "  Napi::Env env = target.Env();", "  SELF_CLASS(%s);" % cls, ""]
    out += ["  // NOTE: the descriptor macros carry their own trailing comma",
            '  Napi::Function lcons = DefineClass(env, "%s",' % export_name, "    {"]
    out += entries
    out += ["    });"]
    if base:
        out += ["",
                "  // lcons->Inherit() has no DefineClass equivalent",
                "  node_gdal::Inherit(lcons, %s::constructor.Value());" % base]
    if extra:
        out += [""] + extra
    out += ["", '  target.Set("%s", lcons);' % export_name, "",
            "  constructor = Napi::Persistent(lcons);",
            "  constructor.SuppressDestruct();", "}"]
    for l in extra:
        print("extra  %s: %s" % (path, l.strip()))

    lines[idx : close + 1] = out
    with open(path, "w", encoding="utf-8") as f:
        f.write("\n".join(lines))
    print("ok     %s (%s, %d entries%s)" % (path, cls, len(entries), ", inherits %s" % base if base else ""))
