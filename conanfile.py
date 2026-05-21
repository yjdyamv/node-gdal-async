from conan import ConanFile
from conan.tools.cmake import CMakeToolchain, CMakeDeps
import os
import json


class NodeGdalAsyncConan(ConanFile):
    name = "gdal-async"
    version = "3.12.3"
    description = "Node.js bindings for GDAL with N-API"
    package_type = "application"
    settings = "os", "compiler", "build_type", "arch"
    generators = "CMakeToolchain", "CMakeDeps"

    default_options = {
        "gdal*:with_arrow": False,
        "gdal*:with_curl": True,
        "gdal*:with_expat": True,
        "gdal*:with_geos": True,
        "gdal*:with_gif": True,
        "gdal*:with_hdf5": False,
        "gdal*:with_jpeg": "libjpeg",
        "gdal*:with_lerc": True,
        "gdal*:with_libcsf": True,
        "gdal*:with_libdeflate": True,
        "gdal*:with_libkml": True,
        "gdal*:with_libiconv": False,
        "gdal*:with_netcdf": True,
        "gdal*:with_opencl": True,
        "gdal*:with_openjpeg": True,
        "gdal*:with_png": True,
        "gdal*:with_muparser": True,
        "gdal*:with_qhull": True,
        "gdal*:with_sqlite3": True,
        "gdal*:with_libaec": True,
        "gdal*:gdal_optional_drivers": True,
        "gdal*:ogr_optional_drivers": True,
        "gdal*:tools": False,
        "*:shared": False,
    }

    def requirements(self):
        # GDAL is the only required direct dependency; all other libs
        # (proj, geos, libaec, openjpeg, expat, sqlite3, curl, etc.)
        # come transitively through GDAL's options (with_geos, etc.)
        # CMakeDeps generates config files for transitive deps automatically
        self.requires("gdal/3.12.1")

    def generate(self):
        # Write data paths for dev-time GDAL_DATA / PROJ_LIB resolution
        data_paths = {}
        for dep_name in ("gdal", "proj"):
            try:
                dep = self.dependencies[dep_name]
                res_dir = os.path.join(dep.package_folder, "res")
                if dep_name == "gdal" and os.path.isdir(os.path.join(res_dir, "gdal")):
                    data_paths["GDAL_DATA"] = os.path.join(res_dir, "gdal")
                elif dep_name == "proj":
                    data_paths["PROJ_LIB"] = res_dir
            except Exception:
                pass

        if data_paths:
            # Output to conan/install which is the default search path in gdal.js
            output_dir = os.path.join(os.getcwd(), "conan", "install")
            os.makedirs(output_dir, exist_ok=True)
            paths_file = os.path.join(output_dir, "conan_data_paths.json")
            with open(paths_file, "w") as f:
                json.dump(data_paths, f)
