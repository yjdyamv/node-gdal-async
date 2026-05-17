cmake_minimum_required(VERSION 3.22)
project(gdal-async LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)

# --- Options ---
option(ENABLE_LOGGING "Enable debug logging" OFF)
option(ENABLE_ASAN "Enable AddressSanitizer" OFF)
option(ENABLE_COVERAGE "Enable code coverage" OFF)

# --- cmake-js ---
include(${CMAKE_JS_INC})

# --- node-addon-api ---
execute_process(
  COMMAND node -p "require('node-addon-api').include"
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  OUTPUT_VARIABLE NODE_ADDON_API_DIR
  OUTPUT_STRIP_TRAILING_WHITESPACE
)
include_directories(${CMAKE_JS_INC})
include_directories(${NODE_ADDON_API_DIR})
include_directories(${CMAKE_SOURCE_DIR}/include)

# --- Platform settings ---
if(WIN32)
  set(PLATFORM "win")
  add_definitions(-DNOMINMAX -DWIN32 -DPLATFORM="win")
  add_definitions(-D_HAS_EXCEPTIONS=1)
  add_definitions(-D_CRT_SECURE_NO_WARNINGS)
  set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
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
find_package(geos CONFIG REQUIRED)
find_package(CURL CONFIG REQUIRED)
find_package(expat CONFIG REQUIRED)
find_package(SQLite3 CONFIG REQUIRED)
find_package(hdf5 CONFIG REQUIRED)
find_package(netcdf-c CONFIG REQUIRED)
find_package(openjpeg CONFIG REQUIRED)
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
  src/gdal_rasterband.cpp
  src/gdal_group.cpp
  src/gdal_mdarray.cpp
  src/gdal_dimension.cpp
  src/gdal_attribute.cpp
  src/gdal_majorobject.cpp
  src/gdal_feature.cpp
  src/gdal_feature_defn.cpp
  src/gdal_field_defn.cpp
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
)

# --- Build the addon ---
add_library(gdal SHARED ${SOURCES})

set_target_properties(gdal PROPERTIES
  PREFIX ""
  SUFFIX ".node"
  CXX_STANDARD 17
  CXX_STANDARD_REQUIRED ON
  CXX_EXTENSIONS ON
)

target_link_libraries(gdal PRIVATE
  GDAL::GDAL
  PROJ::proj
  geos::geos_c
  CURL::libcurl
  expat::expat
  SQLite::SQLite3
  hdf5::hdf5
  netcdf-c::netcdf-c
  openjpeg::openjpeg
  libaec::libaec
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
  target_compile_options(gdal PRIVATE /MP /GR /EHsc /permissive-)
else()
  target_compile_options(gdal PRIVATE -Wno-cast-function-type-mismatch)
endif()

# --- Post-build: yatag + copy to lib/binding ---
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

add_custom_command(TARGET gdal POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_SOURCE_DIR}/${BINDING_DIR}"
  COMMAND ${CMAKE_COMMAND} -E copy "$<TARGET_FILE:gdal>" "${CMAKE_SOURCE_DIR}/${BINDING_DIR}/gdal.node"
  COMMAND npx yatag
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  COMMENT "Copying gdal.node to ${BINDING_DIR} and generating TypeScript declarations"
)

# --- cmake-js final configuration ---
cmake_js_configure_target(gdal)
