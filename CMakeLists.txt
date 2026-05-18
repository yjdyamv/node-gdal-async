cmake_minimum_required(VERSION 3.22)
project(gdal-async LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)

# --- Options ---
option(ENABLE_LOGGING "Enable debug logging" OFF)
option(ENABLE_ASAN "Enable AddressSanitizer" OFF)
option(ENABLE_COVERAGE "Enable code coverage" OFF)

# --- cmake-js ---
include_directories(${CMAKE_JS_INC})

# --- node-addon-api ---
execute_process(
  COMMAND node -p "require('node-addon-api').include"
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  OUTPUT_VARIABLE NODE_ADDON_API_DIR
  OUTPUT_STRIP_TRAILING_WHITESPACE
)
# --- nan (still needed until Item 3 N-API migration) ---
execute_process(
  COMMAND node -p "require('nan')"
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  OUTPUT_VARIABLE NAN_DIR
  OUTPUT_STRIP_TRAILING_WHITESPACE
)
include_directories(${NODE_ADDON_API_DIR})
include_directories(${NAN_DIR})
include_directories(${CMAKE_SOURCE_DIR}/include)

# --- Platform settings ---
if(WIN32)
  set(PLATFORM "win")
  add_definitions(-DNOMINMAX -DWIN32 -DPLATFORM="win")
  add_definitions(-D_HAS_EXCEPTIONS=1)
  add_definitions(-D_CRT_SECURE_NO_WARNINGS)
  # Force dynamic CRT (/MD) to match vcpkg DLLs; cmake-js defaults to static CRT (/MT)
  set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL" CACHE STRING "" FORCE)
elseif(APPLE)
  set(PLATFORM "mac")
  add_definitions(-DPLATFORM="mac" -DDARWIN)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -frtti -fexceptions")
  set(CMAKE_OSX_DEPLOYMENT_TARGET "13.0")
else()
  set(PLATFORM "linux")
  add_definitions(-DPLATFORM="linux" -DLINUX)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -frtti -fexceptions")
  set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,--exclude-libs,ALL")
endif()

add_definitions(-D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64 -DNOGDI=1 -DHAVE_LIBZ=1)

if(ENABLE_LOGGING)
  add_definitions(-DENABLE_LOGGING=1)
endif()

# --- vcpkg dependencies ---
find_package(GDAL CONFIG REQUIRED)
find_package(PROJ CONFIG REQUIRED)
find_package(GEOS CONFIG REQUIRED)
find_package(CURL REQUIRED)
find_package(EXPAT REQUIRED)
find_package(SQLite3 REQUIRED)
find_package(HDF5 REQUIRED)
find_package(netCDF CONFIG REQUIRED)
find_package(OpenJPEG REQUIRED)
find_package(libaec CONFIG REQUIRED)
find_package(muparser CONFIG REQUIRED)

# --- Sources ---
set(SOURCES
  src/utils/typed_array.cpp
  src/utils/string_list.cpp
  src/utils/number_list.cpp
  src/utils/warp_options.cpp
  src/utils/ptr_manager.cpp
  src/node_gdal.cpp
  src/async.cpp
  src/gdal_common.cpp
  src/gdal_dataset.cpp
  src/gdal_driver.cpp
  src/gdal_driver_napi.cpp
  src/gdal_rasterband.cpp
  src/gdal_group.cpp
  src/gdal_mdarray.cpp
  src/gdal_dimension.cpp
  src/gdal_attribute.cpp
  src/gdal_majorobject.cpp
  src/gdal_feature.cpp
  src/gdal_feature_defn.cpp
  src/gdal_feature_defn_napi.cpp
  src/gdal_field_defn.cpp
  src/gdal_field_defn_napi.cpp
  src/geometry/gdal_geometry.cpp
  src/geometry/gdal_point.cpp
  src/geometry/gdal_simplecurve.cpp
  src/geometry/gdal_compoundcurve.cpp
  src/geometry/gdal_linestring.cpp
  src/geometry/gdal_circularstring.cpp
  src/geometry/gdal_linearring.cpp
  src/geometry/gdal_polygon.cpp
  src/geometry/gdal_geometrycollection.cpp
  src/geometry/gdal_multipoint.cpp
  src/geometry/gdal_multilinestring.cpp
  src/geometry/gdal_multicurve.cpp
  src/geometry/gdal_multipolygon.cpp
  src/gdal_layer.cpp
  src/gdal_coordinate_transformation.cpp
  src/gdal_coordinate_transformation_napi.cpp
  src/gdal_spatial_reference.cpp
  src/gdal_warper.cpp
  src/gdal_algorithms.cpp
  src/gdal_algebra.cpp
  src/gdal_memfile.cpp
  src/gdal_utils.cpp
  src/gdal_fs.cpp
  src/collections/dataset_bands.cpp
  src/collections/dataset_layers.cpp
  src/collections/layer_features.cpp
  src/collections/layer_fields.cpp
  src/collections/group_groups.cpp
  src/collections/group_arrays.cpp
  src/collections/group_attributes.cpp
  src/collections/group_dimensions.cpp
  src/collections/array_dimensions.cpp
  src/collections/array_attributes.cpp
  src/collections/feature_fields.cpp
  src/collections/feature_defn_fields.cpp
  src/collections/geometry_collection_children.cpp
  src/collections/polygon_rings.cpp
  src/collections/linestring_points.cpp
  src/collections/compound_curves.cpp
  src/collections/rasterband_overviews.cpp
  src/collections/rasterband_pixels.cpp
  src/collections/gdal_drivers.cpp
  src/collections/colortable.cpp
  src/collections/colortable_napi.cpp
)

