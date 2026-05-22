// napi
#include <napi.h>

// gdal
#include <gdal.h>

// node-gdal
#include "gdal_algorithms_napi.hpp"
#include "gdal_dataset_napi.hpp"
#include "gdal_driver_napi.hpp"
#include "gdal_rasterband_napi.hpp"
#include "gdal_multi_napi.hpp"
#include "gdal_algebra_napi.hpp"
#include "gdal_coordinate_transformation_napi.hpp"
#include "gdal_feature_napi.hpp"
#include "gdal_feature_defn_napi.hpp"
#include "gdal_field_defn_napi.hpp"
#include "geometry/gdal_geometrycollection_napi.hpp"
#include "geometry/gdal_geometry_napi.hpp"
#include "gdal_layer_napi.hpp"
#include "geometry/gdal_simplecurve_napi.hpp"
#include "geometry/gdal_linearring_napi.hpp"
#include "geometry/gdal_linestring_napi.hpp"
#include "geometry/gdal_circularstring_napi.hpp"
#include "geometry/gdal_compoundcurve_napi.hpp"
#include "geometry/gdal_point_napi.hpp"
#include "geometry/gdal_polygon_napi.hpp"
#include "gdal_spatial_reference_napi.hpp"
#include "gdal_memfile_napi.hpp"

#include "utils/field_types.hpp"

// collections
#include "collections/gdal_drivers_napi.hpp"
#include "collections/colortable_napi.hpp"
#include "collections/collections_napi.hpp"
#include "gdal_utils_napi.hpp"
#include "gdal_stubs_napi.hpp"
#include "gdal_vsi_napi.hpp"

// std
#include <sstream>
#include <string>
#include <thread>
#include <vector>

