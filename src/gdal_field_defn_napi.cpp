#include "gdal_field_defn_napi.hpp"
#include "utils/field_types.hpp"

namespace node_gdal {

Napi::FunctionReference FieldDefnNapi::constructor;

// ---------------------------------------------------------------------------
// Init
// ---------------------------------------------------------------------------
Napi::Object FieldDefnNapi::Init(Napi::Env env, Napi::Object exports) {
  Napi::Function func = DefineClass(
    env,
    "FieldDefnNapi",
    {
      InstanceMethod("toString", &FieldDefnNapi::toString),
      InstanceAccessor<&FieldDefnNapi::nameGetter, &FieldDefnNapi::nameSetter>("name"),
      InstanceAccessor<&FieldDefnNapi::typeGetter, &FieldDefnNapi::typeSetter>("type"),
      InstanceAccessor<&FieldDefnNapi::justificationGetter,
                        &FieldDefnNapi::justificationSetter>("justification"),
      InstanceAccessor<&FieldDefnNapi::widthGetter, &FieldDefnNapi::widthSetter>("width"),
      InstanceAccessor<&FieldDefnNapi::precisionGetter,
                        &FieldDefnNapi::precisionSetter>("precision"),
      InstanceAccessor<&FieldDefnNapi::ignoredGetter, &FieldDefnNapi::ignoredSetter>("ignored"),
    });

  constructor = Napi::Persistent(func);
  constructor.SuppressDestruct();

  exports.Set("FieldDefnNapi", func);
  return exports;
}

// ---------------------------------------------------------------------------
// Constructor
// ---------------------------------------------------------------------------
FieldDefnNapi::FieldDefnNapi(const Napi::CallbackInfo &info)
  : Napi::ObjectWrap<FieldDefnNapi>(info), this_(nullptr), owned_(false) {

  if (!info.IsConstructCall()) {
    Napi::Error::New(info.Env(), "Cannot call constructor as function, use 'new' keyword")
      .ThrowAsJavaScriptException();
    return;
  }

  if (info.Length() > 0 && info[0].IsExternal()) {
    // External(OGRFieldDefn*) from static factory
    this_ = info[0].As<Napi::External<OGRFieldDefn>>().Data();
    owned_ = true;
  } else {
    std::string field_name;
    std::string type_name = "string";

    if (info.Length() < 1 || !info[0].IsString()) {
      Napi::Error::New(info.Env(), "field name must be given as a string")
        .ThrowAsJavaScriptException();
      return;
    }
    field_name = info[0].As<Napi::String>().Utf8Value();

    if (info.Length() > 1 && info[1].IsString()) {
      type_name = info[1].As<Napi::String>().Utf8Value();
    }

    int field_type = getFieldTypeByName(type_name);
    if (field_type < 0) {
      Napi::Error::New(info.Env(), "Unrecognized field type").ThrowAsJavaScriptException();
      return;
    }

    this_ = new OGRFieldDefn(field_name.c_str(), static_cast<OGRFieldType>(field_type));
    owned_ = true;
  }
}

FieldDefnNapi::~FieldDefnNapi() {
  if (this_ && owned_) {
    delete this_;
  }
  this_ = nullptr;
}

// ---------------------------------------------------------------------------
// Static factories
// ---------------------------------------------------------------------------
Napi::Value FieldDefnNapi::New(Napi::Env env, const OGRFieldDefn *def) {
  if (!def) return env.Null();
  OGRFieldDefn *copy = new OGRFieldDefn(def);
  return FieldDefnNapi::New(env, copy, true);
}

Napi::Value FieldDefnNapi::New(Napi::Env env, OGRFieldDefn *def, bool owned) {
  Napi::EscapableHandleScope scope(env);

  if (!def) return scope.Escape(env.Null());
  if (!owned) { def = new OGRFieldDefn(def); }

  Napi::Object obj =
    constructor.New({Napi::External<OGRFieldDefn>::New(env, def)});

  return scope.Escape(obj);
}

// ---------------------------------------------------------------------------
// toString
// ---------------------------------------------------------------------------
Napi::Value FieldDefnNapi::toString(const Napi::CallbackInfo &info) {
  return Napi::String::New(info.Env(), "FieldDefn");
}

// ---------------------------------------------------------------------------
// Getters
// ---------------------------------------------------------------------------
Napi::Value FieldDefnNapi::nameGetter(const Napi::CallbackInfo &info) {
  FieldDefnNapi *def = FieldDefnNapi::Unwrap(info.This().As<Napi::Object>());
  if (!def || !def->isAlive()) {
    Napi::Error::New(info.Env(), "FieldDefnNapi object has already been destroyed")
      .ThrowAsJavaScriptException();
    return info.Env().Undefined();
  }
  return SafeStringNapi(info.Env(), def->this_->GetNameRef());
}

Napi::Value FieldDefnNapi::typeGetter(const Napi::CallbackInfo &info) {
  FieldDefnNapi *def = FieldDefnNapi::Unwrap(info.This().As<Napi::Object>());
  if (!def || !def->isAlive()) {
    Napi::Error::New(info.Env(), "FieldDefnNapi object has already been destroyed")
      .ThrowAsJavaScriptException();
    return info.Env().Undefined();
  }
  return SafeStringNapi(info.Env(), getFieldTypeName(def->this_->GetType()));
}

Napi::Value FieldDefnNapi::justificationGetter(const Napi::CallbackInfo &info) {
  FieldDefnNapi *def = FieldDefnNapi::Unwrap(info.This().As<Napi::Object>());
  if (!def || !def->isAlive()) {
    Napi::Error::New(info.Env(), "FieldDefnNapi object has already been destroyed")
      .ThrowAsJavaScriptException();
    return info.Env().Undefined();
  }
  OGRJustification j = def->this_->GetJustify();
  if (j == OJRight) return Napi::String::New(info.Env(), "Right");
  if (j == OJLeft) return Napi::String::New(info.Env(), "Left");
  return info.Env().Undefined();
}

Napi::Value FieldDefnNapi::widthGetter(const Napi::CallbackInfo &info) {
  FieldDefnNapi *def = FieldDefnNapi::Unwrap(info.This().As<Napi::Object>());
  if (!def || !def->isAlive()) {
    Napi::Error::New(info.Env(), "FieldDefnNapi object has already been destroyed")
      .ThrowAsJavaScriptException();
    return info.Env().Undefined();
  }
  return Napi::Number::New(info.Env(), def->this_->GetWidth());
}

Napi::Value FieldDefnNapi::precisionGetter(const Napi::CallbackInfo &info) {
  FieldDefnNapi *def = FieldDefnNapi::Unwrap(info.This().As<Napi::Object>());
  if (!def || !def->isAlive()) {
    Napi::Error::New(info.Env(), "FieldDefnNapi object has already been destroyed")
      .ThrowAsJavaScriptException();
    return info.Env().Undefined();
  }
  return Napi::Number::New(info.Env(), def->this_->GetPrecision());
}

Napi::Value FieldDefnNapi::ignoredGetter(const Napi::CallbackInfo &info) {
  FieldDefnNapi *def = FieldDefnNapi::Unwrap(info.This().As<Napi::Object>());
  if (!def || !def->isAlive()) {
    Napi::Error::New(info.Env(), "FieldDefnNapi object has already been destroyed")
      .ThrowAsJavaScriptException();
    return info.Env().Undefined();
  }
  return Napi::Boolean::New(info.Env(), def->this_->IsIgnored());
}

// ---------------------------------------------------------------------------
// Setters
// ---------------------------------------------------------------------------
void FieldDefnNapi::nameSetter(const Napi::CallbackInfo &info, const Napi::Value &value) {
  FieldDefnNapi *def = FieldDefnNapi::Unwrap(info.This().As<Napi::Object>());
  if (!def || !def->isAlive()) {
    Napi::Error::New(info.Env(), "FieldDefnNapi object has already been destroyed")
      .ThrowAsJavaScriptException();
    return;
  }
  if (!value.IsString()) {
    Napi::Error::New(info.Env(), "Name must be string").ThrowAsJavaScriptException();
    return;
  }
  std::string name = value.As<Napi::String>().Utf8Value();
  def->this_->SetName(name.c_str());
}

void FieldDefnNapi::typeSetter(const Napi::CallbackInfo &info, const Napi::Value &value) {
  FieldDefnNapi *def = FieldDefnNapi::Unwrap(info.This().As<Napi::Object>());
  if (!def || !def->isAlive()) {
    Napi::Error::New(info.Env(), "FieldDefnNapi object has already been destroyed")
      .ThrowAsJavaScriptException();
    return;
  }
  if (!value.IsString()) {
    Napi::Error::New(info.Env(), "type must be a string").ThrowAsJavaScriptException();
    return;
  }
  std::string name = value.As<Napi::String>().Utf8Value();
  int type = getFieldTypeByName(name.c_str());
  if (type < 0) {
    Napi::Error::New(info.Env(), "Unrecognized field type").ThrowAsJavaScriptException();
  } else {
    def->this_->SetType(static_cast<OGRFieldType>(type));
  }
}

void FieldDefnNapi::justificationSetter(const Napi::CallbackInfo &info,
                                        const Napi::Value &value) {
  FieldDefnNapi *def = FieldDefnNapi::Unwrap(info.This().As<Napi::Object>());
  if (!def || !def->isAlive()) {
    Napi::Error::New(info.Env(), "FieldDefnNapi object has already been destroyed")
      .ThrowAsJavaScriptException();
    return;
  }

  OGRJustification justification;
  if (value.IsString()) {
    std::string str = value.As<Napi::String>().Utf8Value();
    if (str == "Left") {
      justification = OJLeft;
    } else if (str == "Right") {
      justification = OJRight;
    } else if (str == "Undefined") {
      justification = OJUndefined;
    } else {
      Napi::Error::New(info.Env(), "Unrecognized justification").ThrowAsJavaScriptException();
      return;
    }
  } else if (value.IsNull() || value.IsUndefined()) {
    justification = OJUndefined;
  } else {
    Napi::Error::New(info.Env(), "justification must be a string or undefined")
      .ThrowAsJavaScriptException();
    return;
  }

  def->this_->SetJustify(justification);
}

void FieldDefnNapi::widthSetter(const Napi::CallbackInfo &info, const Napi::Value &value) {
  FieldDefnNapi *def = FieldDefnNapi::Unwrap(info.This().As<Napi::Object>());
  if (!def || !def->isAlive()) {
    Napi::Error::New(info.Env(), "FieldDefnNapi object has already been destroyed")
      .ThrowAsJavaScriptException();
    return;
  }
  if (!value.IsNumber()) {
    Napi::Error::New(info.Env(), "width must be an integer").ThrowAsJavaScriptException();
    return;
  }
  def->this_->SetWidth(value.As<Napi::Number>().Int32Value());
}

void FieldDefnNapi::precisionSetter(const Napi::CallbackInfo &info, const Napi::Value &value) {
  FieldDefnNapi *def = FieldDefnNapi::Unwrap(info.This().As<Napi::Object>());
  if (!def || !def->isAlive()) {
    Napi::Error::New(info.Env(), "FieldDefnNapi object has already been destroyed")
      .ThrowAsJavaScriptException();
    return;
  }
  if (!value.IsNumber()) {
    Napi::Error::New(info.Env(), "precision must be an integer").ThrowAsJavaScriptException();
    return;
  }
  def->this_->SetPrecision(value.As<Napi::Number>().Int32Value());
}

void FieldDefnNapi::ignoredSetter(const Napi::CallbackInfo &info, const Napi::Value &value) {
  FieldDefnNapi *def = FieldDefnNapi::Unwrap(info.This().As<Napi::Object>());
  if (!def || !def->isAlive()) {
    Napi::Error::New(info.Env(), "FieldDefnNapi object has already been destroyed")
      .ThrowAsJavaScriptException();
    return;
  }
  if (!value.IsBoolean()) {
    Napi::Error::New(info.Env(), "ignored must be a boolean").ThrowAsJavaScriptException();
    return;
  }
  def->this_->SetIgnored(value.As<Napi::Boolean>().Value());
}

} // namespace node_gdal
