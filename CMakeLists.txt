cmake_minimum_required(VERSION 3.21)

project(ObituarySKSE VERSION 1.0.0 LANGUAGES CXX)

# ---------------------------------------------------------------------------
# SKSE plugin skeleton (Skyrim SE/AE, target runtime 1.7.104.0, SKSE 2.3.1).
# Built against a *prebuilt* CommonLibSSE-NG tree on disk. No vcpkg manifest
# mode, no registry, no network: the tree is consumed directly.
# Generator must be "Visual Studio 18 2026" (Ninja deadlocks in mspdbsrv here).
# ---------------------------------------------------------------------------

if(NOT MSVC OR NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
    message(FATAL_ERROR "An SKSE plugin requires MSVC x64.")
endif()

# Match the vcpkg triplet (x64-windows-static-md): dynamic MSVC runtime, static deps.
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreadedDLL")
set(CMAKE_MAP_IMPORTED_CONFIG_DEBUG Release)
set(CMAKE_MAP_IMPORTED_CONFIG_RELWITHDEBINFO Release)
set(CMAKE_MAP_IMPORTED_CONFIG_MINSIZEREL Release)

# Root of the offline CommonLibSSE-NG install tree (vcpkg x64-windows-static-md).
# The default is derived from this file's location, so no non-ASCII literal is
# needed here; pass -DCOMMONLIBSSE_PREFIX=... to point at another tree.
get_filename_component(_ob_default_prefix
    "${CMAKE_CURRENT_SOURCE_DIR}/../vcpkg-install/x64-windows-static-md" ABSOLUTE)
set(COMMONLIBSSE_PREFIX "${_ob_default_prefix}" CACHE PATH
    "Root of the prebuilt CommonLibSSE-NG (x64-windows-static-md) install tree")

set(CLIB_INCLUDE_DIR "${COMMONLIBSSE_PREFIX}/include")
set(CLIB_LIBRARY     "${COMMONLIBSSE_PREFIX}/lib/CommonLibSSE.lib")

if(NOT EXISTS "${CLIB_INCLUDE_DIR}/RE/Skyrim.h" OR NOT EXISTS "${CLIB_LIBRARY}")
    message(FATAL_ERROR
        "CommonLibSSE-NG not found.\n"
        "  include: ${CLIB_INCLUDE_DIR}\n"
        "  library: ${CLIB_LIBRARY}\n"
        "Run build.ps1 from the project root, or pass -DCOMMONLIBSSE_PREFIX=<tree>.")
endif()

message(STATUS "CommonLibSSE-NG include dir : ${CLIB_INCLUDE_DIR}")
message(STATUS "CommonLibSSE-NG library     : ${CLIB_LIBRARY}")

# The NG export lists Microsoft::DirectXTK in its link interface but its config
# never pulls that target in. Point it straight at the same install tree.
if(NOT TARGET Microsoft::DirectXTK)
    add_library(Microsoft::DirectXTK STATIC IMPORTED GLOBAL)
    set_target_properties(Microsoft::DirectXTK PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES
            "${COMMONLIBSSE_PREFIX}/include/directxtk;${COMMONLIBSSE_PREFIX}/include/directxmath"
    )
    # A multi-config generator needs a location for every configuration.
    foreach(_ob_cfg DEBUG RELEASE RELWITHDEBINFO MINSIZEREL)
        set_target_properties(Microsoft::DirectXTK PROPERTIES
            "IMPORTED_LOCATION_${_ob_cfg}" "${COMMONLIBSSE_PREFIX}/lib/DirectXTK.lib")
    endforeach()
endif()

# find_dependency(spdlog) inside CommonLibSSEConfig.cmake resolves through this.
list(PREPEND CMAKE_PREFIX_PATH "${COMMONLIBSSE_PREFIX}")
find_package(CommonLibSSE CONFIG REQUIRED)

# The NG export hardcodes an openvr_api.lib path under D:/VS/vcpkg/buildtrees
# (it is not part of the install prefix). Turn a deleted/moved buildtrees tree
# into an explicit warning instead of a bare LNK1104 at link time.
get_target_property(_ob_clib_link_libs CommonLibSSE::CommonLibSSE INTERFACE_LINK_LIBRARIES)
if(_ob_clib_link_libs)
    foreach(_ob_dep IN LISTS _ob_clib_link_libs)
        if(_ob_dep MATCHES "openvr_api\\.lib$" AND NOT EXISTS "${_ob_dep}")
            message(WARNING
                "CommonLibSSE-NG references an openvr_api.lib that does not exist:\n"
                "  ${_ob_dep}\n"
                "Restore D:/VS/vcpkg/buildtrees/commonlibsse-ng, or link your own openvr_api.lib.")
        endif()
    endforeach()
    unset(_ob_dep)
endif()
unset(_ob_clib_link_libs)

add_library(${PROJECT_NAME} SHARED
    src/plugin.cpp
    src/Config.cpp
    src/Localization.cpp
    src/Obituary.cpp
    src/Notifier.cpp
    src/Menu.cpp
)

set_target_properties(${PROJECT_NAME} PROPERTIES
    OUTPUT_NAME "ObituarySKSE"
    PREFIX ""
)

target_link_libraries(${PROJECT_NAME} PRIVATE CommonLibSSE::CommonLibSSE)

# SKSE Menu Framework 官方头文件（external\SKSEMenuFramework.h）
target_include_directories(${PROJECT_NAME} PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/external"
)

# CommonLibSSE-NG's exported target carries INTERFACE_COMPILE_FEATURES cxx_std_23,
# so CMake would raise a cxx_std_20 request to 23 anyway. State it explicitly.
target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_23)

target_compile_definitions(${PROJECT_NAME} PRIVATE
    _UNICODE
    UNICODE
    NOMINMAX
    WIN32_LEAN_AND_MEAN
    _ITERATOR_DEBUG_LEVEL=0
)

if(MSVC)
    # Sources are UTF-8 with Chinese comments. /utf-8 is not usable here (it also
    # forces the execution charset and trips C4828 on non-ASCII build paths), but
    # telling the compiler the *source* charset keeps comments from mojibaking and
    # silences C4819. Narrow literals stay ASCII, so the execution charset is moot.
    target_compile_options(${PROJECT_NAME} PRIVATE
        /W4
        /permissive-
        /Zc:preprocessor
        /source-charset:utf-8
    )
endif()