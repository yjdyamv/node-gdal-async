# N-API migration — working notes

Goal: port `gdal-async` from NAN to N-API (node-addon-api).

## Verified baseline (NAN, unmodified C++)

* Built against **GDAL 3.12.2** (system, `--shared_gdal` equivalent) + **Node 24.19.0** headers, WSL Linux.
* Compiles: 61/61 translation units.
* Links and loads.
* Test suite: **1453 passing, 42 pending, 1 failing**
  * the single failure is `test/open_vsicurl.test.ts` ("HTTP response code: 0") — requires network.

Reproduce:
```
.commandcode/wsl/setup.sh          # one-off: fetch toolchain (no root needed)
.commandcode/wsl/run.sh build      # ~30s, parallel
.commandcode/wsl/run.sh test       # ~1min
```

## Scale of the migration

| item | count |
|---|---|
| `Nan::*` call sites | 3267 across 118 files |
| `NAN_METHOD` / `NAN_GETTER` / `NAN_SETTER` | 454 / 169 / 53 |
| `Nan::ObjectWrap` | 328 |
| `Nan::New` / `ThrowError` / `Set` / `Persistent` | 1139 / 424 / 331 / 122 |
| `Nan::SetPrivate`+`GetPrivate` | 141 |
| `Nan::AsyncWorker` family | `async.hpp` (1 file) |

## Design decisions

* Methods stay **static** and are registered as *instance* methods. node-addon-api's
  `InstanceMethod` only accepts member-function pointers, but
  `Napi::ClassPropertyDescriptor<T>` has a **public ctor from `napi_property_descriptor`**,
  so a trampoline over a plain function pointer works and 676 signatures stay put.
* Class construction switches from `new T(x)` + `Nan::NewInstance(External)` to
  `T(const Napi::CallbackInfo&)` reading `Napi::External<T>` (node-addon-api's
  `ObjectWrap` allocates the native object itself).
* `Nan::SetPrivate/GetPrivate` → `Napi::Symbol` property keys (N-API has no private symbols).
* `Nan::Persistent<Object>` in `ObjectStore` → `Napi::Reference<Object>` (needs an explicit `Env`).
* `Nan::AsyncProgressWorkerBase<GDALProgressInfo>` → `Napi::AsyncProgressQueueWorker<GDALProgressInfo>`
  (`Execute` / `OnProgress` / `OnOK` / `OnError` map 1:1).

## Constraint that forces a "big bang"

NAN and N-API cannot coexist in one translation unit (NAN hands out `v8::Local<>`,
node-addon-api never exposes v8 handles). Since `gdal_common.hpp` is shared by all 118
files, the macro layer and every file must land together; the tree will not compile
until the conversion is complete.

## WIP

Ported and syntax-verified against node-addon-api 8.5.0 / Node 24:

* `gdal_common.hpp` — macro layer complete, including the `NODE_WRAPPED_ASYNC_*`,
  `*_CPLERR_*`, `*_OGRERR_*`, `NODE_UNWRAP_CHECK`, `IS_WRAPPED` and `ATTR*` families.
  * `napi_non_enumerable` does not exist in N-API; the non-hidden descriptor
    helpers now set `napi_enumerable` and the hidden ones rely on the N-API
    default. This matches NAN, where `Nan::SetPrototypeMethod`/`SetAccessor`
    were enumerable and only the `*Async`/`*_DONT_ENUM` accessors were `DontEnum`.
  * `extern Napi::Env napi_env` moved to the top of the file, so the trampolines
    now spell the type `::napi_env` (it is shadowed by the variable).
  * `SafeString::New(const char*)` kept as an overload over the ambient env.
* `utils/ptr_manager.{hpp,cpp}` — `Napi::Reference<Napi::Object>` replaces
  `Nan::Persistent<Object>`, `ObjectStore::get()` returns `Napi::Object`.
  `<uv.h>` is now included explicitly (NAN used to pull it in via `nan.h`).
* `async.{hpp,cpp}` — `Napi::AsyncProgressQueueWorker<GDALProgressInfo>`.
  * node-addon-api's `AsyncWorker` has neither `SaveToPersistent`/`GetFromPersistent`
    nor a promise form, so the worker owns a
    `std::map<std::string, Napi::Reference<Napi::Object>>` and the promise worker
    wraps a `Napi::Promise::Deferred`.
  * The result is delivered through `GetResult()` (callback) or `OnOK`/`OnError`
    (promise). `Reference::Reset()` before `delete` because the argument macros
    call `SuppressDestruct()`.
