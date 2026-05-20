#include <memory>
#include <string>
#include "gdal_driver_napi.hpp"
#include "gdal_dataset_napi.hpp"
#include "gdal_common.hpp"
#include "utils/napi_object_store.hpp"

namespace node_gdal {

// ---------------------------------------------------------------------------
// DriverNapi static members
// ---------------------------------------------------------------------------
Napi::FunctionReference DriverNapi::constructor;

// ---------------------------------------------------------------------------
// Init – register the class with N-API
// ---------------------------------------------------------------------------
Napi::Object DriverNapi::Init(Napi::Env env, Napi::Object exports) {
  Napi::Function func = DefineClass(
    env,
    "DriverNapi",
    {
      InstanceMethod("toString", &DriverNapi::toString),
      InstanceMethod("open", &DriverNapi::open),
      InstanceMethod("openAsync", &DriverNapi::openAsync),
      InstanceMethod("create", &DriverNapi::create),
      InstanceMethod("createAsync", &DriverNapi::createAsync),
      InstanceMethod("createCopy", &DriverNapi::createCopy),
      InstanceMethod("createCopyAsync", &DriverNapi::createCopyAsync),
      InstanceMethod("deleteDataset", &DriverNapi::deleteDataset),
      InstanceMethod("rename", &DriverNapi::rename),
      InstanceMethod("copyFiles", &DriverNapi::copyFiles),
      InstanceMethod("getMetadata", &DriverNapi::getMetadata),
      InstanceAccessor<&DriverNapi::descriptionGetter>("description"),
    });

  constructor = Napi::Persistent(func);
  constructor.SuppressDestruct();

  exports.Set("DriverNapi", func);
  return exports;
}

// ---------------------------------------------------------------------------
// Constructor
// ---------------------------------------------------------------------------
DriverNapi::DriverNapi(const Napi::CallbackInfo &info)
  : Napi::ObjectWrap<DriverNapi>(info), this_gdaldriver(nullptr), uid(0) {

  if (!info.IsConstructCall()) {
    Napi::Error::New(info.Env(), "Cannot call constructor as function, use 'new' keyword")
      .ThrowAsJavaScriptException();
    return;
  }

  if (info.Length() > 0 && info[0].IsExternal()) {
    this_gdaldriver = info[0].As<Napi::External<GDALDriver>>().Data();
    LOG("Created GDAL DriverNapi [%p]", this_gdaldriver);
  } else if (info.Length() == 0 || info[0].IsUndefined()) {
    // default constructor
  } else {
    Napi::Error::New(info.Env(), "Cannot create DriverNapi directly").ThrowAsJavaScriptException();
    return;
  }
}

DriverNapi::~DriverNapi() {
  if (this_gdaldriver) {
    LOG("Disposing GDAL DriverNapi [%p]", this_gdaldriver);
    this_gdaldriver = nullptr;
  }
}

// ---------------------------------------------------------------------------
// Static factory – create a JS DriverNapi from a GDALDriver*
// ---------------------------------------------------------------------------
Napi::Value DriverNapi::New(Napi::Env env, GDALDriver *driver) {
  Napi::EscapableHandleScope scope(env);

  if (!driver) { return scope.Escape(env.Null()); }
  if (napi_obj_store_has<GDALDriver *>(driver)) {
    Napi::Object existing = napi_obj_store_get<GDALDriver *>(env, driver);
    if (!existing.IsEmpty()) return scope.Escape(existing);
  }

  Napi::Object obj = constructor.New({Napi::External<GDALDriver>::New(env, driver)});
  napi_obj_store_add<GDALDriver *>(driver, obj);

  DriverNapi *wrapped = DriverNapi::Unwrap(obj);
  if (wrapped) { wrapped->uid = 0; }

  return scope.Escape(obj);
}

// ---------------------------------------------------------------------------
// Simple methods
// ---------------------------------------------------------------------------

Napi::Value DriverNapi::toString(const Napi::CallbackInfo &info) {
  return Napi::String::New(info.Env(), "Driver");
}

Napi::Value DriverNapi::descriptionGetter(const Napi::CallbackInfo &info) {
  DriverNapi *driver = DriverNapi::Unwrap(info.This().As<Napi::Object>());
  if (!driver || !driver->isAlive()) {
    Napi::Error::New(info.Env(), "DriverNapi object has already been destroyed")
      .ThrowAsJavaScriptException();
    return info.Env().Undefined();
  }
  return SafeStringNapi(info.Env(), driver->getGDALDriver()->GetDescription());
}

Napi::Value DriverNapi::deleteDataset(const Napi::CallbackInfo &info) {
  std::string name;
  NAPI_ARG_STR(0, "dataset name", name);

  DriverNapi *driver = DriverNapi::Unwrap(info.This().As<Napi::Object>());
  if (!driver || !driver->isAlive()) {
    Napi::Error::New(info.Env(), "DriverNapi object has already been destroyed")
      .ThrowAsJavaScriptException();
    return info.Env().Undefined();
  }

  CPLErr err = driver->getGDALDriver()->Delete(name.c_str());
  if (err) NAPI_THROW_LAST_CPLERR;
  return info.Env().Undefined();
}

Napi::Value DriverNapi::rename(const Napi::CallbackInfo &info) {
  std::string new_name, old_name;
  NAPI_ARG_STR(0, "new name", new_name);
  NAPI_ARG_STR(1, "old name", old_name);

  DriverNapi *driver = DriverNapi::Unwrap(info.This().As<Napi::Object>());
  if (!driver || !driver->isAlive()) {
    Napi::Error::New(info.Env(), "DriverNapi object has already been destroyed")
      .ThrowAsJavaScriptException();
    return info.Env().Undefined();
  }

  CPLErr err = driver->getGDALDriver()->Rename(new_name.c_str(), old_name.c_str());
  if (err) NAPI_THROW_LAST_CPLERR;
  return info.Env().Undefined();
}

Napi::Value DriverNapi::copyFiles(const Napi::CallbackInfo &info) {
  std::string new_name, old_name;
  NAPI_ARG_STR(0, "new name", new_name);
  NAPI_ARG_STR(1, "old name", old_name);

  DriverNapi *driver = DriverNapi::Unwrap(info.This().As<Napi::Object>());
  if (!driver || !driver->isAlive()) {
    Napi::Error::New(info.Env(), "DriverNapi object has already been destroyed")
      .ThrowAsJavaScriptException();
    return info.Env().Undefined();
  }

  CPLErr err = driver->getGDALDriver()->CopyFiles(new_name.c_str(), old_name.c_str());
  if (err) NAPI_THROW_LAST_CPLERR;
  return info.Env().Undefined();
}

Napi::Value DriverNapi::getMetadata(const Napi::CallbackInfo &info) {
  DriverNapi *driver = DriverNapi::Unwrap(info.This().As<Napi::Object>());
  if (!driver || !driver->isAlive()) {
    Napi::Error::New(info.Env(), "DriverNapi object has already been destroyed")
      .ThrowAsJavaScriptException();
    return info.Env().Undefined();
  }

  std::string domain;
  NAPI_ARG_OPT_STR(0, "domain", domain);

  GDALDriver *raw = driver->getGDALDriver();
  char **md = raw->GetMetadata(domain.empty() ? nullptr : domain.c_str());

  Napi::Object result = Napi::Object::New(info.Env());
  if (md) {
    for (int i = 0; md[i]; i++) {
      std::string pair = md[i];
      std::size_t i_equal = pair.find_first_of('=');
      if (i_equal != std::string::npos) {
        result.Set(pair.substr(0, i_equal), Napi::String::New(info.Env(), pair.substr(i_equal + 1)));
      }
    }
  }
  return result;
}

// ---------------------------------------------------------------------------
// open, openAsync
// ---------------------------------------------------------------------------

GDAL_ASYNCABLE_DEFINE_NAPI(DriverNapi, open) {
  DriverNapi *driver = DriverNapi::Unwrap(info.This().As<Napi::Object>());
  if (!driver || !driver->isAlive()) {
    Napi::Error::New(info.Env(), "DriverNapi object has already been destroyed")
      .ThrowAsJavaScriptException();
    return info.Env().Undefined();
  }

  std::string path;
  std::string mode = "r";
  GDALAccess access = GA_ReadOnly;

  NAPI_ARG_STR(0, "path", path);
  NAPI_ARG_OPT_STR(1, "mode", mode);

  if (mode == "r+") {
    access = GA_Update;
  } else if (mode != "r") {
    Napi::Error::New(info.Env(), "Invalid open mode. Must be \"r\" or \"r+\"")
      .ThrowAsJavaScriptException();
    return info.Env().Undefined();
  }

  auto options = std::make_shared<NapiStringList>();
  if (info.Length() > 2 && !options->parse(info[2])) {
    Napi::Error::New(info.Env(), "Failed parsing options").ThrowAsJavaScriptException();
    return info.Env().Undefined();
  }

  GDALDriver *raw = driver->getGDALDriver();

  GDALAsyncableJobNapi<GDALDataset *> job;
  job.main = [raw, path, access, options]() {
    const char *driver_list[2] = {raw->GetDescription(), nullptr};
    CPLErrorReset();
    GDALDataset *ds = (GDALDataset *)GDALOpenEx(
      path.c_str(), access, driver_list, options->get(), nullptr);
    if (!ds) throw CPLGetLastErrorMsg();
    return ds;
  };
  job.rval = [](Napi::Env env, GDALDataset *ds) -> Napi::Value {
    return DatasetNapi::New(env, ds);
  };

  return job.run(info, async, 3);
}

// ---------------------------------------------------------------------------
// create, createAsync
// ---------------------------------------------------------------------------

GDAL_ASYNCABLE_DEFINE_NAPI(DriverNapi, create) {
  DriverNapi *driver = DriverNapi::Unwrap(info.This().As<Napi::Object>());
  if (!driver || !driver->isAlive()) {
    Napi::Error::New(info.Env(), "DriverNapi object has already been destroyed")
      .ThrowAsJavaScriptException();
    return info.Env().Undefined();
  }

  std::string filename;
  int x_size = 0, y_size = 0, n_bands = 0;
  GDALDataType type = GDT_Byte;
  std::string type_name;

  NAPI_ARG_STR(0, "filename", filename);

  auto options = std::make_shared<NapiStringList>();

  if (info.Length() < 3) {
    if (info.Length() > 1 && !options->parse(info[1])) {
      Napi::Error::New(info.Env(), "Failed parsing options").ThrowAsJavaScriptException();
      return info.Env().Undefined();
    }
  } else {
    NAPI_ARG_INT(1, "x size", x_size);
    NAPI_ARG_INT(2, "y size", y_size);
    NAPI_ARG_INT_OPT(3, "number of bands", n_bands);
    NAPI_ARG_OPT_STR(4, "data type", type_name);
    if (info.Length() > 5 && !options->parse(info[5])) {
      Napi::Error::New(info.Env(), "Failed parsing options").ThrowAsJavaScriptException();
      return info.Env().Undefined();
    }
    if (!type_name.empty()) { type = GDALGetDataTypeByName(type_name.c_str()); }
  }

  if (x_size == 0 && y_size == 0 && type_name.empty()) { type = GDT_Unknown; }

  GDALDriver *raw = driver->getGDALDriver();

  GDALAsyncableJobNapi<GDALDataset *> job;
  job.main = [raw, filename, x_size, y_size, n_bands, type, options]() {
    CPLErrorReset();
    GDALDataset *ds = raw->Create(
      filename.c_str(), x_size, y_size, n_bands, type, options->get());
    if (!ds) throw CPLGetLastErrorMsg();
    return ds;
  };
  job.rval = [](Napi::Env env, GDALDataset *ds) -> Napi::Value {
    return DatasetNapi::New(env, ds);
  };

  return job.run(info, async, 6);
}

// ---------------------------------------------------------------------------
// createCopy, createCopyAsync
// ---------------------------------------------------------------------------

GDAL_ASYNCABLE_DEFINE_NAPI(DriverNapi, createCopy) {
  DriverNapi *driver = DriverNapi::Unwrap(info.This().As<Napi::Object>());
  if (!driver || !driver->isAlive()) {
    Napi::Error::New(info.Env(), "DriverNapi object has already been destroyed")
      .ThrowAsJavaScriptException();
    return info.Env().Undefined();
  }

  std::string filename;
  NAPI_ARG_STR(0, "filename", filename);

  if (info.Length() < 2 || !info[1].IsObject()) {
    Napi::Error::New(info.Env(), "source dataset must be a Dataset object")
      .ThrowAsJavaScriptException();
    return info.Env().Undefined();
  }

  auto options = std::make_shared<NapiStringList>();
  if (info.Length() > 2 && !options->parse(info[2])) {
    Napi::Error::New(info.Env(), "Failed parsing options").ThrowAsJavaScriptException();
    return info.Env().Undefined();
  }

  bool strict = false;
  NAPI_ARG_BOOL_OPT(3, "strict", strict);

  GDALDriver *raw = driver->getGDALDriver();

  GDALAsyncableJobNapi<GDALDataset *> job;
  job.main = [raw, filename, strict, options]() {
    // TODO: accept Dataset parameter once ported
    CPLErrorReset();
    GDALDataset *ds = raw->CreateCopy(
      filename.c_str(), nullptr, strict, options->get(), nullptr, nullptr);
    if (!ds) throw CPLGetLastErrorMsg();
    return ds;
  };
  job.rval = [](Napi::Env env, GDALDataset *ds) -> Napi::Value {
    return DatasetNapi::New(env, ds);
  };

  return job.run(info, async, 5);
}

} // namespace node_gdal
