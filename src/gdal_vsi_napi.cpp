#include "gdal_vsi_napi.hpp"
#include "gdal_common_napi.hpp"
#include "async_napi.hpp"

namespace node_gdal {
namespace VsiNapi {

// =========================================================================
// stat_do
// =========================================================================

static Napi::Value stat_do(const Napi::CallbackInfo &info, bool async);
static Napi::Value stat(const Napi::CallbackInfo &info) { return stat_do(info, false); }
static Napi::Value statAsync(const Napi::CallbackInfo &info) { return stat_do(info, true); }
static Napi::Value stat_do(const Napi::CallbackInfo &info, bool async) {
  std::string filename;
  bool bigint = false;
  NAPI_ARG_STR(0, "filename", filename);
  NAPI_ARG_BOOL_OPT(1, "bigint", bigint);

  struct StatResult {
    VSIStatBufL buf;
    bool bigint;
  };

  GDALAsyncableJobNapi<StatResult> job;
  job.main = [filename, bigint]() -> StatResult {
    StatResult r;
    r.bigint = bigint;
    CPLErrorReset();
    if (VSIStatL(filename.c_str(), &r.buf) < 0) throw CPLGetLastErrorMsg();
    return r;
  };

  job.rval = [](Napi::Env env, StatResult r) -> Napi::Value {
    Napi::Object obj = Napi::Object::New(env);
    auto setBig = [&](const char *key, auto val) {
      obj.Set(key, Napi::BigInt::New(env, static_cast<int64_t>(val)));
    };
    auto setNum = [&](const char *key, double val) {
      obj.Set(key, Napi::Number::New(env, val));
    };
    auto setUndef = [&](const char *key) {
      obj.Set(key, env.Undefined());
    };

    if (r.bigint) {
      setBig("dev", r.buf.st_dev);
      setBig("mode", r.buf.st_mode);
      setBig("nlink", r.buf.st_nlink);
      setBig("uid", r.buf.st_uid);
      setBig("gid", r.buf.st_gid);
      setBig("rdev", r.buf.st_rdev);
#ifndef WIN32
      setBig("blksize", r.buf.st_blksize);
      setBig("ino", r.buf.st_ino);
      setBig("size", r.buf.st_size);
      setBig("blocks", r.buf.st_blocks);
#else
      setUndef("blksize");
      setUndef("ino");
      setBig("size", r.buf.st_size);
      setUndef("blocks");
#endif
    } else {
      setNum("dev", static_cast<uint32_t>(r.buf.st_dev));
      setNum("mode", r.buf.st_mode);
      setNum("nlink", static_cast<uint32_t>(r.buf.st_nlink));
      setNum("uid", r.buf.st_uid);
      setNum("gid", r.buf.st_gid);
      setNum("rdev", static_cast<uint32_t>(r.buf.st_rdev));
#ifndef WIN32
      setNum("blksize", static_cast<double>(r.buf.st_blksize));
      setNum("ino", static_cast<double>(r.buf.st_ino));
      setNum("size", static_cast<double>(r.buf.st_size));
      setNum("blocks", static_cast<double>(r.buf.st_blocks));
#else
      setUndef("blksize");
      setUndef("ino");
      setNum("size", static_cast<double>(r.buf.st_size));
      setUndef("blocks");
#endif
    }

    obj.Set("atime", Napi::Date::New(env, r.buf.st_atime * 1000));
    obj.Set("mtime", Napi::Date::New(env, r.buf.st_mtime * 1000));
    obj.Set("ctime", Napi::Date::New(env, r.buf.st_ctime * 1000));
    return obj;
  };

  return job.run(info, async, 2);
}

// =========================================================================
// readDir_do
// =========================================================================

static Napi::Value readDir_do(const Napi::CallbackInfo &info, bool async);
static Napi::Value readDir(const Napi::CallbackInfo &info) { return readDir_do(info, false); }
static Napi::Value readDirAsync(const Napi::CallbackInfo &info) { return readDir_do(info, true); }
static Napi::Value readDir_do(const Napi::CallbackInfo &info, bool async) {
  std::string dir;
  NAPI_ARG_STR(0, "directory", dir);

  GDALAsyncableJobNapi<std::vector<std::string>> job;
  job.main = [dir]() -> std::vector<std::string> {
    CPLErrorReset();
    std::vector<std::string> result;
    char **files = VSIReadDir(dir.c_str());
    if (!files) {
      if (CPLGetLastErrorType() != CE_None) throw CPLGetLastErrorMsg();
      return {}; // empty directory
    }
    for (int i = 0; files[i]; i++) result.push_back(files[i]);
    CSLDestroy(files);
    return result;
  };

  job.rval = [](Napi::Env env, std::vector<std::string> files) -> Napi::Value {
    Napi::Array arr = Napi::Array::New(env, files.size());
    for (size_t i = 0; i < files.size(); i++)
      arr.Set(i, Napi::String::New(env, files[i]));
    return arr;
  };

  return job.run(info, async, 1);
}

// =========================================================================
// Init
// =========================================================================

Napi::Object Init(Napi::Env env, Napi::Object exports) {
  auto obj = Napi::Object::New(env);

  obj.Set("stat", Napi::Function::New(env, stat));
  obj.Set("statAsync", Napi::Function::New(env, statAsync));
  obj.Set("readDir", Napi::Function::New(env, readDir));
  obj.Set("readDirAsync", Napi::Function::New(env, readDirAsync));

  exports.Set("VsiNapi", obj);
  exports.Set("fs", obj);  // gdal.fs.stat / readDir (NAN compatibility)
  return exports;
}

} // namespace VsiNapi
} // namespace node_gdal
