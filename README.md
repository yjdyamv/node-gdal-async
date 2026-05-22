# @yjdyamv/gdal-async

Fork of node-gdal-async with N-API migration — fully async bindings to GDAL for Node.js 24+.

![logo](gdal-async-logo.svg)

[![License](https://img.shields.io/badge/License-Apache%202.0-blue.svg)](https://opensource.org/licenses/Apache-2.0)
![Node.js CI](https://github.com/yjdyamv/node-gdal-async/workflows/Node.js%20CI/badge.svg)

Read and write raster and vector geospatial datasets straight from [Node.js](http://nodejs.org) with this native N-API asynchronous [GDAL](http://www.gdal.org/) binding.

Key differences from upstream:
- **N-API** — ABI-stable across Node versions, no NAN dependency
- **Conan** — C++ dependency management via Conan 2.x
- **Node 24+** — targets modern Node.js LTS

---

## Installation

```sh
npm install @yjdyamv/gdal-async
```

Prebuilt binaries are available for **linux-x64**, **win32-x64**, and **darwin-arm64**.

To build from source:

```sh
npm install @yjdyamv/gdal-async --build-from-source
```

Requires [Conan](https://conan.io/) 2.x and CMake.

---

## API

API documentation: [https://mmomtchev.github.io/node-gdal-async/](https://mmomtchev.github.io/node-gdal-async/)

When in doubt, check the [unit tests](test/).

---

## Credits

Forked from [mmomtchev/node-gdal-async](https://github.com/mmomtchev/node-gdal-async).

Original contributors: Brandon Reavis, Momtchil Momtchev, and others.
