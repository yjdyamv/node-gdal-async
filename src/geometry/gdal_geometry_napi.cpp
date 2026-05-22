#include "gdal_geometry_napi.hpp"
#include "gdal_point_napi.hpp"
#include "gdal_linestring_napi.hpp"
#include "gdal_linearring_napi.hpp"
#include "gdal_polygon_napi.hpp"
#include "../gdal_spatial_reference_napi.hpp"
#include "gdal_circularstring_napi.hpp"
#include "gdal_compoundcurve_napi.hpp"
#include "gdal_geometrycollection_napi.hpp"
#include <node_buffer.h>

namespace node_gdal {

Napi::FunctionReference GeometryNapi::constructor;

// Extract OGRGeometry* from any geometry JS object (Geometry, Point,
// LineString, Polygon, etc.) without requiring C++ inheritance.
// Needed because e.g. PointNapi does not inherit from GeometryNapi in C++.
static OGRGeometry *GetGeometryFromObject(Napi::Object obj) {
  if (obj.InstanceOf(GeometryNapi::constructor.Value())) {
    auto *g = GeometryNapi::Unwrap(obj);
    return (g && g->isAlive()) ? g->this_ : nullptr;
  }
  if (obj.InstanceOf(PointNapi::constructor.Value())) {
    auto *p = PointNapi::Unwrap(obj);
    return (p && p->isAlive()) ? p->this_ : nullptr;
  }
  if (obj.InstanceOf(LineStringNapi::constructor.Value())) {
    auto *l = LineStringNapi::Unwrap(obj);
    return (l && l->isAlive()) ? l->this_ : nullptr;
  }
  if (obj.InstanceOf(LinearRingNapi::constructor.Value())) {
    auto *r = LinearRingNapi::Unwrap(obj);
    return (r && r->isAlive()) ? r->this_ : nullptr;
  }
  if (obj.InstanceOf(PolygonNapi::constructor.Value())) {
    auto *p = PolygonNapi::Unwrap(obj);
    return (p && p->isAlive()) ? p->this_ : nullptr;
  }
  if (obj.InstanceOf(CircularStringNapi::constructor.Value())) {
    auto *c = CircularStringNapi::Unwrap(obj);
    return (c && c->isAlive()) ? c->this_ : nullptr;
  }
  if (obj.InstanceOf(CompoundCurveNapi::constructor.Value())) {
    auto *c = CompoundCurveNapi::Unwrap(obj);
    return (c && c->isAlive()) ? c->this_ : nullptr;
  }
  if (obj.InstanceOf(GeometryCollectionNapi::constructor.Value())) {
    auto *c = GeometryCollectionNapi::Unwrap(obj);
    return (c && c->isAlive()) ? c->this_ : nullptr;
  }
  return nullptr;
}

// Unwrap geometry from this — throws if destroyed
#define NAPI_UNWRAP_GEOM(var)                                                                  \
  OGRGeometry *var = GetGeometryFromObject(info.This().As<Napi::Object>());                    \
  if (!var) {                                                                                  \
    Napi::Error::New(info.Env(), "Geometry object has already been destroyed")                  \
      .ThrowAsJavaScriptException();                                                           \
    return info.Env().Undefined();                                                             \
  }

#define NAPI_UNWRAP_GEOM_VOID(var)                                                             \
  OGRGeometry *var = GetGeometryFromObject(info.This().As<Napi::Object>());                    \
  if (!var) {                                                                                  \
    Napi::Error::New(info.Env(), "Geometry object has already been destroyed")                  \
      .ThrowAsJavaScriptException();                                                           \
    return;                                                                                    \
  }

// ---------------------------------------------------------------------------
// Init
// ---------------------------------------------------------------------------
Napi::Object GeometryNapi::Init(Napi::Env env, Napi::Object exports) {
  Napi::Function func = DefineClass(
    env, "Geometry",
    {
      InstanceMethod("toString", &GeometryNapi::toString),
      InstanceMethod("isEmpty", &GeometryNapi::isEmpty),
      InstanceMethod("isEmptyAsync", &GeometryNapi::isEmptyAsync),
      InstanceMethod("isValid", &GeometryNapi::isValid),
      InstanceMethod("isValidAsync", &GeometryNapi::isValidAsync),
      InstanceMethod("isSimple", &GeometryNapi::isSimple),
      InstanceMethod("isSimpleAsync", &GeometryNapi::isSimpleAsync),
      InstanceMethod("isRing", &GeometryNapi::isRing),
      InstanceMethod("isRingAsync", &GeometryNapi::isRingAsync),
      InstanceMethod("clone", &GeometryNapi::clone),
      InstanceMethod("empty", &GeometryNapi::empty),
      InstanceMethod("emptyAsync", &GeometryNapi::emptyAsync),
      InstanceMethod("intersects", &GeometryNapi::intersects),
      InstanceMethod("intersectsAsync", &GeometryNapi::intersectsAsync),
      InstanceMethod("equals", &GeometryNapi::equals),
      InstanceMethod("equalsAsync", &GeometryNapi::equalsAsync),
      InstanceMethod("disjoint", &GeometryNapi::disjoint),
      InstanceMethod("disjointAsync", &GeometryNapi::disjointAsync),
      InstanceMethod("touches", &GeometryNapi::touches),
      InstanceMethod("touchesAsync", &GeometryNapi::touchesAsync),
      InstanceMethod("crosses", &GeometryNapi::crosses),
      InstanceMethod("crossesAsync", &GeometryNapi::crossesAsync),
      InstanceMethod("within", &GeometryNapi::within),
      InstanceMethod("withinAsync", &GeometryNapi::withinAsync),
      InstanceMethod("contains", &GeometryNapi::contains),
      InstanceMethod("containsAsync", &GeometryNapi::containsAsync),
      InstanceMethod("overlaps", &GeometryNapi::overlaps),
      InstanceMethod("overlapsAsync", &GeometryNapi::overlapsAsync),
      InstanceMethod("boundary", &GeometryNapi::boundary),
      InstanceMethod("boundaryAsync", &GeometryNapi::boundaryAsync),
      InstanceMethod("distance", &GeometryNapi::distance),
      InstanceMethod("distanceAsync", &GeometryNapi::distanceAsync),
      InstanceMethod("convexHull", &GeometryNapi::convexHull),
      InstanceMethod("convexHullAsync", &GeometryNapi::convexHullAsync),
      InstanceMethod("buffer", &GeometryNapi::buffer),
      InstanceMethod("bufferAsync", &GeometryNapi::bufferAsync),
      InstanceMethod("intersection", &GeometryNapi::intersection),
      InstanceMethod("intersectionAsync", &GeometryNapi::intersectionAsync),
      InstanceMethod("union", &GeometryNapi::unionGeometry),
      InstanceMethod("unionAsync", &GeometryNapi::unionGeometryAsync),
      InstanceMethod("difference", &GeometryNapi::difference),
      InstanceMethod("differenceAsync", &GeometryNapi::differenceAsync),
      InstanceMethod("symDifference", &GeometryNapi::symDifference),
      InstanceMethod("symDifferenceAsync", &GeometryNapi::symDifferenceAsync),
      InstanceMethod("simplify", &GeometryNapi::simplify),
      InstanceMethod("simplifyAsync", &GeometryNapi::simplifyAsync),
      InstanceAccessor<&GeometryNapi::srsGetter, &GeometryNapi::srsSetter>("srs"),
      InstanceAccessor<&GeometryNapi::typeGetter>("wkbType"),
      InstanceAccessor<&GeometryNapi::nameGetter>("name"),
      InstanceAccessor<&GeometryNapi::wkbSizeGetter>("wkbSize"),
      InstanceAccessor<&GeometryNapi::dimensionGetter>("dimension"),
      InstanceAccessor<&GeometryNapi::coordinateDimensionGetter,
                        &GeometryNapi::coordinateDimensionSetter>("coordinateDimension"),
    });
  constructor = Napi::Persistent(func);
  constructor.SuppressDestruct();

  // Static factory methods
  func.Set("fromWKT", Napi::Function::New(env, createFromWkt, "fromWKT"));
  func.Set("fromWKB", Napi::Function::New(env, createFromWkb, "fromWKB"));
  func.Set("fromGeoJson", Napi::Function::New(env, createFromGeoJson, "fromGeoJson"));
  func.Set("getName", Napi::Function::New(env, getName, "getName"));
  func.Set("getConstructor", Napi::Function::New(env, getConstructor, "getConstructor"));

  exports.Set("Geometry", func);
  return exports;
}

// ---------------------------------------------------------------------------
// Constructor
// ---------------------------------------------------------------------------
GeometryNapi::GeometryNapi(const Napi::CallbackInfo &info)
  : Napi::ObjectWrap<GeometryNapi>(info) {
  if (!info.IsConstructCall()) {
    Napi::Error::New(info.Env(), "Cannot call constructor as function, use 'new' keyword")
      .ThrowAsJavaScriptException();
    return;
  }
  if (info.Length() > 0 && info[0].IsExternal()) {
    this_ = info[0].As<Napi::External<OGRGeometry>>().Data();
    owned_ = true;
  } else {
    Napi::Error::New(info.Env(),
      "Geometry doesn't have a constructor, use Geometry.fromWKT(), Geometry.fromWKB() "
      "or type-specific constructor. ie. new ogr.Point()")
      .ThrowAsJavaScriptException();
  }
}

Napi::Value GeometryNapi::New(Napi::Env env, OGRGeometry *geom, bool owned) {
  Napi::EscapableHandleScope scope(env);
  if (!geom) return scope.Escape(env.Null());
  if (!owned) { geom = geom->clone(); }

  // Route to type-specific constructor based on geometry type
  OGRwkbGeometryType type = geom->getGeometryType();
  // Fix LinearRing type
  if (std::string(geom->getGeometryName()) == "LINEARRING")
    type = (OGRwkbGeometryType)(wkbLinearRing | (type & wkb25DBit));

  Napi::Value obj;
  switch (type) {
    case wkbPoint: obj = PointNapi::New(env, static_cast<OGRPoint *>(geom)); break;
    case wkbLineString: obj = LineStringNapi::New(env, static_cast<OGRLineString *>(geom)); break;
    case wkbLinearRing: obj = LinearRingNapi::New(env, static_cast<OGRLinearRing *>(geom)); break;
    case wkbPolygon: obj = PolygonNapi::New(env, static_cast<OGRPolygon *>(geom)); break;
    case wkbCircularString: obj = CircularStringNapi::New(env, static_cast<OGRCircularString *>(geom)); break;
    case wkbCompoundCurve: obj = CompoundCurveNapi::New(env, static_cast<OGRCompoundCurve *>(geom)); break;
    case wkbMultiPoint: obj = MultiPointNapi::New(env, static_cast<OGRMultiPoint *>(geom)); break;
    case wkbMultiLineString:
      obj = MultiLineStringNapi::New(env, static_cast<OGRMultiLineString *>(geom));
      break;
    case wkbMultiPolygon:
      obj = MultiPolygonNapi::New(env, static_cast<OGRMultiPolygon *>(geom));
      break;
    case wkbMultiCurve: obj = MultiCurveNapi::New(env, static_cast<OGRMultiCurve *>(geom)); break;
    case wkbGeometryCollection:
      obj = GeometryCollectionNapi::New(env, static_cast<OGRGeometryCollection *>(geom));
      break;
    default: obj = GeometryBaseNapi<GeometryNapi, OGRGeometry>::New(env, geom); break;
  }
  return scope.Escape(obj);
}

// ---------------------------------------------------------------------------
// Reusable helper lambdas
// ---------------------------------------------------------------------------

// Creates a GDALAsyncableJobNapi for a boolean predicate (takes 1 Geometry arg)
static auto GeoBoolPred(Napi::Env, OGRGeometry *geom, GeometryNapi *other,
                        OGRBoolean (OGRGeometry::*fn)(const OGRGeometry *) const) {
  GDALAsyncableJobNapi<int> job;
  job.main = [geom, other, fn]() { return (geom->*fn)(other->get()); };
  job.rval = [](Napi::Env env, int r) -> Napi::Value { return Napi::Boolean::New(env, r); };
  return job;
}

// Creates a GDALAsyncableJobNapi for a geometry-returning unary operation
static auto GeoUnary(OGRGeometry *(OGRGeometry::*fn)() const) {
  return [fn](Napi::Env env, OGRGeometry *geom) {
    GDALAsyncableJobNapi<OGRGeometry *> job;
    job.main = [geom, fn]() { return (geom->*fn)(); };
    job.rval = [](Napi::Env env, OGRGeometry *g) -> Napi::Value {
      if (!g) return env.Null();
      return GeometryNapi::New(env, g);
    };
    return job;
  };
}

// ---------------------------------------------------------------------------
// toString
// ---------------------------------------------------------------------------
Napi::Value GeometryNapi::toString(const Napi::CallbackInfo &info) {
  NAPI_UNWRAP_GEOM(geom);
  std::ostringstream ss;
  ss << "Geometry (" << geom->getGeometryName() << ")";
  return Napi::String::New(info.Env(), ss.str());
}

Napi::Value GeometryNapi::clone(const Napi::CallbackInfo &info) {
  NAPI_UNWRAP_GEOM(geom);
  return GeometryNapi::New(info.Env(), geom->clone());
}

// ---------------------------------------------------------------------------
// Simple bool predicates (no args)
// ---------------------------------------------------------------------------
GDAL_ASYNCABLE_DEFINE_NAPI(GeometryNapi, isEmpty) {
  NAPI_UNWRAP_GEOM(geom);
  GDALAsyncableJobNapi<int> job;
  job.main = [geom]() { return geom->IsEmpty(); };
  job.rval = [](Napi::Env env, int r) { return Napi::Boolean::New(env, r); };
  return job.run(info, async, 0);
}
GDAL_ASYNCABLE_DEFINE_NAPI(GeometryNapi, isValid) {
  NAPI_UNWRAP_GEOM(geom);
  GDALAsyncableJobNapi<int> job;
  job.main = [geom]() { return geom->IsValid(); };
  job.rval = [](Napi::Env env, int r) { return Napi::Boolean::New(env, r); };
  return job.run(info, async, 0);
}
GDAL_ASYNCABLE_DEFINE_NAPI(GeometryNapi, isSimple) {
  NAPI_UNWRAP_GEOM(geom);
  GDALAsyncableJobNapi<int> job;
  job.main = [geom]() { return geom->IsSimple(); };
  job.rval = [](Napi::Env env, int r) { return Napi::Boolean::New(env, r); };
  return job.run(info, async, 0);
}
GDAL_ASYNCABLE_DEFINE_NAPI(GeometryNapi, isRing) {
  NAPI_UNWRAP_GEOM(geom);
  GDALAsyncableJobNapi<int> job;
  job.main = [geom]() { return geom->IsRing(); };
  job.rval = [](Napi::Env env, int r) { return Napi::Boolean::New(env, r); };
  return job.run(info, async, 0);
}

// ---------------------------------------------------------------------------
// Void operations
// ---------------------------------------------------------------------------
GDAL_ASYNCABLE_DEFINE_NAPI(GeometryNapi, empty) {
  NAPI_UNWRAP_GEOM(geom);
  GDALAsyncableJobNapi<int> job;
  job.main = [geom]() { geom->empty(); return 0; };
  job.rval = [](Napi::Env env, int) { return env.Undefined(); };
  return job.run(info, async, 0);
}
GDAL_ASYNCABLE_DEFINE_NAPI(GeometryNapi, closeRings) {
  NAPI_UNWRAP_GEOM(geom);
  GDALAsyncableJobNapi<int> job;
  job.main = [geom]() { geom->closeRings(); return 0; };
  job.rval = [](Napi::Env env, int) { return env.Undefined(); };
  return job.run(info, async, 0);
}
GDAL_ASYNCABLE_DEFINE_NAPI(GeometryNapi, swapXY) {
  NAPI_UNWRAP_GEOM(geom);
  GDALAsyncableJobNapi<int> job;
  job.main = [geom]() { geom->swapXY(); return 0; };
  job.rval = [](Napi::Env env, int) { return env.Undefined(); };
  return job.run(info, async, 0);
}
GDAL_ASYNCABLE_DEFINE_NAPI(GeometryNapi, flattenTo2D) {
  NAPI_UNWRAP_GEOM(geom);
  GDALAsyncableJobNapi<int> job;
  job.main = [geom]() { geom->flattenTo2D(); return 0; };
  job.rval = [](Napi::Env env, int) { return env.Undefined(); };
  return job.run(info, async, 0);
}

// ---------------------------------------------------------------------------
// Boolean predicates (1 Geometry arg)
// ---------------------------------------------------------------------------
#define GEO_BOOL_PRED(name, fn)                                                                \
  GDAL_ASYNCABLE_DEFINE_NAPI(GeometryNapi, name) {                                             \
    NAPI_UNWRAP_GEOM(geom);                                                       \
    GeometryNapi *other;                                                                        \
    NAPI_ARG_WRAPPED(0, "geometry", GeometryNapi, other);                                       \
    auto job = GeoBoolPred(info.Env(), geom, other, &OGRGeometry::fn);                           \
    return job.run(info, async, 1);                                                             \
  }

GEO_BOOL_PRED(intersects, Intersects)
GEO_BOOL_PRED(equals, Equals)
GEO_BOOL_PRED(disjoint, Disjoint)
GEO_BOOL_PRED(touches, Touches)
GEO_BOOL_PRED(crosses, Crosses)
GEO_BOOL_PRED(within, Within)
GEO_BOOL_PRED(contains, Contains)
GEO_BOOL_PRED(overlaps, Overlaps)

// ---------------------------------------------------------------------------
// Unary operations returning Geometry
// ---------------------------------------------------------------------------
#define GEO_UNARY(name, fn)                                                                    \
  GDAL_ASYNCABLE_DEFINE_NAPI(GeometryNapi, name) {                                             \
    NAPI_UNWRAP_GEOM(geom);                                                       \
    auto job = GeoUnary(&OGRGeometry::fn)(info.Env(), geom);                                    \
    return job.run(info, async, 0);                                                             \
  }

GEO_UNARY(boundary, Boundary)
GEO_UNARY(convexHull, ConvexHull)
// centroid has different signature (OGRErr Centroid(OGRPoint*)), implemented below
GDAL_ASYNCABLE_DEFINE_NAPI(GeometryNapi, getEnvelope) {
  NAPI_UNWRAP_GEOM(geom);
  GDALAsyncableJobNapi<OGREnvelope *> job;
  job.main = [geom]() {
    auto env = new OGREnvelope();
    geom->getEnvelope(env);
    return env;
  };
  job.rval = [](Napi::Env env, OGREnvelope *e) -> Napi::Value {
    if (!e) return env.Null();
    Napi::Object r = Napi::Object::New(env);
    r.Set("minX", Napi::Number::New(env, e->MinX));
    r.Set("maxX", Napi::Number::New(env, e->MaxX));
    r.Set("minY", Napi::Number::New(env, e->MinY));
    r.Set("maxY", Napi::Number::New(env, e->MaxY));
    delete e;
    return r;
  };
  return job.run(info, async, 0);
}
GDAL_ASYNCABLE_DEFINE_NAPI(GeometryNapi, getEnvelope3D) {
  NAPI_UNWRAP_GEOM(geom);
  GDALAsyncableJobNapi<OGREnvelope3D *> job;
  job.main = [geom]() {
    auto env = new OGREnvelope3D();
    geom->getEnvelope(env);
    return env;
  };
  job.rval = [](Napi::Env env, OGREnvelope3D *e) -> Napi::Value {
    if (!e) return env.Null();
    Napi::Object r = Napi::Object::New(env);
    r.Set("minX", Napi::Number::New(env, e->MinX));
    r.Set("maxX", Napi::Number::New(env, e->MaxX));
    r.Set("minY", Napi::Number::New(env, e->MinY));
    r.Set("maxY", Napi::Number::New(env, e->MaxY));
    r.Set("minZ", Napi::Number::New(env, e->MinZ));
    r.Set("maxZ", Napi::Number::New(env, e->MaxZ));
    delete e;
    return r;
  };
  return job.run(info, async, 0);
}

// ---------------------------------------------------------------------------
// distance (1 Geometry arg, returns double)
// ---------------------------------------------------------------------------
GDAL_ASYNCABLE_DEFINE_NAPI(GeometryNapi, distance) {
  NAPI_UNWRAP_GEOM(geom);
  GeometryNapi *other;
  NAPI_ARG_WRAPPED(0, "geometry", GeometryNapi, other);
  GDALAsyncableJobNapi<double> job;
  job.main = [geom, other]() { return geom->Distance(other->this_); };
  job.rval = [](Napi::Env env, double d) { return Napi::Number::New(env, d); };
  return job.run(info, async, 1);
}

// ---------------------------------------------------------------------------
// buffer (double arg, returns Geometry)
// ---------------------------------------------------------------------------
GDAL_ASYNCABLE_DEFINE_NAPI(GeometryNapi, buffer) {
  NAPI_UNWRAP_GEOM(geom);
  double dist;
  NAPI_ARG_DOUBLE(0, "distance", dist);
  GDALAsyncableJobNapi<OGRGeometry *> job;
  job.main = [geom, dist]() { return geom->Buffer(dist); };
  job.rval = [](Napi::Env env, OGRGeometry *g) -> Napi::Value {
    if (!g) return env.Null();
    return GeometryNapi::New(env, g);
  };
  return job.run(info, async, 1);
}

// ---------------------------------------------------------------------------
// Binary operations (1 Geometry arg, returns Geometry)
// ---------------------------------------------------------------------------
#define GEO_BINARY(name, fn)                                                                   \
  GDAL_ASYNCABLE_DEFINE_NAPI(GeometryNapi, name) {                                             \
    NAPI_UNWRAP_GEOM(geom);                                                       \
    GeometryNapi *other;                                                                        \
    NAPI_ARG_WRAPPED(0, "geometry", GeometryNapi, other);                                       \
    GDALAsyncableJobNapi<OGRGeometry *> job;                                                    \
    job.main = [geom, other]() { return (geom->fn)(other->this_); };                    \
    job.rval = [](Napi::Env env, OGRGeometry *g) -> Napi::Value {                              \
      if (!g) return env.Null();                                                                \
      return GeometryNapi::New(env, g);                                                        \
    };                                                                                          \
    return job.run(info, async, 1);                                                             \
  }

GEO_BINARY(intersection, Intersection)
GEO_BINARY(unionGeometry, Union)
GEO_BINARY(difference, Difference)
GEO_BINARY(symDifference, SymDifference)

// ---------------------------------------------------------------------------
// simplify / simplifyPreserveTopology (double arg, returns Geometry)
// ---------------------------------------------------------------------------
GDAL_ASYNCABLE_DEFINE_NAPI(GeometryNapi, simplify) {
  NAPI_UNWRAP_GEOM(geom);
  double tol;
  NAPI_ARG_DOUBLE(0, "tolerance", tol);
  GDALAsyncableJobNapi<OGRGeometry *> job;
  job.main = [geom, tol]() { return geom->Simplify(tol); };
  job.rval = [](Napi::Env env, OGRGeometry *g) -> Napi::Value {
    if (!g) return env.Null();
    return GeometryNapi::New(env, g);
  };
  return job.run(info, async, 1);
}
GDAL_ASYNCABLE_DEFINE_NAPI(GeometryNapi, simplifyPreserveTopology) {
  NAPI_UNWRAP_GEOM(geom);
  double tol;
  NAPI_ARG_DOUBLE(0, "tolerance", tol);
  GDALAsyncableJobNapi<OGRGeometry *> job;
  job.main = [geom, tol]() { return geom->SimplifyPreserveTopology(tol); };
  job.rval = [](Napi::Env env, OGRGeometry *g) -> Napi::Value {
    if (!g) return env.Null();
    return GeometryNapi::New(env, g);
  };
  return job.run(info, async, 1);
}

// ---------------------------------------------------------------------------
// segmentize (double arg, void)
// ---------------------------------------------------------------------------
GDAL_ASYNCABLE_DEFINE_NAPI(GeometryNapi, segmentize) {
  NAPI_UNWRAP_GEOM(geom);
  double len;
  NAPI_ARG_DOUBLE(0, "segment length", len);
  GDALAsyncableJobNapi<int> job;
  job.main = [geom, len]() { geom->segmentize(len); return 0; };
  job.rval = [](Napi::Env env, int) { return env.Undefined(); };
  return job.run(info, async, 1);
}

// ---------------------------------------------------------------------------
// Export methods
// ---------------------------------------------------------------------------
GDAL_ASYNCABLE_DEFINE_NAPI(GeometryNapi, exportToWKT) {
  NAPI_UNWRAP_GEOM(geom);
  GDALAsyncableJobNapi<char *> job;
  job.main = [geom]() {
    char *wkt = nullptr;
    geom->exportToWkt(&wkt);
    return wkt;
  };
  job.rval = [](Napi::Env env, char *wkt) -> Napi::Value {
    if (!wkt) return env.Null();
    Napi::String result = Napi::String::New(env, wkt);
    CPLFree(wkt);
    return result;
  };
  return job.run(info, async, 0);
}
GDAL_ASYNCABLE_DEFINE_NAPI(GeometryNapi, exportToWKB) {
  NAPI_UNWRAP_GEOM(geom);
  GDALAsyncableJobNapi<std::pair<unsigned char *, int>> job;
  job.main = [geom]() -> std::pair<unsigned char *, int> {
    int size = geom->WkbSize();
    auto buf = new unsigned char[size];
    geom->exportToWkb(wkbNDR, buf);
    return {buf, size};
  };
  job.rval = [](Napi::Env env, std::pair<unsigned char *, int> r) -> Napi::Value {
    auto buf = Napi::Buffer<unsigned char>::Copy(env, r.first, r.second);
    delete[] r.first;
    return buf;
  };
  return job.run(info, async, 0);
}
GDAL_ASYNCABLE_DEFINE_NAPI(GeometryNapi, exportToJSON) {
  NAPI_UNWRAP_GEOM(geom);
  GDALAsyncableJobNapi<char *> job;
  job.main = [geom]() {
    return geom->exportToJson(nullptr);
  };
  job.rval = [](Napi::Env env, char *json) -> Napi::Value {
    if (!json) return env.Null();
    Napi::String result = Napi::String::New(env, json);
    CPLFree(json);
    return result;
  };
  return job.run(info, async, 0);
}
GDAL_ASYNCABLE_DEFINE_NAPI(GeometryNapi, exportToKML) {
  NAPI_UNWRAP_GEOM(geom);
  GDALAsyncableJobNapi<char *> job;
  job.main = [geom]() {
    return geom->exportToKML();
  };
  job.rval = [](Napi::Env env, char *kml) -> Napi::Value {
    if (!kml) return env.Null();
    Napi::String result = Napi::String::New(env, kml);
    CPLFree(kml);
    return result;
  };
  return job.run(info, async, 0);
}
GDAL_ASYNCABLE_DEFINE_NAPI(GeometryNapi, exportToGML) {
  NAPI_UNWRAP_GEOM(geom);
  GDALAsyncableJobNapi<char *> job;
  job.main = [geom]() {
    return geom->exportToGML();
  };
  job.rval = [](Napi::Env env, char *gml) -> Napi::Value {
    if (!gml) return env.Null();
    Napi::String result = Napi::String::New(env, gml);
    CPLFree(gml);
    return result;
  };
  return job.run(info, async, 0);
}

// ---------------------------------------------------------------------------
// Static constructors
// ---------------------------------------------------------------------------
Napi::Value GeometryNapi::createFromWkt(const Napi::CallbackInfo &info) {
  std::string wkt;
  NAPI_ARG_STR(0, "wkt", wkt);
  OGRGeometry *geom = nullptr;
  OGRErr err = OGRGeometryFactory::createFromWkt(wkt.c_str(), nullptr, &geom);
  if (err != OGRERR_NONE) { NAPI_THROW_OGRERR(err); }
  return GeometryNapi::New(info.Env(), geom);
}
Napi::Value GeometryNapi::createFromWkb(const Napi::CallbackInfo &info) {
  if (info.Length() < 1 || !info[0].IsBuffer()) {
    Napi::TypeError::New(info.Env(), "buffer must be provided").ThrowAsJavaScriptException();
    return info.Env().Undefined();
  }
  Napi::Buffer<unsigned char> buf = info[0].As<Napi::Buffer<unsigned char>>();
  OGRGeometry *geom = nullptr;
  OGRErr err = OGRGeometryFactory::createFromWkb(buf.Data(), nullptr, &geom, (int)buf.Length());
  if (err != OGRERR_NONE) { NAPI_THROW_OGRERR(err); }
  return GeometryNapi::New(info.Env(), geom);
}
Napi::Value GeometryNapi::createFromGeoJson(const Napi::CallbackInfo &info) {
  std::string json;
  NAPI_ARG_STR(0, "geojson", json);
  OGRGeometry *geom = OGRGeometryFactory::createFromGeoJson(json.c_str());
  if (!geom) NAPI_THROW_LAST_CPLERR;
  return GeometryNapi::New(info.Env(), geom);
}

Napi::Value GeometryNapi::getName(const Napi::CallbackInfo &info) {
  std::string wkb_type = "Unknown";
  if (info.Length() > 0 && info[0].IsNumber()) {
    int type = info[0].As<Napi::Number>().Int32Value();
    auto geom = OGRGeometryFactory::createGeometry(static_cast<OGRwkbGeometryType>(type));
    if (geom) {
      wkb_type = geom->getGeometryName();
      OGRGeometryFactory::destroyGeometry(geom);
    }
  }
  return Napi::String::New(info.Env(), wkb_type);
}
Napi::Value GeometryNapi::getConstructor(const Napi::CallbackInfo &info) {
  return info.Env().Null(); // TODO: return type-specific constructors
}

// ---------------------------------------------------------------------------
// Getters & Setters
// ---------------------------------------------------------------------------
Napi::Value GeometryNapi::srsGetter(const Napi::CallbackInfo &info) {
  NAPI_UNWRAP_GEOM(geom);
  const OGRSpatialReference *srs = geom->getSpatialReference();
  if (!srs) return info.Env().Null();
  return SpatialReferenceNapi::New(info.Env(), srs);
}
void GeometryNapi::srsSetter(const Napi::CallbackInfo &info, const Napi::Value &value) {
  NAPI_UNWRAP_GEOM_VOID(geom);
  if (value.IsNull() || value.IsUndefined()) {
    geom->assignSpatialReference(nullptr);
    return;
  }
  if (value.IsObject() && value.As<Napi::Object>().InstanceOf(SpatialReferenceNapi::constructor.Value())) {
    SpatialReferenceNapi *srs = SpatialReferenceNapi::Unwrap(value.As<Napi::Object>());
    if (!srs || !srs->isAlive()) {
      Napi::Error::New(info.Env(), "SpatialReferenceNapi object has already been destroyed")
        .ThrowAsJavaScriptException();
      return;
    }
    geom->assignSpatialReference(srs->get());
    return;
  }
  Napi::Error::New(info.Env(), "srs must be a SpatialReferenceNapi object")
    .ThrowAsJavaScriptException();
}

Napi::Value GeometryNapi::typeGetter(const Napi::CallbackInfo &info) {
  NAPI_UNWRAP_GEOM(geom);
  OGRwkbGeometryType type = geom->getGeometryType();
  if (std::string(geom->getGeometryName()) == "LINEARRING")
    type = (OGRwkbGeometryType)(wkbLinearRing | (type & wkb25DBit));
  return Napi::Number::New(info.Env(), type);
}
Napi::Value GeometryNapi::nameGetter(const Napi::CallbackInfo &info) {
  NAPI_UNWRAP_GEOM(geom);
  return Napi::String::New(info.Env(), geom->getGeometryName());
}
Napi::Value GeometryNapi::wkbSizeGetter(const Napi::CallbackInfo &info) {
  NAPI_UNWRAP_GEOM(geom);
  return Napi::Number::New(info.Env(), geom->WkbSize());
}
Napi::Value GeometryNapi::dimensionGetter(const Napi::CallbackInfo &info) {
  NAPI_UNWRAP_GEOM(geom);
  return Napi::Number::New(info.Env(), geom->getDimension());
}
Napi::Value GeometryNapi::coordinateDimensionGetter(const Napi::CallbackInfo &info) {
  NAPI_UNWRAP_GEOM(geom);
  return Napi::Number::New(info.Env(), geom->getCoordinateDimension());
}
void GeometryNapi::coordinateDimensionSetter(const Napi::CallbackInfo &info, const Napi::Value &value) {
  NAPI_UNWRAP_GEOM_VOID(geom);
  if (!value.IsNumber()) {
    Napi::Error::New(info.Env(), "coordinateDimension must be a number").ThrowAsJavaScriptException();
    return;
  }
  geom->setCoordinateDimension(value.As<Napi::Number>().Int32Value());
}

// Add inherited geometry methods to subclass prototypes.
// InstanceMethod in GeometryNapi::DefineClass calls GeometryNapi::Unwrap()
// which fails for subclass objects (PointNapi, LineStringNapi, etc.).
// Instead, we attach the same logic as raw Napi::Function lambdas that use
// GetGeometryFromObject(), which handles all geometry types correctly.
static void setProtoMethod(Napi::Env env, napi_value proto, const char *name, Napi::Function fn) {
  napi_property_descriptor desc = {};
  desc.utf8name = name;
  desc.method = nullptr;  // We use value instead of method for instance methods
  desc.value = static_cast<napi_value>(fn);
  desc.attributes = static_cast<napi_property_attributes>(napi_writable | napi_configurable);
  napi_define_properties(env, proto, 1, &desc);
}

void GeometryNapi::AddInheritedMethods(Napi::Env env, Napi::Function ctor) {
  napi_value proto_nv;
  napi_get_named_property(env, ctor, "prototype", &proto_nv);
  Napi::Object proto = Napi::Object(env, proto_nv);

#define GEO_METHOD(name, body) \
  setProtoMethod(env, proto_nv, name, Napi::Function::New(env, [](const Napi::CallbackInfo &info) -> Napi::Value { \
    OGRGeometry *geom = GetGeometryFromObject(info.This().As<Napi::Object>()); \
    if (!geom) { Napi::Error::New(info.Env(), "Geometry destroyed").ThrowAsJavaScriptException(); return info.Env().Undefined(); } \
    body \
  }, name))

  GEO_METHOD("isValid", return Napi::Boolean::New(info.Env(), geom->IsValid()); );
  GEO_METHOD("isSimple", return Napi::Boolean::New(info.Env(), geom->IsSimple()); );
  GEO_METHOD("isEmpty", return Napi::Boolean::New(info.Env(), geom->IsEmpty()); );
  GEO_METHOD("swapXY", geom->swapXY(); return info.Env().Undefined(); );
  GEO_METHOD("clone", return GeometryNapi::New(info.Env(), geom->clone()); );
  GEO_METHOD("toString",
    std::ostringstream ss;
    ss << "Geometry (" << geom->getGeometryName() << ")";
    return Napi::String::New(info.Env(), ss.str());
  );
  GEO_METHOD("exportToWKT",
    char *wkt = nullptr; geom->exportToWkt(&wkt);
    if (!wkt) return info.Env().Null();
    Napi::String result = Napi::String::New(info.Env(), wkt); CPLFree(wkt);
    return result;
  );
  GEO_METHOD("exportToJSON",
    char *json = geom->exportToJson(nullptr);
    if (!json) return info.Env().Null();
    Napi::String result = Napi::String::New(info.Env(), json); CPLFree(json);
    return result;
  );
#undef GEO_METHOD
}

} // namespace node_gdal