* `node_gdal.cpp` — `NODE_API_MODULE` + `Napi::Object Init(Napi::Env, Napi::Object)`,
  defines `node_gdal::napi_env`. No errors left inside the file itself.

Harness:

* `.commandcode/wsl/check.sh <files...>` — per-TU syntax check with
  `-Werror=return-type` (catches NAN methods that used `info.GetReturnValue()`
  and now must `return`).
* `.commandcode/wsl/nan2napi.py` — the mechanical substitutions; `--env` picks
  `info.Env()` (method bodies, default) or `env` (module init).
* `.commandcode/wsl/napi_header.py` — converts a plain `ObjectWrap` class header
  (see "Class conversion shape"); `napi_header_fix.py` repairs its output.
* `.commandcode/wsl/check_headers.sh <hpp...>` — per-header syntax check, by
  compiling a throwaway TU that includes exactly one header. This is how the
  header phase is verified without the (still NAN) `.cpp` bodies.

`Driver` is converted end to end and is the reference for the other 28 classes:
`src/gdal_driver.{hpp,cpp}` plus the headers in its include closure
(`gdal_dataset.hpp`, `gdal_majorobject.hpp`, `utils/string_list.hpp`).

## Order matters: headers before bodies

The `NAN_METHOD`/`NAN_GETTER`/`NAN_SETTER` names are defined by **both** `nan.h`
and `gdal_common.hpp`; whichever is included last wins. Worse, `nan.h` uses its
own `NAN_GETTER_ARGS_TYPE`/`NAN_GETTER_RETURN_TYPE` typedefs, so redefining those
in `gdal_common.hpp` breaks `nan.h` itself (they were unused and are now gone).
Consequence: as long as *any* header in a translation unit still includes
`nan-wrapper.h`, the macro layer gets clobbered and every check is noise.

Therefore: convert all headers first, so `nan.h` leaves every include closure,
and only then convert the `.cpp` bodies. Once a file's closure is NAN-free,
`check.sh` gives real, actionable errors.

## Class conversion shape

`.hpp`:

