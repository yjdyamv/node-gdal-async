cmake_minimum_required(VERSION 3.22)
project(gdal-async LANGUAGES CXX)

# Enable N-API module support
set(CMAKE_JS_NODE_API ON)

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
include_directories(${CMAKE_SOURCE_DIR}/include)

# --- Platform settings ---
if(WIN32)
  set(PLATFORM "win")
  add_definitions(-DNOMINMAX -DWIN32 -DPLATFORM="win")
  add_definitions(-D_HAS_EXCEPTIONS=1)
  add_definitions(-D_CRT_SECURE_NO_WARNINGS)
  # Force dynamic CRT (/MD) to match shared deps; cmake-js defaults to static CRT (/MT)
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

# --- Dependencies ---
find_package(GDAL CONFIG REQUIRED)
find_package(PROJ CONFIG REQUIRED)
find_package(GEOS CONFIG REQUIRED)
find_package(CURL REQUIRED)
find_package(expat REQUIRED)
find_package(SQLite3 REQUIRED)
# HDF5 disabled due to GDAL 3.12.1 Conan recipe incompatibility
# find_package(HDF5 REQUIRED)
find_package(netCDF CONFIG REQUIRED)
find_package(OpenJPEG REQUIRED)
find_package(libaec CONFIG REQUIRED)
find_package(muparser CONFIG REQUIRED)

# --- Sources ---
# All NAN .cpp files replaced by *_napi.cpp equivalents
# node_gdal.cpp fully migrated to N-API (InitNan removed)
set(SOURCES
  src/node_gdal.cpp
  # N-API modules
  src/gdal_dataset_napi.cpp
  src/gdal_driver_napi.cpp
  src/gdal_rasterband_napi.cpp
  src/gdal_multi_napi.cpp
  src/gdal_feature_napi.cpp
  src/gdal_feature_defn_napi.cpp
  src/gdal_field_defn_napi.cpp
  src/geometry/gdal_geometry_napi.cpp
  src/geometry/gdal_point_napi.cpp
  src/geometry/gdal_simplecurve_napi.cpp
  src/geometry/gdal_compoundcurve_napi.cpp
  src/geometry/gdal_linestring_napi.cpp
  src/geometry/gdal_circularstring_napi.cpp
  src/geometry/gdal_linearring_napi.cpp
  src/geometry/gdal_polygon_napi.cpp
  src/geometry/gdal_geometrycollection_napi.cpp
  src/gdal_layer_napi.cpp
  src/gdal_coordinate_transformation_napi.cpp
  src/gdal_spatial_reference_napi.cpp
  src/gdal_algebra_napi.cpp
  src/gdal_memfile_napi.cpp
  src/collections/collections_napi.cpp
  src/gdal_utils_napi.cpp
  src/gdal_algorithms_napi.cpp
  src/gdal_warper_napi.cpp
  src/gdal_vsi_napi.cpp
  src/gdal_stubs_napi.cpp
  src/collections/gdal_drivers_napi.cpp
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
  expat::expat
  SQLite::SQLite3
  # HDF5::HDF5
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
  target_link_options(gdal PRIVATE /NXCOMPAT /DYNAMICBASE /GUARD:CF)
  # Allow multiply-defined symbols: node.exe exports zlib/openssl via node.lib,
  # our static libs provide identical symbols; /FORCE:MULTIPLE uses first definition
  target_link_options(gdal PRIVATE /FORCE:MULTIPLE)
  set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
else()
  target_compile_options(gdal PRIVATE -Wall -Wextra -Wpedantic -Wno-cast-function-type-mismatch)
  # Allow multiply-defined symbols for static libs that overlap with node's exports
  if(APPLE)
    target_link_options(gdal PRIVATE -Wl,-force_multiple)
  else()
    target_link_options(gdal PRIVATE -Wl,--allow-multiple-definition)
  endif()
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

# Collect shared libs from all Conan packages (CONAN_RUNTIME_LIB_DIRS set by toolchain)
set(CONAN_SHARED_LIBS "")
if(DEFINED CONAN_RUNTIME_LIB_DIRS)
  foreach(dir ${CONAN_RUNTIME_LIB_DIRS})
    if(EXISTS "${dir}")
      if(WIN32)
        file(GLOB LIBS "${dir}/*.dll")
      elseif(APPLE)
        file(GLOB LIBS "${dir}/*.dylib")
      else()
        file(GLOB LIBS "${dir}/*.so")
      endif()
      list(APPEND CONAN_SHARED_LIBS ${LIBS})
    endif()
  endforeach()
endif()

add_custom_command(TARGET gdal POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_SOURCE_DIR}/${BINDING_DIR}"
  COMMAND ${CMAKE_COMMAND} -E copy "$<TARGET_FILE:gdal>" "${CMAKE_SOURCE_DIR}/${BINDING_DIR}/gdal.node"
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  COMMENT "Copying gdal.node to ${BINDING_DIR}"
)

# Copy shared libs if any (only needed when some deps are DLLs)
list(LENGTH CONAN_SHARED_LIBS CONAN_SHLIB_COUNT)
if(CONAN_SHLIB_COUNT GREATER 0)
  add_custom_command(TARGET gdal POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CONAN_SHARED_LIBS} "${CMAKE_SOURCE_DIR}/${BINDING_DIR}/"
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Copying ${CONAN_SHLIB_COUNT} shared libs to ${BINDING_DIR}"
  )
endif()

# RPATH for non-Windows platforms so .node finds .so/.dylib relative to itself
if(APPLE)
  set_target_properties(gdal PROPERTIES
    INSTALL_RPATH "@loader_path"
    BUILD_RPATH "${CONAN_RUNTIME_LIB_DIRS}"
  )
elseif(NOT WIN32)
  set_target_properties(gdal PROPERTIES
    INSTALL_RPATH "$ORIGIN"
    BUILD_RPATH "${CONAN_RUNTIME_LIB_DIRS}"
  )
endif()

# --- cmake-js Node.js integration ---
target_link_libraries(gdal PRIVATE ${CMAKE_JS_LIB})
