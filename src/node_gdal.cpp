// node
#include <node.h>
#include <node_buffer.h>
#include <node_version.h>

// nan
#include "nan-wrapper.h"

// napi (for module init switch)
#include <napi.h>

// gdal
#include <gdal.h>

// node-gdal
#include "gdal_algorithms.hpp"
#include "gdal_common.hpp"
#include "gdal_dataset.hpp"
#include "gdal_dataset_napi.hpp"
#include "gdal_driver.hpp"
#include "gdal_driver_napi.hpp" // Phase 0 N-API prototype
#include "gdal_rasterband.hpp"
#include "gdal_rasterband_napi.hpp"
#include "gdal_group.hpp"
#include "gdal_mdarray.hpp"
#include "gdal_dimension.hpp"
#include "gdal_attribute.hpp"
#include "gdal_multi_napi.hpp"
#include "gdal_warper.hpp"
#include "gdal_utils.hpp"
#include "gdal_algebra.hpp"
#include "gdal_algebra_napi.hpp"

#include "gdal_coordinate_transformation.hpp"
#include "gdal_coordinate_transformation_napi.hpp"
#include "gdal_feature.hpp"
#include "gdal_feature_napi.hpp"
#include "gdal_feature_defn.hpp"
#include "gdal_feature_defn_napi.hpp"
#include "gdal_field_defn.hpp"
#include "gdal_field_defn_napi.hpp"
#include "geometry/gdal_geometry.hpp"
#include "geometry/gdal_geometrycollection.hpp"
#include "geometry/gdal_geometrycollection_napi.hpp"
#include "geometry/gdal_geometry_napi.hpp"
#include "gdal_layer.hpp"
#include "gdal_layer_napi.hpp"
#include "geometry/gdal_simplecurve.hpp"
#include "geometry/gdal_simplecurve_napi.hpp"
#include "geometry/gdal_linearring.hpp"
#include "geometry/gdal_linearring_napi.hpp"
#include "geometry/gdal_linestring.hpp"
#include "geometry/gdal_linestring_napi.hpp"
#include "geometry/gdal_circularstring.hpp"
#include "geometry/gdal_circularstring_napi.hpp"
#include "geometry/gdal_compoundcurve.hpp"
#include "geometry/gdal_compoundcurve_napi.hpp"
#include "geometry/gdal_multilinestring.hpp"
#include "geometry/gdal_multicurve.hpp"
#include "geometry/gdal_multipoint.hpp"
#include "geometry/gdal_multipolygon.hpp"
#include "geometry/gdal_point.hpp"
#include "geometry/gdal_point_napi.hpp"
#include "geometry/gdal_polygon.hpp"
#include "geometry/gdal_polygon_napi.hpp"
#include "gdal_spatial_reference.hpp"
#include "gdal_spatial_reference_napi.hpp"
#include "gdal_memfile.hpp"
#include "gdal_memfile_napi.hpp"
#include "gdal_fs.hpp"

#include "utils/field_types.hpp"

// collections
#include "collections/dataset_bands.hpp"
#include "collections/dataset_layers.hpp"
#include "collections/group_groups.hpp"
#include "collections/group_arrays.hpp"
#include "collections/group_dimensions.hpp"
#include "collections/group_attributes.hpp"
#include "collections/array_dimensions.hpp"
#include "collections/array_attributes.hpp"
#include "collections/feature_defn_fields.hpp"
#include "collections/feature_fields.hpp"
#include "collections/gdal_drivers.hpp"
#include "collections/gdal_drivers_napi.hpp"
#include "collections/geometry_collection_children.hpp"
#include "collections/layer_features.hpp"
#include "collections/layer_fields.hpp"
#include "collections/linestring_points.hpp"
#include "collections/polygon_rings.hpp"
#include "collections/compound_curves.hpp"
#include "collections/rasterband_overviews.hpp"
#include "collections/rasterband_pixels.hpp"
#include "collections/colortable.hpp"
#include "collections/colortable_napi.hpp"
#include "collections/collections_napi.hpp"
#include "gdal_utils_napi.hpp"
#include "gdal_stubs_napi.hpp"
#include "gdal_vsi_napi.hpp"

// std
#include <sstream>
#include <string>
#include <vector>

