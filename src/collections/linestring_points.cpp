#include "linestring_points.hpp"
#include "../gdal_common.hpp"
#include "../geometry/gdal_geometry.hpp"
#include "../geometry/gdal_linestring.hpp"
#include "../geometry/gdal_point.hpp"

namespace node_gdal {

Napi::FunctionReference LineStringPoints::constructor;

void LineStringPoints::Initialize(Napi::Object target) {
  Napi::Env env = target.Env();
  SELF_CLASS(LineStringPoints);

  // NOTE: the descriptor macros carry their own trailing comma
  Napi::Function lcons = DefineClass(env, "LineStringPoints",
    {
        METHOD(toString)
        METHOD(count)
        METHOD(get)
        METHOD(set)
        METHOD(add)
        METHOD(reverse)
        METHOD(resize)
    });

  target.Set("LineStringPoints", lcons);

  constructor = Napi::Persistent(lcons);
  constructor.SuppressDestruct();
}

LineStringPoints::LineStringPoints() : Nan::ObjectWrap() {
}

LineStringPoints::~LineStringPoints() {
}

/**
 * An encapsulation of a {@link LineString}'s points.
 *
 * @class LineStringPoints
 */
NAN_METHOD(LineStringPoints::New) {

  if (!info.IsConstructCall()) {
    Napi::Error::New(node_gdal::napi_env, "Cannot call constructor as function, you need to use 'new' keyword").ThrowAsJavaScriptException();
    return node_gdal::napi_env.Undefined();
  }
  if (info[0].IsExternal()) {
    Local<External> ext = info[0].As<External>();
    void *ptr = ext->Value(V8_TYPE_TAG);
    LineStringPoints *geom = static_cast<LineStringPoints *>(ptr);
    geom->Wrap(info.This());
    return info.This();
    return node_gdal::napi_env.Undefined();
  } else {
    Napi::Error::New(node_gdal::napi_env, "Cannot create LineStringPoints directly").ThrowAsJavaScriptException();
    return node_gdal::napi_env.Undefined();
  }
}

Napi::Value LineStringPoints::New(Napi::Value geom) {

  LineStringPoints *wrapped = new LineStringPoints();

  Napi::Value ext = Nan::New<External>(wrapped);
  v8::Local<v8::Object> obj =
    Nan::NewInstance(Nan::GetFunction(Napi::String::New(node_gdal::napi_env, LineStringPoints::constructor)), 1, &ext)
      .ToLocalChecked();
  Nan::SetPrivate(obj, Napi::String::New(node_gdal::napi_env, "parent_"), geom);

  return obj;
}

NAN_METHOD(LineStringPoints::toString) {
  return Napi::String::New(node_gdal::napi_env, "LineStringPoints");
}

/**
 * Returns the number of points that are part of the line string.
 *
 * @method count
 * @instance
 * @memberof LineStringPoints
 * @return {number}
 */
NAN_METHOD(LineStringPoints::count) {

  Napi::Object parent =
    Nan::GetPrivate(info.This(), Napi::String::New(node_gdal::napi_env, "parent_")).ToLocalChecked().As<Object>();
  LineString *geom = node_gdal::UnwrapWrapped<LineString>(parent);

  return Napi::Number::New(node_gdal::napi_env, geom->get()->getNumPoints());
}

/**
 * Reverses the order of all the points.
 *
 * @method reverse
 * @instance
 * @memberof LineStringPoints
 */
NAN_METHOD(LineStringPoints::reverse) {

  Napi::Object parent =
    Nan::GetPrivate(info.This(), Napi::String::New(node_gdal::napi_env, "parent_")).ToLocalChecked().As<Object>();
  LineString *geom = node_gdal::UnwrapWrapped<LineString>(parent);

  geom->get()->reversePoints();

  return node_gdal::napi_env.Undefined();
}

/**
 * Adjusts the number of points that make up the line string.
 *
 * @method resize
 * @instance
 * @memberof LineStringPoints
 * @param {number} count
 */
NAN_METHOD(LineStringPoints::resize) {

  Napi::Object parent =
    Nan::GetPrivate(info.This(), Napi::String::New(node_gdal::napi_env, "parent_")).ToLocalChecked().As<Object>();
  LineString *geom = node_gdal::UnwrapWrapped<LineString>(parent);

  int count;
  NODE_ARG_INT(0, "point count", count)
  geom->get()->setNumPoints(count);

  return node_gdal::napi_env.Undefined();
}

/**
 * Returns the point at the specified index.
 *
 * @method get
 * @instance
 * @memberof LineStringPoints
 * @param {number} index 0-based index
 * @throws {Error}
 * @return {Point}
 */
NAN_METHOD(LineStringPoints::get) {

  Napi::Object parent =
    Nan::GetPrivate(info.This(), Napi::String::New(node_gdal::napi_env, "parent_")).ToLocalChecked().As<Object>();
  LineString *geom = node_gdal::UnwrapWrapped<LineString>(parent);

  OGRPoint pt;
  int i;

  NODE_ARG_INT(0, "index", i);
  if (i < 0 || i >= geom->get()->getNumPoints()) {
    Napi::Error::New(node_gdal::napi_env, "Invalid point requested").ThrowAsJavaScriptException();
    return node_gdal::napi_env.Undefined();
  }

  geom->get()->getPoint(i, &pt);

  // New will copy the point with GDAL clone()
  return Point::New(&pt, false);
}

/**
 * Sets the point at the specified index.
 *
 * @example
 *
 * lineString.points.set(0, new gdal.Point(1, 2));
 *
 * @method set
 * @instance
 * @memberof LineStringPoints
 * @throws {Error}
 * @param {number} index 0-based index
 * @param {Point|xyz} point
 */

/**
 * @method set
 * @instance
 * @memberof LineStringPoints
 * @throws {Error}
 * @param {number} index 0-based index
 * @param {number} x
 * @param {number} y
 * @param {number} [z]
 */
NAN_METHOD(LineStringPoints::set) {

  Napi::Object parent =
    Nan::GetPrivate(info.This(), Napi::String::New(node_gdal::napi_env, "parent_")).ToLocalChecked().As<Object>();
  LineString *geom = node_gdal::UnwrapWrapped<LineString>(parent);

  int i;
  NODE_ARG_INT(0, "index", i);
  if (i < 0 || i >= geom->get()->getNumPoints()) {
    Napi::Error::New(node_gdal::napi_env, "Point index out of range").ThrowAsJavaScriptException();
    return node_gdal::napi_env.Undefined();
  }

  int n = info.Length() - 1;

  if (n == 0) {
    Napi::Error::New(node_gdal::napi_env, "Point must be given").ThrowAsJavaScriptException();
    return node_gdal::napi_env.Undefined();
  } else if (n == 1) {
    if (!info[1].IsObject()) {
      Napi::Error::New(node_gdal::napi_env, "Point or object expected for second argument").ThrowAsJavaScriptException();
      return node_gdal::napi_env.Undefined();
    }
    if (IS_WRAPPED(info[1], Point)) {
      // set from Point object
      Point *pt = node_gdal::UnwrapWrapped<Point>(info[1].As<Object>());
      geom->get()->setPoint(i, pt->get());
    } else {
      Napi::Object obj = info[1].As<Object>();
      // set from object {x: 0, y: 5}
      double x, y;
      NODE_DOUBLE_FROM_OBJ(obj, "x", x);
      NODE_DOUBLE_FROM_OBJ(obj, "y", y);

      Local<String> z_prop_name = Napi::String::New(node_gdal::napi_env, "z");
      if (Nan::HasOwnProperty(obj, z_prop_name).FromMaybe(false)) {
        Napi::Value z_val = Nan::Get(obj, z_prop_name).ToLocalChecked();
        if (!z_val->IsNumber()) {
          Napi::Error::New(node_gdal::napi_env, "z property must be number").ThrowAsJavaScriptException();
          return node_gdal::napi_env.Undefined();
        }
        geom->get()->setPoint(i, x, y, Nan::To<double>(z_val).ToChecked());
      } else {
        geom->get()->setPoint(i, x, y);
      }
    }
  } else {
    // set x, y, z from numeric arguments
    if (!info[1].IsNumber()) {
      Napi::Error::New(node_gdal::napi_env, "Number expected for second argument").ThrowAsJavaScriptException();
      return node_gdal::napi_env.Undefined();
    }
    if (!info[2].IsNumber()) {
      Napi::Error::New(node_gdal::napi_env, "Number expected for third argument").ThrowAsJavaScriptException();
      return node_gdal::napi_env.Undefined();
    }
    if (n == 2) {
      geom->get()->setPoint(i, Nan::To<double>(info[1]).ToChecked(), Nan::To<double>(info[2]).ToChecked());
    } else {
      if (!info[3].IsNumber()) {
        Napi::Error::New(node_gdal::napi_env, "Number expected for fourth argument").ThrowAsJavaScriptException();
        return node_gdal::napi_env.Undefined();
      }

      geom->get()->setPoint(
        i,
        Nan::To<double>(info[1]).ToChecked(),
        Nan::To<double>(info[2]).ToChecked(),
        Nan::To<double>(info[3]).ToChecked());
    }
  }

  return node_gdal::napi_env.Undefined();
}

/**
 * Adds point(s) to the line string. Also accepts any object with an x and y
 * property.
 *
 * @example
 *
 * lineString.points.add(new gdal.Point(1, 2));
 * lineString.points.add([
 *     new gdal.Point(1, 2)
 *     new gdal.Point(3, 4)
 * ]);
 *
 * @method add
 * @instance
 * @memberof LineStringPoints
 * @throws {Error}
 * @param {Point|xyz|(Point|xyz)[]} points
 */

/**
 *
 * @method add
 * @instance
 * @memberof LineStringPoints
 * @throws {Error}
 * @param {number} x
 * @param {number} y
 * @param {number} [z]
 */
NAN_METHOD(LineStringPoints::add) {

  Napi::Object parent =
    Nan::GetPrivate(info.This(), Napi::String::New(node_gdal::napi_env, "parent_")).ToLocalChecked().As<Object>();
  LineString *geom = node_gdal::UnwrapWrapped<LineString>(parent);

  int n = info.Length();

  if (n == 0) {
    Napi::Error::New(node_gdal::napi_env, "Point must be given").ThrowAsJavaScriptException();
    return node_gdal::napi_env.Undefined();
  } else if (n == 1) {
    if (!info[0].IsObject()) {
      Napi::Error::New(node_gdal::napi_env, "Point, object, or array of points expected").ThrowAsJavaScriptException();
      return node_gdal::napi_env.Undefined();
    }
    if (IS_WRAPPED(info[0], Point)) {
      // set from Point object
      Point *pt = node_gdal::UnwrapWrapped<Point>(info[0].As<Object>());
      geom->get()->addPoint(pt->get());
    } else if (info[0].IsArray()) {
      // set from array of points
      Napi::Array array = info[0].As<Array>();
      int length = array->Length();
      for (int i = 0; i < length; i++) {
        Napi::Value element = Nan::Get(array, i).ToLocalChecked();
        if (!element->IsObject()) {
          Napi::Error::New(node_gdal::napi_env, "All points must be Point objects or objects").ThrowAsJavaScriptException();
          return node_gdal::napi_env.Undefined();
        }
        Napi::Object element_obj = element.As<Object>();
        if (IS_WRAPPED(element_obj, Point)) {
          // set from Point object
          Point *pt = node_gdal::UnwrapWrapped<Point>(element_obj);
          geom->get()->addPoint(pt->get());
        } else {
          // set from object {x: 0, y: 5}
          double x, y;
          NODE_DOUBLE_FROM_OBJ(element_obj, "x", x);
          NODE_DOUBLE_FROM_OBJ(element_obj, "y", y);

          Local<String> z_prop_name = Napi::String::New(node_gdal::napi_env, "z");
          if (Nan::HasOwnProperty(element_obj, z_prop_name).FromMaybe(false)) {
            Napi::Value z_val = Nan::Get(element_obj, z_prop_name).ToLocalChecked();
            if (!z_val->IsNumber()) {
              Napi::Error::New(node_gdal::napi_env, "z property must be number").ThrowAsJavaScriptException();
              return node_gdal::napi_env.Undefined();
            }
            geom->get()->addPoint(x, y, Nan::To<double>(z_val).ToChecked());
          } else {
            geom->get()->addPoint(x, y);
          }
        }
      }
    } else {
      // set from object {x: 0, y: 5}
      Napi::Object obj = info[0].As<Object>();
      double x, y;
      NODE_DOUBLE_FROM_OBJ(obj, "x", x);
      NODE_DOUBLE_FROM_OBJ(obj, "y", y);

      Local<String> z_prop_name = Napi::String::New(node_gdal::napi_env, "z");
      if (Nan::HasOwnProperty(obj, z_prop_name).FromMaybe(false)) {
        Napi::Value z_val = Nan::Get(obj, z_prop_name).ToLocalChecked();
        if (!z_val->IsNumber()) {
          Napi::Error::New(node_gdal::napi_env, "z property must be number").ThrowAsJavaScriptException();
          return node_gdal::napi_env.Undefined();
        }
        geom->get()->addPoint(x, y, Nan::To<double>(z_val).ToChecked());
      } else {
        geom->get()->addPoint(x, y);
      }
    }
  } else {
    // set x, y, z from numeric arguments
    if (!info[0].IsNumber()) {
      Napi::Error::New(node_gdal::napi_env, "Number expected for first argument").ThrowAsJavaScriptException();
      return node_gdal::napi_env.Undefined();
    }
    if (!info[1].IsNumber()) {
      Napi::Error::New(node_gdal::napi_env, "Number expected for second argument").ThrowAsJavaScriptException();
      return node_gdal::napi_env.Undefined();
    }
    if (n == 2) {
      geom->get()->addPoint(Nan::To<double>(info[0]).ToChecked(), Nan::To<double>(info[1]).ToChecked());
    } else {
      if (!info[2].IsNumber()) {
        Napi::Error::New(node_gdal::napi_env, "Number expected for third argument").ThrowAsJavaScriptException();
        return node_gdal::napi_env.Undefined();
      }

      geom->get()->addPoint(
        Nan::To<double>(info[0]).ToChecked(),
        Nan::To<double>(info[1]).ToChecked(),
        Nan::To<double>(info[2]).ToChecked());
    }
  }

  return node_gdal::napi_env.Undefined();
}

} // namespace node_gdal