# --- Build the addon ---
add_library(gdal SHARED ${SOURCES})

target_include_directories(gdal PRIVATE ${NODE_ADDON_API_DIR})
if(MSVC)
  target_compile_options(gdal PRIVATE "/I${NODE_ADDON_API_DIR}")
endif()

set_target_properties(gdal PROPERTIES
  PREFIX ""
  SUFFIX ".node"
  CXX_STANDARD 20
  CXX_STANDARD_REQUIRED ON
  CXX_EXTENSIONS ON
)

target_link_libraries(gdal PRIVATE
  GDAL::GDAL
  PROJ::proj
  GEOS::geos_c
  CURL::libcurl
  EXPAT::EXPAT
  SQLite::SQLite3
  hdf5::hdf5-shared
  netCDF::netcdf
  openjp2
  libaec::aec
  muparser::muparser
)

if(WIN32)
  target_link_libraries(gdal PRIVATE secur32 odbccp32 odbc32 crypt32 ws2_32 advapi32 wbemuuid Shlwapi)
endif()

if(APPLE)
  target_link_libraries(gdal PRIVATE "-framework Security")
  set_target_properties(gdal PROPERTIES
    LINK_FLAGS "-Wl,-bind_at_load"
  )
endif()

# --- Compile options ---
target_compile_definitions(gdal PRIVATE
  $<$<CONFIG:Debug>:DEBUG>
  $<$<CONFIG:Release>:NDEBUG>
)

if(ENABLE_ASAN)
  target_compile_options(gdal PRIVATE -fsanitize=address)
  target_link_options(gdal PRIVATE -fsanitize=address)
endif()

if(ENABLE_COVERAGE)
  target_compile_options(gdal PRIVATE -fprofile-arcs -ftest-coverage)
  target_link_options(gdal PRIVATE -lgcov --coverage)
endif()

if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
  target_compile_options(gdal PRIVATE /MP /GR /EHsc /permissive- /Zc:__cplusplus /utf-8 /GS /guard:cf /W4 -WX-)
  # Security hardening
  target_link_options(gdal PRIVATE /NXCOMPAT /DYNAMICBASE /GUARD:CF)
  # Export all symbols for DLL
  set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
else()
  target_compile_options(gdal PRIVATE -Wall -Wextra -Wpedantic -Wno-cast-function-type-mismatch)
endif()

# --- Post-build: copy to lib/binding ---
# Determine the node ABI and platform for the binding directory
execute_process(
  COMMAND node -p "process.versions.modules"
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  OUTPUT_VARIABLE NODE_ABI
  OUTPUT_STRIP_TRAILING_WHITESPACE
)
execute_process(
  COMMAND node -p "process.platform"
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  OUTPUT_VARIABLE NODE_PLATFORM
  OUTPUT_STRIP_TRAILING_WHITESPACE
)
execute_process(
  COMMAND node -p "process.arch"
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  OUTPUT_VARIABLE NODE_ARCH
  OUTPUT_STRIP_TRAILING_WHITESPACE
)
set(BINDING_DIR "lib/binding/node-v${NODE_ABI}-${NODE_PLATFORM}-${NODE_ARCH}")

# Gather shared libraries that need to be alongside gdal.node
set(VCPKG_BIN_DIR "${CMAKE_BINARY_DIR}/vcpkg_installed/${VCPKG_TARGET_TRIPLET}/bin")
if(WIN32)
  file(GLOB VCPKG_DLLS "${VCPKG_BIN_DIR}/*.dll")
  # Do not filter DLLs at configure time; all needed DLLs are copied
  # Debug DLLs (e.g. gdald.dll) won't be loaded at runtime in Release builds
endif()

add_custom_command(TARGET gdal POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_SOURCE_DIR}/${BINDING_DIR}"
  COMMAND ${CMAKE_COMMAND} -E copy "$<TARGET_FILE:gdal>" "${CMAKE_SOURCE_DIR}/${BINDING_DIR}/gdal.node"
  COMMAND ${CMAKE_COMMAND} -E copy ${VCPKG_DLLS} "${CMAKE_SOURCE_DIR}/${BINDING_DIR}/"
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  COMMENT "Copying gdal.node and dependencies to ${BINDING_DIR}"
)

# RPATH settings so .node finds .so/.dylib relative to itself (Linux/macOS)
if(NOT WIN32)
  if(APPLE)
    set_target_properties(gdal PROPERTIES
      INSTALL_RPATH "@loader_path"
      BUILD_RPATH "${VCPKG_BIN_DIR}"
    )
  else()
    set_target_properties(gdal PROPERTIES
      INSTALL_RPATH "$ORIGIN"
      BUILD_RPATH "${VCPKG_BIN_DIR}"
    )
  endif()
endif()

# --- cmake-js Node.js integration ---
target_link_libraries(gdal PRIVATE ${CMAKE_JS_LIB})
