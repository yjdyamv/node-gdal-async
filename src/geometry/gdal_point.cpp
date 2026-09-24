#include "gdal_point.hpp"
#include "../gdal_common.hpp"
#include "gdal_geometry.hpp"

#include <stdlib.h>

namespace node_gdal {

Napi::FunctionReference Point::constructor;

void Point::Initialize(Napi::Object target) {
  Napi::Env env = target.Env();
  SELF_CLASS(Point);

  // NOTE: the descriptor macros carry their own trailing comma
  Napi::Function lcons = DefineClass(env, "Point",
    {
        METHOD(toString)
        ATTR(lcons, "x", xGetter, xSetter)
        ATTR(lcons, "y", yGetter, ySetter)
        ATTR(lcons, "z", zGetter, zSetter)
    });

  // lcons->Inherit() has no DefineClass equivalent
  node_gdal::Inherit(lcons, Geometry::constructor.Value());

  // properties

  target.Set("Point", lcons);

  constructor = Napi::Persistent(lcons);
  constructor.SuppressDestruct();
}

/**
 * Point class.
 *
 * @constructor
 * @class Point
 * @extends Geometry
 * @param {number} x
 * @param {number} y
 * @param {number} [z]
 */
NAN_METHOD(Point::New) {
  Point *f;
  OGRPoint *geom;
  double x = 0, y = 0, z = 0;

  if (!info.IsConstructCall()) {
    Napi::Error::New(node_gdal::napi_env, "Cannot call constructor as function, you need to use 'new' keyword").ThrowAsJavaScriptException();
    return node_gdal::napi_env.Undefined();
  }

  if (info[0].IsExternal()) {
    Local<External> ext = info[0].As<External>();
    void *ptr = ext->Value(V8_TYPE_TAG);
    f = static_cast<Point *>(ptr);

  } else {
    NODE_ARG_DOUBLE_OPT(0, "x", x);
    NODE_ARG_DOUBLE_OPT(1, "y", y);
    NODE_ARG_DOUBLE_OPT(2, "z", z);

    if (info.Length() == 1) {
      Napi::Error::New(node_gdal::napi_env, "Point constructor must be given 0, 2, or 3 arguments").ThrowAsJavaScriptException();
      return node_gdal::napi_env.Undefined();
    }

    if (info.Length() == 3) {
      geom = new OGRPoint(x, y, z);
    } else {
      geom = new OGRPoint(x, y);
    }

    f = new Point(geom);
  }

  f->Wrap(info.This());
  return info.This();
}

NAN_METHOD(Point::toString) {
  return Napi::String::New(node_gdal::napi_env, "Point");
}

/**
 * @kind member
 * @name x
 * @instance
 * @memberof Point
 * @type {number}
 */
NAN_GETTER(Point::xGetter) {
  Point *geom = node_gdal::UnwrapWrapped<Point>(info.This().As<Napi::Object>());
  return Napi::Number::New(node_gdal::napi_env, (geom->this_)->getX());
}

NAN_SETTER(Point::xSetter) {
  Point *geom = node_gdal::UnwrapWrapped<Point>(info.This().As<Napi::Object>());

  if (!value->IsNumber()) {
    Napi::Error::New(node_gdal::napi_env, "y must be a number").ThrowAsJavaScriptException();
    return;
  }
  double x = value.As<Napi::Number>().DoubleValue().ToChecked();

  ((OGRPoint *)geom->this_)->setX(x);
}

/**
 * @kind member
 * @name y
 * @instance
 * @memberof Point
 * @type {number}
 */
NAN_GETTER(Point::yGetter) {
  Point *geom = node_gdal::UnwrapWrapped<Point>(info.This().As<Napi::Object>());
  return Napi::Number::New(node_gdal::napi_env, (geom->this_)->getY());
}

NAN_SETTER(Point::ySetter) {
  Point *geom = node_gdal::UnwrapWrapped<Point>(info.This().As<Napi::Object>());

  if (!value->IsNumber()) {
    Napi::Error::New(node_gdal::napi_env, "y must be a number").ThrowAsJavaScriptException();
    return;
  }
  double y = value.As<Napi::Number>().DoubleValue().ToChecked();

  ((OGRPoint *)geom->this_)->setY(y);
}

/**
 * @kind member
 * @name z
 * @instance
 * @memberof Point
 * @type {number}
 */
NAN_GETTER(Point::zGetter) {
  Point *geom = node_gdal::UnwrapWrapped<Point>(info.This().As<Napi::Object>());
  return Napi::Number::New(node_gdal::napi_env, (geom->this_)->getZ());
}

NAN_SETTER(Point::zSetter) {
  Point *geom = node_gdal::UnwrapWrapped<Point>(info.This().As<Napi::Object>());

  if (!value->IsNumber()) {
    Napi::Error::New(node_gdal::napi_env, "z must be a number").ThrowAsJavaScriptException();
    return;
  }
  double z = value.As<Napi::Number>().DoubleValue().ToChecked();

  ((OGRPoint *)geom->this_)->setZ(z);
}

} // namespace node_gdal