namespace node_gdal {

FILE *log_file = NULL;
bool eventLoopWarn = true;
std::thread::id mainV8ThreadId;

// N-API accessor for gdal.lastError
static Napi::Value LastErrorGetter(const Napi::CallbackInfo &info) {
  Napi::Env env = info.Env();
  int errtype = CPLGetLastErrorType();
  if (errtype == CE_None) return env.Null();
  Napi::Object result = Napi::Object::New(env);
  result.Set("code", Napi::Number::New(env, CPLGetLastErrorNo()));
  result.Set("message", Napi::String::New(env, CPLGetLastErrorMsg()));
  result.Set("level", Napi::Number::New(env, errtype));
  return result;
}

static void LastErrorSetter(const Napi::CallbackInfo &info) {
  if (info[0].IsNull()) {
    CPLErrorReset();
  } else {
    Napi::Error::New(info.Env(), "'lastError' only supports being set to null").ThrowAsJavaScriptException();
  }
}

static Napi::Value EventLoopWarningGetter(const Napi::CallbackInfo &info) {
  return Napi::Boolean::New(info.Env(), eventLoopWarn);
}

static void EventLoopWarningSetter(const Napi::CallbackInfo &info) {
  if (!info[0].IsBoolean()) {
    Napi::Error::New(info.Env(), "'eventLoopWarning' must be a boolean value").ThrowAsJavaScriptException();
    return;
  }
  eventLoopWarn = info[0].As<Napi::Boolean>();
}

// N-API module init – registers classes, constants, and functions
Napi::Object InitNapi(Napi::Env napiEnv, Napi::Object exports) {
  mainV8ThreadId = std::this_thread::get_id();

  // N-API top-level module functions
  exports.Set("open", Napi::Function::New(napiEnv, [](const Napi::CallbackInfo &info) -> Napi::Value {
    std::string path, mode = "r";
    NAPI_ARG_STR(0, "path", path);
    NAPI_ARG_OPT_STR(1, "mode", mode);
    unsigned int flags = GDAL_OF_VERBOSE_ERROR;
    for (size_t i = 0; i < mode.length(); i++) {
      if (mode[i] == 'r') { flags |= (i+1<mode.length()&&mode[i+1]=='+') ? (i++, GDAL_OF_UPDATE) : GDAL_OF_READONLY; }
      else if (mode[i] == 'm') { flags |= GDAL_OF_MULTIDIM_RASTER; }
      else if (mode[i] == 't') flags |= GDAL_OF_THREAD_SAFE | GDAL_OF_RASTER;
      else { Napi::Error::New(info.Env(), "Invalid open mode").ThrowAsJavaScriptException(); return info.Env().Undefined(); }
    }
    GDALDataset *ds = (GDALDataset *)GDALOpenEx(path.c_str(), flags, nullptr, nullptr, nullptr);
    if (!ds) NAPI_THROW_LAST_CPLERR;
    return DatasetNapi::New(info.Env(), ds);
  }, "open"));
  // openAsync – async variant using GDALAsyncableJobNapi
  exports.Set("openAsync", Napi::Function::New(napiEnv, [](const Napi::CallbackInfo &info) -> Napi::Value {
    std::string path, mode = "r";
    NAPI_ARG_STR(0, "path", path);
    NAPI_ARG_OPT_STR(1, "mode", mode);
    unsigned int flags = GDAL_OF_VERBOSE_ERROR;
    for (size_t i = 0; i < mode.length(); i++) {
      if (mode[i] == 'r') { flags |= (i+1<mode.length()&&mode[i+1]=='+') ? (i++, GDAL_OF_UPDATE) : GDAL_OF_READONLY; }
      else if (mode[i] == 'm') { flags |= GDAL_OF_MULTIDIM_RASTER; }
      else if (mode[i] == 't') flags |= GDAL_OF_THREAD_SAFE | GDAL_OF_RASTER;
      else { Napi::Error::New(info.Env(), "Invalid open mode").ThrowAsJavaScriptException(); return info.Env().Undefined(); }
    }
    GDALAsyncableJobNapi<GDALDataset *> job;
    std::string p = path; unsigned int f = flags;
    job.main = [p, f]() -> GDALDataset * {
      CPLErrorReset();
      GDALDataset *ds = (GDALDataset *)GDALOpenEx(p.c_str(), f, nullptr, nullptr, nullptr);
      if (!ds) throw CPLGetLastErrorMsg();
      return ds;
    };
    job.rval = [](Napi::Env env, GDALDataset *ds) { return DatasetNapi::New(env, ds); };
    return job.run(info, true, static_cast<int>(info.Length()) - 1);
  }, "openAsync"));
  exports.Set("setConfigOption", Napi::Function::New(napiEnv, [](const Napi::CallbackInfo &info) -> Napi::Value {
    std::string name; NAPI_ARG_STR(0, "name", name);
    if (info.Length() < 2) { Napi::Error::New(info.Env(), "value required").ThrowAsJavaScriptException(); return info.Env().Undefined(); }
    if (info[1].IsString()) CPLSetConfigOption(name.c_str(), info[1].As<Napi::String>().Utf8Value().c_str());
    else if (info[1].IsNull() || info[1].IsUndefined()) CPLSetConfigOption(name.c_str(), nullptr);
    else { Napi::Error::New(info.Env(), "value must be string or null").ThrowAsJavaScriptException(); }
    return info.Env().Undefined();
  }, "setConfigOption"));
  exports.Set("getConfigOption", Napi::Function::New(napiEnv, [](const Napi::CallbackInfo &info) -> Napi::Value {
    std::string name; NAPI_ARG_STR(0, "name", name);
    return SafeStringNapi(info.Env(), CPLGetConfigOption(name.c_str(), nullptr));
  }, "getConfigOption"));
  exports.Set("decToDMS", Napi::Function::New(napiEnv, [](const Napi::CallbackInfo &info) -> Napi::Value {
    double angle; std::string axis; int precision = 2;
    NAPI_ARG_DOUBLE(0, "angle", angle); NAPI_ARG_STR(1, "axis", axis);
    NAPI_ARG_INT_OPT(2, "precision", precision);
    if (!axis.empty()) axis[0] = (char)toupper(axis[0]);
    if (axis != "Lat" && axis != "Long") { Napi::Error::New(info.Env(), "Axis must be 'lat' or 'long'").ThrowAsJavaScriptException(); return info.Env().Undefined(); }
    return SafeStringNapi(info.Env(), GDALDecToDMS(angle, axis.c_str(), precision));
  }, "decToDMS"));
  exports.Set("setPROJSearchPath", Napi::Function::New(napiEnv, [](const Napi::CallbackInfo &info) -> Napi::Value {
    std::string path; NAPI_ARG_STR(0, "path", path);
    const char *paths[] = {path.c_str(), nullptr};
    OSRSetPROJSearchPaths(paths);
    return info.Env().Undefined();
  }, "setPROJSearchPath"));
  exports.Set("_triggerCPLError", Napi::Function::New(napiEnv, [](const Napi::CallbackInfo &info) -> Napi::Value {
    CPLError(CE_Failure, CPLE_AppDefined, "Mock error");
    return info.Env().Undefined();
  }, "_triggerCPLError"));

  exports.Set("quiet", Napi::Function::New(napiEnv, [](const Napi::CallbackInfo &info) -> Napi::Value {
    CPLSetErrorHandler(CPLQuietErrorHandler);
    return info.Env().Undefined();
  }, "quiet"));
  exports.Set("verbose", Napi::Function::New(napiEnv, [](const Napi::CallbackInfo &info) -> Napi::Value {
    CPLSetErrorHandler(CPLDefaultErrorHandler);
    return info.Env().Undefined();
  }, "verbose"));
  exports.Set("startLogging", Napi::Function::New(napiEnv, [](const Napi::CallbackInfo &info) -> Napi::Value {
#ifdef ENABLE_LOGGING
    std::string filename; NAPI_ARG_STR(0, "filename", filename);
    if (filename.empty()) { Napi::Error::New(info.Env(), "Invalid filename").ThrowAsJavaScriptException(); return info.Env().Undefined(); }
    if (log_file) fclose(log_file);
    log_file = fopen(filename.c_str(), "w");
    if (!log_file) { Napi::Error::New(info.Env(), "Error creating log file").ThrowAsJavaScriptException(); }
#else
    Napi::Error::New(info.Env(), "Logging requires node-gdal be compiled with --enable_logging=true").ThrowAsJavaScriptException();
#endif
    return info.Env().Undefined();
  }, "startLogging"));
  exports.Set("stopLogging", Napi::Function::New(napiEnv, [](const Napi::CallbackInfo &info) -> Napi::Value {
#ifdef ENABLE_LOGGING
    if (log_file) { fclose(log_file); log_file = NULL; }
#endif
    return info.Env().Undefined();
  }, "stopLogging"));

  // --- N-API constants ---
  exports.Set("DMD_LONGNAME", Napi::String::New(napiEnv, GDAL_DMD_LONGNAME));
  exports.Set("DMD_MIMETYPE", Napi::String::New(napiEnv, GDAL_DMD_MIMETYPE));
  exports.Set("DMD_HELPTOPIC", Napi::String::New(napiEnv, GDAL_DMD_HELPTOPIC));
  exports.Set("DMD_EXTENSION", Napi::String::New(napiEnv, GDAL_DMD_EXTENSION));
  exports.Set("DMD_CREATIONOPTIONLIST", Napi::String::New(napiEnv, GDAL_DMD_CREATIONOPTIONLIST));
  exports.Set("DMD_CREATIONDATATYPES", Napi::String::New(napiEnv, GDAL_DMD_CREATIONDATATYPES));
  exports.Set("CE_None", Napi::Number::New(napiEnv, CE_None));
  exports.Set("CE_Debug", Napi::Number::New(napiEnv, CE_Debug));
  exports.Set("CE_Warning", Napi::Number::New(napiEnv, CE_Warning));
  exports.Set("CE_Failure", Napi::Number::New(napiEnv, CE_Failure));
  exports.Set("CE_Fatal", Napi::Number::New(napiEnv, CE_Fatal));
  exports.Set("CPLE_None", Napi::Number::New(napiEnv, CPLE_None));
  exports.Set("CPLE_AppDefined", Napi::Number::New(napiEnv, CPLE_AppDefined));
  exports.Set("CPLE_OutOfMemory", Napi::Number::New(napiEnv, CPLE_OutOfMemory));
  exports.Set("CPLE_FileIO", Napi::Number::New(napiEnv, CPLE_FileIO));
  exports.Set("CPLE_OpenFailed", Napi::Number::New(napiEnv, CPLE_OpenFailed));
  exports.Set("CPLE_IllegalArg", Napi::Number::New(napiEnv, CPLE_IllegalArg));
  exports.Set("CPLE_NotSupported", Napi::Number::New(napiEnv, CPLE_NotSupported));
  exports.Set("CPLE_AssertionFailed", Napi::Number::New(napiEnv, CPLE_AssertionFailed));
  exports.Set("CPLE_NoWriteAccess", Napi::Number::New(napiEnv, CPLE_NoWriteAccess));
  exports.Set("CPLE_UserInterrupt", Napi::Number::New(napiEnv, CPLE_UserInterrupt));
  exports.Set("CPLE_ObjectNull", Napi::Number::New(napiEnv, CPLE_ObjectNull));
  exports.Set("DCAP_CREATE", Napi::String::New(napiEnv, GDAL_DCAP_CREATE));
  exports.Set("DCAP_CREATECOPY", Napi::String::New(napiEnv, GDAL_DCAP_CREATECOPY));
  exports.Set("DCAP_VIRTUALIO", Napi::String::New(napiEnv, GDAL_DCAP_VIRTUALIO));
  exports.Set("OLCRandomRead", Napi::String::New(napiEnv, OLCRandomRead));
  exports.Set("OLCSequentialWrite", Napi::String::New(napiEnv, OLCSequentialWrite));
  exports.Set("OLCRandomWrite", Napi::String::New(napiEnv, OLCRandomWrite));
  exports.Set("OLCFastSpatialFilter", Napi::String::New(napiEnv, OLCFastSpatialFilter));
  exports.Set("OLCFastFeatureCount", Napi::String::New(napiEnv, OLCFastFeatureCount));
  exports.Set("OLCFastGetExtent", Napi::String::New(napiEnv, OLCFastGetExtent));
  exports.Set("OLCCreateField", Napi::String::New(napiEnv, OLCCreateField));
  exports.Set("OLCDeleteField", Napi::String::New(napiEnv, OLCDeleteField));
  exports.Set("OLCReorderFields", Napi::String::New(napiEnv, OLCReorderFields));
  exports.Set("OLCAlterFieldDefn", Napi::String::New(napiEnv, OLCAlterFieldDefn));
  exports.Set("OLCTransactions", Napi::String::New(napiEnv, OLCTransactions));
  exports.Set("OLCDeleteFeature", Napi::String::New(napiEnv, OLCDeleteFeature));
  exports.Set("OLCStringsAsUTF8", Napi::String::New(napiEnv, OLCStringsAsUTF8));
  exports.Set("OLCIgnoreFields", Napi::String::New(napiEnv, OLCIgnoreFields));
  exports.Set("OLCCreateGeomField", Napi::String::New(napiEnv, OLCCreateGeomField));
  exports.Set("ODsCCreateLayer", Napi::String::New(napiEnv, ODsCCreateLayer));
  exports.Set("ODsCDeleteLayer", Napi::String::New(napiEnv, ODsCDeleteLayer));
  exports.Set("ODrCCreateDataSource", Napi::String::New(napiEnv, ODrCCreateDataSource));
  exports.Set("ODrCDeleteDataSource", Napi::String::New(napiEnv, ODrCDeleteDataSource));
  exports.Set("GDT_Byte", Napi::String::New(napiEnv, GDALGetDataTypeName(GDT_Byte)));
  exports.Set("GDT_UInt16", Napi::String::New(napiEnv, GDALGetDataTypeName(GDT_UInt16)));
  exports.Set("GDT_Int16", Napi::String::New(napiEnv, GDALGetDataTypeName(GDT_Int16)));
  exports.Set("GDT_UInt32", Napi::String::New(napiEnv, GDALGetDataTypeName(GDT_UInt32)));
  exports.Set("GDT_Int32", Napi::String::New(napiEnv, GDALGetDataTypeName(GDT_Int32)));
  exports.Set("GDT_Int64", Napi::String::New(napiEnv, GDALGetDataTypeName(GDT_Int64)));
  exports.Set("GDT_UInt64", Napi::String::New(napiEnv, GDALGetDataTypeName(GDT_UInt64)));
  exports.Set("GDT_CInt16", Napi::String::New(napiEnv, GDALGetDataTypeName(GDT_CInt16)));
  exports.Set("GDT_CInt32", Napi::String::New(napiEnv, GDALGetDataTypeName(GDT_CInt32)));
  exports.Set("GDT_Unknown", napiEnv.Null());
  exports.Set("GDT_Float32", Napi::String::New(napiEnv, GDALGetDataTypeName(GDT_Float32)));
  exports.Set("GDT_Float64", Napi::String::New(napiEnv, GDALGetDataTypeName(GDT_Float64)));
  exports.Set("GDT_CFloat32", Napi::String::New(napiEnv, GDALGetDataTypeName(GDT_CFloat32)));
  exports.Set("GDT_CFloat64", Napi::String::New(napiEnv, GDALGetDataTypeName(GDT_CFloat64)));
  exports.Set("wkb25DBit", Napi::Number::New(napiEnv, wkb25DBit));
  exports.Set("wkbUnknown", Napi::Number::New(napiEnv, wkbUnknown));
  exports.Set("wkbPoint", Napi::Number::New(napiEnv, wkbPoint));
  exports.Set("wkbLineString", Napi::Number::New(napiEnv, wkbLineString));
  exports.Set("wkbCircularString", Napi::Number::New(napiEnv, wkbCircularString));
  exports.Set("wkbCompoundCurve", Napi::Number::New(napiEnv, wkbCompoundCurve));
  exports.Set("wkbMultiCurve", Napi::Number::New(napiEnv, wkbMultiCurve));
  exports.Set("wkbPolygon", Napi::Number::New(napiEnv, wkbPolygon));
  exports.Set("wkbMultiPoint", Napi::Number::New(napiEnv, wkbMultiPoint));
  exports.Set("wkbMultiLineString", Napi::Number::New(napiEnv, wkbMultiLineString));
  exports.Set("wkbMultiPolygon", Napi::Number::New(napiEnv, wkbMultiPolygon));
  exports.Set("wkbGeometryCollection", Napi::Number::New(napiEnv, wkbGeometryCollection));
  exports.Set("wkbNone", Napi::Number::New(napiEnv, wkbNone));
  exports.Set("wkbLinearRing", Napi::Number::New(napiEnv, wkbLinearRing));
  exports.Set("wkbPoint25D", Napi::Number::New(napiEnv, wkbPoint25D));
  exports.Set("wkbLineString25D", Napi::Number::New(napiEnv, wkbLineString25D));
  exports.Set("wkbPolygon25D", Napi::Number::New(napiEnv, wkbPolygon25D));
  exports.Set("wkbMultiPoint25D", Napi::Number::New(napiEnv, wkbMultiPoint25D));
  exports.Set("wkbMultiLineString25D", Napi::Number::New(napiEnv, wkbMultiLineString25D));
  exports.Set("wkbMultiPolygon25D", Napi::Number::New(napiEnv, wkbMultiPolygon25D));
  exports.Set("wkbGeometryCollection25D", Napi::Number::New(napiEnv, wkbGeometryCollection25D));
  exports.Set("wkbLinearRing25D", Napi::Number::New(napiEnv, wkbLinearRing | wkb25DBit));
  exports.Set("OFTInteger", Napi::String::New(napiEnv, getFieldTypeName(OFTInteger)));
  exports.Set("OFTReal", Napi::String::New(napiEnv, getFieldTypeName(OFTReal)));
  exports.Set("OFTRealList", Napi::String::New(napiEnv, getFieldTypeName(OFTRealList)));
  exports.Set("OFTString", Napi::String::New(napiEnv, getFieldTypeName(OFTString)));
  exports.Set("OFTBinary", Napi::String::New(napiEnv, getFieldTypeName(OFTBinary)));
  exports.Set("OFTDate", Napi::String::New(napiEnv, getFieldTypeName(OFTDate)));
  exports.Set("OFTTime", Napi::String::New(napiEnv, getFieldTypeName(OFTTime)));
  exports.Set("OFTDateTime", Napi::String::New(napiEnv, getFieldTypeName(OFTDateTime)));
  exports.Set("DIM_VERTICAL", Napi::String::New(napiEnv, GDAL_DIM_TYPE_VERTICAL));
  exports.Set("DIM_TEMPORAL", Napi::String::New(napiEnv, GDAL_DIM_TYPE_TEMPORAL));
  exports.Set("DIM_PARAMETRIC", Napi::String::New(napiEnv, GDAL_DIM_TYPE_PARAMETRIC));
  exports.Set("OFTIntegerList", Napi::String::New(napiEnv, getFieldTypeName(OFTIntegerList)));
  exports.Set("OFTStringList", Napi::String::New(napiEnv, getFieldTypeName(OFTStringList)));
  exports.Set("OFTWideString", Napi::String::New(napiEnv, getFieldTypeName(OFTWideString)));
  exports.Set("OFTWideStringList", Napi::String::New(napiEnv, getFieldTypeName(OFTWideStringList)));
  exports.Set("GRA_NearestNeighbor", Napi::String::New(napiEnv, "NearestNeighbor"));
  exports.Set("GRA_Bilinear", Napi::String::New(napiEnv, "Bilinear"));
  exports.Set("GRA_Cubic", Napi::String::New(napiEnv, "Cubic"));
  exports.Set("GRA_CubicSpline", Napi::String::New(napiEnv, "CubicSpline"));
  exports.Set("GRA_Lanczos", Napi::String::New(napiEnv, "Lanczos"));
  exports.Set("GRA_Average", Napi::String::New(napiEnv, "Average"));
  exports.Set("GRA_Mode", Napi::String::New(napiEnv, "Mode"));
  exports.Set("GRA_Min", Napi::String::New(napiEnv, "Min"));
  exports.Set("GRA_Max", Napi::String::New(napiEnv, "Max"));
  exports.Set("GRA_Med", Napi::String::New(napiEnv, "Med"));
  exports.Set("GRA_Q1", Napi::String::New(napiEnv, "Q1"));
  exports.Set("GRA_Q3", Napi::String::New(napiEnv, "Q3"));
  exports.Set("GCI_Undefined", Napi::Number::New(napiEnv, GCI_Undefined));
  exports.Set("GCI_GrayIndex", Napi::Number::New(napiEnv, GCI_GrayIndex));
  exports.Set("GCI_PaletteIndex", Napi::Number::New(napiEnv, GCI_PaletteIndex));
  exports.Set("GCI_RedBand", Napi::Number::New(napiEnv, GCI_RedBand));
  exports.Set("GCI_GreenBand", Napi::Number::New(napiEnv, GCI_GreenBand));
  exports.Set("GCI_BlueBand", Napi::Number::New(napiEnv, GCI_BlueBand));
  exports.Set("GCI_AlphaBand", Napi::Number::New(napiEnv, GCI_AlphaBand));
  exports.Set("GCI_HueBand", Napi::Number::New(napiEnv, GCI_HueBand));
  exports.Set("GCI_SaturationBand", Napi::Number::New(napiEnv, GCI_SaturationBand));
  exports.Set("GCI_LightnessBand", Napi::Number::New(napiEnv, GCI_LightnessBand));
  exports.Set("GCI_CyanBand", Napi::Number::New(napiEnv, GCI_CyanBand));
  exports.Set("GCI_MagentaBand", Napi::Number::New(napiEnv, GCI_MagentaBand));
  exports.Set("GCI_YellowBand", Napi::Number::New(napiEnv, GCI_YellowBand));
  exports.Set("GCI_BlackBand", Napi::Number::New(napiEnv, GCI_BlackBand));
  exports.Set("GPI_RGB", Napi::Number::New(napiEnv, GPI_RGB));
  exports.Set("GPI_CMYK", Napi::Number::New(napiEnv, GPI_CMYK));
  exports.Set("GPI_HLS", Napi::Number::New(napiEnv, GPI_HLS));
  exports.Set("GA_ReadOnly", Napi::Number::New(napiEnv, GA_ReadOnly));
  exports.Set("GA_Update", Napi::Number::New(napiEnv, GA_Update));
  exports.Set("GF_READ", Napi::Number::New(napiEnv, GF_Read));
  exports.Set("GF_WRITE", Napi::Number::New(napiEnv, GF_Write));
  exports.Set("GDAL_OF_READONLY", Napi::Number::New(napiEnv, GDAL_OF_READONLY));
  exports.Set("GDAL_OF_UPDATE", Napi::Number::New(napiEnv, GDAL_OF_UPDATE));
  exports.Set("GDAL_OF_ALL", Napi::Number::New(napiEnv, GDAL_OF_ALL));
  exports.Set("GDAL_OF_RASTER", Napi::Number::New(napiEnv, GDAL_OF_RASTER));
  exports.Set("GDAL_OF_VECTOR", Napi::Number::New(napiEnv, GDAL_OF_VECTOR));
  exports.Set("GDAL_OF_VERBOSE_ERROR", Napi::Number::New(napiEnv, GDAL_OF_VERBOSE_ERROR));
  exports.Set("GDAL_OF_THREAD_SAFE", Napi::Number::New(napiEnv, GDAL_OF_THREAD_SAFE));
  exports.Set("GDAL_OF_SHARED", Napi::Number::New(napiEnv, GDAL_OF_SHARED));
  exports.Set("OJ_LEFT", Napi::String::New(napiEnv, "LEFT"));
  exports.Set("OJ_RIGHT", Napi::String::New(napiEnv, "RIGHT"));
  exports.Set("OJ_UNDEFINED", Napi::String::New(napiEnv, "UNDEFINED"));
  exports.Set("CPLS_DEBUG", Napi::String::New(napiEnv, "CPL_DEBUG"));
  exports.Set("VSISTDIN", Napi::String::New(napiEnv, "/vsistdin/"));
  exports.Set("VSISTDOUT", Napi::String::New(napiEnv, "/vsistdout/"));
  exports.Set("GEDTC_String", Napi::String::New(napiEnv, "String"));
  exports.Set("GEDTC_Compound", Napi::String::New(napiEnv, "Compound"));
  exports.Set("DIR_PAST", Napi::String::New(napiEnv, "PAST"));
  exports.Set("version", Napi::String::New(napiEnv, GDAL_RELEASE_NAME));
#ifdef BUNDLED_GDAL
  exports.Set("bundled", Napi::Boolean::New(napiEnv, true));
#else
  exports.Set("bundled", Napi::Boolean::New(napiEnv, false));
#endif
  exports.Set("log", Napi::Function::New(napiEnv, [](const Napi::CallbackInfo &info) -> Napi::Value {
    std::string msg; NAPI_ARG_STR(0, "message", msg);
    msg = msg + "\n";
#ifdef ENABLE_LOGGING
    if (log_file) { fputs(msg.c_str(), log_file); fflush(log_file); }
#endif
    return info.Env().Undefined();
  }, "log"));
  exports.Set("supports", Napi::Object::New(napiEnv));

  exports.DefineProperties({
    Napi::PropertyDescriptor::Accessor(napiEnv, exports, "lastError", LastErrorGetter, LastErrorSetter),
    Napi::PropertyDescriptor::Accessor(
      napiEnv, exports, "eventLoopWarning", EventLoopWarningGetter, EventLoopWarningSetter),
  });

  node_gdal::DriverNapi::Init(napiEnv, exports);
  node_gdal::DatasetNapi::Init(napiEnv, exports);
  node_gdal::RasterBandNapi::Init(napiEnv, exports);
  node_gdal::LayerNapi::Init(napiEnv, exports);
  node_gdal::FeatureNapi::Init(napiEnv, exports);
  node_gdal::FeatureDefnNapi::Init(napiEnv, exports);
  node_gdal::FieldDefnNapi::Init(napiEnv, exports);
  node_gdal::SpatialReferenceNapi::Init(napiEnv, exports);
  node_gdal::CoordinateTransformationNapi::Init(napiEnv, exports);
  node_gdal::ColorTableNapi::Init(napiEnv, exports);
  node_gdal::GDALDriversNapi::Init(napiEnv, exports);
  node_gdal::GeometryNapi::Init(napiEnv, exports);       // base class: must be first
  node_gdal::GeometryCollectionNapi::Init(napiEnv, exports);
  node_gdal::PointNapi::Init(napiEnv, exports);
  node_gdal::SimpleCurveNapi::Init(napiEnv, exports);
  node_gdal::LineStringNapi::Init(napiEnv, exports);
  node_gdal::LinearRingNapi::Init(napiEnv, exports);
  node_gdal::PolygonNapi::Init(napiEnv, exports);
  node_gdal::CircularStringNapi::Init(napiEnv, exports);
  node_gdal::CompoundCurveNapi::Init(napiEnv, exports);
  node_gdal::MultiPointNapi::Init(napiEnv, exports);
  node_gdal::MultiLineStringNapi::Init(napiEnv, exports);
  node_gdal::MultiPolygonNapi::Init(napiEnv, exports);
  node_gdal::MultiCurveNapi::Init(napiEnv, exports);
  node_gdal::GroupNapi::Init(napiEnv, exports);
  node_gdal::MDArrayNapi::Init(napiEnv, exports);
  node_gdal::DimensionNapi::Init(napiEnv, exports);
  node_gdal::AttributeNapi::Init(napiEnv, exports);
  node_gdal::DatasetBandsNapi::Init(napiEnv, exports);
  node_gdal::DatasetLayersNapi::Init(napiEnv, exports);
  node_gdal::LayerFeaturesNapi::Init(napiEnv, exports);
  node_gdal::WarperNapi::Init(napiEnv, exports);
  node_gdal::AlgorithmsNapi::Init(napiEnv, exports);
  node_gdal::UtilsNapi::Init(napiEnv, exports);
  node_gdal::RasterBandPixelsNapi::Init(napiEnv, exports);
  node_gdal::RasterBandOverviewsNapi::Init(napiEnv, exports);
  node_gdal::LayerFieldsNapi::Init(napiEnv, exports);
  node_gdal::FeatureFieldsNapi::Init(napiEnv, exports);
  node_gdal::FeatureDefnFieldsNapi::Init(napiEnv, exports);
  node_gdal::GeometryCollectionChildrenNapi::Init(napiEnv, exports);
  node_gdal::PolygonRingsNapi::Init(napiEnv, exports);
  node_gdal::LineStringPointsNapi::Init(napiEnv, exports);
  node_gdal::CompoundCurveCurvesNapi::Init(napiEnv, exports);
  node_gdal::GroupGroupsNapi::Init(napiEnv, exports);
  node_gdal::GroupArraysNapi::Init(napiEnv, exports);
  node_gdal::GroupAttributesNapi::Init(napiEnv, exports);
  node_gdal::GroupDimensionsNapi::Init(napiEnv, exports);
  node_gdal::ArrayDimensionsNapi::Init(napiEnv, exports);
  node_gdal::ArrayAttributesNapi::Init(napiEnv, exports);
  node_gdal::VsiNapi::Init(napiEnv, exports);
  node_gdal::AlgebraNapi::Init(napiEnv, exports);
  node_gdal::MemfileNapi::Init(napiEnv, exports);

  // Replace NAN GDALDrivers with N-API version
  exports.Set("drivers", GDALDriversNapi::New(napiEnv));

  return exports;
}

} // namespace node_gdal

// N-API registration wrapper
static Napi::Object InitNapiWrapper(Napi::Env env, Napi::Object exports) {
  return node_gdal::InitNapi(env, exports);
}

NODE_API_MODULE(gdal, InitNapiWrapper);