namespace node_gdal {

using namespace node;
using namespace v8;

FILE *log_file = NULL;
ObjectStore object_store;
bool eventLoopWarn = true;

static NAN_GETTER(LastErrorGetter) {

  int errtype = CPLGetLastErrorType();
  if (errtype == CE_None) {
    info.GetReturnValue().Set(Nan::Null());
    return;
  }

  Local<Object> result = Nan::New<Object>();
  Nan::Set(result, Nan::New("code").ToLocalChecked(), Nan::New(CPLGetLastErrorNo()));
  Nan::Set(result, Nan::New("message").ToLocalChecked(), Nan::New(CPLGetLastErrorMsg()).ToLocalChecked());
  Nan::Set(result, Nan::New("level").ToLocalChecked(), Nan::New(errtype));
  info.GetReturnValue().Set(result);
}

static NAN_SETTER(LastErrorSetter) {

  if (value->IsNull()) {
    CPLErrorReset();
  } else {
    Nan::ThrowError("'lastError' only supports being set to null");
    return;
  }
}

static NAN_GETTER(EventLoopWarningGetter) {
  info.GetReturnValue().Set(Nan::New<Boolean>(eventLoopWarn));
}

static NAN_SETTER(EventLoopWarningSetter) {
  if (!value->IsBoolean()) {
    Nan::ThrowError("'eventLoopWarning' must be a boolean value");
    return;
  }
  eventLoopWarn = Nan::To<bool>(value).ToChecked();
}

extern "C" {

static NAN_METHOD(QuietOutput) {
  CPLSetErrorHandler(CPLQuietErrorHandler);
  return;
}

static NAN_METHOD(VerboseOutput) {
  CPLSetErrorHandler(CPLDefaultErrorHandler);
  return;
}

#ifdef ENABLE_LOGGING
static NAN_GC_CALLBACK(beforeGC) {
  LOG("%s", "Starting garbage collection");
}

static NAN_GC_CALLBACK(afterGC) {
  LOG("%s", "Finished garbage collection");
}
#endif

static NAN_METHOD(StartLogging) {

#ifdef ENABLE_LOGGING
  std::string filename = "";
  NODE_ARG_STR(0, "filename", filename);
  if (filename.empty()) {
    Nan::ThrowError("Invalid filename");
    return;
  }
  if (log_file) fclose(log_file);
  log_file = fopen(filename.c_str(), "w");
  if (!log_file) {
    Nan::ThrowError("Error creating log file");
    return;
  }

  Nan::AddGCPrologueCallback(beforeGC);
  Nan::AddGCEpilogueCallback(afterGC);

#else
  Nan::ThrowError("Logging requires node-gdal be compiled with --enable_logging=true");
#endif

  return;
}

static NAN_METHOD(StopLogging) {
#ifdef ENABLE_LOGGING
  if (log_file) {
    fclose(log_file);
    log_file = NULL;
  }
#endif

  return;
}

static NAN_METHOD(Log) {
  std::string msg;
  NODE_ARG_STR(0, "message", msg);
  msg = msg + "\n";

#ifdef ENABLE_LOGGING
  if (log_file) {
    fputs(msg.c_str(), log_file);
    fflush(log_file);
  }
#endif

  return;
}

/*
 * Common code for sync and async opening.
 */
// GDAL_ASYNCABLE_GLOBAL(gdal_open);
// GDAL_ASYNCABLE_DEFINE(gdal_open) {
// 
//   std::string path;
//   std::string mode = "r";
// 
//   NODE_ARG_STR(0, "path", path);
//   NODE_ARG_OPT_STR(1, "mode", mode);
// 
//   unsigned int flags = 0;
//   for (unsigned i = 0; i < mode.length(); i++) {
//     if (mode[i] == 'r') {
//       if (i < mode.length() - 1 && mode[i + 1] == '+') {
//         flags |= GDAL_OF_UPDATE;
//         i++;
//       } else {
//         flags |= GDAL_OF_READONLY;
//       }
//     } else if (mode[i] == 'm') {
// #if GDAL_VERSION_MAJOR > 3 || (GDAL_VERSION_MAJOR == 3 && GDAL_VERSION_MINOR >= 1)
//       flags |= GDAL_OF_MULTIDIM_RASTER;
// #else
//       Nan::ThrowError("Multidimensional support requires GDAL 3.1");
// #endif
//     } else if (mode[i] == 't') {
// #if GDAL_VERSION_MAJOR > 3 || (GDAL_VERSION_MAJOR == 3 && GDAL_VERSION_MINOR >= 10)
//       flags |= GDAL_OF_THREAD_SAFE | GDAL_OF_RASTER;
// #else
//       Nan::ThrowError("Thread-safe read-only reading requires GDAL 3.10");
//       return;
// #endif
//     } else {
//       Nan::ThrowError("Invalid open mode. Must contain only \"r\" or \"r+\" and \"m\" or \"t\" ");
//       return;
//     }
//   }
//   flags |= GDAL_OF_VERBOSE_ERROR;
// 
//   GDALAsyncableJob<GDALDataset *> job(0);
//   job.rval = [](GDALDataset *ds, const GetFromPersistentFunc &) { return Dataset::New(ds); };
//   job.main = [path, flags](const GDALExecutionProgress &) {
//     GDALDataset *ds = (GDALDataset *)GDALOpenEx(path.c_str(), flags, NULL, NULL, NULL);
//     if (!ds) throw CPLGetLastErrorMsg();
//     return ds;
//   };
//   job.run(info, async, 2);
// }

static NAN_METHOD(setConfigOption) {

  std::string name;

  NODE_ARG_STR(0, "name", name);

  if (info.Length() < 2) {
    Nan::ThrowError("string or null value must be provided");
    return;
  }
  if (info[1]->IsString()) {
    std::string val = *Nan::Utf8String(info[1]);
    CPLSetConfigOption(name.c_str(), val.c_str());
  } else if (info[1]->IsNull() || info[1]->IsUndefined()) {
    CPLSetConfigOption(name.c_str(), NULL);
  } else {
    Nan::ThrowError("value must be a string or null");
    return;
  }

  return;
}

static NAN_METHOD(getConfigOption) {

  std::string name;
  NODE_ARG_STR(0, "name", name);

  info.GetReturnValue().Set(SafeString::New(CPLGetConfigOption(name.c_str(), NULL)));
}

/**
 * @typedef {string[]|Record<string, string|number|(string|number)[]>} StringOptions
 */

/**
 * Convert decimal degrees to degrees, minutes, and seconds string.
 *
 * @static
 * @method decToDMS
 * @param {number} angle
 * @param {string} axis `"lat"` or `"long"`
 * @param {number} [precision=2]
 * @return {string} A string nndnn'nn.nn'"L where n is a number and L is either
 * N or E
 */
static NAN_METHOD(decToDMS) {

  double angle;
  std::string axis;
  int precision = 2;
  NODE_ARG_DOUBLE(0, "angle", angle);
  NODE_ARG_STR(1, "axis", axis);
  NODE_ARG_INT_OPT(2, "precision", precision);

  if (axis.length() > 0) { axis[0] = toupper(axis[0]); }
  if (axis != "Lat" && axis != "Long") {
    Nan::ThrowError("Axis must be 'lat' or 'long'");
    return;
  }

  info.GetReturnValue().Set(SafeString::New(GDALDecToDMS(angle, axis.c_str(), precision)));
}

/**
 * Set paths where proj will search it data.
 *
 * @static
 * @method setPROJSearchPaths
 * @param {string} path `c:\ProjData`
 */
static NAN_METHOD(setPROJSearchPath) {
  std::string path;

  NODE_ARG_STR(0, "path", path);

#if GDAL_VERSION_MAJOR >= 3
  const char *const paths[] = {path.c_str(), nullptr};
  OSRSetPROJSearchPaths(paths);
#endif
}

static NAN_METHOD(ThrowDummyCPLError) {
  CPLError(CE_Failure, CPLE_AppDefined, "Mock error");
  return;
}

static NAN_METHOD(isAlive) {

  long uid;
  NODE_ARG_INT(0, "uid", uid);

  info.GetReturnValue().Set(Nan::New(object_store.isAlive(uid)));
}

void Cleanup(void *) {
  object_store.cleanup();
}

static void InitNan(Local<Object> target, Local<v8::Value>, void *) {
  static bool initialized = false;
  if (initialized) {
    Nan::ThrowError("gdal-async does not yet support multiple instances per V8 isolate");
    return;
  }
  initialized = true;
  mainV8ThreadId = std::this_thread::get_id();

  // Top-level functions now in N-API (registered before InitNan call)
  // Nan__SetAsyncableMethod(target, "open", gdal_open);
  // Nan::SetMethod(target, "setConfigOption", setConfigOption);
  // Nan::SetMethod(target, "getConfigOption", getConfigOption);
  // Nan::SetMethod(target, "decToDMS", decToDMS);
  // Nan::SetMethod(target, "setPROJSearchPath", setPROJSearchPath);
  // Nan::SetMethod(target, "_triggerCPLError", ThrowDummyCPLError);
  // Nan::SetMethod(target, "_isAlive", isAlive);

//   Warper::Initialize(target);
//   Algorithms::Initialize(target);
#if GDAL_VERSION_MAJOR > 3 || (GDAL_VERSION_MAJOR == 3 && GDAL_VERSION_MINOR >= 12)
//   Algebra::Initialize(target);
#endif

//   Driver::Initialize(target);
  // TODO Phase 7: DriverNapi::Init(env, target) replaces Driver::Initialize
  // N-API bridge will be activated when module switches to NODE_API_MODULE
//   Dataset::Initialize(target);
//   RasterBand::Initialize(target);
#if GDAL_VERSION_MAJOR > 3 || (GDAL_VERSION_MAJOR == 3 && GDAL_VERSION_MINOR >= 1)
//   Group::Initialize(target);
//   MDArray::Initialize(target);
//   Dimension::Initialize(target);
//   Attribute::Initialize(target);
#endif

//   Layer::Initialize(target);
//   Feature::Initialize(target);
//   FeatureDefn::Initialize(target);
//   FieldDefn::Initialize(target);
//   Geometry::Initialize(target);
//   Point::Initialize(target);
//   SimpleCurve::Initialize(target);
//   LineString::Initialize(target);
//   LinearRing::Initialize(target);
//   Polygon::Initialize(target);
//   GeometryCollection::Initialize(target);
//   MultiPoint::Initialize(target);
//   MultiLineString::Initialize(target);
//   MultiPolygon::Initialize(target);
//   CircularString::Initialize(target);
//   CompoundCurve::Initialize(target);
//   MultiCurve::Initialize(target);

//   SpatialReference::Initialize(target);
//   CoordinateTransformation::Initialize(target);
//   ColorTable::Initialize(target);

//   DatasetBands::Initialize(target);
//   DatasetLayers::Initialize(target);
#if GDAL_VERSION_MAJOR > 3 || (GDAL_VERSION_MAJOR == 3 && GDAL_VERSION_MINOR >= 1)
//   GroupGroups::Initialize(target);
//   GroupArrays::Initialize(target);
//   GroupDimensions::Initialize(target);
//   GroupAttributes::Initialize(target);
//   ArrayDimensions::Initialize(target);
//   ArrayAttributes::Initialize(target);
#endif
//   LayerFeatures::Initialize(target);
//   FeatureFields::Initialize(target);
//   LayerFields::Initialize(target);
//   FeatureDefnFields::Initialize(target);
//   GeometryCollectionChildren::Initialize(target);
//   PolygonRings::Initialize(target);
//   LineStringPoints::Initialize(target);
//   CompoundCurveCurves::Initialize(target);
//   RasterBandOverviews::Initialize(target);
//   RasterBandPixels::Initialize(target);
//   Memfile::Initialize(target);
//   Utils::Initialize(target);
//   VSI::Initialize(target);

  /**
   * The collection of all drivers registered with GDAL
   *
   * @readonly
   * @static
   * @constant
   * @name drivers
   * @type {GDALDrivers}
   */
  // GDALDrivers::Initialize(target); // N-API: GDALDriversNapi::Init handles GDALAllRegister
  // gdal.drivers set by GDALDriversNapi::New() in InitNapi()

  /*
   * DMD Constants
   */

  /**
   * @final
   * @constant
   * @type {string}
   * @name DMD_LONGNAME
   */
  Nan::Set(target, Nan::New("DMD_LONGNAME").ToLocalChecked(), Nan::New(GDAL_DMD_LONGNAME).ToLocalChecked());
  /**
   * @final
   * @constant
   * @name DMD_MIMETYPE
   * @type {string}
   */
  Nan::Set(target, Nan::New("DMD_MIMETYPE").ToLocalChecked(), Nan::New(GDAL_DMD_MIMETYPE).ToLocalChecked());
  /**
   * @final
   * @constant
   * @name DMD_HELPTOPIC
   * @type {string}
   */
  Nan::Set(target, Nan::New("DMD_HELPTOPIC").ToLocalChecked(), Nan::New(GDAL_DMD_HELPTOPIC).ToLocalChecked());
  /**
   * @final
   * @constant
   * @name DMD_EXTENSION
   * @type {string}
   */
  Nan::Set(target, Nan::New("DMD_EXTENSION").ToLocalChecked(), Nan::New(GDAL_DMD_EXTENSION).ToLocalChecked());
  /**
   * @final
   * @constant
   * @name DMD_CREATIONOPTIONLIST
   * @type {string}
   */
  Nan::Set(
    target,
    Nan::New("DMD_CREATIONOPTIONLIST").ToLocalChecked(),
    Nan::New(GDAL_DMD_CREATIONOPTIONLIST).ToLocalChecked());
  /**
   * @final
   * @constant
   * @name DMD_CREATIONDATATYPES
   * @type {string}
   */
  Nan::Set(
    target, Nan::New("DMD_CREATIONDATATYPES").ToLocalChecked(), Nan::New(GDAL_DMD_CREATIONDATATYPES).ToLocalChecked());

  /*
   * CE Error levels
   */

  /**
   * Error level: (no error)
   *
   * @final
   * @constant
   * @name CE_None
   * @type {number}
   */
  Nan::Set(target, Nan::New("CE_None").ToLocalChecked(), Nan::New(CE_None));
  /**
   * Error level: Debug
   *
   * @final
   * @constant
   * @name CE_Debug
   * @type {number}
   */
  Nan::Set(target, Nan::New("CE_Debug").ToLocalChecked(), Nan::New(CE_Debug));
  /**
   * Error level: Warning
   *
   * @final
   * @constant
   * @name CE_Warning
   * @type {number}
   */
  Nan::Set(target, Nan::New("CE_Warning").ToLocalChecked(), Nan::New(CE_Warning));
  /**
   * Error level: Failure
   *
   * @final
   * @constant
   * @name CE_Failure
   * @type {number}
   */
  Nan::Set(target, Nan::New("CE_Failure").ToLocalChecked(), Nan::New(CE_Failure));
  /**
   * Error level: Fatal
   *
   * @final
   * @constant
   * @name CE_Fatal
   * @type {number}
   */
  Nan::Set(target, Nan::New("CE_Fatal").ToLocalChecked(), Nan::New(CE_Fatal));

  /*
   * CPL Error codes
   */

  /**
   * @final
   * @constant
   * @name CPLE_None
   * @type {number}
   */
  Nan::Set(target, Nan::New("CPLE_None").ToLocalChecked(), Nan::New(CPLE_None));
  /**
   * @final
   * @constant
   * @name CPLE_AppDefined
   * @type {number}
   */
  Nan::Set(target, Nan::New("CPLE_AppDefined").ToLocalChecked(), Nan::New(CPLE_AppDefined));
  /**
   * @final
   * @constant
   * @name CPLE_OutOfMemory
   * @type {number}
   */
  Nan::Set(target, Nan::New("CPLE_OutOfMemory").ToLocalChecked(), Nan::New(CPLE_OutOfMemory));
  /**
   * @final
   * @constant
   * @name CPLE_FileIO
   * @type {number}
   */
  Nan::Set(target, Nan::New("CPLE_FileIO").ToLocalChecked(), Nan::New(CPLE_FileIO));
  /**
   * @final
   * @constant
   * @name CPLE_OpenFailed
   * @type {number}
   */
  Nan::Set(target, Nan::New("CPLE_OpenFailed").ToLocalChecked(), Nan::New(CPLE_OpenFailed));
  /**
   * @final
   * @constant
   * @name CPLE_IllegalArg
   * @type {number}
   */
  Nan::Set(target, Nan::New("CPLE_IllegalArg").ToLocalChecked(), Nan::New(CPLE_IllegalArg));
  /**
   * @final
   * @constant
   * @name CPLE_NotSupported
   * @type {number}
   */
  Nan::Set(target, Nan::New("CPLE_NotSupported").ToLocalChecked(), Nan::New(CPLE_NotSupported));
  /**
   * @final
   * @constant
   * @name CPLE_AssertionFailed
   * @type {number}
   */
  Nan::Set(target, Nan::New("CPLE_AssertionFailed").ToLocalChecked(), Nan::New(CPLE_AssertionFailed));
  /**
   * @final
   * @constant
   * @name CPLE_NoWriteAccess
   * @type {number}
   */
  Nan::Set(target, Nan::New("CPLE_NoWriteAccess").ToLocalChecked(), Nan::New(CPLE_NoWriteAccess));
  /**
   * @final
   * @constant
   * @name CPLE_UserInterrupt
   * @type {number}
   */
  Nan::Set(target, Nan::New("CPLE_UserInterrupt").ToLocalChecked(), Nan::New(CPLE_UserInterrupt));
  /**
   * @final
   * @constant
   * @name CPLE_objectNull
   * @type {number}
   */
  Nan::Set(target, Nan::New("CPLE_ObjectNull").ToLocalChecked(), Nan::New(CPLE_ObjectNull));

  /*
   * Driver Dataset creation constants
   */

  /**
   * @final
   * @constant
   * @name DCAP_CREATE
   * @type {string}
   */
  Nan::Set(target, Nan::New("DCAP_CREATE").ToLocalChecked(), Nan::New(GDAL_DCAP_CREATE).ToLocalChecked());
  /**
   * @final
   * @constant
   * @name DCAP_CREATECOPY
   * @type {string}
   */
  Nan::Set(target, Nan::New("DCAP_CREATECOPY").ToLocalChecked(), Nan::New(GDAL_DCAP_CREATECOPY).ToLocalChecked());
  /**
   * @final
   * @constant
   * @name DCAP_VIRTUALIO
   * @type {string}
   */
  Nan::Set(target, Nan::New("DCAP_VIRTUALIO").ToLocalChecked(), Nan::New(GDAL_DCAP_VIRTUALIO).ToLocalChecked());

  /*
   * OLC Constants
   */

  /**
   * @final
   * @constant
   * @name OLCRandomRead
   * @type {string}
   */
  Nan::Set(target, Nan::New("OLCRandomRead").ToLocalChecked(), Nan::New(OLCRandomRead).ToLocalChecked());
  /**
   * @final
   * @constant
   * @name OLCSequentialWrite
   * @type {string}
   */
  Nan::Set(target, Nan::New("OLCSequentialWrite").ToLocalChecked(), Nan::New(OLCSequentialWrite).ToLocalChecked());
  /**
   * @final
   * @constant
   * @name OLCRandomWrite
   * @type {string}
   */
  Nan::Set(target, Nan::New("OLCRandomWrite").ToLocalChecked(), Nan::New(OLCRandomWrite).ToLocalChecked());
  /**
   * @final
   * @constant
   * @name OLCFastSpatialFilter
   * @type {string}
   */
  Nan::Set(target, Nan::New("OLCFastSpatialFilter").ToLocalChecked(), Nan::New(OLCFastSpatialFilter).ToLocalChecked());
  /**
   * @final
   * @constant
   * @name OLCFastFeatureCount
   * @type {string}
   */
  Nan::Set(target, Nan::New("OLCFastFeatureCount").ToLocalChecked(), Nan::New(OLCFastFeatureCount).ToLocalChecked());
  /**
   * @final
   * @constant
   * @name OLCFastGetExtent
   * @type {string}
   */
  Nan::Set(target, Nan::New("OLCFastGetExtent").ToLocalChecked(), Nan::New(OLCFastGetExtent).ToLocalChecked());
  /**
   * @final
   * @constant
   * @name OLCCreateField
   * @type {string}
   */
  Nan::Set(target, Nan::New("OLCCreateField").ToLocalChecked(), Nan::New(OLCCreateField).ToLocalChecked());
  /**
   * @final
   * @constant
   * @name OLCDeleteField
   * @type {string}
   */
  Nan::Set(target, Nan::New("OLCDeleteField").ToLocalChecked(), Nan::New(OLCDeleteField).ToLocalChecked());
  /**
   * @final
   * @constant
   * @name OLCReorderFields
   * @type {string}
   */
  Nan::Set(target, Nan::New("OLCReorderFields").ToLocalChecked(), Nan::New(OLCReorderFields).ToLocalChecked());
  /**
   * @final
   * @constant
   * @name OLCAlterFieldDefn
   * @type {string}
   */
  Nan::Set(target, Nan::New("OLCAlterFieldDefn").ToLocalChecked(), Nan::New(OLCAlterFieldDefn).ToLocalChecked());
  /**
   * @final
   * @constant
   * @name OLCTransactions
   * @type {string}
   */
  Nan::Set(target, Nan::New("OLCTransactions").ToLocalChecked(), Nan::New(OLCTransactions).ToLocalChecked());
  /**
   * @final
   * @constant
   * @name OLCDeleteFeature
   * @type {string}
   */
  Nan::Set(target, Nan::New("OLCDeleteFeature").ToLocalChecked(), Nan::New(OLCDeleteFeature).ToLocalChecked());
  /**
   * @final
   * @constant
   * @name OLCFastSetNextByIndex
   * @type {string}
   */
  Nan::Set(
    target, Nan::New("OLCFastSetNextByIndex").ToLocalChecked(), Nan::New(OLCFastSetNextByIndex).ToLocalChecked());
  /**
   * @final
   * @constant
   * @name OLCStringsAsUTF8
   * @type {string}
   */
  Nan::Set(target, Nan::New("OLCStringsAsUTF8").ToLocalChecked(), Nan::New(OLCStringsAsUTF8).ToLocalChecked());
  /**
   * @final
   * @constant
   * @name OLCIgnoreFields
   * @type {string}
   */
  Nan::Set(target, Nan::New("OLCIgnoreFields").ToLocalChecked(), Nan::New(OLCIgnoreFields).ToLocalChecked());

#ifdef OLCCreateGeomField
  /**
   * @final
   * @constant
   * @name OLCCreateGeomField
   * @type {string}
   */
  Nan::Set(target, Nan::New("OLCCreateGeomField").ToLocalChecked(), Nan::New(OLCCreateGeomField).ToLocalChecked());
#endif
#ifdef ODsCCreateGeomFieldAfterCreateLayer

  /*
   * ODsC constants
   */

  /**
   * @final
   * @constant
   * @name ODsCCreateLayer
   * @type {string}
   */
  Nan::Set(target, Nan::New("ODsCCreateLayer").ToLocalChecked(), Nan::New(ODsCCreateLayer).ToLocalChecked());
  /**
   * @final
   * @constant
   * @name ODsCDeleteLayer
   * @type {string}
   */
  Nan::Set(target, Nan::New("ODsCDeleteLayer").ToLocalChecked(), Nan::New(ODsCDeleteLayer).ToLocalChecked());
  /**
   * @final
   * @constant
   * @name ODsCCreateGeomFieldAfterCreateLayer
   * @type {string}
   */
  Nan::Set(
    target,
    Nan::New("ODsCCreateGeomFieldAfterCreateLayer").ToLocalChecked(),
    Nan::New(ODsCCreateGeomFieldAfterCreateLayer).ToLocalChecked());
#endif
  /**
   * @final
   * @constant
   * @name ODrCCreateDataSource
   * @type {string}
   */
  Nan::Set(target, Nan::New("ODrCCreateDataSource").ToLocalChecked(), Nan::New(ODrCCreateDataSource).ToLocalChecked());
  /**
   * @final
   * @constant
   * @name ODrCDeleteDataSource
   * @type {string}
   */
  Nan::Set(target, Nan::New("ODrCDeleteDataSource").ToLocalChecked(), Nan::New(ODrCDeleteDataSource).ToLocalChecked());

  /*
   * open flags
   */

  /**
   * @final
   * @constant
   * @name GA_Readonly
   * @type {number}
   */
  NODE_DEFINE_CONSTANT(target, GA_ReadOnly);

  /**
   * @final
   * @constant
   * @name GA_Update
   * @type {number}
   */
  NODE_DEFINE_CONSTANT(target, GA_Update);

  /*
   * RasterIO flags
   */

  /**
   * @final
   * @constant
   * @name GF_Read
   * @type {number}
   */
  NODE_DEFINE_CONSTANT(target, GF_Read);

  /**
   * @final
   * @constant
   * @name GF_Write
   * @type {number}
   */
  NODE_DEFINE_CONSTANT(target, GF_Write);

  /*
   * Pixel data types.
   */

  /**
   * Unknown or unspecified type
   * @final
   * @constant
   * @name GDT_Unknown
   * @type {string}
   */
  Nan::Set(target, Nan::New("GDT_Unknown").ToLocalChecked(), Nan::Undefined());
  /**
   * Eight bit unsigned integer
   * @final
   * @constant
   * @name GDT_Byte
   * @type {string}
   */
  Nan::Set(target, Nan::New("GDT_Byte").ToLocalChecked(), Nan::New(GDALGetDataTypeName(GDT_Byte)).ToLocalChecked());
  /**
   * Sixteen bit unsigned integer
   * @final
   * @constant
   * @name GDT_UInt16
   * @type {string}
   */
  Nan::Set(target, Nan::New("GDT_UInt16").ToLocalChecked(), Nan::New(GDALGetDataTypeName(GDT_UInt16)).ToLocalChecked());
  /**
   * Sixteen bit signed integer
   * @final
   * @constant
   * @name GDT_Int16
   * @type {string}
   */
  Nan::Set(target, Nan::New("GDT_Int16").ToLocalChecked(), Nan::New(GDALGetDataTypeName(GDT_Int16)).ToLocalChecked());
  /**
   * Thirty two bit unsigned integer
   * @final
   * @constant
   * @name GDT_UInt32
   * @type {string}
   */
  Nan::Set(target, Nan::New("GDT_UInt32").ToLocalChecked(), Nan::New(GDALGetDataTypeName(GDT_UInt32)).ToLocalChecked());
  /**
   * Thirty two bit signed integer
   * @final
   * @constant
   * @name GDT_Int32
   * @type {string}
   */
  Nan::Set(target, Nan::New("GDT_Int32").ToLocalChecked(), Nan::New(GDALGetDataTypeName(GDT_Int32)).ToLocalChecked());
#if GDAL_VERSION_MAJOR > 3 || (GDAL_VERSION_MAJOR == 3 && GDAL_VERSION_MINOR >= 5)
  /**
   * Sixty four bit signed integer
   * @final
   * @constant
   * @name GDT_Int64
   * @type {string}
   */
  Nan::Set(target, Nan::New("GDT_Int64").ToLocalChecked(), Nan::New(GDALGetDataTypeName(GDT_Int64)).ToLocalChecked());
  /**
   * Sixty four bit unsigned integer
   * @final
   * @constant
   * @name GDT_UInt64
   * @type {string}
   */
  Nan::Set(target, Nan::New("GDT_UInt64").ToLocalChecked(), Nan::New(GDALGetDataTypeName(GDT_UInt64)).ToLocalChecked());
#endif
#if GDAL_VERSION_MAJOR > 3 || (GDAL_VERSION_MAJOR == 3 && GDAL_VERSION_MINOR >= 11)
  /**
   * Sixteen bit floating point
   * @final
   * @constant
   * @name GDT_Float16
   * @type {string}
   */
  Nan::Set(
    target, Nan::New("GDT_Float16").ToLocalChecked(), Nan::New(GDALGetDataTypeName(GDT_Float16)).ToLocalChecked());
#endif
  /**
   * Thirty two bit floating point
   * @final
   * @constant
   * @name GDT_Float32
   * @type {string}
   */
  Nan::Set(
    target, Nan::New("GDT_Float32").ToLocalChecked(), Nan::New(GDALGetDataTypeName(GDT_Float32)).ToLocalChecked());
  /**
   * Sixty four bit floating point
   * @final
   * @constant
   * @name GDT_Float64
   * @type {string}
   */
  Nan::Set(
    target, Nan::New("GDT_Float64").ToLocalChecked(), Nan::New(GDALGetDataTypeName(GDT_Float64)).ToLocalChecked());
  /**
   * Complex Int16
   * @final
   * @constant
   * @name GDT_CInt16
   * @type {string}
   */
  Nan::Set(target, Nan::New("GDT_CInt16").ToLocalChecked(), Nan::New(GDALGetDataTypeName(GDT_CInt16)).ToLocalChecked());
  /**
   * Complex Int32
   * @final
   * @constant
   * @name GDT_CInt32
   * @type {string}
   */
  Nan::Set(target, Nan::New("GDT_CInt32").ToLocalChecked(), Nan::New(GDALGetDataTypeName(GDT_CInt32)).ToLocalChecked());
#if GDAL_VERSION_MAJOR > 3 || (GDAL_VERSION_MAJOR == 3 && GDAL_VERSION_MINOR >= 11)
  /**
   * Complex Float16
   * @final
   * @constant
   * @name GDT_CFloat16
   * @type {string}
   */
  Nan::Set(
    target, Nan::New("GDT_CFloat16").ToLocalChecked(), Nan::New(GDALGetDataTypeName(GDT_CFloat16)).ToLocalChecked());
#endif
  /**
   * Complex Float32
   * @final
   * @constant
   * @name GDT_CFloat32
   * @type {string}
   */
  Nan::Set(
    target, Nan::New("GDT_CFloat32").ToLocalChecked(), Nan::New(GDALGetDataTypeName(GDT_CFloat32)).ToLocalChecked());
  /**
   * Complex Float64
   * @final
   * @constant
   * @name GDT_CFloat64
   * @type {string}
   */
  Nan::Set(
    target, Nan::New("GDT_CFloat64").ToLocalChecked(), Nan::New(GDALGetDataTypeName(GDT_CFloat64)).ToLocalChecked());

#if GDAL_VERSION_MAJOR > 3 || (GDAL_VERSION_MAJOR == 3 && GDAL_VERSION_MINOR >= 1)
  /**
   * String extended type for MDArrays (GDAL >= 3.1)
   * @final
   * @constant
   * @name GEDTC_String
   * @type {string}
   */
  Nan::Set(target, Nan::New("GEDTC_String").ToLocalChecked(), Nan::New("String").ToLocalChecked());

  /**
   * String extended type for MDArrays (GDAL >= 3.1)
   * @final
   * @constant
   * @name GEDTC_Compound
   * @type {string}
   */
  Nan::Set(target, Nan::New("GEDTC_Compound").ToLocalChecked(), Nan::New("Compound").ToLocalChecked());
#endif

  /*
   * Justification
   */

  /**
   * @final
   * @constant
   * @name OJUndefined
   * @type {string}
   */
  Nan::Set(target, Nan::New("OJUndefined").ToLocalChecked(), Nan::Undefined());
  /**
   * @final
   * @constant
   * @name OJLeft
   * @type {string}
   */
  Nan::Set(target, Nan::New("OJLeft").ToLocalChecked(), Nan::New("Left").ToLocalChecked());
  /**
   * @final
   * @constant
   * @name OJRight
   * @type {string}
   */
  Nan::Set(target, Nan::New("OJRight").ToLocalChecked(), Nan::New("Right").ToLocalChecked());

  /*
   * Color interpretation constants
   */

  /**
   * @final
   * @constant
   * @name GCI_Undefined
   * @type {string}
   */
  Nan::Set(target, Nan::New("GCI_Undefined").ToLocalChecked(), Nan::Undefined());
  /**
   * @final
   * @constant
   * @name GCI_GrayIndex
   * @type {string}
   */
  Nan::Set(
    target,
    Nan::New("GCI_GrayIndex").ToLocalChecked(),
    Nan::New(GDALGetColorInterpretationName(GCI_GrayIndex)).ToLocalChecked());
  /**
   * @final
   * @constant
   * @name GCI_PaletteIndex
   * @type {string}
   */
  Nan::Set(
    target,
    Nan::New("GCI_PaletteIndex").ToLocalChecked(),
    Nan::New(GDALGetColorInterpretationName(GCI_PaletteIndex)).ToLocalChecked());
  /**
   * @final
   * @constant
   * @name GCI_RedBand
   * @type {string}
   */
  Nan::Set(
    target,
    Nan::New("GCI_RedBand").ToLocalChecked(),
    Nan::New(GDALGetColorInterpretationName(GCI_RedBand)).ToLocalChecked());
  /**
   * @final
   * @constant
   * @name GCI_GreenBand
   * @type {string}
   */
  Nan::Set(
    target,
    Nan::New("GCI_GreenBand").ToLocalChecked(),
    Nan::New(GDALGetColorInterpretationName(GCI_GreenBand)).ToLocalChecked());
  /**
   * @final
   * @constant
   * @name GCI_BlueBand
   * @type {string}
   */
  Nan::Set(
    target,
    Nan::New("GCI_BlueBand").ToLocalChecked(),
    Nan::New(GDALGetColorInterpretationName(GCI_BlueBand)).ToLocalChecked());
  /**
   * @final
   * @constant
   * @name GCI_AlphaBand
   * @type {string}
   */
  Nan::Set(
    target,
    Nan::New("GCI_AlphaBand").ToLocalChecked(),
    Nan::New(GDALGetColorInterpretationName(GCI_AlphaBand)).ToLocalChecked());
  /**
   * @final
   * @constant
   * @name GCI_HueBand
   * @type {string}
   */
  Nan::Set(
    target,
    Nan::New("GCI_HueBand").ToLocalChecked(),
    Nan::New(GDALGetColorInterpretationName(GCI_HueBand)).ToLocalChecked());
  /**
   * @final
   * @constant
   * @name GCI_SaturationBand
   * @type {string}
   */
  Nan::Set(
    target,
    Nan::New("GCI_SaturationBand").ToLocalChecked(),
    Nan::New(GDALGetColorInterpretationName(GCI_SaturationBand)).ToLocalChecked());
  /**
   * @final
   * @constant
   * @name GCI_LightnessBand
   * @type {string}
   */
  Nan::Set(
    target,
    Nan::New("GCI_LightnessBand").ToLocalChecked(),
    Nan::New(GDALGetColorInterpretationName(GCI_LightnessBand)).ToLocalChecked());
  /**
   * @final
   * @constant
   * @name GCI_CyanBand
   * @type {string}
   */
  Nan::Set(
    target,
    Nan::New("GCI_CyanBand").ToLocalChecked(),
    Nan::New(GDALGetColorInterpretationName(GCI_CyanBand)).ToLocalChecked());
  /**
   * @final
   * @constant
   * @name GCI_MagentaBand
   * @type {string}
   */
  Nan::Set(
    target,
    Nan::New("GCI_MagentaBand").ToLocalChecked(),
    Nan::New(GDALGetColorInterpretationName(GCI_MagentaBand)).ToLocalChecked());
  /**
   * @final
   * @constant
   * @name GCI_YellowBand
   * @type {string}
   */
  Nan::Set(
    target,
    Nan::New("GCI_YellowBand").ToLocalChecked(),
    Nan::New(GDALGetColorInterpretationName(GCI_YellowBand)).ToLocalChecked());
  /**
   * @final
   * @constant
   * @name GCI_BlackBand
   * @type {string}
   */
  Nan::Set(
    target,
    Nan::New("GCI_BlackBand").ToLocalChecked(),
    Nan::New(GDALGetColorInterpretationName(GCI_BlackBand)).ToLocalChecked());
  /**
   * @final
   * @constant
   * @name GCI_YCbCr_YBand
   * @type {string}
   */
  Nan::Set(
    target,
    Nan::New("GCI_YCbCr_YBand").ToLocalChecked(),
    Nan::New(GDALGetColorInterpretationName(GCI_YCbCr_YBand)).ToLocalChecked());
  /**
   * @final
   * @constant
   * @name GCI_YCbCr_CbBand
   * @type {string}
   */
  Nan::Set(
    target,
    Nan::New("GCI_YCbCr_CbBand").ToLocalChecked(),
    Nan::New(GDALGetColorInterpretationName(GCI_YCbCr_CbBand)).ToLocalChecked());
  /**
   * @final
   * @constant
   * @name GCI_YCbCr_CrBand
   * @type {string}
   */
  Nan::Set(
    target,
    Nan::New("GCI_YCbCr_CrBand").ToLocalChecked(),
    Nan::New(GDALGetColorInterpretationName(GCI_YCbCr_CrBand)).ToLocalChecked());

  /*
   * Palette types.
   */

  /**
   * Grayscale, only c1 defined
   * @final
   * @constant
   * @name GPI_Gray
   * @type {string}
   */
  Nan::Set(target, Nan::New("GPI_Gray").ToLocalChecked(), Nan::New("Gray").ToLocalChecked());

  /**
   * RGBA, alpha in c4
   * @final
   * @constant
   * @name GPI_RGB
   * @type {string}
   */
  Nan::Set(target, Nan::New("GPI_RGB").ToLocalChecked(), Nan::New("RGB").ToLocalChecked());

  /**
   * CMYK
   * @final
   * @constant
   * @name GPI_CMYK
   * @type {string}
   */
  Nan::Set(target, Nan::New("GPI_CMYK").ToLocalChecked(), Nan::New("CMYK").ToLocalChecked());

  /**
   * HLS, c4 is not defined
   * @final
   * @constant
   * @name GPI_HLS
   * @type {string}
   */
  Nan::Set(target, Nan::New("GPI_HLS").ToLocalChecked(), Nan::New("HLS").ToLocalChecked());

  /*
   * WKB Variants
   */

  /**
   * Old-style 99-402 extended dimension (Z) WKB types.
   * Synonymous with 'wkbVariantOldOgc' (gdal >= 2.0)
   *
   * @final
   * @constant
   * @name wkbVariantOgc
   * @type {string}
   */
  Nan::Set(target, Nan::New("wkbVariantOgc").ToLocalChecked(), Nan::New("OGC").ToLocalChecked());

  /**
   * Old-style 99-402 extended dimension (Z) WKB types.
   * Synonymous with 'wkbVariantOgc' (gdal < 2.0)
   *
   * @final
   * @constant
   * @name wkbVariantOldOgc
   * @type {string}
   */
  Nan::Set(target, Nan::New("wkbVariantOldOgc").ToLocalChecked(), Nan::New("OGC").ToLocalChecked());

  /**
   * SFSQL 1.2 and ISO SQL/MM Part 3 extended dimension (Z&M) WKB types.
   *
   * @final
   * @constant
   * @name wkbVariantIso
   * @type {string}
   */
  Nan::Set(target, Nan::New("wkbVariantIso").ToLocalChecked(), Nan::New("ISO").ToLocalChecked());

  /*
   * WKB Byte Ordering
   */

  /**
   * @final
   * @constant
   * @name wkbXDR
   * @type {string}
   */
  Nan::Set(target, Nan::New("wkbXDR").ToLocalChecked(), Nan::New("MSB").ToLocalChecked());
  /**
   * @final
   * @constant
   * @name wkbNDR
   * @type {string}
   */
  Nan::Set(target, Nan::New("wkbNDR").ToLocalChecked(), Nan::New("LSB").ToLocalChecked());

  /*
   * WKB Geometry Types
   */

  /**
   * @final
   *
   * The `wkb25DBit` constant can be used to convert between 2D types to 2.5D
   * types
   *
   * @example
   *
   * // 2 -> 2.5D
   * wkbPoint25D = gdal.wkbPoint | gdal.wkb25DBit
   *
   * // 2.5D -> 2D (same as wkbFlatten())
   * wkbPoint = gdal.wkbPoint25D & (~gdal.wkb25DBit)
   *
   * @constant
   * @name wkb25DBit
   * @type {number}
   */
  Nan::Set(target, Nan::New("wkb25DBit").ToLocalChecked(), Nan::New<Integer>(wkb25DBit));

  int wkbLinearRing25D = wkbLinearRing | wkb25DBit;

  /**
   * @final
   * @constant
   * @name wkbUnknown
   * @type {number}
   */
  Nan::Set(target, Nan::New("wkbUnknown").ToLocalChecked(), Nan::New<Integer>(wkbUnknown));
  /**
   * @final
   * @constant
   * @name wkbPoint
   * @type {number}
   */
  Nan::Set(target, Nan::New("wkbPoint").ToLocalChecked(), Nan::New<Integer>(wkbPoint));
  /**
   * @final
   * @constant
   * @name wkbLineString
   * @type {number}
   */
  Nan::Set(target, Nan::New("wkbLineString").ToLocalChecked(), Nan::New<Integer>(wkbLineString));
  /**
   * @final
   * @constant
   * @name wkbCircularString
   * @type {number}
   */
  Nan::Set(target, Nan::New("wkbCircularString").ToLocalChecked(), Nan::New<Integer>(wkbCircularString));
  /**
   * @final
   * @constant
   * @name wkbCompoundCurve
   * @type {number}
   */
  Nan::Set(target, Nan::New("wkbCompoundCurve").ToLocalChecked(), Nan::New<Integer>(wkbCompoundCurve));
  /**
   * @final
   * @constant
   * @name wkbMultiCurve
   * @type {number}
   */
  Nan::Set(target, Nan::New("wkbMultiCurve").ToLocalChecked(), Nan::New<Integer>(wkbMultiCurve));
  /**
   * @final
   * @constant
   * @name wkbPolygon
   * @type {number}
   */
  Nan::Set(target, Nan::New("wkbPolygon").ToLocalChecked(), Nan::New<Integer>(wkbPolygon));
  /**
   * @final
   * @constant
   * @name wkbMultiPoint
   * @type {number}
   */
  Nan::Set(target, Nan::New("wkbMultiPoint").ToLocalChecked(), Nan::New<Integer>(wkbMultiPoint));
  /**
   * @final
   * @constant
   * @name wkbMultiLineString
   * @type {number}
   */
  Nan::Set(target, Nan::New("wkbMultiLineString").ToLocalChecked(), Nan::New<Integer>(wkbMultiLineString));
  /**
   * @final
   * @constant
   * @name wkbMultiPolygon
   * @type {number}
   */
  Nan::Set(target, Nan::New("wkbMultiPolygon").ToLocalChecked(), Nan::New<Integer>(wkbMultiPolygon));
  /**
   * @final
   * @constant
   * @name wkbGeometryCollection
   * @type {number}
   */
  Nan::Set(target, Nan::New("wkbGeometryCollection").ToLocalChecked(), Nan::New<Integer>(wkbGeometryCollection));
  /**
   * @final
   * @constant
   * @name wkbNone
   * @type {number}
   */
  Nan::Set(target, Nan::New("wkbNone").ToLocalChecked(), Nan::New<Integer>(wkbNone));
  /**
   * @final
   * @constant
   * @name wkbLinearRing
   * @type {string}
   */
  Nan::Set(target, Nan::New("wkbLinearRing").ToLocalChecked(), Nan::New<Integer>(wkbLinearRing));
  /**
   * @final
   * @constant
   * @name wkbPoint25D
   * @type {number}
   */
  Nan::Set(target, Nan::New("wkbPoint25D").ToLocalChecked(), Nan::New<Integer>(wkbPoint25D));
  /**
   * @final
   * @constant
   * @name wkbLineString25D
   * @type {number}
   */
  Nan::Set(target, Nan::New("wkbLineString25D").ToLocalChecked(), Nan::New<Integer>(wkbLineString25D));
  /**
   * @final
   * @constant
   * @name wkbPolygon25D
   * @type {number}
   */
  Nan::Set(target, Nan::New("wkbPolygon25D").ToLocalChecked(), Nan::New<Integer>(wkbPolygon25D));
  /**
   * @final
   * @constant
   * @name wkbMultiPoint25D
   * @type {number}
   */
  Nan::Set(target, Nan::New("wkbMultiPoint25D").ToLocalChecked(), Nan::New<Integer>(wkbMultiPoint25D));
  /**
   * @final
   * @constant
   * @name wkbMultiLineString25D
   * @type {number}
   */
  Nan::Set(target, Nan::New("wkbMultiLineString25D").ToLocalChecked(), Nan::New<Integer>(wkbMultiLineString25D));
  /**
   * @final
   * @constant
   * @name wkbMultiPolygon25D
   * @type {number}
   */
  Nan::Set(target, Nan::New("wkbMultiPolygon25D").ToLocalChecked(), Nan::New<Integer>(wkbMultiPolygon25D));
  /**
   * @final
   * @constant
   * @name wkbGeometryCollection25D
   * @type {number}
   */
  Nan::Set(target, Nan::New("wkbGeometryCollection25D").ToLocalChecked(), Nan::New<Integer>(wkbGeometryCollection25D));
  /**
   * @final
   * @constant
   * @name wkbLinearRing25D
   * @type {number}
   */
  Nan::Set(target, Nan::New("wkbLinearRing25D").ToLocalChecked(), Nan::New<Integer>(wkbLinearRing25D));

  /*
   * Field types
   */

  /**
   * @final
   * @constant
   * @name OFTInteger
   * @type {string}
   */
  Nan::Set(target, Nan::New("OFTInteger").ToLocalChecked(), Nan::New(getFieldTypeName(OFTInteger)).ToLocalChecked());
  /**
   * @final
   * @constant
   * @name OFTIntegerList
   * @type {string}
   */
  Nan::Set(
    target, Nan::New("OFTIntegerList").ToLocalChecked(), Nan::New(getFieldTypeName(OFTIntegerList)).ToLocalChecked());

  /**
   * @final
   * @constant
   * @name OFTInteger64
   * @type {string}
   */
  Nan::Set(
    target, Nan::New("OFTInteger64").ToLocalChecked(), Nan::New(getFieldTypeName(OFTInteger64)).ToLocalChecked());
  /**
   * @final
   * @constant
   * @name OFTInteger64List
   * @type {string}
   */
  Nan::Set(
    target,
    Nan::New("OFTInteger64List").ToLocalChecked(),
    Nan::New(getFieldTypeName(OFTInteger64List)).ToLocalChecked());
  /**
   * @final
   * @constant
   * @name OFTReal
   * @type {string}
   */
  Nan::Set(target, Nan::New("OFTReal").ToLocalChecked(), Nan::New(getFieldTypeName(OFTReal)).ToLocalChecked());
  /**
   * @final
   * @constant
   * @name OFTRealList
   * @type {string}
   */
  Nan::Set(target, Nan::New("OFTRealList").ToLocalChecked(), Nan::New(getFieldTypeName(OFTRealList)).ToLocalChecked());
  /**
   * @final
   * @constant
   * @name OFTString
   * @type {string}
   */
  Nan::Set(target, Nan::New("OFTString").ToLocalChecked(), Nan::New(getFieldTypeName(OFTString)).ToLocalChecked());
  /**
   * @final
   * @constant
   * @name OFTStringList
   * @type {string}
   */
  Nan::Set(
    target, Nan::New("OFTStringList").ToLocalChecked(), Nan::New(getFieldTypeName(OFTStringList)).ToLocalChecked());
  /**
   * @final
   * @constant
   * @name OFTWideString
   * @type {string}
   */
  Nan::Set(
    target, Nan::New("OFTWideString").ToLocalChecked(), Nan::New(getFieldTypeName(OFTWideString)).ToLocalChecked());
  /**
   * @final
   * @constant
   * @name OFTWideStringList
   * @type {string}
   */
  Nan::Set(
    target,
    Nan::New("OFTWideStringList").ToLocalChecked(),
    Nan::New(getFieldTypeName(OFTWideStringList)).ToLocalChecked());
  /**
   * @final
   * @constant
   * @name OFTBinary
   * @type {string}
   */
  Nan::Set(target, Nan::New("OFTBinary").ToLocalChecked(), Nan::New(getFieldTypeName(OFTBinary)).ToLocalChecked());
  /**
   * @final
   * @constant
   * @name OFTDate
   * @type {string}
   */
  Nan::Set(target, Nan::New("OFTDate").ToLocalChecked(), Nan::New(getFieldTypeName(OFTDate)).ToLocalChecked());
  /**
   * @final
   * @constant
   * @name OFTTime
   * @type {string}
   */
  Nan::Set(target, Nan::New("OFTTime").ToLocalChecked(), Nan::New(getFieldTypeName(OFTTime)).ToLocalChecked());
  /**
   * @final
   * @constant
   * @name OFTDateTime
   * @type {string}
   */
  Nan::Set(target, Nan::New("OFTDateTime").ToLocalChecked(), Nan::New(getFieldTypeName(OFTDateTime)).ToLocalChecked());

  /*
   * Resampling options that can be used with the gdal.reprojectImage() and gdal.RasterBandPixels.read methods.
   */

  /**
   * @final
   * @constant
   * @name GRA_NearestNeighbor
   * @type {string}
   */
  Nan::Set(target, Nan::New("GRA_NearestNeighbor").ToLocalChecked(), Nan::New("NearestNeighbor").ToLocalChecked());
  /**
   * @final
   * @constant
   * @name GRA_Bilinear
   * @type {string}
   */
  Nan::Set(target, Nan::New("GRA_Bilinear").ToLocalChecked(), Nan::New("Bilinear").ToLocalChecked());
  /**
   * @final
   * @constant
   * @name GRA_Cubic
   * @type {string}
   */
  Nan::Set(target, Nan::New("GRA_Cubic").ToLocalChecked(), Nan::New("Cubic").ToLocalChecked());
  /**
   * @final
   * @constant
   * @name GRA_CubicSpline
   * @type {string}
   */
  Nan::Set(target, Nan::New("GRA_CubicSpline").ToLocalChecked(), Nan::New("CubicSpline").ToLocalChecked());
  /**
   * @final
   * @constant
   * @name GRA_Lanczos
   * @type {string}
   */
  Nan::Set(target, Nan::New("GRA_Lanczos").ToLocalChecked(), Nan::New("Lanczos").ToLocalChecked());
  /**
   * @final
   * @constant
   * @name GRA_Average
   * @type {string}
   */
  Nan::Set(target, Nan::New("GRA_Average").ToLocalChecked(), Nan::New("Average").ToLocalChecked());
  /**
   * @final
   * @constant
   * @name GRA_Mode
   * @type {string}
   */
  Nan::Set(target, Nan::New("GRA_Mode").ToLocalChecked(), Nan::New("Mode").ToLocalChecked());

#if GDAL_VERSION_MAJOR > 3 || (GDAL_VERSION_MAJOR == 3 && GDAL_VERSION_MINOR >= 1)
  /*
   * Dimension types for gdal.Dimension (GDAL >= 3.3)
   */

  /**
   * @final
   * @constant
   * @name DIM_HORIZONTAL_X
   * @type {string}
   */
  Nan::Set(
    target, Nan::New("DIM_HORIZONTAL_X").ToLocalChecked(), Nan::New(GDAL_DIM_TYPE_HORIZONTAL_X).ToLocalChecked());

  /**
   * @final
   * @constant
   * @name DIM_HORIZONTAL_Y
   * @type {string}
   */
  Nan::Set(
    target, Nan::New("DIM_HORIZONTAL_Y").ToLocalChecked(), Nan::New(GDAL_DIM_TYPE_HORIZONTAL_Y).ToLocalChecked());

  /**
   * @final
   * @constant
   * @name DIM_VERTICAL
   * @type {string}
   */
  Nan::Set(target, Nan::New("DIM_VERTICAL").ToLocalChecked(), Nan::New(GDAL_DIM_TYPE_VERTICAL).ToLocalChecked());

  /**
   * @final
   * @constant
   * @name DIM_TEMPORAL
   * @type {string}
   */
  Nan::Set(target, Nan::New("DIM_TEMPORAL").ToLocalChecked(), Nan::New(GDAL_DIM_TYPE_TEMPORAL).ToLocalChecked());

  /**
   * @final
   * @constant
   * @name DIM_PARAMETRIC
   * @type {string}
   */
  Nan::Set(target, Nan::New("DIM_PARAMETRIC").ToLocalChecked(), Nan::New(GDAL_DIM_TYPE_PARAMETRIC).ToLocalChecked());
#endif

  /*
   * Direction types for gdal.Dimension (GDAL >= 3.3)
   */

  /**
   * @final
   * @constant
   * @name DIR_EAST
   * @type {string}
   */
  Nan::Set(target, Nan::New("DIR_EAST").ToLocalChecked(), Nan::New("EAST").ToLocalChecked());

  /**
   * @final
   * @constant
   * @name DIR_WEST
   * @type {string}
   */
  Nan::Set(target, Nan::New("DIR_WEST").ToLocalChecked(), Nan::New("WEST").ToLocalChecked());

  /**
   * @final
   * @constant
   * @name DIR_SOUTH
   * @type {string}
   */
  Nan::Set(target, Nan::New("DIR_SOUTH").ToLocalChecked(), Nan::New("SOUTH").ToLocalChecked());

  /**
   * @final
   * @constant
   * @name DIR_NORTH
   * @type {string}
   */
  Nan::Set(target, Nan::New("DIR_NORTH").ToLocalChecked(), Nan::New("NORTH").ToLocalChecked());

  /**
   * @final
   * @constant
   * @name DIR_UP
   * @type {string}
   */
  Nan::Set(target, Nan::New("DIR_UP").ToLocalChecked(), Nan::New("UP").ToLocalChecked());

  /**
   * @final
   * @constant
   * @name DIR_DOWN
   * @type {string}
   */
  Nan::Set(target, Nan::New("DIR_DOWN").ToLocalChecked(), Nan::New("DOWN").ToLocalChecked());

  /**
   * @final
   * @constant
   * @name DIR_FUTURE
   * @type {string}
   */
  Nan::Set(target, Nan::New("DIR_FUTURE").ToLocalChecked(), Nan::New("FUTURE").ToLocalChecked());

  /**
   * @final
   * @constant
   * @name DIR_PAST
   * @type {string}
   */
  Nan::Set(target, Nan::New("DIR_PAST").ToLocalChecked(), Nan::New("PAST").ToLocalChecked());

  /**
   * GDAL version (not the binding version)
   *
   * @final
   * @constant {string} version
   */
  Nan::Set(target, Nan::New("version").ToLocalChecked(), Nan::New(GDAL_RELEASE_NAME).ToLocalChecked());

  /**
   * GDAL library - system library (false) or bundled (true)
   *
   * @final
   * @constant {boolean} bundled
   */
#ifdef BUNDLED_GDAL
  Nan::Set(target, Nan::New("bundled").ToLocalChecked(), Nan::New(true));
#else
  Nan::Set(target, Nan::New("bundled").ToLocalChecked(), Nan::New(false));
#endif

  /**
   * Details about the last error that occurred. The property
   * will be null or an object containing three properties: "number",
   * "message", and "type".
   *
   * @var {object} lastError
   */
  Nan::SetAccessor(target, Nan::New<v8::String>("lastError").ToLocalChecked(), LastErrorGetter, LastErrorSetter);

  /**
   * Should a warning be emitted to stderr when a synchronous operation
   * is blocking the event loop, can be safely disabled unless
   * the user application needs to remain responsive at all times
   * Use `(gdal as any).eventLoopWarning = false` to set the value from TypeScript
   *
   * @var {boolean} eventLoopWarning
   */
  Nan::SetAccessor(
    target, Nan::New<v8::String>("eventLoopWarning").ToLocalChecked(), EventLoopWarningGetter, EventLoopWarningSetter);

  // Local<Object> versions = Nan::New<Object>();
  // Nan::Set(versions, Nan::New("node").ToLocalChecked(),
  // Nan::New(NODE_VERSION+1)); Nan::Set(versions,
  // Nan::New("v8").ToLocalChecked(), Nan::New(V8::GetVersion()));
  // Nan::Set(target, Nan::New("versions").ToLocalChecked(), versions);

  /**
   * Disables all output.
   *
   * @static
   * @method quiet
   */
  Nan::SetMethod(target, "quiet", QuietOutput);

  /**
   * Displays extra debugging information from GDAL.
   *
   * @static
   * @method verbose
   */
  Nan::SetMethod(target, "verbose", VerboseOutput);

  Nan::SetMethod(target, "startLogging", StartLogging);
  Nan::SetMethod(target, "stopLogging", StopLogging);
  Nan::SetMethod(target, "log", Log);

  Local<Object> supports = Nan::New<Object>();
  Nan::Set(target, Nan::New("supports").ToLocalChecked(), supports);

  NODE_DEFINE_CONSTANT(target, CPLE_OpenFailed);
  NODE_DEFINE_CONSTANT(target, CPLE_IllegalArg);
  NODE_DEFINE_CONSTANT(target, CPLE_NotSupported);
  NODE_DEFINE_CONSTANT(target, CPLE_AssertionFailed);
  NODE_DEFINE_CONSTANT(target, CPLE_NoWriteAccess);
  NODE_DEFINE_CONSTANT(target, CPLE_UserInterrupt);

  auto *env = GetCurrentEnvironment(target->GetIsolate()->GetCurrentContext());
  AtExit(env, Cleanup, nullptr);
}
}

// N-API public init – bridges nan registrations + registers N-API classes
Napi::Object InitNapi(Napi::Env napiEnv, Napi::Object exports) {
  napi_value nv = static_cast<napi_value>(exports);
  v8::Local<v8::Object> target;
  memcpy(&target, &nv, sizeof(nv));

  // N-API top-level module functions
  exports.Set("open", Napi::Function::New(napiEnv, [](const Napi::CallbackInfo &info) -> Napi::Value {
    std::string path, mode = "r";
    NAPI_ARG_STR(0, "path", path);
    NAPI_ARG_OPT_STR(1, "mode", mode);
    unsigned int flags = GDAL_OF_VERBOSE_ERROR;
    for (size_t i = 0; i < mode.length(); i++) {
      if (mode[i] == 'r') { flags |= (i+1<mode.length()&&mode[i+1]=='+') ? (i++, GDAL_OF_UPDATE) : GDAL_OF_READONLY; }
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
    return job.run(info, true, 3);
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

  InitNan(target, v8::Local<v8::Value>(), nullptr);

  // --- N-API overrides for InitNan() functionality ---
  // These override NAN registrations with N-API equivalents.
  // Once all overrides are complete, InitNan() can be removed.

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

  // --- N-API constants (migrated from InitNan) ---
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

  exports.Set("eventLoopWarning", Napi::Boolean::New(napiEnv, true));
  exports.Set("lastError", napiEnv.Null());

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