* `class X : public Nan::ObjectWrap` → `class X : public Napi::ObjectWrap<X>`
* `static Nan::Persistent<FunctionTemplate> constructor;` → `static Napi::FunctionReference constructor;`
* `static void Initialize(Local<Object> target);` → `static void Initialize(Napi::Object target);`
* `static NAN_METHOD(New);` → gone (the `ObjectWrap` constructor *is* the JS constructor)
* `X(); X(GDALX *x);` → `X(const Napi::CallbackInfo &info);`
* `private: ~X();` → `~X();` **in the public section** — node-addon-api's finalizer
  does `delete static_cast<T *>(instance)`, so a private destructor does not compile
  (NAN got away with it because deletion went through the base's virtual destructor)
* `static Local<Value> New(...)` → `static Napi::Value New(...)`, and it keeps its
  single argument: it reads the ambient `node_gdal::napi_env`, which keeps the ~200
  `X::New(ptr)` call sites unchanged
* drop `<node.h>`/`<node_object_wrap.h>`/`nan-wrapper.h` and `using namespace v8; using namespace node;`

`.cpp`:

* `Initialize` changes from statements to a descriptor list:
  `Nan::SetPrototypeMethod(lcons, "m", m);` → `METHOD(m)`,
  `Nan__SetPrototypeAsyncableMethod(lcons, "m", m);` → `METHOD_ASYNCABLE(m)`,
  `ATTR(lcons, ...)` stays as it is; all of them hand their own **trailing comma**
  to `DefineClass(env, "X", { ... })`, so the call sites must not add one
* `SELF_CLASS(X);` before `DefineClass` (the descriptor macros reference `SELF`)
* `constructor = Napi::Persistent(func); constructor.SuppressDestruct();`
* factory: `new X(x)` + `Nan::NewInstance(...)` → `constructor.Value().New({Napi::External<T>::New(env, x)})`,
  then `wrapped->uid = object_store.add(x, *wrapped, parent_uid)` (`ObjectWrap` *is* a
  `Reference<Object>`, populated by `napi_wrap` with a weak reference)
* the `.cpp` constructor reads `info[0].As<Napi::External<T>>().Data()` and rejects
  non-`new` / non-`External` calls
* `x->handle()` → `x->Value()` (`Nan::ObjectWrap::handle()` returned the JS object)
* `x->persistent()` → `*x`

`gdal_common.hpp` fixes needed by the above: `UnwrapWrapped`/`IsInstanceOf`/
`RejectPromise` were declared at global scope while the macros reference them as
`node_gdal::...` — they are now inside the namespace.

## Header phase: complete

All 53 headers parse standalone (`.commandcode/wsl/check_headers.sh`), and the
only `Nan::` left in any header is in two comments in `gdal_common.hpp`.

What the header phase turned up, beyond the mechanical rewriting:

* **Class headers must include `gdal_common.hpp`, not `napi-wrapper.h`.** They used
  to get `NAN_METHOD`/`NAN_GETTER` from `nan.h`; in the N-API layer those macros
  live in `gdal_common.hpp`.
* **`GDALObject<T>`**: node-addon-api's own `ObjectWrap<T>` constructor trampoline
  calls `new T(callbackInfo)` and intercepts a constructor invoked without `new`,
  but throws a message the test suite does not accept (`api_classes.test.ts` wants
  `/Cannot call constructor/`). Every class now derives from
  `GDALObject<T>` (`gdal_common.hpp`) instead of `Napi::ObjectWrap<T>` directly,
  which is the one place that message lives.
* **Constructors are the JS constructors.** node-addon-api allocates the native
  instance itself, so the NAN `new T(x)` + `Nan::NewInstance(External)` pattern is
  inverted: the factory passes `Napi::External<GDALX *>` and the constructor reads
  it. `Nan::ObjectWrap::handle()` becomes `ObjectWrap::Value()`.
* **Not-constructible classes must throw.** `api_classes.test.ts` walks every
  exported class and asserts `new gdal.X()` throws `/Cannot create .* directly/`
  (and `gdal.Dataset()` throws `Cannot create dataset directly`), while the
  array-valued entries really are constructed (`new gdal.Point(0, 0)`).
  * classes built from a GDAL pointer guard on the External;
  * the group/array collections now receive their two parents as *constructor
    arguments* so the constructor can tell an internal construction from a direct
    `new gdal.X()`;
  * `Geometry` and `SimpleCurve` are abstract in JS and get their own throwing
    constructors, with the original messages ("Geometry doesnt have a
    constructor, ..." and "SimpleCurve is an abstract class and cannot be
    instantiated");
  * `Point` gets its own constructor because it accepts `(x, y[, z])`.
* **`::napi_env`** is required wherever the *type* is spelled inside
  `namespace node_gdal` - `node_gdal::napi_env` shadows it (the trampolines and
  `Memfile::finalize`).
* `geometry/`'s `GeometryBase<T, OGRT>` CRTP hierarchy maps onto
  `GDALObject<T>` one to one, because every concrete class passes itself as `T`.

## Still open, discovered in the header phase

* **JS prototype chains.** `Nan::Inherit()`/`lcons->Inherit()` has no counterpart
  in `Napi::ObjectWrap::DefineClass`. `api_classes.test.ts` asserts
  `assert.instanceOf(o, gdal.Geometry)` (and `gdal.SimpleCurve`,
  `gdal.GeometryCollection`, ...), so every derived class' `Initialize` has to
  chain both `prototype` and the constructor object onto the base's:
  `lcons.Get("prototype").As<Object>().SetPrototypeOf(base.Get("prototype").As<Object>())`
  plus `lcons.SetPrototypeOf(base)`.
* `Memfile`'s anonymous-buffer cleanup becomes `napi_add_finalizer` (the N-API
  equivalent of the NAN weak callback); named buffers keep the strong reference.

Not started: every `.cpp` body - each `Initialize` (statement-style registration →
`DefineClass` descriptor list + prototype chaining), each factory, and the
`utils/` bodies (`string_list.cpp`, `typed_array.cpp`, `warp_options.cpp`,
`number_list.cpp`).
`Nan::` call sites remaining: ~2000 across ~57 `.cpp` files.



Known follow-ups:

* `Nan::AddGCPrologueCallback`/`AddGCEpilogueCallback` in `node_gdal.cpp` are
  still NAN — only compiled under `ENABLE_LOGGING`, which is off.
* `node_gdal.cpp` still includes `<node.h>`/`<node_buffer.h>`; `node_buffer.h`
  can go once `rasterband_pixels` no longer uses `Nan::NewBuffer`.


