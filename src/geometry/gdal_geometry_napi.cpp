#include "gdal_geometry_napi.hpp"
#include "gdal_point_napi.hpp"
#include "gdal_linestring_napi.hpp"
#include "gdal_linearring_napi.hpp"
#include "gdal_polygon_napi.hpp"
#include "gdal_circularstring_napi.hpp"
#include "gdal_compoundcurve_napi.hpp"
#include "gdal_geometrycollection_napi.hpp"
#include <node_buffer.h>

namespace node_gdal {

Napi::FunctionReference GeometryNapi::constructor;

// ---------------------------------------------------------------------------
// Init
// ---------------------------------------------------------------------------
Napi::Object GeometryNapi::Init(Napi::Env env, Napi::Object exports) {
  Napi::Function func = DefineClass(
    env, "GeometryNapi",
    {
      // Static methods
      StaticMethod("fromWKT", &GeometryNapi::createFromWkt),
      StaticMethod("fromWKTAsync", &GeometryNapi::createFromWktAsync),
      StaticMethod("fromWKB", &GeometryNapi::createFromWkb),
      StaticMethod("fromWKBAsync", &GeometryNapi::createFromWkbAsync),
      StaticMethod("fromGeoJson", &GeometryNapi::createFromGeoJson),
      StaticMethod("fromGeoJsonAsync", &GeometryNapi::createFromGeoJsonAsync),
      StaticMethod("getName", &GeometryNapi::getName),
      StaticMethod("getConstructor", &GeometryNapi::getConstructor),
      // Instance methods
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
      InstanceMethod("centroid", &GeometryNapi::centroid),
      InstanceMethod("centroidAsync", &GeometryNapi::centroidAsync),
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
  exports.Set("GeometryNapi", func);
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
static auto GeoBoolPred(Napi::Env, GeometryNapi *self, GeometryNapi *other,
                        OGRBoolean (OGRGeometry::*fn)(const OGRGeometry *) const) {
  GDALAsyncableJobNapi<int> job;
  job.main = [self, other, fn]() { return (self->this_->*fn)(other->this_); };
  job.rval = [](Napi::Env env, int r) -> Napi::Value { return Napi::Boolean::New(env, r); };
  return job;
}

// Creates a GDALAsyncableJobNapi for a geometry-returning unary operation
static auto GeoUnary(OGRGeometry *(OGRGeometry::*fn)() const) {
  return [fn](Napi::Env env, GeometryNapi *self) {
    GDALAsyncableJobNapi<OGRGeometry *> job;
    job.main = [self, fn]() { return (self->this_->*fn)(); };
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
  NAPI_UNWRAP_THIS(GeometryNapi, self);
  std::ostringstream ss;
  ss << "Geometry (" << self->this_->getGeometryName() << ")";
  return Napi::String::New(info.Env(), ss.str());
}

Napi::Value GeometryNapi::clone(const Napi::CallbackInfo &info) {
  NAPI_UNWRAP_THIS(GeometryNapi, self);
  return GeometryNapi::New(info.Env(), self->this_->clone());
}

// ---------------------------------------------------------------------------
// Simple bool predicates (no args)
// ---------------------------------------------------------------------------
GDAL_ASYNCABLE_DEFINE_NAPI(GeometryNapi, isEmpty) {
  NAPI_UNWRAP_THIS(GeometryNapi, self);
  GDALAsyncableJobNapi<int> job;
  job.main = [self]() { return self->this_->IsEmpty(); };
  job.rval = [](Napi::Env env, int r) { return Napi::Boolean::New(env, r); };
  return job.run(info, async, 0);
}
GDAL_ASYNCABLE_DEFINE_NAPI(GeometryNapi, isValid) {
  NAPI_UNWRAP_THIS(GeometryNapi, self);
  GDALAsyncableJobNapi<int> job;
  job.main = [self]() { return self->this_->IsValid(); };
  job.rval = [](Napi::Env env, int r) { return Napi::Boolean::New(env, r); };
  return job.run(info, async, 0);
}
GDAL_ASYNCABLE_DEFINE_NAPI(GeometryNapi, isSimple) {
  NAPI_UNWRAP_THIS(GeometryNapi, self);
  GDALAsyncableJobNapi<int> job;
  job.main = [self]() { return self->this_->IsSimple(); };
  job.rval = [](Napi::Env env, int r) { return Napi::Boolean::New(env, r); };
  return job.run(info, async, 0);
}
GDAL_ASYNCABLE_DEFINE_NAPI(GeometryNapi, isRing) {
  NAPI_UNWRAP_THIS(GeometryNapi, self);
  GDALAsyncableJobNapi<int> job;
  job.main = [self]() { return self->this_->IsRing(); };
  job.rval = [](Napi::Env env, int r) { return Napi::Boolean::New(env, r); };
  return job.run(info, async, 0);
}

// ---------------------------------------------------------------------------
// Void operations
// ---------------------------------------------------------------------------
GDAL_ASYNCABLE_DEFINE_NAPI(GeometryNapi, empty) {
  NAPI_UNWRAP_THIS(GeometryNapi, self);
  GDALAsyncableJobNapi<int> job;
  job.main = [self]() { self->this_->empty(); return 0; };
  job.rval = [](Napi::Env env, int) { return env.Undefined(); };
  return job.run(info, async, 0);
}
GDAL_ASYNCABLE_DEFINE_NAPI(GeometryNapi, closeRings) {
  NAPI_UNWRAP_THIS(GeometryNapi, self);
  GDALAsyncableJobNapi<int> job;
  job.main = [self]() { self->this_->closeRings(); return 0; };
  job.rval = [](Napi::Env env, int) { return env.Undefined(); };
  return job.run(info, async, 0);
}
GDAL_ASYNCABLE_DEFINE_NAPI(GeometryNapi, swapXY) {
  NAPI_UNWRAP_THIS(GeometryNapi, self);
  GDALAsyncableJobNapi<int> job;
  job.main = [self]() { self->this_->swapXY(); return 0; };
  job.rval = [](Napi::Env env, int) { return env.Undefined(); };
  return job.run(info, async, 0);
}
GDAL_ASYNCABLE_DEFINE_NAPI(GeometryNapi, flattenTo2D) {
  NAPI_UNWRAP_THIS(GeometryNapi, self);
  GDALAsyncableJobNapi<int> job;
  job.main = [self]() { self->this_->flattenTo2D(); return 0; };
  job.rval = [](Napi::Env env, int) { return env.Undefined(); };
  return job.run(info, async, 0);
}

// ---------------------------------------------------------------------------
// Boolean predicates (1 Geometry arg)
// ---------------------------------------------------------------------------
#define GEO_BOOL_PRED(name, fn)                                                                \
  GDAL_ASYNCABLE_DEFINE_NAPI(GeometryNapi, name) {                                             \
    NAPI_UNWRAP_THIS(GeometryNapi, self);                                                       \
    GeometryNapi *other;                                                                        \
    NAPI_ARG_WRAPPED(0, "geometry", GeometryNapi, other);                                       \
    auto job = GeoBoolPred(info.Env(), self, other, &OGRGeometry::fn);                           \
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
    NAPI_UNWRAP_THIS(GeometryNapi, self);                                                       \
    auto job = GeoUnary(&OGRGeometry::fn)(info.Env(), self);                                    \
    return job.run(info, async, 0);                                                             \
  }

GEO_UNARY(boundary, Boundary)
GEO_UNARY(convexHull, ConvexHull)
GEO_UNARY(centroid, Centroid)
GDAL_ASYNCABLE_DEFINE_NAPI(GeometryNapi, getEnvelope) {
  NAPI_UNWRAP_THIS(GeometryNapi, self);
  GDALAsyncableJobNapi<OGREnvelope *> job;
  job.main = [self]() {
    auto env = new OGREnvelope();
    self->this_->getEnvelope(env);
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
  NAPI_UNWRAP_THIS(GeometryNapi, self);
  GDALAsyncableJobNapi<OGREnvelope3D *> job;
  job.main = [self]() {
    auto env = new OGREnvelope3D();
    self->this_->getEnvelope(env);
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
  NAPI_UNWRAP_THIS(GeometryNapi, self);
  GeometryNapi *other;
  NAPI_ARG_WRAPPED(0, "geometry", GeometryNapi, other);
  GDALAsyncableJobNapi<double> job;
  job.main = [self, other]() { return self->this_->Distance(other->this_); };
  job.rval = [](Napi::Env env, double d) { return Napi::Number::New(env, d); };
  return job.run(info, async, 1);
}

// ---------------------------------------------------------------------------
// buffer (double arg, returns Geometry)
// ---------------------------------------------------------------------------
GDAL_ASYNCABLE_DEFINE_NAPI(GeometryNapi, buffer) {
  NAPI_UNWRAP_THIS(GeometryNapi, self);
  double dist;
  NAPI_ARG_DOUBLE(0, "distance", dist);
  GDALAsyncableJobNapi<OGRGeometry *> job;
  job.main = [self, dist]() { return self->this_->Buffer(dist); };
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
    NAPI_UNWRAP_THIS(GeometryNapi, self);                                                       \
    GeometryNapi *other;                                                                        \
    NAPI_ARG_WRAPPED(0, "geometry", GeometryNapi, other);                                       \
    GDALAsyncableJobNapi<OGRGeometry *> job;                                                    \
    job.main = [self, other]() { return (self->this_->fn)(other->this_); };                    \
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
  NAPI_UNWRAP_THIS(GeometryNapi, self);
  double tol;
  NAPI_ARG_DOUBLE(0, "tolerance", tol);
  GDALAsyncableJobNapi<OGRGeometry *> job;
  job.main = [self, tol]() { return self->this_->Simplify(tol); };
  job.rval = [](Napi::Env env, OGRGeometry *g) -> Napi::Value {
    if (!g) return env.Null();
    return GeometryNapi::New(env, g);
  };
  return job.run(info, async, 1);
}
GDAL_ASYNCABLE_DEFINE_NAPI(GeometryNapi, simplifyPreserveTopology) {
  NAPI_UNWRAP_THIS(GeometryNapi, self);
  double tol;
  NAPI_ARG_DOUBLE(0, "tolerance", tol);
  GDALAsyncableJobNapi<OGRGeometry *> job;
  job.main = [self, tol]() { return self->this_->SimplifyPreserveTopology(tol); };
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
  NAPI_UNWRAP_THIS(GeometryNapi, self);
  double len;
  NAPI_ARG_DOUBLE(0, "segment length", len);
  GDALAsyncableJobNapi<int> job;
  job.main = [self, len]() { self->this_->segmentize(len); return 0; };
  job.rval = [](Napi::Env env, int) { return env.Undefined(); };
  return job.run(info, async, 1);
}

// ---------------------------------------------------------------------------
// Export methods
// ---------------------------------------------------------------------------
GDAL_ASYNCABLE_DEFINE_NAPI(GeometryNapi, exportToWKT) {
  NAPI_UNWRAP_THIS(GeometryNapi, self);
  GDALAsyncableJobNapi<char *> job;
  job.main = [self]() {
    char *wkt = nullptr;
    self->this_->exportToWkt(&wkt);
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
  NAPI_UNWRAP_THIS(GeometryNapi, self);
  GDALAsyncableJobNapi<std::pair<unsigned char *, int>> job;
  job.main = [self]() -> std::pair<unsigned char *, int> {
    int size = self->this_->WkbSize();
    auto buf = new unsigned char[size];
    self->this_->exportToWkb(wkbNDR, buf);
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
  NAPI_UNWRAP_THIS(GeometryNapi, self);
  GDALAsyncableJobNapi<char *> job;
  job.main = [self]() {
    return self->this_->exportToJson(nullptr);
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
  NAPI_UNWRAP_THIS(GeometryNapi, self);
  GDALAsyncableJobNapi<char *> job;
  job.main = [self]() {
    return self->this_->exportToKML(nullptr);
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
  NAPI_UNWRAP_THIS(GeometryNapi, self);
  GDALAsyncableJobNapi<char *> job;
  job.main = [self]() {
    return self->this_->exportToGML(nullptr);
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
GDAL_ASYNCABLE_DEFINE_NAPI(GeometryNapi, create) {
  return info.Env().Undefined(); // stub - not used from JS
}
GDAL_ASYNCABLE_DEFINE_NAPI(GeometryNapi, createFromWkt) {
  std::string wkt;
  NAPI_ARG_STR(0, "wkt", wkt);
  GDALAsyncableJobNapi<OGRGeometry *> job;
  job.main = [wkt]() -> OGRGeometry * {
    OGRGeometry *geom;
    OGRErr err = OGRGeometryFactory::createFromWkt(wkt.c_str(), nullptr, &geom);
    if (err != OGRERR_NONE) throw "Failed to create geometry from WKT";
    return geom;
  };
  job.rval = [](Napi::Env env, OGRGeometry *g) -> Napi::Value {
    return GeometryNapi::New(env, g);
  };
  return job.run(info, async, 1);
}
GDAL_ASYNCABLE_DEFINE_NAPI(GeometryNapi, createFromWkb) {
  if (info.Length() < 1 || !info[0].IsBuffer()) {
    Napi::TypeError::New(info.Env(), "buffer must be provided").ThrowAsJavaScriptException();
    return info.Env().Undefined();
  }
  Napi::Buffer<unsigned char> buf = info[0].As<Napi::Buffer<unsigned char>>();
  GDALAsyncableJobNapi<OGRGeometry *> job;
  job.main = [buf_data = buf.Data(), buf_len = buf.Length()]() -> OGRGeometry * {
    OGRGeometry *geom;
    OGRErr err = OGRGeometryFactory::createFromWkb(buf_data, nullptr, &geom, buf_len);
    if (err != OGRERR_NONE) throw "Failed to create geometry from WKB";
    return geom;
  };
  job.rval = [](Napi::Env env, OGRGeometry *g) -> Napi::Value {
    return GeometryNapi::New(env, g);
  };
  return job.run(info, async, 1);
}
GDAL_ASYNCABLE_DEFINE_NAPI(GeometryNapi, createFromGeoJson) {
  std::string json;
  NAPI_ARG_STR(0, "geojson", json);
  GDALAsyncableJobNapi<OGRGeometry *> job;
  job.main = [json]() -> OGRGeometry * {
    OGRGeometry *geom = OGRGeometryFactory::createFromGeoJson(json.c_str());
    if (!geom) throw CPLGetLastErrorMsg();
    return geom;
  };
  job.rval = [](Napi::Env env, OGRGeometry *g) -> Napi::Value {
    return GeometryNapi::New(env, g);
  };
  return job.run(info, async, 1);
}

// ---------------------------------------------------------------------------
// Static helpers
// ---------------------------------------------------------------------------
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
  // TODO: return type-specific constructor functions
  return info.Env().Null();
}

// ---------------------------------------------------------------------------
// Getters & Setters
// ---------------------------------------------------------------------------
Napi::Value GeometryNapi::srsGetter(const Napi::CallbackInfo &info) {
  NAPI_UNWRAP_THIS(GeometryNapi, self);
  OGRSpatialReference *srs = self->this_->getSpatialReference();
  if (!srs) return info.Env().Null();
  // TODO: return SpatialReferenceNapi when ported
  return info.Env().Null();
}
void GeometryNapi::srsSetter(const Napi::CallbackInfo &info, const Napi::Value &value) {
  NAPI_UNWRAP_THIS_VOID(GeometryNapi, self);
  if (value.IsNull() || value.IsUndefined()) {
    self->this_->assignSpatialReference(nullptr);
    return;
  }
  // TODO: accept SpatialReferenceNapi
  Napi::Error::New(info.Env(), "srs setter requires SpatialReferenceNapi (not yet ported)")
    .ThrowAsJavaScriptException();
}

Napi::Value GeometryNapi::typeGetter(const Napi::CallbackInfo &info) {
  NAPI_UNWRAP_THIS(GeometryNapi, self);
  OGRwkbGeometryType type = self->this_->getGeometryType();
  if (std::string(self->this_->getGeometryName()) == "LINEARRING")
    type = (OGRwkbGeometryType)(wkbLinearRing | (type & wkb25DBit));
  return Napi::Number::New(info.Env(), type);
}
Napi::Value GeometryNapi::nameGetter(const Napi::CallbackInfo &info) {
  NAPI_UNWRAP_THIS(GeometryNapi, self);
  return Napi::String::New(info.Env(), self->this_->getGeometryName());
}
Napi::Value GeometryNapi::wkbSizeGetter(const Napi::CallbackInfo &info) {
  NAPI_UNWRAP_THIS(GeometryNapi, self);
  return Napi::Number::New(info.Env(), self->this_->WkbSize());
}
Napi::Value GeometryNapi::dimensionGetter(const Napi::CallbackInfo &info) {
  NAPI_UNWRAP_THIS(GeometryNapi, self);
  return Napi::Number::New(info.Env(), self->this_->getDimension());
}
Napi::Value GeometryNapi::coordinateDimensionGetter(const Napi::CallbackInfo &info) {
  NAPI_UNWRAP_THIS(GeometryNapi, self);
  return Napi::Number::New(info.Env(), self->this_->getCoordinateDimension());
}
void GeometryNapi::coordinateDimensionSetter(const Napi::CallbackInfo &info, const Napi::Value &value) {
  NAPI_UNWRAP_THIS_VOID(GeometryNapi, self);
  if (!value.IsNumber()) {
    Napi::Error::New(info.Env(), "coordinateDimension must be a number").ThrowAsJavaScriptException();
    return;
  }
  self->this_->setCoordinateDimension(value.As<Napi::Number>().Int32Value());
}

} // namespace node_gdal
